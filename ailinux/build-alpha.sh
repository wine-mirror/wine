#!/usr/bin/env bash
set -euo pipefail
SRC_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${AILINUX_WINE_BUILD_DIR:-${SRC_DIR}/../wine-build-alpha}"
PREFIX="${AILINUX_WINE_PREFIX:-${HOME}/.local/opt/ailinux-wine/alpha-0.1.0}"
JOBS="${AILINUX_WINE_JOBS:-4}"
mkdir -p "$BUILD_DIR"
printf 'Source: %s\nBuild: %s\nPrefix: %s\nJobs: %s\n' "$SRC_DIR" "$BUILD_DIR" "$PREFIX" "$JOBS"
cd "$BUILD_DIR"
"$SRC_DIR/configure" --prefix="$PREFIX" --enable-win64
make -j "$JOBS"
make install
"$PREFIX/bin/wine" --version
printf 'Installed isolated alpha at %s\n' "$PREFIX"
