# MSI and verified snapshot-restore regression

The included WiX-compatible WXS template builds a harmless text-file MSI
using wixl on Linux. No third-party installer is used.

On zombie-pc, an isolated Wine prefix was installed through
Manage-AILinuxWindowsPackages.ps1: SHA-256 verified; snapshot verified;
msiexec returned 0; installed payload compared byte-for-byte with source;
then prefix transaction restore removed the file and preserved system.reg.

Rebuild from repository (write artifacts outside git):

    mkdir -p /tmp/ailinux-msi-smoke
    cp ailinux/packages/tests/*wxs ailinux/packages/tests/*txt /tmp/ailinux-msi-smoke/
    (cd /tmp/ailinux-msi-smoke && wixl -o smoke.msi ailinux-smoke.wxs)
    sha256sum /tmp/ailinux-msi-smoke/smoke.msi

When scripting noninteractive installation, use PowerShell -Command and
an explicit native -Confirm:$false invocation. For regular use, inspect
first, and then -WhatIf. The default still asks for confirmation.
Never use arbitrary unreviewed vendor packages with the test prefix.

Failure injection (Win32 PE exit 23) and portable ZIP smoke tests passed on zombie-pc. The failing executable wrote a marker, and automatic snapshot restore removed it. A ZIP path traversal was rejected, while a SHA256-pinned archive with a Windows PE was installed and restored.
