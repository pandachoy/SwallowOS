#!/bin/sh
set -e
. ./iso.sh

qemu-img create -f raw floppy_disk.img 1440K
printf 'HELLOFLOPPY!' | dd of=floppy_disk.img bs=512 seek=1 conv=notrunc status=none

qemu-system-$(./target-triplet-to-arch.sh $HOST) \
    -cdrom SwallowOS.iso \
    -m 2G \
    -drive file=floppy_disk.img,if=floppy,format=raw