"""Apply MCU-specific build settings carried by the Nbed library."""

from os.path import join, realpath

# SCons supplies Import/DefaultEnvironment and imports these globals at runtime.
Import("env", "projenv", "pio_lib_builder")  # pyright: ignore[reportUndefinedVariable]


LIBRARY_DIR = realpath(pio_lib_builder.path)  # pyright: ignore[reportUndefinedVariable]
INCLUDE_DIR = join(LIBRARY_DIR, "include")
MCU = env.BoardConfig().get("build.mcu", "").lower()  # pyright: ignore[reportUndefinedVariable]
LINKER_SCRIPTS = {
  "stm32f446ret6": "STM32F446RETX_FLASH.ld",
  "stm32f303k8t6": "STM32F303K8TX_FLASH.ld",
  "stm32f767zit6": "STM32F767ZITX_FLASH.ld",
}
if MCU not in LINKER_SCRIPTS:
  raise ValueError("Nbed does not support MCU: " + MCU)
LDSCRIPT_PATH = join(LIBRARY_DIR, "linker", LINKER_SCRIPTS[MCU])
FAMILY = {"stm32f303k8t6": "f3", "stm32f446ret6": "f4",
          "stm32f767zit6": "f7"}[MCU]
# Select only this MCU's implementation. This also keeps clangd's compile
# database from replacing F3 entries with empty F4 compilations, or vice versa.
env.Replace(SRC_FILTER=["-<*>", "+<" + FAMILY + "/>"])  # pyright: ignore[reportUndefinedVariable]

BUILD_FLAGS = [
  "-Os" if MCU == "stm32f303k8t6" else "-O3",
  "-flto",
  "-ffunction-sections",
  "-fdata-sections",
  "-std=gnu++23",
  "-Wall",
  "-Wextra",
  "-Wl,-u,_printf_float",
  "-Wl,--gc-sections",
]

UNFLAGS = [
  "-Os",
  "-std=gnu11",
  "-std=gnu++14",
]


def configure_build_environment(build_env):
  """Make one PlatformIO build environment use the Nbed defaults."""
  for flag in UNFLAGS:
    build_env.ProcessUnFlags(flag)

  build_env.ProcessFlags(BUILD_FLAGS)
  build_env.Prepend(CPPPATH=[INCLUDE_DIR])


# The library environment builds Nbed itself. The global and project
# environments make the same settings available to the HAL and application.
configure_build_environment(env)  # pyright: ignore[reportUndefinedVariable]
configure_build_environment(projenv)  # pyright: ignore[reportUndefinedVariable]

global_env = DefaultEnvironment()  # pyright: ignore[reportUndefinedVariable]
configure_build_environment(global_env)
global_env.Replace(LDSCRIPT_PATH=LDSCRIPT_PATH)
global_env.Depends("$BUILD_DIR/$PROGNAME$PROGSUFFIX", LDSCRIPT_PATH)
