The Wine development release 10.10 is now available.

What's new in this release:
  - Mono engine updated to version 10.1.0.
  - OSMesa library no longer needed.
  - More support for generating Windows Runtime metadata in WIDL.
  - Locale data updated to Unicode CLDR 47.
  - P010 format support in Media Foundation.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/10.x/wine-10.10.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-10.10/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 10.10 (total 38):

 - #17614  Rise of Nations: Both mouse keys required for single left-click
 - #19226  Braid: Both Shift keys needed to move puzzle pieces
 - #19662  Lotus Freelance Graphics 2.1 hangs at the splash screen
 - #21369  HTML-Kit 292's tab bar isn't fully visible without scrolling at 96 DPI
 - #24026  Tab completion for cmd
 - #24209  Burger Shop is shifted to the top left corner in full screen mode
 - #33302  regedit: binary values editor layout is broken
 - #35103  Baofeng5-5.31.1128's Welcome window crash on start
 - #40144  Installing Canon printer driver does not work
 - #44120  Steam Big Picture mode fails to display content, it shows a black screen (d3d10)
 - #44453  Noteworthy Composer crashes in winealsa
 - #50450  Ricoh Digital Camera Utility 5  crashes when switching from Browser to Laboratory and vise versa
 - #51350  Horizon Chase freezes on startup
 - #53002  Wondershare Uniconverter 13 not displaying characters
 - #53071  AVCLabs Video Enhancer AI crashes on start
 - #53401  regedit does not import .reg files
 - #55593  F.E.A.R crashes with "Out of memory" error when starting a new game
 - #55985  S.T.A.L.K.E.R. Anomaly: Crashes when loading into save file.
 - #57230  F.E.A.R Combat black screen at startup due to out of memory error
 - #58000  New thread stack uses too much memory
 - #58001  "Eador. Masters of the Broken World" bad map textures after starting game
 - #58064  Unreal 2 hangs with a black screen when switching to 1440X900 resolution
 - #58078  StarCraft Remastered: game is not started with wine-10.5
 - #58107  PlayOnline Viewer: Window not activated when restoring from a minimised state.
 - #58182  Virtual desktop corked in 10.6
 - #58190  secur32:ntlm tests fail on Windows 11 24H2
 - #58196  d3d9:device WM_WINDOWPOSCHANGED test fails consistently on Linux since Wine 10.5
 - #58233  gitlab-ci shows occasional crashes in tests dmsynth, dmusic, winmm:midi
 - #58246  HP Prime Virtual Calculator: crashes on start
 - #58253  Multiple games have a beeping noise on exit (The Fidelio Incident, Vampyr)
 - #58259  Qt Installer for Windows doesn't work
 - #58275  Wrong "range" selection logic using SHIFT
 - #58286  Cannot create a 64-bit wineprefix with old wow64
 - #58293  Regression caused by switch to realloc which doesn't zero added memory
 - #58328  "msvcrt: Use RVAs in rtti and exception data on all platforms except i386" breaks RTTI on arm32
 - #58329  Smartsuite 3.1 installer crashes
 - #58340  dbghelp: symt_add_func_line, possible use after free.
 - #58349  Build failure with clang for x86_64 target due to recent RTTI changes

### Changes since 10.9:
```
Alexandre Julliard (29):
      configure: Always check for a valid 64-bit libdir.
      configure: Don't use the 64-bit tools if --enable-tools is specified.
      configure: Cache the results of the MTLDevice check.
      winebuild: Extend the -syscall flag to allow specifying the syscall number.
      tools: Support explicit syscall numbers in make_specfiles.
      ntdll: Add explicit ids to a number of syscalls.
      msvcrt: Add macros to wrap RTTI initialization functions.
      msvcrt: Always define full type data for exception types.
      msvcrt: Use RVAs for exception data but not RTTI on 32-bit ARM.
      msvcrt: Use varargs macros to define C++ exception types.
      msvcrt: Always define full RTTI data.
      msvcrt: Use varargs macros to define RTTI data.
      msvcrt/tests: Simplify the RTTI macros.
      kernelbase: Flesh out RaiseFailFastException() implementation.
      msvcrt: Define C++ type info as an array to avoid & operator.
      msvcrt: Define C++ type info using RVAs for PE builds.
      msvcrt: Define RTTI base descriptor as an array to avoid & operator.
      msvcrt: Define RTTI data using RVAs for PE builds.
      faudio: Import upstream release 25.06.
      mpg123: Import upstream release 1.33.0.
      png: Import upstream release 1.6.48.
      xslt: Import upstream release 1.1.43.
      nls: Update locale data to CLDR version 47.
      msvcrt: Use old-style gcc varargs macros for RTTI data.
      ntdll: Only clear 64K of the initial thread stack.
      ntdll: Handle FileNetworkOpenInformation directly in fill_file_info().
      ntdll: Use ObjectNameInformation to retrieve file name for FileNameInformation.
      msvcrt: Point to the correct type info entry in the PE build macros.
      server: Return the correct status depending on object type for ObjectNameInformation.

Alexandros Frantzis (2):
      winewayland: Improve cleanup of text-input pending state.
      winewayland: Ignore text-input "done" events that don't modify state.

Aurimas Fišeras (1):
      po: Update Lithuanian translation.

Bernhard Übelacker (8):
      taskschd/tests: Fix test failure on Windows 7.
      shell32/tests: Avoid hang in test_rename.
      d3d10: Return S_OK from parse_fx10_annotations instead of variable hr.
      mmdevapi: Avoid race between main and notify thread.
      d2d1/tests: Skip tests when device creation fails.
      dbghelp: Avoid use after free by moving assignment before vector_add (ASan).
      kernel32/tests: Don't call pNtCompareObjects in Windows 8.
      kernel32/tests: Re-enable console handle test in Windows 8 with broken.

Brendan McGrath (4):
      mfplat/tests: Add image size tests for P010.
      mfplat: Add support for the P010 format.
      winegstreamer: Add support for the P010 format.
      winedmo: Add support for the P010 format.

Brendan Shanks (3):
      winemac: Remove additional pre-macOS 10.12 workarounds.
      configure: Remove unnecessary METAL_LIBS variable.
      winemac: Remove #defines for conflicting macOS function names.

Daniel Lehman (2):
      msvcr120/tests: Clear math error callback after test.
      musl: Set EDOM in exp for NAN.

Dmitry Timoshkov (6):
      kerberos: Add support for SECBUFFER_STREAM to SpUnsealMessage().
      ldap: Avoid code duplication between sasl_client_start() and sasl_client_step().
      Revert "combase: Find correct apartment in ClientRpcChannelBuffer_SendReceive()."
      msv1_0: Perform NULL check before looking for a buffer of particular type.
      kerberos: EncryptMessage() should fail if context doesn't support confidentiality.
      kerberos: When requested confidentiality InitializeSecurityContext() should also add integrity.

Elizabeth Figura (11):
      dxgi: Do not print a FIXME for DXGI_PRESENT_ALLOW_TEARING.
      amstream/tests: Test reconnection done by CreateSample().
      amstream: Don't bother calling SetFormat() if a NULL surface was passed.
      amstream: Call SetFormat() before creating the sample.
      amstream: Unblock GetBuffer() in Decommit().
      amstream: Add more traces.
      wined3d/spirv: Limit parameters to the relevant shader versions.
      wined3d/vk: Use the XY11 fixup for SNORM formats only for legacy d3d.
      amstream/tests: Avoid creating a ddraw RGB24 surface.
      amstream/tests: Avoid calling GetAllocatorRequirements() from DecideAllocator().
      d3d11: Implement CreateRasterizerState2().

Eric Pouech (5):
      dbghelp: Speed up global symbols at startup.
      dbghelp: Get fpo stream information directly in new PDB reader.
      dbghelp: Simplify signature of PDB unwinders.
      dbghelp: Rename declarations for old PBD backend.
      dbghelp: Let new PDB reader exist independently of the old one.

Esme Povirk (1):
      mscoree: Update Wine Mono to 10.1.0.

Gabriel Ivăncescu (4):
      mshtml/tests: Test mixed charset encodings for document and text resources.
      mshtml: Try to guess the script encoding when there's no BOM.
      mshtml: Remove outdated FIXME comment.
      urlmon: Skip fragment part when checking filenames for file protocol.

Georg Lehmann (1):
      winevulkan: Update to VK spec version 1.4.318.

Hans Leidekker (21):
      widl: Add typeref rows for enums.
      widl: Add typedef, field and constant rows for enums.
      widl: Add rows for the flags attribute.
      widl: Add rows for the contract attribute.
      widl: Add rows for the version attribute.
      msv1_0: Pass a SecBuffer to create_signature() instead of an index.
      msv1_0: Pass a SecBuffer to verify_signature() instead of an index.
      msv1_0: Support SECBUFFER_STREAM in ntlm_SpUnsealMessage().
      kerberos: Fix the wow64 thunk for unseal_message().
      kerberos: Avoid buffer copy in kerberos_SpUnsealMessage().
      ldap: Use SECBUFFER_STREAM in sasl_decode().
      widl: Add rows for the apicontract type.
      widl: Add rows for the contractversion attribute.
      widl: Add rows for the apicontract attribute.
      widl: Add rows for the struct type.
      widl: Add rows for the interface type.
      widl: Add rows for the uuid attribute.
      widl: Add rows for the exclusiveto attribute.
      widl: Add rows for the requires keyword.
      find: Support /c switch.
      secur32/tests: Fix test failure on Windows 11.

Jacek Caban (3):
      winegcc: Allow specifying multiple debug files.
      makedep: Introduce output_debug_files helper.
      configure: Support generating both DWARF and PDB debug info in a single build.

Jacob Czekalla (8):
      taskschd/tests: Adds a test for IRegisteredTaskCollection get_Item and get_Count.
      taskschd: Implements IRegisteredTaskCollection get_Item().
      taskschd: Implements IRegisteredTaskCollection get_Count().
      comdlg32/tests: Add tests for changing devmode properties in the hook procedure for PrintDlgW.
      comdlg32: Don't use a shadow devmode structure in PrintDlgW.
      comdlg32/tests: Add tests for changing devmode properties in the hook procedure for PrintDlgA.
      comdlg32: Don't use a shadow devmode structure in PrintDlgA.
      comdlg32/tests: Remove tests in printer_properties_hook_proc for A and W versions.

Joe Souza (2):
      include: Fix mistitled field in CONSOLE_READCONSOLE_CONTROL.
      cmd: Implement tab completion for command line entry.

Lorenzo Ferrillo (1):
      shell32: Create an internal IDropSource in SHDoDragDrop if it wasn't passed by the caller.

Matteo Bruni (1):
      d3dcompiler/tests: Avoid precision issues in the trigonometry tests.

Nikolay Sivov (3):
      d3d10/tests: Check d3d buffer sizes in effects tests.
      include: Add GetCurrentThreadStackLimits() prototype.
      d3dx9/tests: Add a few tests for technique/pass access with the effect compiler API.

Paul Gofman (11):
      crypt32: Duplicate provided root store in CRYPT_CreateChainEngine().
      msvcrt: Print FIXME when WideCharToMultiByte() fails in create_locinfo().
      msvcrt: Support j modifier in scanf.
      wbemprox: Pass current directory correctly to CreateProcessW() in process_create().
      kernel32/tests: Test loading dll as resource or datafile with wow64 FS redirection disabled.
      version/tests: Test GetFileVersionInfoW() with wow64 FS redirection.
      msi/tests: Test installing 64 bit library loaded into wow64 installer process.
      msi: Allocate buffer in msi_get_file_version_info().
      msi: Get system directory just once.
      msi: Fix getting version info for library loaded into wow64 process.
      xaudio2/tests: Add some tests for XAudio2 refcounting.

Piotr Caban (3):
      ucrtbase: Fix CP_UTF8 handling in _toupper_l.
      ucrtbase: Fix CP_UTF8 handling in _tolower_l.
      ucrtbase: Fix case mapping and ctype1 tables for utf8 locale.

Ralf Habacker (1):
      ws2_32: Keep parameters in traces for bind() synchronized with connect().

Rémi Bernon (52):
      opengl32: Simplify wglMake(Context)Current control flow.
      opengl32: Cache context creation attributes.
      opengl32: Use cached attributes to detect legacy contexts.
      opengl32: Generate args loading / locking for wrapped functions.
      opengl32/tests: Test wglShareLists with modified contexts.
      opengl32: Track current context attributes changes.
      opengl32: Track a subset of the context attributes.
      opengl32: Implement wglCopyContext with tracked attributes.
      opengl32/tests: Add more tests with wglCopyContext after usage.
      win32u: Drop now unused wglCopyContext entry points.
      opengl32: Implement wglShareLists by copying after recreation.
      win32u: Drop now unused wglShareLists entry points.
      win32u: Avoid closing NULL egl_handle on dlopen failure.
      win32u: Return hwnd from get_full_window_handle when invalid.
      ole32/tests: Add an test with implicit MTA creation.
      ole32/tests: Add more tests with RPC from the wrong thread.
      ole32/tests: Check calling a proxy after re-creating the STA.
      winex11: Flag offscreen formats as bitmap compatible if possible.
      winex11: Check XVisualInfo vs GLXFBConfig depth to avoid BadMatch.
      opengl32: Expose every pixel format on memory DCs.
      opengl32/tests: Relax memory DC pixel format selection.
      windows.devices.enumeration: Return S_OK from IDeviceWatcher::Start with unsupported filter.
      win32u: Remove unnecessary window_entry member.
      winex11: Remove unnecessary gl3_context context member.
      winex11: Remove old window drawable lookup and check.
      server: Fix shared object offset when additional blocks are allocated.
      win32u: Remove unnecessary shared object lock reset.
      server: Allocate shared memory objects for windows.
      server: Move window dpi_context to the shared memory.
      win32u: Pass id and offset separately to find_shared_session_object.
      win32u: Read window dpi_context from the shared memory.
      opengl32: Move glFlush / glFinish hooking from win32u.
      winex11: Use opengl_funcs for glFlush / glFinish.
      winemac: Use opengl_funcs for glFlush.
      win32u: Pass the window instance to create_window request.
      server: Introduce a dedicated init_window_info request.
      server: Get rid of set_window_info request flags.
      win32u: Use a separate variable for windows free_list iteration.
      win32u: Use a dedicated struct for window destroy entries.
      win32u: Fix copy of vulkan surfaces list on window destruction.
      opengl32: Flush the contexts on gl(Draw|Read)Pixels and glViewport.
      win32u: Use a pbuffer to implement GL on memory DCs.
      win32u: Drop now unnecessary OSMesa dependency.
      win32u: Remove now unnecessary context and pbuffer funcs.
      widl: Write apicontracts macros before they are used.
      widl: Skip writing type definition if already written.
      widl: Always write WinRT enum type definitions.
      win32u: Implement NtUserRegisterWindowMessage.
      win32u/tests: Test that user atoms are not a global atoms.
      win32u/tests: Test some pinned global and user atoms.
      user32: Implement RegisterWindowMessage(A|W) using NtUserRegisterWindowMessage.
      user32: Implement RegisterClipboardFormat(A|W) using NtUserRegisterWindowMessage.

Stefan Dösinger (2):
      d3d9/tests: Relax the focus==device window reactivate WM_WINDOWPOSCHANGED test.
      d3d9/tests: Make the reactivate_messages_filtered test more meaningful on Windows.

Tim Clem (3):
      gitlab: Switch to a Sequoia-based VM for the macOS CI build.
      vcruntime140_1: Add a version resource.
      msvcp140_2: Add a version resource.

Topi-Matti Ritala (1):
      po: Update Finnish translation.

Vibhav Pant (6):
      bluetoothapis: Return the correct error value in BluetoothAuthenticateDeviceEx.
      include: Add windows.networking.sockets.idl.
      include: Add windows.devices.bluetooth.rfcomm.idl.
      include: Add definitions for Windows.Devices.Bluetooth.BluetoothDevice.
      windows.devices.bluetooth/tests: Add tests for BluetoothDeviceStatics.
      windows.devices.bluetooth: Add stub for IBluetoothDeviceStatics.

Vitaly Lipatov (1):
      wine.inf: Add subkey Parameters for Tcpip6 and Dnscache services.

Yuxuan Shui (24):
      amstream: Remove sample from update queue when releasing it.
      msctf: Fix read of invalid memory in SINK_FOR_EACH.
      cmd: Fix out-of-bound access when parsing commands that start with >.
      cmd: Don't check for quotes past the start of the command.
      hid/tests: Fix stack use-after-return in test_read_device.
      gdiplus/tests: Use correctly formatted description.
      gdiplus/tests: Make sure stride is correct for GdipCreateBitmapFromScan0.
      inetmib1: Fix table emptiness check.
      iphlpapi: Fix use-after-free of apc context.
      kernel32/tests: Cancel IO before returning from function.
      server: Fix leak of object name in device_open_file.
      d3dx9_36/tests: Use correct row pitch for D3DXLoadSurfaceFromMemory.
      kernel32/tests: Fix the string argument to WritePrivateProfileSectionA.
      kernelbase/tests: Make sure buffer is big enough for test paths.
      mshtml: Fix buffer underflow in range_to_string.
      d3dx9: Set shared parameters pointer to NULL when freeing it.
      mfreadwrite/tests: Fix missing terminator in attribute_desc.
      nsiproxy.sys: Fix size mismatch writing into ipstatlist.
      kernel32/tests: Fix out-of-bound read in test_CreateFileA.
      msi/tests: Fix out-of-bound memcmp.
      oleaut32: Fix out-of-bound read in OleLoadPicturePath when szURLorPath is empty.
      propsys/tests: Use correct length for debugstr_wn in test_PropVariantToBSTR.
      setupapi/tests: Fix double close of HSPFILEQ.
      shell32/tests: Make sure to use the right type of strings.

Zhiyi Zhang (14):
      d3d11/tests: Test d3d11 device context interfaces.
      d3d11: Add ID3D11DeviceContext2 stub.
      d3d11: Add ID3D11DeviceContext3 stub.
      d3d11: Add ID3D11DeviceContext4 stub.
      kernelbase: Silence a noisy FIXME in AppPolicyGetWindowingModel().
      comctl32/taskdialog: Fix layout when there is no button.
      ntdll: Implement RtlQueryInformationActiveActivationContext().
      include: Add errhandlingapi.h.
      combase: Add RoFailFastWithErrorContextInternal2() stub.
      combase: Add RoGetErrorReportingFlags() stub.
      combase: Add RoReportUnhandledError() stub.
      combase: Add an error message when class is not found.
      d3dcompiler_47: Implement D3DCreateLinker().
      d3dcompiler_47: Implement D3DCreateFunctionLinkingGraph().
```
