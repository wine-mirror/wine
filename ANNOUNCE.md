The Wine development release 11.1 is now available.

What's new in this release:
  - Various changes that were deferred during code freeze.
  - More pixel format conversions in WindowsCodecs.
  - More work on ActiveX Data Objects (MSADO).
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/11.x/wine-11.1.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-11.1/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 11.1 (total 22):

 - #16009  SureThing CD Labeler crashes when starting a new project
 - #26645  PAF5 throws exception after canceling print from help viewer
 - #38740  Mathearbeit G 5.6 installer hangs during installation (ShellFolder attributes for virtual folder 'CLSID_Printers', clsid '{2227a280-3aea-1069-a2de-08002b30309d}' missing in registry)
 - #45930  Microsoft .NET Framework 4.x GAC update fails for some assemblies, 'Error compiling mscorlib, Path not found. (Exception from HRESULT: 0x80070003)' (legacy GAC missing: '%systemroot%\assembly')
 - #51232  calling WsaGetLastError() after calling getsockopt() with a file descriptor that is not a socket differs from Windows
 - #52030  Multiple game have corrupted text rendering (Project Cars 2, SnowRunner)
 - #54160  Disk Usage page of PDFsam installer says that empty CD drive has the same amount of free space as the previous drive in the list
 - #54300  LdrLoadDll illegal memory access on DllPath
 - #56378  Microsoft Edge and Edge-based WebView2 do not function without --no-sandbox option
 - #58143  Variable refresh rate does not work properly with winewayland.
 - #58158  Sacred Gold 2.28 ASE crashes when initialising multiplayer
 - #58757  Camerabag Pro 2025.2 fails to load image picture data unless winetricks windowscodecs used
 - #58793  iTunes 12.9.3 freezes after opening
 - #58905  Many Microsoft Store installers fail to start with System.TypeLoadException
 - #59046  Multiple applications crash on unimplemented ICU functions
 - #59149  Icons are broken in Wayland
 - #59153  Wine 11.0-rc2 Ubisoft Connect freezes
 - #59155  FastStone Image Viewer fails to open some PNG images due to unimplemented wincodecs conversion (copypixels_to_24bppBGR)
 - #59162  GOG GALAXY launcher starts on an all white screen
 - #59172  CEnumerateSerial needs unimplemented function msports.dll.ComDBOpen
 - #59178  Genshin Impact requires ntdll.CloseThreadpoolCleanupGroupMembers to not hang when called from a DLL_PROCESS_DETACH notification
 - #59227  Program under LMDE6 and wine  run but not anymore under LMDE7 and wine

### Changes since 11.0:
```
Alex Henrie (2):
      winecfg: Remove unnecessary calls to strdupU2W.
      include: Annotate VirtualAlloc functions with __WINE_ALLOC_SIZE.

Alexandre Esteves (1):
      ntdll: Accept device paths in LdrAddDllDirectory.

Alexandre Julliard (19):
      winebth.sys: Don't include config.h from another header.
      oledb32: Move non version-related resources out of version.rc.
      tasklist: Make version resource language-neutral.
      wrc: Stop translating the product name in version resources.
      widl: Fix a typo in a comment.
      resources: Rename version.rc files if they also contain non-version resources.
      include: Add ntverp.h and common.ver.
      include: Use common.ver to implement wine_common_ver.rc.
      include: Add Wine version to the default product name in version resources.
      start: On failure, report the error from the initial ShellExecuteW.
      resources: Don't rely on headers being included by wine_common_ver.rc.
      resources: Use common.ver directly in tests.
      kernel32: Use common.ver directly.
      ntdll: Remove the machine frame in KiUserExceptionDispatcher on ARM64.
      ntdll/tests: Add a test for passing BOOLEAN values in syscalls.
      ntdll: Move NT syscall Unix-side support to a new syscall.c file.
      ntdll: Add syscall wrappers to work around ABI breakages.
      win32u: Add syscall wrappers to work around the macOS ABI breakage on ARM64.
      ntdll/tests: Mark a test that sometimes fails as flaky.

Alistair Leslie-Hughes (3):
      include: Add more DB_E_ defines.
      include: Add peninputpanel.idl.
      include: Add some coclasses.

Ashley Hauck (1):
      winewayland: Support wl_seat version 8.

Bernhard Übelacker (1):
      iccvid/tests: Reserve enough memory for input in test_formats (ASan).

Biswapriyo Nath (6):
      include: Add ID3D12VideoProcessor in d3d12video.idl.
      include: Add ID3D12VideoProcessCommandList in d3d12video.idl.
      include: Add D3D12_FEATURE_DATA_VIDEO_PROCESS_SUPPORT in d3d12video.idl.
      include: Add D3D12_VIDEO_ENCODER_MOTION_ESTIMATION_PRECISION_MODE_EIGHTH_PIXEL.
      include: Add missing flags in D3D12_RENDER_PASS_FLAGS.
      propsys: Fix parameter type of PropVariantToGUID.

Brendan McGrath (11):
      mfsrcsnk: Use stream_map for index value on stream start.
      mfsrcsnk/tests: Add test for end of presentation.
      mfsrcsnk: Remain in running state on EOP.
      mfmediaengine/tests: Test we receive a frame before calling Play.
      mf/tests: Add test for sample grabber seek.
      mf/tests: Test sample grabber {Set,Cancel}Timer.
      mf/tests: Check StreamSinkMarker event.
      mf/tests: Test samplegrabber flush then seek.
      mf/tests: Test seek with sample grabber whilst ignoring clock.
      mf/tests: Check contents of sample collection.
      mf/tests: Test sample grabber pause and resume.

Connor McAdams (5):
      d3dx10: Move file/resource loading helper functions into shared code.
      d3dx11: Implement D3DX11GetImageInfoFromFile{A,W}().
      d3dx11: Implement D3DX11GetImageInfoFromResource{A,W}().
      d3dx11: Implement D3DX11CreateTextureFromFile{A,W}().
      d3dx11: Implement D3DX11CreateTextureFromResource{A,W}().

Derek Lesho (2):
      win32u: Implement glImportMemoryWin32(Handle/Name)EXT for opaque handle types.
      win32u: Implement glImportSemaphoreWin32(Handle/Name)EXT for opaque handle types.

Dmitry Timoshkov (8):
      win32u: Also print hwnd in set_window_text() trace.
      windowscodecs: Add support for converting 8bpp indexed to 24bpp format.
      windowscodecs: Support trivial cases for copying 1bpp indexed data.
      windowscodecs: Remove optimization for 1bpp pixel formats from WICConvertBitmapSource().
      windowscodecs: Add support for converting from 32bpp RGBA to 64bpp RGBA.
      windowscodecs: Fix conversion from 64bpp RGBA to 32bpp BGRA.
      kerberos: Add support for EXTENDED_ERROR flags.
      windowscodecs: Add fallback path to copypixels_to_64bppRGBA().

Elizabeth Figura (11):
      wined3d: Separate a vk_blitter_conversion_supported() helper.
      wined3d: Hold a reference to the wined3d in adapter_vk_destroy_device().
      wined3d/spirv: Set FFP caps.
      wined3d/spirv: Set shader_caps.varying_count.
      wined3d: Downgrade the multiple devices FIXME to a WARN.
      kernel32/tests: Test GetDiskSpaceInformationA() on a volume path.
      mountmgr: Implement FileFsFullSizeInformationEx.
      wined3d/glsl: Fix WINED3DTSS_TCI_SPHEREMAP calculation.
      d3d8/tests: Test generated texcoords.
      d3d9/tests: Test generated texcoords.
      wined3d/hlsl: Implement generated texcoords.

Eric Pouech (4):
      cmd: Detect and report more syntax errors in MKLINK command.
      cmd: Fix two potential buffer overflows in MKLINK command.
      cmd/tests: Add test for quoted wildcarded sets in FOR instruction.
      cmd: Strip quotes from element in FOR set when it contains wildcards.

Francisco Casas (1):
      wined3d: Don't print opcodes for cs padding packets.

Gabriel Ivăncescu (2):
      jscript: Treat DISPATCH_PROPERTYGET | DISPATCH_METHOD as property getter in ES5+ modes.
      mshtml: Handle DISPATCH_PROPERTYGET | DISPATCH_METHOD in dispex_value as property getter.

Georg Lehmann (2):
      winevulkan: Update to VK spec version 1.4.339.
      winevulkan: Update to VK spec version 1.4.340.

Giovanni Mascellani (2):
      gitlab: Install PCSC lite and Samba for i386 as well.
      gitlab: Install the OSSv4 header.

Hans Leidekker (9):
      netprofm: Add IPv6 support.
      netprofm: Skip the loopback interface.
      netprofm: Trace returned connectivity values.
      winhttp: Convert build_wpad_url() to Unicode.
      winhttp: Enforce a 5 second resolve timeout in detect_autoproxyconfig_url_dns().
      msi: Don't list drives for which GetVolumeInformationW() or GetDiskFreeSpaceExW() fails.
      fusion: Create the legacy GAC directory in CreateAssemblyCache().
      ncrypt: Support ECDSA_P256.
      crypt32: Sync backing store when contexts are added or removed.

Jacek Caban (3):
      include: Add more stdio.h function declarations.
      include: Add more corecrt_wstdio.h function declarations.
      include: Move intrinsic pragmas below function declarations.

Jactry Zeng (1):
      winemenubuilder: Skip .desktop file creation when not on an XDG desktop.

Jiajin Cui (2):
      windowscodecs: Correct GIF LZW compression initialization for images with truncated color palette.
      ntdll: Prevent null pointer dereference in directory scanning.

Louis Lenders (1):
      shell32: Add attributes to registry for CLSID_Printers.

Michael Stopa (1):
      cryptnet: Close file handle.

Mohamad Al-Jaf (3):
      chakra: Add stub dll.
      windows.ui/tests: Add IUISettings2::get_TextScaleFactor() tests.
      windows.ui: Stub IUISettings2::get_TextScaleFactor().

Nikolay Sivov (10):
      d2d1/tests: Add a test for default stroke transform type.
      d2d1/stroke: Fix querying for ID2D1StrokeStyle1.
      d2d1/tests: Add some tests for transformed geometries used with uninitialized paths.
      dwrite/tests: Handle COLRv1 in GetGlyphImageFormats() tests.
      dwrite: Improve underline thickness estimation when font hasn't provided one.
      dwrite/tests: Add some tests for HitTestTextPosition().
      dwrite/layout: Run script and levels analyzer on whole text instead of segments.
      dwrite/layout: Use single structure for formatted runs.
      dwrite/layout: Add a list to keep textual and inline runs together.
      d2d1: Implement GetGlyphRunWorldBounds().

Paul Gofman (19):
      ntdll: Fix LdrLoadDll() prototype.
      ntdll: Handle search flags in LdrLoadDll().
      kernel32/tests: Add tests for LdrLoadDll().
      ntdll: Use LDR_DONT_RESOLVE_REFS instead of DONT_RESOLVE_DLL_REFERENCES.
      ntdll: Don't fill output handle on error in LdrLoadDll().
      kernelbase: Pass search flags instead of path to LdrLoadDll().
      wbemprox: Add Win32_SystemEnclosure.SerialNumber.
      wbemprox: Mark Win32_SystemEnclosure.Tag as key column.
      dwrite: Zero 'current' in create_font_collection().
      ntdll: Make VEH registration structure compatible.
      win32u: Wrap vkGetPhysicalDeviceProperties.
      win32u: Fixup Vulkan deviceName to match win32u GPU name.
      ntdll: Factor out get_random() function.
      ntdll: Generate process cookie and return it from NtQueryInformationProcess( ProcessCookie ).
      ntdll: Reimplement RtlEncodePointer / RtlDecodePointer using process cookie.
      kernel32: Don't use export forwarding for ntdll function table functions.
      ntdll/tests: Add some tests for RtlWalkFrameChain() / RtlCaptureStackBackTrace() on x64.
      ntdll: Stop walk in RtlWalkFrameChain() if there is no function entry on x64.
      ntdll: Add stub function for NtWorkerFactoryWorkerReady().

Piotr Caban (21):
      include: Add msdshape.h header.
      msado15: Improve column lookup performance.
      msado15: Add initial ADOConnectionConstruction tests.
      msado15: Add _Connection:{get,put}_ConnectionTimeout implementation.
      msado15: Add partial adoconstruct_WrapDSOandSession implementation.
      msado15: Fix lock and cursor type initialization in recordset_Open.
      msado15: Request more features when opening table directly.
      msado15: Fix active connection leak on Recordset release.
      msado15: Improve connection validation in recordset_put_ActiveConnection.
      msado15: Make dso and session arguments optional in adoconstruct_WrapDSOandSession.
      msado15: Handle SetProperties error in adoconstruct_WrapDSOandSession.
      msado15: Add initital IRowset wrapper tests.
      msado15: Add IRowset wrapper stub.
      msado15: Silence FIXME message when querying for known interfaces in rowset wrapper.
      msado15: Implement IRowsetExactScroll:GetExactPosition in rowset wrapper.
      msado15: Handle VT_I8 bookmarks in cache_get() helper.
      msado15: Add _Recordset::Seek implementation.
      msado15: Fix IDBProperties use after free in adoconstruct_WrapDSOandSession.
      msado15: Respect DBPROP_CANHOLDROWS property in cache_get().
      msado15: Dump rowset properties if logging is enabled.
      msado15: Fix WrapDSOandSession usage in connection_Open.

Ralf Habacker (1):
      ws2_32: Use debug strings in trace outputs for *socket*() and bind().

Rémi Bernon (28):
      win32u: Use surfaces for client area if the backend doesn't support PutImage.
      server: Track foreground process id, instead of looking for active window.
      server: Remove now unnecessary process-wide set_foreground flag.
      win32u: Make sysparam GPU devices less Vulkan specific.
      win32u: Enumerate system GPU devices from EGL devices.
      win32u: Expose device LUID / nodes mask to opengl32.
      windows.gaming.input: Enfore axis usage mapping for HID devices.
      win32u: Load glBlitFramebuffer function pointer.
      opengl32: Fix front buffer emulation glBlitFramebuffer parameters.
      opengl32: Use glBlitFramebuffer instead of glBlitNamedFramebuffer.
      opengl32: Resolve default framebuffer before flushing.
      win32u: Remove unnecessary window_surface exports.
      winevulkan: Move copied structs without chain to a separate set.
      winevulkan: Only require conversion if there are extensions.
      winevulkan: Enable VK_KHR_maintenance7 extension.
      winevulkan: Remove mostly unnecessary WINE_VK_VERSION.
      winevulkan: Mind unsupported extensions dependencies.
      winevulkan: Introduce an Extension class.
      winevulkan: Get rid of the VkRegistry class.
      winevulkan: Use the conversion names instead of rebuilding them.
      winevulkan: Include physical device in the instance functions.
      winevulkan: Simplify function prototype generation.
      winevulkan: Get rid of remaining extra client parameter.
      winevulkan: Simplify function lists generation.
      winevulkan: Add parameter names to function pointers.
      winevulkan: Simplify function parameters generation.
      winevulkan: Simplify function protoypes generation.
      mf/session: Discard end of presentation on IMFMediaSession_Stop.

SeongChan Lee (2):
      winebus: Use smallest report size for joystick axis.
      winebus: Use 16-bit relative axis range for bus_sdl.

Stefan Brüns (2):
      msports: Add stub implementations for ComDBOpen/ComDBClose.
      advapi32: Use same default values for both GetEffectiveRightsFromAclA/W.

Stefan Dösinger (3):
      d3d8/tests: ATI2 DS surface creation attempt crashes nvidia.
      d3d8/tests: Clarify a skip message.
      d3d9/tests: Clarify a skip message.

Sven Baars (1):
      msi/tests: Fix some typos (Coverity).

Tim Clem (4):
      msvcp140: Bump the version number.
      msvcp140_2: Bump the version number.
      vcruntime140_1: Bump the version number.
      wineboot: Check if ai_canonname is NULL.

Tobias Gruetzmacher (1):
      winhttp: Implement WINHTTP_AUTOPROXY_ALLOW_STATIC.

Vibhav Pant (5):
      winebth.sys: Fix potential memory leaks when async DBus calls fail.
      winebth.sys: Simplify DBus signal matching in bluez_filter.
      winebth.sys: Use a helper for logging DBus errors.
      winebth.sys: Add heap allocation annotations to unix_name_{get_or_create, dup}.
      rometadata: Add helpers for bounds checks while accessing offsets to assembly data.

Yeshun Ye (2):
      kernel32/tests: Add tests for CreateProcessA.
      kernelbase: Trim whitespaces from the quoted command line.

Yuxuan Shui (1):
      qasf: Protect against unconnected pins in media_seeking_ChangeCurrent.

Zhiyi Zhang (5):
      winex11.drv: Delete unused parameters for X11DRV_init_desktop().
      comctl32_v6: Add horizontal spacing around button text.
      comctl32: Use BOOLEAN instead of BOOL for RegisterClassNameW().
      comctl32/tests: Test that there is no flatsb_class32 window class.
      comctl32: Remove flatsb_class32 window class.

Ziia Shi (1):
      ntdll: Avoid infinite wait during process termination.
```
