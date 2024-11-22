The Wine development release 9.22 is now available.

What's new in this release:
  - Support for display mode virtualization.
  - Locale data updated to Unicode CLDR 46.
  - More support for network sessions in DirectPlay.
  - Wayland driver enabled in default configuration.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/9.x/wine-9.22.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-9.22/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 9.22 (total 19):

 - #42606  wine doesn't *fully* respect locale settings in some corner cases
 - #52105  Cygwin setup hangs (handle to \Device\NamedPipe\ used as the RootDirectory for NtCreateNamedPipeFile)
 - #53019  MusicBee: inconsistent CJK/non-Latin support with Tahoma, no support on any other font.
 - #53321  snakeqr: Unhandled page fault on write access in A_SHAFinal (needs NtdllDefWindowProc_A)
 - #56466  Dark souls remastered crashing with winewayland when trying to open "pc settings" in game
 - #56790  wine binds dedicatedServer.exe to "lo" adapter
 - #56833  Installer of LabOne 24.04 stops with error  " ... setup wizard ended prematurely ..."
 - #57072  Window is flashing when painting transluent effects
 - #57277  Wine 9.19 fails to compile
 - #57290  String Substitution not working
 - #57325  MS Office 2007 and MS Office 2013 setup fails
 - #57334  FL Studio - huge graphical glitch when moving windows inside the app
 - #57341  Heidisql  7.0 crashes
 - #57370  The Steam systray icon does not respond to mouse clicks.
 - #57382  World Of Warcraft no longer start
 - #57388  Major perf loss with blocking ReadFile() & OVERLAPPED
 - #57391  FSCTL_DISMOUNT_VOLUME does not work on drives with spaces in path
 - #57407  Windows Movie Maker hangs in Win7 mode (regression)
 - #57423  Active window no longer receives keyboard input after losing and regaining focus (only in virtual desktop)

### Changes since 9.21:
```
Agustin Principe (1):
      d2d1: Accept DXGI_FORMAT_R8G8B8A8_UNORM format for WIC targets.

Alexandre Julliard (63):
      user32: Add an ANSI version of the desktop window proc.
      user32: Add an ANSI version of the icon title window proc.
      user32: Add an ANSI version of the menu window proc.
      user32: All builtin window procs are now dual A/W.
      ntdll/tests: Use the function pointer for NtWow64QueryInformationProcess64.
      server: Fix a thread reference leak.
      ntdll: Add support for the builtin window procs table.
      user32: Use the ntdll definitions for builtin windows procs.
      user32: Use the ntdll function table for builtin window procs.
      user32: Make the builtin window procs table layout compatible with Windows.
      ntdll/tests: Add tests for the builtin window procs table.
      user32/tests: Add tests for ntdll builtin window procs.
      user32/tests: Remove some obsolete winproc tests.
      kernel32/tests: Add some tests for EnumSystemFirmwareTables().
      ntdll: Implement BIOS table enumeration.
      kernelbase: Implement EnumSystemFirmwareTables().
      tools: Download all Unicode data files before generating anything.
      nls: Update locale data to CLDR version 46.
      mpg123: Import upstream release 1.32.9.
      fluidsynth: Import upstream release 2.4.0.
      rundll32: Don't bother cleaning up at process exit.
      rundll32: Use crt allocation functions.
      rundll32: Rewrite command line parsing.
      rundll32: Restart itself if the dll is for a different architecture.
      ntdll: Always return 0 length on failure in SystemFirmwareTableInformation.
      ntdll/tests: Fix a test failure on 64-bit Windows.
      server: Print a warning if page size isn't 4k.
      server: Move the generated part of request.h to a separate header.
      server: Move the generated part of trace.c to a separate header.
      server: Simplify updating the protocol version.
      server: Use an explicit union instead of a typedef for APC calls.
      server: Use an explicit union instead of a typedef for APC results.
      server: Use an explicit struct instead of a typedef for user APCs.
      server: Use an explicit struct instead of a typedef for async I/O data.
      server: Use an explicit union instead of a typedef for message data.
      server: Use an explicit union instead of a typedef for hardware input.
      server: Use an explicit union instead of a typedef for debug event data.
      server: Use an explicit union instead of a typedef for IRP params.
      server: Use an explicit union instead of a typedef for select operations.
      win32u: Implement NtUserBuildPropList().
      win32u: Implement NtUserBuildNameList().
      user32: Move PostQuitMessage() implementation to win32u.
      user32: Move support for posting a DDE message to win32u.
      user32: Don't use server data types in clipboard.c.
      win32u: Implement NtUserQueryWindow().
      server: Use an explicit struct instead of a typedef for generic access mappings.
      server: Use an explicit struct instead of a typedef for process startup info.
      server: Use an explicit struct instead of a typedef for PE image info.
      server: Use an explicit struct instead of a typedef for window property data.
      server: Use an explicit struct instead of a typedef for cursor positions.
      win32u: Implement the remaining arguments of NtUserBuildHwndList().
      user32: Reimplement the enum window functions using NtUserBuildHwndList().
      user32: Reimplement GetDlgItem() using NtUserBuildHwndList().
      user32: Reimplement WIN_ListChildren() using NtUserBuildHwndList().
      user32: Move GetLastActivePopup() implementation to win32u.
      user32: Move GetLastInputInfo() implementation to win32u.
      server: Add a new request to find sibling windows by class name.
      win32u: Reimplement list_window_children() using NtUserBuildHwndList().
      server: Use an explicit union instead of a typedef for TCP connections.
      server: Use an explicit union instead of a typedef for UDP endpoints.
      server: Use an explicit struct instead of a typedef for object locators.
      server: Use an explicit struct instead of a typedef for register contexts.
      server: Use an explicit struct instead of a typedef for rectangles.

Alistair Leslie-Hughes (1):
      user32: Implement GetDpiAwarenessContextForProcess.

Anton Baskanov (27):
      dplayx/tests: Add missing pragma pack directives.
      dplayx/tests: Check that groups from SUPERENUMPLAYERSREPLY are added to the session.
      dplayx: Add group to the parent group in DP_CreateGroup().
      dplayx: Set group data in DP_CreateGroup().
      dplayx: Return HRESULT from DP_CreateGroup().
      dplayx: Inform the SP about group creation in DP_CreateGroup().
      dplayx: Add groups from SUPERENUMPLAYERSREPLY to the session.
      dplayx: Respect enumeration flags in EnumGroups().
      dplayx: Always set the data size in GetGroupData().
      dplayx: Factor out a function for adding player to a group.
      dplayx: Add group players from SUPERENUMPLAYERSREPLY to the group.
      dplayx/tests: Test client side of AddPlayerToGroup() separately.
      dplayx: Queue DPSYS_ADDPLAYERTOGROUP in DP_AddPlayerToGroup().
      dplayx: Send ADDPLAYERTOGROUP in AddPlayerToGroup().
      dplayx/tests: Test that group data is updated from GROUPDATACHANGED.
      dplayx: Enter the critical section when accessing the group list.
      dplayx: Handle GROUPDATACHANGED, update the group data and queue DPSYS_SETPLAYERORGROUPDATA.
      dplayx: Factor out a function for reading service providers.
      dplayx: Convert connection name to UNICODE when enumerating through UNICODE interface.
      dplayx/tests: Test UNICODE version of EnumSessions().
      dplayx: Convert session name and password to UNICODE when enumerating through UNICODE interface.
      dplayx: Forward IDirectPlay3A to IDirectPlay4A.
      dplayx: Cache connections.
      dplayx: Read connection name from descriptionW and descriptionA when available.
      dplayx: Use DP_GetRegValueW() to read SP GUID.
      dplayx: Reimplement DirectPlayEnumerateAW() using DP_GetConnections().
      dplayx: Reimplement DP_LoadSP() using DP_GetConnections().

Aurimas Fišeras (1):
      po: Update Lithuanian translation.

Bernhard Übelacker (4):
      msvcrt: Do not create a separate heap in newer msvcrt versions.
      kernel32/tests: Remove todo_wine from now succeeding heap test.
      include: Add ucrt _sprintf_l declaration.
      wineps.drv: Use locale aware variants _sprintf_l and _sscanf_l (ASan).

Billy Laws (1):
      msi: Also set x64 properties for arm64 hosts.

Biswapriyo Nath (1):
      include: Add UI Automation Annotation Type ID definitions.

Brendan McGrath (2):
      mf: Retry PROCESSINPUTNOTIFY if TRANSFORM_TYPE_NOT_SET is returned.
      mf: Send MEError when IMFStreamSink_ProcessSample fails.

Brendan Shanks (1):
      Add .gitattributes file to mark generated files.

Charlotte Pabst (2):
      jscript: Handle star and opt operators while matching global regex properly.
      jscript/tests: Add tests for star and opt operators in global regex.

Connor McAdams (6):
      d3dx9/tests: Include ddraw.h in surface.c for DDS header flag definitions.
      d3dx9: Don't attempt to save palettized surfaces in D3DXSaveSurfaceToFileInMemory().
      d3dx9/tests: Add more tests for saving surfaces as DDS files.
      d3dx9: Improve save_dds_surface_to_memory().
      d3dx9: Set the DDSCAPS_ALPHA flag when saving DDS files with a pixel format containing an alpha channel.
      d3dx9: Add support for saving paletted surfaces to DDS files.

Daniel Lehman (2):
      kernel32/tests: Add some tests for Thai and Mongolian codes.
      nls: Set alpha bit on some Thai and Mongolian codes.

Elias Norberg (4):
      wintrust: Implement CryptCATAdminAcquireContext2().
      wintrust/tests: Add CryptCATAdminAcquireContext2() tests.
      wintrust: Implement CryptCATAdminCalcHashFromFileHandle2().
      wintrust/tests: Add CryptCATAdminCalcHashFromFileHandle2() tests.

Elizabeth Figura (17):
      winevulkan: Use extend() instead of passing two separate roots to functions.
      winevulkan: Use the correct logger method.
      wined3d: Do not clamp fog in the VS.
      wined3d: Calculate the texture matrix solely from the vertex declaration.
      wined3d: Rewrite the comment in compute_texture_matrix().
      wined3d: Clear caps to zero in shader caps query functions.
      wined3d: Initialize max_blend_stages in the SPIRV fragment pipe.
      wined3d: Move shader_trace().
      wined3d: Move shader parsing to shader_set_function().
      wined3d: Create stub FFP pixel shaders.
      wined3d: Create stub FFP vertex shaders.
      wined3d: Account for HLSL FFP shaders in find_ps_compile_args().
      wined3d: Allow using the HLSL FFP replacement with GL.
      wined3d: Use the FFP HLSL pipeline for pretransformed draws as well.
      wined3d: Beginnings of an HLSL FFP vertex shader implementation.
      include: Add dxvahd.idl.
      dxva2: Stub DXVAHD_CreateDevice().

Eric Pouech (17):
      kernel32/tests: Don't hardcode page size in buffer size.
      advapi32/tests: Fix typo in manifest constant.
      advapi32: Test some other cases of process access rights mapping.
      server: Amend process rights mapping.
      cmd/tests: Add tests about substring substitution in variable expansion.
      cmd: Fix substring substitution in variable expansion.
      cmd: Implement 'touch' equivalent in COPY builtin.
      cmd/tests: Add test about IF EXIST.
      cmd: Modifiers in tilde variable expansion are case insensitive.
      cmd: Fix 'IF EXIST DIRECTORY' test condition evaluation.
      dbghelp: Don't try to load PDB for a RSDS debug directory in .buildid section.
      dbghelp: Only WARN on stripped PE images.
      dbghelp/tests: Improve SymSrvGetFileIndexInfo tests.
      dbghelp: Fill-in data in SymSrvGetFileIndexIndo if BAD_EXE_FORMAT.
      dbghelp/tests: Add retry wrapper around SymRefreshModuleList().
      dbghelp/tests: Add tests for SymRefreshModuleList().
      dbghelp: Implement SymRefreshModuleList().

Etaash Mathamsetty (1):
      explorer: Enable the Wayland driver.

Fabian Maurer (9):
      net/tests: Add test for stopping non existing service.
      net: Correct error code for stopping non existing service.
      msi/tests: Add more tests for MsiSummaryInfoPersist.
      msi: Make MsiGetSummaryInformationW open database as direct instead of transacted.
      ieframe: Add IERefreshElevationPolicy stub.
      comctl32/listbox: Close a few leaked window handles.
      comctl32/combo: Add tests for keypresses showing search functionality.
      comctl32/listbox: Add tests for keypresses showing search functionality.
      msxml3: Undo removal of xmlThrDefTreeIndentString.

Floris Renaud (1):
      po: Update Dutch translation.

Gabriel Ivăncescu (18):
      mshtml: Reset builtin function props to their default values when deleted.
      mshtml: Throw invalid action for IE8 window prop deletion.
      jscript: Add basic semi-stub implementation of GetMemberProperties.
      mshtml: Use BSTR to store global prop's name.
      mshtml: Override window's element prop directly rather than using GLOBAL_DISPEXVAR.
      mshtml: Check if window global prop still exists before returning its id.
      mshtml: Forward deletion for GLOBAL_SCRIPTVAR to the script's object.
      jscript: Delete external props before redefining them.
      jscript: Make most builtin global objects configurable.
      mshtml: Use actual referenced prop flags for window script props.
      mshtml: Don't use cycle collection for nsChannel.
      mshtml: Enumerate all own builtin props from host object's NextProperty.
      mshtml/tests: Add initial tests for prototype chain props.
      mshtml: Expose ownerDocument from NodePrototype.
      mshtml: Don't expose removeNode from NodePrototype.
      mshtml: Don't expose replaceNode from NodePrototype.
      mshtml: Don't expose swapNode from NodePrototype.
      mshtml: Set the name of the non-function constructors to the same as the object.

Georg Lehmann (1):
      winevulkan: Update to VK spec version 1.3.302.

Gerald Pfeifer (3):
      dpwsockx: Don't use true as a variable name.
      msi: Don't use bool as a variable name.
      jscript: Don't use bool as a variable name.

Hans Leidekker (3):
      wininet: Accept UTC as the equivalent of GMT.
      wininet: Use InternetTimeToSystemTimeW() to convert header values.
      iphlpapi: Sort adapters by route metric in GetAdaptersAddresses().

Haoyang Chen (5):
      winhttp/tests: Add some tests for WinHttpRequestOption_SslErrorIgnoreFlags in IWinHttpRequest_{put,get}_Option.
      winhttp: Add support WinHttpRequestOption_SslErrorIgnoreFlags in IWinHttpRequest_put_Option.
      winhttp: Add support WinHttpRequestOption_SslErrorIgnoreFlags in IWinHttpRequest_get_Option.
      winex11: Fix URL encoding for non-ASCII characters.
      wined3d: Fix a memory leak.

Jacek Caban (6):
      configure: Use -ffunction-sections for PE targets.
      winegcc: Pass -fms-hotpatch to the linker.
      configure: Use -fms-hotpatch when available.
      configure: Preserve original CFLAGS when adding LLVM flags.
      ntdll: Use proper format string for ULONG type.
      windowscodecs/tests: Always use a format string in winetest_push_context calls.

Jinoh Kang (2):
      server: Don't crash when opening null path with a console handle as RootDirectory.
      server: Don't crash when opening null path with a console server as RootDirectory.

John Chadwick (2):
      wintab32: Align WTPACKET for 32/64-bit archs.
      winex11: Remove stub tablet_get_packet wow64 thunk.

Marc-Aurel Zent (5):
      winemac.drv: Allow symbol vkeys to match on Mac virtual key codes.
      winemac.drv: Add Mac virtual key code information to the German layout.
      winemac.drv: Add additional French symbol vkeys mappings.
      include: Add Japanese IME virtual key codes to kbd.h.
      winex11: Include kbd.h instead of ime.h.

Matteo Bruni (1):
      wined3d: Allow reusing current GL context without a current RT.

Michael Müller (2):
      ntdll: Implement HashLinks field in LDR module data.
      ntdll: Use HashLinks when searching for a dll using the basename.

Mohamad Al-Jaf (3):
      icmui: Add stub dll.
      icmui: Add SetupColorMatchingW() stub.
      icmui/tests: Add some SetupColorMatchingW() tests.

Nikolay Sivov (4):
      dwrite/layout: Skip to the next typography range when current one has no features.
      comctl32/listview: Initialize hot cursor handle.
      comctl32/listview: Send LVN_HOTTRACK in response to mouse moves.
      d2d1/tests: Add some tests for WIC target formats.

Paul Gofman (4):
      kernel32/tests: Factor out is_old_loader_struct().
      kernel32/tests: Add tests for module hash links.
      server: Don't update cursor pos in set_window_pos() if window wasn't moved.
      opengl: Avoid infinite recursion in bezier_approximate() in case of degraded curve.

Piotr Caban (2):
      services: Sort services start order by start type.
      wine.inf: Set MountMgr service start option to SERVICE_BOOT_START.

Pétur Runólfsson (1):
      wtsapi32: Handle WTSSessionInfo class in WTSQuerySessionInformationW().

Rémi Bernon (63):
      d3d9/tests: Avoid creating visible windows concurrently.
      d3d9/tests: Use static class for the dummy window.
      d3d8/tests: Avoid creating visible windows concurrently.
      d3d8/tests: Use static class for the dummy window.
      wined3d: Cast format_id when comparing it to the last format index.
      winewayland: Fix surface scaling with HiDPI compositor.
      win32u: Offset the new display modes relative to the primary source.
      server: Use the monitor infos to map points from raw to virt.
      win32u: Compute monitors raw DPI from the physical / current mode ratio.
      win32u: Introduce a new registry setting to emulate modesetting.
      desk.cpl: Expose the modesetting emulation registry setting.
      winex11: Avoid requesting unnecessary _NET_WM_STATE changes.
      winex11: Avoid requesting unnecessary window config changes.
      winex11: Avoid updating _NET_WM_STATE on iconic windows.
      winex11: Simplify the control flow in WM_STATE handlers.
      winex11: Simplify the control flow in ConfigureNotify handlers.
      winex11: Reset embedded window position to 0x0 before docking it.
      winex11: Reset the window relative position when it gets reparented.
      winex11: Introduce a new host_window_send_configure_events helper.
      winex11: Retrieve the HWND for the host window's child window.
      winex11: Avoid overriding previously received ConfigureNotify events.
      winex11: Generate ConfigureNotify events for the children tree.
      winex11: Always generate ConfigureNotify events for embedded windows.
      winex11: Ignore focus changes during WM_STATE transitions.
      winex11: Use the new window state tracker to get _NET_WM_STATE value.
      winex11: Use the new window state tracker to get WM_STATE value.
      winex11: Introduce a new window_update_client_state helper.
      winex11: Introduce a new window_update_client_config helper.
      winebus: Always return success from PID effect control.
      winebus: Enable all PID effect types for wheel devices.
      winebus: Build HID report descriptors on device creation.
      winebus: Lookup device HID usage and usage page on the PE side.
      winebus: Count HID buttons and pass it to is_hidraw_enabled.
      winebus: Enable hidraw by default for various HOTAS controllers.
      dinput: Assume that clipping the cursor uses the requested rectangle.
      dinput: Only call SetCursorPos if ClipCursor fails.
      winex11: Listen to PropertyNotify events on the virtual desktop window.
      winex11: Don't expect WM_STATE events on override-redirect windows.
      winex11: Wait for pending _NET_WM_STATE before updating the client state.
      winex11: Wait for pending ConfigureNotify before updating the client state.
      winex11: Update the window client config on window state changes.
      winex11: Request window state updates asynchronously.
      d3d9/tests: Flush events after minimizing and restoring focus window.
      evr: Use D3DCREATE_MULTITHREADED device creation flag.
      winex11: Use the state tracker to decide if changes can be made directly.
      winex11: Update other window state properties within window_set_wm_state.
      winex11: Call window_set_wm_state when unmapping embedded windows.
      winex11: Get rid of the now unnecessary iconic field.
      winex11: Get rid of the now unnecessary mapped field.
      kernel32/tests: Check for the _SW_INVALID bit presence only.
      gitlab: Wait for the fvwm process to start.
      win32u: Check if parent is the desktop window in get_win_monitor_dpi.
      win32u: Map cursor pos to raw DPI before calling drivers SysCommand.
      secur32/tests: Update the tests to expect HTTP/2 headers.
      urlmon/tests: Expect "Upgrade, Keep-Alive" connection string.
      wininet: Parse multi-token Connection strings for Keep-Alive.
      winex11: Introduce a new get_window_state_updates helper.
      winex11: Generate GravityNotify events instead of ConfigureNotify.
      winex11: Avoid sending WM_WINDOWPOSCHANGING when applying window manager config.
      winex11: Delay window config request when restoring from fullscreen/maximized.
      user32/tests: Workaround a FVWM maximized window state bug.
      winex11: Update the Win32 window state outside of event handlers.
      winex11: Remove now unnecessary WindowPosChanged re-entry guards.

Vibhav Pant (5):
      setupapi: Add stub for SetupDiGetDevicePropertyKeys.
      setupapi/tests: Add tests for SetupDiGetDevicePropertyKeys.
      setupapi: Implement SetupDiGetDevicePropertyKeys.
      threadpoolwinrt: Fix potential NULL dereference in QueryInterface for IAsyncAction.
      threadpoolwinrt: Associate work items with the appropriate callback environment.

Vitor Ramos (2):
      include: Add cpp header guard to the pathcch.h.
      include: Use enum for PATHCCH_ options.

Yuxuan Shui (1):
      dinput: Keep the module around while input thread is running.

Zhiyi Zhang (14):
      urlmon: Support Uri_HOST_IDN.
      urlmon: Support Uri_DISPLAY_NO_FRAGMENT.
      urlmon: Support Uri_PUNYCODE_IDN_HOST.
      urlmon: Support Uri_DISPLAY_IDN_HOST.
      urlmon/tests: Test flags for getting properties.
      ntdll/tests: Add NtSetIoCompletionEx() tests.
      ntdll: Implement NtSetIoCompletionEx().
      wintypes: Implement RoParseTypeName().
      wintypes/tests: Add RoParseTypeName() tests.
      win32u: Print the correct index when source_enum_display_settings() fails.
      ntdll/tests: Remove a workaround for older systems.
      ntdll/tests: Add more NtSetInformationFile() tests.
      server: Set overlapped fd to signaled after setting completion information.
      kernel32: Add GetCurrentPackageInfo() stub.

Ziqing Hui (6):
      propsys: Add PropVariantToBSTR stub.
      propsys/tests: Test PropVariantToBSTR.
      propsys/tests: Test truncating for PropVariantToString.
      propsys: Implement PropVariantToBSTR.
      propsys: Use debugstr_variant for the trace in VariantToPropVariant.
      propsys: Support converting to BSTR for PropVariantToVariant.
```
