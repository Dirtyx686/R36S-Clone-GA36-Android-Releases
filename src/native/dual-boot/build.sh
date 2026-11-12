#!/bin/sh
set -eu
cd -- "$(dirname -- "$0")"
mkdir -p out
: "${ZIG:=zig}"
for pair in dual-init.c:init-dual dual-images.c:image-loops preboot.c:preboot-menu; do
  "$ZIG" cc -target arm-linux-musleabi -mcpu=cortex_a7 -Os -static -Wall -Wextra -Werror -Wno-unused-function -Wno-misleading-indentation "${pair%%:*}" -o "out/${pair##*:}"
done
