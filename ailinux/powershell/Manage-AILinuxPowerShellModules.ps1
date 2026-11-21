# SPDX-License-Identifier: LGPL-2.1-or-later
# Microsoft PSResourceGet. Linux PowerShell host only: not Wine module injection.
[CmdletBinding(SupportsShouldProcess=$true,ConfirmImpact='Medium')]
param(
 [ValidateSet('list','search','install')][string]$Action='list',
 [ValidateSet('Microsoft.PowerShell.SecretManagement','Microsoft.PowerShell.SecretStore',
              'Pester')][string]$Name='Microsoft.PowerShell.SecretManagement',
 [string]$Version=''
)
$ErrorActionPreference='Stop'
if(-not $IsLinux){throw 'This script is intended for the native Linux PowerShell control plane'}
Import-Module Microsoft.PowerShell.PSResourceGet -ErrorAction Stop
$repo=Get-PSResourceRepository -Name PSGallery -ErrorAction Stop
if($repo.Uri -notmatch '^https://www[.]powershellgallery[.]com/api/v2/?$') {
  throw 'Refusing unexpected PowerShell Gallery repository URL'
}
switch($Action) {
 'list' { Get-InstalledPSResource -ErrorAction SilentlyContinue | Select-Object Name,Version,Repository; return }
 'search' {
   Find-PSResource -Name $Name -Repository PSGallery -ErrorAction Stop |
     Select-Object Name,Version,Repository,Description
   return
 }
 'install' {
   if(-not $PSCmdlet.ShouldProcess($Name,"Install PSResourceGet module in Linux CurrentUser scope from PSGallery")){return}
   $args = @{Name=$Name;Repository='PSGallery';Scope='CurrentUser';AcceptLicense=$true;ErrorAction='Stop'}
   if($Version){$args.Version=$Version}
   # Do not set TrustRepository, disable dependency checks, or elevate.
   Install-PSResource @args
   $installed=Get-InstalledPSResource -Name $Name -ErrorAction Stop
   if(-not $installed){throw "PSResourceGet did not report $Name installed"}
   $installed | Select-Object Name,Version,Repository
 }
}
