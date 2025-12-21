#!/bin/sh
set -e
. ./build.sh

mkdir -p isodir/boot/grub

cp sysroot/boot/SwallowOS.kernel isodir/boot/SwallowOS.kernel
cat > isodir/boot/grub/grub.cfg << EOF
menuentry "SwallowOS" {
    multiboot /boot/SwallowOS.kernel
}
EOF
grub-mkrescue -o SwallowOS.iso isodir