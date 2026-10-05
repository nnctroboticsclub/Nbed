"""Apply the NUCLEO-F446RE build settings carried by the Nbed library."""

from os.path import join, realpath

Import("env", "projenv", "pio_lib_builder")


LIBRARY_DIR = realpath(pio_lib_builder.path)
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
configure_build_environment(env)
configure_build_environment(projenv)

global_env = DefaultEnvironment()
configure_build_environment(global_env)
global_env.Replace(LDSCRIPT_PATH=LDSCRIPT_PATH)
global_env.Depends("$BUILD_DIR/$PROGNAME$PROGSUFFIX", LDSCRIPT_PATH)
