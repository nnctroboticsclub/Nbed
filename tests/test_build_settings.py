"""Verify MCU-specific settings in the library's PlatformIO hook."""

from pathlib import Path
import runpy
import unittest
from types import SimpleNamespace

ROOT = Path(__file__).resolve().parents[1]


class BuildEnvironment(dict):
  def __init__(self, mcu):
    super().__init__()
    self.mcu = mcu
    self.flags = []
    self.unflags = []
    self.includes = []
    self.dependencies = []

  def BoardConfig(self):
    return {"build.mcu": self.mcu}

  def ProcessFlags(self, flags):
    self.flags.extend(flags)

  def ProcessUnFlags(self, flag):
    self.unflags.append(flag)

  def Prepend(self, CPPPATH):
    self.includes.extend(CPPPATH)

  def Replace(self, **kwargs):
    self.update(kwargs)

  def Depends(self, target, dependency):
    self.dependencies.append((target, dependency))


def configure(mcu):
  environments = [BuildEnvironment(mcu) for _ in range(3)]
  result = runpy.run_path(str(ROOT / "extra_script.py"), init_globals={
      "env": environments[0], "projenv": environments[1],
      "pio_lib_builder": SimpleNamespace(path=str(ROOT)),
      "Import": lambda *args: None,
      "DefaultEnvironment": lambda: environments[2],
  })
  return environments, result


class BuildSettingsTests(unittest.TestCase):
  def test_supported_mcus(self):
    for mcu, family, linker, optimization in (
        ("stm32f446ret6", "f4", "STM32F446RETX_FLASH.ld", "-O3"),
        ("stm32f303k8t6", "f3", "STM32F303K8TX_FLASH.ld", "-Os")):
      with self.subTest(mcu=mcu):
        environments, result = configure(mcu)
        self.assertEqual(environments[0]["SRC_FILTER"], ["-<*>", f"+<{family}/>"])
        for environment in environments:
          self.assertIn(optimization, environment.flags)
          self.assertIn("-std=gnu++23", environment.flags)
          self.assertIn("-flto", environment.flags)
          self.assertIn("-Os", environment.unflags)
          self.assertEqual(environment.includes, [str(ROOT / "include")])
        self.assertEqual(Path(environments[2]["LDSCRIPT_PATH"]).name, linker)
        self.assertTrue(Path(environments[2]["LDSCRIPT_PATH"]).is_file())
        self.assertEqual(len(environments[2].dependencies), 1)
        self.assertEqual(result["FAMILY"], family)

  def test_unsupported_mcu_is_rejected(self):
    with self.assertRaisesRegex(ValueError, "does not support MCU"):
      configure("stm32f767zit6")


if __name__ == "__main__":
  unittest.main()
