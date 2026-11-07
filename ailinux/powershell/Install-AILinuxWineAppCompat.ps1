# SPDX-License-Identifier: LGPL-2.1-or-later
# Install validated Windows runtime components into a dedicated prefix only.
[CmdletBinding(SupportsShouldProcess=$true, ConfirmImpact='Medium')]
param(
    [string]$Prefix = "$HOME/.local/share/ailinux-wine/prefixes/win11-appcompat",
    [string]$Wine = "$HOME/.local/opt/ailinux-wine/alpha-0.1.0/bin/wine",
    [switch]$IncludeExtras
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$homeRoot = [IO.Path]::GetFullPath($HOME).TrimEnd('/')
$allowed = "$homeRoot/.local/share/ailinux-wine/prefixes/"
$target = [IO.Path]::GetFullPath($Prefix)
if (-not $target.StartsWith($allowed, [StringComparison]::Ordinal) -or
    $target -eq "$homeRoot/.wine" -or (Test-Path -LiteralPath $target -PathType Leaf)) { throw "Unsafe prefix: $Prefix" }
if ((Test-Path -LiteralPath $target) -and (Get-Item -LiteralPath $target -Force).LinkType) { throw 'Symlink prefix refused' }
if (-not $Wine.StartsWith("$homeRoot/.local/opt/ailinux-wine/", [StringComparison]::Ordinal) -or -not (Test-Path -LiteralPath $Wine -PathType Leaf)) { throw 'Unmanaged Wine binary' }
if (-not $PSCmdlet.ShouldProcess($target,'Create Windows 11 prefix, install DXVK/VKD3D, official VC++ v14 and optional Winetricks extras')) { return }
Import-Module (Join-Path $PSScriptRoot 'AILinux.Wine.psm1') -Force
$env:WINEPREFIX = $target
$env:WINEARCH = 'win64'
$env:WINEDEBUG = '-all'
$env:AILINUX_WINE_PREFIX_DIR = $target
$env:AILINUX_WINE_BIN = $Wine
$env:AILINUX_VC14_CACHE = "$homeRoot/workspace/.wine-build-state/ms-vc14"
$env:PATH = "$(Split-Path $Wine):$env:PATH"
if (-not (Test-Path -LiteralPath (Join-Path $target 'system.reg'))) {
    & $Wine wineboot -u
    if ($LASTEXITCODE -ne 0) { throw 'wineboot failed' }
    & (Join-Path (Split-Path $Wine) 'wineserver') -w
}
& $Wine winecfg -v win11
if ($LASTEXITCODE -ne 0) { throw 'winecfg version failed' }
& (Join-Path (Split-Path $Wine) 'wineserver') -w
$snapshot = New-AILinuxWineSnapshot -Prefix $target -Wine $Wine -Confirm:$false
Write-Output "BASELINE_SNAPSHOT=$($snapshot.backup)"
Invoke-AILinuxWineGraphicsRuntime -Prefix $target -Wine $Wine -Action install -Confirm:$false
Write-Output 'GRAPHICS_INSTALLED'
& (Join-Path $root 'vc14-xvfb-install.sh')
if ($LASTEXITCODE -ne 0) { throw 'Official VC++ v14 installer failed; preserve snapshot for recovery' }
Write-Output 'VCRUN_INSTALLER_VERIFIED'
if ($IncludeExtras) {
    & (Join-Path $root 'win11-profile.sh') install-extras
    if ($LASTEXITCODE -ne 0) { throw 'Winetricks extras failed; inspect and restore snapshot' }
    Write-Output 'EXTRAS_INSTALLER_COMPLETED'
}
$health = Compare-AILinuxWineProfile -Prefix $target -Wine $Wine
if (-not $health.Compliant) { throw "Runtime profile drift: $($health | ConvertTo-Json -Compress)" }
Write-Output 'PROFILE_COMPLIANT'
Write-Output "PREFIX=$target"
