# SPDX-License-Identifier: LGPL-2.1-or-later
[CmdletBinding()]
param(
  [string]$Prefix = "$HOME/.local/share/ailinux-wine/prefixes/win11-complete-alpha",
  [string]$Wine = "$HOME/.local/opt/ailinux-wine/alpha-0.1.0/bin/wine"
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
Import-Module (Join-Path $PSScriptRoot 'AILinux.Wine.psm1') -Force
$profile = Compare-AILinuxWineProfile -Prefix $Prefix -Wine $Wine
$integrity = Test-AILinuxWineIntegrity -Prefix $Prefix -Wine $Wine
if (-not $profile.Compliant -or -not $integrity.Healthy) { throw 'Wine runtime configuration is not healthy' }
$env:WINEPREFIX = $Prefix
$env:WINEDEBUG = '-all'
& (Join-Path $root 'run-vc14-probe.sh') $Wine
if ($LASTEXITCODE -ne 0) { throw 'VC++ x86/x64 behavior test failed' }
& (Join-Path $root 'run-win11-api-probe.sh') $Wine
if ($LASTEXITCODE -ne 0) { throw 'Windows API tests failed' }
$env:WINEDLLOVERRIDES = 'd3d8=n;d3d9=n;d3d10core=n;d3d11=n;dxgi=n;d3d12=n;d3d12core=n'
& (Join-Path $root 'run-graphics-probe.sh') $Wine
if ($LASTEXITCODE -ne 0) { throw 'Graphics hardware tests failed' }
Write-Output 'WIN11_PREFIX_VERIFIED'
