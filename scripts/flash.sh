#!/usr/bin/env bash
# flash.sh — write fip.bin onto a microSD card
#
# Usage: ./scripts/flash.sh /dev/sdX
#
# The Milk-V Duo Mask ROM looks for a file named "fip.bin" in the first
# FAT32 partition of the microSD card.

set -euo pipefail

if [[ $# -lt 1 ]]; then
    echo "Usage: $0 <device>  (e.g. /dev/sdc or /dev/mmcblk0)"
    exit 1
fi

DEV="$1"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
FIP="${SCRIPT_DIR}/../build/fip.bin"

if [[ ! -f "$FIP" ]]; then
    echo "fip.bin not found — build the project first:"
    echo "  cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/riscv64-baremetal.cmake"
    echo "  cmake --build build"
    exit 1
fi

if [[ ! -b "$DEV" ]]; then
    echo "Not a block device: $DEV"
    exit 1
fi

echo "WARNING: this will erase $DEV"
read -r -p "Continue? [y/N] " ans
[[ "$ans" =~ ^[Yy]$ ]] || { echo "Aborted."; exit 0; }

echo "==> Partitioning $DEV"
sudo parted -s "$DEV" \
    mklabel msdos \
    mkpart primary fat32 1MiB 100%

sleep 1
sudo partprobe "$DEV" 2>/dev/null || true
sleep 1

# Determine partition device name (sdX1 or mmcblk0p1)
if [[ "$DEV" =~ "mmcblk" ]]; then
    PART="${DEV}p1"
else
    PART="${DEV}1"
fi

echo "==> Formatting $PART as FAT32"
sudo mkfs.vfat -F 32 -n MILKVDUO "$PART"

echo "==> Mounting and copying fip.bin"
MNT=$(mktemp -d)
sudo mount "$PART" "$MNT"
sudo cp "$FIP" "$MNT/fip.bin"
sudo sync
sudo umount "$MNT"
rmdir "$MNT"

# Detect the most likely serial device for the UART adapter
SERIAL=""
for dev in /dev/ttyACM0 /dev/ttyUSB0 /dev/ttyACM1 /dev/ttyUSB1; do
    if [[ -e "$dev" ]]; then
        SERIAL="$dev"
        break
    fi
done

echo ""
echo "==> Done. SD card is ready."
echo ""
echo "Wiring (Milk-V Duo 40-pin header):"
echo "  Adapter RX  →  GP12  (UART0 TX)"
echo "  Adapter TX  →  GP13  (UART0 RX)"
echo "  Adapter GND →  GND"
echo ""
if [[ -n "$SERIAL" ]]; then
    echo "Serial device detected: $SERIAL"
    echo "Connect with:"
    echo "  picocom -b 115200 $SERIAL"
else
    echo "No serial device detected yet. Once the adapter is plugged in, connect with:"
    echo "  picocom -b 115200 /dev/ttyACM0   # or /dev/ttyUSB0"
fi
echo ""
echo "Insert the SD card into the board and power on."
echo "Exit picocom with: Ctrl+A then Ctrl+X"
