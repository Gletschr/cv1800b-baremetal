cmake_minimum_required(VERSION 3.20)

# Toolchain must be set before project() so CMake uses the right compiler
# for all checks.  The CACHE PATH form lets CLion / cmake CLI override it.
set(CMAKE_TOOLCHAIN_FILE
    ${CMAKE_SOURCE_DIR}/cmake/riscv64-baremetal.cmake
    CACHE PATH "Cross-compilation toolchain file")

project(cv1800b-baremetal C ASM)

# ---------------------------------------------------------------------------
# Compiler / linker flags
# ---------------------------------------------------------------------------

# Flags shared between compilation and linking
set(ARCH_FLAGS
    -march=rv64imac   # T-Head C906 — RV64 base + multiply + atomics + compressed
    -mabi=lp64        # 64-bit integers, no hardware float
    -mcmodel=medany   # PC-relative addressing — required for position-independence
)

set(COMMON_FLAGS
    ${ARCH_FLAGS}
    -Os               # optimise for size (binary must fit in the FSBL slot)
    -g                # include debug info for GDB / disassembly
    -ffreestanding    # no hosted C runtime assumptions
    -nostdlib         # do not link the standard library
    -nostartfiles     # do not use the compiler's own crt0
)

# ---------------------------------------------------------------------------
# Firmware ELF target
# ---------------------------------------------------------------------------

add_executable(hello
    src/startup.c
    src/uart.c
    src/term.c
    src/main.c
)

target_include_directories(hello PRIVATE src)

target_compile_options(hello PRIVATE
    ${COMMON_FLAGS}
    -Wall
    -Wextra
)

target_link_options(hello PRIVATE
    ${COMMON_FLAGS}
    -T ${CMAKE_SOURCE_DIR}/linker.ld
)

# Re-link when the linker script changes
set_target_properties(hello PROPERTIES
    SUFFIX       ".elf"
    LINK_DEPENDS ${CMAKE_SOURCE_DIR}/linker.ld
)

# ---------------------------------------------------------------------------
# Paths to tools inside the dev container (set at build time, not configure)
# ---------------------------------------------------------------------------

set(FIPTOOL    python3 /fiptool/fiptool)
set(DDR_PARAM  /fiptool/data/ddr_param.bin)
set(RTOS       /fiptool/data/cvirtos.bin)

# ---------------------------------------------------------------------------
# Flat binary (ELF → raw binary stripped of ELF headers)
# ---------------------------------------------------------------------------

add_custom_command(
    OUTPUT  ${CMAKE_BINARY_DIR}/hello.bin
    COMMAND ${CMAKE_OBJCOPY} -O binary
            $<TARGET_FILE:hello>
            ${CMAKE_BINARY_DIR}/hello.bin
    DEPENDS hello
    COMMENT "objcopy → hello.bin"
)

# ---------------------------------------------------------------------------
# Dummy blobs — zero-filled placeholders for the OpenSBI and U-Boot FIP slots
# (the Mask ROM requires them to be present even though we never execute them)
# ---------------------------------------------------------------------------

add_custom_command(
    OUTPUT  ${CMAKE_BINARY_DIR}/dummy.bin
    COMMAND dd if=/dev/zero of=${CMAKE_BINARY_DIR}/dummy.bin bs=512 count=1
    COMMENT "Creating dummy FIP blob"
)

# ---------------------------------------------------------------------------
# FIP image — the container format the Mask ROM reads from the microSD card
# ---------------------------------------------------------------------------

add_custom_command(
    OUTPUT  ${CMAKE_BINARY_DIR}/fip.bin
    COMMAND ${FIPTOOL}
                --fsbl     ${CMAKE_BINARY_DIR}/hello.bin
                --ddr_param ${DDR_PARAM}
                --opensbi  ${CMAKE_BINARY_DIR}/dummy.bin
                --uboot    ${CMAKE_BINARY_DIR}/dummy.bin
                --rtos     ${RTOS}
                ${CMAKE_BINARY_DIR}/fip.bin
    DEPENDS ${CMAKE_BINARY_DIR}/hello.bin
            ${CMAKE_BINARY_DIR}/dummy.bin
    COMMENT "Packing FIP image → fip.bin"
)

# Build fip.bin by default when you hit Build
add_custom_target(fip ALL DEPENDS ${CMAKE_BINARY_DIR}/fip.bin)

# ---------------------------------------------------------------------------
# flash — write fip.bin to the microSD card
#
# The target device can be overridden:
#   cmake --build build --target flash -DFLASH_DEVICE=/dev/sdb
# ---------------------------------------------------------------------------

set(FLASH_DEVICE /dev/mmcblk0 CACHE STRING "microSD block device to flash")

add_custom_target(flash
    COMMAND sudo umount ${FLASH_DEVICE}p1 2>/dev/null || true
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/flash.sh ${FLASH_DEVICE}
    DEPENDS ${CMAKE_BINARY_DIR}/fip.bin
    COMMENT "Flashing fip.bin to ${FLASH_DEVICE}"
    USES_TERMINAL   # keep interactive prompts (sudo password, y/N) visible
)

# ---------------------------------------------------------------------------
# disasm — print annotated disassembly of the firmware (useful for debugging)
# ---------------------------------------------------------------------------

add_custom_target(disasm
    COMMAND ${CMAKE_OBJDUMP} -d $<TARGET_FILE:hello>
    DEPENDS hello
    COMMENT "Disassembly of hello.elf"
    USES_TERMINAL
)
