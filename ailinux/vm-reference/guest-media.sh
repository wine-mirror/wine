#!/bin/bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$HOME/workspace/.wine-build-state/windows-reference"
ISO="$OUT/ailinux-reference-tools.iso"
test -s "$OUT/winapi-probe-x86_64.exe"
test -s "$OUT/winapi-probe-i686.exe"
STAGE="$(mktemp -d)"
trap 'rm -rf "$STAGE"' EXIT
cp "$OUT"/winapi-probe-*.exe "$STAGE/"
cp "$OUT"/win11-api-probe-*.exe "$STAGE/"
cp "$ROOT/ailinux/vm-reference/Run-WindowsReference.ps1" "$STAGE/"
cp "$ROOT/ailinux/vm-reference/Test-WindowsReference.ps1" "$STAGE/"
cp "$ROOT/ailinux/vm-reference/Update-WindowsReference.ps1" "$STAGE/"
xorriso -as mkisofs -quiet -V AILINUX_REF -o "$ISO" "$STAGE"
sha256sum "$ISO"
