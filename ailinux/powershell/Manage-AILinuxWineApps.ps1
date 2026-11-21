# SPDX-License-Identifier: LGPL-2.1-or-later
[CmdletBinding(SupportsShouldProcess=$true)]
param(
 [ValidateSet('list','check','launch')][string]$Action='list',
 [string]$App='',
 [string]$Prefix="$HOME/.local/share/ailinux-wine/prefixes/win11-complete-alpha",
 [string]$Wine="$HOME/.local/opt/ailinux-wine/alpha-0.1.0/bin/wine"
)
$ErrorActionPreference='Stop'
Import-Module (Join-Path $PSScriptRoot 'AILinux.Wine.psm1') -Force
$ctx=Get-AILinuxWineStatus -Prefix $Prefix -Wine $Wine
$catalog=Get-Content -Raw (Join-Path $PSScriptRoot '../apps/catalog.json') | ConvertFrom-Json
if($catalog.schema -ne 1){throw 'Unsupported catalog schema'}
$items = foreach($property in $catalog.apps.PSObject.Properties){
 $id=$property.Name
 $entry=$property.Value
 $available=$false
 $path=$null
 if($entry.kind -eq 'wine-builtin'){
   $basename=$entry.exe
   $path=Join-Path $ctx.Prefix "drive_c/windows/system32/$basename"
   if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
     $fallback=Join-Path $ctx.Prefix "drive_c/windows/$basename"
     if (Test-Path -LiteralPath $fallback -PathType Leaf) { $path=$fallback }
   }
   $available=Test-Path -LiteralPath $path -PathType Leaf
 } elseif($entry.kind -eq 'staged-unverified'){
   $path=Join-Path $ctx.Prefix ('drive_c/'+($entry.exe -replace '^C:\\','' -replace '\\','/'))
   $available=$false # Windows executable present, but functional smoke failed
 }
 [pscustomobject]@{Id=$id;Name=$entry.display;Kind=$entry.kind;Available=$available;Path=$path;Executable=$entry.exe}
}
if($Action -eq 'list'){ $items | Select-Object Id,Name,Kind,Available; return }
if([string]::IsNullOrWhiteSpace($App)){throw '-App is required'}
$match=@($items|Where-Object {$_.Id -eq $App})
if($match.Count -ne 1){throw "Unknown app identifier: $App"}
$item=$match[0]
if($Action -eq 'check'){$item|Select-Object Id,Name,Kind,Available,Path;return}
if(-not $item.Available){throw "App not available under Wine: $($item.Name) ($($item.Kind))"}
if(-not $PSCmdlet.ShouldProcess($item.Name,"Start Windows application in $($ctx.Prefix)")){return}
$env:WINEPREFIX=$ctx.Prefix
$env:WINEDEBUG='-all'
$env:PATH="$(Split-Path $ctx.Engine):$env:PATH"
& $ctx.Engine $item.Executable
if($LASTEXITCODE -ne 0){throw "Wine app exited with $LASTEXITCODE"}
