#!/usr/bin/env bash
# Compile one SDK/app source file of the PC build, to check it compiles (seconds instead of a full build).
# Takes the compile command from the PC build's ninja graph; the object goes to /tmp and is discarded, the build
# tree is not touched.
# Usage: tools/build/compile-check.sh sdk/src/graphics/native/native_renderer.cpp
set -euo pipefail
. "$(dirname "$0")/env.sh"
F=$(realpath "${1:?source file}")
B="$DS2_ROOT/out/app-host"
[ -f "$B/build.ninja" ] || { echo "no $B/build.ninja: configure the PC build first (tools/build/build-pc.sh)"; exit 1; }
NAME=$(basename "$F")
OBJ=$(ninja -C "$B" -t targets all 2>/dev/null | cut -d: -f1 | grep -F "/$NAME.o" | while read -r o; do
  ninja -C "$B" -t query "$o" 2>/dev/null | grep -qF "$F" && { echo "$o"; break; }; done)
[ -n "$OBJ" ] || { echo "$F is not compiled by the PC build"; exit 1; }
CMD=$(ninja -C "$B" -t commands "$OBJ" | tail -1)
OUT=$(echo "$CMD" | grep -oE -- "-o [^ ]+" | head -1 | cut -c4-)
CMD=${CMD/-o $OUT/-o \/tmp\/ds2-compile-check.o}
(cd "$B" && bash -c "$CMD") && echo "OK: $F compiles"
