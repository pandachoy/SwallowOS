#!/bin/sh
set -e
. ./iso.sh
. ./floppy.sh

qemu-system-$(./target-triplet-to-arch.sh $HOST) \
    -cdrom SwallowOS.iso \
    -m 2G \
    -drive file=floppy_disk.img,if=floppy,format=raw \
    -boot d \
    -no-reboot \
