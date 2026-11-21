# SPDX-License-Identifier: LGPL-2.1-or-later
# Install *only* reviewed local MSI/EXE assets in isolated Wine prefixes.
[CmdletBinding(SupportsShouldProcess=$true,ConfirmImpact='High')]
param(
  [ValidateSet('capabilities','inspect','install','list','audit','remove','check-update','update')][string]$Action='capabilities',
  [string]$PackageFile='',
  [string]$Sha256='',
  [ValidateSet('msi','exe','zip')][string]$Type='msi',
  [string]$AppName='',
  [string]$Version='unknown',
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
$portable=Join-Path $PSScriptRoot '../packages/safe-portable.py'
if($Action -in @('list','audit','remove','check-update','update')){
  if($Type -ne 'zip'){throw 'These actions currently support portable ZIP only; MSI/EXE remain unmanaged'}
  if($Action -ne 'list' -and $AppName -notmatch '^[a-zA-Z0-9][a-zA-Z0-9._-]{0,79}$'){
    throw 'Portable ZIP requires a safe -AppName'
  }
  if($Action -eq 'list'){
    & /usr/bin/python3 -B $portable --action list --prefix $ctx.Prefix
    if($LASTEXITCODE -ne 0){throw 'Portable package listing failed'}
    return
  }
  if($Action -eq 'audit'){
    & /usr/bin/python3 -B $portable --action audit --prefix $ctx.Prefix --name $AppName
    if($LASTEXITCODE -ne 0){throw 'Portable package audit failed'}
    return
  }
  if($Action -eq 'check-update'){
    if($Sha256 -notmatch '^[0-9a-fA-F]{64}$'){throw 'Candidate SHA256 required'}
    & /usr/bin/python3 -B $portable --action check-update --prefix $ctx.Prefix --name $AppName --sha256 $Sha256
    if($LASTEXITCODE -ne 0){throw 'Portable update check failed'}
    return
  }
  if($Action -eq 'remove'){
    if(-not $PSCmdlet.ShouldProcess($ctx.Prefix,"Remove managed portable app $AppName after verified snapshot")){return}
    $env:WINEPREFIX=$ctx.Prefix
    $env:PATH="$(Split-Path $ctx.Engine):$env:PATH"
    $server=Join-Path (Split-Path $ctx.Engine) 'wineserver'
    & $server -w
    if($LASTEXITCODE -ne 0){throw 'Wine server is not idle'}
    $backup=New-AILinuxWineSnapshot -Prefix $ctx.Prefix -Wine $ctx.Engine -Confirm:$false
    if(-not $backup.backup){throw 'Snapshot failed'}
    Write-Output "ROLLBACK_PREFIX=$($backup.backup)"
    try{
      & /usr/bin/python3 -B $portable --action remove --prefix $ctx.Prefix --name $AppName
      if($LASTEXITCODE -ne 0){throw 'Portable remove rejected'}
      $health=Test-AILinuxWineIntegrity -Prefix $ctx.Prefix -Wine $ctx.Engine
      if(-not $health.Healthy){throw 'Prefix integrity failed after removal'}
    }catch{
      $msg=$_.Exception.Message
      try{
        $restore=Restore-AILinuxWineSnapshot -Prefix $ctx.Prefix -Wine $ctx.Engine -Backup $backup.backup -Confirm:$false
        if(-not $restore.verified){throw 'Restore unverified'}
        Write-Output "AUTO_ROLLBACK_VERIFIED=$($backup.backup)"
      }catch{throw "Removal failed: $msg; restore failed: $_; backup: $($backup.backup)"}
      throw "Removal failed: $msg; prefix restored"
    }
    return
  }
}
if(-not $PackageFile){throw 'Explicit local package file required'}
$full=[IO.Path]::GetFullPath($PackageFile)
if(-not (Test-Path -LiteralPath $full -PathType Leaf)){throw 'Package file not found'}
if((Get-Item -LiteralPath $full -Force).LinkType){throw 'Symlink package file refused'}
$ext=[IO.Path]::GetExtension($full).ToLowerInvariant()
if($ext -ne ".$Type"){throw 'Package extension does not match installer type'}
if($full -match '[\r\n]'){throw 'Invalid package path'}
if($Type -eq 'zip' -and $AppName -notmatch '^[a-zA-Z0-9][a-zA-Z0-9._-]{0,79}$'){throw 'Portable ZIP needs safe -AppName'}
if($Action -eq 'update' -and $Type -ne 'zip'){throw 'Only portable ZIP update is supported'}
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
try {
  if($Type -eq 'msi'){
    & $ctx.Engine msiexec /i $full /qn /norestart
  }elseif($Type -eq 'zip'){
    $portableAction=if($Action -eq 'update'){'update'}else{'install'}
    & /usr/bin/python3 -B $portable --action $portableAction --prefix $ctx.Prefix --archive $full --sha256 $digest --name $AppName --version $Version
  }else{
    & $ctx.Engine $full
  }
  if($LASTEXITCODE -ne 0){throw "Installer exited with code $LASTEXITCODE"}
  & $server -w
  if($LASTEXITCODE -ne 0){throw "Wine server returned code $LASTEXITCODE"}
  $health=Test-AILinuxWineIntegrity -Prefix $ctx.Prefix -Wine $ctx.Engine
  if(-not $health.Healthy){throw 'Prefix integrity failed after package install'}
}catch{
  $errorMessage=$_.Exception.Message
  Write-Warning "Install failed: $errorMessage"
  try {
    # Stop only this prefix's wineserver; never kill system-wide Wine.
    & $server -k 2>$null
    $recovery=Restore-AILinuxWineSnapshot -Prefix $ctx.Prefix -Wine $ctx.Engine -Backup $backup.backup -Confirm:$false
    if(-not $recovery.verified){throw 'Restore did not report verified status'}
    Write-Output "AUTO_ROLLBACK_VERIFIED=$($backup.backup)"
  }catch{
    throw "Install failed: $errorMessage; automatic restore ALSO FAILED: $($_.Exception.Message); manual backup: $($backup.backup)"
  }
  throw "Install failed: $errorMessage; prefix restored from $($backup.backup)"
}
Write-Output 'INSTALLER_EXECUTED_APPLICATION_VERIFICATION_REQUIRED'
