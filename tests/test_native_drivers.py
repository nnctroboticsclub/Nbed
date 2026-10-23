"""Run driver logic against a tiny host HAL, without attached hardware."""

from pathlib import Path
import os
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class NativeDriverTests(unittest.TestCase):
  def run_family(self, family, macro):
    compiler = shutil.which(os.environ.get("CXX", "g++"))
    if compiler is None:
      self.skipTest("Set CXX to a host C++23 compiler, or install g++")
    with tempfile.TemporaryDirectory(prefix="nbed-native-") as temporary:
      directory = Path(temporary)
      for header in (f"stm32{family}xx.h", f"stm32{family}xx_hal.h",
                     f"stm32{family}xx_hal_gpio.h", f"stm32{family}xx_hal_gpio_ex.h",
                     f"stm32{family}xx_hal_rcc.h", f"stm32{family}xx_hal_uart.h"):
        (directory / header).write_text('#include "hal_stubs.h"\n', encoding="utf-8")
      executable = directory / f"{family}-test.exe"
      sources = [ROOT / "src" / family / f"{name}.cc" for name in
                 ("gpio", "pin_mapper", "time", "timer_reservation", "uart")]
      command = [compiler, "-std=c++23", "-Wall", "-Wextra", "-Werror", f"-D{macro}",
                 "-I", str(directory), "-I", str(ROOT / "tests/native"),
                 "-I", str(ROOT / "include"), str(ROOT / "tests/native/drivers_test.cc"),
                 *map(str, sources), "-o", str(executable)]
      result = subprocess.run(command, capture_output=True, text=True)
      self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
      try:
        result = subprocess.run([str(executable)], capture_output=True, text=True)
      except OSError as error:
        if getattr(error, "winerror", None) == 4551:
          self.skipTest(f"{family}: compiled (including static assertions); "
                        "Windows application control blocked execution (4551)")
        raise
      self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

  def test_f3_drivers(self):
    self.run_family("f3", "STM32F303x8")

  def test_f4_drivers(self):
    self.run_family("f4", "STM32F446xx")


if __name__ == "__main__":
  unittest.main()
