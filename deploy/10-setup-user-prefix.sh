#!/usr/bin/env bash
set -euo pipefail
# Run ONCE per Tally user, AS THAT USER. Independent Wine prefix (own wineserver
# + desktop -> isolated input) with a UNIQUE Windows computer name, the shared
# data mapped to Z:, a per-user Tally port, and a cache-hardened tally.ini.
export WINE="${WINE:-/opt/wine-tally/bin/wine}"
export WINEPREFIX="${WINEPREFIX:-$HOME/.tally-wine}"
export WINEARCH=win64
DATA="${TALLY_DATA:-/srv/tally/data}"
NAME="TLY-$(id -un | tr 'a-z' 'A-Z' | cut -c1-11)"
# Per-user port isolation (VM-4 convention 9050, 9052, 9054 ...). A managed index
# avoids any collision; this derives a stable even port from the uid.
PORT="${TALLY_PORT:-$(( 9050 + (($(id -u) % 400) * 2) ))}"
TALLY_DIR="$WINEPREFIX/drive_c/Program Files/TallyPrime"

"$WINE" wineboot --init
sleep 2

# FIX 1a: unique workstation identity via registry (baseline; works without the
# version.dll shim). VM-4 additionally uses an in-memory version.dll hook on
# GetComputerName* (see VERSION-DLL.md), which supersedes this when installed.
"$WINE" reg add "HKLM\System\CurrentControlSet\Control\ComputerName\ComputerName" /v ComputerName /t REG_SZ /d "$NAME" /f
"$WINE" reg add "HKLM\System\CurrentControlSet\Control\ComputerName\ActiveComputerName" /v ComputerName /t REG_SZ /d "$NAME" /f

# Shared LOCAL data mapped into this prefix as Z:
ln -sfn "$DATA" "$WINEPREFIX/dosdevices/z:"

# Cache-hardened tally.ini (prevents client-side dirty-buffer desync in multi-user)
mkdir -p "$TALLY_DIR"
cat > "$TALLY_DIR/tally.ini" <<'INI'
[General]
Data=Z:\
Load Buffering=No
EnableOptimizedCmpLoading=No
Port=__PORT__
INI
sed -i "s/__PORT__/$PORT/" "$TALLY_DIR/tally.ini"

echo "Prefix $WINEPREFIX ready: workstation=$NAME  port=$PORT  data -> $DATA"
echo "On first launch set License = Use License From Network (Windows Gateway VM)."
