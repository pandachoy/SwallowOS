#!/bin/sh
set -e
. ./iso.sh

qemu-img create -f raw floppy_disk.img 1440K
mkfs.fat -F 12 floppy_disk.img
LOOP_DEVICE=$(udisksctl loop-setup -f floppy_disk.img | grep -o '/dev/loop[0-9]\+')
echo loop_device=$LOOP_DEVICE
MOUNTPOINT=$(udisksctl mount -b $LOOP_DEVICE | awk '{print $4}')
echo "I am happy to join with you today in what will go down in history as the greatest demonstration for freedom in the history of our nation.  Five score years ago, a great American, in whose symbolic shadow we stand, signed the Emancipation Proclamation. This momentous decree came as a great beacon light of hope to millions of Negro slaves had been seared in the flames of withering injustice.  It came as a joyous daybreak to end the long night of captivity.\nBut one hundred years later, we must face the tragic fact that the Negro is still not free. One hundred years later, the life of the Negro is still sadly crippled by the manacles of segregation and the chains of discrimination.  One hundred years later, the Negro lives on a lonely island of poverty in the midst of a vast ocean of material prosperity. One hundred years later the Negro is still languishing in the comers of American society and finds himself an exile in his own land. So we have come here today to dramatize an appalling condition." > "$MOUNTPOINT/dream.txt"
echo "$LOOP_DEVICE" > /tmp/SwallowOS.loop
udisksctl unmount -b $(cat /tmp/SwallowOS.loop) || true
udisksctl loop-delete -b $(cat /tmp/SwallowOS.loop) || true

qemu-system-$(./target-triplet-to-arch.sh $HOST) \
    -cdrom SwallowOS.iso \
    -m 2G \
    -drive file=floppy_disk.img,if=floppy,format=raw \
    -boot d \
    