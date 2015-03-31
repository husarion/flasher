#! /bin/sh -e
arm-none-eabi-objcopy -O binary test.elf original.bin
arm-none-eabi-objcopy -O ihex test.elf original.hex

SIZE=$(stat -c "%s" original.bin)

echo $SIZE

ARG="--hard"
if [ "$1" = "soft" ]; then
	ARG="--soft"
fi
../bin/flasher $ARG --device=/dev/ttyUSB0 --speed 230400 original.hex

st-flash read flash_content.bin 0x08008000 $SIZE && truncate flash_content.bin -s $SIZE

DIFF_BYTES=$(cmp -l original.bin flash_content.bin | wc -l)

echo "diff $DIFF_BYTES"

if [ "$1" = "soft" ]; then
	if [ "$DIFF_BYTES" != "4" ]; then
		exit 1
	fi
else
	if [ "$DIFF_BYTES" != "0" ]; then
		exit 1
	fi
fi
echo "OK"
