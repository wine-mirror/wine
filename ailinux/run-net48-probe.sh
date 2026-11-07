#!/usr/bin/env bash
set -euo pipefail
WINE_BIN="${1:-$HOME/.local/opt/ailinux-wine/alpha-0.1.0/bin/wine}"
: "${WINEPREFIX:?Set an explicit, initialized isolated .NET Framework prefix}"
[[ "$WINEPREFIX" != "$HOME/.wine" && "$WINEPREFIX" = "$HOME"/* ]] || exit 2
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
mkdir -p "$WINEPREFIX/drive_c/ailinux-probe"
cp "$ROOT/ailinux/probes/net48.cs" "$WINEPREFIX/drive_c/ailinux-probe/net48.cs"
for arch in x64 x86; do
  if [[ "$arch" = x64 ]]; then framework=Framework64; options=(); else framework=Framework; options=(/platform:x86); fi
  timeout 60s "$WINE_BIN" "C:\\windows\\Microsoft.NET\\$framework\\v4.0.30319\\csc.exe" /nologo /target:exe "${options[@]}" "/out:C:\\ailinux-probe\\net48_$arch.exe" 'C:\ailinux-probe\net48.cs'
  timeout 45s "$WINE_BIN" "C:\\ailinux-probe\\net48_$arch.exe"
done
