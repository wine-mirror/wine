#!/usr/bin/env bash
set -euo pipefail
# Per-user RDP sessions. This is what gives each user their own screen size and
# their own keyboard/mouse (isolated input) — the shared wineserver never shares
# a display. xorgxrdp supports client-driven dynamic resolution.
export DEBIAN_FRONTEND=noninteractive
apt-get update
apt-get install -y --no-install-recommends xrdp xorgxrdp fluxbox x11-xserver-utils
adduser xrdp ssl-cert 2>/dev/null || true
systemctl enable --now xrdp
echo "xrdp ready: each RDP login is an independent session, sized to the client,"
echo "with its own keyboard input. Tally never shares a screen between users."
