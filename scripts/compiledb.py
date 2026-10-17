"""Portable PlatformIO compilation databases for applications and library projects.

Use: extra_scripts = pre:scripts/compiledb.py
Outputs: <project>/compile_commands.json and
         <configured build_dir>/compiledb/<environment>/compile_commands.json.
No assumptions are made about source/library directories, framework or compiler.
"""

from contextlib import contextmanager
import errno
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import sys
import tempfile
import time
import uuid

SCHEMA = 1
DRIVER = re.compile(r"^\s*(?:\"([^\"]+)\"|'([^']+)'|(\S+))(?:\s+(.*))?$", re.DOTALL)
EXCLUDED_TARGETS = {
  "clean", "cleanall", "fullclean", "buildfs", "uploadfs", "uploadfsota", "envdump",
  "nobuild", "sizedata", "idedata", "__idedata", "monitor",
}


def warn(message):
  print(f"[compiledb] WARNING: {message}", file=sys.stderr)


def path_key(path):
  return os.path.normcase(str(Path(path).resolve()))


def load_json(path):
  try:
    return json.loads(Path(path).read_text(encoding="utf-8-sig"))
  except (OSError, ValueError) as error:
    warn(f"Cannot read {path}: {error}")
    return None


def atomic_write(path, data):
  """Replace complete UTF-8 JSON; unchanged data keeps its timestamp."""
  path = Path(path)
  content = (json.dumps(data, ensure_ascii=False, indent=2) + "\n").encode("utf-8")
  try:
    if path.read_bytes() == content:
      return False
  except FileNotFoundError:
    pass
  path.parent.mkdir(parents=True, exist_ok=True)
  temporary = None
  try:
    with tempfile.NamedTemporaryFile(prefix=path.name + ".tmp.", dir=path.parent, delete=False) as stream:
      temporary = Path(stream.name)
      stream.write(content)
      stream.flush()
    # Windows indexers/antivirus can temporarily hold the destination open.
    for attempt in range(6):
      try:
        os.replace(temporary, path)
        return True
      except PermissionError:
        if attempt == 5:
          raise
        time.sleep(0.05 * (attempt + 1))
  finally:
    if temporary is not None:
      temporary.unlink(missing_ok=True)


@contextmanager
def publication_lock(project, timeout=10):
  """Serialize publishers across processes; the OS releases locks on exit."""
  digest = hashlib.sha256(os.fsencode(path_key(project))).hexdigest()
  lock_path = Path(tempfile.gettempdir()) / "platformio-compiledb" / (digest + ".lock")
  lock_path.parent.mkdir(parents=True, exist_ok=True)
  with lock_path.open("a+b") as stream:
    if stream.tell() == 0:
      stream.write(b"0")
      stream.flush()
    deadline = time.monotonic() + timeout
    while True:
      try:
        stream.seek(0)
        if os.name == "nt":
          import msvcrt
          msvcrt.locking(stream.fileno(), msvcrt.LK_NBLCK, 1)
        else:
          import fcntl
          fcntl.flock(stream.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)
        break
      except OSError as error:
        if error.errno not in (errno.EACCES, errno.EAGAIN, errno.EDEADLK):
          raise
        if time.monotonic() >= deadline:
          raise TimeoutError("Another compilation database publisher is still running") from error
        time.sleep(0.05)
    try:
      yield
    finally:
      stream.seek(0)
      if os.name == "nt":
        import msvcrt
        msvcrt.locking(stream.fileno(), msvcrt.LK_UNLCK, 1)
      else:
        import fcntl
        fcntl.flock(stream.fileno(), fcntl.LOCK_UN)


def resolve_driver(driver, directory, where_is):
  path = Path(driver)
  if path.is_absolute():
    resolved = path
  elif path.parent != Path("."):
    resolved = directory / path
  else:
    found = where_is(driver)
    if not found:
      raise ValueError(f"Compiler is absent from PlatformIO PATH: {driver}")
    resolved = Path(found)
  resolved = resolved.resolve()
  if not resolved.is_file():
    raise ValueError(f"Compiler does not exist: {resolved}")
  return resolved


def normalize_entry(entry, project, where_is):
  """Preserve arguments/response files; resolve only paths we can identify."""
  if not isinstance(entry, dict) or not isinstance(entry.get("file"), str) or not entry["file"]:
    raise ValueError("Compilation entry has no source filename")
  directory = entry.get("directory") or str(project)
  if not isinstance(directory, str):
    raise ValueError("Compilation entry has an invalid working directory")
  directory = (project / directory).resolve()
  source = (directory / entry["file"]).resolve()
  if not source.is_file():
    raise ValueError(f"Source file does not exist: {source}")
  normalized = entry.copy()
  normalized.update(directory=str(directory), file=str(source))
  arguments = entry.get("arguments")
  if arguments is not None:
    if not isinstance(arguments, list) or not arguments or not all(isinstance(a, str) for a in arguments):
      raise ValueError("Compilation arguments must be a nonempty string array")
    normalized["arguments"] = [str(resolve_driver(arguments[0], directory, where_is))] + arguments[1:]
    normalized.pop("command", None)
  else:
    command = entry.get("command")
    match = DRIVER.match(command) if isinstance(command, str) else None
    if match is None:
      raise ValueError("Compilation entry has no valid command or arguments")
    driver = next(value for value in match.groups()[:3] if value is not None)
    compiler = resolve_driver(driver, directory, where_is).as_posix()
    # POSIX quoting must also protect dollar signs/backticks in user paths.
    quoted = ('"' + compiler + '"') if os.name == "nt" else shlex.quote(compiler)
    normalized["command"] = quoted + " " + (match.group(4) or "").rstrip()
  if isinstance(entry.get("output"), str):
    normalized["output"] = str((directory / entry["output"]).resolve())
  return path_key(source), normalized


def collect_entries(data, project, where_is, strict=False):
  if not isinstance(data, list) or not data:
    raise ValueError("Compilation database is not a nonempty JSON array")
  result = {}
  for entry in data:
    try:
      key, normalized = normalize_entry(entry, project, where_is)
      result[key] = normalized
    except (OSError, ValueError, RuntimeError) as error:
      if strict:
        raise ValueError(f"Cannot publish current compilation database: {error}") from error
      warn(f"Skipping stale/invalid cached command: {error}")
  return result


def publish_database(build_env, raw_path, database_root, context):
  """Only this environment uses its toolchain PATH; cached commands stay absolute."""
  project = Path(build_env.subst("$PROJECT_DIR")).resolve()
  active = str(build_env["PIOENV"])
  valid_envs = set(build_env.GetProjectConfig().envs())
  current = collect_entries(load_json(raw_path), project, build_env.WhereIs, strict=True)
  # No publication happens if current generation is empty, malformed, or unresolved.
  with publication_lock(project):
    merged = {}
    for folder in sorted(database_root.iterdir()):
      if not folder.is_dir() or folder.name == active or folder.name not in valid_envs:
        continue
      metadata_path = folder / "context.json"
      if not metadata_path.is_file():
        continue  # Unversioned caches from old scripts must be regenerated once.
      if load_json(metadata_path) != dict(context, environment=folder.name):
        continue
      cached = load_json(folder / "compile_commands.json")
      if not isinstance(cached, list) or not cached:
        continue
      # Never resolve an old environment's compiler with the active toolchain.
      merged.update(collect_entries(cached, project, lambda _: None))
    merged.update(current)
    current_dir = database_root / active
    atomic_write(current_dir / "compile_commands.json", [v for _, v in sorted(current.items())])
    atomic_write(current_dir / "context.json", dict(context, environment=active))
    entries = [value for _, value in sorted(merged.items())]
    atomic_write(database_root / "compile_commands.json", entries)
    changed = atomic_write(project / "compile_commands.json", entries)
    if changed:
      print(f"[compiledb] Updated {len(entries)} commands (active: {active})")


def configure(build_env):
  """Called as a PRE project extra_script, before source objects are created."""
  from SCons.Script import Action, COMMAND_LINE_TARGETS

  targets = set(COMMAND_LINE_TARGETS)
  explicit = "compiledb" in targets
  if build_env.IsCleanTarget() or build_env.IsIntegrationDump():
    return
  if not explicit and targets & EXCLUDED_TARGETS:
    return
  if build_env.get("_COMPILEDB_CONFIGURED"):
    return
  build_env["_COMPILEDB_CONFIGURED"] = True
  project = Path(build_env.subst("$PROJECT_DIR")).resolve()
  build_root = Path(build_env.subst("$PROJECT_BUILD_DIR"))
  if not build_root.is_absolute():
    build_root = project / build_root
  database_root = build_root.resolve() / "compiledb"
  active_dir = database_root / str(build_env["PIOENV"])
  active_dir.mkdir(parents=True, exist_ok=True)
  # Different processes building the same environment cannot overwrite raw JSON.
  raw_path = active_dir / ("commands.raw." + uuid.uuid4().hex + ".json")
  config = build_env.GetProjectConfig()
  context = {
    "schema": SCHEMA,
    "project": path_key(project),
    "configuration": hashlib.sha256(config.to_json().encode("utf-8")).hexdigest(),
  }

  def after_database(target, source, env):
    del target, source
    try:
      publish_database(env, raw_path, database_root, context)
      return 0
    except (OSError, ValueError, RuntimeError) as error:
      warn(f"Database update failed; existing root database was kept: {error}")
      # An explicit request must report failure. Optional editor data must not
      # prevent firmware compilation/upload when its storage is unavailable.
      return 1 if explicit else 0
    finally:
      try:
        raw_path.unlink(missing_ok=True)
      except OSError as error:
        warn(f"Cannot remove temporary database {raw_path}: {error}")

  if not explicit:
    build_env.Tool("compilation_db")
    # Some platforms (notably native) reload their compiler tools after PRE
    # scripts, replacing the source emitters installed by compilation_db.
    # The documented source middleware runs after tool setup, before objects.
    collector_builders = set()
    def ensure_collector(source_env, node):
      builder = source_env["BUILDERS"].get("StaticObject")
      identity = id(builder)
      if identity not in collector_builders:
        source_env.Tool("compilation_db")
        collector_builders.add(identity)
      return node
    build_env.AddBuildMiddleware(ensure_collector)
  build_env.Replace(COMPILATIONDB_PATH=str(raw_path), COMPILATIONDB_USE_ABSPATH=True)
  callback = Action(after_database, "[compiledb] Publish compilation databases")
  if explicit:
    # PlatformIO creates the actual database builder after POST scripts.
    build_env.AddPostAction(build_env.Alias("compiledb"), callback)
  else:
    database = build_env.CompilationDatabase(str(raw_path))
    build_env.AddPostAction(database, callback)
    if not targets:
      build_env.Default(database)
    # Explicit build/upload/test aliases also refresh DB, including no-op builds.
    # Use order/dependency edges, not alias sources: upload tools may use
    # $SOURCE/$SOURCES as the firmware input and must never receive a JSON file.
    for alias in ("buildprog", "upload", "program", "__test", "__debug", "exec"):
      build_env.Depends(build_env.Alias(alias), database)


# Importable with ordinary Python for regression tests; SCons supplies Import.
try:
  _import_environment = Import
except NameError:
  _import_environment = None
if _import_environment is not None:
  _import_environment("env")
  configure(env)
