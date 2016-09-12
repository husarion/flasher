#!/bin/bash -e

if [ -z "$BOOTLOADER_DIR" ]; then
	echo "No BOOTLOADER_DIR"
	exit 1
fi

mkdir -p gen/
OUT_FILE=gen/bootloaders.cpp
OUT_HFILE=gen/bootloaders.h
rm -f $OUT_FILE $OUT_HFILE

cat > $OUT_HFILE <<EOF
#ifndef __BOOTLOADERS_H__
#define __BOOTLOADERS_H__

struct TBootloaderData
{
	const char* name;
	const char* data;
};

extern TBootloaderData bootloaders[];
EOF

cat > $OUT_FILE <<EOF
#include "bootloaders.h"
EOF

for file in $(find $BOOTLOADER_DIR/bin -name "bootloader_*_*_*_*_*.hex"); do

	NAME=$(basename $file .hex)
	echo $NAME
	
	echo -n "const char *data_$NAME = \"" >> $OUT_FILE
	cat $file | hexdump -v -e '/1 "@x%02x"' | tr @ '\\' >> $OUT_FILE
	echo '";' >> $OUT_FILE
done

cat >> $OUT_FILE <<EOF

TBootloaderData bootloaders[] = {
EOF

for file in $(find $BOOTLOADER_DIR/bin -name "bootloader_*_*_*_*.hex"); do

	NAME=$(basename $file .hex)
	echo $NAME
	
	echo "{ \"$NAME\", data_$NAME }," >> $OUT_FILE
done

echo "{ 0, 0 }," >> $OUT_FILE

cat >> $OUT_FILE <<EOF
};

EOF

cat >> $OUT_HFILE <<EOF

#endif
EOF
