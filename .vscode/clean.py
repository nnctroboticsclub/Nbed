"""Remove only this template's generated outputs, without following external links."""

from pathlib import Path
import shutil


def clean(project):
  project = project.resolve()
  if not (project / "platformio.ini").is_file():
    raise RuntimeError(f"Not a PlatformIO project: {project}")

  # Fixed paths only: never search recursively for directories named build/cache.
  targets = [project / name for name in (
    "build", ".cache", ".pio/build", "compile_commands.json",
  )]
  targets.extend(project.glob("compile_commands.json.tmp.*"))
  # Validate every absolute destination before deleting any file or directory.
  for target in targets:
    resolved = target.resolve()
    if resolved == project or project not in resolved.parents:
      raise RuntimeError(f"Refusing to remove a path outside the project: {target}")

  for target in targets:
    if target.is_symlink():
      target.unlink()
    elif target.is_dir():
      shutil.rmtree(target)
    else:
      target.unlink(missing_ok=True)
    print(f"[clean] {target}")


if __name__ == "__main__":
  clean(Path(__file__).resolve().parent.parent)
