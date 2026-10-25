#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WINE_BIN="${1:-wine}"
: "${WINEPREFIX:?Explicit isolated prefix required}"
[[ "$WINEPREFIX" = "$HOME"/* && "$WINEPREFIX" != "$HOME/.wine" ]] || exit 2
OUT="${AILINUX_WINE_TEST_DIR:-$ROOT/../wine-test-alpha}"
mkdir -p "$OUT"
for arch in i686 x86_64; do
  "$arch-w64-mingw32-gcc-posix" -O2 -Wall -Wextra -Werror "$ROOT/ailinux/probes/win11-api-probe.c" -o "$OUT/win11-api-probe-$arch.exe"
  timeout 50s "$WINE_BIN" "$OUT/win11-api-probe-$arch.exe" | tee "$OUT/win11-api-$arch.log"
done
