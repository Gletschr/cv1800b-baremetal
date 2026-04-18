# cv1800b-baremetal

Bare-metal firmware for the **Milk-V Duo v1.2** (CV1800B SoC, T-Head C906 RISC-V).

Boots directly from a microSD card by replacing the vendor FSBL in a FIP image.
No vendor SDK, no operating system — pure C with two lines of inline assembly
for the initial stack setup.

Features:
- UART0 driver (NS16550, 115 200 baud)
- Interactive terminal with command parsing (quoted arguments, backspace editing)

---

## Hardware

| Item | Detail |
|---|---|
| Board | Milk-V Duo v1.2 |
| SoC | CV1800B — T-Head C906, RV64IMAC, 1 GHz |
| RAM | 64 MB DDR (built-in) |
| Debug UART | UART0 — **GP12 = TX**, **GP13 = RX** (default pin function, no pinmux needed) |

**UART wiring** (connect a USB-UART adapter between the board and your laptop):

```
Adapter RX  →  GP12  (board TX)
Adapter TX  →  GP13  (board RX)
Adapter GND →  GND
```

---

## Building

The project uses **CMake** and a RISC-V bare-metal cross-compiler.
The easiest way to get the right toolchain is to use the provided **Dev Container**
(Docker-based, works with CLion or VS Code).

### Option A — Dev Container (recommended)

**CLion:** Open the project, choose *Remote Development → Dev Containers*,
select the project folder. CLion builds the image and connects automatically.

**VS Code:** Install the *Dev Containers* extension, then
*Reopen in Container*.

**CLI (Docker):**
```bash
docker build -t cv1800b-baremetal .
docker run --rm --privileged -v $(pwd):/work -w /work cv1800b-baremetal \
    cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/riscv64-baremetal.cmake
docker run --rm --privileged -v $(pwd):/work -w /work cv1800b-baremetal \
    cmake --build build
```

### Option B — host toolchain

Install `gcc-riscv64-unknown-elf`, `cmake ≥ 3.20`, and Python 3,
then clone [sophgo/fiptool](https://github.com/sophgo/fiptool) to `/fiptool`.

```bash
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/riscv64-baremetal.cmake
cmake --build build
```

After a successful build, `build/fip.bin` is ready to flash.

---

## Flashing

Insert the microSD card. The flash script partitions it, formats FAT32,
and copies `fip.bin` — **all existing data will be erased**.

```bash
# from inside the dev container or with the host toolchain:
cmake --build build --target flash
```

The default device is `/dev/mmcblk0`. Override it if your card appears elsewhere:

```bash
cmake --build build --target flash -- FLASH_DEVICE=/dev/sdb
```

Or run the script directly:

```bash
./scripts/flash.sh /dev/mmcblk0
```

---

## Connecting over UART

Find the serial device your USB-UART adapter registered as:

```bash
ls /dev/ttyUSB* /dev/ttyACM*
```

Open a terminal session at 115 200 baud:

```bash
picocom -b 115200 /dev/ttyACM0
```

Power on the board (or insert the SD card and power on). After a brief pause
you will see the boot banner and a prompt:

```
  Milk-V Duo v1.2
  CPU : T-Head C906  RV64IMAC  1 GHz
  RAM : 64 MB DDR

duo>
```

Exit picocom with **Ctrl+A** then **Ctrl+X**.

---

## Terminal commands

| Command | Description |
|---|---|
| `echo [text ...]` | Print arguments to the terminal |
| `echo "text with spaces"` | Quoted argument is treated as one token |
| `help` | List available commands |

---

## Build targets

| Target | Description |
|---|---|
| `fip` *(default)* | Compile firmware and pack `build/fip.bin` |
| `flash` | Flash `fip.bin` to the microSD card |
| `disasm` | Print annotated disassembly of `build/hello.elf` |

---

## Project structure

```
.devcontainer/devcontainer.json   CLion / VS Code dev container config
cmake/riscv64-baremetal.cmake     CMake cross-compilation toolchain file
CMakeLists.txt                    Build system
Dockerfile                        Dev container image (Ubuntu 22.04 + toolchain)
linker.ld                         Linker script — DDR at 0x80000000, 64 MB
scripts/flash.sh                  microSD flash helper
src/
  startup.c   Reset vector (_start) and C startup (_c_init, BSS clear)
  uart.c/h    NS16550 UART driver (init, getc, putc, puts)
  term.c/h    Interactive terminal (readline, argument parser, commands)
  main.c      Application entry point
docs/         CV1800B datasheet, Milk-V Duo datasheet and schematic
```

## Boot flow

```
Power on
  └─ Mask ROM (in silicon, 0x04400000)
       └─ initialises built-in DDR
            └─ reads fip.bin from FAT32 microSD partition
                 └─ loads FSBL slot → DDR at 0x80000000
                      └─ jumps to 0x80000000
                           └─ _start  (set stack pointer)
                                └─ _c_init  (zero BSS, call main)
                                     └─ main  (init UART, start terminal)
```
