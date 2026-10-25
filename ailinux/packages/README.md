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
