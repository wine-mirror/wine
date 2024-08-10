The Wine development release 9.15 is now available.

What's new in this release:
  - Prototype and constructor objects in MSHTML.
  - More support for ODBC Windows drivers.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/9.x/wine-9.15.tar.xz>

Binary packages for various distributions will be available
from <https://www.winehq.org/download>

You will find documentation on <https://www.winehq.org/documentation>

Wine is available thanks to the work of many people.
See the file [AUTHORS][1] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/raw/wine-9.15/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 9.15 (total 18):

 - #35991  WinProladder v3.x crashes during 'PLC connect check' (async event poll worker writes to user event mask buffer whose lifetime is limited)
 - #39513  Desperados: input lag after resuming from pause
 - #51704  Final Fantasy XI Online: Short Freezes / Stutters Every Second
 - #53531  FTDI Vinculum II IDE gets "Out of memory" error on startup
 - #54861  UK's Kalender: Crashes when adding or changing event category - comctl32 related
 - #56140  ListView with a custom column sorter produces wrong results
 - #56494  Splashtop RMM v3.6.6.0 crashes
 - #56811  Jade Empire configuration tool fails to show up (only in virtual desktop mode)
 - #56984  Star Wars: Knights of the Old Republic (Steam, GOG): broken rendering when soft shadows enabled
 - #56989  Doom 3: BFG Edition fails to start in virtual desktop
 - #56993  Can not change desktop window resolution (pixel size)
 - #57005  Wine segfaults on macOS when run from install
 - #57008  _fdopen(0) does not return stdin after it was closed
 - #57012  Astra 2 needs kernel32.SetFirmwareEnvironmentVariableA
 - #57026  compile_commands.json change causes segmentation faults when running configure.
 - #57028  LTSpice will not print with WINE 9.xx on Ubuntu 24.04
 - #57033  ChessBase 17 crashes after splash screen again
 - #57042  rsaenh RSAENH_CPDecrypt crashes when an application tries to decrypt an empty string

### Changes since 9.14:
```
Alex Henrie (2):
      atl: Correct comment above AtlModuleRegisterTypeLib function.
      atl: Only warn in AtlModuleGetClassObject if the class was not found.

Alexandre Julliard (2):
      makedep: Don't add empty cflags to a compile command.
      dnsapi/tests: Update tests for winehq.org DNS changes.

Alistair Leslie-Hughes (2):
      include: Add *_SHIFT macros.
      include: Forward declare all gdiplus classes.

Billy Laws (1):
      configure: Test PE compilers after setting their target argument.

Brendan Shanks (5):
      include: Ensure that x86_64 syscall thunks have a consistent length when built with Clang.
      ntdll: Use environ/_NSGetEnviron() directly rather than caching it in main_envp.
      ntdll: Use _NSGetEnviron() instead of environ when spawning the server on macOS.
      msv1_0: Use _NSGetEnviron() instead of environ on macOS.
      mmdevapi: Remove unused critical section from MMDevice.

Connor McAdams (9):
      d3dx9/tests: Add tests for D3DXLoadSurfaceFromMemory() with a multisampled surface.
      d3dx9: Return success in D3DXLoadSurfaceFromMemory() for multisampled destination surfaces.
      d3dx9: Return failure from D3DXLoadSurfaceFromMemory() if d3dx_load_pixels_from_pixels() fails.
      d3dx9/tests: Add d3dx filter argument value tests.
      d3dx9: Introduce helper function for retrieving the mip filter value in texture from file functions.
      d3dx9: Further validate filter argument passed to D3DXFilterTexture().
      d3dx9: Validate filter argument in D3DXLoadVolumeFrom{Volume,FileInMemory,Memory}().
      d3dx9: Validate filter argument in D3DXLoadSurfaceFrom{Surface,FileInMemory,Memory}().
      d3dx9: Validate filter argument in texture from file functions.

Dmitry Timoshkov (1):
      sechost: Check both lpServiceName and lpServiceProc for NULL in StartServiceCtrlDispatcher().

Elizabeth Figura (14):
      wined3d: Invalidate push constant flags only for the primary stateblock.
      wined3d: Feed the material through a push constant buffer.
      wined3d: Move get_projection_matrix() to glsl_shader.c.
      wined3d: Feed the projection matrix through a push constant buffer.
      wined3d: Do not use the normal or modelview matrices when drawing pretransformed vertices.
      wined3d: Feed modelview matrices through a push constant buffer.
      wined3d: Pass d3d_info and stream_info pointers to wined3d_ffp_get_[vf]s_settings().
      wined3d: Feed the precomputed normal matrix through a push constant buffer.
      wined3d: Store the normal matrix as a struct wined3d_matrix.
      wined3d: Hardcode 1.0 point size for shader model >= 4.
      wined3d: Feed point scale constants through a push constant buffer.
      d3d9/tests: Test position attribute W when using the FFP.
      wined3d: Use 1.0 for position W when using the FFP.
      d3d9/tests: Add comprehensive fog tests.

Eric Pouech (1):
      cmd: Fix test failures for SET /P command.

Esme Povirk (6):
      comctl32: Handle WM_GETOBJECT in tab control.
      gdi32: Fix out-of-bounds write in EMR_ALPHABLEND handling.
      win32u: Implement EVENT_SYSTEM_CAPTURESTART/END.
      user32: Implement EVENT_OBJECT_STATECHANGE for BST_PUSHED.
      user32: Implement EVENT_OBJECT_STATECHANGE for BM_SETCHECK.
      gdi32: Bounds check EMF handle tables.

Fabian Maurer (5):
      kernel32: Add SetFirmwareEnvironmentVariableA stub.
      odbc32: In get_drivers prevent memory leak in error case (coverity).
      odbc32: In get_drivers simplify loop condition.
      iphlpapi: Add stub for SetCurrentThreadCompartmentId.
      win32u: Remove superflous null check (coverity).

Gabriel Ivăncescu (1):
      jscript: Implement arguments.caller.

Gerald Pfeifer (1):
      nsiproxy.sys: Fix the build on non-Apple, non-Linux systems.

Hans Leidekker (25):
      odbc32: Forward SQLGetConnectAttr() to the Unicode version if needed.
      odbc32: Forward SQLGetConnectOption() to the Unicode version if needed.
      odbc32: Forward SQLGetCursorName() to the Unicode version if needed.
      odbc32: Forward SQLGetDescField() to the Unicode version if needed.
      odbc32: Forward SQLGetDescRec() to the Unicode version if needed.
      odbc32: Forward SQLGetDescField() to the Unicode version if needed.
      odbc32: Forward SQLGetInfo() to the Unicode version if needed.
      odbc32: Forward SQLGetStmtAttr() to the Unicode version if needed.
      odbc32: Forward SQLGetTypeInfo() to the Unicode version if needed.
      odbc32: Forward SQLNativeSql() to the Unicode version if needed.
      odbc32: Forward SQLPrepare() to the Unicode version if needed.
      odbc32: Forward SQLPrimaryKeys() to the Unicode version if needed.
      odbc32: Forward SQLProcedureColumns() to the Unicode version if needed.
      odbc32: Forward SQLProcedures() to the Unicode version if needed.
      odbc32: Make the driver loader thread-safe.
      odbc32: Return an error when a required driver entry point is missing.
      odbc32: Forward SQLSetConnectAttr() to the Unicode version if needed.
      odbc32: Forward SQLSetConnectOption() to the Unicode version if needed.
      odbc32: Forward SQLSetCursorName() to the Unicode version if needed.
      odbc32: Forward SQLSetDescField() to the Unicode version if needed.
      odbc32: Forward SQLSetStmtAttr() to the Unicode version if needed.
      odbc32: Forward SQLSpecialColumns() to the Unicode version if needed.
      odbc32: Forward SQLStatistics() to the Unicode version if needed.
      odbc32: Forward SQLTablePrivileges() to the Unicode version if needed.
      odbc32: Forward SQLTables() to the Unicode version if needed.

Herman Semenov (1):
      gdiplus: Fixed order of adding offset and result ternary operator.

Jacek Caban (83):
      mshtml: Use dispex_next_id in NextProperty implementation.
      jscript: Ensure that external property is still valid in jsdisp_next_prop.
      mshtml: Use host object script bindings for storage objects.
      mshtml: Use host object script bindings for frame elements.
      mshtml: Use host object script bindings for iframe elements.
      mshtml: Introduce get_outer_iface and use it instead of get_dispatch_this.
      jscript: Allow host objects to specify an outer interface.
      mshtml: Return E_UNEXPECTED for unknown ids in JSDispatchHost_CallFunction.
      mshtml: Use get_prop_descs for window object.
      mshtml: Use host object script bindings for Window object.
      mshtml: Introduce get_script_global and use it instead of get_compat_mode.
      mshtml: Use HTMLPluginContainer for DispatchEx functions in object element.
      mshtml: Store property name in HTMLPluginContainer.
      mshtml: Use host object script bindings for object elements.
      mshtml: Use host object script bindings for select elements.
      mshtml: Use host object script bindings for HTMLRect.
      mshtml: Use host object script bindings for DOMTokenList.
      mshtml: Use dispex_index_prop_desc for HTMLFiltersCollection.
      mshtml: Use host object script bindings for HTMLAttributeCollection.
      mshtml: Use dispex_index_prop_desc for HTMLElementCollection.
      mshtml: Use host object script bindings for HTMLDOMChildrenCollection.
      mshtml: Use host object script bindings for HTMLStyleSheetsCollection.
      mshtml: Use host object script bindings for HTMLStyleSheet.
      mshtml: Use host object script bindings for HTMLStyleSheetRulesCollection.
      mshtml: Use host object script bindings for HTMLStyleSheetRule.
      mshtml: Use get_prop_desc for legacy function object implementation.
      mshtml: Use host object script bindings for style objects.
      mshtml: Add initial constructor implementation.
      mshtml: Store vtbl in dispex_data_t.
      mshtml: Split ensure_dispex_info.
      mshtml: Factor out init_dispatch_from_desc.
      mshtml: Add initial support for MSHTML prototype objects.
      mshtml: Don't expose prototype properties directly from object instances.
      mshtml: Store name in dispex_data_t.
      mshtml: Use proper prototype names.
      jscript: Allow using MSHTML constructors in instanceof expressions.
      maintainers: Remove shdocvw from WebBrowser control section.
      mshtml: Add support for navigator prototype objects.
      mshtml: Add support for HTMLBodyElement object.
      mshtml: Add initial support for prototype chains.
      mshtml: Add support for Element and Node prototype objects.
      mshtml: Add support for Storage prototype objects.
      mshtml: Add support for document prototype objects.
      mshtml: Add support for window prototype objects.
      include: Always declare _setjmp in setjmp.h on i386 targets.
      mshtml: Add support for image element prototype objects.
      jscript: Introduce HostConstructor function type.
      mshtml: Use host constructor script bindings for Image constructor object.
      mshtml: Use host constructor script bindings for XMLHttpRequest constructor object.
      mshtml: Add support for option element prototype objects.
      mshtml: Use host object script bindings for Option constructor object.
      mshtml: Add support for MutationObserver consturctor and prototype objects.
      include: Add DECLSPEC_CHPE_PATCHABLE definition.
      mshtml/tests: Use winetest.js helpers in xhr.js.
      mshtml: Add support for anchor element prototype objects.
      mshtml: Add support for area element prototype objects.
      mshtml: Add support for form element prototype objects.
      mshtml: Add support for frame elements prototype objects.
      mshtml: Add support for head elements prototype objects.
      mshtml: Add support for input elements prototype objects.
      mshtml: Add support for link element prototype objects.
      mshtml: Add support for object and embed element prototype objects.
      mshtml: Add support for script element prototype objects.
      mshtml: Add support for select element prototype objects.
      mshtml: Add support for style element prototype objects.
      mshtml: Add support for table and tr element prototype objects.
      mshtml: Add support for td element prototype objects.
      mshtml: Add support for textarea element prototype objects.
      mshtml: Add support for svg element prototype objects.
      mshtml: Add support for circle SVG element prototype objects.
      mshtml: Add support for tspan SVG element prototype objects.
      mshtml: Add support for document type node prototype objects.
      mshtml: Add support for text node prototype objects.
      mshtml: Get object name from its ID when possible.
      mshtml: Add support for computed style prototype objects.
      mshtml: Add support for style prototype objects.
      mshtml: Add support for current style prototype objects.
      mshtml: Add support for style sheet prototype objects.
      mshtml: Add support for style sheet list prototype objects.
      mshtml: Add support for CSS rule list prototype objects.
      mshtml: Add support for CSS rule prototype objects.
      mshtml: Add support for rect prototype objects.
      mshtml: Make mutation_observer_ctor_dispex_vtbl const.

Jacob Czekalla (6):
      comctl32/tests: Add test for listview sorting order.
      comctl32: Fix sorting for listview.
      comctl32/tests: Add test for propsheet page creation when propsheet gets initialized.
      comctl32/propsheet: Create pages with PSP_PREMATURE on initialization.
      comctl32/tests: Add test for PSN_QUERYINITIALFOCUS for the propsheet.
      comctl32: Add handling for PSN_QUERYINITIALFOCUS in prop.c.

Jactry Zeng (1):
      ntdll: Try to use page size from host_page_size() for macOS.

Jakub Petrzilka (1):
      rsaenh: Don't crash when decrypting empty strings.

Kieran Geary (1):
      shell32: Make SHGetStockIconInfo() attempt to set icon.

Martino Fontana (2):
      dinput/tests: Update tests for DIPROP_SCANCODE.
      dinput: Implement DIPROP_SCANCODE.

Matteo Bruni (1):
      d3dx9: Don't silently ignore d3dx_calculate_pixel_size() errors.

Matthias Gorzellik (3):
      hidparse: Pre-process descriptor to find TLCs.
      winebus: Store pending reads per report-id.
      hidclass: Create a child PDO for each HID TLC.

Nikolay Sivov (3):
      d3dx9/effect: Document one remaining header field.
      d3dx9/tests: Add some tests for D3DXEFFECT_DESC fields.
      d3dx9/effect: Return creator string from GetDesc().

Paul Gofman (11):
      nsiproxy.sys: Only get owning pid when needed in udp_endpoint_enumerate_all().
      mmdevapi: Return stub interface from ASM_GetSessionEnumerator().
      mmdevapi: Add implementation for IAudioSessionEnumerator.
      mmdevapi/tests: Add test for IAudioSessionEnumerator.
      ntdll: Stub NtQuerySystemInformation[Ex]( SystemProcessorIdleCycleTimeInformation ).
      kernel32: Implement QueryIdleProcessorCycleTime[Ex]().
      ntdll: Implement NtQuerySystemInformationEx( SystemProcessorIdleCycleTimeInformation ) on Linux.
      ntdll: Raise exception on failed CS wait.
      mmdevapi: Unlock session in create_session_enumerator().
      mmdevapi: Implement control_GetSessionIdentifier().
      mmdevapi: Implement control_GetSessionInstanceIdentifier().

Piotr Caban (4):
      msvcrt: Reuse standard streams after they are closed.
      ntdll: Optimize NtReadVirtualMemory for in-process reads.
      kernel32/tests: Test ReadProcessMemory on PAGE_NOACCESS memory.
      wineps.drv: Fix EMR_SETPIXELV record playback.

Rémi Bernon (27):
      win32u: Simplify offscreen surface previous surface reuse check.
      winex11: Rely on win32u previous surface reuse.
      wineandroid: Rely on win32u previous surface reuse.
      winewayland: Rely on win32u previous surface reuse.
      winemac: Remove unnecessary old window surface bounds copy.
      winemac: Rely on win32u previous surface reuse.
      win32u: Avoid sending WM_PAINT to layered window surfaces.
      win32u: Merge drivers CreateLayeredWindow with CreateWindowSurface.
      dinput/tests: Add more tests reading multiple TLCs reports.
      hidparse: Use ExFreePool to free preparsed data.
      hidclass: Keep HID device desc on the FDO device.
      hidclass: Start PDO thread in IRP_MN_START_DEVICE.
      hidclass: Allocate child PDOs array dynamically.
      win32u: Force updating the display cache when virtual desktop state changes.
      hidclass: Use poll_interval == 0 for non-polled devices.
      hidclass: Read reports with the largest input report size over TLCs.
      hidclass: Use a single lock for PDO queues and removed flag.
      hidclass: Pass HIDP_DEVICE_DESC to find_report_with_type_and_id.
      hidclass: Start the HID device thread with the FDO.
      win32u: Always use the dummy surface if a surface isn't needed.
      win32u: Fix a typo in read_source_from_registry.
      win32u: Always enumerate the primary source first.
      win32u: Remove unnecessary UpdateLayeredWindow driver entry args.
      wineandroid: Remove now unnecessary WindowPosChanging checks.
      winemac: Remove now unnecessary WindowPosChanging checks.
      winewayland: Remove now unnecessary WindowPosChanging checks.
      winex11: Remove now unnecessary WindowPosChanging checks.

Spencer Wallace (2):
      shell32/tests: Add tests for moving dir(s) to destination(s) with conflicting dir.
      shell32: Fix FO_MOVE when destination has conflicting directory.

Sven Baars (1):
      ntdll: Use the module debug channel in virtual_map_builtin_module().

Vijay Kiran Kamuju (4):
      include: Add more Task Scheduler Trigger interface definitions.
      include: Add ISessionStateChangeTrigger declaration.
      include: Added IEventTrigger declaration.
      include: Add gdiplus effect parameter structs.

Ziqing Hui (14):
      propsys: Add stubs for variant conversion functions.
      propsys/tests: Add tests for VariantToPropVariant.
      propsys: Initially implement VariantToPropVariant.
      include: Fix name of CODECAPI_AVDecVideoAcceleration_H264.
      include: Add video encoder statistical guids.
      include: Add video encoder header guids.
      include: Add video encoder chroma defines.
      include: Add video encoder color defines.
      include: Add video encode guids.
      include: Add video encoder max guids.
      include: Add video encoder inverse telecine guids.
      include: Add video encoder source defines.
      include: Add more video encoder codec api guids.
      winegstreamer/quartz_parser: Handle 0 size in read_thread.

Đorđe Mančić (1):
      kernelbase: Implement GetTempPath2A() and GetTempPath2W().
```
