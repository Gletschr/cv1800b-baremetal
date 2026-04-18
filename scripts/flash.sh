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
FIP="$(dirname "$0")/../bin/fip.bin"

if [[ ! -f "$FIP" ]]; then
    echo "fip.bin not found — run 'make' first"
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
# Wipe existing partition table, then create one primary FAT32 partition
sudo parted -s "$DEV" \
    mklabel msdos \
    mkpart primary fat32 1MiB 100%

# Give the kernel a moment to re-read the partition table
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

echo "==> Done.  Insert SD card into board, connect UART adapter:"
echo "    laptop RX  →  GP12 (UART0 TX, board side)"
echo "    laptop TX  →  GP13 (UART0 RX, board side)"
echo "    GND        →  GND"
echo "    open terminal: screen /dev/ttyUSB0 115200"
