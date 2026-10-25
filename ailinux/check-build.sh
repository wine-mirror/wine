#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PREFIX="${AILINUX_WINE_PREFIX:-${HOME}/.local/opt/ailinux-wine/alpha-0.1.0}"
printf 'Repo revision: '; git -C "$ROOT" rev-parse --short HEAD
printf 'Repo status:\n'; git -C "$ROOT" status --short
printf 'Source version: '; cat "$ROOT/VERSION"
for tool in gcc make bison flex pkg-config; do command -v "$tool"; done
if [ -x "$PREFIX/bin/wine" ]; then WINEPREFIX="$HOME/.local/share/ailinux-wine/prefixes/alpha-smoke-built" "$ROOT/ailinux/smoke.sh" "$PREFIX/bin/wine"; else echo 'No compiled binary installed yet'; fi
