#!/bin/bash
# SPDX-License-Identifier: LGPL-2.1-or-later
set -euo pipefail
NAME=ailinux-win11-reference
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
INBOX=/var/lib/libvirt/ailinux-reference/inbox
OUTBOX=/var/lib/libvirt/ailinux-reference/outbox
ACTION="${1:-status}"

# SSH and automation shells lack DISPLAY, while KDE may already be running.
# Read only the recognized GUI keys from the user's session manager.
prepare_gui_display() {
  local environment key value
  export XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}"
  if environment="$(systemctl --user show-environment 2>/dev/null)"; then
    for key in DISPLAY WAYLAND_DISPLAY XAUTHORITY XDG_SESSION_TYPE XDG_CURRENT_DESKTOP; do
      value="$(printf '%s\n' "$environment" | sed -n "s/^$key=//p" | head -n 1)"
      if [[ -n "$value" ]]; then
        printf -v "$key" '%s' "$value"
        export "$key"
      fi
    done
  fi
  if [[ -n "${DISPLAY:-}" && -n "${XAUTHORITY:-}" ]] &&
     [[ -r "$XAUTHORITY" ]] &&
     command -v xwininfo >/dev/null 2>&1 &&
     env DISPLAY="$DISPLAY" XAUTHORITY="$XAUTHORITY" timeout 5s xwininfo -root -stats >/dev/null 2>&1; then
    export GDK_BACKEND=x11
    return 0
  fi
  if [[ -n "${WAYLAND_DISPLAY:-}" ]] &&
     [[ -S "$XDG_RUNTIME_DIR/$WAYLAND_DISPLAY" ]]; then
    export GDK_BACKEND=wayland
    return 0
  fi
  printf '%s\n' \
    "No graphical KDE session is reachable." \
    "Log into Plasma locally, or use the VM's loopback-only VNC console" \
    "through a secure SSH tunnel." >&2
  return 1
}

case "$ACTION" in
  status)
    virsh -c qemu:///system dominfo "$NAME" | grep -E '^(Name|State|CPU|Max memory|Autostart)'
    virsh -c qemu:///system vncdisplay "$NAME" || true
    ;;
  view)
    prepare_gui_display
    if [[ "$(virsh -c qemu:///system domstate "$NAME")" != running ]]; then
      echo "VM not running: use vm-control.sh start" >&2
      exit 3
    fi
    exec virt-viewer --connect qemu:///system --attach "$NAME"
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
