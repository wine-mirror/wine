#!/usr/bin/env bash
# SPDX-License-Identifier: LGPL-2.1-or-later
set -euo pipefail
SRC_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WINE_BIN="${1:-wine}"
OUT_DIR="${AILINUX_WINE_TEST_DIR:-${SRC_DIR}/../wine-test-alpha}"
export WINEPREFIX="${WINEPREFIX:-${HOME}/.local/share/ailinux-wine/prefixes/alpha-functional}"
export WINEARCH=win64
export WINEDEBUG="${WINEDEBUG:--all}"
[[ "$WINEPREFIX" != "$HOME/.wine" ]] || { echo "Refusing default Wine prefix" >&2; exit 2; }
mkdir -p "$OUT_DIR" "$WINEPREFIX"
command -v "$WINE_BIN" >/dev/null
for arch in x86_64 i386; do
    if [[ "$arch" == x86_64 ]]; then cc=x86_64-w64-mingw32-gcc; else cc=i686-w64-mingw32-gcc; fi
    if ! command -v "$cc" >/dev/null; then echo "SKIP $arch: missing $cc"; continue; fi
    "$cc" -O2 -Wall -Wextra "$SRC_DIR/ailinux/probes/functional-probe.c" \
       -o "$OUT_DIR/functional-probe-$arch.exe" -lws2_32 -lole32 -ladvapi32 -lbcrypt
    echo "== $arch with $("$WINE_BIN" --version) =="
    timeout 120s "$WINE_BIN" "$OUT_DIR/functional-probe-$arch.exe" | tee "$OUT_DIR/functional-probe-$arch.log"
done
