# Tally-on-WINE — multi-user VM deployment (5–16 users, no corruption)

Runs many Linux users on ONE VM against ONE shared company data folder without
data corruption, each on their own screen size and their own keyboard.

## The model (why it is safe and does not block input)

- **ONE shared wineserver.** Every Tally process runs as the `tally` service
  account against ONE WINEPREFIX, so all file opens land in a single lock
  domain. Tally's multi-user locking (share modes + record locks) then behaves
  exactly as on one Windows machine. Lock correctness does NOT depend on the
  number of users — 6 or 16, it stays correct. Only save *performance* drops as
  users grow; that (not integrity) is the only reason to consider TallyPrime
  Server later.
- **Many independent sessions.** Each user logs into the VM over RDP (xrdp) with
  their own Linux credentials and gets their OWN X session: own resolution
  (sized to their laptop, dynamic resize) and own keyboard/mouse. Input never
  collides because sessions are independent — only the lock domain is shared.

This is the Linux equivalent of a Windows multi-session server: one machine =
one lock domain, many isolated desktops.

## Hard requirements (break these and corruption returns)

- Company data on a LOCAL disk (ext4/xfs) on THIS VM. Never a network/SMB mount
  (byte-range locks silently stop enforcing on network filesystems).
- All users on ONE VM. A second VM on the same data = a second lock domain =
  corruption. Scale the VM UP, not OUT.
- License is served from the Windows Gateway Server over the network (unchanged).

## Deploy order (as root, on each tenant VM)

1. build + install the custom WINE to /opt/wine-tally (see repo root / loader/wine.inf.in)
2. ./00-service-account.sh                 # `tally` account + shared prefix + local data dir
3. ./01-install-xrdp.sh                     # per-user RDP sessions, dynamic resolution
4. cp 10-sudoers-tally /etc/sudoers.d/tally && chmod 440 /etc/sudoers.d/tally
5. install -m755 tally-launch.sh /usr/local/bin/tally-launch.sh
6. configure tally.ini once in the prefix: Data points to the local data dir
   ($WINEPREFIX/drive_c/TallyData), license = Use License From Network (Windows VM).

## Per user

- Create a normal Linux user; add to group `tallyusers`.
- Put `xsession-tally` as that user's ~/.xsession (chmod +x).
- On RDP login, Tally opens maximized in their session at their screen size.
