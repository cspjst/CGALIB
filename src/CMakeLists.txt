# Copyright (c) 2026 Jeremy Simon Thornton, License: MIT
cmake_minimum_required(VERSION 3.10)

# --- Toolchain: Open Watcom v2 -> 16-bit DOS ---------------------
# Must precede project(): that is where CMake fixes platform and compiler.
set(CMAKE_SYSTEM_NAME      DOS)
set(CMAKE_SYSTEM_PROCESSOR I86)
set(CMAKE_C_COMPILER       wcl)
set(CMAKE_LINKER           wlink)

# Skip CMake's compiler probe: it cannot run DOS binaries on an ARM Mac.
set(CMAKE_C_COMPILER_WORKS 1)

project(TEST VERSION 0.1.0 LANGUAGES C)

# Set after project(): the DOS platform file would overwrite it.
set(CMAKE_EXECUTABLE_SUFFIX ".exe")

# --- Compiler options --------------------------------------------
# Reference: https://users.pja.edu.pl/~jms/qnx/help/watcom/compiler-tools/cpopts.html
# Applied unconditionally: if WATCOM detection ever failed, a guard
# would silently drop -0 and let 186+ opcodes reach the XT.
add_compile_options(
    -0          # 8086/8088 instructions only
    -bt=dos     # Target DOS
    -ml         # Large memory model (project standard)
    -za99       # C99 (partial in OW2)
    -zp1        # 1-byte struct packing (mind asm-shared structs)
    -d2         # Full debug info; also disables optimisation unless -o* given
    -w2         # Warning level 2
    -zq         # Quiet
)
# Optional switches:
#   -s      disable stack overflow checks (on by default; release builds)
#   -e<n>   stop after n errors
#   -ox     full optimisation   -os favour size   -ot favour speed
#   -fpi87  assume 8087 present (only if the target XT has one)

# --- Sources -----------------------------------------------------
# GLOB for convenience: rerun ./cmk.sh after adding files.
file(
  GLOB SOURCES CONFIGURE_DEPENDS
  *.c
)

add_executable(TEST ${SOURCES})
