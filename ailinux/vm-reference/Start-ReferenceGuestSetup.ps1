# Windows-only, operator-initiated guest setup. No secrets or auto-logon.
[CmdletBinding(SupportsShouldProcess=$true, ConfirmImpact='High')]
param([switch]$InstallGuestAgent)
$ErrorActionPreference='Stop'
if([Environment]::OSVersion.Platform -ne [PlatformID]::Win32NT){
 throw 'Execute only in a Windows guest'
}
$volume=Get-Volume -FileSystemLabel 'AILINUX_QGA' -ErrorAction Stop |
 Where-Object { $_.DriveLetter } | Select-Object -First 1
if(-not $volume){throw 'AILINUX_QGA media is not mounted'}
$media=$volume.DriveLetter + ':\'
$installer=Join-Path $media 'Install-QemuGuestAgent.ps1'
if(-not (Test-Path -LiteralPath $installer -PathType Leaf)){
 throw 'Approved guest agent installer missing from media'
}
$identity=[Security.Principal.WindowsIdentity]::GetCurrent()
$principal=[Security.Principal.WindowsPrincipal]::new($identity)
if(-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)){
 throw 'Run Windows PowerShell as administrator'
}
Write-Output 'GUEST_AGENT_MEDIA_VERIFIED_BY_VOLUME_LABEL'
if(-not $InstallGuestAgent){
 Write-Output 'CHECK_ONLY: pass -InstallGuestAgent to install the approved agent'
 return
}
if(-not $PSCmdlet.ShouldProcess('Windows reference VM','Install official QEMU agent and VirtIO driver')){return}
& $installer -MediaRoot $media -Confirm:$false
if(-not $?){throw 'Guest agent install failed'}
Write-Output 'GUEST_AGENT_INSTALL_ATTEMPT_COMPLETE: verify after guest reboot'
