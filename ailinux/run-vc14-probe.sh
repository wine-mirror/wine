#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WINE_BIN="${1:-$HOME/.local/opt/ailinux-wine/alpha-0.1.0/bin/wine}"
: "${WINEPREFIX:?Set explicit isolated VC++ prefix}"
[[ "$WINEPREFIX" = "$HOME"/* && "$WINEPREFIX" != "$HOME/.wine" ]] || exit 2
OUT="${AILINUX_WINE_TEST_DIR:-$ROOT/../wine-test-alpha}"
mkdir -p "$OUT"
for arch in x86_64 i686; do
    cc="$arch-w64-mingw32-gcc-posix"
    "$cc" -O2 -Wall -Wextra "$ROOT/ailinux/probes/vc14-probe.c" -o "$OUT/vc14-probe-$arch.exe"
    timeout 45s "$WINE_BIN" "$OUT/vc14-probe-$arch.exe"
done
