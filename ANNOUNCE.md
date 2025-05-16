The Wine development release 10.8 is now available.

What's new in this release:
  - User handles in shared memory for better performance.
  - Improvements to TIFF image support.
  - More work on the new PDB backend.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/10.x/wine-10.8.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-10.8/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 10.8 (total 18):

 - #37813  Defiance fails to connect to login Server
 - #51248  UrlGetPart produces different results from Windows
 - #54995  msys2-64/cygwin64: git clone fails with 'Socket operation on non-socket'
 - #56723  Vegas Pro 14: crash upon creating the main window
 - #56983  UI: Application using ModernWPF crashes, Windows.UI.ViewManagement.UISettings not implemented
 - #57148  cmd,for: ghidraRun.bat shows: Syntax error: unexpected IN
 - #57397  Apps hang when trying to show tooltip
 - #57424  msys2-64/cygwin64: mintty.exe not able to show bash.exe output.
 - #57575  dmsynth: incorrect condition for buffer underrun in synthsink.c:synth_sink_write_data
 - #57805  Wine CoreMIDI: Extra program change event on sending program change through MIDI Output from winmm
 - #57941  Build broken with libglvnd <=1.3.3
 - #57986  Final Fantasy XI Online window borders and content behave strangely
 - #58067  comctl32/edit: Unable to enter values in Adobe Lightroom Classic 10.4
 - #58085  foobar2000. Columns UI user interface error listing fonts using DirectWrite
 - #58185  Country Siblings: EXCEPTION_ACCESS_VIOLATION related with D3D11
 - #58191  dwrite tests fail to compile with mingw-gcc 15 due to attempt to link to truncf
 - #58207  Caret gets broken in Edit (incl Combobox) when using long texts
 - #58212  boost::interprocess::named_mutex does not work

### Changes since 10.7:
```
Adam Markowski (1):
      po: Update Polish translation.

Akihiro Sagawa (5):
      d2d1: Fix a crash in Clear method if no target is set.
      d2d1: Fix a crash in DrawBitmap method family if no target is set.
      d2d1: Fix a crash in DrawGeometry method family if no target is set.
      d2d1: Fix a crash in FillGeometry method family if no target is set.
      d2d1: Fix a crash in DrawGlyphRun method family if no target is set.

Alex Henrie (5):
      dsound/tests: Allocate right amount of memory in test_secondary8 (ASan).
      opcservices: Fix string comparison in opc_part_uri_get_rels_uri.
      mfmediaengine/tests: Mark a refcount test in test_SetCurrentTime as flaky.
      quartz: Clamp MediaSeeking_GetCurrentPosition to the stop position.
      quartz/tests: Increase the timeout of a pause test in test_media_event.

Alexandre Julliard (16):
      shell32/tests: Remove leftover file that hides succeeding tests.
      tools: Add support for syscalls with a custom entry point.
      ntdll: Define NtQueryInformationProcess as a custom syscall.
      ntdll: Don't require syscall flag on Zw functions.
      ntdll/tests: Add a test for syscall numbering.
      dbghelp: Fix wrong variable used in EX case.
      mountmgr: Remove redundant casts.
      odbc32: Remove redundant casts.
      opengl32: Remove redundant casts.
      winebth.sys: Remove redundant casts.
      winegstreamer: Remove redundant casts.
      wineps: Remove redundant casts.
      winedmo: Remove redundant casts.
      ntdll: Use the appropriate type for the xstate_features_size variable.
      include: Add some fields to the user shared data.
      include: Fix the name of the user shared data structure.

Alexandros Frantzis (2):
      winewayland: Use ARGB buffers for shaped windows.
      winex11: Initialize ex_style_mask output parameter.

Alfred Agrell (9):
      ntdll: Fix RtlUTF8ToUnicodeN for expected output ending with a surrogate pair that doesn't fit.
      include: Fix a typoed vtable call macro.
      include: Fix some typoed vtable call macros.
      include: Fix a typoed vtable call macro.
      include: Fix a typoed vtable call macro.
      include: Fix some typoed vtable call macros.
      include: Fix some typoed comments.
      include: Fix some typoed vtable call macros.
      include: Fix some typoed vtable call macros.

Alistair Leslie-Hughes (6):
      odbc32: SQLColAttributesW accept field id SQL_COLUMN_NAME.
      include: Add ApplicationRecoveryInProgress/Finished prototype.
      include: Add InitNetworkAddressControl prototype.
      include: Add some SHARD_* defines.
      include: Add some ENDSESSION_* defines.
      include: Add IHTMLOptionButtonElement interface.

Andrew Nguyen (1):
      wine.inf: Add BootId value for Session Manager\Memory Management\PrefetchParameters key.

Aurimas Fišeras (1):
      po: Update Lithuanian translation.

Bernhard Übelacker (1):
      server: Improve returned value in member WriteQuotaAvailable.

Brendan McGrath (3):
      winegstreamer: Only align the first plane in gst_video_info_align.
      winegstreamer: Allow decodebin to eliminate caps that don't use system memory.
      mfreadwrite: Use stream_index from ASYNC_SAMPLE_READY command.

Brendan Shanks (5):
      winemac: Set the OpenGL backbuffer size to the size in window DPI.
      winhttp: Use GlobalAlloc for the result of WinHttpDetectAutoProxyConfigUrl.
      winhttp: Add a cache to WinHttpDetectAutoProxyConfigUrl().
      nsiproxy: Set the name of internal threads.
      nsiproxy: Implement change notifications for NSI_IP_UNICAST_TABLE on macOS.

Connor McAdams (6):
      d3dx9: Make functions for pixel copying/conversion/filtering static.
      d3dx9: Move code for format conversion of a single pixel into a common helper function.
      d3dx9/tests: Add some color key tests.
      d3dx9: Set all color channels to zero when color keying.
      d3dx9: Calculate a range of color key channel values in d3dx_load_pixels_from_pixels() if necessary.
      d3dx9: Don't color key compressed pixel formats on 32-bit d3dx9.

Dmitry Timoshkov (3):
      combase: Find correct apartment in ClientRpcChannelBuffer_SendReceive().
      user32/tests: Add a message test for listbox redrawing after LB_SETCOUNT.
      ldap: Use correct SPN when authenticating to Kerberos DC.

Eric Pouech (17):
      dbghelp: Introduce a cache for loading blocks in new PDB reader.
      winedump: Dump some symbols for managed code.
      dbghelp: Silence symbol for managed code.
      dbghelp: Silence FIXME when dealing with empty hash table.
      dbghelp: Fix using Start parameter in TI_FINDCHILDREN request.
      dbghelp: Get rid of code quality warning.
      dbghelp: Return method_result from pdb_reader_request_cv_typeid.
      dbghelp: Load global symbols from DBI.
      dbghelp: Beef up reading compiland header helper.
      dbghelp: Build compiland table for new PDB reader.
      dbghelp: Create all symt* from new PDB reader.
      dbghelp: Workaround SAST false positive.
      dbghelp: Skip compilands without MSF stream (new PDB).
      dbghelp: Add method to query backend for symbol by address.
      dbghelp: Add method to query symbols by name.
      dbghelp: Add method to enumerate symbols.
      dbghelp: Load compilands on demand (new PDB).

Gabriel Ivăncescu (29):
      mshtml: Ignore setting non-writable external props.
      mshtml: Don't redefine deleted props in dispex_define_property.
      jscript: Properly fill the builtin props.
      mshtml: Expose "arguments" from host functions in IE9+ modes.
      mshtml: Expose "caller" from host functions in IE9+ modes.
      mshtml: Expose "arguments" and "caller" from host constructors in IE9+ modes.
      mshtml: Get rid of useless "iter" member in the attribute collection enum.
      mshtml/tests: Test iframe window navigation resetting props.
      mshtml: Fill the props in the host method instead of enumerating next prop.
      mshtml: Only fill the external props once, unless they are volatile.
      mshtml: Properly fill the prototype's "constructor" prop.
      mshtml: Properly fill the constructor's "prototype" prop.
      mshtml: Properly fill the window's constructors.
      mshtml: Properly fill the window's script vars.
      mshtml: Enumerate all own custom props if requested.
      mshtml: Implement nodeType prop for attributes.
      mshtml: Implement attributes prop for attributes.
      mshtml: Implement ownerDocument prop for attributes.
      mshtml: Implement cloneNode for attributes.
      mshtml: Implement appendChild for attributes.
      mshtml: Implement insertBefore for attributes.
      mshtml: Implement hasChildNodes for attributes.
      mshtml: Implement childNodes prop for attributes.
      mshtml: Implement firstChild prop for attributes.
      mshtml: Implement lastChild prop for attributes.
      mshtml: Implement previousSibling prop for attributes.
      mshtml: Implement nextSibling prop for attributes.
      mshtml: Implement replaceChild for attributes.
      mshtml: Implement removeChild for attributes.

Georg Lehmann (1):
      winevulkan: Update to VK spec version 1.4.315.

Hans Leidekker (2):
      winedump: Improve formatting of CLR metadata.
      wldap32: Don't map errors from ldap_set_optionW().

Jacek Caban (2):
      mshtml: Add CSSStyleDeclaration::content property implementation.
      ntdll/tests: Initialize out buffer in threadpool tests.

Jinoh Kang (2):
      include: Fix ARM64EC acq/rel barrier to match ARM64.
      user32/tests: Test BeginPaint() clipbox of cropped window with CS_PARENTDC.

Mike Swanson (1):
      notepad: Restructure menus and make hot-keys unique.

Mohamad Al-Jaf (6):
      windows.ui: Implement IUISettings4 stub.
      windows.ui: Implement IUISettings5 stub.
      coremessaging: Add IDispatcherQueueControllerStatics stub.
      coremessaging/tests: Add IDispatcherQueueControllerStatics::CreateOnDedicatedThread() tests.
      coremessaging/tests: Add CreateDispatcherQueueController() tests.
      coremessaging: Partially implement CreateDispatcherQueueController().

Nikolay Sivov (17):
      windowscodecs/tests: Use wide-char literals in tests.
      windowscodecs/converter: Add 48bppRGBHalf -> 32bppBGRA conversion path.
      windowscodecs/converter: Add 48bppRGBHalf -> 128bppRGBFloat conversion path.
      windowscodecs/tests: Add a test for 24bpp TIFF with separate sample planes.
      windowscodecs/tiff: Add support for files with separate planes.
      windowscodecs: Fix information strings for the Ico decoder.
      windowscodecs: Fix information strings for the Jpeg decoder.
      windowscodecs: Fix information strings for the Tiff decoder.
      libs: Enable JPEG codec in libtiff.
      windowscodecs/tiff: Use libjpeg for colorspace conversion YCbCr -> RGB.
      windowscodecs/tests: Add some tests for multi-frame tiffs vs SUBFILETYPE tag.
      windowscodecs/tiff: Skip frames marked with SUBFILETYPE(0x1).
      comctl32/edit: Remove change notifications on Ctrl+A selections.
      user32/edit: Reset internal capture state on WM_CAPTURECHANGED.
      comctl32/edit: Reset internal capture state on WM_CAPTURECHANGED.
      windowscodecs/tiff: Zero-initialize decoder structure.
      gdi32/uniscribe: Do not limit the number of items in ScriptStringAnalyse().

Paul Gofman (10):
      win32u: Don't redraw window in expose_window_surface() if window has surface.
      win32u: Don't inflate rect in expose_window_surface().
      shell32: Avoid writing past end of xlpFile or lpResult in SHELL_FindExecutable().
      ntdll: Bump current build number to 19045 (Win10 22H2).
      services: Create ImagePath registry value with REG_EXPAND_SZ type.
      ntdll: Allocate output string if needed for REG_EXPAND_SZ in RTL_ReportRegistryValue().
      user32: Add stub for IsWindowArranged().
      msvcp: Use _beginthreadex() in _Thrd_start().
      kernel32/tests: Add test for removing completion port association with job.
      server: Support removing completion port association with job.

Piotr Caban (12):
      netapi32: Validate bufptr argument before accessing it.
      netapi32: Fix WOW64 server_getinfo thunk.
      netapi32: Fix WOW64 share_add thunk.
      netapi32: Fix WOW64 wksta_getinfo thunk.
      netapi32: Use correct allocators for buffers returned by NetServerGetInfo and NetWkstaGetInfo.
      netapi32: Add NetShareGetInfo implementation for remote machines.
      ntdsapi: Add DsMakeSpnA implementation.
      ntdsapi: Fix referrer handling in DsMakeSpnW function.
      msado15: Don't return early in _Recordset_Open if there are no columns.
      msado15: Initialize columns in ADORecordsetConstruction_put_Rowset.
      msado15: Fix Fields object refcounting.
      msado15/tests: Fix some memory leaks.

Rémi Bernon (37):
      winebus: Ignore mouse / keyboard hidraw devices by default.
      server: Remove const qualifier from shared memory pointers.
      include: Implement ReadAcquire64.
      server: Use NTUSER_OBJ constants for user object types.
      server: Allocate a session shared memory header structure.
      server: Move the user object handle table to the shared memory.
      win32u: Use the session shared object to implement is_window.
      win32u: Use the session shared object for user handle entries.
      winewayland: Implement window surface shape and color keying.
      windows.gaming.input: Move async impl interfaces to a dedicated IDL.
      windows.media.speech: Sync async impl with windows.gaming.input.
      cryptowinrt: Sync async impl with windows.gaming.input.
      windows.devices.enumeration: Sync async impl with windows.gaming.input.
      windows.security.credentials.ui.userconsentverifier: Sync async impl with windows.gaming.input.
      server: Remove unused get_window_info atom reply parameter.
      win32u: Read the windows full handle from the shared memory.
      win32u: Use the session user entries for is_current_thread_window.
      win32u: Use the session user entries for is_current_process_window.
      win32u: Read window tid / pid from the session shared memory.
      wineandroid: Get rid of now unnecessary function loading.
      winex11: Get rid of now unnecessary function loading.
      winemac: Get rid of now unnecessary function loading.
      winewayland: Get rid of now unnecessary function loading.
      win32u: Pass thread id to next_process_user_handle_ptr.
      win32u: Get rid of struct WND tid member.
      win32u: Get rid of struct user_object.
      gitlab: Run ntoskrnl tests with win64 architecture.
      windows.devices.enumeration: Factor DeviceWatcher creation in a device_watcher_create helper.
      windows.devices.enumeration: Implement DeviceWatcher::get_Status.
      windows.devices.enumeration: Implement DeviceWatcher async state changes.
      windows.devices.enumeration: Split setupapi device enumeration to a separate helper.
      windows.media.speech/tests: Make refcount tests results deterministic.
      win32u: Don't load the display drivers in service processes.
      win32u: Rename set_window_style to set_window_style_bits.
      win32u: Add NtUserAlterWindowStyle syscall stub.
      win32u: Implement and use NtUserAlterWindowStyle.
      winebus: Skip device stop if it wasn't started.

Tim Clem (2):
      ntdll: Treat TokenElevationTypeDefault tokens in the admin group as elevated.
      advapi32/tests: Skip a token elevation test if OpenProcess fails.

Vibhav Pant (8):
      windows.devices.enumeration/tests: Add conformance tests for FindAllAsync().
      windows.devices.enumeration: Add a stubbed implementation for FindAllAsync() and DeviceInformationCollection.
      windows.devices.enumeration: Add a stubbed IDeviceInformation implementation for device interfaces.
      windows.devices.enumeration: Create IDeviceInformation objects from all present device interfaces.
      windows.devices.enumeration: Add tests for initial device enumeration in DeviceWatcher.
      windows.devices.enumeration: Implement IDeviceInformationStatics::CreateWatcher.
      windows.devices.enumeration: Implement EnumerationCompleted handler for DeviceWatcher.
      windows.devices.enumeration: Implement initial device enumeration for DeviceWatcher::Start().

Yeshun Ye (3):
      dssenh: Add 'CRYPT_VERIFYCONTEXT | CRYPT_NEWKEYSET' support for CPAcquireContext.
      dssenh/tests: Add test for CryptAcquireContextA.
      find: Support switch /i or /I.

Yuxuan Shui (3):
      win32u: Fix potential use of uninitialized variables.
      msvcrt: Use _LDOUBLE type in exported functions.
      ntdll: Also relocate entry point for builtin modules.

Zhiyi Zhang (4):
      windows.ui: Implement IWeakReferenceSource for IUISettings.
      windows.ui: Implement weak_reference_source_GetWeakReference().
      include: Add windows.ui.viewmanagement.core.idl.
      include: Add ID3D11FunctionLinkingGraph.

Ziqing Hui (11):
      shell32/tests: More use of check_file_operation helper in test_copy.
      shell32: Fail if wildcards are in target file names.
      shell32: Introduce file_entry_{init,destroy} helpers.
      shell32: Remove a useless index variable in parse_file_list.
      shell32: Rework FO_COPY operation.
      shell32: Remove useless bFromRelative member.
      shell32: Add overwrite confirmation for FO_COPY.
      shell32/tests: Add more wildcard target tests.
      shell32: Introduce has_wildcard helper.
      shell32: Handle invalid parameter correctly for SHFileOperationW.
      shell32: Introduce parse_target_file_list helper.
```
