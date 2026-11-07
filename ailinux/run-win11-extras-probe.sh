#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WINE_BIN="${1:-wine}"
: "${WINEPREFIX:?Explicit prefix required}"
[[ "$WINEPREFIX" = "$HOME"/* && "$WINEPREFIX" != "$HOME/.wine" ]] || exit 2
OUT="${AILINUX_WINE_TEST_DIR:-$ROOT/../wine-test-alpha}"
mkdir -p "$OUT"
for arch in x86_64 i686; do
 "$arch-w64-mingw32-gcc-posix" -O2 -Wall -Wextra -Werror "$ROOT/ailinux/probes/win11-extras-probe.c" -o "$OUT/win11-extras-$arch.exe" -lole32 -luuid
 timeout 45s "$WINE_BIN" "$OUT/win11-extras-$arch.exe"
done
