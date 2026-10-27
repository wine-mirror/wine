#!/usr/bin/env bash
set -euo pipefail
echo "== AILinux Wine cross-build preflight =="
printf 'host: '; uname -srmo
printf 'version: '; cat "$(dirname "$0")/../VERSION"
for tool in gcc make bison flex pkg-config nasm i686-w64-mingw32-gcc-posix x86_64-w64-mingw32-gcc-posix i686-w64-mingw32-g++-posix x86_64-w64-mingw32-g++-posix; do
  if command -v "$tool" >/dev/null; then
    printf '[OK] %-30s %s\n' "$tool" "$(command -v "$tool")"
  else
    printf '[MISSING] %s\n' "$tool"
  fi
done
for lib in freetype2 x11 xext xrender xrandr xi xfixes egl gl gnutls vulkan wayland-client xkbregistry libpulse alsa gstreamer-1.0 gstreamer-video-1.0; do
  if pkg-config --exists "$lib"; then
    printf '[OK] %-20s %s\n' "$lib" "$(pkg-config --modversion "$lib")"
  else
    printf '[MISSING] %s (optional or blocking, see configure.log)\n' "$lib"
  fi
done
echo "Original system wine: $(wine --version)"
