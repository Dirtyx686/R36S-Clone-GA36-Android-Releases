#!/bin/sh
set -eu
cd -- "$(dirname -- "$0")"
: "${ZIG:=zig}"
mkdir -p bin
for os in 7 11; do
 "$ZIG" cc -target arm-linux-musleabi -mcpu=cortex_a7 -Os -static -Wall -Wextra -Werror -Wno-unused-function -Wno-misleading-indentation -DGA36_OS="$os" dual-init.c -o "bin/init-$os"
done
for pair in 'clean-images.c clean-images' 'hash.c ga36-hash'; do
 set -- $pair
 "$ZIG" cc -target arm-linux-musleabi -mcpu=cortex_a7 -Os -static -Wall -Wextra -Werror -Wno-unused-function -Wno-misleading-indentation "$1" -o "bin/$2"
done
