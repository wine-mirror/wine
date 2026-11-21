# Runtime qualification results, 25 October 2026

| Profile | State | Evidence |
|---|---|---|
| windows11-appcompat | Installed | VC++ v14 x86/x64 vendor Burn logs, DXVK/VKD3D state, system integrity report |
| fonts | Installed | corefonts, tahoma, fontsmooth verb completed |
| legacy-games | Installed | d3dx9/xact installation and DLL presence; full game rendering not tested |
| dotnet48 | Verified in dedicated existing prefix | Windows x86/x64 C# compilation, CLR 4.0.30319.42000, SHA256 and Decimal.Round tests |
| legacy-apps | Blocked | vcrun2010 installer returned error; snapshot rollback reported success |
| allfonts | Not verified | Not enabled by default |

Dotnet48 requires older Windows-version compatibility during setup: its test
prefix currently reports Windows 7. Do not silently switch that test prefix
to Windows 11 or claim Win11 application compatibility without retesting.

A new core Wine patch is not justified by the legacy vcrun2010 installer
failure alone. Compare a fresh Xvfb-backed prefix, inspect vendor logs, and
verify Windows user-interface/installer requirements before patching Wine.

No Microsoft redistributable files or proprietary Windows DLLs are committed.
