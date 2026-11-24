#!/usr/bin/env bash
set -euo pipefail
# Shared execution identity + one shared prefix + LOCAL data dir.
# Every Tally process runs as `tally` so they all share ONE wineserver.
TALLY_HOME=/opt/tally
PREFIX="$TALLY_HOME/prefix"
DATA="$PREFIX/drive_c/TallyData"

id tally &>/dev/null || useradd --system --home "$TALLY_HOME" --shell /usr/sbin/nologin tally
getent group tallyusers >/dev/null || groupadd tallyusers

install -d -o tally -g tally "$TALLY_HOME" "$PREFIX" "$PREFIX/drive_c" "$DATA"

# Fail loud if the data dir is not on a local lock-honouring filesystem.
FSTYPE="$(stat -f -c %T "$DATA")"
case "$FSTYPE" in
  ext2/ext3|ext4|xfs|btrfs) echo "data filesystem OK: $FSTYPE" ;;
  *) echo "WARNING: $DATA is on '$FSTYPE'. Byte-range locks may NOT enforce here."
     echo "         Move the data to a local ext4/xfs disk or corruption can occur." ;;
esac
echo "Service account + shared prefix ready at $PREFIX"
