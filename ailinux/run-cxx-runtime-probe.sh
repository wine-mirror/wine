#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WINE_BIN="${1:-wine}"
: "${WINEPREFIX:?Explicit Wine prefix required}"
[[ "$WINEPREFIX" = "$HOME"/* && "$WINEPREFIX" != "$HOME/.wine" ]] || exit 2
OUT="${AILINUX_WINE_TEST_DIR:-$ROOT/../wine-test-alpha}"
mkdir -p "$OUT"
for arch in i686 x86_64; do
  "$arch-w64-mingw32-g++-posix" -std=c++17 -O2 -Wall -Wextra -Werror -static -pthread \
     "$ROOT/ailinux/probes/cxx-runtime-probe.cpp" -o "$OUT/cxx-runtime-probe-$arch.exe"
  timeout 45s "$WINE_BIN" "$OUT/cxx-runtime-probe-$arch.exe"
done
