#!/usr/bin/env bash
# SPDX-License-Identifier: LGPL-2.1-or-later
set -euo pipefail
SELF_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WINE_BIN="${AILINUX_WINE_BIN:-$HOME/.local/opt/ailinux-wine/alpha-0.1.0/bin/wine}"
PREFIX="${AILINUX_WINE_PREFIX_DIR:-$HOME/.local/share/ailinux-wine/prefixes/ailinux-win11}"
ACTION="${1:-status}"
case "$ACTION" in init|status|verify|install-fonts|install-corefonts|install-allfonts|install-dotnet48|install-vcrun2022|install-extras|install-graphics|remove-graphics) ;; *)
    echo "Usage: $0 {init|status|verify|install-fonts|install-corefonts|install-allfonts|install-dotnet48|install-vcrun2022|install-extras|install-graphics|remove-graphics}" >&2; exit 2 ;;
esac
[[ "$WINE_BIN" = /* && -x "$WINE_BIN" ]] || { echo "Wine binary not executable" >&2; exit 2; }
[[ "$PREFIX" = "$HOME"/* && "$PREFIX" != "$HOME/.wine" ]] || { echo "Unsafe Wine prefix" >&2; exit 2; }
[[ ! -L "$PREFIX" ]] || { echo "Symlinked prefix refused" >&2; exit 2; }
export WINEPREFIX="$PREFIX" WINEARCH=win64 WINEDEBUG="${WINEDEBUG:--all}"
export PATH="$(dirname "$WINE_BIN"):$PATH"
check_prefix() {
  [[ -f "$PREFIX/system.reg" && -d "$PREFIX/drive_c/windows/system32" ]] || { echo "Missing prefix: first run init" >&2; exit 2; }
}
case "$ACTION" in
  init)
    if [[ ! -e "$PREFIX" ]]; then mkdir -p "$(dirname "$PREFIX")"; "$WINE_BIN" wineboot -u; "$(dirname "$WINE_BIN")/wineserver" -w; fi
    check_prefix
    "$WINE_BIN" winecfg -v win11
    "$(dirname "$WINE_BIN")/wineserver" -w
    "$WINE_BIN" cmd /c ver
    ;;
  status|verify)
    check_prefix
    "$WINE_BIN" --version
    "$WINE_BIN" cmd /c ver
    python3 -B "$SELF_DIR/runtime-manager.py" --prefix "$PREFIX" status
    if [[ "$ACTION" == verify ]]; then "$SELF_DIR/smoke.sh" "$WINE_BIN"; fi
    ;;
  install-graphics|remove-graphics)
    check_prefix
    if [[ "$ACTION" == install-graphics ]]; then mode=install; else mode=remove; fi
    python3 -B "$SELF_DIR/runtime-manager.py" --prefix "$PREFIX" "$mode"
    ;;
  install-fonts|install-corefonts|install-allfonts|install-dotnet48|install-vcrun2022|install-extras)
    check_prefix
    command -v winetricks >/dev/null
    BACKUP_ROOT="$HOME/.local/share/ailinux-wine/backups"
    mkdir -p "$BACKUP_ROOT"
    BACKUP="$BACKUP_ROOT/$(basename "$PREFIX")-$(date +%Y%m%d-%H%M%S)-$ACTION"
    cp -a --reflink=auto "$PREFIX" "$BACKUP"
    echo "Prefix backup: $BACKUP"
    case "$ACTION" in
      install-fonts) VERBS=(fontsmooth=rgb) ;;
      install-corefonts) VERBS=(corefonts) ;;
      install-allfonts) VERBS=(allfonts) ;;
      install-dotnet48) VERBS=(dotnet48) ;;
      install-vcrun2022) VERBS=(vcrun2022) ;;
      install-extras) VERBS=(d3dcompiler_47 gdiplus msxml6) ;;
    esac
    WINE="$WINE_BIN" WINESERVER="$(dirname "$WINE_BIN")/wineserver" winetricks -q "${VERBS[@]}"
    "$(dirname "$WINE_BIN")/wineserver" -w
    echo "Done. Backup: $BACKUP"
    ;;
esac
