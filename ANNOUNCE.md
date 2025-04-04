The Wine development release 10.5 is now available.

What's new in this release:
  - Support for larger page sizes on ARM64.
  - Mono engine updated to version 10.0.0.
  - Pairing support in the Bluetooth driver.
  - Vulkan H.264 decoding.
  - %GS register swapping on macOS.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/10.x/wine-10.5.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-10.5/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 10.5 (total 22):

 - #2155   Failure of SetFocus, SetActiveWindow and SetForegroundWindow
 - #52715  wine segfaults on asahi linux due to 16k pages (apple M1 hardware, linux kernel/userland)
 - #56377  Microsoft Edge freezes almost immediately after launching
 - #56479  Shrift 2 Translation Patch 1.3Gamev2.07: could not load file or assembly. access denied.
 - #56837  Bad file pointer after a file write in appending mode
 - #57010  Pantheon - error during installation
 - #57406  Brothers - A Tale of Two Sons launcher fails to launch
 - #57784  Video modes are broken
 - #57919  Rally Trophy: Keyboard key is not configurable anymore, regression
 - #57920  Rally Trophy: Input device configuration doesn't show mapped keys anymore, regression
 - #57940  ntoskrnl.exe tests can trigger BSOD
 - #57948  Listview / LVM_GETORIGIN returns wrong coordinates
 - #57973  Listbox selected items are rendered even with LBS_NOSEL
 - #57987  The Queen of Heart 99 SE : Config.exe crashes just before saving and quitting
 - #57996  PS Core installer crashes (regression)
 - #58006  winegcc seg-faults in wine 10.4
 - #58008  wine-10.4 hangs on macOS (Rosetta 2)
 - #58022  mscoree:comtest and mscoree:mscoree fail
 - #58024  Listview does not handle WM_VSCROLL(SB_BOTTOM)
 - #58038  Keyboard input lost after alt-tabbing in virtual desktop
 - #58050  Game Controllers panel crashes on exit
 - #58052  TipTap launcher for OverField crashes: Call to unimplemented function USER32.dll.IsTopLevelWindow

### Changes since 10.4:
```
Alex Henrie (4):
      ntdll: Correct NtAllocateReserveObject arguments in specfile.
      ntdll: Implement querying StorageDeviceProperty for optical discs.
      cryptui: Use wide character string literal for L"".
      shell32/tests: Use wide character string literals.

Alexandre Julliard (48):
      winegcc: Store the output name in a global variable.
      winegcc: Use -nostdlib for MSVC builds.
      winegcc: Always add strip flag to the linker args explicitly.
      winegcc: Validate the target option after all arguments have been handled.
      winegcc: Always pass the -x option through the files list.
      winegcc: Store the linking options in global variables.
      winegcc: Store the files arguments in a global variable.
      ntdll: Limit the header mapping size to the size of the file.
      ntdll: Don't use private writable mappings on macOS.
      ntdll: Retrieve the host page size on the Unix side on ARM64.
      ntdll: Align virtual memory allocations to the host page size.
      ntdll: Align virtual memory deallocations to the host page size.
      ntdll: Apply virtual page protections to the entire host page.
      ntdll: Align file mappings to the host page size.
      ntdll: Align stack guard pages to the host page size.
      ntdll: Align to the host page size when calling mlock/munlock/msync.
      server: Support the host page size being different from the Windows page size.
      widl: Preserve pragma pack() statements in header files.
      widl: Use pragma pack in generated files.
      include: Use pragma pack in idl files.
      ntdll: Round the size of the .so NT header to the file alignment.
      winegcc: Don't forward -v to the compiler by default.
      winegcc: Only pass explicit linker arguments with -print-libgcc-file-name.
      winegcc: Set the entry point option in get_link_args().
      winegcc: Add a helper function to read options from a file.
      include: Move InitializeObjectAttributes definition to ntdef.h.
      ntdll: Use the InitializeObjectAttributes macro in more places.
      ntdll/tests: Use the InitializeObjectAttributes macro in more places.
      kernelbase: Use the InitializeObjectAttributes macro in more places.
      wine.inf: Add list of known dlls.
      wineboot: Fetch the list of supported machines once at startup.
      wineboot: Create section objects for known dlls.
      advapi32: Delay import cryptsp.
      comctl32: Delay import oleacc.
      comdlg32: Delay import winspool
      gdiplus: Delay import mlang.
      imagehlp: Delay import dbghelp.
      wldap32: Delay import some libraries.
      ntdll: Only enable redirection around the calls that access the file system.
      ntdll: Add a helper to build an NT name from the system directory.
      ntdll: Try to open dll mappings from the KnownDlls directory first.
      ntdll: Only check HAVE_PTHREAD_TEB when setting %fs on Linux.
      wineps.drv: Delay import winspool.
      kernelbase: Redirect system32 paths manually for delayed file moves.
      setupapi: Don't print an error when failing to replace a native dll.
      setupapi: Delay fake dll installation until reboot for files in use.
      makefiles: Handle the makefile disable flags directly in configure.
      makefiles: Skip building some programs that are only useful for the host architecture.

Alfred Agrell (1):
      server: Fall back to epoll_wait if epoll_pwait2 doesn't work.

Anders Kjersem (1):
      setupapi: Don't add backslash on empty folder.

André Zwing (2):
      bluetoothapis/tests: Don't test function directly when reporting GetLastError().
      kernel32/tests: Don't test function directly when reporting GetLastError().

Attila Fidan (1):
      winewayland: Don't use a destroyed surface in text input.

Bartosz Kosiorek (4):
      gdiplus/test: Add GdipReversePath test for Path Pie.
      gdiplus: Fix GdipReversePath for mixed Bezier and Line points in subpath.
      gdiplus/tests: Add tests for GdipGetPathWorldBounds with single point.
      gdiplus: Add single point support for GdipGetPathWorldBounds.

Bernhard Übelacker (1):
      joy.cpl: Set devnotify to NULL after it got unregistered (ASan).

Brendan McGrath (5):
      mfplat/tests: Fix crash in MFShutdown on Windows.
      mfplat/tests: Fix leak of media events.
      mfplat/tests: Fix leak of media source.
      winegstreamer: Allow NULL for time_format.
      mfsrcsnk: Allow NULL for time_format.

Brendan Shanks (6):
      ntdll: Remove ugly fallback method for getting a thread's GSBASE on macOS.
      ntdll: Ensure init_handler runs in signal handlers before any compiler-generated memset calls.
      ntdll: Don't access the TEB through %gs when using the kernel stack in x86_64 syscall dispatcher.
      ntdll: Set %rsp to be inside syscall_frame before accessing %gs in x86_64 syscall dispatcher.
      ntdll: On macOS x86_64, swap GSBASE between the TEB and macOS TSD when entering/leaving PE code.
      ntdll: Remove x86_64 Mac-specific TEB access workarounds that are no longer needed.

Connor McAdams (5):
      d3dx9/tests: Add a test for reading back a texture saved as D3DXIFF_JPG.
      d3dx9/tests: Add more tests for saving textures to DDS files.
      d3dx9: Add support for saving IDirect3DTexture9 textures to DDS files in D3DXSaveTextureToFileInMemory().
      d3dx9: Add support for saving IDirect3DCubeTexture9 textures to DDS files in D3DXSaveTextureToFileInMemory().
      d3dx9: Add support for saving IDirect3DVolumeTexture9 textures to DDS files in D3DXSaveTextureToFileInMemory().

Dmitry Timoshkov (1):
      bcrypt/tests: Add some tests for export/import of BCRYPT_AES_WRAP_KEY_BLOB.

Elizabeth Figura (18):
      wined3d: Feed fog constants through a push constant buffer.
      wined3d: Introduce a separate wined3d_extra_ps_args state for point sprite enable.
      wined3d/glsl: Move legacy shade mode to shader_glsl_apply_draw_state().
      wined3d: Partially move shade mode to wined3d_extra_ps_args.
      kernel32/tests: Add more tests for orphaned console handles.
      server: Fail to create an unbound input/output when there is no console.
      server: Use a list of screen buffers per console.
      server: Allow waiting on an orphaned screen buffer.
      server: Track unbound input signaled state based on its original console.
      server: Track unbound output signaled state based on its original console.
      d3d11: Plumb SubmitDecoderBuffers() through wined3d.
      wined3d: Create and submit video decode command buffers.
      wined3d: Call vkQueueSubmit() if there are semaphores.
      wined3d: Add a "next" pointer to wined3d_context_vk_create_image().
      wined3d: Implement Vulkan H.264 decoding.
      d3d11/tests: Add tests for H.264 decoding.
      wined3d: Invert gl_FragCoord w.
      wined3d: Use VKD3D_SHADER_COMPILE_OPTION_TYPED_UAV_READ_FORMAT_UNKNOWN if possible.

Eric Pouech (11):
      dbghelp: Fix order of non-commutative binary op:s in PDB/FPO unwinder.
      dbghelp: Introduce new PDB reader, use it for unwinding FPO frames.
      dbghelp: Don't keep PDB files mapped after parsing is done.
      dbghelp: Introduce a vtable per module_format.
      dbghelp: Rename struct internal_line into lineinfo.
      dbghelp: Always copy the source file string.
      dbghelp: Introduce interface for line info access.
      dbghelp: Introduce method to get next/prev line information.
      dbghelp: Introduce method to enumerate line numbers.
      dbghelp: Add method to enumerate source files.
      dbghelp: No longer store line information from old PDB reader.

Eric Tian (1):
      gdiplus: Avoid storing NULL in gdip_font_link_section.

Esme Povirk (9):
      comctl32: Implement EVENT_OBJECT_VALUECHANGE for edit controls.
      comctl32/tests: Test MSAA events for edit controls.
      mscoree: Update Wine Mono to 10.0.0.
      win32u: Implement EVENT_OBJECT_NAMECHANGE.
      comctl32/tests: Add test for SysLink accDoDefaultAction.
      comctl32: Implement accDoDefaultAction for SysLink controls.
      comctl32/tests: Test accLocation values on SysLink control.
      comctl32/tests: Test SetWindowText and LM_GETITEM for SysLink.
      comctl32/tests: Test MSAA events for SysLink.

Francisco Casas (1):
      d2d1: Add [loop] attribute in sample_gradient() shader function.

Gabriel Ivăncescu (8):
      mshtml: Keep the link from the inner window to the outer window even when detached.
      jscript: Obtain the jsdisp for host objects in other contexts.
      mshtml: Make sure manually created document dispex info is initialized in IE9+ modes.
      mshtml: Release the node if there's no script global when handling events.
      jscript: Get rid of the funcprot argument.
      mshtml: Don't mess with the outer window if we're already detached.
      jscript: Don't leak when popping (u)int values off the stack.
      jscript: Don't leak when return value of host constructor is not used.

Hans Leidekker (7):
      adsldp/tests: Fix test failures.
      wbemprox: Get rid of unused imports.
      wbemprox: Get rid of the per-table lock.
      bcrypt: Make secret parameter to create_symmetric_key()/generate_symmetric_key() const.
      bcrypt: Only print a fixme if a vector has been set.
      bcrypt: Add support for BCRYPT_AES_WRAP_KEY_BLOB in BCryptExportKey().
      bcrypt: Add support for BCRYPT_AES_WRAP_KEY_BLOB in BCryptImportKey().

Kevin Martinez (2):
      shell32: Added stub for IEnumObjects interface.
      shell32: Added stub for IObjectCollection interface.

Kostin Mikhail (1):
      regsvr32: Ignore obsolete flag /c.

Louis Lenders (3):
      win32u: Add stub for NtUserSetAdditionalForegroundBoostProcesses.
      powrprof: Add stub for PowerRegisterForEffectivePowerModeNotifications.
      dwmapi: Do not prefer native dll.

Marc-Aurel Zent (5):
      server: Implement ThreadPriority class in NtSetInformationThread.
      server: Move last thread information to get_thread_info flags.
      server: Return the correct base/actual thread priorities.
      kernelbase: Use ProcessPriorityClass info class in GetPriorityClass.
      server: Return the correct base/actual process priority.

Nikolay Sivov (13):
      user32/tests: Add some tests for current selection with LBS_NOSEL.
      comctl32/tests: Add some tests for current selection with LBS_NOSEL.
      user32/listbox: Do not paint item selection with LBS_NOSEL.
      comctl32/listbox: Do not paint item selection with LBS_NOSEL.
      libtiff: Provide zlib allocation functions.
      windowscodecs/metadata: Always initialize handler pointer.
      windowscodecs/jpeg: Add support for App1 metadata blocks in the decoder.
      windowscodecs/tests: Add some tests for the App0 reader.
      comctl32/listview: Handle WM_VSCROLL(SB_BOTTOM).
      windowscodecs/converter: Implement BW -> 24BGR conversion.
      windowscodecs/converter: Implement 48bppRGB -> 128bppRGBFloat conversion.
      wmphoto: Use CRT allocation functions.
      d3d10/effect: Fix constant buffer overrun when updating expression constants (ASan).

Paul Gofman (11):
      wintrust: Don't set desktop window when initializing provider data.
      kernel32/tests: Add tests for known dlls load specifics.
      ntdll/tests: Fix a test failure on Windows wow64.
      msvcp140: Implement codecvt_char16 ctors and dtor.
      msvcp140: Implement codecvt_char16_do_out().
      msvcp140: Implement codecvt_char16_do_in().
      msvcp: Fix output size check in codecvt_wchar_do_out().
      setupapi/tests: Add a test for SetupDiOpenDeviceInterface().
      cfgmgr32: Implement CM_Get_Device_Interface_List[_Size][_Ex]W().
      cfgmgr32: Implement CM_Get_Device_Interface_List[_Size][_Ex]A().
      cfgmgr32: Implement CM_Get_Device_Interface_PropertyW() for DEVPKEY_Device_InstanceId.

Piotr Caban (1):
      msvcrt: Update file position in _flsbuf() in append mode.

Robert Lippmann (1):
      server: Use INOTIFY_CFLAGS.

Rémi Bernon (54):
      winex11: Use the current state when deciding how to reply to WM_TAKE_FOCUS.
      winex11: Use the state tracker for the desktop window _NET_WM_STATE.
      opengl32: Remove unused type parameter.
      opengl32: Remove unnecessary interlocked exchange.
      opengl32: Use an explicit GLsync pointer in the union.
      opengl32: Introduce an opengl_context_from_handle helper.
      opengl32: Introduce a wgl_pbuffer_from_handle helper.
      winex11: Introduce a new handle_state_change helper.
      winex11: Track _NET_ACTIVE_WINDOW property changes.
      dinput/tests: Load cfgmgr32 dynamically.
      dinput/tests: Add more EnumDevicesBySemantics tests.
      dinput: Fix EnumDeviceBySemantics user checks.
      dinput: Fix keyboard / mouse semantics matching.
      dinput: Only load mappings that have not yet been set.
      dinput: Avoid overriding app-configured action map controls.
      winex11: Get rid of now unnecessary unmap_window helper.
      winex11: Keep _NET_WM_USER_TIME on individual toplevel windows.
      winex11: Avoid stealing focus during foreground window mapping.
      win32u: Introduce a new opengl_driver_funcs structure.
      win32u: Implement opt-in WGL_(ARB|EXT)_extensions_string support.
      wineandroid: Use win32u for WGL_(ARB|EXT)_extensions_string support.
      winemac: Use win32u for WGL_(ARB|EXT)_extensions_string support.
      winewayland: Use win32u for WGL_(ARB|EXT)_extensions_string support.
      winex11: Use win32u for WGL_(ARB|EXT)_extensions_string support.
      win32u: Add a nulldrv init_wgl_extensions implementation.
      cfgmgr32/tests: Load some functions dynamically.
      winemac: Use the default wglGetPixelFormatAttribivARB implementation.
      winemac: Remove the driver wglChoosePixelFormatARB implementation.
      win32u: Implement opt-in generic wgl(Get|Set)PixelFormat(WINE).
      wineandroid: Use win32u generic wgl(Get|Set)PixelFormat(WINE).
      winewayland: Use win32u generic wgl(Get|Set)PixelFormat(WINE).
      winex11: Use win32u generic wgl(Get|Set)PixelFormat(WINE).
      winemac: Use win32u generic wgl(Get|Set)PixelFormat(WINE).
      win32u: Add a null driver set_pixel_format implementation.
      winex11: Only set focus_time_prop for managed windows.
      winex11: Keep track of whether mapped window needs activation.
      winex11: Refactor X11DRV_GetWindowStateUpdates control flow.
      win32u: Return foreground window changes from GetWindowStateUpdates.
      win32u: Call ActivateWindow callback when foreground window changes.
      winex11: Use _NET_ACTIVE_WINDOW to request/track window activation.
      win32u: Move WGL_ARB_pixel_format extension registration.
      winex11: Ignore GL drawable creation failure on reparent.
      win32u: Remove unnecessary win32u_(get|set)_window_pixel_format exports.
      setupapi: Implement SetupDiOpenDeviceInterface(A|W).
      winex11: Cleanup code style in X11DRV_wglCreatePbufferARB.
      winex11: Cleanup code style in X11DRV_wglQueryPbufferARB.
      winex11: Cleanup code style in X11DRV_wglBindTexImageARB.
      winex11: Compute texture binding enum from the texture target.
      winex11: Remove some pbuffer related dead code.
      opengl32/tests: Cleanup memory DC rendering tests.
      opengl32/tests: Update memory DC pixel format tests.
      opengl32/tests: Update memory DC drawing tests.
      gdi32/tests: Test selecting bitmap on a D3DKMT memory DC.
      opengl32/tests: Test GL rendering on D3DKMT memory DCs.

Sven Baars (9):
      dmime/tests: Use wine_dbgstr_longlong.
      win32u: Store window styles in a DWORD (Coverity).
      win32u: Always set DriverVersion.
      makedep: Fix a compilation warning.
      d3dxof: Fix a memory leak on error path (Coverity).
      d3dxof/tests: Fix a memory leak on error path (Coverity).
      adsldp: Fix a leak on error path in search_ExecuteSearch() (Valgrind).
      adsldp: Free ber in search_GetNextRow() (Valgrind).
      adsldp/tests: Fix a name leak (Valgrind).

Tim Clem (1):
      imm32: Add a stub for CtfImmRestoreToolbarWnd.

Tomasz Pakuła (4):
      dinput/tests: Add tests for 6-axis force feedback joystick.
      include: Define the max number of supported HID PID axes.
      winebus: Support creation of dynamic number of PID axes.
      winebus: Get the number of haptic axes from SDL.

Vibhav Pant (16):
      ws2_32/tests: Add tests for creating Bluetooth RFCOMM sockets.
      server: Add support for creating Bluetooth RFCOMM sockets.
      ws2_32: Add bind tests for Bluetooth RFCOMM sockets.
      ws2_32: Support bind for Bluetooth RFCOMM addresses.
      winebth.sys: Register a pairing agent with BlueZ during startup.
      winebth.sys: Broadcast a WINEBTH_AUTHENTICATION_REQUEST PnP event on receiving a RequestConfirmation request from BlueZ.
      winebth.sys: Implement IOCTL_WINEBTH_AUTH_REGISTER.
      bluetoothapis: Implement BluetoothRegisterForAuthenticationEx and BluetoothUnregisterForAuthentication.
      bluetoothapis/tests: Add tests for BluetoothRegisterForAuthenticationEx and BluetoothUnregisterAuthentication.
      winebth.sys: Support cancellation of pairing sessions via BlueZ.
      winebth.sys: Implement IOCTL_WINEBTH_RADIO_SEND_AUTH_RESPONSE.
      bluetoothapis: Add stub for BluetoothSendAuthenticationResponseEx.
      bluetoothapis: Implement BluetoothSendAuthenticationResponseEx.
      bluetoothapis/tests: Add tests for BluetoothSendAuthenticationResponseEx.
      winebth.sys: Allow service auth requests from BlueZ for authenticated devices.
      ws2_32/tests: Check for WSAEAFNOSUPPORT or WSAEPROTONOSUPPORT if socket() fails for AF_BTH.

Vitaly Lipatov (1):
      wow64: Skip memcpy for null pointer.

Yongjie Yao (1):
      wmp: Check the return value of IOleClientSite_QueryInterface().

Yuri Hérouard (1):
      comctl32: Avoid segfault in PROPSHEET_DoCommand when psInfo is NULL.

Ziqing Hui (7):
      shell32/tests: Remove old windows tests for test_copy.
      shell32/tests: Add more tests to test_copy.
      shell32/tests: Introduce check_file_operation helper.
      shell32/tests: Use more check_file_operation.
      shell32/tests: Use check_file_operation in test_rename.
      shell32/tests: Use check_file_operation in test_delete.
      shell32/tests: Use check_file_operation in test_move.
```
