#!/bin/bash
set -euo pipefail
NAME=ailinux-win11-reference
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
INBOX=/var/lib/libvirt/ailinux-reference/inbox
OUTBOX=/var/lib/libvirt/ailinux-reference/outbox
ACTION=status
if test "$#" -gt 0; then ACTION="$1"; fi
case "$ACTION" in
 status)
  virsh -c qemu:///system dominfo "$NAME" | grep -E '^(Name|State|CPU|Max memory|Autostart)'
  virsh -c qemu:///system vncdisplay "$NAME" || true
  ;;
 view)
  exec virt-viewer --connect qemu:///system "$NAME"
  ;;
 start)
  virsh -c qemu:///system start "$NAME"
  ;;
 shutdown)
  virsh -c qemu:///system shutdown "$NAME"
  ;;
 push-probes)
  test -d "$INBOX"
  "$ROOT/ailinux/vm-reference/build-and-test.sh"
  OUT="$HOME/workspace/.wine-build-state/windows-reference"
  install -m 0644 "$OUT/winapi-probe-i686.exe" "$INBOX/winapi-probe-i686.exe"
  install -m 0644 "$OUT/winapi-probe-x86_64.exe" "$INBOX/winapi-probe-x86_64.exe"
  install -m 0644 "$ROOT/ailinux/vm-reference/Run-WindowsReference.ps1" "$INBOX/Run-WindowsReference.ps1"
  sha256sum "$INBOX"/winapi-probe-*.exe
  ;;
 compare)
  OUT="$HOME/workspace/.wine-build-state/windows-reference"
  for arch in i686 x86_64; do
    test -f "$OUT/wine-winapi-$arch.json"
    test -f "$OUTBOX/windows-winapi-$arch.json"
    python3 -B "$ROOT/ailinux/vm-reference/compare-probes.py" \
      --windows "$OUTBOX/windows-winapi-$arch.json" \
      --wine "$OUT/wine-winapi-$arch.json" \
      > "$OUT/comparison-$arch.json"
    echo "COMPARISON_READY=$OUT/comparison-$arch.json"
  done
  ;;
 *)
  echo "Usage: $0 status|view|start|shutdown|push-probes|compare" >&2
  exit 2
  ;;
esac
