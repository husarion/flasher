#!/bin/bash -e
rm -f CMakeCache.txt
cmake . && make

rm -f CMakeCache.txt
cmake -DX86=1 . && make

rm -f CMakeCache.txt
cmake -DWIN32=1 -DCROSS=1 . && make
