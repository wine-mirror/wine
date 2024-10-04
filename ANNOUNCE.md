The Wine development release 9.19 is now available.

What's new in this release:
  - Character tables updates to Unicode 16.0.0.
  - Better window positioning in the Wayland driver.
  - More support for network sessions in DirectPlay.
  - Support for plug&play device change notifications.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/9.x/wine-9.19.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-9.19/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 9.19 (total 11):

 - #41268  Songr 1 installation fails
 - #52208  Malus crashes on unimplemented function WS2_32.dll.WSCGetApplicationCategory
 - #56875  WordSmith 9.0 shows error message on start
 - #56975  Death to Spies: black screen during video playback
 - #57079  Quicken WillMaker Plus 2007 requires unimplemented msvcp70.dll.?getline@std@@YAAAV?$basic_istream@DU?$char_traits@D@std@@@1@AAV21@AAV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@1@@Z
 - #57139  SET changes errorlevel in .bat files
 - #57147  exit /B doesn't break for loop
 - #57205  FL Studio - ALL RECENT VERSIONS - After Wine 9.17 I cannot drag and drop audio files from file manager into the app, and then file manager crashes
 - #57215  cnc-ddraw OpenGL renderer is broken again in 9.18
 - #57240  Wine 9.18 - Regression - FL Studio (and probably other apps) don't export correct file formats anymore
 - #57242  Quicken WillMaker Plus 2007 requires unimplemented msvcp70.dll.??0?$basic_ofstream@DU?$char_traits@D@std@@@std@@QAE@PBDH@Z

### Changes since 9.18:
```
Aida Jonikienė (1):
      server: Move last_active variable initialization (Valgrind).

Alexandre Julliard (14):
      include: Use __readfsdword intrinsic on MSVC.
      include: Remove some obsolete MSVC version checks.
      widl: Output statements contained inside modules.
      gitlab: Add 'build' tag on Linux build jobs.
      tools: Upgrade the config.guess/config.sub scripts.
      server: Match formatting of the English manpage.
      programs: Formatting tweaks in the man pages.
      tools: Formatting tweaks in the man pages.
      loader: Formatting tweaks in the man pages.
      kernel32/tests: Use a different invalid character since u1806 is now valid.
      gitlab: Remove make -j options.
      documentation: Update URLs to point to the Gitlab Wiki.
      appwiz.cpl: Update URLs to point to the Gitlab Wiki.
      winedbg: Update URLs to point to the Gitlab Wiki.

Andrew Nguyen (2):
      wininet: Add additional tests for handling of user agent configuration in requests.
      wininet: Only include non-empty session user agent string in request headers.

Anton Baskanov (36):
      dplayx/tests: Test client-side of Open() separately.
      dplayx: Check session desc size first in DP_SecureOpen().
      dplayx: Return DPERR_NOSESSIONS from DP_SecureOpen() when there are no sessions.
      dpwsockx: Store name server address in Open().
      dpwsockx: Start listening for incoming TCP connections in Open().
      dplayx: Unlink and clean up the reply struct on error paths in DP_MSG_ExpectReply().
      dplayx: Return and check HRESULT from DP_MSG_ExpectReply().
      dplayx: Allow specifying multiple reply command ids in DP_MSG_ExpectReply().
      dplayx: Expect SUPERENUMPLAYERSREPLY in DP_MSG_ForwardPlayerCreation().
      dpwsockx: Return DP_OK from CreatePlayer().
      dpwsockx: Add a partial SendEx() implementation.
      dplayx: Use SendEx() instead of Send().
      dplayx: Initialize the unknown field in DP_MSG_ForwardPlayerCreation().
      dpwsockx: Set player data in CreatePlayer().
      dplayx: Use the documented message layout in DP_MSG_ForwardPlayerCreation().
      dplayx: Merge DP_CalcSessionDescSize() into DP_CopySessionDesc().
      dplayx: Add a session duplication helper function and use it in DP_SetSessionDesc() and NS_AddRemoteComputerAsNameServer().
      dplayx: Return SP message header from NS_WalkSessions() and get rid of NS_GetNSAddr().
      dplayx: Set session desc when joining a session.
      dplayx/tests: Test Open() with two enumerated sessions.
      dplayx: Find the matching session instead of using the first one.
      dplayx/tests: Test that players from SUPERENUMPLAYERSREPLY are added to the session.
      dplayx: Don't enumerate system players.
      dplayx: Add player to the system group in DP_CreatePlayer().
      dplayx: Set player data in DP_CreatePlayer().
      dplayx: Return HRESULT from DP_CreatePlayer().
      dplayx: Inform the SP about player creation in DP_CreatePlayer().
      dplayx: Set SP data in DP_CreatePlayer().
      dplayx: Return message header from DP_MSG_ExpectReply().
      dplayx: Parse SUPERENUMPLAYERSREPLY and add players to the session.
      dplayx/tests: Also check the names returned by GetPlayerName() in checkPlayerListCallback().
      dplayx: Don't check dwSize in DP_CopyDPNAMEStruct().
      dplayx: Add a name copying helper function and use it in DP_IF_GetGroupName() and DP_IF_GetPlayerName().
      dplayx: Store the names contiguously.
      dplayx: Store the names as both Unicode and ANSI.
      dplayx: Pass ANSI name when enumerating through ANSI interface.

Bernhard Kölbl (1):
      mscoree: Register mono log handler callback.

Bernhard Übelacker (2):
      ntdll: Add warning if dlopen of unixlib failed.
      mmdevapi: Add error if no driver could be initialized.

Billy Laws (3):
      ntdll: Match the new ARM64 KiUserExceptionDispatcher stack layout.
      regsvr32: Explicitly specify the target machine when relaunching.
      msi: Disable WOW64 redirection for all 64-bit package archs.

Biswapriyo Nath (9):
      include: Add new value in DWRITE_GLYPH_IMAGE_FORMATS in dcommon.idl.
      include: Add UI Automation Text Attribute ID definitions.
      include: Add UI Automation Landmark Type ID definitions.
      include: Add IDWriteFontFace7 definition in dwrite_3.idl.
      include: Add ISystemMediaTransportControls2 definition in windows.media.idl.
      include: Add DataWriter runtimeclass in windows.storage.streams.idl.
      include: Add IRandomAccessStreamReferenceStatics in windows.storage.streams.idl.
      include: Add IRandomAccessStreamReference in windows.storage.streams.idl.
      include: Add IOutputStream in windows.storage.streams.idl.

Charlotte Pabst (2):
      comdlg32: Update current itemdlg filter in SetFileTypeIndex.
      comdlg32/tests: Add test for SetFileTypeIndex updating current filter.

Connor McAdams (7):
      d3dx9/tests: Add more d3d format conversion tests.
      d3dx9/tests: Add format conversion tests for premultiplied alpha DXTn formats.
      d3dx9: Clamp source components to unorm range.
      d3dx9: Store pixel value range alongside pixel values when reading pixels.
      d3dx9: Add support for D3DFMT_Q8W8V8U8.
      d3dx9: Add support for D3DFMT_V8U8.
      d3dx9: Use format_from_d3dx_color() instead of fill_texture().

Dmitry Timoshkov (1):
      msi: Also set "MsiTrueAdminUser" property.

Elizabeth Figura (11):
      d3d11: Stub ID3D11VideoDecoder.
      d3d11: Stub ID3D11VideoDecoderOutputView.
      d3d11: Stub ID3D11VideoContext.
      wined3d: Invalidate the FFP VS when diffuse presence changes.
      wined3d: Move geometry_shader_init_stream_output().
      wined3d: Call geometry_shader_init_stream_output() from shader_set_function().
      wined3d: Remove the redundant "device" parameter to shader_set_function().
      wined3d: Merge the rest of vertex_shader_init() into shader_set_function().
      wined3d: Merge the rest of pixel_shader_init() into shader_set_function().
      win32u: Implement drawing transformed arcs.
      win32u: Implement drawing transformed round rectangles.

Eric Pouech (17):
      cmd: Rewrite part of WCMD_expand_envvar.
      cmd: Fix consecutive ! in variable expansion.
      cmd: Add tests for 'EXIT /B' inside FOR loops.
      cmd: EXIT /B shall break FOR loops.
      cmd/tests: Add tests for running .BAT files.
      cmd: Don't always set errorlevel for some builtin commands.
      cmd: Extend tests for FOR loop variables.
      cmd: Extend the range of usable variables in FOR loops.
      cmd: Better detection of %~ as modifier.
      cmd: Rewrite parsing of tokens= options in FOR /F loop.
      kernel32: Correctly advertize unicode environment for AeDebug startup.
      cmd/tests: Add test about external commands with dots.
      cmd: Fix searching external commands with dots in their basename.
      cmd/tests: Add more tests for SET command.
      cmd: Use CRT memory function for environment.
      cmd: Free environment strings.
      cmd: Fix 'SET =' invocation.

Esme Povirk (2):
      comctl32: Implement MSAA events for buttons.
      comctl32/tests: Test MSAA events for buttons.

Gabriel Ivăncescu (5):
      mshtml: Don't make hidden props enumerable.
      mshtml: Move lookup_dispid and get_dispid calls out of get_builtin_id.
      mshtml: Fix builtin style translation in removeAttribute for IE9+ modes.
      mshtml: Move the hook invocations inside of the builtin_prop* helpers.
      mshtml: Add support for host object accessor props.

Georg Lehmann (2):
      winevulkan: Avoid empty struct extension conversions.
      winevulkan: Update to VK spec version 1.3.296.

Hans Leidekker (11):
      winedump: Dump CLR metadata.
      fc: Add support for comparing files with default options.
      fc/tests: Add tests.
      findstr: Support case insensitive search.
      findstr: Support search with regular expressions.
      crypt32/tests: Fix a test failure.
      ntdll: Return success for NtSetInformationProcess(ProcessPowerThrottlingState).
      findstr: Preserve line endings.
      cmd: Call ReadConsoleW() with standard input handles only.
      imagehlp: Add a test to show that ImageGetDigestStream() supports 64-bit images.
      wine.inf: Add a couple of NTFS registry values.

Jinoh Kang (1):
      win32u: Don't release surface before passing it to create_offscreen_window_surface().

Louis Lenders (1):
      kernelbase: Add stub for FindFirstFileNameW.

Matteo Bruni (3):
      d3dx9/tests: Handle uncommon test results in test_D3DXCheckTextureRequirements().
      d3dx9/tests: Handle lack of support for D3DFMT_A1R5G5B5 in test_D3DXFillCubeTexture().
      d3dx9: Clean up color key handling in convert_argb_pixels() and point_filter_argb_pixels().

Nikolay Sivov (1):
      unicode: Update to Unicode 16.0.0.

Owen Rudge (2):
      odbc32: Wrap dlerror in debugstr_a to avoid potential buffer overflow.
      odbc32: Return SQL_NO_DATA from SQLGetDiagRec if no active connection.

Paul Gofman (18):
      ntdll: Implement RtlRbInsertNodeEx().
      ntdll: Implement RtlRbRemoveNode().
      ntdll/tests: Add tests for RTL RB tree.
      ntdll: Fill LDR_DATA_TABLE_ENTRY.BaseAddressIndexNode.
      ntdll: Use base address tree in get_modref().
      ntdll: Use base address tree in LdrFindEntryForAddress().
      ntdll/tests: Add more tests for process instrumentation callback.
      ntdll: Call instrumentation callback from wine_syscall_dispatcher on x64.
      ntdll: Call instrumentation callback for KiUserExceptionDispatcher on x64.
      ntdll: Call instrumentation callback for LdrInitializeThunk on x64.
      ntdll: Call instrumentation callback for KiUserModeCallback on x64.
      ntdll: Support old parameter layout for NtSetInformationProcess( ProcessInstrumentationCallback ).
      wow64: Fix length check in wow64_NtSetInformationProcess().
      uxtheme: Better detect the presence of default transparent colour in prepare_alpha().
      win32u: Call set_active_window() helper directly from handle_internal_message().
      win32u: Correctly fill new foreground window TID in WM_ACTIVATEAPP.
      kernel32/tests: Add test for finding loaded module when the module file is renamed.
      ntdll: Skip dll file search when getting module handle from short name.

Piotr Caban (2):
      msvcp70: Export std::getline function.
      msvcp70: Add basic_ofstream constructor implementation.

Rémi Bernon (69):
      mfplat/tests: Add tests for VIDEOINFOHEADER(2) user data conversion.
      mfplat: Fill user data when converting VIDEOINFOHEADER formats.
      winedmo: Avoid printing errors on expected statuses.
      winedmo: Set the buffer size to zero on read failure.
      mfsrcsnk: Send EOS event only when there is no more samples queued.
      mfsrcsnk: Fill the stream mapping for unknown stream types too.
      winex11: Avoid recreating the offscreen GL window if not necessary.
      win32u: Notify drivers of the child surfaces state when their ancestor moves.
      winewayland: Use window DPI for the OpenGL client surface size.
      winewayland: Call ensure_window_surface_contents with the toplevel window.
      winewayland: Keep the toplevel hwnd on the wayland_client_surface.
      winewayland: Update the client separately from the window surface updates.
      winewayland: Split wayland_win_data_update_wayland_surface helper.
      winewayland: Pass the client surface rect to wayland_surface_reconfigure_client.
      winewayland: Attach client client surfaces to their toplevel surface.
      winewayland: Let the render threads commit changes to client surfaces.
      winegstreamer: Pass H264 codec data as streamheader.
      mfsrcsnk: Serialize stream sample requests with the state callbacks.
      winedmo: Hoist the demuxer input format.
      winedmo: Simplify demuxer creation error handling.
      winedmo: Allocate a dedicated demuxer structure.
      winedmo: Move the last packet to the demuxer struct.
      winedmo: Create bitstream filters for demuxer streams.
      winedmo: Pass demuxer packets through the bitstream filters.
      winedmo: Convert H264 streams to Annex B format.
      winex11: Use NtUserGetDpiForWindow when only checking for empty rect.
      win32u: Keep per display source monitor DPI values.
      win32u: Introduce a new NtUserGetWinMonitorDpi call for drivers.
      win32u: Only reuse scaling target surface if it matches the monitor rect.
      win32u: Update the window state when display settings changes.
      user32: Stub DisplayConfigSetDeviceInfo.
      user32/tests: Add more tests for GetDpiForMonitorInternal.
      user32/tests: Add more tests with display source DPI scaling.
      user32/tests: Add tests with windows and monitor DPI scaling.
      winewayland: Move surface title change to wayland_surface_make_toplevel.
      winewayland: Call wayland_surface_clear_role in wayland_surface_destroy.
      winewayland: Introduce a new wayland_surface role enumeration.
      winewayland: Introduce a new wayland_surface_reconfigure_xdg helper.
      winewayland: Introduce a new update_wayland_surface_state_toplevel helper.
      winewayland: Use subsurfaces for unmanaged windows.
      user32/tests: Load more DPI-related functions dynamically.
      user32/tests: Move the monitor DPI tests below others.
      user32/tests: Use more commonly available resolutions.
      mfplat/tests: Test source resolver bytestream interactions.
      mfplat: Seek byte stream to the start for URL hint detection.
      win32u: Make sure to load drivers when updating the display cache.
      win32u: Always write the source current mode to the registry.
      win32u: Keep the source depth separately from the current mode.
      win32u: Remove now unnecessary GetDisplayDepth driver entry.
      win32u: Cache display source current display settings.
      win32u: Remove unnecessary GetCurrentDisplaySettings call.
      win32u: Add virtual modes when drivers report a single display mode.
      win32u: Keep track of the display source physical display mode.
      win32u: Use the current display mode as monitor rect.
      winemac: Remove unnecessary MoveWindowBits implementation.
      win32u: Pass rects in window DPI to MoveWindowBits.
      win32u: Pass a MONITOR_DPI_TYPE param to monitor_get_dpi.
      win32u: Pass a MONITOR_DPI_TYPE param to monitor_get_rect.
      win32u: Pass a MONITOR_DPI_TYPE param to NtUserGetVirtualScreenRect.
      win32u: Pass a MONITOR_DPI_TYPE param to get_monitor_from_rect.
      win32u: Use the parent window monitor DPI for child windows.
      win32u: Pass a MONITOR_DPI_TYPE param to NtUserGetWinMonitorDpi.
      dinput/tests: Add tests for IoReportTargetDeviceChange(Asynchronous).
      sechost: Pass individual parameters to I_ScRegisterDeviceNotification.
      sechost: Filter the device notifications before copying them.
      sechost: Keep device notification temporary copies in a list.
      sechost: Get rid of the device_notification_details internal struct.
      plugplay: Pass a device path to plugplay notifications.
      win32u: Read AppCompatFlags DPI awareness overrides from the registry.

Sergei Chernyadyev (5):
      shell32: Move icon related fields in notify_data into separate struct.
      shell32: Introduce a new get_bitmap_info helper.
      shell32: Cleanup some local variable names.
      shell32: Introduce a new fill_icon_info helper.
      shell32: Add support for balloon icon copying.

Tim Clem (6):
      nsiproxy: Only set the connection count from tcp_conns_enumerate_all when appropriate.
      nsiproxy: Only set the endpoint count from udp_endpoint_enumerate_all when appropriate.
      advapi32: Use CreateProcessAsUser to implement CreateProcessWithToken.
      user32/tests: Test that shell windows are per-desktop and should be set on the default desktop.
      server: Make shell, taskman, and progman windows per-desktop.
      explorer: Set the shell window when creating the Default desktop.

Vibhav Pant (17):
      bluetoothapis: Add stub for BluetoothSdpGetElementData.
      bluetoothapis/tests: Add tests for BluetoothSdpGetElementData.
      bluetoothapis: Implement BluetoothSdpGetElementData.
      bluetoothapis: Add stub for BluetoothSdpGetContainerElementData.
      bluetoothapis/tests: Add tests for BluetoothSdpGetContainerElementData.
      bluetoothapis: Implement BluetoothSdpGetContainerElementData.
      bluetoothapis: Add stub for BluetoothSdpEnumAttributes.
      bluetoothapis/tests: Add tests for BluetoothSdpEnumAttributes.
      bluetoothapis: Implement BluetoothSdpEnumAttributes.
      bluetoothapis: Add stub for BluetoothSdpGetAttributeValue.
      bluetoothapis/tests: Add tests for BluetoothSdpGetAttributeValue.
      bluetoothapis: Implement BluetoothSdpGetAttributeValue.
      ntoskrnl: Add stub for IoReportTargetDeviceChange.
      plugplay: Only broadcast WM_DEVICECHANGE for DBT_DEVTYP_DEVICEINTERFACE.
      sechost: Add support for DBT_DEVTYP_HANDLE notifications.
      user32: Add support for DBT_DEVTYP_HANDLE notifications.
      ntoskrnl: Implement IoReportTargetDeviceChange.

Vijay Kiran Kamuju (3):
      ws2_32: Add stub WSCGetApplicationCategory().
      windows.ui: Add stubs for UIViewSettings class.
      windows.ui: Add stub IInputPaneStatics implementation.

Yuxuan Shui (1):
      d3d11: Stub ID3D11VideoDevice1.

Zhiyi Zhang (9):
      user32/tests: Test that DragDetect() uses client coordinates instead of screen coordinates.
      ntdll/tests: Add RtlConvertDeviceFamilyInfoToString() tests.
      ntdll: Implement RtlConvertDeviceFamilyInfoToString().
      include: Add Windows.ApplicationModel.DesignMode runtime class.
      include: Add more Windows.Foundation.IReference<> interfaces.
      include: Add windows.ui.xaml.interop.idl.
      include: Add windows.ui.xaml.idl.
      include: Add Windows.Globalization.ApplicationLanguages runtime class.
      windows.ui: Register Windows.UI.Core.CoreWindow.
```
