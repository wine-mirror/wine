# AILinux Wine Windows 11 compatibility profile

Wine 11.20 supports win11 in winecfg; the version reported by this upstream
mode is Windows 10.0 build 22000. That is **not** a Windows 11 26H2 API layer.
Do not edit Wine build-number constants merely to fool applications. Missing
APIs require tests and correct implementations based on published API contracts.

## Isolated alpha installation

```sh
cd ~/workspace/wine
export AILINUX_WINE_BIN="$HOME/.local/opt/ailinux-wine/alpha-0.1.0/bin/wine"
export AILINUX_WINE_PREFIX_DIR="$HOME/.local/share/ailinux-wine/prefixes/ailinux-win11"
./ailinux/win11-profile.sh init
./ailinux/win11-profile.sh verify
./ailinux/win11-profile.sh install-graphics
```

## Runtimes (install separately, into a disposable prefix first)

```sh
./ailinux/win11-profile.sh install-fonts
./ailinux/win11-profile.sh install-corefonts
# Only for applications that genuinely need many Microsoft-compatible fonts:
./ailinux/win11-profile.sh install-allfonts
./ailinux/win11-profile.sh install-vcrun2022
./ailinux/win11-profile.sh install-dotnet48
./ailinux/win11-profile.sh install-extras
```

The actions above create a Btrfs-friendly copy of the entire prefix before
running Winetricks (backup path is printed). Use separate prefixes for .NET,
for games and for applications where installers conflict. Dotnet48/VC++/fonts
may download Microsoft installers; this code never bundles their DLLs or
redistributes them. Check licenses and installation logs before release.

VC++ 2015-2022 and newer v14 runtimes are binary compatible for many apps;
Microsoft now also supports 2017-2026 v14 from current official downloads.
Winetricks vcrun2022 is an application-compatibility recipe, not necessarily
the latest vendor v14 build. Prefer published Microsoft source URLs rather
than third-party DLL bundles; validate per-architecture install versions.

.NET Framework 4.8 is not .NET 8/9/10. Wine-Mono can handle some .NET Framework
apps, but Microsoft .NET Framework 4.8 may be required for specific programs.
Do not assume installing both gives complete Windows compatibility.

Current Windows 11 releases (Microsoft):
https://learn.microsoft.com/windows/release-health/windows11-release-information
VC++ redistributables:
https://learn.microsoft.com/cpp/windows/latest-supported-vc-redist
Winetricks:
https://github.com/Winetricks/winetricks

## Additional compatibility roadmap

- DirectX: verified DXVK 3.1.1 and VKD3D-Proton 3.0.1 x86/x64 GPU device
  creation on zombie-pc against the own-built Wine 11.20 engine.
- Fonts: prefer installed Linux Noto/Liberation/Carlito and Wine fontconfig;
  Winetricks Microsoft-font install is opt-in, not copied into Git.
- Windows Media Foundation, WinHTTP, WinINet, Schannel, BCrypt, COM/OLE,
  MSXML, D3DCompiler, HID/RawInput, XAudio, WASAPI require behavior tests.
- .NET desktop, WebView2, WinUI 3, Windows App SDK are **separate** platform
  dependencies and are not made automatically compatible by the win11 flag.
- Windows kernel drivers and anti-cheat cannot be translated by a normal Wine
  userspace DLL; do not claim Windows NT kernel equivalence.

## Current Microsoft VC++ v14 (2026)

The currently tested official aka.ms/vc14 URLs redirect to Visual Studio 18
runtime installers, rather than the fixed 2024 binaries embedded in old
Winetricks recipes. A separate `ailinux/vc14-manager.sh` invokes official x86
and x64 installers in an *isolated prefix*, saving a full copy first.

**Important:** In a fresh test prefix both vendor installers returned exit code
130. The x64 registry contained version 14.51.36247.00, but x86 registration
was absent. That is not a clean successful installation: do not treat it as a
validated package. See the prefix-specific registry and vendor setup logs.
Use `./ailinux/vc14-manager.sh status` to inspect, and `install` only on
an initialized disposable prefix. Keep its snapshot for rollback.

**.NET Framework 4.8:** Winetricks may downgrade its configured Wine OS mode
and remove Wine-Mono; install to a separate test prefix and verify with actual
.NET applications, not only a marker file. No .NET 4.8 install is claimed yet.
