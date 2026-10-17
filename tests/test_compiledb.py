"""Regression tests for the reusable PlatformIO DB script (standard library only)."""

import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

SCRIPT = Path(__file__).resolve().parents[1] / "scripts" / "compiledb.py"
spec = importlib.util.spec_from_file_location("compiledb", SCRIPT)
db = importlib.util.module_from_spec(spec)
spec.loader.exec_module(db)


class FakeConfig:
  def envs(self):
    return ["alpha", "beta"]


class FakeEnvironment(dict):
  def __init__(self, project, active):
    super().__init__(PIOENV=active)
    self.project = project

  def subst(self, text):
    return str(self.project) if text == "$PROJECT_DIR" else text

  def GetProjectConfig(self):
    return FakeConfig()

  def WhereIs(self, driver):
    return str(self.project / "tool chain" / driver)


class DatabaseTests(unittest.TestCase):
  def setUp(self):
    self.temp = tempfile.TemporaryDirectory(prefix="compiledb 日本語 ")
    self.addCleanup(self.temp.cleanup)
    self.root = Path(self.temp.name).resolve()
    self.compiler = self.root / "tool chain" / "compiler.exe"
    self.compiler.parent.mkdir()
    self.compiler.write_bytes(b"fixture")
    self.source = self.root / "source space.cpp"
    self.source.write_text("int example;", encoding="utf-8")
    self.env = FakeEnvironment(self.root, "alpha")
    self.context = {"schema": db.SCHEMA, "project": db.path_key(self.root), "configuration": "test"}

  def entry(self, source=None):
    return {"file": str(source or self.source), "directory": str(self.root),
            "command": 'compiler.exe -DVALUE=1 @"response file.rsp" -c "source space.cpp"'}

  def publish(self, active="alpha", entry=None):
    self.env["PIOENV"] = active
    storage = self.root / "custom outputs" / "compiledb"
    raw = storage / active / "raw.json"
    db.atomic_write(raw, [entry or self.entry()])
    db.publish_database(self.env, raw, storage, self.context)
    return storage

  def test_relative_compiler_and_unicode_path(self):
    key, value = db.normalize_entry(self.entry(), self.root, self.env.WhereIs)
    self.assertEqual(key, db.path_key(self.source))
    self.assertIn(self.compiler.as_posix(), value["command"])
    self.assertIn('@"response file.rsp"', value["command"])

  def test_double_quoted_compiler(self):
    entry = self.entry()
    entry["command"] = f'"{self.compiler}" -c test.cpp'
    self.assertIn(self.compiler.as_posix(), db.normalize_entry(entry, self.root, self.env.WhereIs)[1]["command"])

  def test_single_quoted_compiler(self):
    entry = self.entry()
    entry["command"] = f"'{self.compiler}' -c test.cpp"
    self.assertIn(self.compiler.as_posix(), db.normalize_entry(entry, self.root, self.env.WhereIs)[1]["command"])

  def test_arguments_array_is_preserved(self):
    entry = self.entry()
    entry["arguments"] = ["compiler.exe", "-DMSG=a b", "@response file.rsp"]
    value = db.normalize_entry(entry, self.root, self.env.WhereIs)[1]
    self.assertNotIn("command", value)
    self.assertEqual(value["arguments"], [str(self.compiler), "-DMSG=a b", "@response file.rsp"])

  def test_relative_source_and_output(self):
    entry = self.entry()
    entry.update(file=self.source.name, directory=".", output="objects/test.o")
    value = db.normalize_entry(entry, self.root, self.env.WhereIs)[1]
    self.assertEqual(value["file"], str(self.source))
    self.assertEqual(value["output"], str(self.root / "objects/test.o"))

  def test_missing_compiler_is_visible(self):
    with self.assertRaisesRegex(ValueError, "Compiler"):
      db.normalize_entry(self.entry(), self.root, lambda _: None)

  def test_missing_source_is_rejected(self):
    with self.assertRaisesRegex(ValueError, "Source file"):
      db.normalize_entry(self.entry(self.root / "absent.cpp"), self.root, self.env.WhereIs)

  def test_malformed_entries_are_rejected(self):
    for entry in [None, {}, {"file": "a", "arguments": []}, dict(self.entry(), arguments=[1])]:
      with self.subTest(entry=entry), self.assertRaises(ValueError):
        db.normalize_entry(entry, self.root, self.env.WhereIs)

  def test_utf8_bom_is_accepted(self):
    path = self.root / "bom.json"
    path.write_text('[{"file":"日本語"}]', encoding="utf-8-sig")
    self.assertEqual(db.load_json(path)[0]["file"], "日本語")

  def test_malformed_json_is_reported(self):
    path = self.root / "broken.json"
    path.write_text("[", encoding="utf-8")
    with mock.patch.object(db, "warn") as warning:
      self.assertIsNone(db.load_json(path))
      warning.assert_called_once()

  def test_unchanged_output_keeps_mtime(self):
    path = self.root / "nested/db.json"
    self.assertTrue(db.atomic_write(path, [self.entry()]))
    stamp = path.stat().st_mtime_ns
    self.assertFalse(db.atomic_write(path, [self.entry()]))
    self.assertEqual(stamp, path.stat().st_mtime_ns)

  def test_failed_replace_keeps_valid_output_and_cleans_temporary(self):
    path = self.root / "db.json"
    db.atomic_write(path, ["old"])
    with mock.patch.object(db.os, "replace", side_effect=OSError("storage failed")):
      with self.assertRaises(OSError):
        db.atomic_write(path, ["new"])
    self.assertEqual(db.load_json(path), ["old"])
    self.assertEqual(list(self.root.glob("db.json.tmp.*")), [])

  def test_windows_transient_permission_failure_is_retried(self):
    path = self.root / "db.json"
    replace = db.os.replace
    attempts = []
    def transient(source, destination):
      attempts.append(1)
      if len(attempts) == 1:
        raise PermissionError()
      return replace(source, destination)
    with mock.patch.object(db.os, "replace", side_effect=transient) as action:
      with mock.patch.object(db.time, "sleep"):
        db.atomic_write(path, [])
      self.assertEqual(action.call_count, 2)
    self.assertEqual(db.load_json(path), [])

  def test_active_environment_wins(self):
    self.publish("alpha")
    entry = self.entry()
    entry["command"] = entry["command"].replace("VALUE=1", "VALUE=2")
    self.publish("beta", entry)
    data = db.load_json(self.root / "compile_commands.json")
    self.assertEqual(len(data), 1)
    self.assertIn("VALUE=2", data[0]["command"])

  def test_wrong_configuration_cache_is_not_merged(self):
    other = self.root / "other.cpp"
    other.write_text("int other;", encoding="utf-8")
    storage = self.publish("beta", self.entry(other))
    db.atomic_write(storage / "beta/context.json", dict(self.context, environment="beta", configuration="old"))
    self.publish("alpha")
    self.assertEqual(len(db.load_json(self.root / "compile_commands.json")), 1)

  def test_foreign_project_cache_is_not_merged(self):
    storage = self.publish("beta")
    db.atomic_write(storage / "beta/context.json", dict(self.context, environment="beta", project="elsewhere"))
    self.publish("alpha")
    self.assertIn("VALUE=1", db.load_json(self.root / "compile_commands.json")[0]["command"])

  def test_broken_inactive_cache_does_not_block_current(self):
    storage = self.publish("beta")
    (storage / "beta/compile_commands.json").write_text("invalid", encoding="utf-8")
    with mock.patch.object(db, "warn"):
      self.publish("alpha")
    self.assertEqual(len(db.load_json(self.root / "compile_commands.json")), 1)

  def test_broken_current_does_not_replace_root(self):
    storage = self.publish()
    before = (self.root / "compile_commands.json").read_bytes()
    raw = storage / "alpha/raw.json"
    raw.write_text("[]", encoding="utf-8")
    with self.assertRaises(ValueError):
      db.publish_database(self.env, raw, storage, self.context)
    self.assertEqual(before, (self.root / "compile_commands.json").read_bytes())

  def test_stale_cached_source_is_removed(self):
    other = self.root / "gone.cpp"
    other.write_text("int other;", encoding="utf-8")
    self.publish("beta", self.entry(other))
    other.unlink()
    with mock.patch.object(db, "warn"):
      self.publish("alpha")
    self.assertEqual(len(db.load_json(self.root / "compile_commands.json")), 1)

  def test_removed_environment_is_not_merged(self):
    storage = self.publish("beta")
    (storage / "beta").rename(storage / "removed")
    self.publish("alpha")
    self.assertEqual(len(db.load_json(self.root / "compile_commands.json")), 1)

  def test_lock_times_out_for_a_competing_process(self):
    code = "\n".join([
      "import importlib.util, pathlib, sys",
      "s=importlib.util.spec_from_file_location('db',sys.argv[1])",
      "m=importlib.util.module_from_spec(s)",
      "s.loader.exec_module(m)",
      "with m.publication_lock(pathlib.Path(sys.argv[2]), timeout=0.1):",
      "  pass",
    ])
    with db.publication_lock(self.root):
      result = subprocess.run([sys.executable, "-c", code, str(SCRIPT), str(self.root)], capture_output=True)
    self.assertNotEqual(result.returncode, 0)
    self.assertIn(b"TimeoutError", result.stderr)

  def test_cached_environment_keeps_its_own_compiler(self):
    other_source = self.root / "beta.cpp"
    other_source.write_text("int beta;", encoding="utf-8")
    other_compiler = self.root / "other toolchain" / "compiler.exe"
    other_compiler.parent.mkdir()
    other_compiler.write_bytes(b"other")
    self.env.WhereIs = lambda _: str(other_compiler)
    self.publish("beta", self.entry(other_source))
    self.env.WhereIs = lambda _: str(self.compiler)
    self.publish("alpha")
    data = db.load_json(self.root / "compile_commands.json")
    beta = next(e for e in data if e["file"] == str(other_source))
    alpha = next(e for e in data if e["file"] == str(self.source))
    self.assertIn(other_compiler.as_posix(), beta["command"])
    self.assertIn(self.compiler.as_posix(), alpha["command"])


if __name__ == "__main__":
  unittest.main()
