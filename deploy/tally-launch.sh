#!/usr/bin/env bash
set -euo pipefail
# Launches Tally in the CURRENT user's OWN prefix (own wineserver + own desktop).
# This per-prefix isolation is what keeps keyboard/mouse focus separate. Do NOT
# run this under a shared service account.
export WINE="${WINE:-/opt/wine-tally/bin/wine}"
export WINEPREFIX="${WINEPREFIX:-$HOME/.tally-wine}"
export WINEARCH=win64
export WINEDEBUG=-all
cd "$WINEPREFIX/drive_c/Program Files/TallyPrime" 2>/dev/null || cd "$WINEPREFIX/drive_c"
exec "$WINE" tally.exe
