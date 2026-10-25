# AILinux Wine native Linux PowerShell control plane

Runs on PowerShell 7.6+ **on Linux**, orchestrating Wine binaries and existing
Python runtime installers. It is NOT Windows PowerShell running inside Wine.
It cannot turn unimplemented Windows kernel APIs into Linux syscalls; Wine's
own DLL/NT compatibility layer must implement them.

No elevated privileges, apt changes, provider repositories or default Wine
prefix changes are performed. System-wide package installation is deliberately
out of scope until a separately approved and rollback-tested packaging stage.

```powershell
Import-Module ./ailinux/powershell/AILinux.Wine.psm1
$prefix = "$HOME/.local/share/ailinux-wine/prefixes/ailinux-win11-alpha-test"
Get-AILinuxWineStatus -Prefix $prefix
Test-AILinuxWineIntegrity -Prefix $prefix
Get-AILinuxWineRuntimeAudit -Prefix $prefix
Get-AILinuxWineHostPackages
Test-AILinuxWineHostIntegrity
Backup-AILinuxWinePrefix -Prefix $prefix -WhatIf
Backup-AILinuxWinePrefix -Prefix $prefix -Confirm
Invoke-AILinuxWineGraphicsRuntime -Prefix $prefix -Action status
# Opt-in, guarded write operations:
Invoke-AILinuxWineGraphicsRuntime -Prefix $prefix -Action install -WhatIf
Invoke-AILinuxWineGraphicsRuntime -Prefix $prefix -Action remove -Confirm
```

Install PowerShell from official Microsoft Linux packages or release archives.
For this developer workstation, a verified PowerShell 7.6.9 archive is unpacked
under `$HOME/.local/opt/ailinux-powershell/7.6.9` without changing the host
package manager. Do not bundle Microsoft PowerShell with this Wine source repo.

See https://learn.microsoft.com/powershell/scripting/install/install-ubuntu
