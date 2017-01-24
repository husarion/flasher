#!/bin/bash -e
# if [ ! -z "$BOOTLOADER_DIR" ]; then
	# export BOOTLOADER_DIR=$(readlink -f $BOOTLOADER_DIR)
# fi

# ./gen_bootloader_files.sh

mkcd() {
	mkdir -p "$1"
	cd "$1"
}

(mkcd build/amd64-linux && cmake -DEMBED_BOOTLOADERS=0 ../.. -B. && make)
(mkcd build/i386-linux && cmake -DEMBED_BOOTLOADERS=0 -DX86=1 -DMULTILIB_HACK=i586-linux-gnu//4.9 ../.. -B. && make)
(mkcd build/win && cmake -DEMBED_BOOTLOADERS=0 -DWIN32=1 -DCROSS=1 ../.. -B. && make)
(mkcd build/rpi-linux && cmake -DCMAKE_TOOLCHAIN_FILE=../../rpitoolchain.cmake ../.. && make)
