# AILinux Wine build and qualification

## Status and policy
- Base: Wine 11.20 source snapshot (VERSION is authoritative).
- This fork is **experimental** until its own binaries complete install and smoke tests.
- The existing /usr/bin/wine and ~/.wine MUST remain unmodified.
- Isolated prefix: $HOME/.local/opt/ailinux-wine/alpha-0.1.0.
- Out-of-tree build: $HOME/workspace/wine-build-alpha-wow64.
- 32/64 bit PE support: --enable-archs=i386,x86_64 with both MinGW cross-compilers (C and C++17, POSIX variants).
- System wine --version checks do not prove the fork was built.

## Local build
Run ./ailinux/preflight.sh and install missing development packages.
Explicit MinGW POSIX C/C++ drivers avoid mixing win32/posix cross-compiler runtimes.
For the KDE/Wayland driver, Wine 11.20 also requires xkbregistry (libxkbregistry-dev).
The build automatically generates private MinGW POSIX compiler symlinks in the build tree: Wine configure relies on conventional executable suffixes ending in -gcc.
From this repository run AILINUX_WINE_JOBS=4 ./ailinux/build-alpha.sh.
Build scripts do not update apt sources, modify system Wine or use sudo.
To smoke-test the new build, run:
WINEPREFIX="$HOME/.local/share/ailinux-wine/prefixes/alpha-smoke-built" ./ailinux/smoke.sh "$HOME/.local/opt/ailinux-wine/alpha-0.1.0/bin/wine"
For API export coverage, run ./ailinux/run-library-probe.sh with
the absolute path to the built Wine binary. Missing optional APIs
are reported and should guide targeted compatibility changes.

## Optional runtime integration, later milestones
- Direct3D 8-11: DXVK (external version-pinned component).
- Direct3D 12: VKD3D-Proton (external version-pinned component).
- .NET: Wine-Mono or licensed Microsoft redistributables as needed.
- WebView/HTML: Wine-Gecko.
- MSVC: vendor redistributables obtained from Microsoft's permitted download channels.
- Low-level API changes: narrow Wine upstream patches with tests and review.

Sources:
- https://gitlab.winehq.org/wine/wine/-/wikis/Building-Wine
- https://gitlab.winehq.org/wine/wine-staging
- https://github.com/doitsujin/dxvk
- https://github.com/HansKristian-Work/vkd3d-proton
- https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist

Do not commit proprietary DLLs or download redistributables from third-party mirrors.
Do not integrate unreviewed ReactOS code; evaluate license and provenance.
