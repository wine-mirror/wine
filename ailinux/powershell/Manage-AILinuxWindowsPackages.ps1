# SPDX-License-Identifier: LGPL-2.1-or-later
# Install *only* reviewed local MSI/EXE assets in isolated Wine prefixes.
[CmdletBinding(SupportsShouldProcess=$true,ConfirmImpact='High')]
param(
  [ValidateSet('capabilities','inspect','install')][string]$Action='capabilities',
  [string]$PackageFile='',
  [string]$Sha256='',
  [ValidateSet('msi','exe')][string]$Type='msi',
  [switch]$Detailed,
  [switch]$AsObject,
  [string]$Prefix="$HOME/.local/share/ailinux-wine/prefixes/win11-complete-alpha",
  [string]$Wine="$HOME/.local/opt/ailinux-wine/alpha-0.1.0/bin/wine"
)
$ErrorActionPreference='Stop'
Import-Module (Join-Path $PSScriptRoot 'AILinux.Wine.psm1') -Force
$catalog=Get-Content -LiteralPath (Join-Path $PSScriptRoot '../packages/catalog.json') -Raw | ConvertFrom-Json
if($catalog.schema -ne 1){ throw 'Unsupported package catalog schema' }
if($Action -eq 'capabilities'){
  if($AsObject){ $catalog.capabilities | Select-Object id,status,detail; return }
  $statusLabels=@{
    'supported-with-verification'='MSI/EXE pruefbar'
    'linux-control-plane'='Linux-PowerShell'
    'planned'='In Planung'
    'unsupported'='Nicht verfuegbar'
  }
  foreach($entry in $catalog.capabilities){
    $label=$statusLabels[$entry.status]
    if(-not $label){$label=$entry.status}
    if($Detailed){
      Write-Output "$($entry.id): $label"
      Write-Output "  $($entry.detail)"
    }else{
      Write-Output ('{0,-23} {1}' -f $entry.id,$label)
    }
  }
  return
}
$ctx=Get-AILinuxWineStatus -Prefix $Prefix -Wine $Wine
if(-not $PackageFile){throw 'Explicit local package file required'}
$full=[IO.Path]::GetFullPath($PackageFile)
if(-not (Test-Path -LiteralPath $full -PathType Leaf)){throw 'Package file not found'}
if((Get-Item -LiteralPath $full -Force).LinkType){throw 'Symlink package file refused'}
$ext=[IO.Path]::GetExtension($full).ToLowerInvariant()
if($ext -ne ".$Type"){throw 'Package extension does not match installer type'}
if($full -match '[\r\n]'){throw 'Invalid package path'}
if($Sha256 -notmatch '^[0-9a-fA-F]{64}$'){throw 'Exact vendor-published SHA256 required'}
$digest=(Get-FileHash -LiteralPath $full -Algorithm SHA256).Hash
if($digest -ne $Sha256.ToUpperInvariant()){throw 'Package checksum mismatch'}
$result=[pscustomobject]@{Package=$full;Type=$Type;SHA256=$digest;Verified=$true;Prefix=$ctx.Prefix}
if($Action -eq 'inspect'){ $result; return }
if(-not $PSCmdlet.ShouldProcess($ctx.Prefix,"Install reviewed $Type installer $full (verified SHA256)")){return}
$env:WINEPREFIX=$ctx.Prefix
$env:WINEDEBUG='-all'
$env:PATH="$(Split-Path $ctx.Engine):$env:PATH"
$server=Join-Path (Split-Path $ctx.Engine) 'wineserver'
# No install while other Wine processes are active in this prefix.
& $server -w
if($LASTEXITCODE -ne 0){throw 'Wineserver not idle'}
$backup=New-AILinuxWineSnapshot -Prefix $ctx.Prefix -Wine $ctx.Engine -Confirm:$false
if(-not $backup.backup){throw 'Snapshot was not created'}
Write-Output "ROLLBACK_PREFIX=$($backup.backup)"
if($Type -eq 'msi'){
  & $ctx.Engine msiexec /i $full /qn /norestart
}else{
  # Explicitly no arbitrary silent flags for unknown EXE installers.
  & $ctx.Engine $full
}
if($LASTEXITCODE -ne 0){throw "Installer failed: $LASTEXITCODE; snapshot: $($backup.backup)"}
& $server -w
if($LASTEXITCODE -ne 0){throw "Wine server returned error; snapshot: $($backup.backup)"}
$health=Test-AILinuxWineIntegrity -Prefix $ctx.Prefix -Wine $ctx.Engine
if(-not $health.Healthy){throw "Prefix integrity failed; snapshot: $($backup.backup)"}
Write-Output 'INSTALLER_EXECUTED_APPLICATION_VERIFICATION_REQUIRED'
