#!/bin/bash
# Probe Windows terminal ANSI/VT output and record unresolved ctrl signals.
set -euo pipefail
WINE_BIN="${AILINUX_WINE_BIN:-/home/zombie/workspace/wine-build-alpha-wow64/wine}"
PREFIX="${AILINUX_CONPTY_PREFIX:-/home/zombie/.local/share/ailinux-wine/prefixes/gui-app-smoke-alpha}"
OUT="${AILINUX_CONPTY_OUT:-/home/zombie/workspace/.wine-build-state/windows-reference}"
mkdir -p "$OUT"
test -x "$WINE_BIN"
test -d "$PREFIX"
for arch in x86_64 i686; do
  exe="$OUT/conpty-control-$arch.exe"
  test -s "$exe"
  output="$OUT/control-vt-$arch.log"
  WINEPREFIX="$PREFIX" WINEDEBUG=-all timeout 20s "$WINE_BIN" "$exe" --vt > "$output"
  grep -q '^CONPTY_VT_PASS' "$output"
  echo "PASS arch=$arch VT"
done
if [[ "${AILINUX_CONPTY_DIAG_SIGNALS:-0}" == 1 ]]; then
  failures=0
  for arch in x86_64 i686; do
    for mode in ctrl-c ctrl-break; do
      output="$OUT/control-$mode-$arch.log"
      if WINEPREFIX="$PREFIX" WINEDEBUG=-all timeout 20s "$WINE_BIN" "$OUT/conpty-control-$arch.exe" --$mode > "$output" 2>&1 &&
         grep -q '^CONPTY_CTRL_.*_PASS' "$output"; then
        echo "PASS arch=$arch signal=$mode"
      else
        echo "UNRESOLVED arch=$arch signal=$mode (see $output)" >&2
        failures=1
      fi
    done
  done
  if (( failures )); then exit 1; fi
fi
echo CONPTY_VT_SUITE_PASS
