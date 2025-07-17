#!/bin/bash

make
mkdir -p isodir/boot/grub
cp SwallowOS.bin isodir/boot/SwallowOS.bin
cp misc/grub.cfg isodir/boot/grub/grub.cfg
grub-mkrescue -o SwallowOS.iso isodir
qemu-system-i386 -cdrom SwallowOS.iso
