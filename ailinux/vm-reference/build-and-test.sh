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
 "$a-w64-mingw32-gcc-posix" -O2 -Wall -Wextra -Werror "$ROOT/ailinux/probes/win11-api-probe.c" -o "$OUT/win11-api-probe-$a.exe"
 WINEPREFIX="$P" WINEDEBUG=-all timeout 40s "$W" "$OUT/win11-api-probe-$a.exe" > "$OUT/wine-behavior-$a.txt"
 grep -q '^SUMMARY pass=5 fail=0' "$OUT/wine-behavior-$a.txt"
 echo "WINE_BEHAVIOR_PASS=$a"
 "$a-w64-mingw32-gcc-posix" -O2 -Wall -Wextra -Werror -municode "$ROOT/ailinux/console/conpty-resize-probe.c" -o "$OUT/conpty-resize-$a.exe"
 echo "CONPTY_PROBE_BUILT=$a"
 echo "WINE_PROBE_PASS=$a"
done
