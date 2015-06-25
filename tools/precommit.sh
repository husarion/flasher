#!/bin/bash
cd "$(dirname "$0")"/..
astyle -r -n --options=tools/astylerc 'src/*.cpp' 'include/*Flasher.h' \
    --exclude=src/xcpmaster.cpp
