# Microsoft VC++ v14 installer issue under Wine 11.20

Observed on zombie-pc on 2026-10-25 in a separate Wine prefix.
Official Microsoft vc_redist.x86.exe and vc_redist.x64.exe 14.51.36247.0.
The Wine launcher returned code 130 for both bundles, while their internal Burn logs show exit 0x582 (decimal 1410). The discrepancy must be investigated; neither is success.
Both dd_vcredist logs report:

- 0x80004005 Failed to create window
- 0x80070582 Failed to load theme controls
- 0x80070582 Failed to create main window

The x64 registry path HKLM/Software/Microsoft/VisualStudio/14.0/VC/Runtimes/x64
was present at version 14.51.36247.00; x86 was not registered. This x64
registry entry is NOT evidence the installer succeeded and may originate from
Wine registration. Investigate Wine UI/theme resource handling in Burn bootstrapper.

Next reproduction: a freshly initialized dedicated prefix in the interactive
KDE/Wayland session, ideally under a nested virtual desktop, logging Win32
CreateWindowExW, FindResource/LoadResource and theme failures. Compare system
Wine Staging and our own Wine; upstream bug to be filed only after isolation.

Never copy Microsoft's DLLs into git or claim a successful install from
file presence alone. Keep installer logs and SHA256 of vendor binaries.

The WiX Toolset upstream documents that Failed to load theme controls can also occur with missing or invalid theme assets, including on Windows. Therefore the log alone does NOT establish a Wine API implementation bug. Compare behavior on native Windows, and inspect theme resources, error 0x80070582, and Win32 window creation before submitting any Wine patch.
