#!/usr/bin/env bash
# AILinux Wine: out-of-tree, isolated and multi-arch native build.
set -euo pipefail
SRC_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${AILINUX_WINE_BUILD_DIR:-${SRC_DIR}/../wine-build-alpha-wow64}"
PREFIX="${AILINUX_WINE_PREFIX:-${HOME}/.local/opt/ailinux-wine/alpha-0.1.0}"
JOBS="${AILINUX_WINE_JOBS:-6}"
ARCHS="i386,x86_64"

[[ "$JOBS" =~ ^[1-9][0-9]*$ ]] || { echo "Bad AILINUX_WINE_JOBS" >&2; exit 2; }
for tool in make gcc bison flex pkg-config i686-w64-mingw32-gcc-posix x86_64-w64-mingw32-gcc-posix i686-w64-mingw32-g++-posix x86_64-w64-mingw32-g++-posix; do
  command -v "$tool" >/dev/null || { echo "Build prerequisite missing: $tool" >&2; exit 2; }
done
test -f "$SRC_DIR/configure" || { echo "Wine sources missing" >&2; exit 2; }
[[ "$BUILD_DIR" != "$SRC_DIR" ]] || { echo "Out-of-tree build required" >&2; exit 2; }
case "$PREFIX" in "$HOME"/*) ;; *) echo "Refusing non-user installation: $PREFIX" >&2; exit 2 ;; esac
echo "Wine source: $SRC_DIR"
echo "Build directory: $BUILD_DIR"
echo "Install prefix: $PREFIX"
echo "Architectures: $ARCHS; jobs: $JOBS"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
# Wine configure determines PE target from executable names ending in -gcc.
# Use wrappers so its target detection works with distro -posix toolchain names.
WRAPPER_DIR="$BUILD_DIR/mingw-posix-bin"
mkdir -p "$WRAPPER_DIR"
for triplet in i686 x86_64; do
  for suffix in gcc g++; do
    ln -sfn "$(command -v "$triplet-w64-mingw32-$suffix-posix")" "$WRAPPER_DIR/$triplet-w64-mingw32-$suffix"
  done
done
env i386_CC="$WRAPPER_DIR/i686-w64-mingw32-gcc" i386_CXX="$WRAPPER_DIR/i686-w64-mingw32-g++" \
    x86_64_CC="$WRAPPER_DIR/x86_64-w64-mingw32-gcc" x86_64_CXX="$WRAPPER_DIR/x86_64-w64-mingw32-g++" \
    "$SRC_DIR/configure" --prefix="$PREFIX" --enable-archs="$ARCHS"
make -j "$JOBS"
make install
mkdir -p "$PREFIX/share/ailinux-wine"
{
  printf 'AILinux Wine release: %s\n' "$(cat "$SRC_DIR/ailinux/VERSION")"
  printf 'Upstream Wine: %s\n' "$(cat "$SRC_DIR/VERSION")"
  printf 'Git revision: %s\n' "$(git -C "$SRC_DIR" rev-parse HEAD)"
  printf 'Architecture set: %s\n' "$ARCHS"
  printf 'Installed prefix: %s\n' "$PREFIX"
} > "$PREFIX/share/ailinux-wine/build-info.txt"
"$PREFIX/bin/wine" --version
echo "Alpha installed, isolated from system Wine: $PREFIX"
