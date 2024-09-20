The Wine development release 9.18 is now available.

What's new in this release:
  - New Media Foundation backend using FFMpeg.
  - Initial support for network sessions in DirectPlay.
  - New Desktop Control Panel applet.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/9.x/wine-9.18.tar.xz>

Binary packages for various distributions will be available
from <https://www.winehq.org/download>

You will find documentation on <https://www.winehq.org/documentation>

Wine is available thanks to the work of many people.
See the file [AUTHORS][1] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/raw/wine-9.18/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 9.18 (total 18):

 - #10648  gRPC library fails to send RPC packets correctly (nonblocking send() should not perform partial writes)
 - #53727  TreeView doesn't check the return value of TREEVIEW_SendExpanding
 - #55347  widl generated winrt headers fails to compile with C++ code
 - #56596  Keyboard keypress generates NumLock keypress for all keys
 - #56873  WordSmith 9.0 doesn't show text in installer
 - #57136  Steinberg Download Assistant crashes (part 2)
 - #57141  Repaper Studio crashes on unimplemented function USER32.dll.CreateSyntheticPointerDevice
 - #57155  Gigapixel ai crashes on startup in win7 mode
 - #57158  HID devices not detected after removal until Wine processes restart
 - #57160  16-bit color no longer works when using Xephyr
 - #57163  msiexec sometimes fails with unquoted filenames
 - #57173  Wine-dev 9.17 does not allow drag-and-drop of files into LTspice
 - #57181  PathGradientBrushTest:Clone fails with InvalidParameter
 - #57189  Caesar 3, Neighbours from Hell 1-2: screen cropped
 - #57190  Configure ends with: Do '' to compile Wine.
 - #57195  wineconsole is broken Wine 9.17 after moving its window (Far File Manager is broken as well, as a result)
 - #57199  Window surface leaks with DPI unaware apps
 - #57200  Warlords III: Darklords Rising shows distorted image

### Changes since 9.17:
```
Aida Jonikienė (1):
      winewayland: Make the pointer protocols optional.

Alexandre Julliard (47):
      combase: Avoid modifying input object attributes in create_key (Clang).
      ole32: Avoid modifying input object attributes in create_key (Clang).
      odbc32: Avoid buffer overflow on empty connection string (Clang).
      ntdll: Remove some dead initializations (Clang).
      configure: Re-generate with autoconf 2.72.
      configure: Disable misguided autoconf error on wow64 builds without large time_t.
      configure: Use the compiler instead of the preprocessor to check CPU defines.
      configure: Remove some no longer used defines.
      server: Add a helper to append data to a buffer.
      server: Remove some dead initializations (Clang).
      server: Avoid a redundant list check (Clang).
      server: Avoid memcpy with null pointer (Clang).
      server: Make masks unsigned (Clang).
      mf/tests: Remove todo_wine from a test that succeeds now.
      ntoskrnl/tests: Mark some failing tests as todo_wine.
      comctl32/tests: Skip hotkey test if window is not foreground.
      comctl32/tests: Wait a bit more for the tooltip to appear.
      d3dx9: Use sizeof on the correct type in malloc (Clang).
      dsound: Use sizeof on the correct type in malloc (Clang).
      gdiplus: Use sizeof on the correct type in malloc (Clang).
      hidclass.sys: Use sizeof on the correct type in malloc (Clang).
      msi: Use sizeof on the correct type in malloc (Clang).
      msvcrt: Use sizeof on the correct type in malloc (Clang).
      wintrust: Use sizeof on the correct type in malloc (Clang).
      win32u: Use the correct type in malloc (Clang).
      mscoree: Use the correct size in malloc (Clang).
      winex11: Use the correct type in malloc (Clang).
      winecfg: Use the correct type in malloc (Clang).
      comctl32/tests: Skip tests if tooltip isn't displayed.
      comctl32/tests: Make some messages optional in propsheet display sequence.
      urlmon/tests: Skip test if ftp connection fails.
      ntdll: Avoid closing an invalid handle (Clang).
      regedit: Fix potential buffer overflow (Clang).
      oleaut32: Fix potential double free (Clang).
      msvcrt: Mark _CxxThrowException noreturn (Clang).
      configure: Update make command in final message.
      configure: Add /usr/share/pkgconfig to the pkg-config path.
      comctl32/tests: Add more optional propsheet messages.
      nsi/tests: Properly cancel all change notifications.
      comctl32/tests: Fix an optional propsheet message id.
      cmd/tests: Comment out test that shows a popup on Windows.
      po: Update files for previous commit.
      sc/tests: Remove todo from a test that succeeds now.
      ntdll/tests: Remove an unreliable test.
      configure: Remove some obsolete checks.
      configure: Remove some no longer needed program checks.
      configure: Remove some no longer needed header checks.

Alexey Lushnikov (1):
      gdi32: Actually return the device context and bitmap from get_bitmap_info().

Alistair Leslie-Hughes (3):
      dplayx: Merged IDirectPlayLobby/2A in to IDirectPlayLobby3A.
      dplayx: Use a single reference count for IDirectPlayLobby interfaces.
      dplayx: Merged IDirectPlayLobby/2 in to IDirectPlayLobby3.

Anton Baskanov (26):
      dpwsockx: Remove endianness conversion macros.
      dpwsockx: Remove unused DPWS_DATA fields.
      dplayx: Don't crash if sdesc is NULL in EnumSessions().
      dplayx/tests: Use CRT allocation functions.
      dplayx/tests: Test client-side of EnumSessions() separately.
      dplayx: Check dwSize of DPSESSIONDESC2 in EnumSessions().
      dpwsockx: Call WSACleanup() in ShutdownEx().
      dpwsockx: Start listening for incoming TCP connections in EnumSessions().
      dpwsockx: Broadcast enumeration request in EnumSessions().
      dplayx/tests: Report lines correctly in session enumeration callback.
      dplayx/tests: Retry enumeration manually instead of returning TRUE from the callback.
      dpwsockx: Add a background thread.
      dpwsockx: Accept incoming TCP connections.
      dpwsockx: Receive TCP messages.
      dplayx: Add a separate session list for walking.
      dplayx: Put the sync enumeration code before the async one.
      dplayx: Move enumeration reset and prune out of DP_InvokeEnumSessionCallbacks().
      dplayx: Enter critical section when accessing the session cache.
      dplayx: Restart session enumeration when the callback returns TRUE.
      dplayx: Respect timeout set by session enumeration callback.
      dplayx: Add a string copying helper function and use it in DP_CalcSessionDescSize() and DP_CopySessionDesc().
      dplayx: Reduce nesting of the async enumeration code.
      dplayx: Send password in session enumeration request.
      dplayx/tests: Correctly compute session enumeration reply size.
      dplayx: Check the message size before access.
      dplayx: Check ENUMSESSIONSREPLY size before access.

Aurimas Fišeras (1):
      po: Update Lithuanian translation.

Biswapriyo Nath (3):
      include: Add Calendar runtimeclass in windows.globalization.idl.
      include: Add DXVA_PicParams_HEVC_RangeExt declaration in dxva.h.
      include: Add new GUIDs in dxva.h.

Brendan Shanks (8):
      winemac: Marshal user-mode callback pointers in macdrv_init wow64 thunk.
      Revert "configure: Don't define HAVE_CLOCK_GETTIME on macOS.".
      ntdll: Always use mach_continuous_time() on macOS.
      sfnt2fon: Replace sprintf with snprintf to avoid deprecation warnings on macOS.
      widl: Replace sprintf with snprintf to avoid deprecation warnings on macOS.
      winebuild: Replace sprintf with snprintf to avoid deprecation warnings on macOS.
      wmc: Replace sprintf with snprintf to avoid deprecation warnings on macOS.
      wrc: Replace sprintf with snprintf to avoid deprecation warnings on macOS.

Charlotte Pabst (2):
      comdlg32: Allow entering a filter in the itemdlg file name field.
      comdlg32/tests: Add tests for itemdlg file name field filters.

Danyil Blyschak (1):
      mlang: Use EnumFontFamiliesEx() in map_font().

Dmitry Timoshkov (4):
      msiexec: Avoid crash if PackageName is NULL.
      advapi32/tests: Add some tests for SERVICE_CONFIG_DELAYED_AUTO_START_INFO.
      services: Add support for ChangeServiceConfig2(SERVICE_CONFIG_DELAYED_AUTO_START_INFO).
      services: Add support for QueryServiceConfig2(SERVICE_CONFIG_DELAYED_AUTO_START_INFO).

Elizabeth Figura (14):
      wined3d: Invalidate the VS from wined3d_device_apply_stateblock() when light state changes.
      wined3d: Invalidate the VS from wined3d_device_apply_stateblock() when the light type changes.
      wined3d: Invalidate the VS from wined3d_device_apply_stateblock() when the vertex declaration changes.
      wined3d: Invalidate the VS from wined3d_device_apply_stateblock() when WINED3D_TSS_TEXCOORD_INDEX changes.
      wined3d: Add an append_structure() helper for get_physical_device_info().
      wined3d: Don't use structures from unsupported extensions in get_physical_device_info().
      wined3d: Do not require EXT_vertex_attribute_divisor.
      winevulkan: Separate a parse_array_len() helper.
      winevulkan: Handle multidimensional static arrays.
      wined3d: Use W as a fog source with shaders and a non-orthogonal matrix.
      wined3d: Use output specular W as a fog source if the shader does not output oFog.
      wined3d: Invalidate STATE_SHADER when WINED3D_TS_PROJECTION changes.
      wined3d: Invalidate the VS from wined3d_device_apply_stateblock() when fog states change.
      wined3d: Invalidate the VS from wined3d_device_apply_stateblock() when WINED3D_RS_VERTEXBLEND changes.

Eric Pouech (10):
      cmd/tests: Add some more tests.
      cmd: Expand command before searching for builtin commands.
      cmd: Introduce internal command to change drive.
      cmd: Introduce helper to search for external program.
      cmd: Introduce helper for running a builtin command.
      cmd: Integrate builtin command search in search_program() helper.
      cmd: Speed-up external command look up.
      cmd: Get rid of circular ref for internal/external commands.
      winedbg: Give user feedback when attaching to a second process.
      dbghelp: Fix discrimination of local variable / parameter in PDB files.

Esme Povirk (3):
      msiexec: Fix allocation to include NULL terminator.
      user32: Implement listbox MSAA events on click.
      gdiplus: Fix assignment of result in GdipCloneBrush.

Francisco Casas (3):
      quartz: Emit FIXME when the rendering surface is smaller than the source in VMR7.
      quartz: Properly copy data to render surfaces of planar formats in VMR7.
      quartz: Align src_pitch for planar formats.

Gabriel Ivăncescu (13):
      mshtml: Don't cast to int to bound the timer.
      mshtml: Remember if timer was blocked.
      mshtml: Don't process tasks recursively.
      mshtml: Don't process tasks recursively from script runners.
      mshtml: Don't process tasks recursively from Gecko events.
      mshtml: Use designated initializers for function_dispex.
      mshtml: Move formatting of the builtin func disp string to a helper.
      mshtml: Implement retrieving the builtin method props for the legacy function objects.
      jscript: Use proper dispatch flags to retrieve the enumerator.
      jscript: Use deferred fill-in if available to fill the exception info.
      mshtml/tests: Add more host object related tests for IE9+ modes.
      mshtml: Return MSHTML_E_INVALID_PROPERTY when trying to construct a legacy function object.
      jscript: Return JS_E_OBJECT_NOT_COLLECTION when object has no DISPID_NEWENUM.

Gijs Vermeulen (1):
      user32: Add CreateSyntheticPointerDevice stub.

Hans Leidekker (1):
      odbc32: Only call process_detach() if the unixlib was successfully loaded.

Jacek Caban (7):
      include: Define __imp_aux symbols in __ASM_DEFINE_IMPORT macro on ARM64EC.
      widl: Use alias qualified names in winrt mode in write_type_left.
      tools: Use GetModuleFileNameA in get_bindir on Windows targets.
      tools: Use /proc/self/exe in get_bindir on Cygwin targets.
      include: Use __has_declspec_attribute in corecrt.h.
      include: Remove DECLSPEC_ALIGN define from sys/stat.h.
      include: Use __has_declspec_attribute in basetsd.h.

Louis Lenders (1):
      uxtheme: Add stub for FlushMenuThemes.

Michael Ehrenreich (2):
      kernelbase: Fix EnumSystemLocalesA/W filtering of default/alternate sort orders.
      kernel32/tests: Add basic tests for EnumSystemLocalesA/W.

Nikolay Sivov (1):
      d2d1/tests: Add a few tests for ComputeArea().

Owen Rudge (2):
      sc: Return error value rather than 1 on failure.
      ntdll: Ensure Unix path strings are wrapped with debugstr_a in traces.

Paul Gofman (6):
      ntdll: Do not call LDR notifications during process shutdown.
      ws2_32/Tests: Add tests for send buffering.
      ntdll: Avoid short writes on nonblocking sockets.
      ntdll: Locally duplicated socket fd for background send.
      ntdll: Don't cancel background socket sends.
      server: Correct STATUS_NOT_FOUND return from (cancel_async).

Rémi Bernon (112):
      winex11: Resize offscreen client surfaces after they are presented.
      winex11: Create OpenGL client windows in window DPI units.
      winex11: Detach offscreen OpenGL windows after creation.
      winex11: Introduce a new present_gl_drawable helper.
      winex11: Implement offscreen window presents with NtGdiStretchBlt.
      winex11: Drop now unnecessary X11DRV_FLUSH_GL_DRAWABLE ExtEscape.
      winex11: Use offscreen rendering to scale DPI-unaware GL windows.
      winedmo: Introduce a new internal DLL.
      winedmo: Link and initialize FFmpeg on load.
      winedmo: Export a new winedmo_demuxer_check function.
      mfsrcsnk: Stub byte stream handlers if demuxing is supported.
      winewayland: Move wayland_surface_get_client to window.c.
      winewayland: Introduce a new wayland_client_surface_attach helper.
      winewayland: Pass hwnd to and return client rect from wayland_surface_get_client.
      winewayland: Move client surface to wayland_win_data struct.
      winewayland: Detach client surfaces when they are not visible.
      mfsrcsnk: Stub the source IMFByteStreamHandler interface.
      mfsrcsnk: Stub the media source IMFMediaSource interface.
      mfsrcsnk: Implement IMFMediaSource_Shutdown for the media sources.
      mfsrcsnk: Implement IMFMediaSource_GetCharacteristics for the media sources.
      mfsrcsnk: Stub the media source IMFGetService interface.
      mfsrcsnk: Stub the media source IMFRateSupport interface.
      mfsrcsnk: Stub the media source IMFRateControl interface.
      winex11: Use the correct dnd_drop_event user dispatch callback.
      win32u: Leave window surface alpha bits to -1 when unset.
      hidclass: Combine waits for pending IRP and I/O thread shutdown.
      mfsrcsnk: Get the IMFByteStream url from MF_BYTESTREAM_ORIGIN_NAME.
      mfsrcsnk: Seek and get the media source IMFByteStream length.
      winedmo: Export new winedmo_demuxer_(create|destroy) functions.
      mfsrcsnk: Create a winedmo_demuxer object on the media sources.
      winedmo: Implement FFmpeg seek and read with user callbacks.
      winedmo: Allocate a client-side stream context with the demuxers.
      winedmo: Use the stream context to track stream position.
      winedmo: Use the stream context as a buffer for larger reads.
      winedmo: Introduce a winedmo_stream callback interface for I/O.
      winedmo: Return detected MIME type from winedmo_demuxer_create.
      winedmo: Improve MIME type detection with the stream url.
      winedmo: Detect and return stream count from winedmo_demuxer_create.
      winedmo: Compute and return total duration from winedmo_demuxer_create.
      winedmo: Export a new winedmo_demuxer_stream_type function.
      mfsrcsnk: Initialize a stream map, sorted for specific mime types.
      winevulkan: Use client rect in window DPI instead of monitor DPI.
      win32u: Pass vulkan driver private data to vulkan_surface_presented.
      winex11: Use a dedicated structure for vulkan surface private data.
      winex11: Update the vulkan surface size when it is presented.
      winex11: Update the GL client window size when it is presented.
      winex11: Only update the client window position in sync_client_position.
      winex11: Move offscreen client window helpers to init.c.
      winex11: Implement vulkan DPI scaling and child window rendering.
      winedmo: Pass stream size by value to winedmo_demuxer_create.
      winedmo: Implement video media type conversion.
      winedmo: Implement audio media type conversion.
      mfsrcsnk: Stub IMFMediaStream objects for the media source.
      mfsrcsnk: Create stream descriptors for the media source streams.
      winedmo: Implement more compressed audio/video formats conversion.
      conhost: Advertise system DPI awareness.
      win32u: Lock the window when removing a vulkan surface from its list.
      winex11: Move update_gl_drawable_size helper around.
      winex11: Resize GL drawable when necessary, if wglSwapBuffer isn't called.
      include: Introduce a new __has_declspec_attribute macro.
      include: Use winnt.h DECLSPEC_UUID definition in rpcndr.h.
      include: Use winnt.h DECLSPEC_NOVTABLE definition in rpcndr.h.
      include: Add DECLSPEC_UUID/DECLSPEC_NOVTABLE to MIDL_INTERFACE.
      include: Remove __need_wint_t/__need_wchar_t definitions.
      include: Don't import atexit from CRTs.
      include: Define __cpuid(ex) as intrinsics when possible.
      win32u: Avoid leaking window surface references with DPI scaling.
      winedmo: Fix winedmo_demuxer_create prototype in spec file.
      winedmo: Export a new winedmo_demuxer_stream_name function.
      mfsrcsnk: Fill the stream descriptors MF_SD_STREAM_NAME attribute.
      winedmo: Export a new winedmo_demuxer_stream_lang function.
      mfsrcsnk: Fill the stream descriptors MF_SD_LANGUAGE attribute.
      mfsrcsnk: Select one stream of each time, exclude others.
      win32u: Use map_dpi_rect in map_dpi_create_struct.
      win32u: Use map_dpi_rect in map_dpi_winpos.
      win32u: Split monitor_from_rect logic to a separate helper.
      win32u: Split get_monitor_info into separate helpers.
      win32u: Use get_monitor_from_handle in get_monitor_dpi.
      win32u: Introduce a new monitor_dpi_from_rect helper.
      win32u: Introduce a new monitor_info_from_rect helper.
      win32u: Introduce a new monitor_info_from_window helper.
      joy.cpl: Refresh devices list when they are plugged in or out.
      joy.cpl: Cleanup the main panel control IDs and text.
      joy.cpl: Reduce the height of some main panel controls.
      joy.cpl: Add advanced settings controls in the main panel.
      win32u: Fix inverted return condition in get_cursor_pos.
      winex11: Also resize or re-create the GL drawable with XComposite child windows.
      mfsrcsnk: Implement asynchronous media source start operation.
      mfsrcsnk: Implement asynchronous media source stop operation.
      mfsrcsnk: Implement asynchronous media source pause operation.
      winedmo: Export a new winedmo_demuxer_seek function.
      winedmo: Export a new winedmo_demuxer_read function.
      winedmo: Read sample flags, timestamps and duration.
      mfsrcsnk: Read samples from the media source demuxer.
      include: Define NULL as 0LL in C++ on 64bit archs.
      win32u: Use get_virtual_screen_rect directly within the module.
      win32u: Use is_window_rect_fullscreen directly in clip_fullscreen_window.
      win32u: Use get_window_rect directly in clip_fullscreen_window.
      win32u: Pass whether a window is fullscreen to drivers WindowPosChanged.
      winewayland: Use the new fullscreen flag instead of NtUserIsWindowRectFullScreen.
      winex11: Use the new fullscreen flag instead of NtUserIsWindowRectFullScreen.
      win32u: Remove now unused NtUserIsWindowRectFullScreen call.
      desk.cpl: Introduce new control panel applet.
      desk.cpl: Enumerate the desktop display devices.
      desk.cpl: Display the virtual desktop and monitors rects.
      desk.cpl: Implement monitor highlight and selection.
      desk.cpl: Enumerate and display available resolutions.
      desk.cpl: Add a reset button to discard display settings changes.
      desk.cpl: Update the monitor rects when changing resolutions.
      desk.cpl: Keep the monitor rectangles snapped together.
      desk.cpl: Implement monitor rectangle positioning.
      desk.cpl: Add a button to apply display settings changes.

Stefan Dösinger (1):
      netapi32: Add a stub NetFreeAadJoinInformation function.
```
