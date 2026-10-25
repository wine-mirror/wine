# AILinux Wine lifecycle: Alpha -> Beta -> RC -> Release

## Purpose

A complete, license-compliant Windows-API compatibility environment for
Ubuntu/KDE and modern Vulkan-capable AMD hardware, built from Wine, targeted
Wine-Staging fixes, DXVK, VKD3D-Proton, and opt-in vendor redistributables.

Do not confuse *DLL export presence* with *Windows compatibility*. Each
subsystem requires behavioral checks, real applications, error telemetry,
regression coverage and a tested uninstallation path.

## Current 0.1.0-alpha.1

- Source baseline: upstream Wine development 11.20, forked by derleiti.
- Separate multiarch PE build: i386 and x86_64, cross-compiler C++17.
- Optional Wine-Staging v11.20 patchset (278 patch applications) in separate
  experimental branch until its own build/functional checks succeed.
- Optional DXVK 3.1.1 and VKD3D-Proton 3.0.1, SHA-256 pinned,
  prefix-local and fully reversible without registry modification.
- Baseline export coverage: 24/24 x86 + 24/24 x64 in system Wine.
- Baseline functional smoke: 6/6 x86 + 6/6 x64 in system Wine.
- GPU smoke: DXGI, D3D11, D3D12 hardware creation successful in BOTH
  PE architectures on Radeon RX 7700 XT with DXVK/VKD3D on system Wine.
- CI: static checks, Win32/Win64 PE probe builds and runtime-manager rollback
  tests, not a full graphics/runtime test.

### Alpha acceptance gates

1. Vanilla Wine engine configure, compilation, install and version check.
2. Vanilla own-engine Wine prefix initializes, Windows command smoke passes.
3. Own-engine DLL export and behavioral probe regression >= system baseline.
4. Wine-Staging compiled separately, identical regressions plus selected
   native Wine dlls and modified behaviors verified.
5. No patch application failures; all patch licenses and notices preserved.
6. DXVK/VKD3D installed on isolated prefix: x86+x64 DXGI/D3D11/D3D12 tests.
7. Verify GPU, audio, Wayland/X11 in actual KDE desktop session, not just SSH.
8. Check source diff/commit, no secrets, no proprietary Windows DLLs bundled.
9. Rollback test restores original files exactly, default Wine untouched.
10. Publish an alpha *only after* the above tests pass, with limitations noted.

## 0.2.0-beta

- Targeted Windows API compatibility gaps from reproducible test failures.
- GPU device/render workloads: D3D8/9/10/11/12, DXGI, WSI, shader-cache.
- Windows application matrix: games, installers, productivity, browsers,
  WebView2 dependencies, C++ runtimes, Wine Mono/Gecko, audio and DRM.
- Input matrix: keyboard, mouse/rawinput, controllers, DPI/multi-monitor,
  KDE/Wayland and X11 fallback.
- Detect missing system libraries such as VA-API, smartcard, CUPS, OpenCL,
  Kerberos and add only libraries supported by the target distro.
- Establish performance benchmarks and compare same prefixes/drivers.

## 0.9.0-rc1

- Feature freeze, patch provenance audit, security and regression testing.
- Reproducible build of *relocatable or fixed-prefix packaging*; no publishing
  a build configured for an arbitrary developer home path as a system package.
- Verified dependency metadata, checksums, build logs, release notes, rollback.
- Fresh VM and existing AILinux installation tests, including removal/upgrade.
- Per-architecture Wine prefix and Vulkan loader consistency checks.

## 1.0.0 release

- Sign versioned AILinux package(s), upload repository with metadata and
  public downloads, verify checksums, install on clean AILinux image.
- Verify new installation, upgrade/rollback, real Windows application matrix.
- No overriding /usr/bin/wine or the default Wine prefix by surprise.
- Publish source/patch attribution, license notices and test evidence.

## Compatibility priorities

The first three priorities are reliability, clean-room licensing and data
isolation. Do not copy Microsoft Windows implementation/source code into
Wine unless it is specifically published under a compatible license, and
avoid bypassing anti-cheat or software licensing controls. Prefer Wine
upstream bug reports with minimal Win32 reproducers and upstreamable patches.
