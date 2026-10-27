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

## DXVK and VKD3D-Proton, opt-in and reversible

ailinux/runtimes.lock.json pins official GitHub release URLs and SHA-256
asset digests. No compiled DXVK or VKD3D DLLs are committed to this repo.

Do not modify any prefix while Windows programs are running. Create a new,
**isolated** prefix with your chosen Wine binary first:

    export ALPHA_WINE="$HOME/.local/opt/ailinux-wine/alpha-0.1.0/bin/wine"
    export ALPHA_PREFIX="$HOME/.local/share/ailinux-wine/prefixes/my-game"
    WINEPREFIX="$ALPHA_PREFIX" WINEARCH=win64 "$ALPHA_WINE" wineboot -u

Then, after the prefix system.reg exists:

    python3 -B ailinux/runtime-manager.py --prefix "$ALPHA_PREFIX" install
    python3 -B ailinux/runtime-manager.py --prefix "$ALPHA_PREFIX" status
    # Only this launch receives native DLL overrides:
    python3 -B ailinux/runtime-manager.py --prefix "$ALPHA_PREFIX" --wine "$ALPHA_WINE" run -- "C:\Games\MyGame\game.exe"
    # Restore the original Wine DLLs:
    python3 -B ailinux/runtime-manager.py --prefix "$ALPHA_PREFIX" remove

The runtime manager requires a preinitialized prefix and rejects ~/.wine.
It checks GitHub-provided SHA-256 release digests and verifies the PE machine
architecture before writing DLLs. Backups remain under the prefix in
.ailinux-wine/runtime-manager/, including original Wine file symlinks.
Uninstall refuses to overwrite externally modified runtime files.
Installation does not alter Wine registry values: WINEDLLOVERRIDES is scoped to
the run command. Test Vulkan drivers and actual applications before releases.

Offline tests:

    python3 -B -m unittest discover -s ailinux/tests -p 'test_*.py' -v

Official component releases:
- DXVK 3.1.1: https://github.com/doitsujin/dxvk/releases/tag/v3.1.1
- VKD3D-Proton 3.0.1: https://github.com/HansKristian-Work/vkd3d-proton/releases/tag/v3.0.1

VKD3D-Proton requires a capable Vulkan driver and DXGI implementation,
which DXVK provides. Neither component installs proprietary Microsoft VC++,
.NET, DirectX redistributables or other Microsoft code; obtain those only
from the vendor subject to the applicable license and terms.

## Direct3D hardware smoke tests (real AMDGPU/Vulkan, not headless CI)

The optional graphics-probe.c client calls DXGI CreateDXGIFactory1,
D3D11CreateDevice(HARDWARE), and D3D12CreateDevice with a real adapter.
It prints raw HRESULT values and returns nonzero on any failure.
Compile and run the x86/x64 probes on a Vulkan-capable host:

    export WINEPREFIX="$ALPHA_PREFIX"
    export WINEDLLOVERRIDES='d3d8=n;d3d9=n;d3d10core=n;d3d11=n;dxgi=n;d3d12=n;d3d12core=n'
    ./ailinux/run-graphics-probe.sh "$ALPHA_WINE"

The tests do not render frames or exercise games, so successful creation alone
does not prove shader correctness, performance or 100% DirectX compatibility.
As of 2026-10-25 the separate system Wine-Staging 11.20 baseline successfully
created both D3D11 and D3D12 devices through DXVK 3.1.1 + VKD3D-Proton 3.0.1
on the local AMD GPU in both PE architectures. The *AILinux-built* Wine engine
must be tested again before any release.
