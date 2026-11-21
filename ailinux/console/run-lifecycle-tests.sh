#!/bin/bash
# Isolated ConPTY process and resize regression for the *built* Wine engine.
set -euo pipefail
WINE_BIN="${AILINUX_WINE_BIN:-/home/zombie/workspace/wine-build-alpha-wow64/wine}"
PREFIX="${AILINUX_CONPTY_PREFIX:-/home/zombie/.local/share/ailinux-wine/prefixes/gui-app-smoke-alpha}"
OUT="${AILINUX_CONPTY_OUT:-/home/zombie/workspace/.wine-build-state/windows-reference}"
PARALLEL="${AILINUX_CONPTY_PARALLEL:-4}"
if [[ ! "$PARALLEL" =~ ^[1-9][0-9]?$ ]] || (( PARALLEL > 12 )); then
  echo "Invalid AILINUX_CONPTY_PARALLEL (expected 1..12)" >&2
  exit 2
fi
test -x "$WINE_BIN"
test -d "$PREFIX"
mkdir -p "$OUT"
for arch in x86_64 i686; do
  exe="$OUT/conpty-resize-$arch.exe"
  test -s "$exe"
  for mode in normal stress close-only; do
    case "$mode" in
      normal) args=(); expected=CONPTY_RESIZE_BEHAVIOR_PASS;;
      stress) args=(--stress); expected=CONPTY_STRESS_PASS;;
      close-only) args=(--close-only); expected=CONPTY_CLOSE_ONLY_READY;;
    esac
    for ((n=1;n<=PARALLEL;n++)); do
      logfile="$OUT/lifecycle-$arch-$mode-$n.log"
      ( WINEPREFIX="$PREFIX" WINEDEBUG=-all timeout 30s "$WINE_BIN" "$exe" "${args[@]}" > "$logfile" 2>&1 &&
        grep -q "$expected" "$logfile" &&
        grep -q '^CONPTY_CLOSE_MS=' "$logfile" ) &
      pids+=( "$!" )
    done
    failed=0
    for pid in "${pids[@]}"; do
      if ! wait "$pid"; then failed=1; fi
    done
    pids=()
    if (( failed )); then
      echo "FAIL arch=$arch mode=$mode (logs under $OUT)" >&2
      exit 1
    fi
    echo "PASS arch=$arch mode=$mode parallel=$PARALLEL"
  done
done
for arch in x86_64 i686; do
  io_exe="$OUT/conpty-io-$arch.exe"
  test -s "$io_exe"
  failed=0
  pids=()
  for ((n=1;n<=PARALLEL;n++)); do
    logfile="$OUT/lifecycle-$arch-io-$n.log"
    ( WINEPREFIX="$PREFIX" WINEDEBUG=-all timeout 30s "$WINE_BIN" "$io_exe" > "$logfile" 2>&1 &&
      grep -q '^CONPTY_IO_CHILD_EXIT=0' "$logfile" &&
      grep -q '^CONPTY_IO_ROUNDTRIP_PASS' "$logfile" ) &
    pids+=( "$!" )
  done
  for pid in "${pids[@]}"; do
    if ! wait "$pid"; then failed=1; fi
  done
  pids=()
  if (( failed )); then
    echo "FAIL arch=$arch mode=io" >&2
    exit 1
  fi
  echo "PASS arch=$arch mode=io parallel=$PARALLEL"
done
echo CONPTY_LIFECYCLE_SUITE_PASS
