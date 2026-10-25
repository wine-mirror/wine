#!/usr/bin/env bash
set -euo pipefail
WINE_BIN="${1:-wine}"
command -v "$WINE_BIN" >/dev/null 2>&1 || { echo "Wine binary missing: $WINE_BIN" >&2; exit 2; }
export WINEPREFIX="${WINEPREFIX:-$HOME/.local/share/ailinux-wine/prefixes/alpha-smoke}"
export WINEARCH=win64
mkdir -p "$WINEPREFIX"
echo "Wine: $("$WINE_BIN" --version)"
echo "Prefix: $WINEPREFIX"
if command -v timeout >/dev/null; then
  timeout 90s "$WINE_BIN" cmd /c ver
  timeout 90s "$WINE_BIN" cmd /c echo AILinux-Wine-smoke-ok
else
  "$WINE_BIN" cmd /c ver
  "$WINE_BIN" cmd /c echo AILinux-Wine-smoke-ok
fi
