# Windows package handling with native Linux PowerShell

Based on official Microsoft package-format requirements (October 2026).
WinGet is delivered through Microsoft.DesktopAppInstaller, requiring
Windows registration and supporting services. Installing winget.exe or
Add-AppxPackage into Wine does NOT provide these OS services.
MSIX/APPX/UWP, Windows Store and kernel drivers are not supported.

The native Linux pwsh orchestrator offers read-only capability discovery
and a restricted install path for verified local classic MSI/EXE payloads.
Every install requires explicit SHA256, a dedicated initialized prefix,
and creates a verified rollback snapshot first. Vendor licenses, code
signatures and actual application behavior require separate review.
Untrusted packages should be tested in a fresh isolated prefix, not the
shared complete-alpha prefix.

    pwsh -NoProfile -File ailinux/powershell/Manage-AILinuxWindowsPackages.ps1 -Action capabilities
    pwsh -NoProfile -File ailinux/powershell/Manage-AILinuxWindowsPackages.ps1 -Action inspect -PackageFile ./vendor.msi -Type msi -Sha256 THE_64_CHARACTER_VENDOR_SHA256
    pwsh -NoProfile -File ailinux/powershell/Manage-AILinuxWindowsPackages.ps1 -Action install -PackageFile ./vendor.msi -Type msi -Sha256 THE_64_CHARACTER_VENDOR_SHA256 -WhatIf

PowerShell Gallery modules can be managed on the Linux control plane
using Microsoft's PSResourceGet with CurrentUser scope, but such modules
do not automatically become installed in the Wine Windows PowerShell runtime.
Do not mark a package installed purely from installer exit code.

The optional PSResourceGet adapter is restricted to known PowerShell modules
and PSGallery over HTTPS, user scope, with support for -WhatIf:

    ~/.local/opt/ailinux-powershell/7.6.9/pwsh -NoProfile -File ailinux/powershell/Manage-AILinuxPowerShellModules.ps1 -Action list
    ~/.local/opt/ailinux-powershell/7.6.9/pwsh -NoProfile -File ailinux/powershell/Manage-AILinuxPowerShellModules.ps1 -Action install -Name Microsoft.PowerShell.SecretManagement -WhatIf

This installs modules into native Linux PowerShell, **not** Windows PowerShell
inside Wine. Modules from PSGallery require independent publisher and code
review before real installation. The repository is not marked trusted.

## Terminal formatting and MSI integration

Capabilities now default to compact one-line text that stays readable in a
narrow terminal. Detailed shows descriptions on their own lines. AsObject
retains pipeline-friendly structured records. Portable ZIP is labeled planned
because its installer has not been implemented.

A harmless MSI installation and restoration end-to-end test passed on
zombie-pc in a disposable Wine prefix, including actual file verification.
See ailinux/packages/tests/README.md. Real vendor packages still require
individual behavioral testing.

Automatic rollback is now enabled after MSI, EXE and ZIP installation errors. Portable ZIP installations require SHA256 and AppName; extracted payloads remain application-unverified until smoke tested.

## Managed portable package lifecycle (alpha)

Portable ZIP installations now write a per-package manifest containing the
local ZIP SHA256, user-supplied version and hashes of extracted payloads.
This is not a vendor signature or a guarantee that the EXE runs correctly.

The PowerShell manager supports list, audit, check-update, update and remove
for Type zip only. All state-changing operations require confirmation and a
verified prefix snapshot. The audit rejects modified files, untracked user
files, missing files and symlinks before deletion or update. An update is
based only on the supplied local candidate ZIP; there is no online tracking.
MSI and EXE installs are not part of the portable package inventory.

Four isolated Python unit tests cover the package lifecycle, modified data,
ZIP path traversal, case-insensitive filename collisions and bad checksums.
A host integration test installed two successive ZIP versions and removed
one managed application using native PowerShell in gui-app-smoke-alpha.
