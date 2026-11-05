#!/usr/bin/env bash
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
OUT="$(mktemp -d)"
trap 'rm -rf "$OUT"' EXIT
cc -std=gnu11 -w -g -DAUTOPILOT=1 -fsanitize=address,undefined \
    '-D__sfr=static volatile unsigned char' '-D__at(x)=' '-D__banked=' \
    -D__SDCC_VERSION_MAJOR=4 -D__SDCC_VERSION_MINOR=4 '-D__sdcccall(x)=' \
    -I"$HERE/stub" -I"$HERE/../src" \
    -I"$ZGDK_PATH/include" -I"$ZOS_PATH/sdk/sdcc/include" \
    -I"$ZVB_SDK_PATH/include" -I"$ZAR_PATH/include" \
    "$HERE/host_play.c" "$HERE/../src/levels.c" -o "$OUT/play"
"$OUT/play" "$HERE/../assets/floes.bin"
