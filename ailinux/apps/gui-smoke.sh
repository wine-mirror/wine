#!/usr/bin/env bash
# SPDX-License-Identifier: LGPL-2.1-or-later
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
APP="${1:-notepad}"
case "$APP" in
notepad|regedit|wordpad|taskmgr|winefile|control|winver) ;;
*) echo "Unknown smoke-test app" >&2; exit 2;;
esac
WINE_BIN="${AILINUX_WINE_BIN:-$HOME/.local/opt/ailinux-wine/alpha-0.1.0/bin/wine}"
PWSH="${AILINUX_PWSH_BIN:-$HOME/.local/opt/ailinux-powershell/7.6.9/pwsh}"
: "${WINEPREFIX:?Set isolated prefix}"
[[ "$WINEPREFIX" = "$HOME"/.local/share/ailinux-wine/prefixes/* &&
   "$WINEPREFIX" != "$HOME/.wine" && -f "$WINEPREFIX/system.reg" ]] || exit 2
export ROOT APP WINE_BIN PWSH
xvfb-run -a -s '-screen 0 1280x800x24 -nolisten tcp' bash -c '
set -euo pipefail
export WINEDEBUG=-all
export PATH="$(dirname "$WINE_BIN"):$PATH"
"$PWSH" -NoProfile -NonInteractive -File "$ROOT/ailinux/powershell/Manage-AILinuxWineApps.ps1" \
 -Action launch -App "$APP" -Prefix "$WINEPREFIX" -Wine "$WINE_BIN" > /dev/null 2>&1 &
pid=$!
trap "kill $pid 2>/dev/null || true" EXIT
case "$APP" in wordpad) pattern="write.exe|wordpad.exe";; *) pattern="$APP.exe";; esac
found=0
for i in $(seq 1 15); do
  if xwininfo -root -tree 2>/dev/null | grep -Eiq "$pattern"; then found=1; break; fi
  sleep 1
done
if test "$found" -eq 0; then echo "GUI_FAIL $APP" >&2; exit 1; fi
echo "GUI_PASS $APP"
kill "$pid" 2>/dev/null || true
wait "$pid" 2>/dev/null || true
"$(dirname "$WINE_BIN")/wineserver" -k 2>/dev/null || true
'
