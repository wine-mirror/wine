[CmdletBinding(SupportsShouldProcess=$true)]
param(
 [string]$SourceFolder="$env:USERPROFILE\Desktop\AILinuxReferenceResults",
 [Parameter(Mandatory=$true)][string]$DestinationFolder
)
$ErrorActionPreference='Stop'
if([Environment]::OSVersion.Platform -ne [PlatformID]::Win32NT){throw 'Windows guest required'}
$names=@('windows-winapi-i686.json','windows-winapi-x86_64.json','windows-behavior-i686.txt','windows-behavior-x86_64.txt')
foreach($name in $names){
 $path=Join-Path $SourceFolder $name
 if(-not (Test-Path -LiteralPath $path -PathType Leaf)){throw "Missing guest reference result: $name"}
 if($name.EndsWith('.json')){
  $report=Get-Content -Raw -LiteralPath $path | ConvertFrom-Json
  if($report.schema -ne 1 -or $report.arch_bits -notin @(32,64) -or $null -eq $report.api){throw "Invalid result: $name"}
 }
}
if(-not (Test-Path -LiteralPath $DestinationFolder -PathType Container)){throw 'Destination not mounted'}
if(-not $PSCmdlet.ShouldProcess($DestinationFolder,'Export four verified Windows probe artifacts')){return}
foreach($name in $names){
 Copy-Item -LiteralPath (Join-Path $SourceFolder $name) -Destination (Join-Path $DestinationFolder $name) -ErrorAction Stop
 Write-Output "WINDOWS_RESULT_EXPORTED=$name"
}
