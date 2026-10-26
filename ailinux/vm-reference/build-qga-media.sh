#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$HOME/workspace/.wine-build-state/windows-reference"
PAYLOAD="$OUT/guest-agent-payload"
ISO="$OUT/ailinux-qga-tools.iso"
( cd "$PAYLOAD"; sha256sum -c SHA256SUMS )
STAGE="$(mktemp -d)"
trap 'rm -rf "$STAGE"' EXIT
mkdir -p "$STAGE/guest-agent" "$STAGE/vioserial/w11/amd64"
cp "$PAYLOAD/guest-agent/qemu-ga-x86_64.msi" "$STAGE/guest-agent/"
cp "$PAYLOAD/vioserial/w11/amd64/"{vioser.inf,vioser.sys,vioser.cat} "$STAGE/vioserial/w11/amd64/"
cp "$ROOT/ailinux/vm-reference/"{Install-QemuGuestAgent.ps1,Run-WindowsReference.ps1,Test-WindowsReference.ps1,Update-WindowsReference.ps1} "$STAGE/"
cp "$OUT"/winapi-probe-{i686,x86_64}.exe "$STAGE/"
cp "$OUT"/win11-api-probe-{i686,x86_64}.exe "$STAGE/"
xorriso -as mkisofs -quiet -J -V AILINUX_QGA -o "$ISO" "$STAGE"
sha256sum "$ISO"
