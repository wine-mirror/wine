#!/usr/bin/env bash
set -euo pipefail
# VM-4 focus fix.
# ROOT CAUSE: the "TallyPrime 7.1 parkcables" shortcut runs every user's Tally
# as avi_user under ONE shared wineserver. One wineserver cannot manage input
# focus across multiple X displays -> the to-and-fro focus steal.
# FIX: repoint the shortcut to the PER-USER launcher so each user gets their own
# prefix -> own wineserver -> own focus.
# Run as root on VM-4.

LAUNCHER="/usr/local/bin/launch-tally.sh"
VERSION_ARG="TallyPrime-7.1-parkcables"   # -> /data/tally/versions/TallyPrime-7.1-parkcables
TS="$(date +%Y%m%dT%H%M%S)"

shopt -s nullglob
found=0
for d in /home/*/Desktop/"TallyPrime 7.1 parkcables.desktop"; do
  cp -a "$d" "$d.bak-shared-$TS"
  sed -i "s|^Exec=.*|Exec=$LAUNCHER $VERSION_ARG|" "$d"
  echo "repointed: $d"
  found=1
done
[ "$found" = 1 ] || echo "No parkcables shortcuts found under /home/*/Desktop/"
echo
echo "Done. Each user must FULLY CLOSE Tally and log out/in once so their OWN"
echo "prefix (own wineserver) is used instead of avi_user's shared one."
echo "Old shortcuts backed up as *.bak-shared-$TS"
