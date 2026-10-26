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


# The behavior probe does not download anything or require administrator rights.
# Record its actual exit code and never claim 5/5 success based on DLL exports.
foreach($arch in @('i686','x86_64')){
 $exe=Join-Path $InputFolder "win11-api-probe-$arch.exe"
 if(-not (Test-Path -LiteralPath $exe)){throw "Missing behavior probe: $exe"}
 $output=Join-Path $OutputFolder "windows-behavior-$arch.txt"
 & $exe | Out-File -LiteralPath $output -Encoding ascii
 $result=$LASTEXITCODE
 Write-Output "WINDOWS_BEHAVIOR_EXIT_$arch=$result"
 if($result -ne 0){throw "Windows behavior probe $arch failed"}
}
