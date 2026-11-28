# VM-4: fix the focus steal, then verify locking (per-user prefixes)

## Root cause
The "TallyPrime 7.1 parkcables" shortcut -> `launch-avi-locktest-*.sh` runs every
user's Tally **as avi_user under one shared wineserver** (its own comment says so).
One wineserver + windows on multiple X displays = global focus contention = the
to-and-fro input steal. Fix = each user runs Tally in their **own** prefix.

## Step 1 - repoint the shortcut
Run `fix-focus-shortcut.sh` as root. It points the shortcut at the per-user
launcher `launch-tally.sh` (runs Tally as the logged-in user in `$HOME/.wine`).

## Step 2 - make per-user locking robust (2 edits to /usr/local/bin/launch-tally.sh)
The per-user launcher is missing two things the lock-test launcher had. Add them:

1. Load the version.dll shim (unique hostname + resize). Change:
   `export WINEDLLOVERRIDES="mscoree=d;mshtml=d"`
   to:
   `export WINEDLLOVERRIDES="version=n,b;mscoree=d;mshtml=d"`

2. Guarantee a UNIQUE workstation name per prefix (distinct Tally LAN node ->
   prevents Error 122). Just before `cd "$USER_TALLY_DIR"` / `wine tally.exe`, add:
   ```
   WS_NAME="TLY-$(id -un | tr 'a-z' 'A-Z' | cut -c1-11)"
   wine reg add "HKLM\System\CurrentControlSet\Control\ComputerName\ComputerName"       /v ComputerName /t REG_SZ /d "$WS_NAME" /f >/dev/null 2>&1 || true
   wine reg add "HKLM\System\CurrentControlSet\Control\ComputerName\ActiveComputerName" /v ComputerName /t REG_SZ /d "$WS_NAME" /f >/dev/null 2>&1 || true
   ```
   (If your version.dll already returns a unique name per user, this is harmless
   redundancy. If it returns a FIXED name for all, remove `version` from the
   override above and rely on this reg name instead.)

3. Confirm the license still reaches the gateway. The lock-test launcher wrote
   `Tally Gateway Server=135.235.193.92:9977` (+ `License Server=`, `Tally Gold
   License Server=`) into each tally.ini. Make sure each user's
   `$HOME/.tallyprime*/tally.ini` still has those lines, or Tally will drop to
   Educational mode.

## Step 3 - verify (the actual lock test)
With TWO users (e.g. avi_user and avi_desk2), each in their OWN RDP session:

- [ ] Both open the SAME company (the shared one at /data/tally/data/...).
- [ ] FOCUS: User A types in a voucher while User B types in theirs. Neither
      should lose focus. (If focus still steals, a shared wineserver is still in
      play - check `pgrep -a wineserver` shows TWO servers, one per user.)
- [ ] LOCK: Both open the SAME existing voucher. The second user must get
      "being used by another user" - NOT a second editable copy.
- [ ] SAVE: User A edits + saves; User B refreshes and sees A's change; no
      corruption, no "data damaged" on reopen.

PASS (focus steady + second user blocked on the same voucher + no corruption)
-> make per-user the default and retire launch-avi-locktest.sh.

FAIL (second user can edit the same voucher, or data corrupts) -> per-prefix is
not holding Tally's locks on this build. Then the shared wineserver was a real
workaround; the correct next step is per-slot Wine VIRTUAL DESKTOPS
(`wine explorer /desktop=<name>,WxH tally.exe`) so one wineserver can still give
each window its own input focus. Tell me and I'll write that variant.
