#!/usr/bin/env bash
# SPDX-License-Identifier: LGPL-2.1-or-later
set -euo pipefail
ACTION="${1:-status}"
WINE_BIN="${AILINUX_WINE_BIN:-$HOME/.local/opt/ailinux-wine/alpha-0.1.0/bin/wine}"
PREFIX="${AILINUX_WINE_PREFIX_DIR:-$HOME/.local/share/ailinux-wine/prefixes/vc14}"
CACHE="${AILINUX_VC14_CACHE:-$HOME/.cache/ailinux-wine/vc14}"
case "$ACTION" in install|status) ;; *) echo "usage: $0 {install|status}" >&2; exit 2 ;; esac
[[ "$WINE_BIN" = "$HOME"/* && -x "$WINE_BIN" ]] || { echo "Wine must be an isolated user build" >&2; exit 2; }
[[ "$PREFIX" = "$HOME"/.local/share/ailinux-wine/prefixes/* && -d "$PREFIX" && ! -L "$PREFIX" && -f "$PREFIX/system.reg" ]] || { echo "Refusing unsafe or uninitialized prefix" >&2; exit 2; }
export WINEPREFIX="$PREFIX" WINEARCH=win64 WINEDEBUG="${WINEDEBUG:--all}"
export PATH="$(dirname "$WINE_BIN"):$PATH"
wine_reg() { "$WINE_BIN" reg query "HKLM\\Software\\Microsoft\\VisualStudio\\14.0\\VC\\Runtimes\\$1" /v Version 2>/dev/null || true; }
if [[ "$ACTION" == status ]]; then
  for arch in x86 x64; do echo "VC++ $arch:"; wine_reg "$arch"; done
  exit 0
fi
"$(dirname "$WINE_BIN")/wineserver" -w
mkdir -p "$CACHE"
BACKUP="$HOME/.local/share/ailinux-wine/backups/$(basename "$PREFIX")-vc14-$(date +%Y%m%d-%H%M%S)-$$"
mkdir -p "$(dirname "$BACKUP")"
cp -a --reflink=auto "$PREFIX" "$BACKUP"
echo "ROLLBACK_PREFIX=$BACKUP"
for arch in x86 x64; do
  target="$CACHE/vc_redist.$arch.exe"
  if [[ ! -s "$target" ]]; then
    curl --fail --silent --show-error --location --retry 2 --max-time 90 "https://aka.ms/vc14/vc_redist.$arch.exe" -o "$target.part"
    [[ "$(head -c2 "$target.part")" == MZ ]] || { echo "Invalid PE installer" >&2; exit 3; }
    mv "$target.part" "$target"
  fi
  echo "VC14_SHA256=$(sha256sum "$target" | cut -d' ' -f1)"
  set +e
  timeout 120s "$WINE_BIN" "$target" /install /quiet /norestart
  result=$?
  set -e
  echo "VC14_$arch EXIT=$result"
  "$(dirname "$WINE_BIN")/wineserver" -w
  version="$(wine_reg "$arch")"
  if [[ "$version" != *REG_SZ* ]]; then
    echo "VC14_$arch NOT_VERIFIED; restore prefix from $BACKUP" >&2
    exit 1
  fi
  echo "$version"
  if [[ "$result" != 0 ]]; then
    echo "WARNING: nonzero exit ($result), inspect vendor logs despite registry entry" >&2
  fi
done
