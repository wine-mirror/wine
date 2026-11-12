#!/usr/bin/env bash
# SPDX-License-Identifier: LGPL-2.1-or-later
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WINE_BIN="${1:-$HOME/.local/opt/ailinux-wine/alpha-0.1.0/bin/wine}"
: "${WINEPREFIX:?Set an isolated VC++ 2010 test prefix}"
[[ "$WINEPREFIX" = "$HOME"/* && "$WINEPREFIX" != "$HOME/.wine" ]] || exit 2
OUT="${AILINUX_WINE_TEST_DIR:-$ROOT/../wine-test-alpha}"
mkdir -p "$OUT"
for arch in i686 x86_64; do
  "$arch-w64-mingw32-gcc-posix" -O2 -Wall -Wextra -Werror "$ROOT/ailinux/probes/vc100-probe.c" -o "$OUT/vc100-probe-$arch.exe"
  timeout 35s "$WINE_BIN" "$OUT/vc100-probe-$arch.exe"
done
