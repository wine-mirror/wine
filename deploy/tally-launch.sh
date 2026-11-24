#!/usr/bin/env bash
set -euo pipefail
# Runs AS the shared `tally` account (via sudo from the user's session).
# Because every user's Tally runs as this same UID against the same prefix,
# they all attach to ONE wineserver -> one lock domain -> no corruption.
export WINEPREFIX=/opt/tally/prefix
export WINEARCH=win64
export WINE=/opt/wine-tally/bin/wine        # the custom-built WINE
export WINEDEBUG=-all
# DISPLAY is inherited from the invoking user's RDP session (per-user screen).
cd "$WINEPREFIX/drive_c/Program Files/TallyPrime" 2>/dev/null || cd "$WINEPREFIX/drive_c"
exec "$WINE" tally.exe
