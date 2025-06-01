The Wine development release 10.9 is now available.

What's new in this release:
  - Bundled vkd3d upgraded to version 1.16.
  - Initial support for generating Windows Runtime metadata in WIDL.
  - Support for compiler-based exception handling with Clang.
  - EGL library support available to all graphics drivers.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/10.x/wine-10.9.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-10.9/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 10.9 (total 34):

 - #10853  Code::Blocks 8.02 IDE: some toolbar strips are too long
 - #10941  Wine does not print in a new allocated console window using AllocConsole()
 - #21666  Heavy Metal Pro fails to print Record Sheet
 - #22018  No MIDI music in Alonix
 - #26017  3D Pinball - Space Cadet: fullscreen does not work properly
 - #34331  Toolbar buttons can be activated without a proper click
 - #35882  Empire Earth 1.x display artifacts
 - #46750  MS Office 2010 on Windows 7 crash
 - #47281  wineserver uses a full CPU core with Ableton Live 10.0.6 and a custom Wine version
 - #51386  ln.exe needs KERNEL32.dll.FindFirstFileNameW
 - #51644  Implementation of wsplitpath_s required for The Legend of Heroes: Trails of Cold Steel
 - #51945  property "Size" from win32_logicaldisk gives bogus results
 - #52239  Partially invisible URL in TurnFlash's About window
 - #54899  EA app launcher problem with text display
 - #55254  Dyson Sphere Program (Steam): white artefact on opening scene
 - #55255  Dyson Sphere Program (Steam): mouse ignored if switching windows
 - #55304  Kerberos authentication stopped to work after PE wldap32 conversion
 - #55928  NtQuerySystemInformation SystemProcessInformation result misaligned
 - #56068  No "Timeout" command
 - #56186  The 32-bit dmusic:dmusic fails in a 64-bit wineprefix
 - #56859  Can't install photoshop CS6 through wine
 - #57105  Steam GPU process crash loop with 64-bit wineprefix
 - #57178  toolbar control doesn't forward WM_NOTIFY to it's original parent (affects 7-zip file manager)
 - #57387  Sega Rally Championship Demo crashes at startup if Direct3d/renderer set to GDI
 - #57613  Calling 'iphlpapi.GetIpNetTable' with a large number of network interfaces present crashes Wine builtin NSI proxy service
 - #58060  Zafehouse: Diaries demo crashes in d3dx_initialize_image_from_wic with unsupported pixel format {6fddc324-4e03-4bfe-b185-3d77768dc902}
 - #58133  Gigapixel ai no longer starts 8.3.3
 - #58203  PL/SQL Developer: All system memory gets eaten
 - #58217  The Journeyman Project 3 doesn't work in virtual desktop mode
 - #58243  Geekbench 6 crashes at start.
 - #58255  Player2 crashes
 - #58269  Build regression in wine 10.7 using clang on aarch64 (error in backend: Invalid register name "x18")
 - #58277  Virtual desktop doesn't resize correctly
 - #58285  Crystal of Atlan - Hard crash after server selection

### Changes since 10.8:
```
Adam Markowski (1):
      po: Update Polish translation.

Alanas Tebuev (4):
      comctl32/tests: Test WM_NOTIFY CBEN_ENDEDITW conversion and forwarding in toolbar.
      comctl32: Rewrite COMCTL32_array_reserve (was PAGER_AdjustBuffer).
      comctl32: Move WM_NOTIFY unicode to ansi conversion code from pager.c to commctrl.c.
      comctl32/toolbar: Forward WM_NOTIFY CBEN_ENDEDITW and CBEN_ENDEDITA.

Alex Henrie (1):
      wintrust: Initialize all cert fields in WINTRUST_AddCert.

Alexandre Julliard (33):
      configure: Correctly override DLLEXT for ARM builds.
      include: Output the name label directly in the __ASM_GLOBL macro.
      include: Add a macro to define a pointer variable in assembly.
      ntdll: Call the syscall dispatcher from assembly on thread init.
      ntdll: Set registers directly when calling __wine_syscall_dispatcher_return() on syscall fault.
      ntdll: Use %r13 to store the TEB in the syscall dispatcher.
      ntdll: Move the syscall frame and syscall table to the ntdll thread data.
      ntdll: Share is_inside_syscall() helper across platforms.
      winedump: Dump syscall numbers.
      ntdll: Add tracing for syscall entry/exit.
      ntdll: Add tracing for user callback entry/exit.
      wineps: Use the correct metrics count in generated font data.
      dbghelp: Use the __cpuid() intrinsic.
      vkd3d: Import upstream release 1.16.
      ntdll: Pre-compute the syscall frame and XState data size.
      ntdll: Only create logical processor information when needed.
      ntdll: Perform some shared data initialization before starting wineboot.
      ntdll: Perform CPU features initialization before starting wineboot.
      ntdll: Perform XState data initialization before starting wineboot.
      ntdll: Fetch the XState supported features mask from the user shared data.
      ntdll: Fetch the XState features mask from the user shared data.
      ntdll: Fetch the XState features size from the user shared data.
      ntdll: Fetch the XState compaction flag from the user shared data.
      ntdll: Fetch the XState features offsets from the user shared data.
      ntdll: Check processor features from the user shared data.
      ntdll: Get rid of the cached cpu_info structure.
      ntdll: Generate the CPU features bits from the user shared data.
      ntdll: Don't bother checking for cpuid availability.
      ntdll: Check directly for valid %fs register instead of using a syscall flag.
      ntdll: Check for wrfsbase availability from the user shared data.
      ntdll: Check for xsavec availability from the user shared data.
      ntdll: Check for fxsave availability from the user shared data.
      ntdll: Check for xsave availability from the user shared data.

Alfred Agrell (1):
      kernel32/tests: Don't import NtCompareObjects before Windows 10.

Bernhard Kölbl (2):
      setupapi/tests: Add display enum tests.
      win32u: Provide more gpu device properties.

Bernhard Übelacker (6):
      ucrtbase/tests: Avoid reading behind buffer in test_swprintf (ASan).
      timeout/tests: Move the ctrl-c tests to an intermediate process.
      nsiproxy: Avoid buffer overflow in ipv4_neighbour_enumerate_all.
      dmusic: Only extract name in EnumInstrument when the valid flag is set.
      ntdll: Align records retrieved by SystemProcessInformation.
      gdiplus: Add check of type parameter to be positive (ASan).

Brendan McGrath (5):
      mf/tests: Add negative timestamp tests for h264.
      winegstreamer: Use provided PTS and duration in video_decoder.
      winegstreamer: Avoid rounding errors when generating timestamps.
      winegstreamer: Don't generate sample timestamps for the WMV decoder.
      winegstreamer: Fixup negative input timestamps.

Brendan Shanks (1):
      ntdll: Don't swap GSBASE on macOS in user_mode_callback_return().

Charlotte Pabst (1):
      mf: Release pending items when sample grabber is stopped.

Connor McAdams (3):
      d3dx9/tests: Add tests for {1,2,4}bpp indexed PNG files.
      d3dx9: Properly handle indexed pixel formats smaller than 8bpp.
      d3dx9: Add support for decoding 2bpp indexed PNG files.

Conor McCarthy (1):
      user32: Restore the previous thread DPI awareness context before leaving CalcChildScroll().

Daniel Lehman (3):
      msvcr120/tests: Test exp.
      musl: Pass math_error callback to exp.
      msvcr120: Add cexp() implementation.

David Kahurani (1):
      xmllite/tests: Add more writer tests.

Dmitry Timoshkov (4):
      ldap: When initializing security context ask for mutual authentication.
      ldap: Sasl_encode()/sasl_decode() should treat encrypted buffer as data + token for Kerberos.
      ldap: Add support for missing ISC_RET_CONFIDENTIALITY in the server response.
      ldap: Save server attributes also on 2nd authentication step.

Elizabeth Figura (23):
      wined3d: Move the remainder of the table fog flag to wined3d_extra_vs_args.
      wined3d: Move the remainder of the shade mode flag to wined3d_extra_vs_args.
      wined3d: Move the ortho_fog flag to wined3d_extra_vs_args.
      wined3d: Remove no longer used transforms from wined3d_state.
      wined3d: Feed point size clamping constants through a push constant buffer.
      amstream/tests: Add more tests for the ddraw stream allocator.
      amstream: Avoid shadowing in ddraw_meminput_Receive().
      amstream: Rename "busy" to "pending".
      amstream: Implement the custom ddraw stream allocator.
      amstream: Fix allocator pitch tests.
      amstream: Allocate the media type in set_mt_from_desc().
      amstream/tests: Add more format tests.
      amstream: Handle 8-bit RGB in set_mt_from_desc().
      amstream: Fill the whole AM_MEDIA_TYPE in set_mt_from_desc().
      amstream: Use set_mt_from_desc() in IMediaSample::GetMediaType().
      amstream: Do not alter the current media type in SetFormat().
      amstream: Take the actual ddraw pitch into account in GetMediaType().
      amstream: Fill the source and target rects in set_mt_from_desc().
      amstream: Report a top-down DIB in set_mt_from_desc().
      wined3d/vk: Use an XY11 fixup for 2-channel SNORM formats.
      wined3d/spirv: Implement clip planes.
      wined3d/spirv: Implement point size.
      wined3d/spirv: Implement point sprite.

Eric Pouech (4):
      dbghelp: Don't dupe some exports on 64bit compilation.
      kernel32/tests: Test closing of std handles when closing console.
      wineconsole: Create child with unbound console handles.
      kernelbase: Don't close handles in FreeConsole() for a Unix console.

Esme Povirk (5):
      gdiplus: Remove repeated points before widening paths.
      comctl32: Add tests for trackbar MSAA events.
      comctl32: Implement EVENT_OBJECT_VALUECHANGE for trackbars.
      comctl32/tests: Fix childid in trackbar statechange event.
      gdiplus: Unify region rasterization in GdipFillRegion.

Gabriel Ivăncescu (5):
      include: Add IHTMLDOMAttribute3 interface.
      mshtml: Expose ownerElement from attribute nodes in IE8+ modes.
      mshtml: Implement ownerElement prop for attribute nodes.
      mshtml: Introduce dispex_builtin_prop_name that returns static string for builtins.
      mshtml: Finish the nsAString in element_has_attribute.

Hans Leidekker (12):
      secur32: Reset function tables in nego_SpInitLsaModeContext() and nego_SpAcceptLsaModeContext().
      widl: Initial support for generating Windows Runtime metadata.
      widl: Write the string stream.
      widl: Write the user string stream.
      widl: Write the blob stream.
      widl: Write the guid stream.
      winedump: Also dump the Culture column of the Assembly table.
      widl: Write the table stream.
      widl: Implement the Module table.
      widl: Implement the TypeDef table.
      widl: Implement the Assembly table.
      widl: Implement the AssemblyRef table.

Huw D. M. Davies (1):
      winemac: Ensure pglGetString is initialized before calling init_gl_info().

Jacek Caban (7):
      msvcrt: Add empty block for __EXCEPT_PAGE_FAULT.
      include: Use ULONG for RpcExceptionCode macro.
      ntdll: Pass exception code to exception handlers in __C_specific_handler.
      ntdll: Mark IsBadStringPtr functions as noinline.
      configure: Enable -fasync-exceptions when supported.
      include: Enable compiler exceptions on Clang for 64-bit MSVC targets.
      ntdll/tests: Register runtime function in test___C_specific_handler.

Jinoh Kang (2):
      server: Add `nested` parameter to redraw_window().
      server: Remove unused parameter `frame` from redraw_window().

Louis Lenders (4):
      wbemprox: Return the SMBIOSTableData[] when querying SMBiosData.
      user32: Add stub for SetProcessLaunchForegroundPolicy.
      pdh: Add stub for PdhEnumObjects{A,W}.
      pdh: Add stub for PdhGetRawCounterArrayW{A,W}.

Makarenko Oleg (1):
      winebus: Fix detection of devices with no axis.

Michael Stefaniuc (4):
      dmime: Handle (skip over) text MIDI meta events.
      dmime: Print the MIDI meta event type for unhandled messages.
      dmime: Support the MIDI meta end of track event.
      dmime: Parse and ignore MIDI key signature meta events.

Mohamad Al-Jaf (10):
      include: Add dxcore C interface macros.
      dxcore/tests: Add DXCoreCreateAdapterFactory() tests.
      dxcore: Implement DXCoreCreateAdapterFactory().
      dxcore/tests: Add IDXCoreAdapterFactory::CreateAdapterList() tests.
      dxcore: Partially implement IDXCoreAdapterFactory::CreateAdapterList().
      dxcore: Implement IDXCoreAdapterList::GetAdapterCount().
      dxcore/tests: Add IDXCoreAdapterList::GetAdapter() tests.
      dxcore: Implement IDXCoreAdapterList::GetAdapter().
      dxcore/tests: Add IDXCoreAdapter::GetProperty() tests.
      dxcore: Implement IDXCoreAdapter::GetProperty(HardwareID).

Nikolay Sivov (14):
      comctl32/taskdialog: Protect WM_COMMAND handler from partially initialized dialog.
      dwrite: Update script information with Unicode 14-16 additions.
      odbc32: Ignore SQL_ATTR_CONNECTION_POOLING on global environment.
      odbc32: Improve string attribute detection in SQLSetStmtAttr().
      odbc32: Fix data buffer size handling when forwarding SQLGetInfo -> SQLGetInfoW.
      windowscodecs/tiff: Remove remaining endianess compiler checks.
      windowscodecs/metadata: Remove remaining endianess compiler checks.
      windowscodecs/tiff: Fix source buffer size for 16bpp format (ASan).
      odbc32: Destroy child objects automatically on successful SQLDisconnect().
      comctl32/listview: Fix lbutton item selection with Shift+Ctrl.
      comctl32/listview: Do not consider key state when navigating with alphanumeric keys.
      d3dx9/tests: Add an effect compiler test for accessing functions.
      mf/samplegrabber: Initialize state -> event map in a more robust way.
      mf/clock: Initialize state -> notification map in a more robust way.

Panayiotis Talianos (1):
      wined3d: Swap wined3d_resource.heap_pointer on swapchain present.

Paul Gofman (4):
      wbemprox: Only use IOCTL_DISK_GET_DRIVE_GEOMETRY_EX if GetDiskFreeSpaceExW() fails.
      mountmgr.sys: Fill DiskNumber and ExtentLength for IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS.
      win32u: Implement NtGdiDdDDIOpenAdapterFromDeviceName().
      ntdll: Avoid excessive committed range scan in NtProtectVirtualMemory().

Piotr Caban (24):
      oledb32: Remove leading whitespaces from property name in datainit_GetDataSource.
      msado15: Don't clear VARIANT in _Record_get_ActiveConnection.
      msado15: Don't store active connection as variant.
      msado15: Add _Recordset_putref_ActiveConnection implementation.
      msado15: Add _Recordset_put_ActiveConnection implementation.
      msado15: Use _Recordset_put_ActiveConnection in recordset_Open implementation.
      msado15: Don't overwrite data in resize_recordset.
      msado15: Avoid unneeded data copy in load_all_recordset_data.
      msado15: Set recordset index in load_all_recordset_data.
      msado15: Handle rowsets with unknown number of rows in load_all_recordset_data.
      msado15: Add support for obtaining base tables and views in _Recordset_Open.
      include: Define DBSCHEMA_* GUIDs.
      include: Add dbsrst.idl.
      msado15: Return error in recordset_MoveNext when there are no more records.
      msado15: Skip columns without name in create_bindings.
      msado15: Handle DBTYPE_VARIANT in load_all_recordset_data.
      msado15: Pass DBCOLUMNINFO to append_field helper.
      msado15: Add Field_put_Precision implementation.
      msado15: Add Field_get_Precision implementation.
      msado15: Add Field_put_NumericScale implementation.
      msado15: Add Field_get_NumericScale implementation.
      msado15: Handle DBTYPE_DATE in load_all_recordset_data.
      msado15: Add connection_OpenSchema implementation.
      include: Add DBBOOKMARK enum.

Rémi Bernon (39):
      winex11: Remove unnecessary mutable_pf member.
      opengl32: Move legacy extensions fixup from winex11.
      winex11: Focus the desktop window when _NET_ACTIVE_WINDOW is None.
      winex11: Don't activate/update foreground in virtual desktop mode.
      win32u: Pass opengl_funcs pointer to init_wgl_extensions.
      win32u: Initialize opengl_funcs tables even on failure.
      win32u: Move the opengl_funcs tables out of the drivers.
      include: Generate EGL prototypes and ALL_EGL_FUNCS macro.
      win32u: Load EGL and expose functions in opengl_funcs.
      winewayland: Use the EGL functions loaded from win32u.
      wineandroid: Use the EGL functions loaded from win32u.
      Revert "winex11: Focus the desktop window when _NET_ACTIVE_WINDOW is None."
      winex11: Track _MOTIF_WM_HINTS property in the window state.
      winex11: Avoid unnecessary _NET_WM_FULLSCREEN_MONITORS requests.
      winex11: Trace more window change request serials.
      winex11: Move managed window check to window_set_net_wm_state.
      winex11: Update the current window state when ignoring events.
      win32u: Open and initialize an EGL platform display.
      wineandroid: Use the EGL display opened from win32u.
      winewayland: Use the EGL display opened from win32u.
      win32u: Introduce an EGL opengl_driver_function table.
      win32u: Implement OpenGL pixel formats over EGL configs.
      wineandroid: Use win32u for EGL display and pixel formats.
      winewayland: Use win32u for EGL display and pixel formats.
      winex11: Keep track of xinerama fullscreen monitors generation.
      winex11: Continue requesting desired window state on no-op event.
      winex11: Serialize individual _NET_WM_STATE bit changes.
      winex11: Track window pending config position / size independently.
      winex11: Keep track of the last config above flag used.
      winex11: Serialize managed window config change requests.
      winex11: Serialize window config requests with some other requests.
      winex11: Don't set _MOTIF_WM_HINTS / _NET_WM_STATE for embedded windows.
      winex11: Avoid requesting impossible state changes over and over.
      winex11: Don't delay config requests for embedded windows.
      winex11: Don't expect an event from config requests while window is unmapped.
      winex11: Wait for decoration change side effects before changing focus.
      user32/tests: Flush events before changing foreground.
      user32/tests: Make test_foregroundwindow test more reliable.
      winex11: Listen for ConfigureNotify events on the virtual desktop.

Santino Mazza (3):
      amstream: Call QueryAccept with the new media type.
      amstream/tests: Test how the ddraw allocator properties change.
      amstream: Return error when calling SetFormat with allocated samples.

Steven Flint (1):
      ntdll: Implement RtlCreateServiceSid.

Vibhav Pant (6):
      winebth.sys: Implement IOCTL_WINEBTH_RADIO_START_AUTH.
      bluetoothapis: Implement BluetoothAuthenticateDeviceEx.
      bluetoothapis: Use a wizard GUI to respond to pairing requests if an authentication callback is not registered.
      bluetoothapis/tests: Add tests for BluetoothAuthenticateDeviceEx.
      winebth.sys: Don't decrement the reference count for the DBusPendingCall received by bluez_device_pair_callback.
      bluetoothapis: Fix a potential handle leak in BluetoothAuthenticateDeviceEx.

Yuxuan Shui (4):
      wineps.drv: Avoid double destroy of PSDRV_Heap.
      ntdll: Fix out-of-bound read in RtlIpv6AddressToStringExA.
      xmllite/tests: Fix out-of-bound read.
      crypt32: Fix invalid access of list head.

Zhiyi Zhang (26):
      include: Avoid a C++ keyword.
      windows.ui: Return a newer IUISettings5 interface.
      windows.ui: Use helpers to implement IWeakReference.
      geolocation: Use helpers to implement IWeakReference.
      wintypes/tests: Use defined constants.
      wintypes: Implement IReference<UINT32>.
      wintypes: Implement IReference<BYTE>.
      wintypes: Implement IReference<FLOAT>.
      wintypes: Implement IReference<GUID>.
      wintypes: Implement IReference<INT16>.
      wintypes: Implement IReference<INT32>.
      wintypes: Implement IReference<INT64>.
      wintypes: Implement IReference<UINT64>.
      wintypes: Implement IReference<DateTime>.
      wintypes: Implement IReference<Point>.
      wintypes: Implement IReference<Rect>.
      wintypes: Implement IReference<Size>.
      wintypes: Implement IReference<TimeSpan>.
      d3d11/tests: Test newer device interfaces.
      d3d11: Add ID3D11Device3 stub.
      d3d11: Add ID3D11Device4 stub.
      d3d11: Add ID3D11Device5 stub.
      dxgi: Add IDXGIResource1 stub.
      dxgi: Add IDXGISurface2 stub.
      dxgi/tests: Add tests for subresource surfaces.
      dxgi: Implement subresource surface support.
```
