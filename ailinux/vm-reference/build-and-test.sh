#!/bin/bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$HOME/workspace/.wine-build-state/windows-reference"
W="$HOME/.local/opt/ailinux-wine/alpha-0.1.0/bin/wine"
P="$HOME/.local/share/ailinux-wine/prefixes/win11-complete-alpha"
mkdir -p "$OUT"
for a in i686 x86_64; do
 "$a-w64-mingw32-gcc-posix" -Wall -Wextra -Werror -O2 "$ROOT/ailinux/vm-reference/winapi-reference-probe.c" -o "$OUT/winapi-probe-$a.exe"
 WINEPREFIX="$P" WINEDEBUG=-all timeout 40s "$W" "$OUT/winapi-probe-$a.exe" > "$OUT/wine-winapi-$a.json"
 python3 -m json.tool "$OUT/wine-winapi-$a.json" >/dev/null
 echo "WINE_PROBE_PASS=$a"
done
