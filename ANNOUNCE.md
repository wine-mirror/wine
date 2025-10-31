The Wine development release 10.18 is now available.

What's new in this release:
  - OpenGL memory mapping using Vulkan in WoW64 mode.
  - Synchronization barriers API.
  - Support for WinRT exceptions.
  - SCSI pass-through in WoW64 mode.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/10.x/wine-10.18.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-10.18/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 10.18 (total 30):

 - #10349  Yukon Trail installer crashes at the end during DDE communication with Progman
 - #24466  Sid Meier's Pirates! frequent hiccups, temporary lock-ups
 - #33492  Yukon Trail installer fails to install its font
 - #48027  cmd pipe | not triggering ReadFile EOF
 - #48787  WineD3D C&C Generals Zero Hour takes a lot time to load or maximize
 - #55981  GL applications (and d3d applications using the GL backend) are slow in new wow64
 - #56209  winetricks vb5run fails
 - #56915  Autodesk Fusion requires GetUserLanguages which is not implemented
 - #57259  Keepass 2 automatic update dialog is broken (uses Taskdialog) EnableVisualStyles() from WinForms evidently doesn't work
 - #57376  Permission denied vs No such file or directory when opening folders with trailing backslash
 - #58229  _SH_SECURE sharing flag is not supported and causing _wfsopen to fail
 - #58257  CD Manipulator cannot detect the drive model name in experimental wow64 mode.
 - #58331  Wait cursor does not display
 - #58391  Exact Audio Copy: Unable to interface with DVD drive on WoW64
 - #58598  Realterm_2.0.0.70 / Crashes on start
 - #58611  Nightshade crashes: err:seh:NtRaiseException Unhandled exception code c0000409 (native vcruntime140 works around the crash)
 - #58697  Jolly Rover hangs upon starting with 100% cpu usage
 - #58747  Directory opus fails start.
 - #58749  Delayed startup of wine processes in Wine-10.16 when winemenubuilder.exe is disabled
 - #58752  assembly:Z:\usr\share\wine\mono\wine-mono-10.2.0\lib\mono\4.5\mscorlib.dll type:DllNotFoundException
 - #58791  MDk 2 games windows
 - #58794  Multiple applications crash with assertion failure in cache_inproc_sync()
 - #58795  The Witcher 2 crashes on start
 - #58804  Incorrect ConfigureNotify processing in xfce
 - #58812  PlayOnline Viewer: Application runs and works otherwise, but only shows a black screen.
 - #58816  macOS: Window focus / activation issue
 - #58828  Configure (build) fails since commit db2e157, if autoreconf needs to be applied before
 - #58837  getCurrentThreadID error when enabling speedhack in Cheat Engine
 - #58839  kernel32.timeGetTime error when enabling speedhack in Cheat Engine
 - #58852  Minimised applications are restored with -4 vertical pixels (redux).

### Changes since 10.17:
```
Adam Markowski (1):
      wine.desktop: Add Polish translation.

Akihiro Sagawa (9):
      ntdll: Drop unused raw SCSI command support for NetBSD.
      ntdll: Remove useless request_sense type check.
      ntdll: Separate the I/O buffers in IOCTL_SCSI_PASS_THROUGH.
      ntdll: Add wow64 support for IOCTL_SCSI_PASS_THROUGH.
      kernel32/tests: Add IOCTL_SCSI_PASS_THROUGH tests for optical drives.
      ntdll: Remove redundant definitions for Linux.
      ntdll: Separate the I/O buffers in IOCTL_SCSI_PASS_THROUGH_DIRECT.
      ntdll: Add wow64 support for IOCTL_SCSI_PASS_THROUGH_DIRECT.
      kernel32/tests: Add IOCTL_SCSI_PASS_THROUGH_DIRECT tests for optical drives.

Alex Henrie (2):
      kernel32: Move some time functions from winmm.
      advpack: Ignore lines that begin with '@' in (Un)RegisterOCXs sections.

Alexandre Julliard (19):
      server: Only use the export name for builtin modules.
      winebuild: Always output an export directory for fake dlls.
      server: Also use the export name for fake dlls.
      configure: Work around install-sh requirement in autoconf <= 2.69.
      makedep: Only output install rules at the top level.
      makedep: Generate the uninstall files list directly from the install commands.
      makedep: Store install command line arguments in the command list.
      makedep: Make the output_rm_filenames helper more generic.
      makedep: Install multiple files at once when possible.
      ntdll: Don't map the PE header as executable.
      gitlab: Fix regexp to extract dll name for Windows tests.
      kernel32/tests: Use CRT allocation functions.
      comctl32/tests: Use CRT allocation functions.
      kernelbase: Fix error returned for paths with a trailing backslash.
      mountmgr.sys: Only create serial and parallel keys when needed.
      include: Add a couple of reparse point definitions.
      ws2_32/tests: Add some tests for AF_UNIX sockets.
      kernelbase: Reject all-space paths in SearchPath().
      kernelbase: Reject empty command lines in CreateProcess().

Alexandros Frantzis (1):
      winewayland: Pass through mouse events for transparent, layered windows.

Alistair Leslie-Hughes (2):
      bcrypt: Add missing breaks.
      include: Correct STACKFRAME for 64bits in imagehlp.h.

Anton Baskanov (8):
      fluidsynth: Import upstream release 2.4.2.
      fluidsynth: Calculate the inverted value explicitly in fluid_mod_transform_source_value().
      fluidsynth: Invert relative to the max value instead of 1.0 in fluid_mod_transform_source_value().
      dmusic/tests: Add tests for GUID_DMUS_PROP_Volume.
      dmusic: Set volume when creating a synth.
      dmusic: Handle GUID_DMUS_PROP_Volume.
      dmime: Set master volume when adding a port.
      dmime: Update master volume when GUID_PerfMasterVolume is set.

Bernd Herd (3):
      sane.ds: Fix native TWAIN transfer mode for 64-bit.
      gphoto2.ds: Decrease the number of remaining pending data transfers after every transfer.
      gphoto2.ds: Return the DIB as a GlobalAlloc handle according to TWAIN spec instead of a HBITMAP.

Brendan Shanks (4):
      winemac.drv: Include Carbon.h in macdrv_cocoa.h and remove duplicate definitions.
      include: Implement C_ASSERT() using the C23 static_assert() if available.
      ntdll: Avoid mmap() failures and Gatekeeper warnings when mapping files with PROT_EXEC.
      configure: Only run checks in WINE_(MINGW_)PACKAGE_FLAGS when CFLAGS or LIBS is non-empty.

Byeong-Sik Jeon (2):
      winemac: Use event handle to detect macdrv ime processing done.
      win32u: Fix typo.

Conor McCarthy (8):
      mf/tests: Skip the scheme resolver tests if network connectivity is unavailable.
      mf/tests: Fix occasional hangs in test_media_session_source_shutdown().
      mf/tests: Skip some tests invalid on current Windows 11.
      mf/tests: Fix a scheme resolver test failure on current Windows 11.
      mf/tests: Test H.264 sink media type height alignment.
      mf/tests: Test H.264 decoder alignment.
      mfmediaengine/tests: Test TransferVideoFrame() to IWICBitmap.
      mfmediaengine: Support TransferVideoFrame() to IWICBitmap.

Dmitry Timoshkov (5):
      wine.inf: Create [fonts] section in win.ini for 16-bit apps.
      user32/tests: Add more tests for PackDDElParam/UnpackDDElParam(WM_DDE_ACK).
      user.exe: Use larger buffer size for checking atom name.
      user.exe: Add special handling for WM_DDE_ACK in response to WM_DDE_INITIATE.
      wldap32/tests: Add one more check for an unreachable server.

Elizabeth Figura (9):
      kernelbase: Remove an unused NtQueryInformationFile() from MoveFileWithProgress().
      ntdll: Call cache_inproc_sync() inside the lock.
      ntdll/tests: Add more reparse point tests.
      ntdll/tests: Test NtQueryAttributesFile() with a root directory.
      shell32/tests: Fix test failures in test_GetAttributesOf().
      include: Pass a PROCESS_INFORMATION to winetest_wait_child_process().
      user32: Implement CascadeWindows().
      user32: Implement TileWindows().
      winex11: Null-terminate a string used with RTL_CONSTANT_STRING().

Eric Pouech (9):
      cmd: Fix lexer on input redirection.
      cmd: Show file size greater than 4G in DIR command.
      cmd: Initialize a local variable.
      cmd: Fix a potential out of bounds access.
      cmd: Fixed an invalid memory access spotted by SAST.
      dbghelp: Use Unicode for some pathnames.
      dbghelp: Remove useless process parameters.
      dbghelp: Rewrite debug format search & load logic.
      dbghelp: {dwarf} Keep .eh_frame mapped when no CU are present.

Felix Hädicke (3):
      wbemprox/tests: Use check_property function in test_MSSMBios_RawSMBiosTables.
      wbemprox/tests: Add tests for MSSMBios_RawSMBiosTables properties.
      wbemprox: Implement more MSSMBios_RawSMBiosTables properties.

Gabriel Ivăncescu (15):
      mshtml: Use a common object implementation for HTMLElementCollection enumerator.
      mshtml: Use the common object implementation for HTMLDOMChildrenCollection enumerator.
      mshtml: Use the common object implementation for HTMLFormElement enumerator.
      mshtml: Use the common object implementation for HTMLSelectElement enumerator.
      mshtml: Use the common object implementation for HTMLRectCollection enumerator.
      mshtml: Use the common object implementation for HTMLStyleSheetsCollection enumerator.
      mshtml: Use the common object implementation for HTMLAttributeCollection enumerator.
      mshtml: Add private stub interfaces for validating form elements.
      mshtml: Add private stub interfaces for validating input elements.
      mshtml: Add private stub interfaces for validating button elements.
      mshtml: Add private stub interfaces for validating object elements.
      mshtml: Add private stub interfaces for validating select elements.
      mshtml: Add private stub interfaces for validating text area elements.
      mshtml: Implement checkValidity for HTMLInputElement.
      mshtml: Implement enctype for HTMLFormElement.

Gerald Pfeifer (1):
      winebus: Don't define hidraw_device_read_report unless used.

Ivan Lyugaev (1):
      sane.ds: Adding saving scanner settings to the registry.

Jacek Caban (16):
      gitlab: Update to llvm-mingw 20251007.
      opengl32: Update to the current OpenGL spec.
      opengl32: Include legacy aliases in the extension registry.
      opengl32: Factor out make_context_current.
      opengl32: Use the actual context GL version in is_extension_supported.
      opengl32: Use stored GL version in check_extension_support.
      opengl32: Hide GL versions larger than 4.3 on wow64 in make_context_current.
      opengl32: Use stored context GL version in get_integer.
      opengl32: Use stored GL version in wrap_glGetString.
      opengl32: Split extensions by null bytes in the registry.
      opengl32: Compute supported extensions in make_context_current.
      opengl32: Use stored extension list in is_extension_supported.
      opengl32: Use stored extensions in filter_extensions.
      opengl32: Wrap buffer storage functions.
      opengl32: Support wow64 buffer storage and persistent memory mapping using Vulkan.
      urlmon/tests: Fix PersistentZoneIdentifier tests on some Windows versions.

Louis Lenders (1):
      uxtheme: Add stub for GetThemeStream.

Marc-Aurel Zent (2):
      ntdll: Do not set main thread QoS class on macOS.
      ntdll: Remove unused parameter from linux_query_event_obj().

Michael Müller (1):
      wineboot: Ensure that the ProxyEnable value is created.

Nikolay Sivov (39):
      oleaut32/tests: Enable more tests for VarAdd().
      oleaut32/tests: Enable more tests for VarOr().
      oleaut32/tests: Enable more tests for VarXor().
      oleaut32/tests: Enable more tests for VarCmp().
      d2d1/tests: Add some tests for resulting format of the WIC target.
      d2d1: Set alpha mode correctly for the WIC target.
      d2d1: Implement Tessellate() for path geometries.
      d2d1: Implement Tessellate() for ellipse geometries.
      d2d1: Implement Tessellate() for rectangle geometries.
      d2d1: Implement Tessellate() for rounded rectangle geometries.
      d2d1/tests: Add a ComputeArea() test for the path geometry.
      d2d1: Implement ComputeArea() for path geometries.
      include: Remove some msxml6 types from msxml2.idl.
      msxml3/tests: Remove duplicated test.
      include: Remove more coclass entries from msxml2.idl.
      d2d1: Implement ComputeArea() for ellipse geometries.
      d2d1: Implement ComputeArea() for rounded rectangle geometries.
      d2d1: Implement Tessellate() for transformed geometries.
      d2d1/tests: Reduce test run count for some tests.
      msxml6/tests: Move remaining tests for SAXXMLReader60.
      include: Remove SAXXMLReader60 from msxml2.idl.
      d2d1/tests: Extend test geometry sink.
      d2d1/tests: Add a basic test for geometry streaming.
      d2d1: Track specific failure reason during geometry population.
      d2d1: Set more appropriate error code on geometry population.
      d2d1: Handle invalid segment flags in SetSegmentFlags().
      msxml6/tests: Add a leading whitespace test.
      msxml3/tests: Add some SAX tests for PI nodes.
      msxml3/sax: Add processing instructions callback.
      msxml3/sax: Add basic support for startDTD()/endDTD().
      msxml3/tests: Add some tests for IXMLDocument::get_charset().
      msxml3/tests: Add some tests for "xmldecl-encoding".
      msxml3/tests: Add more tests for document properties.
      msxml3/tests: Add more tests for supported interfaces.
      d2d1/tests: Add some property tests for the Gaussian Blur effect.
      d2d1/tests: Add some property tests for the Point Specular effect.
      d2d1/tests: Add some property tests for the Arithmetic Composite effect.
      msxml3: Update to IXMLDocument2.
      msxml3: Return static string from get_version().

Paul Gofman (9):
      user32: Move GetWindowDisplayAffinity() stub to win32u.
      kernel32: Avoid RBP-based frame in BaseThreadInitThunk() on x64.
      ntdll: Add synchronization barrier stubs.
      ntdll: Implement synchronization barrier functions.
      kernelbase: Implement synchronization barrier functions.
      tdh: Add semi-stub for TdhEnumerateProviders().
      server: Use process_vm_readv() for other process memory read when available.
      ntdll: Get written size from server in NtWriteVirtualMemory().
      server: Use process_vm_writev() for other process memory write when available.

Piotr Caban (17):
      d3d11: Fix wait_child_process() related test failures.
      dinput: Fix wait_child_process() related test failures.
      imm32: Fix wait_child_process() related test failures.
      kernel32: Fix wait_child_process() related test failures.
      mfplat: Fix wait_child_process() related test failures.
      user32: Fix wait_child_process() related test failures.
      win32u: Fix wait_child_process() related test failures.
      windowscodecs: Fix wait_child_process() related test failures.
      timeout: Fix wait_child_process() related test failures.
      msvcrt: Deduplicate exception copying code.
      msvcrt: Handle IUnknown exceptions in __DestructExceptionObject().
      msvcrt: Handle IUnknown exceptions in copy_exception helper.
      msvcrt: Call __DestructExceptionObject() in __ExceptionPtrDestroy.
      ucrtbase: Fix wait_child_process crash.
      msado15/tests: Test rowset queried interfaces instead of tracing them.
      msado15: Mark recordset open in ADORecordsetConstruction:put_Rowset.
      msvcrt: Rename exception *IUNKNOWN flags to *WINRT.

Robert Wilhelm (1):
      scrrun: Implement MoveFolder().

Rémi Bernon (40):
      server: Don't set QS_HARDWARE for every hardware message.
      win32u: Only return from NtWaitForMultipleObjects if signaled.
      win32u: Restore queue masks after processing driver events.
      opengl32: Use a buffer map for gl(Draw|Read)Buffer(s).
      opengl32: Implement front buffer rendering emulation.
      win32u: Reset internal pixel format when pixel format is set.
      win32u: Use floats for map_monitor_rect computation.
      win32u: Stub NtGdiDdDDI(Acquire|Release)KeyedMutex(2) syscalls.
      win32u/tests: Test acquire / release of D3DKMT keyed mutexes.
      win32u: Set the process idle event from the client side.
      winex11: Give colormap ownership to the surface after it's allocated.
      ntdll: Fix (Nt|Zw)WaitForMultipleObjects signature.
      win32u: Process driver events and internal hardware messages in NtUserSetCursor.
      winegcc: Support subsystem version number in link.exe-style cmdline.
      winegcc: Consistently use the subsystem option with Clang and MinGW.
      comctl32/tests: Force console subsystem version 5.2.
      dxgi/tests: Force console subsystem version 5.2.
      quartz/tests: Force console subsystem version 5.2.
      user32/tests: Force console subsystem version 5.2.
      winegcc: Set the default console and windows subsystems version to 6.0.
      win32u: Use a dedicated struct for D3DKMT mutex objects.
      server: Use a dedicated struct for D3DKMT mutex objects.
      win32u: Pass D3DKMT keyed mutex initial value on creation.
      win32u: Implement acquire / release of D3DKMT keyed mutexes.
      winex11: Move state change checks to window_update_client_state.
      winex11: Remove now unnecessary SC_MOVE command handling.
      win32u: Update the window config before its state, as internal config.
      win32u: Fix incorrect resource allocation object type.
      win32u: Call d3dkmt_destroy_sync from NtGdiDdDDIDestroySynchronizationObject.
      win32u: Simplify some vulkan resource cleanup calls.
      win32u: Query resource runtime data in d3dkmt_open_resource.
      win32u: Open resource keyed mutex and sync object on import.
      winevulkan: Require timeline semaphores for VK_KHR_win32_keyed_mutex.
      win32u/tests: Test vulkan / D3DKMT keyed mutex interop.
      win32u: Create timeline semaphores for resources keyed mutexes.
      win32u: Grow vkQueueSubmit buffers to allow for keyed mutex semaphores.
      win32u: Reserve an array of timeline semaphore submit structs.
      win32u: Implement keyed mutex acquire / release with timeline semaphores.
      winex11: Don't keep track of the minimized window rects.
      winex11: Use window rects for minimization in virtual desktop mode.

Tim Clem (2):
      winemac.drv: Never set a zero backing size for a GL context.
      winemac.drv: Explicitly track the shape layer used for letter/pillarboxes.

Vibhav Pant (14):
      rometadata: Implement IMetaDataTables::{Get{String,Blob,Guid,UserString}HeapSize, GetNumTables}.
      rometadata: Implement IMetaDataTables::GetTableInfo.
      rometadata: Implement IMetaDataTables::GetRow.
      rometadata: Implement IMetaDataTables::Get{String,Blob,Guid}.
      rometadata/tests: Add tests for IMetaDataTables::{GetColumnInfo, GetColumn}.
      rometadata: Implement IMetaDataTables::{GetColumn, GetColumnInfo}.
      kernelbase: Release console_section in case of an allocation failure in alloc_console.
      vccorlib140: Add AllocateException and FreeException stubs.
      vccorlib140: Add AllocateExceptionWithWeakRef stub.
      vccorlib140: Add stubs for exceptions-related functions.
      vccorlib140: Add tests for exceptions-related functions.
      vccorlib140: Add stubs for Exception constructors.
      vccorlib140: Implement AllocateException(WithWeakRef), FreeException.
      msvcrt: Update WinRT exception type info in _CxxThrowException.

Yuxuan Shui (4):
      wmvcore/tests: Check what happens to a WMSyncReader when its allocator fails.
      winegstreamer: Return NS_E_NO_MORE_SAMPLES from WMSyncReader if sample fails to read.
      user32/tests: Make sure testtext has enough null bytes at the end.
      reg: Fix out-of-bound read when string is empty.

Zhengyong Chen (1):
      server: Fix incorrect key modification in rename_key function.

Zhiyi Zhang (27):
      windows.ui.core.textinput: Add stub DLL.
      windows.ui.core.textinput: Add ICoreInputViewStatics stub.
      windows.ui.core.textinput: Add core_input_view_statics_GetForCurrentView() stub.
      windows.ui.core.textinput: Add core_input_view_GetCoreInputViewOcclusions() stub.
      windows.ui.core.textinput: Add core_input_view_add_OcclusionsChanged() stub.
      windows.ui.core.textinput: Implement IWeakReferenceSource for ICoreInputView.
      comctl32: Move uxtheme headers to comctl32.h.
      comctl32: Add a helper to open theme for a window.
      comctl32: Add a helper to close the theme for a window.
      comctl32: Add a helper to handle WM_THEMECHANGED.
      comctl32: Add a helper to handle WM_NCPAINT messages.
      comctl32/datetime: Add a helper to draw the background.
      comctl32/datetime: Remove theming for comctl32 v5.
      uxtheme: Add a stub at ordinal 49.
      comctl32/header: Add a helper to check if theme is enabled.
      comctl32/header: Add a helper to get the item text rect.
      comctl32/header: Add a helper to draw the item text.
      comctl32/header: Add a helper to draw the rest of the background.
      comctl32/header: Remove theming for comctl32 v5.
      comctl32/hotkey: Remove theming for comctl32 v5.
      comctl32/ipaddress: Add a helper to get the theme text state.
      comctl32/ipaddress: Add a helper to get the text colors.
      comctl32/ipaddress: Add a helper to draw the background.
      comctl32/ipaddress: Add a helper to draw the dots.
      comctl32/ipaddress: Remove theming for comctl32 v5.
      comctl32/listview: Remove theming for comctl32 v5.
      comctl32/monthcal: Remove theming for comctl32 v5.

Ziqing Hui (8):
      mfreadwrite: Add attributes member to writer struct.
      mfreadwrite: Add converter transform to stream.
      mfreadwrite: Implement IMFSinkWriterEx.
      mfreadwrite: Implement sink_writer_SetInputMediaType.
      mfreadwrite/tests: Test getting transform for a writer without converter.
      qasf: Implement seeking for asf reader.
      qasf: Add asf_reader_stop_stream helper.
      qasf: Add asf_reader_start_stream helper.
```
