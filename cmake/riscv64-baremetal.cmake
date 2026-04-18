# cmake/riscv64-baremetal.cmake
# Cross-compilation toolchain for the T-Head C906 (RISC-V RV64IMAC).
#
# Tell CMake we are targeting a bare-metal system — no OS, no standard
# C runtime.  "Generic" is the conventional CMake name for this case.
set(CMAKE_SYSTEM_NAME       Generic)
set(CMAKE_SYSTEM_PROCESSOR  riscv64)

set(CMAKE_C_COMPILER        riscv64-unknown-elf-gcc)
set(CMAKE_ASM_COMPILER      riscv64-unknown-elf-gcc)

find_program(CMAKE_OBJCOPY  riscv64-unknown-elf-objcopy REQUIRED)
find_program(CMAKE_OBJDUMP  riscv64-unknown-elf-objdump REQUIRED)

# Without this, CMake tries to compile and *run* a test executable to verify
# the compiler works — impossible when cross-compiling for bare metal.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
