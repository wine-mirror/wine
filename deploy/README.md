# Tally-on-WINE — multi-user deployment (per-prefix, the working architecture)

Runs 5–16 Linux users on ONE VM against ONE shared company-data folder without
corruption, each on their own screen size and keyboard.

## Model: one Wine prefix PER user (do NOT share a wineserver)

- Each Linux user has their OWN `WINEPREFIX` -> own `wineserver` -> own WINE
  desktop. That is what keeps keyboard/mouse focus isolated (a shared wineserver
  causes focus deadlock + transaction-rollback crashes — avoid it).
- Data safety does NOT come from a shared lock domain. It comes from **POSIX
  byte-range locks**: WINE maps Tally's `LockFileEx` record locks to real
  `fcntl(F_SETLK)` locks, which the Linux kernel enforces across every prefix —
  but only on a LOCAL filesystem.

## The fixes (what makes per-prefix safe)

1. **Unique workstation name per prefix** (`10-setup-user-prefix.sh`) — each
   prefix reports a distinct Windows computer name, so Tally sees N separate LAN
   workstations (its supported multi-user mode). This is the version.dll shim's job.
2. **Shared data on LOCAL ext4/xfs** (`00-shared-data.sh`) — so fcntl byte-range
   locks actually enforce across prefixes. NEVER NFS/SMB.
3. **Group-writable shared dir, setgid 2775** (`00-shared-data.sh`) — every
   user's writes stay accessible; no ownership drift.
4. **Per-prefix sessions** (`01-install-xrdp.sh`, `xsession-tally`) — own
   wineserver/desktop each = isolated input; xrdp sizes each session to its device.
5. **Same custom WINE + Tally version everywhere; license from the Windows
   Gateway Server** over the network.

## Deploy order (as root on the VM, then per user)
1. build + install custom WINE to /opt/wine-tally
2. ./00-shared-data.sh
3. ./01-install-xrdp.sh
4. install -m755 tally-launch.sh /usr/local/bin/tally-launch.sh
5. per user: usermod -aG tallyusers <user>; then AS that user: ./10-setup-user-prefix.sh
   and put xsession-tally as their ~/.xsession (chmod +x)

## Standing rules (break either and corruption returns)
- Data stays on a LOCAL disk. Never a network mount.
- All users on ONE VM. Never point a second VM at the same data.

## Production hardening (VM-4 parity)

On top of the baseline, the live VM-4 setup adds:

- **Unique hostname** — baseline uses `reg add`; VM-4 uses an in-memory
  `version.dll` hook (see VERSION-DLL.md). Either prevents Tally Error 122.
- **Dynamic monitor resizing** — `version.dll` `GetSystemMetrics` hook + X11
  work-area query, so each session fits its device (no letterboxing).
- **DB cache hardening** — `tally.ini`: `Load Buffering = No`,
  `EnableOptimizedCmpLoading = No`. Now written by `10-setup-user-prefix.sh`.
- **Port isolation** — a unique Tally port per user (9050, 9052, ...), written
  to `tally.ini` by `10-setup-user-prefix.sh`.
