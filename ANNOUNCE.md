The Wine development release 9.21 is now available.

What's new in this release:
  - More support for network sessions in DirectPlay.
  - Header fixes for C++ compilation.
  - I/O completion fixes.
  - More formats supported in D3DX9.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/9.x/wine-9.21.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-9.21/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 9.21 (total 16):

 - #27933  Implement sort.exe command
 - #47776  Multiple games crash on unimplemented function D3DXOptimizeVertices (Timeshift, Call of Duty 2 modding tools, Rise of Nations: Rise of Legends 2010)
 - #48235  Multiple applications need 'ntdll.NtWow64QueryInformationProcess64' (IP Camera Viewer 4.x)
 - #48796  Saints Row 2 needs GUID_WICPixelFormat48bppRGB
 - #52078  MusicBee: exception when attempting to drag tabs (  (QueryInterface for the interface with IID '{83E07D0D-0C5F-4163-BF1A-60B274051E40}' gives Exception E_NOINTERFACE))
 - #54295  Touhou Puppet Dance Performance: Shard of Dreams Can't Locate Base Game Data After Installation
 - #54623  MediRoutes crashes on unimplemented function websocket.dll.WebSocketCreateClientHandle
 - #56219  Paint Shop Pro 9.01, printing function doesn't work
 - #57164  Can't start RtlpWaitForCriticalSection
 - #57183  9.17-devel: Drag and Drop no longer works on Ubuntu 24.04 Noble
 - #57275  Black screen when using full-screen mode from version 9.18
 - #57292  unimplemented function apphelp.dll.SdbSetPermLayerKeys
 - #57296  WineHQ-devel-9.19: Renders Distorted Radio Buttons on WinXP Solitaire
 - #57314  Metal Gear Solid V gametrainer needs wmi Win32_Process executablepath property
 - #57355  Window-resize won't refresh controls on mainform
 - #57392  AnyRail msi crashes

### Changes since 9.20:
```
Alex Henrie (2):
      ntdll: Implement NtWow64QueryInformationProcess64.
      include: Annotate PFN_CMSG_ALLOC with __WINE_ALLOC_SIZE.

Alexandre Julliard (22):
      winetest: Remove strmake() len argument for consistency with other modules.
      winetest: Get default tag and URL from Gitlab CI variables.
      capstone: Allow callers to specify their memory allocators.
      capstone: Comment out error printfs.
      opengl32: Cache downloaded files in make_opengl.
      opencl: Cache downloaded files in make_opencl.
      winevulkan: Cache downloaded files in make_vulkan.
      include: Use __attribute__ in preference to __declspec.
      wow64: Fix handle conversion in NtWow64QueryInformationProcess64.
      wow64: Move NtWow64QueryInformationProcess64 to process.c.
      user32: Add some new entry points and ordinals.
      gdi32: Add some new entry points and ordinals.
      win32u: Add some new stub entry points.
      ntdll/tests: Skip the syscall relocation test if the file on disk is not updated.
      kernel32/tests: Search the current directory for newly-created dlls.
      netstat: Use wide character string literals.
      notepad: Use wide character string literals.
      oleview: Use wide character string literals.
      services: Use wide character string literals.
      taskmgr: Use wide character string literals.
      winefile: Use wide character string literals.
      wordpad: Use wide character string literals.

Alexey Prokhin (1):
      kernelbase: Set the proper error code in GetQueuedCompletionStatus{Ex} when the handle is closed.

Alistair Leslie-Hughes (11):
      include: Add rstscr.idl.
      include: Add rstxsc.idl.
      include: Add rstfnd.idl.
      include: Add rstidn.idl.
      include: Add DBGUID_DBSQL define.
      include: Add DB_S_ROWLIMITEXCEEDED define.
      include: Dbs.idl: Added DBVECTOR/DB_VARNUMERIC types.
      include: Move ISAXXMLFilter interface to after base ISAXXMLReader.
      dplayx: Use a single reference count for IDirectPlay interfaces.
      dplayx: Remove numIfaces variable in IDirectPlayLobby.
      dplayx: Use default DllCanUnloadNow implementation.

Anton Baskanov (31):
      dplayx: Inline logic from CreatePlayer() functions into DP_IF_CreatePlayer().
      dplayx: Allow storing group SP data.
      dpwsockx: Get player address from SP header and use it in SendEx().
      dpwsockx: Add partial SendToGroupEx() implementation.
      dplayx: Send CREATEPLAYER instead of ADDFORWARDREQUEST in CreatePlayer().
      dplayx: Queue DPSYS_CREATEPLAYERORGROUP on player creation.
      dplayx: Remove received message from the queue.
      dplayx: Set message sender and receiver IDs in Receive().
      dplayx: Make a deep copy of the message.
      dplayx: Set message data size in Receive().
      dplayx: Set flags correctly in CreatePlayer().
      dplayx: Always set the data size in GetPlayerData().
      dplayx/tests: Test that player from CREATEPLAYER is added to the session.
      dplay: Handle CREATEPLAYER and add player to the session.
      dplayx/tests: Use the correct system player ID in sendSuperEnumPlayersReply().
      dplayx/tests: Test client side of Send() separately.
      dplayx: Queue the message for local players in SendEx().
      dplayx: Remove the separate branch for DPID_ALLPLAYERS in SendEx().
      dplayx: Send the message in SendEx().
      dplayx/tests: Test client side of Receive() separately.
      dplayx: Handle game messages.
      dplayx: Check the buffer size in Receive().
      dplayx: Return DPERR_BUFFERTOOSMALL from Receive() if data is NULL.
      dplayx: Handle DPRECEIVE_TOPLAYER and DPRECEIVE_FROMPLAYER in Receive().
      dplayx: Enter the critical section in DP_IF_Receive().
      dplayx/tests: Test non-guaranteed Send().
      dpwsockx: Support non-guaranteed delivery.
      dplayx/tests: Test receiving UDP messages.
      dpwsockx: Receive UDP messages.
      dplayx/tests: Test that PINGREPLY is sent in reply to PING.
      dplayx: Handle PING and send PINGREPLY.

Aurimas Fišeras (1):
      po: Update Lithuanian translation.

Bartosz Kosiorek (2):
      gdiplus: Add GdipGetEffectParameterSize stub and fix GdipDeleteEffect.
      gdiplus/tests: Add GdipGetEffectParameterSize test.

Biswapriyo Nath (2):
      include: Add new property keys in propkey.h.
      include: Add windows.applicationmodel.datatransfer.idl.

Brendan Shanks (2):
      ntdll/tests: Add test for direct syscalls on x86_64.
      ntdll: Add SIGSYS handler to support syscall emulation on macOS Sonoma and later.

Christian Costa (1):
      d3dx9: Add semi-stub for D3DXOptimizeVertices().

Connor McAdams (10):
      d3dx9: Introduce d3dx_pixel_format_id enumeration.
      d3dx9: Use the d3dx_pixel_format_id enumeration inside of the d3dx_image structure.
      d3dx9: Use the d3dx_pixel_format_id enumeration inside of the DDS pixel format lookup structure.
      d3dx9: Use the d3dx_pixel_format_id enumeration inside of the WIC pixel format lookup structure.
      d3dx9/tests: Add more tests for handling JPG/PNG files.
      d3dx9: Report 24bpp RGB as 32bpp XRGB for JPG and PNG files.
      d3dx9: Add support for decoding 64bpp RGBA PNG files.
      d3dx9: Add support for decoding 48bpp RGB PNG files.
      d3dx9: Add support for loading surfaces from 48bpp RGB PNG files.
      d3dx9: Add support for loading volumes from 48bpp RGB PNG files.

Elizabeth Figura (7):
      win32u: Normalize inverted rectangles in dibdrv_RoundRect().
      win32u: Correctly handle transforms which flip in get_arc_points().
      win32u: Do not convert back to integer before finding intersections.
      win32u: Forward to Rectangle() if the ellipse width or height is zero.
      wmilib.sys: Add stub DLL.
      ntoskrnl: Stub PoRequestPowerIrp().
      ntdll: Do not queue completion for a synchronous file.

Eric Pouech (2):
      midiseq: Reduce race window when closing sequencer.
      kernelbase: Add undocumented EXTENDED_FLAGS to process attribute list.

Fabian Maurer (13):
      comdlg32/tests: Fix compilation for gcc 4.7.
      mf/tests: Fix compilation for gcc 4.7.
      wbemprox/tests: Add test for Win32_Process querying "ExecutablePath" propery.
      wbemprox: Add property "ExecutablePath" to Win32_Process.
      userenv/tests: Add another test for GetProfilesDirectoryA.
      ntdll/tests: Add more tests for RtlExpandEnvironmentStrings/_U.
      kernel32/tests: Add tests for ExpandEnvironmentStringsW.
      kernel32/tests: Add more tests for ExpandEnvironmentStringsA.
      kernel32/tests: Add ExpandEnvironmentStringsA tests for japanese.
      ntdll: Rework RtlExpandEnvironmentStrings/_U to account for corner cases.
      kernel32: Rework ExpandEnvironmentStringsW error handling.
      kernel32: Rework ExpandEnvironmentStringsA to return ansi size and fix corner cases.
      userenv: Fix GetProfilesDirectoryA return value.

Hans Leidekker (2):
      fc: Support /c option.
      wintypes/tests: Add tests for RoResolveNamespace().

Jacek Caban (2):
      winecrt0: Use version 2 of CHPE metadata.
      winegcc: Skip --no-default-config in find_libgcc.

Jactry Zeng (11):
      msvcrt/tests: Test tolower() with DBCS.
      msvcrt: Improve DBCS support for _tolower_l().
      msvcrt/tests: Test _tolower_l() with DBCS.
      msvcrt: Correct the result of non-ASCII characters for _strnicmp_l().
      msvcrt/tests: Test _stricmp() with multiple bytes characters.
      include: Add _strnicmp_l() declaration.
      msvcrt/tests: Add tests of _strnicmp_l().
      msvcrt/tests: Test toupper() with DBCS.
      msvcrt: Improve DBCS support for _toupper_l().
      msvcrt/tests: Add tests for locale information.
      msvcrt: Try to generate CTYPE data according to the given codepage.

Jinoh Kang (2):
      server: Allow creating named pipes using \Device\NamedPipe\ as RootDirectory.
      server: Implement more FSCTLs on \Device\NamedPipe and \Device\NamedPipe\.

Matteo Bruni (2):
      d3dx9/tests: Disable test sometimes crashing on Windows.
      winegstreamer: Split large WMA samples.

Maxim Karasev (1):
      klist: Migrate to KerbQueryTicketCacheExMessage.

Michael Lelli (1):
      ntdll: Use __wine_unix_spawnvp() to invoke unmount command.

Paul Gofman (7):
      ntdll: Introduce a separate per-thread object for internal completion waits.
      ntdll: Assign completion to thread when wait for completion is satisfied.
      ntdll: Handle user APCs explicitly in NtRemoveIoCompletionEx().
      server: Signal completion port waits on handle close.
      ntdll/tests: Add tests for completion port signaling.
      server: Sync cursor position on window position change.
      mountmgr.sys: Stub StorageDeviceSeekPenaltyProperty query.

Piotr Caban (1):
      kernelbase: Support backslashes when parsing relative URL in UrlCombine.

Rémi Bernon (59):
      win32u: Do not adjust old valid rect when moving child window bits.
      winex11: Introduce a new struct host_window for host-only windows.
      winex11: Create host windows recursively up to root_window.
      winex11: Keep track of the host window children of interest.
      winex11: Keep track of the host windows relative rects.
      winex11: Keep track of the host windows children window rects.
      winex11: Use the new host windows to register foreign window events.
      winex11: Generate relative ConfigureNotify on parent ConfigureNotify events.
      winex11: Get rid of the now unnecessary foreign windows.
      windows.gaming.input: Invoke event handlers outside of the critical section.
      windows.devices.enumeration: Invoke event handlers outside of the critical section.
      windows.media.speech: Invoke event handlers outside of the critical section.
      winex11: Avoid processing RRNotify events in xrandr14_get_id.
      winemac: Merge DND structures and rename constants / functions.
      winemac: Introduce a new QUERY_DRAG_DROP_ENTER query.
      winemac: Use the new win32u drag'n'drop interface.
      winex11: Set configure_serial when resizing on display mode change.
      winex11: Rename read_net_wm_state to get_window_net_wm_state.
      winex11: Move ConfigureNotify checks after computing visible rect.
      winex11: Move embedded check in update_net_wm_states / sync_window_style.
      winex11: Register PropertyChangeMask for unmanaged windows.
      win32u: Clear display device before refreshing the registry cache.
      win32u: Allocate device manager context gpu dynamically.
      win32u: Keep the source registry key on the source struct.
      win32u: Allocate device manager context source dynamically.
      win32u: Allocate device manager context monitors dynamically.
      winex11: Track WM_STATE window property requests and updates.
      winex11: Track _XEMBED_INFO window property changes.
      winex11: Introduce a new window_set_wm_state helper.
      winex11: Introduce a new window_set_net_wm_state helper.
      winex11: Track _NET_WM_STATE window property requests and updates.
      winex11: Introduce a new window_set_config helper.
      winex11: Track window config requests and updates.
      win32u: Introduce a NTGDI_RGN_MONITOR_DPI flag for NtGdiGetRandomRgn.
      winex11: Compute absolute rect using the window data window rects.
      winex11: Use the toplevel window drawable to create DCs.
      winex11: Remove now unused child_window_from_point helper.
      win32u: Compute the owner window hint on behalf of the drivers.
      include: Add __pctype_func declaration.
      include: Add some localized ctype.h function declarations.
      include: Add some struct timespec definitions.
      include: Add some ___mb_cur_max_func declarations.
      include: Add some __sys_nerr declaration.
      include: Fix _strtod_l/strtold/_strtold_l declarations.
      include: Remove non-existing _atold definition.
      include: Add math.h _(l|f)dtest function declarations.
      include: Add wcscat_s C++ wrapper definitions.
      include: Add abs C++ wrapper definitions.
      include: Add atan2l inline definition.
      include: Add max_align_t definition.
      include: Add CaptureStackBackTrace macro definition.
      include: Add SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE flag.
      include: Fix IMAGE_IMPORT_BY_NAME declaration.
      win32u: Implement get_win_monitor_dpi.
      win32u: Use MDT_RAW_DPI monitor DPI type in the drivers.
      win32u: Move some monitor info getter code around.
      win32u: Inform wineserver about the winstation monitors.
      server: Use the monitor infos to compute the virtual screen rect.
      winex11: Skip faking ConfigureNotify if state/config change is expected.

Sebastian Krzyszkowiak (1):
      mciseq: Don't seek to the end of the root chunk in RMID files.

Semenov Herman (Семенов Герман) (1):
      ole32: Fixed copy paste error with OFFSET_PS_MTIMEHIGH in UpdateRawDirEntry.

Vijay Kiran Kamuju (11):
      gdiplus: Add GdipCreateEffect implementation.
      gdiplus: Partial implementation of GdipGetEffectParameterSize.
      apphelp: Add stub SdbSetPermLayerKeys().
      apphelp: Add stub SdbGetPermLayerKeys().
      apphelp: Add stub SetPermLayerState().
      include: Add missing defines and enums for IDragSourceHelper2.
      websocket: Add stub for WebCreateClientHandle.
      websocket: Add stub for WebSocketAbortHandle.
      websocket: Add stub for WebSocketDeleteHandle.
      taskschd: Implement IDailyTrigger_put_EndBoundary.
      taskschd: Implement IDailyTrigger_get_EndBoundary.

Zhiyi Zhang (30):
      include: Add Windows.Foundation.PropertyValue runtime class.
      wintypes: Use DEFINE_IINSPECTABLE.
      wintypes: Add IPropertyValueStatics stub.
      wintypes: Support IPropertyValue primitive objects.
      wintypes: Support IPropertyValue primitive array objects.
      wintypes: Implement IReference<boolean>.
      wintypes: Implement IReference<HSTRING>.
      wintypes: Implement IReference<DOUBLE>.
      wintypes: Add IPropertyValueStatics tests.
      include: Add windows.applicationmodel.datatransfer.dragdrop.idl.
      include: Add windows.applicationmodel.datatransfer.dragdrop.core.idl.
      include: Add dragdropinterop.idl.
      dataexchange: Add initial dll.
      dataexchange: Add ICoreDragDropManagerStatics stub.
      dataexchange: Add IDragDropManagerInterop stub.
      dataexchange: Implement dragdrop_manager_interop_GetForWindow().
      dataexchange/tests: Add ICoreDragDropManagerStatics tests.
      dataexchange/tests: Add ICoreDragDropManager tests.
      dataexchange: Make core_dragdrop_manager_add_TargetRequested() return S_OK.
      iertutil: Add IUriRuntimeClassFactory stub.
      iertutil: Implement uri_factory_CreateUri().
      iertutil: Implement uri_RawUri().
      iertutil: Add uri_AbsoluteUri() semi-stub.
      iertutil/tests: Add IUriRuntimeClassFactory tests.
      iertutil/tests: Add IUriRuntimeClass tests.
      user32: Add EnableMouseInPointerForThread() stub.
      user32: Add RegisterTouchPadCapable() stub.
      include: Add IAgileReference and INoMarshal.
      combase: Implement RoGetAgileReference().
      combase/tests: Add RoGetAgileReference() tests.
```
