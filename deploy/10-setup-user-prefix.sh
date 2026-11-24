#!/usr/bin/env bash
set -euo pipefail
# Run ONCE per Tally user, AS THAT USER. Creates their OWN Wine prefix (own
# wineserver + own desktop -> isolated keyboard/mouse) and gives it a UNIQUE
# Windows computer name so Tally treats each user as a distinct LAN workstation.
# This unique-name step is FIX 1 (what the version.dll shim achieves).
export WINE="${WINE:-/opt/wine-tally/bin/wine}"
export WINEPREFIX="${WINEPREFIX:-$HOME/.tally-wine}"
export WINEARCH=win64
DATA="${TALLY_DATA:-/srv/tally/data}"
NAME="TLY-$(id -un | tr 'a-z' 'A-Z' | cut -c1-11)"   # <=15 chars, unique per user

"$WINE" wineboot --init
sleep 2

# FIX 1: unique workstation identity per prefix
"$WINE" reg add "HKLM\System\CurrentControlSet\Control\ComputerName\ComputerName" /v ComputerName /t REG_SZ /d "$NAME" /f
"$WINE" reg add "HKLM\System\CurrentControlSet\Control\ComputerName\ActiveComputerName" /v ComputerName /t REG_SZ /d "$NAME" /f

# Map the shared LOCAL data dir into this prefix as drive Z:
ln -sfn "$DATA" "$WINEPREFIX/dosdevices/z:"

echo "Prefix ready: $WINEPREFIX   workstation=$NAME   data=Z:\\ -> $DATA"
echo "In TallyPrime set Data path = Z:\\ and License = Use License From Network (Windows VM)."
