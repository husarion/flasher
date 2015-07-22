#!/bin/bash -e
if [ ! -z "$BOOTLOADER_DIR" ]; then
	export BOOTLOADER_DIR=$(readlink -f $BOOTLOADER_DIR)
fi

./gen_bootloader_files.sh

mkcd() {
    mkdir -p "$1"
    cd "$1"
}

(mkcd build/amd64-linux && cmake -DEMBED_BOOTLOADERS=1 ../.. && make)
(mkcd build/i386-linux && cmake -DEMBED_BOOTLOADERS=1 -DX86=1 ../.. && make)
(mkcd build/win && cmake -DEMBED_BOOTLOADERS=1 -DWIN32=1 -DCROSS=1 ../.. && make)
