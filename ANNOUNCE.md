The Wine development release 9.14 is now available.

What's new in this release:
  - Mailslots reimplemented using server-side I/O.
  - More support for ODBC Windows drivers.
  - Still more user32 data structures in shared memory.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/9.x/wine-9.14.tar.xz>

Binary packages for various distributions will be available
from <https://www.winehq.org/download>

You will find documentation on <https://www.winehq.org/documentation>

Wine is available thanks to the work of many people.
See the file [AUTHORS][1] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/raw/wine-9.14/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 9.14 (total 20):

 - #11268  Civilization I for Windows (16-bt) incorrectly displays some dialogues
 - #32679  cmd.exe Add support for || and &&
 - #48167  1000 Mots V4.0.2 freeze when 3 words are pronounced - Underrun of data
 - #48455  Multiple .inf driver installers hang due to missing handling of architecture-specific SourceDisks{Names,Files} .inf sections (Native Instruments Native Access 1.9, WinCDEmu 4.1)
 - #49944  Multiple games fail to detect audio device (Tom Clancy's Splinter Cell: Conviction, I Am Alive)
 - #50231  Ys: Origin shows black screen during video playback
 - #54735  AOL (America Online) Desktop Beta fails when installing .net 4.8
 - #54788  AOL 5.0 Installation fails
 - #55662  Different behaviour of "set" command
 - #55798  Unreal Engine 5.2: Wine minidumps take hours to load into a debugger
 - #56751  New WoW64 compilation fails on Ubuntu Bionic
 - #56861  Background color of selected items in ListView(or ListCtrl) is white
 - #56954  Regression causes wine to generate ntlm_auth <defunct> processes
 - #56956  MSVC cl.exe 19.* fails to flush intermediate file
 - #56957  CEF application (BSG Launcher) freezes on mouse hover action
 - #56958  ChessBase 17 crashes after splash screen
 - #56969  Act of War (Direct Action, High Treason) crashes in wined3d when loading the mission
 - #56972  Warlords III: Darklords Rising shows empty screen in virtual desktop
 - #56977  accept()-ed socket fds are never marked as cacheable
 - #56994  mbstowcs for UTF8, with an exact (not overallocated) output buffer size fails

### Changes since 9.13:
```
Alex Henrie (1):
      shell32: Put temp directory in %LOCALAPPDATA%\Temp by default.

Alexandre Julliard (3):
      ntdll: Implement KiUserEmulationDispatcher on ARM64EC.
      urlmon/tests: Fix a test that fails after WineHQ updates.
      ntdll: Always clear xstate flag on collided unwind.

Alexandros Frantzis (2):
      winex11: Query proper GLX attribute for pbuffer bit.
      opengl32: Fix match criteria for WGL_DRAW_TO_PBUFFER_ARB.

Alistair Leslie-Hughes (5):
      odbc32: Handle NULL handles in SQLError/W.
      odbc32: Fake success for SQL_ATTR_CONNECTION_POOLING in SQLSetEnvAttr.
      odbc32: Handle NULL EnvironmentHandle in SQLTransact.
      msado15: Fake success in _Recordset::CancelUpdate.
      msado15: Report we support all options in _Recordset::Supports.

Arkadiusz Hiler (1):
      ntdll: Use the correct io callback when writing to a socket.

Billy Laws (1):
      ntdll: Map PSTATE.SS to the x86 trap flag on ARM64EC.

Biswapriyo Nath (1):
      include: Fix return type of IXAudio2MasteringVoice::GetChannelMask in xaudio2.idl.

Brendan Shanks (1):
      wine.inf: Don't register wineqtdecoder.dll.

Connor McAdams (14):
      d3d9/tests: Add tests for IDirect3DDevice9::UpdateSurface() with a multisampled surface.
      d3d9: Return failure if a multisampled surface is passed to IDirect3DDevice9::UpdateSurface().
      ddraw/tests: Add tests for preserving d3d scene state during primary surface creation.
      d3d9/tests: Add a test for device reset after beginning a scene.
      d3d8/tests: Add a test for device reset after beginning a scene.
      wined3d: Clear scene state on device state reset.
      d3dx9/tests: Make some test structures static const.
      d3dx9/tests: Reorder test structure members.
      d3dx9/tests: Add more D3DXCreateCubeTextureFromFileInMemory{Ex}() tests.
      d3dx9: Refactor texture creation and cleanup in D3DXCreateCubeTextureFromFileInMemoryEx().
      d3dx9: Cleanup texture value argument handling in D3DXCreateCubeTextureFromFileInMemoryEx().
      d3dx9: Use d3dx_image structure inside of D3DXCreateCubeTextureFromFileInMemoryEx().
      d3dx9: Add support for specifying which array layer to get pixel data from to d3dx_image_get_pixels().
      d3dx9: Add support for loading non-square cubemap DDS files into cube textures.

Daniel Lehman (3):
      odbc32: Handle NULL attribute in SQLColAttribute[W].
      odbc32: Return success for handled attributes in SQLSetConnectAttrW.
      mshtml: Add application/pdf MIME type.

Dmitry Timoshkov (3):
      odbc32: Correct 'WINAPI' placement for function pointers.
      light.msstyles: Use slightly darker color for GrayText to make text more readable.
      dwrite: Return correct rendering and gridfit modes from ::GetRecommendedRenderingMode().

Elizabeth Figura (32):
      setupapi/tests: Add more tests for SetupGetSourceFileLocation().
      setupapi: Correctly interpret the INFCONTEXT parameter in SetupGetSourceFileLocation().
      setupapi: Return the file's relative path from SetupGetSourceFileLocation().
      setupapi: Use SetupGetIntField() in SetupGetSourceFileLocation().
      ddraw: Call wined3d_stateblock_texture_changed() when the color key changes.
      wined3d: Store all light constants in a separate structure.
      wined3d: Store the cosines of the light angle in struct wined3d_light_constants.
      wined3d: Feed light constants through a push constant buffer.
      wined3d: Sort light constants by type.
      wined3d: Transform light coordinates by the view matrix in wined3d_device_apply_stateblock().
      setupapi: Use SetupGetSourceFileLocation() in get_source_info().
      setupapi: Use SetupGetSourceInfo() in get_source_info().
      setupapi/tests: Test installing an INF file with architecture-specific SourceDisks* sections.
      setupapi: Fix testing for a non-empty string in get_source_info().
      ntoskrnl/tests: Remove unnecessary bits from add_file_to_catalog().
      setupapi/tests: Make function pointers static.
      setupapi/tests: Move SetupCopyOEMInf() tests to devinst.c.
      setupapi/tests: Use a randomly generated directory and hardcoded file paths in test_copy_oem_inf().
      setupapi/tests: Use a signed catalog file in test_copy_oem_inf().
      wined3d: Avoid division by zero in wined3d_format_get_float_color_key().
      wined3d: Don't bother updating the colour key if the texture doesn't have WINED3D_CKEY_SRC_BLT.
      wined3d: Move clip plane constant loading to shader_glsl_load_constants().
      wined3d: Correct clip planes for the view transform in wined3d_device_apply_stateblock().
      wined3d: Pass stream info to get_texture_matrix().
      wined3d: Do not use the texture matrices when drawing pretransformed vertices.
      wined3d: Feed texture matrices through a push constant buffer.
      server: Make pipe ends FD_TYPE_DEVICE.
      kernel32/tests: Add more mailslot tests.
      server: Treat completion with error before async_handoff() as error.
      ntdll: Respect the "options" argument to NtCreateMailslotFile.
      server: Reimplement mailslots using server-side I/O.
      ntdll: Stub NtQueryInformationToken(TokenUIAccess).

Eric Pouech (20):
      winedump: Protect against corrupt minidump files.
      winedump: Dump comment streams in minidump.
      cmd: Add success/failure tests for pipes and drive change.
      cmd: Set success/failure for change drive command.
      cmd: Run pipe LHS & RHS outside of any batch context.
      cmd: Better test error handling for pipes.
      cmd: Enhance CHOICE arguement parsing.
      cmd: Implement timeout support in CHOICE command.
      include/mscvpdb.h: Use flexible array members for all trailing array fields.
      include/msvcpdb.h: Use flexible array members for codeview_fieldtype union.
      include/mscvpdb.h: Use flexible array members for codeview_symbol union.
      include/mscvpdb.h: Use flexible array members for codeview_type with variable.
      include/mscvpdb.h: Use flexible array members for the rest of structures.
      cmd: Some tests about tampering with current batch file.
      cmd: Link env_stack to running context.
      cmd: Split WCMD_batch() in two functions.
      cmd: Introduce helpers to find a label.
      cmd: No longer pass a HANDLE to WCMD_ReadAndParseLine.
      cmd: Save and restore file position from BATCH_CONTEXT.
      cmd: Don't keep batch file opened.

Esme Povirk (3):
      win32u: Implement EVENT_OBJECT_DESTROY.
      win32u: Implement EVENT_OBJECT_STATECHANGE for WS_DISABLED.
      win32u: Implement EVENT_OBJECT_VALUECHANGE for scrollbars.

Fabian Maurer (1):
      ntdll: Prevent double close in error case (coverity).

Fan WenJie (1):
      win32u: Fix incorrect comparison in add_virtual_modes.

Francisco Casas (2):
      quartz: Emit FIXME when the rendering surface is smaller than the source in VMR9.
      quartz: Properly copy data to render surfaces of planar formats in VMR9.

François Gouget (1):
      wineboot: Downgrade the wineprefix update message to a trace.

Gabriel Ivăncescu (11):
      mshtml: Make sure we aren't detached before setting interactive ready state.
      jscript: Make JS_COUNT_OPERATION a no-op.
      mshtml: Implement event.cancelBubble.
      mshtml: Implement HTMLEventObj's cancelBubble on top of the underlying event's cancelBubble.
      mshtml: Fix special case between stopImmediatePropagation and setting cancelBubble to false.
      mshtml: Use bitfields for the event BOOL fields.
      mshtml: Don't use -moz prefix for box-sizing CSS property.
      mshtml/tests: Add more tests with invalid CSS props for (get|set)PropertyValue.
      mshtml: Compactify the style_props expose tests for each style object into a single function.
      mshtml: Implement style msTransition.
      mshtml: Implement style msTransform.

Hans Leidekker (27):
      odbc32: Fix a couple of spec file entries.
      odbc32: Use LoadLibraryExW() instead of LoadLibraryW().
      odbc32: Forward SQLDriverConnect() to the Unicode version if needed.
      odbc32: Forward SQLGetDiagRec() to the Unicode version if needed.
      odbc32: Forward SQLBrowseConnect() to the Unicode version if needed.
      odbc32: Forward SQLColAttributes() to the Unicode version if needed.
      odbc32: Forward SQLColAttribute() to the Unicode version if needed.
      odbc32: Properly handle string length in traces.
      winhttp/tests: Mark a test as broken on old Windows versions.
      winhttp/tests: Fix test failures introduced by the server upgrade.
      odbc32: Forward SQLColumnPrivileges() to the Unicode version if needed.
      odbc32: Forward SQLColumns() to the Unicode version if needed.
      odbc32: Forward SQLConnect() to the Unicode version if needed.
      odbc32: Forward SQLDescribeCol() to the Unicode version if needed.
      odbc32: Forward SQLError() to the Unicode version if needed.
      odbc32: Handle missing Unicode driver entry points.
      odbc32: Forward SQLExecDirect() to the Unicode version if needed.
      odbc32: Forward SQLForeignKeys() to the Unicode version if needed.
      odbc32: Avoid a clang warning.
      odbc32: Get rid of the wrappers for SQLGetDiagRecA() and SQLDataSourcesA().
      odbc32/tests: Add tests for SQLTransact().
      odbc32: Fix setting the Driver registry value.
      odbc32: Find the driver filename through the ODBCINST.INI key.
      secur32: Handle GNUTLS_MAC_AEAD.
      secur32/tests: Switch to TLS 1.2 for connections to test.winehq.org.
      winhttp/tests: Mark more test results as broken on old Windows versions.
      secur32/tests: Mark some test results as broken on old Windows versions.

Herman Semenov (3):
      dbghelp: Fix misprint access to struct with invalid case.
      dplayx: Fix check structure before copy.
      gdiplus: Fixed order of adding offset and result ternary operator.

Jacek Caban (32):
      mshtml: Use host object script bindings for Attr class.
      mshtml: Use host object script bindings for event objects.
      mshtml: Use host object script bindings for PluginArray class.
      mshtml: Use host object script bindings for MimeTypeArray class.
      mshtml: Use host object script bindings for MSNamespaceInfoCollection class.
      mshtml: Use host object script bindings for MSEventObj class.
      mshtml: Use host object script bindings for XMLHttpRequest class.
      mshtml: Directly use dispex_prop_put and dispex_prop_get in HTMLElement implementation.
      mshtml: Factor out dispex_next_id.
      mshtml: Don't use BSTR in find_dispid.
      mshtml: Don't use BSTR in lookup_dispid.
      mshtml: Don't use BSTR in get_dispid.
      mshtml: Factor out dispex_get_id.
      mshtml: Use dispex_prop_put in HTMLDOMAttribute_put_nodeValue.
      mshtml: Factor out dispex_prop_name.
      jscript: Check if PROP_DELETED is actually an external property in find_prop_name.
      mshtml: Use host object script bindings for DOM nodes.
      mshtml: Use dispex_get_id in JSDispatchHost_LookupProperty.
      mshtml: Introduce get_prop_desc call.
      mshtml: Use host object script bindings for HTMLRectCollection.
      jscript: Make sure to use the right name for a prototype reference in find_prop_name_prot.
      jscript: Fixup prototype references as part of lookup.
      jscript: Use a dedicated jsclass_t entry for host objects.
      jscript: Improve invoke_prop_func error handling.
      mshtml: Explicitly specify case insensitive search in GetIDsOfNames.
      jscript: Treat external properties as volatile.
      jscript: Suport deleting host object properties.
      jscript: Support configuring host properties.
      jscript: Allow host objects to implement fdexNameEnsure.
      mshtml: Use host object script bindings for HTMLFormElement.
      mshtml: Use ensure_real_info in dispex_compat_mode.
      mshtml: Use host object script bindings for document nodes.

Jinoh Kang (1):
      server: Mark the socket as cacheable when it is an accepted or accepted-into socket.

Nikolay Sivov (2):
      winhttp/tests: Add some more tests for string options in WinHttpQueryOption().
      winhttp: Handle exact buffer length match in WinHttpQueryOption().

Paul Gofman (2):
      mshtml: Check get_document_node() result in get_node().
      dxdiagn: Fill szHardwareId for sound render devices.

Piotr Caban (4):
      ucrtbase: Fix mbstowcs on UTF8 strings.
      msvcrt: Use thread-safe functions in _ctime64_s.
      msvcrt: Use thread-safe functions in _ctime32_s.
      msvcrt: Don't access input string after NULL-byte in mbstowcs.

Rémi Bernon (16):
      ddraw/tests: Make sure the window is restored after some minimize tests.
      winex11: Reset empty window shape even without a window surface.
      server: Expose the thread input keystate through shared memory.
      win32u: Introduce a new NtUserGetAsyncKeyboardState call.
      win32u: Use the thread input shared memory in GetKeyboardState.
      win32u: Use the desktop shared memory in get_async_keyboard_state.
      server: Make the get_key_state request key code mandatory.
      server: Expose the thread input keystate lock through shared memory.
      win32u: Use the thread input shared memory for GetKeyState.
      winemac: Use window surface shape for color key transparency.
      winemac: Use window surface shape for window shape region.
      winevulkan: Fix incorrect 32->64 conversion of debug callbacks.
      winevulkan: Use integer types in debug callbacks parameter structs.
      winevulkan: Serialize debug callbacks parameter structures.
      opengl32: Use integer types in debug callbacks parameter structs.
      opengl32: Serialize debug callbacks message string.

Santino Mazza (1):
      gdiplus: Support string alignment in GdipMeasureString.

Tim Clem (2):
      nsiproxy.sys: Use the pcblist64 sysctl to enumerate TCP connections on macOS.
      nsiproxy.sys: Use the pcblist64 sysctl to enumerate UDP connections on macOS.

Vijay Kiran Kamuju (1):
      cmd: Do not set enviroment variable when no input is provided by set /p command.

Zhiyi Zhang (3):
      light.msstyles: Add Explorer::ListView subclass.
      comctl32/tests: Add more treeview background tests.
      comctl32/treeview: Use window color to fill background.

Ziqing Hui (5):
      include: Add encoder codec type guids.
      include: Add encoder common format guids.
      include: Add encoder common attribute defines.
      include: Add H264 encoder attribute guids.
      include: Add video encoder output frame rate defines.
```
