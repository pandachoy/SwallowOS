#!/bin/bash

gdb -ex "target remote:1234" isodir/boot/swallowos.kernel
