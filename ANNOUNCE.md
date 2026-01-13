The Wine team is proud to announce that the stable release Wine 11.0
is now available.

This release represents a year of development effort, around 6,300
individual changes, and more than 600 bug fixes. It contains a large
number of improvements that are listed below. The main highlights are
the NTSYNC support and the completion of the new WoW64 architecture.

The source is available at <https://dl.winehq.org/wine/source/11.0/wine-11.0.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-11.0/AUTHORS

----------------------------------------------------------------

## What's new in Wine 11.0

### WoW64

- The _new WoW64_ mode that was first introduced as experimental feature in
  Wine 9.0 is considered fully supported, and essentially has feature parity
  with the old WoW64 mode.

- 16-bit applications are supported in the new WoW64 mode.

- It is possible to force an old WoW64 installation to run in new WoW64 mode
  by setting the variable `WINEARCH=wow64`. This requires the prefix to have
  been created as 64-bit (the default).

- Pure 32-bit prefixes created with `WINEARCH=win32` are deprecated, and are
  not supported in new WoW64 mode.

- The `wine64` loader binary is removed, in favor of a single `wine` loader
  that selects the correct mode based on the binary being executed. For
  binaries that have both 32-bit and 64-bit versions installed, it defaults
  to 64-bit. The 32-bit version can then be launched with an explicit path,
  e.g. `wine c:\\windows\\syswow64\\notepad.exe`.


### Synchronization / Threading

- The NTSync Linux kernel module is used when available, to improve the
  performance of synchronization primitives. The needed kernel module is
  shipped with the Linux kernel starting from version 6.14.

- Thread priority changes are implemented on Linux and macOS.  On Linux,
  this is constrained by the system nice limit, and current distributions
  require some configuration to change the nice hard limit to a negative
  value (in the -19,-1 range, where -5 is usually enough, and anything lower
  is not recommended). See `man limits.conf(5)` for more information.

- NTDLL synchronization barriers are implemented.

- On macOS, the `%gs` register is swapped in the syscall dispatcher.  This
  avoids conflicts between the Windows TEB and the macOS thread descriptor.


### Kernel

- NT Reparse Points are implemented, with support for the mount point and
  symlink types of reparse points.

- Write Watches take advantage of userfaultfd on Linux if available, to
  avoid the cost of handling page faults in user space.

- NT system calls use the same syscall numbering as recent Windows, to
  support applications that hardcode syscall numbers.

- On ARM64, there is support for simulating a 4K page size on top of larger
  host pages (typically 16K or 64K). This works for simple applications, but
  because it is not possible to completely hide the differences, more
  demanding applications may not work correctly. Using a 4K-page kernel is
  strongly recommended.


### Graphics

- The OSMesa dependency is removed, and OpenGL bitmap rendering is
  implemented with the hardware accelerated OpenGL runtime.

- The EGL OpenGL backend is extended, and used by default on the X11
  platform. The GLX backend is deprecated but remains available, and is used
  as fallback if EGL isn't available. It can also be forced by setting the
  value `UseEGL=N` in the `HKCU\Software\Wine\X11 Driver` registry key.

- The `VK_KHR_external_memory_win32`, `VK_KHR_external_semaphore_win32`,
  `VK_KHR_external_fence_win32`, `VK_KHR_win32_keyed_mutex` extensions and
  the related D3DKMT APIs are implemented.

- In new WoW64 mode, OpenGL buffers are mapped to 32-bit memory space using
  Vulkan extensions if available.

- Front buffer OpenGL rendering is emulated for platforms that don't support
  it natively.

- OpenGL context sharing implementation in wglShareLists is improved.

- The Vulkan API version 1.4.335 is supported.

- Image metadata handling is better supported in WindowsCodecs.

- Many more conversions between various pixel formats are supported in
  WindowsCodecs.


### Desktop integration

- X11 Window Manager integration is improved: window activation requests are
  sent to the Window Manager, and the EWMH protocol is used to keep the X11
  and the Win32 active windows consistent.

- Exclusive fullscreen mode is supported, and D3D fullscreen mode is
  improved, especially improving older DDraw games.

- Shaped and color-keyed windows are supported in the experimental Wayland
  driver.

- Performance of several windowing-related functions is improved, using
  shared memory for communication between processes.

- Clipboard support is implemented in the Wayland driver.

- Input Methods are supported in the Wayland driver.


### Direct3D

- Hardware decoding of H.264 video through Direct3D 11 video APIs is
  implemented over Vulkan Video. Note that the Vulkan renderer must be used.
  As in previous Wine versions, the Vulkan renderer can be used by setting
  `renderer` to `vulkan` using the `Direct3D` registry key or
  `WINE_D3D_CONFIG` environment variable.

- Direct3D 11 sampler minimum/maximum reduction filtering is implemented if
  `GL_ARB_texture_filter_minmax` is available (when using the GL renderer)
  or `VK_EXT_sampler_filter_minmax` (when using the Vulkan renderer).

- The following legacy Direct3D features are implemented for the Vulkan
  renderer:
  - Point size control.
  - Point sprite control.
  - Vertex blending.
  - Fixed-function bump mapping.
  - Color keying in draws.
  - Flat shading.
  - Alpha test.
  - User clip planes.
  - Several resource formats.

  Additionally, the bundled copy of vkd3d-shader includes many improvements
  for Shader Model 1, 2, and 3 shaders, including notably support for Shader
  Model 1 pixel shaders and basic Shader Model 1 texturing.  The Vulkan
  renderer is not yet at parity with the GL renderer, and is therefore not
  yet the default.


### Direct3D helper libraries

- `D3DXSaveSurfaceToFileInMemory` is reimplemented for PNG, JPEG and BMP
  files, enabling support for formats and other edge cases not supported by
  WindowsCodecs. It also supports saving surfaces to TARGA files.

- D3DX 11 texture loading functions are implemented, using code shared with
  earlier D3DX versions.

- Box filtering is supported in all versions.

- `D3DXSaveTextureToFileInMemory` supports saving textures to DDS files.

- D3DX 9 supports reading 1-bit, 2-bit, and 4-bit indexed pixel formats, as
  well as the CxV8U8 format.

- D3DX 10 and 11 support compressing and decompressing BC4 and BC5 formats.

- D3DX 10 and 11 support generating mipmap levels while loading textures.

- `ID3DXEffect::SetRawValue()` is partially implemented.

- `ID3DXSkinInfo::UpdateSkinnedMesh()` is implemented.


### Input / HID devices

- Compatibility with more Joystick devices is improved through the `hidraw`
  backend. Per-vendor and per-device registry options are available to
  selectively opt into the hidraw backend.

- Force feedback support is improved, with increased compatibility for
  joysticks and driving wheels, and better performance.

- Better support for gamepads in the Windows.Gaming.Input API and with the
  evdev backend when SDL is not available or disabled.

- There is a configuration tab for the Windows.Gaming.Input API in the Game
  Controllers Control Panel applet.

- DirectInput compatibility with older games that use action maps and device
  semantics is improved.

- More device enumeration APIs from Windows.Devices.Enumeration and cfgmgr32
  are implemented.


### Bluetooth

- The Bluetooth driver supports scanning and configuring host device
  discoverability, with some basic support for pairing via both the API and
  a wizard. At this point, this is only supported on Linux systems using
  BlueZ.

- Bluetooth radios and devices (both classic and low-energy) are visible to
  Windows applications.

- Applications can make low-level RFCOMM connections to remote devices using
  winsock APIs.

- There is initial support for Bluetooth Low Energy (BLE) Generic Attribute
  Profile (GATT) services and characteristics, making them visible through
  the Win32 BLE APIs.


### Scanner support

- `DAT_IMAGENATIVEXFER` is supported.

- Scanner selection and configuration are saved in the registry.

- TWAIN 2.0 API for scanning is implemented, which allows scanning to work
  in 64-bit applications.

- Multi-page and Automatic Document Feed scans are supported.

- There is a user interface showing scanning progress and error messages.

- The scanner user interface no longer blocks the application using it.

- Windows-native scanner drivers can be loaded if they're installed in Wine.


### Multimedia

- The Multimedia Streaming library implements a custom allocator for
  DirectDraw streams, reducing the number of buffer copies required for
  filters which support a downstream custom allocator.

- Dynamic format change is supported in the DMO Wrapper, AVI Decoder, and
  GStreamer-based demuxer and transform filters.

- GStreamer-based demuxer filters support the Indeo 5.0 codec.

- The DirectSound Renderer filter more properly signals end-of-stream.
  Previously end-of-stream could be signaled too early, clipping the end of
  an audio stream.

- The ASF Reader filter supports seeking.

- The AVI Decoder filter supports nontrivial source and destination
  rectangles.


### DirectMusic

- SoundFont(SF2) supports more features:
  - Parsing of preset, instrument and default modulators.
  - Layering support required for many SF2 instruments.
  - Reuse of downloaded waves and zero-copy access sample data to prevent
    out-of-memory errors.
  - Instrument normalization.

- The Synthesizer is improved:
  - The latency clock is derived from the master clock to fix uneven
    playback in certain tracks.
  - Voice shutdown is instant and the synth better handles channel pressure
    events and LFO connections.
  - Setting the volume is supported and is automatically done when creating
    a synth or adding a port.

- The DX7 version of the Style form is supported.

- Cache management improvements in the loader.

- More MIDI meta events are supported.


### Mono / .NET / WinRT

- XNA4 applications run based on SDL3, and render using the new SDL_GPU API
  by default.

- A text layout engine supporting System.Windows.Documents APIs is added to
  WPF (Windows Presentation Framework).

- Theming works in Windows Forms.

- WinRT metadata files can be generated by `widl`, and there is an initial
  implementation of the loader classes.

- WinRT C++ exceptions are supported.


### Internationalization

- Locale data is generated from the Unicode CLDR database version 48. The
  following additional locales are supported: `bqi-IR`, `bua-RU`, `cop-EG`,
  `ht-HT`, `kek-GT`, `lzz-TR`, `mww-Hmnp-US`, `oka-CA`, `pi-Latn-GB`,
  `pms-IT`, `sgs-LT`, `suz-Deva-NP`, and `suz-Sunu-NP`,

- Unicode character tables are based on version 17.0.0 of the Unicode
  Standard.

- The timezone data is based on version 2025a of the IANA timezone database.


### Internet and networking

- MSHTML exposes DOM attributes as proper DOM nodes in standards-compliant
  mode.

- JavaScript typed arrays are supported.

- The MSHTML objects DOMParser, XDomainRequest and msCrypto are implemented.

- Ping is implemented for ICMPv6.


### Databases

- MSADO supports writing changes to the database.

- Most of the MSADO Recordset functions are implemented.

- ODBC remaps Unicode strings to support ANSI-only Win32 drivers.


### Debugging

- The PDB file loader in DbgHelp is reimplemented, to support large files
  (> 4G), faster loading, and use fewer memory resources.

- NT system calls can be traced with `WINEDEBUG=syscall`. Unlike
  `WINEDEBUG=relay`, this is transparent to the application, and avoids
  breaking applications that hook system call entry points.

- It is possible to generate both DWARF and PDB debug information in a
  single build.


### Builtin applications

- The Audio tab of WineCfg allows configuring the default MIDI device.

- The Command Prompt tool `cmd` can create reparse points with `mklink /j`,
  and display them in directory listings.

- The Command Prompt tool `cmd` supports more complex instructions, and file
  name auto completion in interactive prompt.

- The Console Hosting application `conhost` supports F1 and F3 keys for
  history retrieval.

- The `timeout` application is implemented.

- The `find` tool supports options `/c` (display match count) and `/i` (case
  insensitive matches).

- The `whoami` tool supports output format specifiers.

- There is a basic implementation of the `subst` command

- There is an initial implementation of the `runas` tool.


### Miscellaneous

- Common Controls version 5 and version 6 are fully separated DLLs, and
  v6-only features are removed from the v5 DLL for better compatibility.

- The PBKDF2 key derivation algorithm is supported in BCrypt.

- The well-known shell folders `UserProgramFiles`, `AccountPictures` and
  `Screenshots` are supported.


### Development tools

- The IDL compiler can generate Windows Runtime metadata files (`.winmd`)
  with the `--winmd` option

- The `winedump` tool supports dumping MUI resources, syscall numbers,
  embedded NE modules, and large PDB files (>4G).

- The `wine/unixlib.h` header is installed as part of the development
  package, as a first step towards supporting use of the Unixlib interface
  in third-party modules. This is still a work in progress.


### Build infrastructure

- The X11-derived `install-sh` script is reimplemented in C, to enable
  installing several files in a single program invocation. This speeds up
  the file copying phase of `make install` by an order of magnitude.

- Compiler exceptions are used to implement `__try/__except` blocks when
  building with Clang for 64-bit MSVC targets.

- The WineHQ Gitlab CI supports ARM64 builds.


### Bundled libraries

- The LLVM Compiler-RT runtime library version 8.0.1 is bundled, and used
  when building modules in MSVC mode.

- The TomCrypt library version 1.18.2 is bundled and used to implement
  cryptographic primitives in the RsaEnh and BCrypt modules.

- Vkd3d is updated to the upstream release [1.18][4].

- Faudio is updated to the upstream release 25.12.

- FluidSynth is updated to the upstream release 2.4.2.

- LCMS2 is updated to the upstream release 2.17.

- LibMPG123 is updated to the upstream release 1.33.0.

- LibPng is updated to the upstream release 1.6.51.

- LibTiff is updated to the upstream release 4.7.1.

- LibXml2 is updated to the upstream release 2.12.10.

- LibXslt is updated to the upstream release 1.1.43.

[4]: https://gitlab.winehq.org/wine/vkd3d/-/releases/vkd3d-1.18


### External dependencies

- The OSMesa library is no longer used. OpenGL bitmap rendering is
  implemented using EGL instead.

- The HwLoc library is used for CPU detection on FreeBSD.
