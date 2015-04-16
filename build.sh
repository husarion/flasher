#!/bin/bash -e
if [ ! -z "$BOOTLOADER_DIR" ]; then
	export BOOTLOADER_DIR=$(readlink -f $BOOTLOADER_DIR)
fi

./gen_bootloader_files.sh

rm -f CMakeCache.txt
cmake -DEMBED_BOOTLOADERS=1 . && make

rm -f CMakeCache.txt
cmake -DEMBED_BOOTLOADERS=1 -DX86=1 . && make

rm -f CMakeCache.txt
cmake -DEMBED_BOOTLOADESR=1 -DWIN32=1 -DCROSS=1 . && make
