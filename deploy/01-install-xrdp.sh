#!/usr/bin/env bash
set -euo pipefail
# Per-user RDP sessions: each login gets its OWN X session, sized to the client
# device (dynamic resize), with its own keyboard/mouse. Combined with per-user
# Wine prefixes this gives full screen + input isolation between users.
export DEBIAN_FRONTEND=noninteractive
apt-get update
apt-get install -y --no-install-recommends xrdp xorgxrdp fluxbox x11-xserver-utils
adduser xrdp ssl-cert 2>/dev/null || true
systemctl enable --now xrdp
echo "xrdp ready: independent, correctly-sized session per user."
