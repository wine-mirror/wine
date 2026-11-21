[CmdletBinding()]
param([string]$OutputFolder="$env:USERPROFILE\Desktop\AILinuxReferenceResults")
$ErrorActionPreference='Stop'
New-Item -ItemType Directory -Force -Path $OutputFolder | Out-Null
$users=foreach($name in @('ailinux-test','ailinux-admin')) {
 $local=Get-LocalUser -Name $name -ErrorAction SilentlyContinue
 if($null -eq $local) {
  [pscustomobject]@{Name=$name;Exists=$false;Enabled=$false;Role='missing'}
  continue
 }
 $admins=Get-LocalGroupMember -SID 'S-1-5-32-544' -ErrorAction SilentlyContinue
 $isAdmin=@($admins | Where-Object { $_.Name -like "*\$name" }).Count -gt 0
 [pscustomobject]@{
  Name=$name;Exists=$true;Enabled=$local.Enabled
  Role=if($isAdmin){'Administrators'}else{'Users'}
 }
}
$os=Get-CimInstance -ClassName Win32_OperatingSystem
$report=[ordered]@{
 schema=1
 computer=$env:COMPUTERNAME
 windows_caption=$os.Caption
 windows_version=$os.Version
 build_number=$os.BuildNumber
 powershell=$PSVersionTable.PSVersion.ToString()
 current_user=$env:USERNAME
 users=@($users)
}
$json=Join-Path $OutputFolder 'guest-system.json'
$report | ConvertTo-Json -Depth 5 | Set-Content -Encoding UTF8 -LiteralPath $json
Write-Output "GUEST_SYSTEM_REPORT=$json"
if(@($users | Where-Object {$_.Name -eq 'ailinux-test' -and $_.Role -eq 'Users' -and $_.Exists}).Count -ne 1) {
 throw 'Local standard reference account is missing or has incorrect role'
}
Write-Output 'STANDARD_GUEST_USER_VERIFIED'
