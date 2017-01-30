#!/bin/bash -e
runcross() {
    sudo docker run -e BUILDER_UID=$(id -u) -e BUILDER_GID=$(id -g) -e BUILDER_USER=$(id -un) -e BUILDER_GROUP=$(id -gn) --volume "$PWD":/work $1 sh -c "$2"
}

build() {
    arch="$1"
    img="$2"
    args="$3"
    self=.

    # using Dockcross
    mkdir -p $self/build/$arch &&
        runcross "$img" "cd $self/build/$arch && cmake $args ../.. && make -j$(nproc)"

    # or using local cross toolchain (TODO)
    # (mkdir -p $self/build/$arch &&
    #     cd $self/build/$arch && cmake $args -DPORT=linux ../.. && make -j$(nproc))
}

build i386-linux dockcross/linux-x86 ""
build amd64-linux dockcross/linux-x64 ""
build rpi-linux dockcross/linux-armv6 "-DCMAKE_TOOLCHAIN_FILE=../../rpitoolchain.cmake"

(mkdir -p build/win && cd build/win && cmake -DEMBED_BOOTLOADERS=0 -DWIN32=1 -DCROSS=1 ../.. -B. && make)
