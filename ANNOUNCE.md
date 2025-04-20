The Wine development release 10.6 is now available.

What's new in this release:
  - New lexer in Command Processor.
  - PBKDF2 algorithm in Bcrypt.
  - More support for image metadata in WindowsCodecs.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/10.x/wine-10.6.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-10.6/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 10.6 (total 27):

 - #6682   IrfanView's 4.44 Help -> About window is missing a picture on the left
 - #13884  No music in Blue Wish Resurrection Plus
 - #29912  No parent button in file selection dialog
 - #31701  Alan Wake crashes on start without native d3dx9_36
 - #35652  Multiple MMORPH game launchers crash on startup or apps fail to update initial window content ('DIALOG_CreateIndirect' needs to trigger WM_PAINT)(Aeria Games 'Aura Kingdom', STOnline)
 - #39453  Graphs not rendering using gdiplus
 - #41729  2GIS 3.0 application crashes on exit.
 - #44978  Text in WC3 World Editor isn't colored properly
 - #45460  Running EVE Online keeps locking up after a few hours
 - #48121  Unity games do not fire OnApplicationFocus/OnApplicationPause events on focus regain
 - #51053  Alan Wake Crashes After Intro Cut Scene
 - #51378  Failures with `DetourCreateProcessWithDllEx` for Microsoft's Detours Library
 - #51546  Xenos 2.3.2 dll injector crashes
 - #51575  Texconv fails with mipmap error unless "-nowic" is supplied to disable WIC use
 - #51584  Zafehouse: Diaries demo needs support for pixel format DXT5 in D3DXSaveSurfaceToFileInMemory
 - #52553  Resource Hacker 5.1.8 fails to render tree view on left, shows white screen
 - #55819  when alt+tab out and in again, the input no longer working
 - #56073  Some Unity games don't receive keyboard input when using virtual desktop
 - #57283  The Queen of Heart 99 SE : corrupted visuals on KO screen
 - #57492  Players can't join Astroneer dedicated server with enabled encryption because BCryptExportKey encryption of key not yet supported
 - #57665  The Medium game launcher has no background image
 - #57738  Title of "Select Topic" window in hh.exe is not translatable
 - #57951  Ultrakill: level 1-1 has invisible tree leaves on WINED3D, works fine on DXVK
 - #57998  ClickOnce apps don't start after installing winetricks dotnet472
 - #58057  Certificate import wizard does not give visual confirmation when a specific certificate store is selected
 - #58061  [FL Studio] When holding CTRL to zoom in the playlist, it also scrolls vertically - possible regression
 - #58066  Virtual desktop doesn't resize correctly (missing window border) when VD size = desktop size

### Changes since 10.5:
```
Akihiro Sagawa (1):
      server: Fix the accumulation method when merging WM_MOUSEWHEEL message.

Alex Henrie (1):
      cryptui: Copy localized name of selected store to textbox.

Alexander Morozov (1):
      ntoskrnl.exe/tests: Fix a test failure on 32-bit Windows 7.

Alexandre Julliard (37):
      winegcc: Set default section alignment to 64k on ARM64.
      winegcc: Add a boolean flag for the -marm64x option.
      winebuild: Align sections to 64k on ARM64.
      ntdll: Move a bit more work into the open_builtin_so_file() helper function.
      ntdll: Add a helper function to open the main image as .so file.
      ntdll: Move some code around to group together all functions related to .so dlls.
      ntdll: Don't build support for .so dlls on platforms that don't have them.
      win32u: Implement NtGdiMakeFontDir().
      gdi32/tests: Add a test for NtGdiMakeFontDir().
      gdi32: Use NtGdiMakeFontDir() to implement CreateScalableFontResourceW().
      win32u: Remove the __wine_get_file_outline_text_metric() syscall.
      win32u: Implement NtUserGetProcessDefaultLayout().
      win32u: Implement NtUserBeginDeferWindowPos().
      win32u: Implement NtUserSetForegroundWindow().
      win32u: Implement NtUserKillSystemTimer().
      cng.sys: Use the native subsystem.
      hidclass.sys: Use the native subsystem.
      hidparse.sys: Use the native subsystem.
      wmilib.sys: Use the native subsystem.
      mouhid.sys: Remove unneeded spec file.
      makefiles: Require .sys modules to use the native subsystem.
      mmdevapi: Move the device GUID cache to the common code.
      mmdevapi: Move the registry device name lookup to the common code.
      mmdevapi: Move assigning a device GUID to the common code.
      mmdevapi: Forward driver entry points to the loaded driver.
      winmm: Always load mmdevapi as audio driver.
      mmdevapi: Get rid of the Wine info device.
      mmdevapi: Move the auxMessage() implementation to the common code.
      mmdevapi: Unload the driver module on process detach.
      mmdevapi: Allow audio drivers to defer MIDI support to a different driver.
      mmdevapi: Move the DriverProc implementation to the common code.
      mmdevapi: Move the mid/modMessage implementations to the common code.
      mmdevapi: Merge mmdevdrv.h into mmdevapi_private.h.
      makefiles: Make spec files optional for driver modules.
      ntdll: Add a stub for NtCreateSectionEx().
      kernelbase: Implement CreateFileMapping2().
      cmd/tests: Fix the NUL device name.

Bernhard Übelacker (7):
      ws2_32/tests: Add broken to test_WSAAddressToString.
      propsys/tests: Add broken to test_PropVariantChangeType_R8.
      psapi/tests: Add broken to test_GetModuleFileNameEx.
      winhttp/tests: Add broken to test_redirect.
      setupapi/tests: Add broken to test_SetupDiOpenDeviceInterface.
      advapi32/tests: Avoid crash in test_LsaLookupNames2 by setting len to zero.
      ntdll/tests: Make single step test succeed for 32-bit systems.

Brendan McGrath (1):
      winegstreamer: Push flush event when flushing.

Byeong-Sik Jeon (9):
      win32u: Preserve result string from multiple WINE_IME_POST_UPDATE calls during ImeProcessKey.
      win32u: Support WM_IME_KEYDOWN message during ImeProcessKey.
      imm32: Fix the WM_IME_COMPOSITION messages to be between the WM_IME_{START|END}COMPOSITION message.
      winewayland: Use an empty string to clear the composition string.
      win32u: Add more CompAttr, CompClause implementation using cursor_begin, cursor_end concept.
      winewayland: Extend cursor_pos using cursor_begin, cursor_end.
      winemac: Extend cursor_pos using cursor_begin, cursor_end.
      winex11: Extend cursor_pos using cursor_begin, cursor_end.
      winex11: Update only when caret pos changed in xic_preedit_caret.

Charlotte Pabst (2):
      mfplat/tests: Add test for MF_XVP_PLAYBACK_MODE.
      winegstreamer: Allow caller to allocate samples in MF_XVP_PLAYBACK_MODE.

Conor McCarthy (1):
      server: Do not call setpriority() if it cannot be used safely.

Daniel Lehman (3):
      oleaut32/tests: Test for bpp.
      oleaut32/tests: Add some test bmps.
      oleaut32: Handle more pixel formats in OleLoadPicture.

Daniel Martin (1):
      activeds: Implement ADsBuildVarArrayInt.

Dmitry Timoshkov (8):
      sane.ds: Fix DC leak.
      sane.ds: Change return type of sane_categorize_value() to void.
      sane.ds: Clarify how SANE mode names map to ICAP_PIXELTYPE values.
      sane.ds: Use sizeof() instead of hard-coded values, avoid zero initializing local variables when not necessary.
      bcrypt/tests: Add a test for exporting/importing AES wrapped blob with different key sizes.
      bcrypt: Add support for exporting AES wrapped blob for a 256-bit key.
      bcrypt: Add support for importing AES wrapped blob for a 256-bit key.
      cryptext: Implement CryptExtOpenCER.

Elizabeth Figura (12):
      user32/tests: Test messages when creating a visible modeless dialog.
      wined3d: Partially move fog mode to wined3d_extra_ps_args.
      wined3d: Move alpha test func to wined3d_extra_ps_args.
      wined3d: Partially move texture index and transform flags to wined3d_extra_ps_args.
      wined3d: Make ffp_vertex_update_clip_plane_constants() static.
      wined3d: Feed clip planes through a push constant buffer.
      wined3d: Add support for a layered DPB.
      ddraw/tests: Test a stretched blit to self with overlap.
      wined3d: Fix a bit of logic around identical fog start/end.
      ws2_32/tests: Add tests for socket handle validity checks in send functions.
      ntdll: Validate fd type in IOCTL_AFD_WINE_COMPLETE_ASYNC.
      ws2_32: Allow using duplicated socket handles in WS2_sendto().

Eric Pouech (31):
      cmd/tests: Add more lexer related tests.
      cmd: Rely on node_builder to get lexer state.
      cmd: Check command buffer instead of keeping whitespace state.
      cmd: Factorize end-of-line conditions.
      cmd: Handle directly commands til eol.
      dbghelp: Fix line number when multiple entries have same offset.
      dbghelp: Always reset all the fields for local scope enumeration.
      dbghelp: Don't report local symbols when they are not present.
      dbghelp: Use new pdb reader for DEFRANGE based local variables.
      dbghelp: Introduce helper to query info from index.
      cmd: Rewrite string handling in lexer.
      cmd: Remove unneeded variable 'thisChar' in lexer.
      cmd: Removed acceptCommand variable.
      cmd: Remove lastWasRedirect variable in lexer.
      cmd: Fix infinite loop in FOR /L.
      dbghelp: Add SYMFLAG_NULL for out of scope local variables.
      dbghelp: Rename ptr <> index conversion helpers.
      dbghelp: Introduce an opaque type to store type of data & function.
      dbghelp: Use opaque symref_t inside typedef symbol.
      dbghelp: Introduce helpers to discrimate symref_t owner.
      dbghelp: Now returning PDB basic types as a symref_t.
      dbghelp: Advertize old PDB reader types into new reader.
      dbghelp: Add method for search type by name.
      dbghelp: Add enum_types debug-info method.
      dbghelp: Move pointer type handle to PDB backend.
      dbghelp: Move array type handling to PDB backend.
      dbghelp: Move function signature type handling to PDB backend.
      dbghelp: Move enumeration type to PDB backend.
      dbghelp: Move UDT type handling to PDB backend.
      dbghelp: No longer preload the types from PDB.
      dbghelp: Optimize request to codeview types.

Esme Povirk (13):
      comctl32: Implement MSAA events for header controls.
      gdiplus: Implement path to region conversion without gdi32.
      gdiplus/tests: Test rounding of region rectangles.
      advapi32: Return success from TreeSetNamedSecurityInfoW.
      comctl32: Implement MSAA events for listbox.
      comctl32/tests: Test MSAA events for listbox.
      comctl32/tests: Add more MSAA event tests for listbox.
      gdiplus: Limit path rasterization to region bounding box.
      comctl32: Implement MSAA events for listview controls.
      comctl32/tests: Test listview MSAA events.
      comctl32/tests: Add test for MSAA event on listview setview.
      gdiplus: Rename a misleading variable.
      gdiplus: Simplify rect region conversion to HRGN.

Etaash Mathamsetty (2):
      winewayland.drv: Implement support for xdg-toplevel-icon.
      winewayland: Implement relative motion accumulator.

Fabian Maurer (2):
      gdiplus/tests: Add test for loading .ico.
      gdiplus: Use correct format guid for .ico files.

Gabriel Ivăncescu (5):
      mshtml: Don't expose "create" from Image constructor in IE9+ modes.
      mshtml: Don't expose "create" from Option constructor in IE9+ modes.
      mshtml: Use own window property for Image constructor.
      mshtml: Use own window property for Option constructor.
      mshtml: Use get_constructor in window's get_XMLHttpRequest.

Georg Lehmann (1):
      winevulkan: Update to VK spec version 1.4.312.

Gerald Pfeifer (2):
      ntdll: Fix build on platforms without getauxval.
      winemaker: Account for FreeBSD.

Giovanni Mascellani (1):
      dxgi/tests: Do not request a frame latency waitable on D3D10.

Hans Leidekker (1):
      rsaenh/tests: Get rid of workarounds for old Windows versions.

Jacek Caban (1):
      mshtml: Rename prototype_id to object_id.

Joachim Priesner (1):
      msvcrt: Concurrency: Fix signed/unsigned comparison.

John Szakmeister (1):
      ntdll: Correctly detect the NUL device under macOS.

Lorenzo Ferrillo (7):
      kernelbase: Factor out common functionality for performance counter functions.
      kernelbase: Add implementation of PerfSetULongCounterValue.
      kernelbase: Add implementation for PerfSetULongLongCounterValue.
      advapi32/tests: Create tests for PerfSetULongCounterValue.
      advapi32/tests: Add test For PerfSetULongLongCounterValue.
      kernelbase: Check for PERF_ATTRIB_BY_REFERENCE attribute in PerfSetCounterRefValue.
      kernelbase: Check for PERF_SIZE_LARGE in PerfSetULongLongCounterValue and PerfSetULongCounterValue.

Marcus Meissner (1):
      configure: Avoid problems with -Werror=return-type in check.

Matteo Bruni (2):
      d3dx9/tests: Clean up D3DXSaveTextureToFileInMemory tests.
      d3dxof/tests: Get rid of test_dump().

Nikolay Sivov (33):
      windowscodecs/tests: Add some tests for initial metadata readers content.
      windowscodecs/tests: Add more tests for the item id handling.
      windowscodecs/tests: Add some tests for bKGD chunk.
      windowscodecs/tests: Add line context to the metadata comparison helper.
      windowscodecs/tests: Remove redundant string length check for VT_LPSTR metadata value.
      windowscodecs/tests: Use wide-char literals in metadata tests.
      msvcirt/tests: Fix buffer overrun with a terminating null (ASan).
      uiautomationcore/tests: Fix use-after-free (ASan).
      windowscodecs/tests: Add another test case for 4bps tiff.
      windowscodecs/tiff: Fix stride value for 4bps RGBA.
      propsys/tests: Add more tests for PropVariantToDouble().
      propsys/tests: Add some tests for PropVariantChangeType(VT_R8).
      propsys/tests: Use correct members to initialize PropVariantToDouble() test input.
      propsys: Fix PropVariantToDouble() for float input.
      propsys: Implement PropVariantChangeType(VT_R8).
      windowscodecs/tests: Add a PNG encoder test with 64bppRGBA format.
      windowscodecs/tests: Add some tests for metadata handlers component info.
      windowscodecs/tests: Add some more tests for creating metadata readers.
      windowscodecs/tests: Add some tests for GetMetadataHandlerInfo().
      windowscodecs/metadatahandler: Remove unused internal vtable entries.
      windowscodecs/metadata: Pass handler pointer to the loader implementation.
      windowscodecs/metadata: Make it possible to populate default items at creation time.
      windowscodecs/metadata: Create default item for the gAMA reader.
      windowscodecs/metadata: Create default items for the cHRM handler.
      windowscodecs/metadata: Create default item for the hIST handler.
      windowscodecs/metadata: Create default items for the tIME handler.
      windowscodecs/tests: Add more tests for initial reader contents.
      windowscodecs/metadata: Add default item for the GifComment handler.
      windowscodecs/metadata: Implement bKGD chunk reader.
      dwrite: Implement GetFontSet() for collections.
      windowscodecs/metadata: Add a stub for bKGD writer.
      windowscodecs/metadata: Add a stub for tIME writer.
      combase: Add a stub for SetRestrictedErrorInfo().

Paul Gofman (2):
      ntdll: Do not mark first stack guard page as committed.
      ntdll: Add some specifics for NtQueryInformationProcess( ProcessDebugObjectHandle ) parameters handling.

Piotr Caban (16):
      conhost: Allow raster fonts.
      conhost: Merge validate_font and validate_font_metric helpers.
      conhost: Imrove best matching font selection in set_first_font.
      conhost: Prioritize font charset when selecting initial font.
      advapi32: Make username and domain match case insensite.
      include: Add some PBKDF2 related definitions.
      bcrypt: Fix BcryptDeriveKeyPBKDF2 with NULL salt.
      bcrypt: Add PBKDF2 algorithm provider.
      bcrypt: Handle PBKDF2 in BCryptGetProperty.
      bcrypt: Handle PBKDF2 in BCryptGenerateSymmetricKey.
      bcrypt: Reorganize hash_handle_from_desc helper so it can be reused.
      bcrypt: Add BCryptKeyDerivation partial implementation (PBKDF2 algorithm).
      include: Add _[w]dupenv_s declaration.
      include: Add _aligned_msize() declaration.
      include: Add wmemmove_s declaration.
      msvcr100/tests: Link to msvcr100.

Rémi Bernon (36):
      winemac: Get DC pixel format from winemac-internal objects.
      winex11: Trace XReconfigureWMWindow requests mask.
      winex11: Send _NET_WM_STATE requests to X root window.
      winex11: Only set NET_WM_STATE_FULLSCREEN for the desktop.
      user32/tests: Add more SW_SHOWNA / SetFocus tests.
      win32u: Set window foreground when setting focus.
      kernel32/tests: Use the public PROCESS_BASIC_INFORMATION definition.
      ntdll/tests: Use the public PROCESS_BASIC_INFORMATION definition.
      winex11: Avoid sending WM_MOUSEACTIVATE on WM_TAKE_FOCUS.
      winemac: Avoid sending WM_MOUSEACTIVATE on WM_TAKE_FOCUS.
      explorer: Paint the desktop even without RDW_ERASE.
      winex11: Avoid setting RDW_ERASE on expose events.
      dbghelp/tests: Remove now succeeding todo_wine.
      psapi/tests: Remove now succeeding todo_wine.
      cfgmgr32/tests: Add Windows 7 broken result.
      activeds: Use VT_I4 instead of VT_UI4.
      inetmib1/tests: Avoid printing large number of failures on macOS.
      kernel32/tests: Avoid printing large number of failures on macOS.
      dbghelp/tests: Remove now succeeding todo_wine.
      win32u: Handle some pixel format initialization.
      opengl32/tests: Avoid leaking contexts.
      opengl32/tests: Add more WGL_ARB_pbuffer tests.
      opengl32/tests: Add more WGL_ARB_render_texture tests.
      win32u: Introduce a generic pbuffer implementation from winex11.
      winewayland: Use the generic pbuffer implementation.
      winemac: Use the generic pbuffer implementation.
      win32u: Add a nulldrv pbuffer stub implementation.
      win32u: Introduce opengl_driver_funcs for memory DCs.
      win32u: Pass pixel format to osmesa_create_context.
      win32u: Implement generic context functions.
      win32u: Check the DC internal pixel formats against the context format.
      wineandroid: Use the generic context functions.
      winemac: Use the generic context functions.
      winewayland: Use the generic context functions.
      winex11: Use the generic context functions.
      win32u: Add nulldrv context functions.

Santino Mazza (5):
      amstream: Implement IMemAllocator stub for ddraw stream.
      amstream/tests: Test for custom allocator in ddraw stream.
      amstream: Implement custom allocator for ddraw stream.
      amstream/tests: Test for dynamic formats in ddraw stream.
      amstream: Implement dynamic formats in ddraw stream.

Sebastian Lackner (1):
      user32: Call UpdateWindow() after showing a dialog.

Stefan Dösinger (1):
      odbc32: Call the driver's SQLGetInfoW after a->w conversion.

Sven Baars (8):
      win32u: Fix a string leak (Valgrind).
      ntdll/tests: Fix a leak on error path (Coverity).
      ntdll/tests: Fix a string leak (Valgrind).
      ntdll: Empty the atom table before destroying it (Valgrind).
      ntdll/tests: Fix some string leaks (Valgrind).
      ntdll/tests: Don't trace invalid pointers (Valgrind).
      ntdll/tests: Fix some uninitialized variable warnings (Valgrind).
      ntdll: Avoid evaluating a possibly uninitialized variable in RtlExpandEnvironmentStrings(). (Valgrind).

Tim Clem (1):
      winemac.drv: Only send key down events to the window's inputContext.

Tobias Gruetzmacher (4):
      wininet: Handle HTTP status code 308 (Permanent Redirect).
      winhttp: Handle HTTP status code 308 (Permanent Redirect).
      urlmon: Handle HTTP status code 308 (Permanent Redirect).
      rsaenh: Ignore reserved field in import_key.

Vibhav Pant (10):
      winebth.sys: Fix use-after-free in dispatch_auth (Coverity).
      winebth.sys: Broadcast PnP event after updating properties for remote devices.
      winebth.sys: Broadcast PnP event when remote devices are removed/lost.
      winebth.sys: Broadcast GUID_BLUETOOTH_RADIO_IN_RANGE events for newly discovered remote devices as well.
      ws2_32/tests: Allow socket() for Bluetooth RFCOMM sockets to fail with WSAEPROTONOSUPPORT.
      winebth.sys: Implement IOCTL_BTH_DISCONNECT_DEVICE.
      winebth.sys: Implement IOCTL_WINEBTH_RADIO_REMOVE_DEVICE.
      bluetoothapis: Add stub for BluetoothRemoveDevice.
      bluetoothapis: Implement BluetoothRemoveDevice.
      bluetoothapis/tests: Add tests for BluetoothRemoveDevice.

Yuri Hérouard (1):
      wined3d: Use temporary buffer when stretching a surface to itself with cpu blit.

Zhiyi Zhang (7):
      user32/tests: Properly test ShowWindow(SW_MAXIMIZE) regarding WS_CAPTION.
      win32u: Fix incorrect work area for maximized windows.
      user32/tests: Add more window placement maximized position tests.
      win32u: Check against the monitor work area in update_maximized_pos().
      win32u: Use a more fitting name for a helper function.
      win32u: Properly scale monitor work area when emulate_modeset is enabled.
      win32u: Support windows spanning multiple monitors in map_window_rects_virt_to_raw().

Ziqing Hui (6):
      shell32/tests: Add more tests to test_rename.
      shell32/tests: Test NULL and empty file name for SHFileOperation.
      shell32/tests: Avoid showing UI when testing.
      shell32: Rework add_file_entry, add more parameters.
      shell32: Don't parse wildcard for rename operation.
      shell32: Rework rename_files.
```
