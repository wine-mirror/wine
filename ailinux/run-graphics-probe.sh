#!/usr/bin/env bash
# Requires Vulkan hardware; not suitable for headless CI without a GPU.
set -euo pipefail
SRC_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WINE_BIN="${1:-wine}"
OUT_DIR="${AILINUX_WINE_TEST_DIR:-${SRC_DIR}/../wine-test-alpha}"
export WINEPREFIX="${WINEPREFIX:-${HOME}/.local/share/ailinux-wine/prefixes/alpha-graphics}"
export WINEARCH=win64
export WINEDEBUG="${WINEDEBUG:--all}"
[[ "$WINEPREFIX" != "$HOME/.wine" ]] || { echo "Refusing default wine prefix" >&2; exit 2; }
mkdir -p "$OUT_DIR" "$WINEPREFIX"
for arch in x86_64 i686; do
  cc="$arch-w64-mingw32-gcc-posix"
  command -v "$cc" >/dev/null || { echo "Missing $cc" >&2; exit 2; }
  "$cc" -O2 -Wall -Wextra "$SRC_DIR/ailinux/probes/graphics-probe.c" \
    -o "$OUT_DIR/graphics-probe-$arch.exe" -ld3d11 -ld3d12 -ldxgi -ldxguid
  echo "== Graphics $arch =="
  timeout 100s "$WINE_BIN" "$OUT_DIR/graphics-probe-$arch.exe" | tee "$OUT_DIR/graphics-probe-$arch.log"
done
