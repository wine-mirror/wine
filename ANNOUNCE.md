The Wine team is proud to announce that the stable release Wine 9.0
is now available.

This release represents a year of development effort and over 7,000
individual changes. It contains a large number of improvements that
are listed below. The main highlights are the new WoW64 architecture
and the experimental Wayland driver.

The source is available at <https://dl.winehq.org/wine/source/9.0/wine-9.0.tar.xz>

Binary packages for various distributions will be available
from <https://www.winehq.org/download>

You will find documentation on <https://www.winehq.org/documentation>

Wine is available thanks to the work of many people.
See the file [AUTHORS][1] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/raw/wine-9.0/AUTHORS


## What's new in Wine 9.0

### WoW64

- All transitions from Windows to Unix code go through the NT syscall
  interface. This is a major milestone that marks the completion of the
  multi-year re-architecturing work to convert modules to PE format and
  introduce a proper boundary between the Windows and Unix worlds.

- All modules that call a Unix library contain WoW64 thunks to enable calling
  the 64-bit Unix library from 32-bit PE code. This means that it is possible to
  run 32-bit Windows applications on a purely 64-bit Unix installation. This is
  called the _new WoW64 mode_, as opposed to the _old WoW64 mode_ where 32-bit
  applications run inside a 32-bit Unix process.

- The new WoW64 mode is not yet enabled by default. It can be enabled by passing
  the `--enable-archs=i386,x86_64` option to configure. This is expected to work
  for most applications, but there are still some limitations, in particular:
  - Lack of support for 16-bit code.
  - Reduced OpenGL performance and lack of `ARB_buffer_storage` extension
    support.

- The new WoW64 mode finally allows 32-bit applications to run on recent macOS
  versions that removed support for 32-bit Unix processes.


### Wayland driver

- There is an experimental Wayland graphics driver. It's still a work in
  progress, but already implements many features, such as basic window
  management, multiple monitors, high-DPI scaling, relative motion events, and
  Vulkan support.

- The Wayland driver is not yet enabled by default. It can be enabled through
  the `HKCU\Software\Wine\Drivers` registry key by running:

      wine reg.exe add HKCU\\Software\\Wine\\Drivers /v Graphics /d x11,wayland

  and then making sure that the `DISPLAY` environment variable is unset.


### ARM64

- The completion of the PE/Unix separation means that it's possible to run
  existing Windows binaries on ARM64.

- The loader supports loading ARM64X and ARM64EC modules.

- The 32-bit x86 emulation interface is implemented. No emulation library is
  provided with Wine at this point, but an external library that exports the
  interface can be used, by specifying its name in the
  `HKLM\Software\Microsoft\Wow64\x86` registry key. The [FEX emulator][2]
  implements this interface when built as PE.

- There is initial support for building Wine for the ARM64EC architecture, using
  an experimental LLVM toolchain. Once the toolchain is ready, this will be used
  to do a proper ARM64X build and enable 64-bit x86 emulation.

[2]: https://fex-emu.com


### Graphics

- The PostScript driver is reimplemented to work from Windows-format spool files
  and avoid any direct calls from the Unix side.

- WinRT theming supports a dark theme option, with a corresponding toggle in
  WineCfg.

- The Vulkan driver supports up to version 1.3.272 of the Vulkan spec.

- A number of GdiPlus functions are optimized for better graphics performance.


### Direct3D

- The multi-threaded command stream sleeps instead of spinning when not
  processing rendering commands. This lowers power consumption in programs which
  do not occupy the command stream's entire available bandwidth. Power
  consumption should be comparable to when the multi-threaded command stream is
  disabled.

- Direct3D 10 effects support many more instructions.

- Various optimizations have been made to core WineD3D and the Vulkan backend.

- The Vulkan renderer properly validates that required features are supported by
  the underlying device, and reports the corresponding Direct3D feature level to
  the application.

- `D3DXFillTextureTX` and `D3DXFillCubeTextureTX` are implemented.

- The legacy OpenGL ARB shader backend supports shadow sampling via
  `ARB_fragment_program_shadow`.

- The HLSL compiler supports matrix majority compilation flags.

- `D3DXLoadMeshHierarchyFromX` and related functions support user data loading
  via `ID3DXLoadUserData`.


### Audio / Video

- The foundation of several of the DirectMusic modules is implemented. Many
  tests are added to validate the behavior of the dmime sequencer and the
  dmsynth MIDI synthesizer.

- DLS1 and DLS2 sound font loading is implemented, as well as SF2 format for
  compatibility with Linux standard MIDI sound fonts.

- MIDI playback is implemented in dmsynth, with the integration of the software
  synthesizer from the FluidSynth library, and using DirectSound for audio
  output.

- Doppler shift is supported in DirectSound.

- The Indeo IV50 Video for Windows decoder is implemented.


### DirectShow

- The Windows Media Video (WMV) decoder DirectX Media Object (DMO) is
  implemented.

- The DirectShow Audio Capture filter is implemented.

- The DirectShow MPEG‑1 Stream Splitter filter supports video and system streams
  as well as audio streams.

- The DirectShow MPEG‑1 Video Decoder filter is implemented.


### Input devices

- DirectInput action maps are implemented, improving compatibility with many old
  games that use this to map controller inputs to in-game actions.


### Desktop integration

- URL/URI protocol associations are exported as URL handlers to the Linux
  desktop.

- Monitor information like name and model id are retrieved from the physical
  monitor's Extended Display Identification Data (EDID).

- In full-screen desktop mode, the desktop window can be closed through the
  "Exit desktop" entry in the Start menu.


### Internationalization

- IME implementation is improved, with better support for native Windows IME
  implementations. Many tests are added to validate the expected behavior of
  these custom IMEs.

- Linux IME integration is improved, using over-the-spot or on-the-spot input
  styles whenever possible, and more accurate IME message sequences.

- Locale data is generated from the Unicode CLDR database version 44. The
  following additional locales are supported: `bew-ID`, `blo-BJ`, `csw-CA`,
  `ie-EE`, `mic-CA`, `prg-PL`, `skr-PK`, `tyv-RU`, `vmw-MZ`, `xnr-IN`, and
  `za-CN`.

- The user interface is translated to Georgian, bringing the total of full
  translations to 16 languages, with partial translations to another 31
  languages.

- Unicode character tables are based on version 15.1.0 of the Unicode Standard.

- The timezone data is generated from the IANA timezone database version 2023c.

- Locales using a script name, like `zh-Hans`, are also supported on macOS.


### Kernel

- The default Windows version for new prefixes is set to Windows 10.

- Address space layout randomization (ASLR) is supported for modern PE binaries,
  to avoid issues with address space conflicts. Note that the selected load
  addresses are not yet properly randomized.

- The Low Fragmentation Heap (LFH) is implemented for better memory allocation
  performance.

- The virtual memory allocator supports memory placeholders, to allow
  applications to reserve virtual space.

- The 64-bit loader and preloader are built as position-independent executables
  (PIE), to free up some of the 32-bit address space.

- Stack unwinding works correctly across NT syscalls and user callbacks.


### Internet and networking

- All builtin MSHTML objects are proper Gecko cycle collector participants.

- Synchronous XMLHttpRequest mode is supported in MSHTML.

- WeakMap object is implemented in JScript.

- The Gecko engine is updated to version 2.47.4.

- Network interface change notifications are implemented.


### Cryptography and security

- Smart cards are supported in the Winscard dll, using the Unix PCSClite
  library.

- Diffie-Hellman keys are supported in BCrypt.

- The Negotiate security package is implemented.


### Mono / .NET

- The Mono engine is updated to version [8.1.0][3].

[3]: https://github.com/madewokherd/wine-mono/releases/tag/wine-mono-8.1.0


### Builtin applications

- The Wine Debugger (winedbg) uses the Zydis library for more accurate x86
  disassembly.

- WineCfg supports selecting old (pre-XP) Windows versions also in 64-bit
  prefixes, to enable using ancient applications with the new WoW64 mode.

- All graphical builtin applications report errors with a message box instead of
  printing messages on the console.

- The `systeminfo` application prints various data from the Windows Management
  Instrumentation database.

- The `klist` application lists Kerberos tickets.

- The `taskkill` application supports terminating child processes.

- The `start` application supports a `/machine` option to select the
  architecture to use when running hybrid x86/ARM executables.

- Most of the functionality of the `tasklist` application is implemented.

- The `findstr` application provides basic functionality.


### Development tools

- The WineDump tool supports printing the contents of Windows registry files
  (REGF format), as well as printing data for both architectures in hybrid
  x86/ARM64 PE files.
  
- The `composable`, `default_overload`, `deprecated`, and `protected` attributes
  are supported in the IDL compiler.

- The `libwine.so` library is removed. It was no longer used, and deprecated
  since Wine 6.0. Winelib ELF applications that were built with Wine 5.0 or
  older will need a rebuild to run on Wine 9.0.


### Bundled libraries

- The FluidSynth library version 2.3.3 is bundled and used for DirectMusic.

- The math library of Musl version 1.2.3 is bundled and used for the math
  functions of the C runtime.

- The Zydis library version is 4.0.0 is bundled and used for x86 disassembly
  support.

- Vkd3d is updated to the upstream release 1.10.

- Faudio is updated to the upstream release 23.12.

- LDAP is updated to the upstream release 2.5.16.

- LCMS2 is updated to the upstream release 2.15.

- LibMPG123 is updated to the upstream release 1.32.2.

- LibPng is updated to the upstream release 1.6.40.

- LibTiff is updated to the upstream release 4.6.0.

- LibXml2 is updated to the upstream release 2.11.5.

- LibXslt is updated to the upstream release 1.1.38.

- Zlib is updated to the upstream release 1.3.


### External dependencies

- The Wayland client library, as well as the xkbcommon and xkbregistry
  libraries, are used when building the Wayland driver.

- The PCSClite library is used for smart card support. On macOS, the PCSC
  framework can be used as an alternative to PCSClite.

- For PE builds, a cross-compiler that supports `.seh` directives for exception
  handling is required on all platforms except i386.
