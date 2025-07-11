The Wine development release 10.12 is now available.

What's new in this release:
  - Optional EGL backend in the X11 driver.
  - Support for Bluetooth Low Energy services.
  - More support for generating Windows Runtime metadata in WIDL.
  - ARM64 builds enabled in Gitlab CI.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/10.x/wine-10.12.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-10.12/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 10.12 (total 17):

 - #19561  Very large memory leak when doing overlapped reads
 - #38337  clang compiling warnings
 - #42882  HDX server software: rs232 port access not working
 - #52720  Speed up cmd.exe:batch in Wine
 - #55776  Some game use video as intro will stock after video is finished
 - #57204  after enable winedmo, Little Witch Nobeta video wont play anymore
 - #57244  Train capacity 300% 2 will crash after enable the winedmo
 - #57489  GetPointerPenInfo not located in USER32.dll when attempting to run Clip Studio Paint 3.0
 - #57790  FindVUK doesn't show drive details
 - #58263  Wine Internet Settings crashes on unimplemented function shdocvw.dll.ParseURLFromOutsideSourceW
 - #58422  Soldier of Fortune II crashes on start
 - #58423  Flickering in games using dxvk/vkd3d-proton on sway
 - #58429  Betfair Poker setup never gets past the splash screen
 - #58435  winepath -w conversion outputs wrong filenames
 - #58436  x11, kotor keeps minimizing
 - #58437  Total Commander 11.55 complains "Access denied" for EVERY file >= 1 MB in "Compare files by content"
 - #58475  Wine Mono corlib tests cause wineserver to assert

### Changes since 10.11:
```
Adam Markowski (1):
      po: Update Polish translation.

Akihiro Sagawa (1):
      po: Update Japanese translation.

Alex Henrie (3):
      ntdll: Don't skip synchronous read when serial read timeout is infinite.
      inetcpl: Restore parse_url_from_outside implementation.
      ntdll: Detect the optical disc type on Linux using a SCSI command.

Alexandre Julliard (16):
      server: Always use the thread Unix id in ptrace().
      rpcrt4: Silence compiler warnings about pointer wraparound.
      gitlab: Build an arm64 image.
      ntdll: Remove duplicate backslashes in get_unix_full_path().
      winevulkan: Skip a couple of unneeded conversion functions.
      gitlab: Add an ARM64 build.
      faudio: Import upstream release 25.07.
      gitlab: Update sarif-converter URL.
      ntdll: Get rid of the wine_unix_to_nt_file_name syscall.
      ntdll: Unify the Unix to NT path conversion helpers.
      gitlab: Add a daily test run for new wow64 mode.
      winetest: Use compiler macros to generate the compiler version info.
      winex11: Disable more wintab code when support is missing.
      oleaut32: Use DOUBLE type where appropriate to match the PSDK.
      include: Use aligned int64 types where possible.
      setupapi/tests: Don't test an uninitialized value.

Alexandros Frantzis (2):
      winewayland: Use system cursor shapes when possible.
      winewayland: Support layered windows with per-pixel alpha.

Alfred Agrell (1):
      include: Enable __uuidof(IUnknown).

Alistair Leslie-Hughes (2):
      shell32: Avoid closing a handle twice.
      msado15: Implement _Connection::get_version.

Anton Baskanov (1):
      quartz: Decommit the allocator before entering the streaming CS in AVIDec.

Arkadiusz Hiler (1):
      cfgmgr32: Preserve registry casing when getting device interface list.

Aurimas Fišeras (1):
      po: Update Lithuanian translation.

Bernhard Übelacker (1):
      dxcore/tests: Skip tests when there is no display available.

Brendan McGrath (3):
      winegstreamer: Remove 'au' alignment for h264.
      winegstreamer: Signal eos on disconnect.
      winegstreamer: Don't hold lock during wg_parser_stream_get_buffer.

Connor McAdams (2):
      d3dx9: Replace txc_fetch_dxtn with bcdec.
      d3dx9: Replace txc_compress_dxtn with stb_dxt.

Daniel Lehman (3):
      ucrtbase/tests: Add tests for expf.
      ucrtbase/tests: Move exp tests from msvcr120.
      include: Add _wcstod_l declaration.

Dmitry Timoshkov (1):
      ntdll: Initialization of XState features should not depend on AVX support.

Elizabeth Figura (5):
      kernel32/tests: Test opening a directory with ReOpenFile().
      kernelbase: Use FILE_NON_DIRECTORY_FILE in ReOpenFile().
      ntdll/tests: Add comprehensive reparse point tests.
      kernel32/tests: Test CreateSymbolicLink().
      xaudio2: Do not register classes from xapofx or x3daudio dlls.

Eric Pouech (7):
      cmd: Introduce struct batch_file to hold information about a .cmd file.
      cmd: Add a cache for the labels lookup.
      cmd/tests: Add tests about .cmd file alteration while executing it.
      widl: Fix segfault when inheriting from an incomplete interface.
      widl: Remove unneeded condition.
      widl: Ensure inherited interface is declared before using it.
      include: Remove duplicated declarations in .idl files.

Esme Povirk (3):
      comctl32: Implement OBJID_QUERYCLASSNAMEIDX for status controls.
      comctl32: Implement OBJID_QUERYCLASSNAMEIDX for toolbars.
      comctl32: Implement OBJID_QUERYCLASSNAMEIDX for progress bars.

Gabriel Ivăncescu (4):
      mshtml: Clear the document before opening it for writing.
      mshtml: Implement document.linkColor.
      mshtml: Implement document.vLinkColor.
      mshtml: Don't add HTMLDOMNode props to the NodeList.

Hans Leidekker (16):
      widl: Add rows for the static attribute.
      widl: Add TYPE_ATTR_ABSTRACT flag if a runtime class has no member interfaces.
      widl: Add rows for the default attribute.
      widl: Add rows for the activatable attribute.
      widl: Add rows for the threading attribute.
      widl: Add rows for the marshalingbehavior attribute.
      widl: Relax check on runtimeclass definitions.
      widl: Store a variable pointer instead of a declaration in expressions.
      widl: Store the runtimeclass for constructor interfaces.
      widl: Factor out an add_member_interfaces() helper.
      widl: Add rows for static interfaces.
      widl: Include version in activatable attribute value.
      widl: Add rows for activation interfaces.
      widl: Add rows for the composable attribute.
      widl: Add rows for composition interfaces.
      widl: Skip writing metadata if winrt_mode is not set.

Huw D. M. Davies (1):
      kernel32: Use the correct buffer length.

Jacek Caban (8):
      webservices: Don't assume that enum is unsigned in WsInitializeMessage.
      dmime/tests: Use ceil instead of round.
      configure: Build PEs with -ffp-exception-behavior=maytrap.
      d3dcompiler: Don't assume that enum is unsigned in d3dcompiler_get_blob_part.
      d3dcompiler: Handle all D3D_BLOB_PART values in d3dcompiler_get_blob_part validation.
      d3dcompiler: Use __WINE_CRT_PRINTF_ATTR.
      wined3d: Use __WINE_CRT_PRINTF_ATTR.
      widl: Rename metadata stream type enum values to avoid conflict with Cygwin limits.h.

Joe Souza (3):
      cmd: COPY should output file names as they are copied.
      cmd/tests: Add tests to verify COPY command output.
      cmd: Allow '+' as delimiter for tab-completion, e.g. 'copy file+file2'.

Mike Kozelkov (1):
      urlmon: Add PersistentZoneIdentifier COM object stubs.

Nikolay Sivov (2):
      kernel32: Add a stub for CreateBoundaryDescriptorA().
      include: Remove duplicated CreateBoundaryDescriptorW() definition.

Owen Rudge (1):
      mshtml: Implement document.aLinkColor.

Paul Gofman (5):
      user32/tests: Add tests for STARTF_USESHOWWINDOW.
      win32u: Respect STARTF_USESHOWWINDOW in show_window().
      winepulse.drv: Avoid hangs on exit when pulse main loop thread gets killed.
      winebus.sys: Check for udev_device_get_subsystem() error in udev_add_device().
      winebus.sys: Close fd if device is not handled in udev_add_device().

Ryan Houdek (1):
      wineboot: Detect TSC frequency on Arm64.

Rémi Bernon (64):
      winex11: Release the GL drawable on creation failure.
      win32u: Keep window GL drawables in a global linked list.
      win32u: Notify the drivers when GL drawables are detached.
      win32u: Update opengl drawables with window state.
      win32u: Update the window state in more places.
      winewayland: Drop now unnecessary GL drawable tracking.
      winex11: Remove now unnecessary drawable tracking.
      winewayland: Remove unnecessary context config.
      wineandroid: Remove unnecessary context config.
      win32u: Move EGL context creation out of the drivers.
      winewayland: Only detach/attach client surface if it is different.
      opengl32: Do not filter legacy extensions.
      opengl32: Add a helper to read OpenGL registry options.
      opengl32: Add an EnabledExtensions registry option.
      user32: Init class name outside of get_versioned_name.
      user32: Move user32_module default out of get_class_info.
      user32: Load class module in get_class_version.
      user32: Move get_class_version out of get_class_info.
      comctl32: Fix class name case in manifest.
      user32: Always copy class names to a temporary buffer.
      win32u: Add traces to wglGetExtensionsString*.
      win32u: Add an EGLSurface pointer to opengl_drawable struct.
      win32u: Move EGL make_current context function out of the drivers.
      winex11: Remove now unnecessary x11drv_context structure.
      winex11: Add an option to use the new EGL OpenGL backend.
      winemac: Remove now unnecessary pbuffer tracking.
      win32u: Avoid calling detach, flush or swap on pbuffer drawables.
      win32u: Avoid releasing opengl drawable within drawables_lock.
      win32u: Implement generic EGL driver pbuffer surface.
      winewayland: Use a distinct wayland_pbuffer struct for pbuffers.
      win32u: Remove now unnecessary opengl_drawable hdc member.
      win32u: Remove VkResult from surface_presented functions.
      win32u: Introduce a new client_surface struct for vulkan surfaces.
      win32u: Keep client surfaces in a global list.
      win32u: Update client surfaces when window state changes.
      win32u: Present client surfaces through the vtable.
      mf/session: Clarify internal states from session states separation.
      mf/session: Move internal states to a separate command_state enum.
      mf/session: Remove unnecessary SESSION_FLAG_PENDING_COMMAND flag.
      mf/session: Replace SESSION_FLAG_FINALIZE_SINKS with dedicated states.
      mf/session: Replace SESSION_FLAG_END_OF_PRESENTATION with dedicated states.
      mf/session: Introduce a SESSION_FLAG_SOURCE_SHUTDOWN presentation flag.
      mf/session: Simplify the media session shutdown event handling.
      win32u: Fix incorrect length in font extension check.
      winegstreamer: Force h264parse to output 'au' aligned byte-stream.
      mp3dmod: Fix some media type leaks.
      mp3dmod: Avoid uninitialized variable access.
      winegstreamer: Fix some video decoder media type leaks.
      mf/tests: Fix some leaks in transform tests.
      mf/tests: Avoid leaking D3D resources through IMFTrackedSample.
      mf/tests: Skip memory-hungry H264 encoder tests on 32bit.
      mfplat/tests: Add more tests for event queue shutdown.
      mfsrcsnk: Queue an event before shutting down the event queues.
      winegstreamer: Queue an event before shutting down the event queues.
      mf/session: Handle an optional MEError event from sources on shutdown.
      winegstreamer: Remove now unnecessary IMFMediaShutdownNotify.
      mf/session: Remove now unnecessary IMFMediaShutdownNotify.
      include: Remove now unnecessary IMFMediaShutdownNotify interface.
      winewayland: Clear the current client surface on vulkan detach.
      winewayland: Merge the vulkan client surface with wayland_client_surface.
      winex11: Move client surface code out of vulkan ifdef.
      winex11: Create client surfaces for opengl drawables.
      win32u: Replace opengl drawables tracking with client surfaces.
      server: Keep owned mutex syncs alive until abandoned.

Shaun Ren (11):
      sapi/tests: Copy SPVTEXTFRAG list into a contiguous array.
      sapi/tests: Introduce simulate_output option in tts.
      sapi/tests: Add some SSML tests in tts.
      sapi/tts: Support XML-related flags in ISpVoice::Speak.
      sapi/xml: Add a stub SSML parser.
      sapi/xml: Parse the <speak> SSML root element.
      sapi/xml: Implement add_sapi_text_fragement().
      sapi/xml: Parse the <p> and <s> SSML elements.
      sapi/xml: Parse the rate attribute in the SSML <prosody> element.
      sapi/xml: Parse the volume attribute in the <prosody> SSML element.
      sapi/xml: Parse the pitch attribute in the <prosody> SSML element.

Vibhav Pant (17):
      include: Add devquerydef.h.
      include: Add devfiltertypes.h.
      include: Add devquery.h.
      cfgmgr32/tests: Add tests for DevGetObjects.
      cfgmgr32: Add a basic implementation for DevGetObjects(Ex) for device interface objects.
      ntoskrnl.exe/tests: Add tests for Io{Get,Set}DeviceInterfacePropertyData.
      ntoskrnl.exe: Implement IoGetDeviceInterfacePropertyData.
      ntoskrnl.exe: Implement IoSetDeviceInterfacePropertyData.
      cfgmgr32/tests: Add tests for fetching specific properties in DevGetObjects.
      cfgmgr32: Stub DevCreateObjectQuery(Ex) functions.
      cfgmgr32: Implement initial device enumeration for DevCreateObjectQuery.
      cfgmgr32: Support fetching properties for device objects in Dev{GetObjects, CreateObjectQueryEx}.
      winebth.sys: Create PDOs for remote Bluetooth devices.
      winebth.sys: Store a list of GATT services discovered on LE devices.
      winebth.sys: Implement IOCTL_WINEBTH_LE_DEVICE_GET_GATT_SERVICES.
      bluetoothapis: Implement BluetoothGATTGetServices.
      bluetoothapis/tests: Add tests for BluetoothGATTGetServices.

Yuxuan Shui (11):
      mscoree/tests: Fix string lengths passed to RegSetKeyValueA.
      msvcp90: Fix vector_base_v4 allocation sizes.
      msvcp90: Fix calculation of segment addresses in vector.
      msvcrt: Don't release io memory during process shutdown.
      wined3d: Fix missing data/resource type bounds check.
      dxgi/tests: Fix out-of-bound in test_cursor_clipping.
      ddraw/tests: Release device after ddraw.
      d3dx9/tests: Fix volume test boxes.
      wined3d: Fix double-free when shader_set_function fails.
      quartz/tests: Fix out-of-bound use of video_types in test_connect_pin.
      dsound: Make sure to null-terminate strings for callbacks.

Zhiyi Zhang (10):
      comctl32/listview: Test WM_PAINT with a subclassed header that paints without validating update regions.
      comctl32/listview: Validate header region after painting it.
      bcp47langs: Add GetFontFallbackLanguageList() stub.
      gdiplus/tests: Test drawing indexed bitmaps with a palette that has alpha.
      gdiplus: Fix drawing indexed bitmaps with a palette that has alpha.
      gdiplus: Don't zero out allocated memory in GdipAlloc().
      win32u: Limit the work area to the monitor rect.
      user32/tests: Add some tests for DefWindowProc() WM_PRINT message handling.
      win32u: Fix the return value for WM_PRINT handling in DefWindowProc().
      win32u: Support PRF_CHILDREN when handling WM_PRINT for DefWindowProc().

Ziqing Hui (1):
      shell32: Rework FO_MOVE operation for SHFileOperationW.
```
