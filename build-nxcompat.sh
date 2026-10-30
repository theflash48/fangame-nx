#!/bin/bash
# Build the existing Horizon compatibility shim for both Ruby and mkxp-z.
set -euo pipefail
ROOT=$(cd "$(dirname "$0")" && pwd -P)
[ ! -L "$ROOT/out" ] || { echo "out/ no puede ser enlace simbolico" >&2; exit 1; }
export DEVKITPRO=C:/devkitPro
export PATH=/c/devkitPro/devkitA64/bin:$PATH
mkdir -p "$ROOT/out/lib" "$ROOT/out/nx/lib"
aarch64-none-elf-gcc -std=gnu11 -O2 -D__SWITCH__ \
  -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE \
  -I"$DEVKITPRO/libnx/include" -I"$DEVKITPRO/portlibs/switch/include" \
  -c "$ROOT/patches/nxcompat.c" -o "$ROOT/out/lib/nxcompat.o"
aarch64-none-elf-ar rcs "$ROOT/out/lib/libnxcompat.a" "$ROOT/out/lib/nxcompat.o"
cp "$ROOT/out/lib/libnxcompat.a" "$ROOT/out/nx/lib/libnxcompat.a"
aarch64-none-elf-nm "$ROOT/out/nx/lib/libnxcompat.a" | grep -E ' (sysconf|getpagesize|setmode|waitpid)$'
