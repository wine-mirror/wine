The Wine development release 9.11 is now available.

What's new in this release:
  - C++ exception handling on ARM platforms.
  - More DPI Awareness support improvements.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/9.x/wine-9.11.tar.xz>

Binary packages for various distributions will be available
from <https://www.winehq.org/download>

You will find documentation on <https://www.winehq.org/documentation>

Wine is available thanks to the work of many people.
See the file [AUTHORS][1] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/raw/wine-9.11/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 9.11 (total 27):

 - #42270  Settlers 4 Gold - Hardware Rendering mode not working
 - #49703  Ghost Recon fails to start
 - #50983  Multiple games stuck playing cutscenes (The Long Dark, The Room 4: Old Sins, Saint Kotar)
 - #51174  api-ms-win-core-version-l1-1-0: Missing GetFileVersionInfoW and GetFileVersionInfoSizeW
 - #52585  Multiple applications need NtQueryDirectoryObject to return multiple entries (Cygwin shells, WinObj 3.01)
 - #53960  ucrt has different struct layout than msvcrt
 - #54615  dwrite:layout - test_system_fallback() gets unexpected "Meiryo UI" font name in Japanese and Chinese on Windows
 - #55362  NeuralNote: Crashes and Rendering issues (alson in VST3 form)
 - #55472  DTS Encoder Suite gets stuck with encode pending from Wine 8.14
 - #56095  Clanbomber 1.05 starts after a long (30 seconds) delay
 - #56397  Numlock status not recognized when using winewayland.drv
 - #56451  catch block fetches bogus frame when using alignas with 32 or higher
 - #56460  Multiple games have stutter issues (Overwatch 2, Aimbeast)
 - #56591  Steam doesn't render individual game pages correctly
 - #56606  PhysX installer fails to start
 - #56640  Genshin Impact: The game-launcher cannot be started anymore
 - #56744  Serial number in smbios system table is not filled on Linux in practice
 - #56747  Steam won’t load in the new wow64 mode when using DXVK
 - #56755  White textures in EverQuest  (Unsupported Conversion in windowscodec/convert.c)
 - #56764  Empire Earth Gold doesn't start in virtual desktop mode
 - #56766  CDmage 1.01.5 does not redraw window contents fully
 - #56781  srcrrun: Dictionary setting item to object fails
 - #56788  ComicRackCE crashes when viewing "info" for a comic file
 - #56800  Nomad Factory plugins GUI is broken
 - #56813  Hard West 2 crashes before entering the main menu (OpenGL renderer)
 - #56824  Postal 2 (20th Anniversary update) crashes when loading the map
 - #56828  Moku.exe crashes on startup

### Changes since 9.10:
```
Adam Rehn (1):
      wineserver: Report non-zero exit code for abnormal process termination.

Alex Henrie (12):
      ntdll/tests: Delete the WineTest registry key when the tests finish.
      ntdll/tests: Rewrite the RtlQueryRegistryValues tests and add more of them.
      ntdll: Succeed in RtlQueryRegistryValues on direct query of nonexistent value.
      ntdll: Don't call QueryRoutine if RTL_QUERY_REGISTRY_DIRECT is set.
      ntdll: Don't call a null QueryRoutine in RtlQueryRegistryValues.
      ntdll/tests: Remove unused WINE_CRASH flag.
      ntdll: Copy the correct number of bytes with RTL_QUERY_REGISTRY_DIRECT.
      ntdll: Calculate the default size even without RTL_QUERY_REGISTRY_DIRECT.
      ntdll: Don't accept a query routine when using RTL_QUERY_REGISTRY_DIRECT.
      ntdll: Set the string size when using RTL_QUERY_REGISTRY_DIRECT.
      ntdll: Only allow string default values with RTL_QUERY_REGISTRY_DIRECT.
      ntdll: Replace the whole string when using RTL_QUERY_REGISTRY_DIRECT.

Alexandre Julliard (40):
      faudio: Import upstream release 24.06.
      msvcrt: Share a helper macro to print an exception type.
      msvcrt: Share the dump_function_descr() helper between platforms.
      msvcrt: Fix the ip_to_state() helper for out of bounds values.
      msvcrt: Don't use rva_to_ptr() for non-RVA values.
      msvcrt: Use the copy_exception() helper in __CxxExceptionFilter.
      msvcrt: Share the __CxxExceptionFilter implementation between platforms.
      msvcrt: Share the common part of _fpieee_flt between platforms.
      msvcrt: Consistently use the rtti_rva() helper.
      winedump: Fix dumping of catchblocks for 32-bit modules.
      msvcrt: The catchblock frame member isn't present on 32-bit.
      msvcrt: Use pointer-sized types instead of hardcoding 64-bit in __CxxFrameHandler.
      msvcrt: Share __CxxFrameHandler implementation with ARM platforms.
      msvcrt: Add platform-specific helpers to call C++ exception handlers.
      msvcrt: Add platform-specific helpers to retrieve the exception PC.
      msvcrt: Use platform-specific handlers also for __CxxFrameHandler4.
      ntdll: Fix stack alignment in __C_ExecuteExceptionFilter on ARM.
      ntdll: Fix a couple of compiler warnings on ARM64EC.
      ntdll: Fix inverted floating point masks on ARM64EC.
      ntdll: Support x87 control word in __os_arm64x_get_x64_information().
      kernel32/tests: Add test for FPU control words on ARM64EC.
      msvcrt: Reimplement __crtCapturePreviousContext() based on RtlWalkFrameChain().
      msvcrt: Use the __os_arm64x functions to get/set mxcsr on ARM64EC.
      msvcrt: Implement asm sqrt functions on ARM platforms.
      msvcrt: Disable SSE2 memmove implementation on ARM64EC.
      kernelbase: Implement the GetProcAddress wrapper on ARM64EC.
      kernel32: Implement the GetProcAddress wrapper on ARM64EC.
      winex11: Fix build error when XShm is missing.
      ntdll: Look for hybrid builtins in the PE directory for the host architecture.
      ntdll: Remove some unnecessary asm macros on ARM plaforms.
      winecrt0: Remove some unnecessary asm macros on ARM plaforms.
      makedep: Build and install ARM64EC-only modules.
      tests: Use ARM64 as architecture in manifests on ARM64EC.
      ntdll: Also load arm64 manifests for amd64 architecture on ARM64EC.
      ntdll: Reimplement __os_arm64x_check_call in assembly.
      ntdll: Move some security Rtl functions to sec.c.
      ntdll: Move some synchronization Rtl functions to sync.c.
      ntdll: Move the error mode Rtl functions to thread.c.
      ntdll: Move the PEB lock Rtl functions to env.c.
      ntdll: Move the memory copy Rtl functions to string.c.

Alexandros Frantzis (4):
      server: Pass desktop to get_first_global_hook.
      server: Check message target process for raw input device flags.
      server: Implement key auto-repeat request.
      win32u: Implement keyboard auto-repeat using new server request.

Alistair Leslie-Hughes (2):
      odbccp32: Check if a full path was supplied for Driver/Setup/Translator entries.
      odbccp32: Stop handle leak on error paths.

Benjamin Mayes (1):
      windowscodecs: Add conversions from PixelFormat32bppBGRA->PixelFormat16bppBGRA5551.

Brendan McGrath (4):
      kernel32/tests: Test error code when FindFirstFileA uses file as directory.
      ntdll/tests: Test error code when NtOpenFile uses file as directory.
      server: Don't always return STATUS_OBJECT_NAME_INVALID on ENOTDIR.
      ntdll: Treat XDG_SESSION_TYPE as special env variable.

Brendan Shanks (1):
      ntdll: On macOS, check for xattr existence before calling getxattr.

Connor McAdams (7):
      d3dx9: Refactor WIC GUID to D3DXIMAGE_FILEFORMAT conversion code.
      d3dx9: Refactor WIC image info retrieval code in D3DXGetImageInfoFromFileInMemory().
      d3dx9: Introduce d3dx_image structure for use in D3DXGetImageInfoFromFileInMemory().
      d3dx9: Use d3dx_image structure in D3DXLoadSurfaceFromFileInMemory().
      d3dx9: Introduce d3dx_load_pixels_from_pixels() helper function.
      d3dx9: Use d3dx_pixels structure in decompression helper function.
      d3dx9: Use d3dx_load_pixels_from_pixels() in D3DXLoadVolumeFromMemory().

Daniel Lehman (3):
      secur32: Allow overriding GnuTLS debug level.
      bcrypt: Allow overriding GnuTLS debug level.
      crypt32: Allow overriding GnuTLS debug level.

Danyil Blyschak (4):
      wineps.drv: Call ResetDCW() to update Devmode in the Unix interface.
      opcservices: Provide memory allocator functions to zlib.
      opcservices: Suppress unnecessary zlib deflate warnings.
      opcservices: Check for memory allocation failure before deflating.

Dmitry Timoshkov (2):
      server: Remove limitation for waiting on idle_event of the current process.
      win32u: Limit GDI object generation to 128.

Elizabeth Figura (25):
      widl: Assign to the right location variable.
      widl: Allow using UDTs with the keyword even when the identifier is also a typedef.
      widl: Invert "declonly".
      widl: Factor out a define_type() helper.
      widl: Update the type location in define_type().
      widl: Do not write type definitions for types defined in an imported header.
      include: Add more types to windows.networking.connectivity.idl.
      wined3d: Update multisample state when the sample count changes.
      wined3d: Invalidate the vertex shader when WINED3D_FFP_PSIZE is toggled.
      wined3d: Just check the vertex declaration for point size usage.
      wined3d: Just check the vertex declaration for colour usage.
      wined3d: Just check the vertex declaration for normal usage.
      wined3d: Just check the vertex declaration for texcoord usage.
      wined3d: Default diffuse to 1.0 in the vertex shader.
      server: Ignore attempts to set a mandatory label on a token.
      server: Inherit the source token's label in token_duplicate().
      advapi32/tests: Test token label inheritance.
      wined3d: Handle a null vertex declaration in glsl_vertex_pipe_vdecl().
      wined3d: Always output normalized fog coordinates from the vertex shader.
      wined3d: Do not create a framebuffer with attachments whose clear is delayed.
      wined3d: Use separate signature elements for oFog and oPts.
      server: Don't set error in find_object_index if the object is not found.
      ntdll: Implement reading multiple entries in NtQueryDirectoryObject.
      server: Generalize get_directory_entries to single_entry case.
      ntdll: Move IOCTL_SERIAL_WAIT_ON_MASK to the server.

Eric Pouech (13):
      cmd: Introduce a helper to set std handles.
      cmd: Introduce structure CMD_REDIRECTION.
      cmd: Create helper to execute a command.
      cmd: Let errorlevel be a signed integer.
      cmd: Separate IF command parsing from execution.
      kernelbase/tests: Fix typo in tests.
      msvcrt/tests: Don't print a NULL string.
      quartz/tests: Fix typo in tests.
      quartz/tests: Add new tests about fullscreen handling.
      quartz: Always expose that non fullscreen mode is supported and active.
      quartz: Fix result in put_FullScreenMode().
      conhost: Handle WM_CHAR for window console.
      conhost: Support IME input in window mode.

Esme Povirk (4):
      win32u: Send EVENT_OBJECT_FOCUS in more cases.
      win32u: Implement EVENT_OBJECT_LOCATIONCHANGE.
      gdiplus: Fix DIB stride calculation in GdipDrawImagePointsRect.
      win32u: Implement EVENT_SYSTEM_MINIMIZESTART/END.

Fabian Maurer (7):
      msvcrt: Fix _libm_sse2_sqrt_precise not using SSE2 sqrt.
      mmdevapi/tests: Add tests for IAudioSessionControl2 GetDisplayName / SetDisplayName.
      mmdevapi/tests: Add tests for IAudioSessionControl2 GetIconPath / SetIconPath.
      mmdevapi/tests: Add tests for IAudioSessionControl2 GetGroupingParam / SetGroupingParam.
      mmdevapi: Implement IAudioSessionControl2 GetDisplayName / SetDisplayName.
      mmdevapi: Implement IAudioSessionControl2 GetIconPath / SetIconPath.
      mmdevapi: Implement IAudioSessionControl2 GetGroupingParam SetGroupingParam.

Giovanni Mascellani (1):
      d3d11/tests: Add a test for NV12 textures.

Hans Leidekker (8):
      wmic: Sort the alias list.
      wmic: Add csproduct and systemenclosure aliases.
      ntdll: Provide fallback values for DMI fields only readable by root.
      odbc32: Use a fixed size buffer for parameter bindings.
      odbc32: Support SQLSetStmtAttr(SQL_ATTR_ROW_ARRAY_SIZE).
      odbc32: Turn SUCCESS() into a static inline function.
      odbc32/tests: Add tests for fetching multiple rows at once and parameter binding.
      winscard: Pass ATR buffer to unixlib in SCardStatusA().

Jacek Caban (4):
      mshtml: Use DispatchEx vtbl for document node GetDispID implementation.
      mshtml: Use DispatchEx for document node InvokeEx implementation.
      mshtml: Use DISPEX_IDISPATCH_IMPL macro for document object implementation.
      mshtml: Use DispatchEx for exposing document node IDispatchEx interface.

Jacob Pfeiffer (1):
      wininet: Unify timeout values closer to hInternet.

Jinoh Kang (2):
      user32/tests: Print regions in test_swp_paint_regions failure cases.
      wow64: Implement reading multiple entries in wow64_NtQueryDirectoryObject.

Lucas Chollet (1):
      dnsapi: Add a stub for DnsServiceBrowse.

Mohamad Al-Jaf (3):
      coremessaging: Add stub DLL.
      include: Add dispatcherqueue.idl file.
      coremessaging: Add CreateDispatcherQueueController() stub.

Nikolay Sivov (3):
      scrrun/dictionary: Implement putref_Item() method.
      dwrite/tests: Fix a test failure on some Win10 machines with CJK locales.
      gdi32/text: Handle null partial extents pointer in GetTextExtentExPointW().

Paul Gofman (12):
      ntdll: Fix test_NtQueryDirectoryFile() on Win11.
      ntdll: Do not ignore trailing dots in match_filename().
      ntdll/tests: Test NtQueryDirectoryFile() masks with more files.
      ntdll: Ignore leading dots in hash_short_file_name().
      ntdll: Mind all the wildcards in has_wildcard().
      ntdll: Match wildcard recursively in match_filename().
      ntdll: Add a special handling for .. in match_filename().
      ntdll: Implement matching DOS_STAR in NtQueryDirectoryFile().
      ntdll: Implement matching DOS_DOT in NtQueryDirectoryFile().
      ntdll: Properly match DOS_QM in match_filename().
      ntdll: Skip name search for wildcards in asterisk handling in match_filename().
      kernelbase: Preprocess wildcarded mask and pass it with NtQueryDirectoryFile().

Piotr Caban (8):
      wineps.drv: Don't use dynamic buffer when writing new page info.
      wineps.drv: Write page orientation hint for every page.
      wineps.drv: Take all pages into account when computing bounding box.
      wineps.drv: Write PageBoundingBox for every page.
      wineps.drv: Add partial support for changing page size.
      msvcp140: Use _get_stream_buffer_pointers() to access FILE internal buffers.
      ucrtbase: Fix _iobuf struct layout.
      msvcrt: Don't use custom standard streams definition.

Rémi Bernon (65):
      widl: Use mangled namespace names in typedef pointer types.
      win32u: Move the window surface color bits to the common struct.
      win32u: Pass BITMAPINFO and a HBITMAP to window_surface_init.
      winex11: Simplify the XSHM extension function calls.
      winex11: Create XImage before initializing the window surface.
      winex11: Create a HBITMAP for the allocated surface pixels.
      win32u: Create a HBITMAP backing the window surface pixels.
      win32u: Restore surface rect, which may offsetted from the window rect.
      win32u: Use a dedicated helper to move bits from a previous surface.
      win32u: Don't map points to the parent window in move_window_bits_parent.
      win32u: Get rid of move_window_bits_parent, using move_window_bits.
      winemac: Merge RESET_DEVICE_METRICS and DISPLAYCHANGE internal messages.
      win32u: Fix a restorer_str typo.
      win32u: Send display change messages when host display mode changes.
      win32u: Move desktop resize on WM_DISPLAYCHANGE out of the drivers.
      gdi32: Use an internal NtUser call for D3DKMTOpenAdapterFromGdiDisplayName.
      wineandroid: Always clear UpdateLayeredWindow target rectangle.
      wineandroid: Use the surface bitmap directly in UpdateLayeredWindow.
      winemac: Always clear UpdateLayeredWindow target rectangle.
      winemac: Use the surface bitmap directly in UpdateLayeredWindow.
      winemac: Blend alpha with NtGdiAlphaBlend instead of window opacity.
      winex11: Always clear UpdateLayeredWindow target rectangle.
      winex11: Use the surface bitmap directly in UpdateLayeredWindow.
      win32u: Introduce a new CreateLayeredWindow driver entry.
      win32u: Move UpdateLayeredWindow implementation out of the drivers.
      server: Avoid calling set_event from within msg_queue_add_queue.
      win32u: Introduce new helpers to convert server rectangle_t.
      win32u: Introduce NtUserAdjustWindowRect call for AdjustWindowRect*.
      win32u: Pass desired DPI to NtUserGet(Client|Window)Rect.
      win32u: Introduce a new get_monitor_rect helper.
      win32u: Pass the rect DPI to NtUserIsWindowRectFullScreen.
      winex11: Wrap more window surface formats with NtGdiDdDDICreateDCFromMemory.
      winex11: Fix some incorrect usage of NtGdiDdDDICreateDCFromMemory.
      gdi.exe16: Fix some incorrect usage of NtGdiDdDDICreateDCFromMemory.
      winegstreamer: Allow to clear video decoder input/output types.
      winegstreamer: Enforce default stride value in the video decoder.
      winegstreamer: Enforce default stride presence in the video processor.
      winegstreamer: Rename allow_size_change to allow_format_change.
      winegstreamer: Only report format changes when frontend supports it.
      winegstreamer: Use a caps to store the desired output format.
      winegstreamer: Request the new transform output format explicitly.
      winevulkan: Remove some unnecessary casts.
      winevulkan: Fix size mismatch when writing to return pointer.
      include: Add and fix some WGL prototypes.
      winewayland: Force the DPI context when restoring cursor clipping.
      winex11: Force the DPI context when restoring cursor clipping.
      win32u: Use get_monitor_rect in more places.
      win32u: Parameterize get_clip_cursor dpi.
      win32u: Parameterize get_monitor_info dpi.
      win32u: Use window monitor DPI in get_windows_offset when dpi is 0.
      winex11: Use NtUserMapWindowPoints instead of NtUserScreenToClient.
      win32u: Call NtUserMapWindowPoints with per-monitor DPI from the drivers.
      winemac: Force thread DPI awareness when calling NtUserSetWindowPos.
      winewayland: Force thread DPI awareness when calling NtUserSetWindowPos.
      winex11: Force thread DPI awareness when calling NtUserSetWindowPos.
      winex11: Force thread DPI awareness when calling NtUserRedrawWindow.
      winex11: Force thread DPI awareness when calling NtUserChildWindowFromPointEx.
      win32u: Remove unused insert_after WindowPosChanging parameter.
      wineandroid: Remove unnecessary visible_rect initialization.
      winemac: Remove unnecessary visible_rect initialization.
      winex11: Remove unnecessary visible_rect initialization.
      win32u: Split WindowPosChanging driver call to a separate CreateWindowSurface.
      winex11: Move layered window mapping to X11DRV_UpdateLayeredWindow.
      winemac: Move layered window mapping to macdrv_UpdateLayeredWindow.
      win32u: Move WM_WINE_DESKTOP_RESIZED into driver internal messages range.

Shengdun Wang (3):
      ucrtbase/tests: Add FILE structure tests.
      ucrtbase: Always use CRITICAL_SECTION for FILE locking.
      ucrtbase: Fix FILE _flag values.

Tim Clem (1):
      gitlab: Update configuration for the new Mac runner.

Zhiyi Zhang (2):
      ws2_32/tests: Test fromlen for recvfrom().
      ntdll: Don't zero out socket address in sockaddr_from_unix().

Ziqing Hui (7):
      qasf/tests: Add more tests for dmo_wrapper_sink_Receive.
      qasf/dmowrapper: Introduce get_output_samples.
      qasf/dmowrapper: Introduce release_output_samples.
      qasf/dmowrapper: Return failure in dmo_wrapper_sink_Receive if samples allocating fails.
      qasf/dmowrapper: Allocate output samples before calling ProcessInput().
      qasf/dmowrapper: Return VFW_E_WRONG_STATE in dmo_wrapper_sink_Receive.
      qasf/dmowrapper: Sync Stop() and Receive() for dmo wrapper filter.
```
