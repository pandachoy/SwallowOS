#!/bin/sh
set -e
. ./config.sh

for PROJECT in $PROJECTS; do
    (cd $PROJECT && $MAKE clean)
done

rm -rf sysroot
rm -rf isodir
rm -rf SwallowOS.iso