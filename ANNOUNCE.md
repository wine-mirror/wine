The Wine development release 10.17 is now available.

What's new in this release:
  - Mono engine updated to version 10.3.0.
  - EGL renderer used by default for OpenGL.
  - COMCTL32 split into separate v5 and v6 modules.
  - Better support for ANSI ODBC drivers.
  - Improved CPU info on FreeBSD.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/10.x/wine-10.17.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-10.17/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 10.17 (total 17):

 - #16214  Copy command does not support CON or CON:
 - #38987  tlReader 10.x crashes when searching dictionary (wxWidgets 2.9.4 questionable theming support detection via comctl32 v6 version resource)
 - #46603  Metro 2033 crashes on exit
 - #46823  in wcmd.exe, trailing slash in "if exist SomeDir/" cause always false
 - #51119  INSIDE ground is black rendered with OpenGL renderer
 - #52633  Some buttons from Free Virtual Keyboard's setting dialog are unthemed, needs comctl32 version 6.
 - #56157  create_logical_proc_info (Stub) : Applications can not spawn multiple threads
 - #56381  "type" does not support binary files
 - #56391  So Blonde: very long loading times
 - #57448  Erratic mouse movement with waylanddriver in Throne and Liberty
 - #57574  Wine 9.20: Application window positioning regression on laptop with multi monitor display setup
 - #57691  wine-mono: ASan gets triggered in mono_path_canonicalize with strcpy-param-overlap.
 - #58726  "Roon" music player only shows a blank white screen but buttons (that aren't displayed) technically work
 - #58737  Download of large file via WinHTTP fails with WSATIMEDOUT
 - #58755  16-bit applications crash under new wow64 on some processors
 - #58770  Unable to Compile 10.16
 - #58796  Wine configuration window doesn't appear after prefix creation

### Changes since 10.16:
```
Alexandre Julliard (37):
      winebuild: Add support for stripping files before setting the builtin flag.
      makedep: Use winebuild to strip installed files.
      tools: Add an STRARRAY_FOR_EACH macro to iterate an strarray.
      makedep: Use the STRARRAY_FOR_EACH macro.
      widl: Use the STRARRAY_FOR_EACH macro.
      wine: Use the STRARRAY_FOR_EACH macro.
      winebuild: Use the STRARRAY_FOR_EACH macro.
      winegcc: Use the STRARRAY_FOR_EACH macro.
      wrc: Use the STRARRAY_FOR_EACH macro.
      server: Consistently use STATUS_TOO_MANY_OPENED_FILES when running out of file descriptors.
      gitlab: Use the release keyword to create a release.
      winedump: Rename struct array to str_array.
      tools: Add a generic array type.
      makedep: Use a generic array for dependencies.
      makedep: Use a generic array for include files.
      makedep: Add an array of install commands instead of abusing strarrays.
      makedep: Build the installed file name when registering the install command.
      configure: Fix syntax error in hwloc check.
      windows.devices.enumeration: Fix the build with older Bison.
      ntdll: Don't assume that the NT name is null-terminated in get_load_order().
      ntdll: Build the NT name string directly in get_mapping_info().
      server: Return the module export name with the mapping info.
      ntdll: Use the export name if any to find the corresponding builtin.
      setupapi: Use the actual source name when registering a manifest.
      makedep: Include maintainer-generated files from the parent source directory.
      makedep: Use normal installation rules when symlinks are not supported.
      comctl32: Move version resource to a separate file.
      makefiles: List headers in the makefile if they shadow a global header.
      setupapi: Only use the actual source name for the first file in the manifest.
      msi/tests: Clear the export directory on copied dll to prevent loading the builtin.
      tools: Move the fatal_perror function to the shared header.
      tools: Add a shared helper to create a directory.
      tools: Add an install tool.
      makedep: Use the new install tool.
      shell32: Move ShellMessageBox implementation to shlwapi.
      configure: Remove a couple of unnecessary checks.
      makedep: Don't use LDFLAGS for special programs like the preloader.

Bernhard Übelacker (1):
      winegcc: Forward large address aware flag with platform windows.

Brendan Shanks (2):
      winemac: Draw black letter/pillarboxes around full-screen content.
      winemac: Remove the no-longer-needed macdrv_dll.h.

Connor McAdams (10):
      d3dx10/tests: Use readback functions from d3d10core tests for resource readback.
      d3dx10/tests: Add tests for the D3DX10_IMAGE_LOAD_INFO structure argument.
      d3dx10: Fill pSrcInfo structure in D3DX10_IMAGE_LOAD_INFO if passed in.
      d3dx10: Create 3D textures for images representing 3D textures.
      d3dx10: Add support for custom texture dimension arguments in D3DX10_IMAGE_LOAD_INFO.
      d3dx10: Add support for handling the format field in D3DX10_IMAGE_LOAD_INFO.
      d3dx10: Handle filter value passed in via D3DX10_IMAGE_LOAD_INFO.
      d3dx10: Add support for generating mipmaps to load_texture_data().
      d3dx10: Handle FirstMipLevel argument in load_texture_data().
      d3dx10: Pass D3DX10_IMAGE_LOAD_INFO texture creation arguments through in load_texture_data().

Damjan Jovanovic (1):
      ntdll: Implement create_logical_proc_info on FreeBSD.

Dmitry Timoshkov (20):
      include: Properly declare IADsPathname interface.
      adsldp: Properly declare IADs and IADsADSystemInfo interfaces.
      include: Add declarations for IADsDNWithString and IADsDNWithBinary interfaces.
      activeds: Add IADsDNWithBinary class implementation.
      adsldp: Use correct DN in IDirectorySearch::ExecuteSearch().
      adsldp: Reimplement IADs::GetInfoEx() on top of IDirectorySearch interface.
      adsldp: Implement IADs::GetEx().
      adsldp/tests: Add some tests for IADs::GetEx().
      adsldp: Implement IADs::get_Schema().
      adsldp: Fail to create IADs if it doesn't have an associated schema attribute.
      activeds: Retry without ADS_SECURE_AUTHENTICATION for an AD path.
      adsldp/tests: Add some tests for IADs::get_Schema().
      adsldp: Accept virtual objects in IADsOpenDSObject::OpenDSObject().
      adsldp/tests: Add a test for opening schema as an ADs object.
      activeds: Add registration of IADsDNWithString and IADsDNWithBinary objects.
      adsldp: Add IADsObjectOptions stub implementation.
      adsldp: Add support for IADsObjectOptions::GetOption(ADS_OPTION_SERVERNAME).
      adsldp: Add support for IADsObjectOptions::GetOption(ADS_OPTION_ACCUMULATIVE_MODIFICATION).
      adsldp: Return success for IADsObjectOptions::SetOption(ADS_OPTION_ACCUMULATIVE_MODIFICATION) stub.
      activeds: Remove misleading comments.

Elizabeth Figura (9):
      ddraw/tests: Fix some tests on modern Windows.
      ddraw/tests: Expand and fix tests for EnumDevices().
      ddraw: Expose the RGB device for version 1.
      ddraw: Reorder ddraw7 enumeration.
      ddraw: Rename the RGB device to "RGB Emulation".
      ddraw: Fix the size for d3d1 and d3d2 EnumDevices().
      user32/tests: Inline process helpers into test_child_process().
      ntdll: Also fix up invalid gsbase in the syscall and unix call dispatchers.
      shell32/tests: Expand attributes tests.

Eric Pouech (13):
      wineconsole: Ensure conhost gets the expected title.
      conhost: Support the close-on-exit configuration option.
      winedbg: No longer expect a startup sequence in auto mode.
      cmd/tests: Add some tests when using "complex" commands in pipes.
      cmd: Introduce a new node for describing blocks.
      cmd: Refactor run_external_full_path().
      cmd: Introduce helper for handling pipe commands.
      cmd: Run single commands in pipe in external cmd instance.
      cmd: Rewrite binary operations for pipes.
      cmd: Add support for IF commands in rewrite.
      cmd: Rewrite FOR commands for pipe.
      cmd: Rewrite explicit blocks for pipe.
      cmd: Remove transition helpers for pipe commands.

Esme Povirk (3):
      winbrand: Add stub dll.
      winbrand: Partially implement BrandingFormatString.
      mscoree: Update Wine Mono to 10.3.0.

Felix Hädicke (1):
      winhttp: Type UINT64 for variables content_length and content_read.

Gabriel Ivăncescu (11):
      mshtml: Factor out XMLHttpRequest creation.
      mshtml: Factor out XMLHttpRequest constructor init.
      mshtml: Separate the ifaces and the other XHR fields.
      mshtml: Factor out XMLHttpRequest's get_responseText.
      mshtml: Factor out XMLHttpRequest's abort.
      mshtml: Factor out XMLHttpRequest's open.
      mshtml: Factor out XMLHttpRequest's send.
      mshtml: Add XDomainRequest factory implementation.
      mshtml: Implement XDomainRequest.open().
      mshtml: Implement timeout for XDomainRequest.
      mshtml: Implement contentType for XDomainRequest.

Georg Lehmann (1):
      winevulkan: Update to VK spec version 1.4.329.

Giovanni Mascellani (22):
      mmdevapi/tests: Test after sleeping with read_packets().
      mmdevapi/tests: Move GetBufferSize() checks to the beginning of the test.
      mmdevapi/tests: Move checking the current padding out of read_packets().
      mmdevapi/tests: Test after overrunning buffer with read_packets().
      mmdevapi/tests: Test after stopping and restarting the client with read_packets().
      mmdevapi/tests: Test after stopping, resetting and restarting the client with read_packets().
      mmdevapi/tests: Iterate independently on sampling rates, channel counts and sample formats when rendering.
      mmdevapi/tests: Tweak the lists of audio format to test.
      mmdevapi/tests: Simplify checking IsFormatSupported() result when rendering.
      mmdevapi/tests: Check that Initialize() matches IsFormatSupported() when rendering.
      mmdevapi/tests: Test flag AUDCLNT_STREAMFLAGS_RATEADJUST when rendering.
      mmdevapi/tests: Test flag AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM when rendering.
      mmdevapi/tests: Test extensible wave formats when rendering.
      dsound: Remove dead lead-in logic.
      dsound: Simplify a condition in DSOUND_MixInBuffer().
      mmdevapi/tests: Iterate independently on sampling rates, channel counts and sample formats when capturing.
      mmdevapi/tests: Tweak the lists of audio format to test when capturing.
      mmdevapi/tests: Simplify checking IsFormatSupported() result when capturing.
      mmdevapi/tests: Check that Initialize() matches IsFormatSupported() when capturing.
      mmdevapi/tests: Test flag AUDCLNT_STREAMFLAGS_RATEADJUST when capturing.
      mmdevapi/tests: Test flag AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM when capturing.
      mmdevapi/tests: Test extensible wave formats when capturing.

Hans Leidekker (1):
      bcrypt: Support import/export for generic ECDH/ECDSA algorithms.

Jacek Caban (4):
      mshtml: Reset event handlers in document.open.
      win32u: Wrap vkGetPhysicalDeviceProperties2[KHR].
      win32u: Move LUID handling from winevulkan.
      winevulkan: Use generated PE thunks for vkGetPhysicalDeviceProperties2[KHR].

James McDonnell (3):
      shell32: Add shield icon.
      shell32: Map stock icons to shell32 resource id.
      shell32/tests: Add a test for stock icons.

Jiangyi Chen (2):
      user32/tests: Add some tests for uiLengthDrawn calculation in DrawTextExW().
      user32: Fix uiLengthDrawn calculation in DrawTextExW().

Jinoh Kang (1):
      ntdll: Use ReadPointerAcquire to get pointer value in RtlRunOnceComplete.

Louis Lenders (2):
      gdi32: Add stub for GetEnhMetaFilePixelFormat.
      uxtheme: Add stub for GetThemeBitmap.

Luca Bacci (1):
      ntdll: Use ReadPointerAcquire to get pointer value in RtlRunOnceBeginInitialize.

Marc-Aurel Zent (4):
      server: Add get_effective_thread_priority() helper.
      server: Add set_thread_disable_boost() helper.
      server: Boost main thread priority.
      ntdll: Switch to CLOCK_BOOTTIME for monotonic counters when available.

Matteo Bruni (6):
      d3dx9: Handle inverse transposed matrix in UpdateSkinnedMesh().
      d3dx9/tests: Skip ID3DXRenderToSurface tests if BeginScene() fails.
      d3dx9/tests: Increase tolerance in test_D3DXSHEvalDirectionalLight().
      d3dx9/tests: Add some fallback formats in the texture tests.
      d3dx10/tests: Disable size validation test.
      d3dx11/tests: Disable size validation test.

Michael Müller (1):
      shell32: Set SFGAO_HASSUBFOLDER correctly for normal shellfolders.

Michael Stefaniuc (11):
      dmstyle: Move the parsing of the style bands to a helper.
      dmstyle: Avoid Hungarian notation in a private struct.
      dmstyle: Simplify the load_band() helper.
      dmstyle: Move the IStream resetting to the load_band() helper.
      dmstyle: Use load_band() for the pattern band too.
      dmstyle: Simplify the style IPersistStream_Load method.
      dmstyle: Remove no longer needed helpers.
      dmstyle: Issue a FIXME for unhandled references to chormaps in a style.
      dmscript: Move the loading of the container to a helper.
      dmscript: Reimplement the loading of a script.
      dmscript: Get rid of the IDirectMusicScriptImpl typedef.

Nikolay Sivov (32):
      odbc32: Implement SQLSpecialColumnsW() for ANSI win32 drivers.
      odbc32: Implement SQLGetInfoW(SQL_SEARCH_PATTERN_ESCAPE) for ANSI win32 drivers.
      odbc32: Implement SQLColumnsW() for ANSI win32 drivers.
      oledb32/tests: Add some more tests for the error object.
      oledb32: Rename error object creation helper.
      oledb32/error: Store a dynamic id for each error record.
      oledb32/errorinfo: Add a helper to access records by index.
      oledb32/errorinfo: Return 0-record info with GetGUID().
      oledb32/errorinfo: Rename a helper.
      oledb32/errorinfo: Return stub error info objects from GetErrorInfo().
      oledb32/errorinfo: Reference error object for each returned IErrorInfo instance.
      oledb32/errorinfo: Implement GetSource() using error lookup service.
      oledb32/errorinfo: Release dynamic error information on cleanup.
      oledb32/errorinfo: Implement GetDescription() using error lookup service.
      oledb32/errorinfo: Implement GetHelpFile() using error lookup service.
      oledb32/errorinfo: Implement GetHelpContext() using error lookup service.
      oledb32/errorinfo: Implement IErrorInfo methods of the error object.
      oledb32/errorinfo: Implement GetGUID().
      oledb32: Consider requested interface in DllGetClassObject().
      oledb32: Use public symbol for the Error Object CLSID.
      dwrite: Update script information with Unicode 16.0 additions.
      dwrite: Explicitly check scripts for "complexity" property.
      mfplat/tests: Fix a crash on some Windows systems.
      ole32: Move CoTreatAsClass() to combase.
      combase: Check for null arguments in CoTreatAsClass().
      combase: Remove unreachable failure path in CoTreatAsClass().
      ole32: Move CoIsOle1Class() stub to combase.
      oleaut32/tests: Enable more tests for VarMul().
      oleaut32/tests: Enable more tests for VarEqv().
      oleaut32/tests: Enable more tests for VarXor().
      oleaut32/tests: Enable more tests for VarAnd().
      bcp47langs/tests: Fix a crash when running on Wine.

Owen Rudge (3):
      taskschd: Only release ITaskDefinition if creation of RegisteredTask fails.
      taskschd/tests: Add test for ITaskDefinition Data property.
      taskschd: Implement ITaskDefinition Data property.

Paul Gofman (12):
      server: Support NULL job handle in get_job_info().
      shell32: Add Screenshots known folder.
      win32u: Bump GPU internal driver versions to a future version.
      ntdll: Add a stub for EtwEventWriteEx().
      advapi32: Add a stub for EnumerateTraceGuidsEx().
      windows.media.speech: Add stub IRandomAccessStream interface to SpeechSynthesisStream class.
      windows.media.speech: Add stub IInputStream interface to SpeechSynthesisStream class.
      windows.media.speech: Add semi-stub for synthesis_stream_input_ReadAsync().
      windows.media.speech: Return semi-stub IAsyncInfo from async_with_progress_uint32_QueryInterface().
      ntdll/tests: Add more tests for nested exceptions on x64.
      ntdll: Make nested_exception_handler() arch specific.
      ntdll: Clear nested exception flag at correct frame on x64.

Piotr Caban (17):
      odbc32: Add SQLDriverConnectA() implementation.
      odbc32: Handle SQL_C_WCHAR conversion in SQLGetData for ANSI drivers.
      odbc32: Implement SQLStatisticsW() for ANSI win32 drivers.
      odbc32: Implement SQLGetInfoW(SQL_STRING_FUNCTIONS) for ANSI win32 drivers.
      odbc32: Implement SQLGetInfoW(SQL_NUMERIC_FUNCTIONS) for ANSI win32 drivers.
      odbc32: Implement SQLGetInfoW(SQL_TIMEDATE_FUNCTIONS) for ANSI win32 drivers.
      odbc32: Implement SQLGetInfoW(SQL_SYSTEM_FUNCTIONS) for ANSI win32 drivers.
      odbc32: Implement SQLGetInfoW(SQL_CONVERT_*) for ANSI win32 drivers.
      odbc32: Implement SQLGetInfoW(SQL_CONCAT_NULL_BEHAVIOR) for ANSI win32 drivers.
      odbc32: Implement SQLGetInfoW(SQL_EXPRESSIONS_IN_ORDERBY) for ANSI win32 drivers.
      odbc32: Implement SQLGetInfoW(SQL_ORDER_BY_COLUMNS_IN_SELECT) for ANSI win32 drivers.
      odbc32: Remove prepare_con() helper.
      odbc32: Support SQLGetInfo(SQL_OJ_CAPABILITIES) in older drivers.
      odbc32: Remove debugstr_sqllen() helper.
      odbc32: Remove debugstr_sqlulen() helper.
      msvcrt: Optimize _ismbcspace_l for ASCII range.
      msvcrt: Support anonymous namespace in __unDName().

Ratchanan Srirattanamet (1):
      msi: Record feature in assembly publication when package has 2+ features.

Reinhold Gschweicher (1):
      msxml3/element: Implement removeAttributeNode function.

Rémi Bernon (76):
      gitlab: Install mesa-vulkan-drivers packages.
      server: Move class info to the shared memory object.
      win32u: Read class info from the shared memory object.
      server: Allocate shared memory objects with dynamic size.
      server: Move extra class info to the shared memory object.
      win32u: Read extra class info from the shared memory object.
      win32u: Ensure D3DKMTQueryVideoMemoryInfo usage is smaller than budget.
      winevulkan: Treat LPCWSTR and HANDLE as pointer sized types.
      win32u: Pass the name of the extension to replace to get_host_extension.
      win32u: Swap VK_KHR_external_memory_win32 with the matching host extension.
      win32u: Create D3DKMT global resources for exported Vulkan memory.
      win32u: Implement Vulkan memory D3DKMT NT shared handle export.
      win32u: Implement Vulkan memory D3DKMT global / shared handle import.
      win32u: Implement Vulkan memory D3DKMT handle import from name.
      maintainers: Assume maintainership of the GL / VK / D3DKMT areas.
      opengl32: Remove deprecated WGL_RENDERER_ID_WINE constant.
      win32u: Enumerate and initialize EGL devices as separate platforms.
      win32u: Initialize some renderer properties with EGL devices.
      win32u: Check maximum context versions with EGL devices.
      win32u: Check renderer version and video memory from EGL devices.
      winevulkan: Swap VK_KHR_external_semaphore_win32 with the host extension.
      win32u: Swap handle types in vkGetPhysicalDeviceExternalSemaphoreProperties.
      win32u: Implement export of semaphores as D3DKMT global handles.
      win32u: Implement import of semaphores from D3DKMT global handles.
      win32u: Implement export of semaphores as D3DKMT shared handles.
      win32u: Implement import of semaphores from D3DKMT shared handles.
      win32u: Implement import of semaphores from D3DKMT object names.
      server: Use the object runtime data size for the reply.
      server: Don't crash in d3dkmt_share_objects if attributes are invalid.
      win32u: Fix swapped security / root object attributes.
      win32u: Introduce a new D3DKMT_ESCAPE_UPDATE_RESOURCE_WINE code.
      winevulkan: Swap VK_KHR_external_fence_win32 with the host extension.
      win32u: Swap handle types in vkGetPhysicalDeviceExternalFenceProperties.
      win32u: Implement export of fences as D3DKMT global handles.
      win32u: Implement import of fences from D3DKMT global handles.
      win32u: Implement export of fences as D3DKMT shared handles.
      win32u: Implement import of fences from D3DKMT shared handles.
      win32u: Implement import of fences from D3DKMT object names.
      winex11: Flush the thread display after processing every event.
      winex11: Remove some arguably unnecessary XFlush calls.
      win32u: Check QS_DRIVER bit before calling ProcessEvents.
      win32u: Pass the caller class name to NtUserCreateWindowEx.
      win32u: Use the caller class name in CREATESTRUCT lpszClass.
      win32u: Set CREATESTRUCT lpszClass to the base class name.
      winex11: Move colormap ownership to x11drv_client_surface.
      winex11: Use the visual depth when creating the client window.
      winex11: Lookup visual from EGL config EGL_NATIVE_VISUAL_ID.
      win32u: Move fetching window icon out of the drivers.
      winex11: Clear WS_VISIBLE to delay showing layered windows.
      winex11: Remove unnecessary calls to map_window helper.
      winex11: Call window_set_wm_state instead of map_window.
      win32u: Use an absolute wait timeout in wait_message.
      win32u: Keep waiting on the queue until it gets signaled.
      server: Clear QS_RAWINPUT | QS_HARDWARE in get_rawinput_buffer.
      win32u: Process internal hardware messages while waiting.
      server: Don't set QS_RAWINPUT for internal hardware messages.
      winevulkan: Move physical device extension checks around.
      winevulkan: Require VK_EXT_map_memory_placed for WOW64 memory export.
      winevulkan: Handle WOW64 conversion of SECURITY_ATTRIBUTES.
      ntdll: Export wine_server_send_fd to other unix libs.
      win32u: Export host memory unix file descriptor.
      win32u: Send D3DKMT object fd to wineserver on creation.
      win32u: Import vulkan memory from the D3DKMT object fd.
      win32u/tests: Avoid closing invalid handle after OpenSharedHandleByName.
      win32u: Use local handles in D3DKMT_ESCAPE_UPDATE_RESOURCE_WINE.
      win32u: Fix incorrect D3DKMT exported name length.
      winex11: Use EGL by default for OpenGL rendering.
      server: Target the default or foreground window for internal hardware messages.
      winex11: Avoid freeing default colormap.
      win32u: Keep exported semaphore fd with the global D3DKMT object.
      win32u: Import Vulkan semaphore from the D3DKMT object fd.
      win32u: Keep exported fence fd with the D3DKMT global object.
      win32u: Import Vulkan fence from the D3DKMT object fd.
      winex11: Update WM_HINTS and NET_WM_STATE when WM_STATE isn't.
      winex11: Simplify the logic to reset cursor clipping.
      winex11: Remove unnecessary extra window hiding condition.

Sven Baars (3):
      ntdll: Cast tv_sec to ULONGLONG before multiplying.
      mmdevapi/tests: Use %I64u instead of %llu.
      winhttp: Use %I64u instead of %llu.

Ugur Sari (1):
      netapi32: Add stubs for NetValidatePasswordPolicy and NetValidatePasswordPolicyFree.

Vasiliy Stelmachenok (5):
      opengl32: Fix incorrect wglQueryCurrentRendererStringWINE call.
      opengl32: Generate entry points for EGL_EXT_device_drm extension.
      win32u: Stub WGL_WINE_query_renderer when EGL devices are supported.
      win32u: Implement wglQueryCurrentRenderer* with wglQueryRenderer*.
      win32u: Implement wglQueryRendererStringWINE with EGL devices.

Vibhav Pant (7):
      makedep: Support including generated WinRT metadata in resource files.
      cfgmgr32/tests: Add tests to ensure DevGetObjects(Ex) does not de-duplicate requested properties.
      windows.devices.enumeration/tests: Add tests for device properties for IDeviceInformation objects.
      windows.devices.enumeration/tests: Add tests for IDeviceInformationStatics::FindAllAsyncAqsFilterAndAdditionalProperties.
      windows.devices.enumeration: Implement IDeviceInformation::get_Properties.
      windows.devices.enumeration: Implement IDeviceInformationStatics::{FindAllAsyncAqsFilterAndAdditionalProperties, CreateWatcherAqsFilterAndAdditionalProperties}.
      rometadata: Add stub for IMetaDataTables.

Vijay Kiran Kamuju (1):
      bcp47langs: Add stub for GetUserLanguages.

Yuxuan Shui (4):
      windows.devices.enumeration: Fix use-after-realloc in devpropcompkeys_append_names.
      windows.devices.enumeration: Don't leave dangling pointers if devpropcompkeys_append_names fails.
      iphlpapi: Give GetBestRoute2 a SOCKETADDR_INET.
      include: Add atomic read/write of pointers.

Zhengyong Chen (1):
      imm32: Do not overwrite input context window with GetFocus() in ime_ui_update_window.

Zhiyi Zhang (26):
      comctl32/tests: Fix a window leak.
      user32: Load version for comctl32 v6 window classes.
      comctl32/tests: Add version tests.
      comctl32: Separate v5 and v6.
      comctl32/tests: Add RegisterClassNameW() tests.
      comctl32: Remove user32 control copies in comctl32 v5.
      comctl32/tests: Test v6 only exports.
      comctl32: Remove taskdialog from comctl32 v5.
      comctl32: Remove syslink from comctl32 v5.
      comctl32: Remove v6 only exports.
      comctl32/tests: Add CCM_SETVERSION tests.
      comctl32/toolbar: Don't change Unicode format when handling CCM_SETVERSION.
      comctl32/listview: Fix CCM_{GET,SET}VERSION handling.
      comctl32/rebar: Fix CCM_{GET,SET}VERSION handling.
      comctl32/toolbar: Fix CCM_{GET,SET}VERSION handling.
      include: Bump COMCTL32_VERSION to 6.
      comctl32/tests: Move toolbar Unicode format tests.
      comctl32: Add a helper to handle CCM_SETVERSION messages.
      user32/tests: Remove some broken() for older systems in test_comctl32_class().
      wintypes: Fix a memory leak.
      wintypes: Restore a pointer check in property_value_statics_CreateString().
      wintypes: Increase string reference count when property_value_GetString() succeeds.
      wintypes: Create a copy for string arrays for property_value_statics_CreateStringArray().
      wintypes: Increase string reference count when property_value_GetStringArray() succeeds.
      winex11.drv: Ignore fullscreen window config changes.
      win32u: Correct a comment.
```
