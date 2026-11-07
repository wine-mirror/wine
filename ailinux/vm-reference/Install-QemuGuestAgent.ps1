[CmdletBinding(SupportsShouldProcess=$true,ConfirmImpact='High')]
param([string]$MediaRoot=(Split-Path -Parent $MyInvocation.MyCommand.Path))
$ErrorActionPreference='Stop'
if([Environment]::OSVersion.Platform -ne [PlatformID]::Win32NT){throw 'Windows VM required'}
$principal=[Security.Principal.WindowsPrincipal]::new([Security.Principal.WindowsIdentity]::GetCurrent())
if(-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)){
 throw 'Run elevated as ailinux-admin'
}
$agent=Join-Path $MediaRoot 'guest-agent\qemu-ga-x86_64.msi'
$driver=Join-Path $MediaRoot 'vioserial\w11\amd64\vioser.inf'
$expected=@{
 $agent='19dcf8abc30f70c2eb6e87282601fd0476e4e0b19936f63d9c80be30db0796ed'
 $driver='a7bb3cc2482e0e4df281b506efb85ad4c76dd8d5a1522e480e9da354568006c0'
}
foreach($pair in $expected.GetEnumerator()){
 if(-not (Test-Path -LiteralPath $pair.Key -PathType Leaf)){throw "Missing $($pair.Key)"}
 if((Get-FileHash -LiteralPath $pair.Key -Algorithm SHA256).Hash -ne $pair.Value.ToUpperInvariant()){
  throw "Checksum mismatch: $($pair.Key)"
 }
}
Write-Output 'QGA_SOURCE_HASHES_VALID'
if(-not $PSCmdlet.ShouldProcess('Windows 11 VM','Install VirtIO serial and QEMU Guest Agent')){return}
$driverArgs=@('/add-driver',('"' + $driver + '"'),'/install')
$d=Start-Process -FilePath "$env:SystemRoot\System32\pnputil.exe" -ArgumentList $driverArgs -PassThru -Wait -NoNewWindow
if($d.ExitCode -notin @(0,3010)){throw "Driver install exit $($d.ExitCode)"}
$msiArgs=@('/i',('"' + $agent + '"'),'/qn','/norestart')
$m=Start-Process -FilePath "$env:SystemRoot\System32\msiexec.exe" -ArgumentList $msiArgs -PassThru -Wait -NoNewWindow
if($m.ExitCode -notin @(0,3010)){throw "Agent install exit $($m.ExitCode)"}
$service=Get-Service -Name 'QEMU-GA' -ErrorAction SilentlyContinue
if(-not $service){throw 'QEMU-GA service not installed'}
if($service.Status -ne 'Running'){
 try{Start-Service -Name 'QEMU-GA' -ErrorAction Stop}catch{Write-Warning 'Agent needs VM restart'}
}
Write-Output "QGA_SERVICE=$((Get-Service -Name 'QEMU-GA').Status)"
Write-Output "REBOOT_POSSIBLY_REQUIRED=$(($d.ExitCode -eq 3010) -or ($m.ExitCode -eq 3010))"
