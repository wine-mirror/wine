[CmdletBinding()]
param(
 [string]$InputFolder=(Split-Path -Parent $MyInvocation.MyCommand.Path),
 [string]$OutputFolder="$env:USERPROFILE\Desktop\AILinuxReferenceResults"
)
$ErrorActionPreference='Stop'
New-Item -ItemType Directory -Force -Path $OutputFolder | Out-Null
foreach($arch in @('i686','x86_64')){
 $exe=Join-Path $InputFolder "winapi-probe-$arch.exe"
 if(-not (Test-Path -LiteralPath $exe)){throw "Missing Windows PE: $exe"}
 $output=Join-Path $OutputFolder "windows-winapi-$arch.json"
 & $exe | Out-File -LiteralPath $output -Encoding ascii
 if($LASTEXITCODE -ne 0){throw "Windows reference probe failed"}
 $json=Get-Content -Raw -LiteralPath $output | ConvertFrom-Json
 if($json.schema -ne 1){throw "Unexpected result schema"}
 Write-Output "WINDOWS_REFERENCE_OK=$output"
}
