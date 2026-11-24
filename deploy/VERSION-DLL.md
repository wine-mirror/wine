# version.dll shim (VM-4 production identity + resize hook)

The baseline scripts set a unique workstation name via `reg add`. VM-4 instead
drops a custom **version.dll** into each prefix that hooks the Win32 API in
memory — more robust than registry edits (no Wine registry-cache quirks) and it
also fixes window sizing. The compiled DLL is your own build; this is how it
plugs in.

## What the shim hooks
- `GetComputerNameA/W` and `GetComputerNameExA/W` -> return the prefix's unique
  name (e.g. `TLY-<USER>`), so Tally sees distinct LAN workstations. Prevents
  Tally Error 122 (rollback fail).
- `GetSystemMetrics` (SM_CXSCREEN / SM_CYSCREEN / SM_CXFULLSCREEN /
  SM_CYFULLSCREEN) -> return the X11 work-area size of the user's session, so
  Tally opens to the client resolution without letterboxing or scrollbars.

## Install per prefix
1. Copy your build to: `$WINEPREFIX/drive_c/windows/system32/version.dll`
2. `tally-launch.sh` already sets `WINEDLLOVERRIDES="version=n,b"` (native then
   builtin), so the shim is used when present and Wine's builtin otherwise.

When the shim is installed it supersedes the `reg add` hostname step; the
`reg add` stays as a harmless fallback for prefixes without the DLL.
