"""Opt-in real PlatformIO builds; run with NBED_RUN_PLATFORMIO_TESTS=1."""

import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

SCRIPT = Path(__file__).resolve().parents[1] / "scripts/compiledb.py"


@unittest.skipUnless(os.environ.get("NBED_RUN_PLATFORMIO_TESTS") == "1", "opt-in PlatformIO integration tests")
class PlatformIOTests(unittest.TestCase):
  def setUp(self):
    self.temp = tempfile.TemporaryDirectory(prefix="PIO space ")
    self.addCleanup(self.temp.cleanup)
    self.root = Path(self.temp.name).resolve()
    self.pio = shutil.which("pio")
    self.assertIsNotNone(self.pio)
    (self.root / "scripts").mkdir()
    shutil.copy2(SCRIPT, self.root / "scripts/compiledb.py")

  def write(self, name, value):
    path = self.root / name
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(value, encoding="utf-8")

  def fixture(self, library=False):
    source = "examples" if library else "src"
    dependencies = "lib_deps = MathFixture=symlink://.\n" if library else ""
    self.write("platformio.ini", f"""[platformio]
+default_envs = alpha
+build_dir = artifacts/custom build
+src_dir = {source}
+[env]
+platform = native@1.2.1
+extra_scripts = pre:scripts/compiledb.py
+{dependencies}
+[env:alpha]
+build_flags = -DCONFIG_VARIANT=1
+build_src_filter = -<*> +<alpha/>
+[env:beta]
+build_flags = -DCONFIG_VARIANT=2
+build_src_filter = -<*> +<beta/>
+""".replace("\n+", "\n"))
    header = "#pragma once\nint fixture_value();\n"
    implementation = '#include "mathfixture.h"\nint fixture_value() { return CONFIG_VARIANT; }\n'
    if library:
      self.write("library.json", json.dumps({"name": "MathFixture", "version": "1.0.0"}))
      self.write("include/mathfixture.h", header)
      self.write("src/mathfixture.cpp", implementation)
    else:
      self.write("lib/MathFixture/src/mathfixture.h", header)
      self.write("lib/MathFixture/src/mathfixture.cpp", implementation)
    main = '#include <chrono>\n#include "mathfixture.h"\nint main() { return fixture_value() - CONFIG_VARIANT + int(std::chrono::seconds(0).count()); }\n'
    for environment in ["alpha", "beta"]:
      self.write(f"{source}/{environment}/main.cpp", main)

  def run_pio(self, *arguments):
    result = subprocess.run([self.pio, "run", *arguments], cwd=self.root, capture_output=True, encoding="utf-8", errors="replace", timeout=120)
    self.assertEqual(result.returncode, 0, result.stdout[-8000:] + result.stderr[-2000:])
    self.assertNotIn("[compiledb] WARNING", result.stdout + result.stderr)
    return result

  def database(self):
    data = json.loads((self.root / "compile_commands.json").read_text(encoding="utf-8"))
    self.assertTrue(data)
    self.assertEqual(data, json.loads((self.root / "artifacts/custom build/compiledb/compile_commands.json").read_text(encoding="utf-8")))
    for entry in data:
      self.assertTrue(Path(entry["file"]).is_absolute())
      self.assertTrue(Path(entry["file"]).is_file())
    return data

  def test_conventional_application_and_noop_generation(self):
    self.fixture()
    self.run_pio()
    data = self.database()
    self.assertTrue(any("lib/MathFixture" in e["file"].replace("\\", "/") for e in data))
    root_db = self.root / "compile_commands.json"
    contents, stamp = root_db.read_bytes(), root_db.stat().st_mtime_ns
    self.run_pio("-e", "alpha", "-t", "compiledb")
    self.assertEqual(root_db.read_bytes(), contents)
    self.assertEqual(root_db.stat().st_mtime_ns, stamp)
    self.run_pio("-e", "alpha", "-t", "buildprog")
    self.assertEqual(root_db.read_bytes(), contents)
    self.assertEqual(root_db.stat().st_mtime_ns, stamp)
    self.run_pio("-e", "alpha", "-t", "clean")
    self.assertEqual(root_db.read_bytes(), contents)

  def test_library_layout_and_environment_priority(self):
    self.fixture(library=True)
    self.run_pio("-e", "alpha", "-e", "beta")
    data = self.database()
    library = next(e for e in data if e["file"].endswith("mathfixture.cpp"))
    self.assertIn("CONFIG_VARIANT=2", library["command"])
    self.assertEqual(len([e for e in data if e["file"].endswith("main.cpp")]), 2)
    self.run_pio("-e", "alpha", "-t", "compiledb")
    library = next(e for e in self.database() if e["file"].endswith("mathfixture.cpp"))
    self.assertIn("CONFIG_VARIANT=1", library["command"])

  def test_concurrent_publishers_keep_both_environments(self):
    self.fixture(library=True)
    self.run_pio("-e", "alpha", "-e", "beta", "-t", "compiledb")
    processes = [subprocess.Popen([self.pio, "run", "-e", name, "-t", "compiledb"], cwd=self.root, stdout=subprocess.PIPE, stderr=subprocess.PIPE) for name in ["alpha", "beta"]]
    for process in processes:
      stdout, stderr = process.communicate(timeout=120)
      self.assertEqual(process.returncode, 0, (stdout + stderr).decode("utf-8", errors="replace")[-6000:])
    self.assertEqual(len([e for e in self.database() if e["file"].endswith("main.cpp")]), 2)
    self.assertEqual(list((self.root / "artifacts/custom build/compiledb").rglob("commands.raw.*.json")), [])

  def test_unicode_library_project_database(self):
    original = self.root
    self.root = original / "日本語 project"
    (self.root / "scripts").mkdir(parents=True)
    shutil.copy2(SCRIPT, self.root / "scripts/compiledb.py")
    self.fixture(library=True)
    self.run_pio("-e", "alpha", "-t", "compiledb")
    self.assertTrue(any("日本語" in entry["file"] for entry in self.database()))

  def test_upload_alias_generates_database(self):
    self.fixture()
    # native's upload alias executes this tiny fixture; no hardware is accessed.
    self.run_pio("-e", "alpha", "-t", "upload")
    self.assertTrue(self.database())

  def test_default_platformio_build_directory(self):
    self.fixture()
    config = self.root / "platformio.ini"
    config.write_text(config.read_text(encoding="utf-8").replace("build_dir = artifacts/custom build\n", ""), encoding="utf-8")
    self.run_pio("-e", "alpha", "-t", "compiledb")
    self.assertTrue((self.root / ".pio/build/compiledb/alpha/compile_commands.json").is_file())
    self.assertTrue(json.loads((self.root / "compile_commands.json").read_text(encoding="utf-8")))

  def test_absolute_build_directory_outside_project(self):
    self.fixture()
    with tempfile.TemporaryDirectory(prefix="PIO outputs ") as output:
      config = self.root / "platformio.ini"
      config.write_text(config.read_text(encoding="utf-8").replace("artifacts/custom build", Path(output).as_posix()), encoding="utf-8")
      self.run_pio("-e", "alpha", "-t", "compiledb")
      self.assertTrue((Path(output) / "compiledb/alpha/compile_commands.json").is_file())
      self.assertTrue(json.loads((self.root / "compile_commands.json").read_text(encoding="utf-8")))

  def test_debug_build_alias_generates_database(self):
    self.fixture()
    # __debug builds its dependencies; this does not launch a debugger.
    self.run_pio("-e", "alpha", "-t", "__debug")
    self.assertTrue(self.database())


if __name__ == "__main__":
  unittest.main()
