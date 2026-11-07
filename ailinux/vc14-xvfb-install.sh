#!/usr/bin/env bash
# Install official MS VC++ v14 x86/x64 in a dedicated Wine prefix under Xvfb.
# SPDX-License-Identifier: LGPL-2.1-or-later
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PREFIX="${AILINUX_WINE_PREFIX_DIR:-$HOME/.local/share/ailinux-wine/prefixes/vc14-xvfb}"
WINE_BIN="${AILINUX_WINE_BIN:-$HOME/.local/opt/ailinux-wine/alpha-0.1.0/bin/wine}"
CACHE="${AILINUX_VC14_CACHE:-$HOME/.cache/ailinux-wine/vc14}"
[[ "$PREFIX" = "$HOME"/.local/share/ailinux-wine/prefixes/* && ! -L "$PREFIX" ]] || { echo 'Unsafe prefix' >&2; exit 2; }
[[ "$WINE_BIN" = "$HOME"/* && -x "$WINE_BIN" ]] || { echo 'Unsafe Wine engine' >&2; exit 2; }
if [[ -z "${DISPLAY:-}" ]]; then
  exec xvfb-run -a -s '-screen 0 1280x800x24 -nolisten tcp' "$0" "$@"
fi
export WINEPREFIX="$PREFIX" WINEARCH=win64 WINEDEBUG="${WINEDEBUG:--all}"
export PATH="$(dirname "$WINE_BIN"):$PATH"
mkdir -p "$CACHE" "$(dirname "$PREFIX")"
if [[ ! -e "$PREFIX" ]]; then
  "$WINE_BIN" wineboot -u
  "$(dirname "$WINE_BIN")/wineserver" -w
fi
[[ -f "$PREFIX/system.reg" ]] || exit 2
"$WINE_BIN" winecfg -v win11
"$(dirname "$WINE_BIN")/wineserver" -w
BACKUP="$HOME/.local/share/ailinux-wine/backups/$(basename "$PREFIX")-vc14-$(date +%Y%m%d-%H%M%S)-$$"
mkdir -p "$(dirname "$BACKUP")"
cp -a --reflink=auto "$PREFIX" "$BACKUP"
echo "ROLLBACK_PREFIX=$BACKUP"
for arch in x86 x64; do
  binary="$CACHE/vc_redist.$arch.exe"
  if [[ ! -f "$binary" ]]; then
    curl -fLsS --retry 2 --max-time 120 -o "$binary.part" "https://aka.ms/vc14/vc_redist.$arch.exe"
    [[ "$(head -c2 "$binary.part")" == MZ ]] || exit 3
    mv "$binary.part" "$binary"
  fi
  sha256sum "$binary"
  logdir="$PREFIX/drive_c/users/$(whoami)/AppData/Local/Temp"
  started="$(date +%s)"
  timeout 130s "$WINE_BIN" "$binary" /install /quiet /norestart
  "$(dirname "$WINE_BIN")/wineserver" -w
  if [[ "$arch" == x86 ]]; then pattern=dd_vcredist_x86_; else pattern=dd_vcredist_amd64_; fi
  newest="$(find "$logdir" -maxdepth 1 -name "$pattern*.log" ! -name '*_000_*' ! -name '*_001_*' -type f -printf '%T@ %p\n' | sort -nr | head -1 | cut -d' ' -f2-)"
  [[ -n "$newest" ]] || { echo "No Burn log for $arch" >&2; exit 1; }
  [[ "$(stat -c %Y "$newest")" -ge "$started" ]] || { echo "Stale Burn log for $arch" >&2; exit 1; }
  python3 -B "$ROOT/bugs/verify-vc14-burn.py" "$newest"
done
echo 'VC14_BOTH_INSTALLERS_VERIFIED'
