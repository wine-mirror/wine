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

## Declarative gaming profile and transactional prefix snapshots

`ailinux/profiles/windows11-gaming.json` defines the desired Win11 mode,
DXVK 3.1.1 and VKD3D-Proton 3.0.1 without embedding installer code.
Compare-AILinuxWineProfile provides **read-only** drift reporting, not an
automatic Windows application installer.

```powershell
Compare-AILinuxWineProfile -Prefix $prefix
# A real verified COW copy; preserves existing prefix:
$snapshot = New-AILinuxWineSnapshot -Prefix $prefix -Confirm:$false
Test-AILinuxWineSnapshot -Prefix $prefix -Backup $snapshot.backup
# Restore requires explicit confirmation and creates a pre-restore backup:
Restore-AILinuxWineSnapshot -Prefix $prefix -Backup $snapshot.backup -WhatIf
```

Registry/runtime hashes in backup manifest are integrity samples, not a full
cryptographic inventory of every installed application file. Restores must be
performed with Wine closed. The transaction tool validates backup provenance
and restricts operations to dedicated AILinux prefix/backup roots. No system
APT modifications, unrestricted software sources, or MS-DLL redistribution.

## Install a disposable Windows 11 AppCompat prefix

The PowerShell orchestrator creates a new isolated Wine prefix, initializes
Win11 mode, creates a verified snapshot, installs pinned DXVK/VKD3D-Proton
and the current official Microsoft VC++ v14 x86/x64 redistributables using
Xvfb. Optional Winetricks extras include d3dcompiler_47, gdiplus, and msxml6.

    pwsh -NoProfile -File ./ailinux/powershell/Install-AILinuxWineAppCompat.ps1 -WhatIf
    pwsh -NoProfile -File ./ailinux/powershell/Install-AILinuxWineAppCompat.ps1 -Confirm:$false

Use -IncludeExtras only after reviewing the required Microsoft licensing and
runtime dependencies. Do not combine Framework 4.8 with all prefixes by
default; managed applications should receive their own tested prefix.

The initial integrated installation on zombie-pc (2026-10-25) completed with
DXVK/VKD3D and VC++ x86/x64 verified, and Winetricks extras completed with
exit code zero. A later Winetricks list-installed check encountered a separate
wineserver protocol mismatch (964/965); the six DLL files for extras were
present on disk, but their behavior still requires regression testing.

## Runtime profile catalog

`ailinux/profiles/runtime-catalog.json` contains isolated installable profiles:
fonts (Corefonts/Tahoma), legacy-games (D3DX9/XACT), office-compat (GDI+/MSXML6/
D3DCompiler), legacy-apps (VC++ 2010/2012/2013 and legacy multimedia),
dotnet48, and optional allfonts. Run explicitly through PowerShell:

    pwsh -NoProfile -File ./ailinux/powershell/Install-AILinuxRuntimeProfile.ps1 -Profile fonts -WhatIf
    pwsh -NoProfile -File ./ailinux/powershell/Install-AILinuxRuntimeProfile.ps1 -Profile fonts -Confirm:$false

Every invocation takes an isolated Prefix snapshot first; optional .NET and
allfonts need a dedicated prefix unless isolation checks are explicitly
bypassed. Never conflate DLL presence with tested application compatibility.

On 2026-10-25 the fonts profile completed on win11-appcompat, and the
legacy-games profile completed successfully in legacy-games-alpha including
x86/x64 D3DX9_43, XACT3_7 and XAudio2_7 files. Win32/Win64 API regressions
passed. The legacy-apps profile stopped at vcrun2010 because Microsoft
vcredist_x86.exe returned exit code 2 in a headless prefix. No remaining
verbs were executed. The snapshot is preserved. This profile is NOT marked
installed; use Xvfb diagnostics and targeted installer tests before retrying.
