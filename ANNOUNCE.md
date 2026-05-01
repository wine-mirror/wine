The Wine development release 11.8 is now available.

What's new in this release:
  - Mono engine updated to version 11.1.0.
  - More work on MSXML reimplementation without libxml2.
  - Improved keyboard layout support using XKBRegistry.
  - More VBScript compatibility improvements.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/11.x/wine-11.8.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-11.8/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 11.8 (total 22):

 - #1201   Microsoft Golf 99 (Direct3D game) crashes on startup (IDirect3D::EnumDevices() needs to return the RGB device second)
 - #32619  Tom Clancy's Rainbow Six: Lockdown can't save a game (directory not created)
 - #39922  Hoot Sound Hardware Emulator: crashes on startup
 - #42400  cscript.exe doesn't show errors
 - #46107  Altium Designer 18.x - 20.x crashes on startup
 - #46951  MSXML Multi line attribute value - #10 char lost
 - #49908  vbscript: ExecuteGlobal is not implemented
 - #50384  Visio 2013 fails to install with builtin msxml6
 - #53844  vbscript invoke_vbdisp not handling let property correctly for VT_DISPATCH arguments
 - #53889  vbscript does not support Get_Item call on IDispatch objects
 - #53962  vbscript does not Eval implemented
 - #54221  vbscript: missing support for GetRef
 - #55006  vbscript: single line if else without else body fails compilation
 - #57511  vbscript: For loop where loop var is not defined throws error without context
 - #58248  vbscript: Me(Idx) fails to compile
 - #59636  Petka crashes when starting a new game
 - #59644  Enigma Virtual Box packed applications fail with file access errors since commit 35916176078
 - #59645  FormatMessageW fails to format ERROR_NOT_A_REPARSE_POINT (4390) with error 317
 - #59678  PLSQL Developer: Won't start on Wine 11.7
 - #59684  ExamDiffPro now Crashes under 11.7
 - #59689  SEC_WINNT_AUTH_IDENTITY_EX support in AcquireCredentialsHandle
 - #59694  Assassin's Creed Shadows fails to start since Wine 11.4, after windows.media.speech changes

### Changes since 11.7:
```
Adam Markowski (4):
      po: Update Polish translation.
      po: Update Polish translation.
      po: Update Polish translation.
      po: Update Polish translation.

Alex Henrie (1):
      imagehlp: Add ReBaseImage64 stub.

Alexandre Julliard (28):
      ntdll: Merge signal_init_threading() and signal_init_process().
      ntdll: Export RtlGetCurrentPeb() on the Unix side.
      win32u: Use RtlGetCurrentPeb() on the Unix side.
      winemac.drv: Use RtlGetCurrentPeb() on the Unix side.
      winewayland.drv: Use RtlGetCurrentPeb() on the Unix side.
      winex11.drv: Use RtlGetCurrentPeb() on the Unix side.
      ntdll: Swap the FPU control word between PE and Unix on i386.
      ntdll: Store the current process id in a global variable.
      ntdll: Export PsGetCurrentProcessId() and PsGetCurrentThreadId() on the Unix side.
      win32u: Silence a compiler warning.
      ntdll: Add a helper to allocate the initial syscall frame.
      ntdll: Allocate the kernel stack and signal stack together.
      ntdll: Get the TEB from the Unix thread data.
      ntdll: Move the exception jmp_buf to the Unix thread data.
      ntdll: Move the thread start params to the Unix thread data.
      ntdll: Move the server file descriptors to the Unix thread data.
      ntdll: Move the allow write flag to the Unix thread data.
      ntdll: Store the thread id in the Unix thread data.
      ntdll: Move the TEB list entry to the Unix thread data.
      ntdll: Use a separate debug_info structure on the kernel side.
      ntdll: Use a consistent code pattern in all signal handler functions.
      ntdll: Pass the thread data structure to signal handling helpers.
      ntdll: Pass the thread data structure to inline helper functions.
      ntdll: Return the thread data structure from init_handler() on i386.
      ntdll: Return the thread data structure from init_handler() on x86-64.
      ntdll: Retrieve the CPU-specific data from the thread data structure on i386.
      ntdll: Retrieve the CPU-specific data from the thread data structure on x86-64.
      ntdll: Rename ntdll_thread_data to teb_data.

Alistair Leslie-Hughes (1):
      windows.ui.core.textinput: Add CoreTextServicesManager stubbed interface.

Antoine Leresche (2):
      kernelbase: Add message string for errors 302-316.
      kernelbase: Add message string for error 318-326.

Aurimas Fišeras (1):
      po: Update Lithuanian translation.

Bartosz Kosiorek (5):
      gdiplus: Add support to line alignment for GdipMeasureString.
      gdiplus: Add support for PixelFormat 16bpp RGB565 and RGB555 to GdipGetNearestColor.
      gdiplus: Initial implementation of GdipWarpPath.
      gdiplus/tests: Extend GdipInitializePalette tests with transparent color.
      gdiplus: Fix returned flags and add transparency to GdipInitializePalette.

Benoît Legat (1):
      secur32: Fix handling by SChannel for CNG/NCrypt client certificates for mTLS.

Bernhard Kölbl (2):
      windows.media.speech: Don't access the all voices vector view directly.
      windows.media.speech: Fix a trace typo.

Brendan McGrath (6):
      amstream/tests: Test enum media type with ddraw.
      amstream/tests: Test enum media type whilst connected.
      amstream/tests: Test enum media type on disconnect.
      amstream: Return current format in media type enum.
      amstream: Return connected media subtype in enum if connected.
      mf: Handle no time and/or duration during markin.

Connor McAdams (10):
      dinput: Only pass instance name string to device_instance_is_disabled().
      dinput: Create a cache of currently connected HID joysticks.
      dinput: Use joystick cache in hid_joystick_create_device().
      dinput: Use joystick cache in hid_joystick_enum_device().
      dinput: Return E_FAIL when opening a joystick device that is currently disconnected.
      dinput: Create a version 1 UUID for dinput joystick devices.
      dinput/tests: Add tests for dinput joystick IDs.
      dinput: Add support for DIPROP_JOYSTICKID.
      dinput/tests: Add tests for GUID_Joystick.
      dinput: Add support for GUID_Joystick.

Conor McCarthy (15):
      mf/tests: Test setting the input type on the topology loader test transform.
      mf/topology_loader: Compare the current type with upstream on direct connection only for output nodes.
      mf/tests: Check IsMediaTypeSupported() is always called on the topology loader test sink.
      mf/topology_loader: Always call IsMediaTypeSupported() on downstream when connecting a branch.
      gdiplus: Elide empty spans.
      gdiplus/tests: Test a region intersect which produces empty spans.
      mf/tests: Introduce a helper for creating an activated media sink.
      mf/tests: Check for sample grabber sink shutdown.
      mf/tests: Add tests for sample grabber sink shutdown.
      mf/session: Ensure the session object is shut down on release.
      mf/session: Shut down all topologies on session shutdown.
      mfmediaengine/tests: Set the volume after the null check in test_TransferVideoFrame().
      mfmediaengine/tests: Test a media engine extension.
      mfmediaengine: Set the source pending flag before creating the source.
      mfmediaengine: Support media engine extensions.

Cooper Morgan (3):
      winewayland: Fix use-after-release of window rects in configure handler.
      winewayland: Handle WS_MINIMIZE to suppress incorrect state transitions.
      winewayland: Restore minimized windows when compositor sends configure.

David Carrasco Flores (1):
      wusa: Recognize runtime.syswow64 expressions.

Dmitry Timoshkov (2):
      gdiplus: Copy logic for trimming bounds width from GdipMeasureString() to GdipMeasureCharacterRanges().
      gdiplus: Copy logic for calculating y-offset from GdipDrawString() to GdipMeasureCharacterRanges().

Ekaterine Papava (1):
      po: Update Georgian translation.

Esme Povirk (1):
      mscoree: Update Wine Mono to 11.1.0.

Francis De Brabandere (30):
      wscript: Implement WScript.Timeout property and //T:nn CLI flag.
      wscript: Terminate the script when WScript.Timeout elapses.
      vbscript: Return specific error codes for unterminated block statements.
      vbscript: Return error 1045 for non-literal constant expressions.
      vbscript: Convert VT_DISPATCH arguments to string in Eval/Execute/ExecuteGlobal.
      vbscript: Track source location in const, class, and function declarations.
      vbscript: Return "Name redefined" error for duplicate global declarations.
      vbscript: Return "Name redefined" error for duplicate class members.
      scrrun: Implement IFolder and IFile ShortName and ShortPath.
      vbscript: Return error 1010 for keyword used where identifier expected in Dim.
      vbscript: Return "Invalid for loop control variable" error for member expressions.
      vbscript: Implement DatePart built-in function.
      vbscript: Fix Sub first argument parentheses handling.
      vbscript: Convert string to number for comparison operators.
      vbscript: Return proper errors for orphan Loop and Next keywords.
      vbscript: Move call_depth check past exec_script's setup phase.
      vbscript: Resolve parser conflicts with %prec and %expect.
      vbscript: Implement GetLocale and SetLocale functions.
      vbscript: Use script LCID in Format* functions.
      vbscript: Pass script LCID to VariantChangeType in to_string helper.
      vbscript: Pass script LCID to VariantChangeType in to_system_time helper.
      vbscript: Implement LenB.
      vbscript: Implement LeftB, RightB, and MidB.
      vbscript: Implement InStrB.
      vbscript: Implement AscB and ChrB.
      vbscript: Support element access on public array properties of class instances.
      scrrun: Implement IFile::Delete.
      scrrun: Implement IFolder::Delete.
      scrrun: Implement IFile::Move.
      scrrun: Implement IFolder::Move.

Hans Leidekker (1):
      secur32: Handle SEC_WINNT_AUTH_IDENTITY_EX in the Negotiate provider.

Hendrik Borchardt (1):
      msvcrt: Only reject negative timestamps with EINVAL in ctime_s.

Henri Verbeet (1):
      winmm: Handle DRVM_MAPPER_CONSOLEVOICECOM_GET messages.

Ivan Ivlev (3):
      odbc32: Compare strings case-insensitively in get_driver_filename().
      odbc32: Skip operations for NULL filename in replicate_odbc_to_registry().
      odbc32: Check if row_number is NULL in param_options_unix().

Jacek Caban (9):
      mshtml/tests: Remove unused variables.
      mshtml/tests: Add missing CHECK_CALLED statements.
      mshtml/tests: Remove unused variables and fix CLEAR_CALLED statements.
      winspool.drv/tests: Remove unused variables.
      localspl/tests: Remove unused variable.
      urlmon/tests: Remove unused variables.
      urlmon/tests: Add missing CHECK_CALLED statements.
      itss/tests: Fix CHECK_CALLED statement.
      ntdll: Use ESR in the ARM64 signal handler when possible.

Joe Meyer (1):
      win32u: Stub NtUserInitializeTouchInjection.

Lubomir Rintel (1):
      cmd: Fix handling of "ECHO.ON".

Matteo Bruni (4):
      winex11: Factor out mapping keycodes to scancodes / keysyms into a separate function.
      winex11: Get Xkb group names.
      winex11: Look up keysyms for the currently selected Xkb group in X11DRV_KEYBOARD_DetectLayout().
      winex11: Decode current keyboard layout from the Xkb group name.

Michał Durak (3):
      nsiproxy.sys: Avoid null pointer dereference on if_nameindex() failure.
      nsiproxy.sys: Avoid malloc(0) in case no network interfaces are present.
      iphlpapi: Avoid out-of-bounds read if no network interfaces are present.

Nikolay Sivov (37):
      msxml3: Fix owner node refcount update when adding children and attributes.
      msxml3: Respect document encoding in Save().
      msxml3: Fix a leak when parsing xml declaration body (Coverity).
      msxml3: Improve cleanup on error paths when creating DOM nodes (Coverity).
      msxml3/tests: Fix copy-paste issue when testing node value (Coverity).
      msxml3/tests: Fix a string double free (Coverity).
      msxml3/tests: Fix some use-after-free (Coverity).
      msxml3/tests: Add a few missing return value checks (Coverity).
      odbc32: Add missing object unlock on return in SQLSetEnvAttr().
      msxml3/tests: Add some tests for loading documents with Shift_JIS encoding.
      msxml3: Handle Shift_JIS encoding in the parser.
      msxml3: Improve handling of incomplete multibyte sequences in parser input.
      msxml3/tests: Separate DTD validation tests.
      msxml3/tests: Add more tests for DTD node methods.
      msxml3: Implement previousSibling() property for DTD node.
      msxml3: Implement nextSibling() property for DTD node.
      msxml3: Do not rely on strings being BSTRs in createNode().
      msxml3/tests: Add more tests for baseName property.
      msxml3: Remove fixme's from implemented methods.
      msxml3/tests: Add more tests for creating entity references.
      msxml3: Improve name validation for entity reference nodes.
      msxml3/tests: Add some tests for creating document fragments.
      msxml3: Ignore name and uri when creating document fragments.
      msxml3/tests: Add some tests for max-element-depth property.
      msxml3/tests: Extend MaxElementDepth tests.
      msxml3/sax: Add support for max-element-depth property.
      msxml3: Add support for MaxElementDepth property in the DOM parser.
      msxml3: Improve attribute value normalization in DOM.
      msxml3: Store attribute types in parsed form.
      msxml3: Store parsed attribute default declaration type.
      msxml6/tests: Add some tests for XMLHTTP API.
      msxml3: Create structured representation for element declarations.
      msxml3/sax: Store reusable type strings separately.
      msxml3/sax: Keep reused strings together.
      msxml3/sax: Keep DTD data in a separate structure.
      msxml3/sax: Retain element declarations as DTD data.
      msxml3: Pass parsed DTD data to the DOM.

Paul Gofman (3):
      ntdll: Do not crash in RtlVirtualUnwind2() in some cases of NULL output parameters on x64.
      ntdll: Catch access violation in RtlVirtualUnwind2() on arm64.
      ntdll: Only take PEB lock to update PEB data in RtlSetCurrentDirectory_U().

Piotr Caban (19):
      odbc32: Support data type conversion on version mismatch.
      msado15: Wrap IAccessor interface in rowset wrapper.
      msado15: Use CoTaskMemFree in dbtype_free.
      msado15: Introduce helper to get bookmark in IRowset wrapper.
      msado15: Add partial implementation of IRowsetFind::FindNextRow in rowset wrapper.
      msado15: Handle BSTR data in IRowsetFind::FindNextRow.
      msado15: Don't leak HACCESSOR on error in _Recordset::Find.
      msado15: Handle empty BSTR bookmark in _Recordset::Find.
      msado15: Don't crash if IRowsetUpdate::Update doesn't allocate status in _Recordset::Update.
      msado15: Take cache size into account when calculating offset from current row in recordset_Find.
      msado15: Initialize recordset->current_row in recordset_Find.
      msado15: Validate arguments in _Recordset::put_CursorLocation.
      msado15: Print FIXME message when client-side cursor is requested.
      msado15: Rename create_mem_rowset to create_client_cursor.
      msado15: Print FIXME message when changing active connection when recordset is open.
      msvcp120: Ensure that segments are not being allocated when segments table is enlarged.
      msvcp120: Don't count segments being allocated when reporting vector capacity.
      Revert "msvcp120: Ensure that segments are not being allocated when segments table is enlarged.".
      concrt140: Sync details.c file with msvcp90.

Rémi Bernon (24):
      winex11: Fix some X11DRV_InitKeyboard loop indentation.
      opengl32: Fix incorrect cleanup when GLsync creation fails.
      opengl32: Avoid leaking client syncs on context destruction.
      opengl32: Move wgl_cs entry / exit out of handle functions.
      dinput: Check for disabled/overridden devices in hid_joystick_try_open.
      dinput: Handle device override directly in hid_joystick_device_try_open.
      dinput: Save and load HID joystick instance GUIDs to the registry.
      opengl32/tests: Remove unnecessary window creation check.
      opengl32/tests: Declare and load every GL/WGL functions.
      opengl32/tests: Test implicit object allocation support.
      opengl32/tests: Test wglShareLists sharing of various types.
      opengl32/tests: Test wglShareLists sharing of GLsync objects.
      opengl32/tests: Test object types shared namespaces.
      dinput: Test and implement support for GUID_Sys(Mouse|Keyboard)Em.
      opengl32: Generate a thunks.h file for extern declarations.
      opengl32: Implement wglMakeCurrent with wglMakeContextCurrentARB.
      opengl32: Implement wglCreateContext with wglCreateContextAttribsARB.
      opengl32: Move context thread id checks to the PE side.
      opengl32: Generate code to call the get_integer hooks.
      opengl32/tests: Test the default framebuffer behavior.
      win32u: Only fill the drawable buffer map for valid buffers.
      opengl32: Validate glDrawBuffers enum before setting the state.
      opengl32: Initialize the context draw / read buffers on creation.
      opengl32: Always use the partial context state tracker.

Sameer Singh (समीर सिंह) (1):
      gdi32/uniscribe: Correct the script tag for Tamil.

Stefan Dösinger (1):
      hhctrl.ocx: Make Contents/Search/Index children of the tab control.

Sven Baars (1):
      ntdll: Make sure User Syscall Dispatch defines are always defined on Linux.

Tim Clem (1):
      cfgmgr32: Catch invalid handles in CM_Unregister_Notification.

Twaik Yont (6):
      wineandroid: Switch java_* variables to direct linking.
      wineandroid: Load AHardwareBuffer symbols from libandroid.
      wineandroid: Simplify client pid handling.
      wineandroid: Use Android logging for ioctl paths.
      wineandroid: Move native window registration to surface_changed.
      wineandroid: Pass JNIEnv explicitly through ioctl handlers.

Yuxuan Shui (2):
      urlmon/tests: Test output length calculation when output buffer is too small.
      iertutil: Fix out-of-bound access in remove_dot_segments.

Zhiyi Zhang (6):
      Revert "win32u: Don't forcefully activate windows before restoring them.".
      iertutil: Fix wrong host type for Punycode-encoded hostnames.
      gdi32: Test that GetDIBits() rejects DDBs that are not 1-bit or 32-bit.
      win32u: Reject DDBs that are not 1-bit or 32-bit for GetDIBits().
      win32u: Optimize the speed of halftone primitives by using 32.32 fixed point arithmetic.
      win32u: Set the normal window position without going through NtUserSetInternalWindowPos().
```
