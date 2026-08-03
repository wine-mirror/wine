#!/bin/bash
set -e

DXVK_DIR="$PWD/build/dxvk/"
DXVK_INTERMEDIATE_DIR="$PWD/build/dxvki/"
cd dxvk-uwp
meson setup --cross-file build-win64.txt --buildtype release --prefix "$DXVK_DIR/x64" "$DXVK_INTERMEDIATE_DIR/w64"
meson setup --cross-file build-win32.txt --buildtype release --prefix "$DXVK_DIR/x32" "$DXVK_INTERMEDIATE_DIR/w32"
cd ..
