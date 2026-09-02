#!/bin/sh
set -e
. ./config.sh

qemu-img create -f raw floppy_disk.img 1440K
mkfs.fat -F 12 floppy_disk.img
for f in $SYSROOT/$APPDIR/*; do
    basename=$(basename "$f")
    upper=$(echo "$basename" | tr "[:lower:]" "[:upper:]")
    mcopy -i floppy_disk.img "$f" ::"$upper"
done
