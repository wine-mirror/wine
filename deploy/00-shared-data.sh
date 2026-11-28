#!/usr/bin/env bash
set -euo pipefail
# FIX 2+3: shared company data on a LOCAL lock-honouring filesystem, group-writable.
# WINE maps Tally's LockFileEx record locks to fcntl() POSIX locks; on a LOCAL
# ext4/xfs disk the kernel enforces them ACROSS every per-user Wine prefix, which
# is what serialises writes and prevents corruption. Never put this on NFS/SMB.
DATA="${TALLY_DATA:-/srv/tally/data}"
getent group tallyusers >/dev/null || groupadd tallyusers
install -d -g tallyusers -m 2775 "$DATA"      # setgid: new files keep the group
chmod g+s "$DATA"
FSTYPE="$(stat -f -c %T "$DATA")"
case "$FSTYPE" in
  ext2/ext3|ext4|xfs|btrfs) echo "data fs OK (local, lock-honouring): $FSTYPE" ;;
  *) echo "FATAL: $DATA is on '$FSTYPE' -> byte-range locks will NOT enforce -> corruption."
     echo "       Move the data to a LOCAL ext4/xfs disk. Never NFS/SMB."; exit 1 ;;
esac
echo "Shared data ready: $DATA (group tallyusers, setgid 2775)"
echo "Add each Tally user: usermod -aG tallyusers <user>"
