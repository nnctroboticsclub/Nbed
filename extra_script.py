"""Apply the NUCLEO-F446RE build settings carried by the Nbed library."""

from os.path import join, realpath

# SCons supplies Import/DefaultEnvironment and imports these globals at runtime.
Import("env", "projenv", "pio_lib_builder")  # pyright: ignore[reportUndefinedVariable]


LIBRARY_DIR = realpath(pio_lib_builder.path)  # pyright: ignore[reportUndefinedVariable]
INCLUDE_DIR = join(LIBRARY_DIR, "include")
LDSCRIPT_PATH = join(LIBRARY_DIR, "linker", "STM32F446RETX_FLASH.ld")

BUILD_FLAGS = [
  "-O3",
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
