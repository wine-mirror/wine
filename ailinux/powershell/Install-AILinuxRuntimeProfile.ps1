# SPDX-License-Identifier: LGPL-2.1-or-later
[CmdletBinding(SupportsShouldProcess=$true, ConfirmImpact='Medium')]
param(
  [ValidateSet('fonts','legacy-games','legacy-apps','office-compat','dotnet48','allfonts')][string]$Profile,
  [string]$Prefix = "$HOME/.local/share/ailinux-wine/prefixes/win11-appcompat",
  [string]$Wine = "$HOME/.local/opt/ailinux-wine/alpha-0.1.0/bin/wine",
  [switch]$AllowSharedDotNet
)
$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
Import-Module (Join-Path $PSScriptRoot 'AILinux.Wine.psm1') -Force
$ctx=Get-AILinuxWineStatus -Prefix $Prefix -Wine $Wine
$catalog=Get-Content -Raw (Join-Path $root 'profiles/runtime-catalog.json') | ConvertFrom-Json
$entry=$catalog.profiles.$Profile
if ($null -eq $entry) { throw "Profile not found: $Profile" }
if (($entry.PSObject.Properties.Name -contains "status") -and $entry.status -eq "blocked") { throw "Profile blocked: $($entry.block_reason)" }
if (($entry.PSObject.Properties.Name -contains 'isolation') -and $entry.isolation -and -not $AllowSharedDotNet -and ($Prefix -notmatch "/$Profile(-|$)")) {
  throw "Profile $Profile needs a separate prefix named $Profile or use -AllowSharedDotNet after reviewing compatibility"
}
if (-not $PSCmdlet.ShouldProcess($ctx.Prefix,"Install runtime profile '$Profile' ($($entry.verbs -join ', '))")) { return }
$before=New-AILinuxWineSnapshot -Prefix $ctx.Prefix -Wine $ctx.Engine -Confirm:$false
Write-Output "SNAPSHOT=$($before.backup)"
$env:WINEPREFIX=$ctx.Prefix
$env:WINEARCH='win64'
$env:WINEDEBUG='-all'
$env:WINE="$($ctx.Engine)"
$env:WINESERVER=Join-Path (Split-Path $ctx.Engine) 'wineserver'
$env:PATH="$(Split-Path $ctx.Engine):$env:PATH"
$env:WINETRICKS_STATS_REPORT='0'
$tool=Join-Path $root 'winetricks-pinned.sh'
try {
foreach($verb in @($entry.verbs)) {
  Write-Output "INSTALL=$verb"
  & $tool -q $verb
  if ($LASTEXITCODE -ne 0) { throw "Winetricks failed at $verb with code $LASTEXITCODE; snapshot=$($before.backup)" }
  & $env:WINESERVER -w
  if ($LASTEXITCODE -ne 0) { throw "Wineserver failed for $verb; snapshot=$($before.backup)" }
}
$integrity=Test-AILinuxWineIntegrity -Prefix $ctx.Prefix -Wine $ctx.Engine
if (-not $integrity.Healthy) { throw "Prefix integrity failed after $Profile; snapshot=$($before.backup)" }
Write-Output "PROFILE_INSTALLED=$Profile"
} catch {
  Write-Warning "Runtime profile $Profile failed. Restoring verified snapshot $($before.backup)"
  try {
    $restored=Restore-AILinuxWineSnapshot -Prefix $ctx.Prefix -Wine $ctx.Engine -Backup $before.backup -Confirm:$false
    Write-Output "ROLLBACK_COMPLETED=$($restored.pre_restore_backup)"
  } catch {
    Write-Warning "Automatic rollback failed; manual recovery needed from $($before.backup): $_"
  }
  throw
}
