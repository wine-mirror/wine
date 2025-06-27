The Wine development release 10.11 is now available.

What's new in this release:
  - Preparation work for NTSync support.
  - More support for generating Windows Runtime metadata in WIDL.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/10.x/wine-10.11.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-10.11/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 10.11 (total 25):

 - #31212  Some VST instruments crash when reloaded in Mixcraft
 - #37131  Clang Static Analyzer:  Division by zero
 - #42033  Fallout 3: Radio music not playing
 - #50278  Diggles: The Myth of Fenris (GOG version) crashes on launch
 - #50577  Saya no Uta: hangs on RtlpWaitForCriticalSection
 - #55019  kernel32:process - Accents cause test_Environment() to fail on Windows
 - #56086  C&C Generals Zero Hour has graphic errors in menu
 - #56128  Genshin Impact: after changing to another window and back, input does not work anymore
 - #56517  osu!: Does not launch since 9.3
 - #57020  Anritsu Software Toolbox doesn't install properly
 - #57656  CryptMsgGetParam() with CMSG_SIGNER_AUTH_ATTR_PARAM/CMSG_SIGNER_UNAUTH_ATTR_PARAM returns success with 0 buffer size
 - #57802  WordPro's "view Settings" not saving properly
 - #58321  Purple Place exits
 - #58343  Multiple games have rendering errors after d0fd9e87c (Kathy Rain 2, Among Us, Green Hell)
 - #58344  Magic The Gathering Arena: Black screen in wine-10.9
 - #58345  Far File Manager 3 x86-64's product features during installation cannot be configured/are missing
 - #58356  Doom I & II Enhanced (2019 re-release based on Unity engine) crashes after the intro videos
 - #58363  Thief II crashes
 - #58364  Pegasus Email draws incorrectly
 - #58372  EZNEC pro2+ 7.0 runs, but calculations have errornous exponential values
 - #58373  Bejeweled 3 runs but the screen is black
 - #58381  "musl: Use __builtin_rint if available" breaks clang builds (except aarch64)
 - #58384  Sid Meier's Civilization III becomes unresponsive
 - #58402  Sid Meier's Civilization III: severe discoloration
 - #58412  winedbg recursively forks until the memory is exhausted

### Changes since 10.10:
```
Alexandre Julliard (36):
      winebuild: Refuse to do non-PE builds on platforms that don't support it.
      winegcc: Refuse to do non-PE builds on platforms that don't support it.
      ntdll: Use NtOpenFile to open nls files in the system directory.
      ntdll: Use UNICODE_STRINGs in the main image loading helpers.
      ntdll: Pass the full image NT path through the server startup information.
      ntdll: Move resolving the initial image name to the get_full_path() helper.
      ntdll: Add a helper to return both NT and Unix names to open a file.
      ntdll: Make get_redirect() static.
      ntdll: Try to build a proper NT name when opening files with \??\unix.
      server: Return the NT file name in ObjectNameInformation for file objects.
      winecrt0: Add a default implementation for DllGetVersion().
      resources: Generate version strings from the corresponding version number.
      mountmgr: Use the \\?\unix prefix to open device files.
      appwiz.cpl: Store the registry key name in Unicode.
      appwiz.cpl: Use GetFinalPathNameByHandleW to get the DOS path of the package to install.
      user32: Add a macro to define the list of user callbacks.
      mscoree: Use GetFinalPathNameByHandleW to get the DOS path of the Mono directory.
      mshtml: Use GetFinalPathNameByHandleW to get the DOS path of the Gecko directory.
      start: Use GetFinalPathNameByHandleW to get the DOS path of a Unix file.
      kernelbase: Convert slashes in Unix paths in GetFinalPathNameByHandleW.
      wineps.drv: Use \\?\unix paths to load the AFM files.
      winemenubuilder: Use \\?\unix paths to load the link files.
      kernel32: Reimplement conversion to DOS name using GetFinalPathNameByHandleW.
      ntdll: Reimplement the RtlGetFullPathName_U Unix path heuristic using ObjectNameInformation.
      ntdll: Restrict some Unixlib helpers to Wine internal usage.
      ntdll: Add a private helper to retrieve a DOS file name.
      ntdll: Return NT paths in the get build/data dir helpers.
      ntdll: Fix get_core_id_regs_arm64() prototype for non-Linux platforms.
      ntdll: Pass the Unix prot flags to the map_fixed_area() helper.
      ntdll: Don't set VPROT_WRITEWATCH flag on pages when using kernel write watches.
      win32u: Add a helper to convert file names to NT format consistently.
      ntdll: Only reset the reported write watch range in NtGetWriteWatch.
      ntdll: Correctly report execute faults on ARM64.
      ntdll: Correctly report execute faults on ARM.
      makefiles: Support specifying the PE architecture as "none".
      makefiles: Fix program installation for Windows builds.

Bernhard Kölbl (1):
      mfmediaengine: Enable XVP for playback topology.

Bernhard Übelacker (10):
      msvcrt/tests: Add broken to new j modifier tests.
      oleaut32: In VarFormat do not count '#' in exponent into fractional digits.
      msi: Use LRESULT to store return value from CallWindowProcW.
      winetest: Fail only if output_size exceeds the limit.
      gitlab: Derive the windows tests from a common .wine-test-windows.
      gitlab: Remove name containing CI environment variables in windows tests.
      comctl32: Use LRESULT to store return value from CallWindowProcW.
      comdlg32: Use LRESULT/INT_PTR to store return value from CallWindowProcA.
      winetest: Use LRESULT to return value from CallWindowProcA.
      gitlab: Remove other user controlled CI environment variables.

Bradan Fleming (1):
      winemenubuilder: Quote Exec arguments in desktop entries.

Brendan McGrath (3):
      mfmediaengine: Only forward the most recent seek time.
      mfreadwrite/tests: Check DEFAULT_STRIDE is not always present.
      mfreadwrite: Fix media type output when video processor is used.

Charlotte Pabst (9):
      mf: Clear pending MFT stream requests when flushing.
      Revert "mf: Release pending items when sample grabber is stopped.".
      ntdll: Treat Rbp as CONTEXT_INTEGER register.
      mf/tests: Rename test_source to test_stub_source.
      mf/tests: Rename test_seek_source to test_source.
      mf/tests: Move some functions.
      mf/tests: Add tests for thinning.
      mf: Don't forward thinning to clock.
      mf: Handle thinning in media session.

Connor McAdams (4):
      comctl32/tests: Add tests for iImage value for listview subitems.
      comctl32/listview: Don't touch iImage value for subitems if LVS_EX_SUBITEMIMAGES is not set.
      comctl32/tests: Add item state value tests for LVS_OWNERDATA controls.
      comctl32/listview: Properly handle item state value for LVS_OWNERDATA controls.

Conor McCarthy (4):
      rtworkq/tests: Test closing a timer or event handle after submission.
      ntdll/tests: Test early closure of handles used for threadpool waits.
      ntdll: Initialise waitable handles with NULL.
      ntdll: Duplicate handles for thread pool waits.

Dylan Donnell (1):
      kernelbase: Allocate a new buffer for the module name in LoadLibraryExA.

Elizabeth Figura (34):
      qasf/tests: Test AllocateStreamingResources()/FreeStreamingResources() calls.
      qasf/dmowrapper: Fail Pause() if there is no DMO.
      qasf/dmowrapper: Call AllocateStreamingResources() and FreeStreamingResources().
      qasf/dmowrapper: Handle a NULL output buffer in GetBufferAndLength().
      ir50_32/tests: Add tests.
      ir50_32/tests: Move compression and decompression tests from mf:transform.
      ir50_32: Use case-insensitive comparison for the compression fourcc.
      ir50_32: Do not handle a NULL input pointer in ICM_DECOMPRESS_GET_FORMAT.
      ir50_32: Return ICERR_OK from ICM_DECOMPRESS_END.
      ir50_32: Suggest 24-bit RGB.
      ir50_32: Do not validate biPlanes.
      ir50_32: Explicitly fill the whole BITMAPINFOHEADER in ICM_DECOMPRESS_GET_FORMAT.
      ir50_32: Fix the return value of ICM_DECOMPRESS_GET_FORMAT.
      ir50_32: Fix the error value for mismatching dimensions.
      ir50_32: Support decoding to RGB565.
      qasf/tests: Test dynamic format change on the DMO wrapper.
      qasf/dmowrapper: Delay SetActualDataLength().
      qasf/dmowrapper: Handle dynamic format change.
      quartz/tests: Port test_source_allocator() to avidec.
      quartz/avidec: Don't set the data length to 0.
      quartz/tests: Test dynamic format change on the AVI decoder.
      quartz: Add a copy_bitmap_header() helper.
      quartz/avidec: Correctly calculate the BITMAPINFOHEADER size for BI_BITFIELDS.
      quartz/avidec: Handle dynamic format change.
      winegstreamer: Support the Indeo 5.0 format in DirectShow.
      server: Use an event sync for thread objects.
      server: Use an event sync for job objects.
      server: Use an event sync for process objects.
      server: Use an event sync for debug objects.
      server: Use an event sync for device manager objects.
      server: Use an event sync for completion port objects.
      server: Use an event sync for timer objects.
      server: Use an event sync for console objects.
      server: Use an event sync for console server objects.

Esme Povirk (10):
      gdiplus: Account for gdi32 clipping in GdipFillRegion.
      gdiplus: Don't clip the HRGN passed to alpha_blend_pixels_hrgn.
      comctl32: Implement MSAA events for updown controls.
      gdiplus/tests: Region hit-testing is done in device coordinates.
      gdiplus: Use graphics transform in GdipIsVisibleRegionPoint.
      gdiplus: Do not create HRGN in GdipIsVisibleRegionPoint.
      comctl32: Implement OBJID_QUERYCLASSNAMEIDX for list boxes.
      comctl32: Implement OBJID_QUERYCLASSNAMEIDX for static controls.
      comctl32: Implement OBJID_QUERYCLASSNAMEIDX for edit controls.
      comctl32: Implement OBJID_QUERYCLASSNAMEIDX for combo boxes.

Hans Leidekker (19):
      widl: Add rows for the delegate type.
      widl: Handle NULL type name.
      widl: Add rows for propget methods.
      widl: Add rows for propput methods.
      widl: Add rows for eventadd methods.
      widl: Add rows for eventremove methods.
      widl: Add rows for regular methods.
      widl: Use a define for maximum name length.
      widl: Add rows for the overload attribute.
      widl: Add rows for the default_overload attribute.
      widl: Add rows for the deprecated attribute.
      widl: Correct element type for interface signature.
      widl: Don't sort the property and event tables.
      widl: Add a helper to build the method name.
      widl: Add a helper to retrieve method attributes and flags.
      widl: Store EventRegistrationToken reference in the real type.
      widl: Add separate property rows for interfaces and classes.
      widl: Add separate event rows for interfaces and classes.
      widl: Add rows for the runtimeclass type.

Ignacy Kuchciński (4):
      user32: Add GetPointerPenInfo stub.
      user32: Add GetPointerDeviceProperties stub.
      user32: Add GetPointerDeviceRects stub.
      user32: Add GetRawPointerDeviceData stub.

Julius Bettin (1):
      kernelbase: Implement HeapSummary.

Nikolay Sivov (9):
      bluetoothapis: Fix typo in a format string (Coverity).
      version/tests: Fix a typo (Coverity).
      amstream/tests: Add a few return value checks (Coverity).
      widl: Always use NdrClientCall2() for interpreted stubs.
      widl: Do not write "const" modifiers for _PARAM_STRUCT fields.
      mfmediaengine: Simplify state -> event mapping.
      kernel32/tests: Use ViewShare value instead of a literal constant.
      ntdll/tests: Add a test for automatically resizing a mapped disk file.
      ntdll/tests: Tweak mapped file test to better match actual use case.

Paul Gofman (4):
      avifil32/tests: Test creating AVI file with OF_CREATE but without access mode.
      avifil32: Assume OF_WRITE for OF_CREATE in AVIFileOpenW().
      avifil32: Fix dwLength counting for fixed size samples.
      user32: Reserve more space in the kernel callback table.

Piotr Caban (10):
      musl: Reimplement rint so it doesn't depend on floating point operations precision.
      musl: Use __builtin_rint if available.
      msvcrt: Use rint() from the bundled musl library.
      msvcrt: Fix allocated buffer size in _getcwd.
      include: Add errlup.idl.
      musl: Don't use __builtin_rint in clang builds.
      musl: Optimize rint when floating point operations use 53-bit precision.
      msvcrt: Fix allocated buffer size in _getdcwd.
      ucrtbase/tests: Cleanup temporary files in _sopen_s tests.
      msvcrt: Support _SH_SECURE in _wsopen_dispatch().

Rémi Bernon (80):
      winex11: Update GL drawable offscreen status instead of recreating.
      winex11: Update every window GL drawable on resize / reparent.
      winex11: Update drawable size and offscreen when presenting.
      winex11: Drop pixmap-based child window workaround.
      winex11: Rename context drawables to draw / read.
      winex11: Remove unnecessary glx_pixel_format pointers.
      winex11: Remove unnecessary hdc context member.
      win32u: Introduce an opengl_drawable base struct.
      win32u: Return an opengl_drawable from pbuffer_create.
      wineandroid: Add a refcount to struct gl_drawable.
      win32u: Add a refcount to struct opengl_drawable.
      include: Add APP_LOCAL_DEVICE_ID definition.
      opengl32: Hook and flush context on glClear.
      winewayland: Update the drawable size on context_flush.
      winex11: Use a separate drawable vtable for pbuffers.
      win32u: Use the drawable vtable to destroy pbuffers.
      win32u: Allocate GL drawables on behalf of the drivers.
      winewayland: Switch client surfaces when presenting.
      win32u: Keep a reference to the GL drawables in the windows.
      win32u: Keep a reference to the pbuffer drawables in the DCs.
      win32u: Track and update opengl drawables in the contexts.
      wineandroid: Remove now unnecessary context sync.
      winemac: Remove now unnecessary context sync.
      winewayland: Remove now unnecessary context sync.
      win32u: Avoid reading GL data past the end of the memory DC bitmap.
      server: Move object grab/release out of (add|remove)_queue.
      server: Add an operation to retrieve an object sync.
      server: Redirect fd-based objects sync to the fd.
      server: Introduce a new event sync object.
      server: Use an event sync for fd objects.
      user32/tests: Cleanup window class versioning tests.
      user32/tests: Test window class versioned name with integer atom.
      user32/tests: Test that window class atom cannot be changed.
      win32u/tests: Test window class name with integer atom.
      win32u: Use the right pointer when destroying window.
      user32/tests: Call flush_event after SetForegroundWindow calls.
      server: Use an event sync for thread apc objects.
      server: Use an event sync for context objects.
      server: Use an event sync for startup info objects.
      ntdll/tests: Link atom functions directly.
      ntdll/tests: Add more integral atom tests.
      ntdll: Set returned atom to 0 when we should.
      ntdll: Allow deleting integral atoms from tables.
      user32: Clamp atom to MAXINTATOM in get_int_atom_value.
      win32u: Clamp atom to MAXINTATOM in get_int_atom_value.
      winemac: Drop the SkipSingleBufferFlushes option.
      win32u: Use the drawable vtable for flush and swap.
      win32u: Add a flags parameter to opengl_drawable flush.
      win32u: Pass opengl_drawable pointers to make_current.
      win32u: Use the DC opengl drawable for the memory DC surface.
      win32u: Keep track of the most recent window GL drawable.
      opengl32: Ignore RGB565 pixel formats with memory DCs.
      server: Use an event sync for file lock objects.
      server: Use an event sync for debug event objects.
      server: Use a static array for atom table atoms.
      server: Use a count instead of last atom index.
      server: Forbid using string atom 0xc000.
      server: Use a static array for atom table hash.
      server: Keep computed atom hash in local variables.
      server: Remove unused atom pinned member.
      winex11: Avoid requesting CWStackMode alone with managed windows.
      server: Use the console sync for screen buffers objects.
      server: Use the console sync for console input objects.
      server: Use the console sync for console output objects.
      server: Get rid of the console signaled flag.
      server: Create a global atom table on startup.
      server: Remove now unnecessary global table checks.
      server: Pass atom table parameter to atom functions.
      server: Introduce a new get_user_atom_name request.
      server: Introduce a new add_user_atom request.
      server: Move some checks inside of mutex do_release.
      server: Split mutex to a dedicated sync object.
      server: Split semaphore to a dedicated sync object.
      server: Use a flag to keep track of message queue waits.
      server: Use a signaled flag for message queue sync.
      win32u/tests: Test that window properties are global atoms.
      user32: Implement GetClipboardFormatNameA with NtUserGetClipboardFormatName.
      win32u: Implement NtUserGetClipboardFormatName using NtUserGetAtomName.
      server: Create and use a user atom table for class names.
      server: Initialize global and user tables with some atoms.

Santino Mazza (1):
      winex11.drv: Add programmer dvorak layout.

Vibhav Pant (17):
      include: Add definitions for IBluetoothDeviceId, BluetoothError.
      include: Add windows.devices.bluetooth.genericattributeprofile.idl.
      include: Add definitions for IBluetoothLEDevice.
      include: Add definitions for IGattDeviceService2.
      windows.devices.bluetooth/tests: Add tests for BluetoothLEDeviceStatics.
      windows.devices.bluetooth: Add stubs for BluetoothLEDeviceStatics.
      setupapi/tests: Add tests for SetupDi{Set,Get}DeviceInterfacePropertyW.
      setupapi: Implement SetupDiSetDeviceInterfacePropertyW.
      setupapi: Implement SetupDiGetDeviceInterfacePropertyW.
      ntoskrnl.exe/tests: Add tests for SetupDiGetDeviceInterfacePropertyW with enabled interfaces.
      setupapi/tests: Add tests for SetupDiGetDeviceInterfacePropertyKeys.
      setupapi: Implement SetupDiGetDeviceInterfacePropertyKeys.
      setupapi: Implement additional built-in properties in SetupDiGetDeviceInterfacePropertyW.
      setupapi/tests: Add additional tests for device instance properties DEVPKEY_{DeviceInterface_ClassGuid, Device_InstanceId}.
      cfgmgr32: Implement CM_Get_Device_Interface_PropertyW for all property keys.
      cfgmgr32/tests: Add additional tests for CM_Get_Device_Interface_PropertyW.
      setupapi: Return built-in property keys for device interfaces even when the Properties subkey for the interface doesn't exist.

William Horvath (1):
      wow64win: Fix UNICODE_STRING thunking in wow64_NtUserRegisterWindowMessage.

Yuxuan Shui (23):
      msvcirt/tests: Fix use-after-free in test_ifstream.
      msvcirt/tests: Avoid out-of-bound access in test_strstreambuf.
      mshtml: Fix misuse of IWinInetHttpInfo_QueryInfo.
      shell32: Fix use-after-free in ShellView_WndProc.
      shell32/tests: Add missing double null termination in shlfileop.
      kernelbase: Handle short urls in UrlIsA.
      uiautomationcore/tests: Fix missing terminators in some nav_seqs.
      wintrust: Fix data length mix-up in asn decoder.
      gdi32: Fix missing terminator element in Devanagari_consonants.
      xcopy: Fix out-of-bound access when parsing arguments.
      usp10/tests: Avoid out-of-bound use of glyphItems when nGlyphs mismatches.
      rpcrt4/tests: Fix out-of-bound write in test_pointer_marshal.
      wininet: Fix handling of empty strings in urlcache_hash_key.
      wininet: Use BYTE instead of char for hash calculation.
      crypt32: Fix missing size check in CSignedEncodeMsg_Open.
      crypt32: Handle missing attributes in CDecodeSignedMsg_GetParam.
      crypt32/tests: Add signed message CryptMsgGetParam tests with > 1 signers.
      crypt32: Fix creating signed message with > 1 signers.
      gdiplus/tests: Fix out-of-bound use of expected in ok_path_fudge.
      hid/tests: Fix out-of-bound use of nodes in test_device_info.
      crypt32: Don't release context in CSignedEncodeMsg_Open.
      find/tests: Fix out-of-bound access to input in mangle_text.
      kernelbase: Fix array underflow when checking for trailing spaces.

Zhiyi Zhang (10):
      include: Fix dcomp.idl method name and order.
      user32: Add ScheduleDispatchNotification() stub.
      ntdll: Use explicit ACTIVATION_CONTEXT type instead of HANDLE.
      kernel32/tests: Add tests for normal activation context stack frame flags.
      ntdll: Set and check 0x8 flag for activation context stack frames.
      ntdll: Implement RtlActivateActivationContextUnsafeFast().
      ntdll: Implement RtlDeactivateActivationContextUnsafeFast().
      kernel32/tests: Add tests for RtlActivateActivationContextUnsafe() and RtlDeactivateActivationContextUnsafeFast().
      comctl32/tests: Test listview background mix mode.
      comctl32/listview: Set the initial background mix mode to TRANSPARENT.
```
