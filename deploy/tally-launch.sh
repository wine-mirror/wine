#!/usr/bin/env bash
set -euo pipefail
# Launch Tally in the CURRENT user's OWN prefix (own wineserver + desktop ->
# isolated keyboard/mouse). Never run under a shared service account.
export WINE="${WINE:-/opt/wine-tally/bin/wine}"
export WINEPREFIX="${WINEPREFIX:-$HOME/.tally-wine}"
export WINEARCH=win64
export WINEDEBUG=-all
# Prefer the custom version.dll shim (unique hostname + GetSystemMetrics resize)
# when present in the prefix; fall back to builtin otherwise. See VERSION-DLL.md.
export WINEDLLOVERRIDES="version=n,b"
cd "$WINEPREFIX/drive_c/Program Files/TallyPrime" 2>/dev/null || cd "$WINEPREFIX/drive_c"
exec "$WINE" tally.exe
