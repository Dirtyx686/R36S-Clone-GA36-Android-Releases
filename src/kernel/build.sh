#!/bin/sh
set -eu
cd -- "$(dirname -- "$0")"
: "${CROSS_COMPILE:=arm-eabi-}"
: "${HOSTCC:=cc}"
: "${JOBS:=4}"
export KBUILD_BUILD_USER=ga36 KBUILD_BUILD_HOST=ga36-v1-k11 KBUILD_BUILD_VERSION=82
mkdir -p build
cp ga36_k11_defconfig build/.config
make -C source O="$PWD/build" ARCH=arm CROSS_COMPILE="$CROSS_COMPILE" HOSTCC="$HOSTCC" oldconfig </dev/null
make -C source O="$PWD/build" ARCH=arm CROSS_COMPILE="$CROSS_COMPILE" HOSTCC="$HOSTCC" KCFLAGS='-Wno-error -fno-pie' -j"$JOBS" zImage
