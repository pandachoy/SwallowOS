#!/bin/sh
set -e
. ./config.sh

qemu-img create -f raw floppy_disk.img 1440K
mkfs.fat -F 12 floppy_disk.img
LOOP_DEVICE=$(udisksctl loop-setup -f floppy_disk.img | grep -o '/dev/loop[0-9]\+')
echo loop_device=$LOOP_DEVICE
MOUNTPOINT=$(udisksctl mount -b $LOOP_DEVICE | awk '{print $4}')

cp $SYSROOT/$APPDIR/* $MOUNTPOINT

echo "$LOOP_DEVICE" > /tmp/swallowos.loop
udisksctl unmount -b $(cat /tmp/swallowos.loop) || true
udisksctl loop-delete -b $(cat /tmp/swallowos.loop) || true
