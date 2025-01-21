The Wine team is proud to announce that the stable release Wine 10.0
is now available.

This release represents a year of development effort and over 6,000
individual changes. It contains a large number of improvements that
are listed below. The main highlights are the new ARM64EC
architecture and the high-DPI scaling support.

The source is available at <https://dl.winehq.org/wine/source/10.0/wine-10.0.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-10.0/AUTHORS

## What's new in Wine 10.0

### ARM64

- The ARM64EC architecture is fully supported, with feature parity with the
  ARM64 support.

- Hybrid ARM64X modules are fully supported. This allows mixing ARM64EC and
  plain ARM64 code into a single binary. All of Wine can be built as ARM64X
  by passing the `--enable-archs=arm64ec,aarch64` option to configure. This
  still requires an experimental LLVM toolchain, but it is expected that the
  upcoming LLVM 20 release will be able to build ARM64X Wine out of the box.

- The 64-bit x86 emulation interface is implemented. This takes advantage of
  the ARM64EC support to run all of the Wine code as native, with only the
  application's x86-64 code requiring emulation.

  No emulation library is provided with Wine at this point, but an external
  library that exports the emulation interface can be used, by specifying
  its name in the `HKLM\Software\Microsoft\Wow64\amd64` registry key. The
  [FEX emulator][4] implements this interface when built as ARM64EC.

- It should be noted that ARM64 support requires the system page size to be
  4K, since that is what the Windows ABI specifies. Running on kernels with
  16K or 64K pages is not supported at this point.

[4]: https://fex-emu.com


### Graphics

- High-DPI support is implemented more accurately, and non-DPI aware windows
  are scaled automatically, instead of exposing high-DPI sizes to
  applications that don't expect it.

- Compatibility flags are implemented to override high-DPI support, either
  per-application or globally in the prefix.

- Vulkan child window rendering is supported with the X11 backend, for
  applications that need 3D rendering on child windows. This was supported
  with OpenGL already, and the Vulkan support is now on par.

- The Vulkan driver supports up to version 1.4.303 of the Vulkan spec. It
  also supports the Vulkan Video extensions.

- Font linking is supported in GdiPlus.


### Desktop integration

- A new opt-in modesetting emulation mechanism is available. It is very
  experimental still, but can be used to force display mode changes to be
  fully emulated, instead of actually changing the display settings.

  The windows are being padded and scaled if necessary to fit in the
  physical display, as if the monitor resolution were changed, but no actual
  modesetting is requested, improving user experience.

- A new Desktop Control Panel applet `desk.cpl` is provided, to inspect and
  modify the display configuration. It can be used as well to change the
  virtual desktop resolution, or to control the new emulated display
  settings.

- Display settings are restored to the default if a process crashes without
  restoring them properly.

- System tray icons can be completely disabled by setting `NoTrayItemsDisplay=1`
  in the `HKLM\Software\Microsoft\Windows\CurrentVersion\Policies\Explorer`
  key.

- Shell launchers can be disabled in desktop mode by setting `NoDesktop=1`
  in the `HKLM\Software\Microsoft\Windows\CurrentVersion\Policies\Explorer`
  key.


### Direct3D

- The GL renderer now requires GLSL 1.20, `EXT_framebuffer_object`, and
  `ARB_texture_non_power_of_two`. The legacy ARB shader backend is no longer
  available, and the `OffscreenRenderingMode` setting has been removed.

- Shader stencil export is implemented for the GL and Vulkan renderers.

- A HLSL-based fixed function pipeline for Direct3D 9 and earlier is
  available, providing support for fixed function emulation for the Vulkan
  renderer. It can also be used for the GL renderer, by setting the D3D
  setting `ffp_hlsl` to a nonzero value using the registry or the
  `WINE_D3D_CONFIG` environment variable.

- The Vulkan renderer uses several dynamic state extensions, if available,
  with the goal of reducing stuttering in games.

- An alternative GLSL shader backend using vkd3d-shader is now available,
  and can be selected by setting the D3D setting `shader_backend` to
  `glsl-vkd3d`. Current vkd3d-shader GLSL support is incomplete relative to
  the built-in GLSL shader backend, but is being actively developed.


### Direct3D helper libraries

- Initial support for compiling Direct3D effects is implemented using
  vkd3d-shader.

- D3DX 9 supports many more bump-map and palettized formats.

- D3DX 9 supports saving palettized surfaces to DDS files.

- D3DX 9 supports mipmap generation when loading volume texture files.

- D3DX 9 supports reading 48-bit and 64-bit PNG files.


### Wayland driver

- The Wayland graphics driver is enabled by default, but the X11 driver
  still takes precedence if both are available. To force using the Wayland
  driver in that case, make sure that the `DISPLAY` environment variable is
  unset.

- Popup windows should be positioned correctly in most cases.

- OpenGL is supported.

- Key auto-repeat is implemented.


### Multimedia

- A new opt-in FFmpeg-based backend is introduced, as an alternative to the
  GStreamer backend. It is intended to improve compatibility with Media
  Foundation pipelines. It is still in experimental stage though, and more
  work will be needed, especially for D3D-aware playback. It can be enabled
  by setting the value `DisableGstByteStreamHandler=1` in the
  `HKCU\Software\Wine\MediaFoundation` registry key.

- Media Foundation multimedia pipelines are more accurately implemented, for
  the many applications that depend on the individual demuxing and decoding
  components to be exposed. Topology resolution with demuxer and decoder
  creation and auto-plugging is improved.

- DirectMusic supports loading MIDI files.


### Input / HID devices

- Raw HID devices with multiple top-level collections are correctly parsed,
  and exposed as individual devices to Windows application.

- Touchscreen input and events are supported with the X11 backend, and basic
  multi-touch support through the `WM_POINTER` messages is
  implemented. Mouse window messages such as `WM_LBUTTON*`, `WM_RBUTTON*`,
  and `WM_MOUSEMOVE` are also generated from the primary touch events.

- A number of USER32 internal structures are stored in shared memory, to
  improve performance and reduce Wine server load by avoiding server
  round-trips.

- An initial version of a Bluetooth driver is implemented, with some basic
  functionality.

- The Joystick Control Panel applet `joy.cpl` enables toggling some advanced
  settings.

- The Dvorak keyboard layout is properly supported.


### Internationalization

- Locale data is generated from the Unicode CLDR database version 46. The
  following additional locales are supported: `kaa-UZ`, `lld-IT`, `ltg-LV`,
  and `mhn-IT`.

- Unicode character tables are based on version 16.0.0 of the Unicode
  Standard.

- The timezone data is based on version 2024a of the IANA timezone database.


### Internet and networking

- The JavaScript engine supports a new object binding interface, used by
  MSHTML to expose its objects in a standard-compliant mode. This eliminates
  the distinction between JavaScript objects and host objects within the
  engine, allowing scripts greater flexibility when interacting with MSHTML
  objects.

- Built-in MSHTML functions are proper JavaScript function objects, and
  other properties use accessor functions where appropriate.

- MSHTML supports prototype and constructor objects for its built-in
  objects.

- Function objects in legacy MSHTML mode support the `call` and `apply`
  methods.

- The JavaScript garbage collector operates globally across all script
  contexts within a thread, improving its accuracy.

- JavaScript ArrayBuffer and DataView objects are supported.


### RPC / COM

- RPC/COM calls are fully supported on ARM platforms, including features
  such as stubless proxies and the typelib marshaler.

- All generated COM proxies use the fully-interpreted marshaling mode on all
  platforms.


### C runtime

- C++ exceptions and Run-Time Type Information (RTTI) are supported on ARM
  platforms.

- The ANSI functions in the C runtime support the UTF-8 codepage.


### Kernel

- Process elevation is implemented, meaning that processes run as a normal
  user by default but can be elevated to administrator access when required.

- Disk labels are retrieved from DBus when possible instead of accessing the
  raw device.

- Mailslots are implemented directly in the Wine server instead of using a
  socketpair, to allow supporting the full Windows semantics.

- Asynchronous waits for serial port events are reimplemented. The previous
  implementation was broken by the PE separation work in Wine 9.0.

- The full processor XState is supported in thread contexts, enabling
  support for newer vector extensions like AVX-512.


### macOS

- When building with Xcode >= 15.3 on macOS, the preloader is no longer
  needed.

- Syscall emulation for applications doing direct NT syscalls is supported
  on macOS Sonoma and later.


### Builtin applications

- The input parser of the Command Prompt tool `cmd` is rewritten, which
  fixes a number of long-standing issues, particularly with variable
  expansion, command chaining, and FOR loops.

- The Wine Debugger `winedbg` uses the Capstone library to enable
  disassembly on all supported CPU types.

- The File Comparison tool `fc` supports comparing files with default
  options.

- The `findstr` application supports regular expressions and case
  insensitive search.

- The `regsvr32` and `rundll32` applications can register ARM64EC modules.

- The `sort` application is implemented.

- The `where` application supports searching files with default options.

- The `wmic` application supports an interactive mode.


### Miscellaneous

- The ODBC library supports loading Windows ODBC drivers, in addition to
  Unix drivers that were already supported through libodbc.so.

- Optimal Asymmetric Encryption Padding (OAEP) is supported for RSA
  encryption.

- Network sessions are supported in DirectPlay.


### Development tools

- The IDL compiler generates correct format strings in interpreted stubs
  mode (`/Oicf` in midl.exe) on all platforms. Interpreted mode is now the
  default, the old mixed-mode stub generation can be selected with `widl
  -Os`.

- The IDL compiler can generate typelibs in the old SLTG format with the
  `--oldtlb` command-line option.

- The `winegcc` and `winebuild` tools can create hybrid ARM64X modules with
  the `-marm64x` option.

- The `winedump` tool supports dumping minidump tables, C++ exception data,
  CLR tables, and typelib resources.


### Build infrastructure

- The `makedep` tool generates a standard-format `compile_commands.json`
  file that can be used with various IDEs.

- Using `.def` files as import libraries with `winegcc` is no longer
  supported, all import libraries need to be in the standard `.a` format. If
  necessary, it is possible to convert a `.def` library to `.a` format using
  `winebuild --implib -E libfoo.def -o libfoo.a`.

- Static analysis is supported using the Clang Static Analyzer. It can be
  enabled by passing the `--enable-sast` option to configure. This is used
  to present Code Quality reports with the Gitlab CI.


### Bundled libraries

- The Capstone library version 5.0.3 is bundled and used for disassembly
  support in the Wine Debugger, to enable disassembly of ARM64 code. This
  replaces the bundled Zydis library, which has been removed.

- Vkd3d is updated to the upstream release [1.14][5].

- Faudio is updated to the upstream release 24.10.

- FluidSynth is updated to the upstream release 2.4.0.

- LDAP is updated to the upstream release 2.5.18.

- LCMS2 is updated to the upstream release 2.16.

- LibJpeg is updated to the upstream release 9f.

- LibMPG123 is updated to the upstream release 1.32.9.

- LibPng is updated to the upstream release 1.6.44.

- LibTiff is updated to the upstream release 4.7.0.

- LibXml2 is updated to the upstream release 2.12.8.

- LibXslt is updated to the upstream release 1.1.42.

- Zlib is updated to the upstream release 1.3.1.

[5]: https://gitlab.winehq.org/wine/vkd3d/-/releases/vkd3d-1.14


### External dependencies

- The FFmpeg libraries are used to implement the new Media Foundation
  backend.

- A PE cross-compiler is required for 32-bit ARM builds, pure ELF builds are
  no longer supported (this was already the case for 64-bit ARM).

- Libunwind is no longer used on ARM platforms since they are built as
  PE. It's only used on x86-64.
