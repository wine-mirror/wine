The Wine development release 9.16 is now available.

What's new in this release:
  - Initial Driver Store implementation.
  - Pbuffer support in the Wayland driver.
  - More prototype objects in MSHTML.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/9.x/wine-9.16.tar.xz>

Binary packages for various distributions will be available
from <https://www.winehq.org/download>

You will find documentation on <https://www.winehq.org/documentation>

Wine is available thanks to the work of many people.
See the file [AUTHORS][1] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/raw/wine-9.16/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 9.16 (total 25):

 - #30938  Update a XIM candidate position when cursor location changes
 - #37732  Corel Paint Shop Pro X7 Installer fails
 - #41607  Piggi 2 Demo version fails to start and throws runtime and fatal error
 - #42997  Opera Neon Installation throws backtrace
 - #43251  Anarchy Online Login Window Play and Settings button disapper after minimising and maximising window
 - #44516  Anarchy Online doesn't start in Windows7 prefix
 - #45105  heap-buffer overflow in gdi32 (CVE-2018-12932)
 - #45106  OOB write in gdi32 (CVE-2018-12933)
 - #45455  Multiple DIFxApp-based USB hardware driver installers fail due to missing 'setupapi.dll.DriverStoreFindDriverPackageW' stub (Cetus3D-Software UP Studio 2.4.x, Plantronics Hub 3.16)
 - #46375  Vietcong: game save thumbnails (screenshots) have corrupted colors
 - #48521  StaxRip 2.0.6.0 (.NET 4.7 app) reports 'System.ComponentModel.Win32Exception (0x80004005): Invalid function' when converting AVI (PowerRequest stubs need to return success)
 - #53839  Anarchy Online (Old Engine) Installer hangs after downloading game files
 - #54587  GMG-Vision - ShellExecuteEx failed: Bad EXE format for install.exe.
 - #55841  Lotus Approach: print "Properties" button ignored
 - #56622  Systray icons now have black background without compositor, and on some panels can be misaligned or don't redraw, becoming invisible
 - #56825  Unable to see screen for Harmony Assistant version 9.9.8d (64 bit)
 - #56884  FoxitPDFReaderUpdateService shows crash dialog
 - #57003  Naver Line tray icon is always gray since 9.13
 - #57036  Splashtop RMM client crashes on Wine 9.14
 - #57055  Window surfaces are empty
 - #57059  Regression - UI broken with winewayland
 - #57070  cnc-ddraw OpenGL performance regression in Wine 9.15
 - #57071  Incorrect window drawing
 - #57073  StaxRip 2.0.6.0  crashes inside gdiplus
 - #57083  ClickStamper: stamp circle is missing.

### Changes since 9.15:
```
Aida Jonikienė (1):
      win32u: Initialize parent_rect variables in window.c.

Akihiro Sagawa (5):
      oleaut32/tests: Add OLE Picture object tests using DIB section.
      oleaut32: Convert 32-bit or 16-bit color bitmaps to 24-bit color DIBs when saving.
      oleaut32: Initialize reserved members before saving.
      gdiplus/metafile: Fix DrawEllipse record size if compressed.
      gdiplus/metafile: Fix FillEllipse record size if compressed.

Alex Henrie (3):
      rpcrt4/tests: Correct a comment in test_pointer_marshal.
      rpcrt4/tests: Test whether Ndr(Get|Free)Buffer calls StubMsg.pfn(Allocate|Free).
      rpcrt4/tests: Allocate stub buffers with NdrOleAllocate.

Alexandre Julliard (15):
      ntdll: Avoid nested ARM64EC notifications.
      ntdll: Support STATUS_EMULATION_SYSCALL exception on ARM64EC.
      ntdll: Add a test for BeginSimulation().
      wow64: Fetch the initial thread context from the CPU backend.
      wow64: Only update necessary registers when raising exceptions.
      include: Unify the syscall thunk on x86-64.
      include: Clean up formatting of long asm statements.
      ntdll: Allocate the ARM64EC code map in high memory.
      ntdll: Allocate the x64 context on the asm side of KiUserExceptionDispatcher.
      ntdll: Pass the full exception record to virtual_handle_fault().
      ntdll: Implement Process/ThreadManageWritesToExecutableMemory.
      ntdll: Implement NtSetInformationVirtualMemory(VmPageDirtyStateInformation).
      ntdll/tests: Add tests for Process/ThreadManageWritesToExecutableMemory.
      ntdll: Read the Chpev2ProcessInfo pointer before accessing the data.
      shell32/tests: Delete a left-over file.

Alexandros Frantzis (5):
      winewayland: Store all window rects in wayland_win_data.
      winewayland: Support WGL_ARB_pbuffer.
      winewayland: Support WGL_ARB_render_texture.
      winewayland: Advertise pbuffer capable formats.
      winewayland: Fix off-by-one error in format check.

Alfred Agrell (2):
      mountmgr.sys: Read and use disk label from dbus.
      kernelbase: Delete now-inaccurate 'FS volume label not available' message.

Alistair Leslie-Hughes (4):
      odbccp32: Support driver config in SQLGet/WritePrivateProfileStringW.
      odbccp32: Correctly hanndle ODBC_BOTH_DSN in SQLWritePrivateProfileStringW.
      odbccp32: Fall back to ConfigDSN when ConfigDSNW cannot be found.
      kernel32: Return a valid handle in PowerCreateRequest.

Anders Kjersem (2):
      reg/tests: Wait for process termination.
      wscript: Implement Sleep.

Andrey Gusev (1):
      wined3d: Fix return for WINED3D_SHADER_RESOURCE_TEXTURE_2DMSARRAY.

Bernhard Übelacker (1):
      userenv: Avoid crash in GetUserProfileDirectoryW.

Billy Laws (4):
      include: Move arm64ec_shared_info to winternl and point to it in the PEB.
      ntdll: Initialize dll search paths before WOW64 init.
      ntdll: Call the exception preparation callback on ARM64EC.
      configure: Don't use CPPFLAGS for PE cross targets.

Brendan McGrath (3):
      mf/tests: Test video processor with 2D buffers.
      mf/tests: Test 1D/2D output on 2D buffers.
      mf/tests: Test 2D buffers can override stride.

Daniel Lehman (3):
      odbc32: Support {}'s in Driver connection string.
      msvcr120: Add feholdexcept stub.
      msvcr120: Implement feholdexcept.

Danyil Blyschak (4):
      win32u: Don't check ansi_cp when adding fallback child font.
      win32u: Include Microsoft Sans Serif as a child font.
      win32u: Include Tahoma and its linked fonts in child font list.
      mlang: Use larger destination buffer with WideCharToMultiByte().

Dmitry Timoshkov (10):
      gdiplus: Add a couple of missing gdi_dc_release().
      msv1_0: Make buffer large enough to hold NTLM_MAX_BUF bytes of base64 encoded data.
      secur32/tests: Don't load secur32.dll dynamically.
      secur32/tests: Avoid assigning a 4-byte status to an 1-byte variable.
      secur32/tests: Don't use fake user/domain/password in NTLM tests.
      secur32/tests: Fix a typo.
      secur32/tests: Separate NTLM signature and encryption tests.
      secur32/tests: Make NTLM encryption tests work on newer Windows versions.
      kernel32/tests: Fix compilation with a PSDK compiler.
      kernel32: EnumCalendarInfo() should ignore CAL_RETURN_NUMBER flag.

Elizabeth Figura (17):
      widl: Explicitly check for top-level parameters before adding FC_ALLOCED_ON_STACK.
      widl: Get rid of the write_embedded_types() helper.
      widl: Ignore strings in is_embedded_complex().
      widl: Propagate attrs to inner pointer types.
      setupapi: Move INF installation functions to devinst.c.
      setupapi: Separate a copy_inf() helper.
      setupapi: Add an any_version_is_compatible() helper.
      setupapi: Implement DriverStoreAddDriverPackage().
      setupapi: Implement DriverStoreDeleteDriverPackage().
      setupapi: Implement DriverStoreFindDriverPackage().
      setupapi: Uninstall from the driver store in SetupUninstallOEMInf().
      setupapi: Install to the driver store in SetupCopyOEMInf().
      setupapi: Add stub spec entries for DriverStoreEnumDriverPackage().
      setupapi/tests: Add Driver Store tests.
      winevulkan: Strip the name tail when parsing members.
      winevulkan: Add video interfaces.
      winevulkan: Fix pointer arithmetic in debug utils callback marshalling.

Eric Pouech (1):
      winedump: Get rid of GCC warning.

Esme Povirk (5):
      user32: Implement EVENT_OBJECT_STATECHANGE for BM_SETSTYLE.
      user32: Implement EVENT_SYSTEM_DIALOGEND.
      user32: Implement EVENT_OBJECT_CREATE for listboxes.
      user32: Implement EVENT_OBJECT_DESTROY for listboxes.
      user32: Implement MSAA events for LB_SETCURSEL.

Etaash Mathamsetty (1):
      netapi32: Move some implementations to netutils.

Fabian Maurer (1):
      printdlg: Allow button id psh1 for "Properties" button.

Gabriel Ivăncescu (6):
      mshtml: Avoid calling remove_target_tasks needlessly.
      mshtml: Don't return default ports from location.host in IE10+ modes.
      mshtml: Implement EmulateIE* modes for X-UA-Compatible.
      mshtml/tests: Accept rare return value from ReportResult on native.
      jscript: Don't use call frame for indirect eval calls for storing variables.
      jscript: Restrict the allowed escape characters of JSON.parse in html mode.

Hans Leidekker (18):
      odbc32: Add locking around driver calls.
      odbc32: Remove unused Unix calls.
      odbc32: Try the DRIVER attribute if the DSN attribute is missing.
      odbc32: Fail freeing an object while child objects are alive.
      odbc32: Call the driver function in SQLBindCol().
      odbc32: Add support for descriptor handles in SQLGet/SetStmtAttr().
      odbc32: Parse incoming connection string in SQLBrowse/DriverConnct().
      odbc32: Set win32_funcs on the descriptors returned from SQLGetStmtAttr().
      odbc32: Implicit descriptors can be retrieved but not set.
      odbc32: Connection string keywords are case insensitive.
      odbccp32: Respect config mode in SQLGet/WritePrivateProfileString().
      odbccp32: Use wide character string literals.
      odbccp32/tests: Get rid of a workaround for XP.
      odbccp32: Handle NULL DSN in SQLValidDSN().
      odbc32: Handle missing SQLSetConnect/EnvAttr().
      odbc32: Set initial login timeout to 15 seconds.
      odbc32: Map SQLColAttributes() to SQLColAttribute().
      odbc32: Map SQLGetDiagRec() to SQLError().

Ilia Docin (3):
      comctl32/tests: Add rebar chevron visibility test.
      comctl32/tests: Add LVM_GETNEXTITEM test.
      comctl32/listview: Do not return items count on getting next item for last one.

Ivo Ivanov (3):
      winebus.sys: Pass each input report regardless of report ID to the pending read IRQ.
      winebus.sys: Add devtype parameter to get_device_subsystem_info().
      winebus.sys: Read vendor/product/serial strings from the usb subsystem.

Jacek Caban (33):
      mshtml: Add support for MSEventObj prototype objects.
      mshtml: Add support for Event prototype objects.
      mshtml: Add support for UIEvent prototype objects.
      mshtml: Add support for mouse event prototype objects.
      mshtml: Add support for keyboard event prototype objects.
      mshtml: Add support for page transition event prototype objects.
      mshtml: Add support for custom event prototype objects.
      mshtml: Add support for message event prototype objects.
      mshtml: Add support for progress event prototype objects.
      mshtml: Add support for storage event prototype objects.
      mshtml: Add support for screen prototype objects.
      mshtml: Add support for history prototype objects.
      mshtml: Add support for plugins collection prototype objects.
      mshtml: Add support for MIME types collection prototype objects.
      mshtml: Add support for performance timing prototype objects.
      mshtml: Add support for performance navigation prototype objects.
      mshtml: Add support for performance prototype objects.
      mshtml: Add support for namespace collection prototype objects.
      mshtml: Add support for console prototype objects.
      mshtml: Add support for media query list prototype objects.
      mshtml: Add support for client rect list prototype objects.
      mshtml: Add support for DOM token list prototype objects.
      mshtml: Add support for named node map prototype objects.
      mshtml: Add support for element collection prototype objects.
      mshtml: Add support for node list prototype objects.
      mshtml: Add support for text range prototype objects.
      mshtml: Add support for range prototype objects.
      mshtml: Add support for selection prototype objects.
      mshtml: Increase buffer size in dispex_to_string.
      mshtml: Add support for unknown element prototype objects.
      mshtml: Add support for comment prototype objects.
      mshtml: Add support for attribute prototype objects.
      mshtml: Add support for document fragment prototype objects.

Jacob Czekalla (4):
      comctl32/tests: Add tests for progress bar states.
      comctl32/progress: Add states for progress bar.
      comctl32/tests: Add test to check if treeview expansion can be denied.
      comctl32/treeview: Allow treeview parent to deny treeview expansion.

Louis Lenders (3):
      kernel32: Fake success in PowerCreateRequest.
      kernel32: Fake success in PowerSetRequest.
      kernel32: Fake success in PowerClearRequest.

Martin Storsjö (1):
      ucrtbase: Export powf on i386.

Nikolay Sivov (7):
      include: Add IWICStreamProvider definition.
      windowscodecs/tests: Add a helper to check for supported interfaces.
      windowscodecs/tests: Remove noisy traces from the test stubs.
      windowscodecs/tests: Add some tests for IWICStreamProvider interface availability.
      windowscodecs/metadata: Add a stub for IWICStreamProvider.
      windowscodecs/metadata: Reset the handler on LoadEx(NULL).
      windowscodecs/tests: Add some more tests for the IWICStreamProvider methods.

Paul Gofman (25):
      qcap/tests: Fix test failure in testsink_Receive() on some hardware.
      qcap/tests: Add a test for simultaneous video capture usage.
      quartz: Propagate graph start error in MediaControl_Run().
      qcap: Delay setting v4l device format until stream start.
      gdi32/tests: Add test for Regular TTF with heavy weight.
      win32u: Always set weight from OS/2 header in freetype_set_outline_text_metrics().
      win32u: Do not embolden heavy weighted fonts.
      kernelbase: Better match Windows behaviour in PathCchStripToRoot().
      kernelbase: Better match Windows behaviour in PathCchRemoveFileSpec().
      kernelbase: Better match Windows behaviour in PathCchIsRoot().
      kernelbase: Reimplement PathIsRootA/W() on top of PathCchIsRoot().
      kernelbase: Reimplement PathStripToRootA/W() on top of PathCchStripToRoot().
      kernelbase: Remimplement PathRemoveFileSpecA/W().
      kernelbase/tests: Add more tests for some file manipulation functions.
      shell32: Register ShellItem coclass.
      include: Add some shell copy engine related constants.
      shell32/tests: Add tests for IFileOperation_MoveItem.
      shell32/tests: Add tests for IFileOperation_MoveItem notifications.
      shell32: Implement file_operation_[Un]Advise().
      shell32: Store operation requested in file_operation_MoveItem().
      shell32: Partially implement MoveItem file operation execution.
      shell32: Implement directory merge for MoveItem.
      shell32: Support sink notifications for file MoveItem operation.
      ws2_32/tests: Test UDP broadcast without SO_BROADCAST.
      server: Support IPV4 UDP broadcast on connected socket without SO_BROADCAST.

Piotr Caban (6):
      ntdll: Accept UNC paths in LdrGetDllPath.
      ntdll: Accept UNC paths in LdrAddDllDirectory.
      msi: Fix msi_add_string signature.
      msi: Fix row index calculation when stringtable contains empty slots.
      vcruntime140_1: Handle empty catchblock type_info in validate_cxx_function_descr4.
      vcruntime140_1: Store exception record in ExceptionInformation[6] during unwinding.

Rozhuk Ivan (1):
      wineoss: Store the OSS device in midi_dest.

Rémi Bernon (59):
      win32u: Don't request a host window surface for child windows.
      win32u: Avoid crashing with nulldrv when creating offscreen surface.
      mfplat/tests: Add more tests for MFCreateMediaBufferFromMediaType.
      mfplat: Implement MFCreateMediaBufferFromMediaType for video formats.
      mf/tests: Cleanup the video processor test list.
      win32u: Introduce a new window rects structure.
      win32u: Use window_rects struct in set_window_pos.
      win32u: Use window_rects struct in update_window_state.
      win32u: Use window_rects struct in NtUserCreateWindowEx.
      win32u: Pass a window_rects struct to calc_winpos helper.
      winex11: Check whether the window surface needs to be re-created.
      win32u: Fix CreateWindowSurface call when updating layered surfaces.
      winex11: Avoid moving embedded windows in the systray dock.
      winex11: Don't call X11DRV_SET_DRAWABLE with invalid drawable.
      win32u: Split driver side window bits move to a separate entry.
      win32u: Use window_rects structs in apply_window_pos.
      win32u: Pass window_rects structs to create_window_surface.
      win32u: Pass a window_rects struct to WindowPosChanged driver entry.
      win32u: Pass a window_rects struct to WindowPosChanging driver entry.
      win32u: Pass a window_rects struct to MoveWindowBits driver entry.
      win32u: Introduce new get_(client|window)_rect_rel helpers.
      win32u: Pass a window_rects struct from get_window_rects helper.
      win32u: Pass a window_rects struct to calc_ncsize helper.
      win32u: Keep a window_rects struct in the WND structure.
      winebus.sys: Always parse uevent PRODUCT= line on input subsystem.
      mfplat/tests: Add missing todo_wine for MFCreateMediaBufferFromMediaType.
      mf/tests: Load MFCreateMediaBufferFromMediaType dynamically.
      win32u: Get visible and client rects in parent-relative coordinates.
      winex11: Discard previous surface when window is directly drawn to.
      win32u: Flush window surface after UpdateLayeredWindow.
      winex11: Remove now unnecessary window surface flushes.
      winemac: Remove unused unminimized window surface.
      winemac: Remove unnecessary window surface invalidation.
      winex11: Fix the exposed window surface region combination.
      win32u: Introduce a new NtUserExposeWindowSurface call.
      wineandroid: Remove now unnecessary window surface pointer.
      winemac: Remove now unnecessary window surface pointer.
      winex11: Remove now unnecessary window surface pointer.
      win32u: Allocate and initialize window surface in window_surface_create.
      wineandroid: Keep a window_rects struct in the driver window data.
      winemac: Remove unnecessary window data rects update.
      winemac: Keep a window_rects struct in the driver window data.
      winemac: Use the window rects to convert host visible to window rect.
      winemac: Use the window rects to convert window to host visible rect.
      win32u: Keep SetIMECompositionWindowPos with other IME entries.
      winex11: Keep a window_rects struct in the driver window data.
      winex11: Use the driver rects to convert from host visible to window rect.
      winemac: Return a macdrv_window_features from get_cocoa_window_features.
      win32u: Move visible rect computation out of the drivers.
      win32u: Move the "Decorated" driver registry option out of the drivers.
      mfplat: Add MFCreateLegacyMediaBufferOnMFMediaBuffer stub.
      mfplat/tests: Test MFCreateLegacyMediaBufferOnMFMediaBuffer.
      mfplat: Implement MFCreateLegacyMediaBufferOnMFMediaBuffer.
      user32: Pass a free_icon_params struct to User16CallFreeIcon.
      user32: Pass a thunk_lock_params struct to User16ThunkLock.
      user32: Introduce a generic KeUserDispatchCallback kernel callback.
      user16: Use NtUserDispatchCallback instead of User16ThunkLock.
      winevulkan: Route kernel callbacks through user32.
      opengl32: Route kernel callbacks through user32.

Sven Püschel (11):
      dssenh/tests: Remove skipping sign tests.
      dssenh/tests: Remove unnecessary verify test cases.
      dssenh/tests: Move test loop out of sub function.
      dssenh/tests: Remove redundant hash test.
      dssenh: Finish hash in the SignHash function.
      dssenh/tests: Fix signLen usage.
      dssenh/tests: Set 0xdeadbeef last error before function execution.
      dssenh/tests: Remove redundant signature verification.
      dssenh/tests: Separate the public key export from the signhash test.
      dssenh/tests: Add test for VerifySignatureA.
      dssenh: Swap the endianness of the signature.

Tim Clem (1):
      winemac.drv: Better detect whether to unminimize a window when the app becomes active.

Tom Helander (4):
      httpapi: Add tests for HttpResponseSendEntityBody.
      http.sys: Skip clean up if HTTP_SEND_RESPONSE_FLAG_MORE_DATA is set.
      httpapi: Handle HTTP_SEND_RESPONSE_FLAG_MORE_DATA flag.
      httpapi: Implement HttpSendResponseEntityBody.

Torge Matthies (1):
      wineandroid: Route kernel callbacks through user32.

Zhiyi Zhang (6):
      riched20: Release IME input context when done using it.
      user32/tests: Add more display DC bitmap tests.
      win32u: Use a full size bitmap for display device contexts.
      win32u: Support setting host IME composition window position for ImmSetCompositionWindow().
      win32u: Set host IME composition window position in set_caret_pos().
      win32u: Set host IME composition window position in NtUserShowCaret().
```
