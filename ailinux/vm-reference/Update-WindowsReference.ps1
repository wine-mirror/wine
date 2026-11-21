# Requires Windows PowerShell 5.1, local administrator, Windows Update service.
# Uses Microsoft's Windows Update Agent (WUA), never third-party packages.
[CmdletBinding(SupportsShouldProcess=$true,ConfirmImpact='High')]
param([switch]$Install)
$ErrorActionPreference='Stop'
if([Environment]::OSVersion.Platform -ne [PlatformID]::Win32NT){throw 'Genuine Windows guest required'}
$identity=[Security.Principal.WindowsIdentity]::GetCurrent()
$principal=[Security.Principal.WindowsPrincipal]::new($identity)
if(-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)){
  throw 'Run Windows PowerShell as administrator (ailinux-admin)'
}
$os=Get-CimInstance Win32_OperatingSystem
Write-Output "BEFORE=$($os.Caption) $($os.Version) build $($os.BuildNumber)"
$session=New-Object -ComObject Microsoft.Update.Session
$session.ClientApplicationID='AILinux Windows 11 reference verification'
$searcher=$session.CreateUpdateSearcher()
$found=$searcher.Search("IsInstalled=0 and IsHidden=0 and Type='Software'")
$selection=New-Object -ComObject Microsoft.Update.UpdateColl
foreach($update in $found.Updates){
  if($update.Title -match '(?i)(preview|vorschau)'){continue}
  Write-Output ("CANDIDATE="+$update.Title)
  [void]$selection.Add($update)
}
Write-Output "CANDIDATE_COUNT=$($selection.Count)"
if(-not $Install){Write-Output 'CHECK_ONLY_NO_INSTALL';return}
if($selection.Count -eq 0){Write-Output 'NO_APPLICABLE_NON_PREVIEW_UPDATES';return}
if(-not $PSCmdlet.ShouldProcess('Windows reference VM',"Download and install $($selection.Count) Microsoft Windows updates")){return}
foreach($item in $selection){if(-not $item.EulaAccepted){$item.AcceptEula()}}
$downloader=$session.CreateUpdateDownloader()
$downloader.Updates=$selection
$downloadResult=$downloader.Download()
Write-Output "DOWNLOAD_RESULT=$($downloadResult.ResultCode)"
if($downloadResult.ResultCode -notin @(2,3)){throw 'Windows Update download did not succeed'}
$installer=$session.CreateUpdateInstaller()
$installer.Updates=$selection
$result=$installer.Install()
Write-Output "INSTALL_RESULT=$($result.ResultCode)"
Write-Output "REBOOT_REQUIRED=$($result.RebootRequired)"
for($i=0;$i -lt $selection.Count;$i++){
  $r=$result.GetUpdateResult($i)
  Write-Output ("UPDATE="+$selection.Item($i).Title+"|RESULT="+$r.ResultCode+"|HRESULT="+$r.HResult)
}
if($result.ResultCode -notin @(2,3)){throw 'Windows Update installation failed'}
Write-Output 'Restart from Windows when ready; verify build 26300.9711 afterwards.'
