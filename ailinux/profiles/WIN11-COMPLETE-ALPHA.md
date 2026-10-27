# Windows 11 application-compatibility prefix, 25 October 2026

Built on zombie-pc using native Linux PowerShell 7.6.9 and our own Wine 11.20.
Prefix: ~/.local/share/ailinux-wine/prefixes/win11-complete-alpha

Installed using Install-AILinuxWineAppCompat.ps1 -IncludeExtras:
- DXVK 3.1.1 (x86 and x64)
- VKD3D-Proton 3.0.1 (x86 and x64)
- Official Microsoft Visual C++ v14 runtime x86 and x64, Burn logs checked
- d3dcompiler_47, gdiplus, msxml6 via pinned Winetricks 20260125
- fonts profile: corefonts, Tahoma and RGB font smoothing

Before runtime installation, PowerShell created a verified baseline snapshot;
additional backups were created before Microsoft and Winetricks operations.

Verification passed:
- cmd /c ver reports Windows 10.0.22000 (Wine's Windows-11 compatibility mode)
- native VC++ v14 dynamic API probes on Win32 and Win64
- Windows AppModel, WinRT HSTRING, Media Foundation startup, DPI, WinHTTP: 5/5 each
- DXGI + Direct3D11 + Direct3D12 hardware device creation on both PE architectures
- PowerShell compliance and critical registry/runtime integrity checks

This is a fully provisioned Windows-11-oriented Wine application prefix,
NOT a full install of Windows 11. Windows kernel drivers, native UWP/WinUI3,
anti-cheat drivers, Windows Update, and complete WinRT/Windows App SDK
compatibility are not provided by Wine. Not all Windows applications work.
.NET Framework 4.8 was tested in a separate prefix and intentionally omitted
here to avoid destabilizing the Windows 11 runtime profile.

Re-run tests from the repository:

    ~/.local/opt/ailinux-powershell/7.6.9/pwsh -NoProfile -File ailinux/powershell/Test-AILinuxWineComplete.ps1

Do not replace /usr/bin/wine or ~/.wine. Rollback through verified snapshots
in AILinux.Wine.psm1, with Wine processes stopped.
