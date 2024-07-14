The Wine development release 9.13 is now available.

What's new in this release:
  - Support for loading ODBC Windows drivers.
  - More user32 data structures in shared memory.
  - More rewriting of the CMD.EXE engine.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/9.x/wine-9.13.tar.xz>

Binary packages for various distributions will be available
from <https://www.winehq.org/download>

You will find documentation on <https://www.winehq.org/documentation>

Wine is available thanks to the work of many people.
See the file [AUTHORS][1] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/raw/wine-9.13/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 9.13 (total 22):

 - #21344  Buffer overflow in WCMD_run_program
 - #35163  Victoria 2: A House Divided crashes on start with built-in quartz
 - #39206  Lylian demo hangs after intro video
 - #44315  Buffer maps cause CPU-GPU synchronization (Guild Wars 2, The Witcher 3)
 - #44888  Wrong texture in Assassin's Creed : Revelations
 - #45810  WINEPATH maximums
 - #52345  Unclosed right-side double quote in if command breaks wine cmd
 - #52346  Filename completion is not supported in cmd
 - #54246  Full Metal Daemon Muramasa stuck at black screen at boot
 - #54499  Native ODBC drivers should be able be used.
 - #54575  False positive detection of mmc reader as hard drive since kernel 6.1
 - #55130  IF EXIST fails when its argument ends on a slash
 - #55401  CMD 'for loop' params are not recognized
 - #56575  CUERipper 2.2.5 Crashes on loading WMA plugin
 - #56600  MEGA TECH Faktura Small Business: Access violation in module kernelbase.dll
 - #56617  Photoshop CC 2024: crashes after a short period (Unimplemented function NETAPI32.dll.NetGetAadJoinInformation)
 - #56882  ConEmu errors: Injecting hooks failed
 - #56895  So Blonde (demo): font display bug (regression)
 - #56938  msiexec crashes with stack overflow when installing python 3.11+ dev.msi
 - #56945  Multiple UI elements in builtin programs is missing (taskbar in Virtual Desktop, right-click menu in RegEdit)
 - #56948  Intel Hardware Accelerated Execution Manager needs unimplemented function ntoskrnl.exe.KeQueryGroupAffinity
 - #56952  PS installer crashes inside msi (regression)

### Changes since 9.12:
```
Alex Henrie (11):
      msi: Initialize size argument to RegGetValueW.
      shell32: Pass size in bytes to RegGetValueW.
      twinapi.appcore: Initialize size argument to RegGetValueW.
      mscoree: Pass size in bytes to RegGetValueW.
      wineboot: Correct size argument to SetupDiSetDeviceRegistryPropertyA.
      advapi32/tests: Test RegGetValue[AW] null termination.
      advapi32/tests: Drop security test workarounds for Windows <= 2000.
      windowscodecs: Use RegQueryValueExW in ComponentInfo_GetStringValue.
      kernelbase: Ensure null termination in RegGetValue[AW].
      ntdll: Double-null-terminate registry multi-strings in RtlQueryRegistryValues.
      ntdll/tests: Remove unused WINE_TODO_DATA flag.

Alexandre Julliard (26):
      kernelbase: Mask extra protection flags in WriteProcessMemory.
      wow64: Call pre- and post- memory notifications also in the current process case.
      wow64: Add more cross-process notifications.
      ntdll/tests: Add tests for in-process memory notifications on ARM64.
      wow64: Add a helper to get the 32-bit TEB.
      ntdll: Always set the dll name pointer in the 32-bit TEB.
      wow64: Fix NtMapViewOfSection CPU backend notifications.
      wow64: Add NtReadFile CPU backend notifications.
      wow64cpu: Simplify the Unix call thunk.
      xtajit64: Add stub dll.
      ntdll: Load xtajit64.dll on ARM64EC.
      ntdll/tests: Add some tests for xtajit64.
      ntdll: Create the cross-process work list at startup on ARM64EC.
      ntdll: Support the ARM64EC work list in RtlOpenCrossProcessEmulatorWorkConnection.
      ntdll: Call the processor information callback on ARM64EC.
      ntdll: Load the processor features from the emulator on ARM64EC.
      ntdll: Call the flush instruction cache callback on ARM64EC.
      ntdll: Call the memory allocation callbacks on ARM64EC.
      ntdll: Call the section map callbacks on ARM64EC.
      ntdll: Call the read file callback on ARM64EC.
      ntdll: Implement ProcessPendingCrossProcessEmulatorWork on ARM64EC.
      wininet/tests: Update issuer check for winehq.org certificate.
      wow64: Fix prototype for the NtTerminateThread CPU backend notification.
      wow64: Add NtTerminateProcess CPU backend notifications.
      ntdll: Call the terminate thread callback on ARM64EC.
      ntdll: Call the terminate process callback on ARM64EC.

Alexandros Frantzis (4):
      opengl32: Add default implementation for wglChoosePixelFormatARB.
      winex11: Remove driver wglChoosePixelFormatARB implementation.
      winewayland: Support WGL_ARB_pixel_format.
      winewayland: Support WGL_ARB_pixel_format_float.

Alfred Agrell (10):
      include: Fix typo in DXGI_DEBUG_APP.
      include: Fix typo in IID_IDWriteStringList.
      include: Fix typo in IID_IAudioLoudness.
      include: Fix typo in GUID_DEVCLASS_1394DEBUG.
      include: Fix typo in IID_IRemoteDebugApplication.
      include: Fix typos in MF_MT_VIDEO_3D and MF_MT_AUDIO_FOLDDOWN_MATRIX.
      include: Fix typos in IID_IMimeWebDocument and IID_IMimeMessageCallback.
      include: Fix typos in IID_IPropertyEnumType2 and CLSID_PropertySystem.
      include: Fix typo in MEDIASUBTYPE_P408.
      include: Fix typo in CLSID_WICXMPMetadataReader.

Austin English (2):
      netapi32: Add NetGetAadJoinInformation stub.
      ntoskrnl.exe: Add a stub for KeQueryGroupAffinity.

Biswapriyo Nath (5):
      include: Add flags for ID3D11ShaderReflection::GetRequiresFlags method in d3d11shader.h.
      include: Add macros for d3d12 shader version in d3d12shader.idl.
      include: Add new names in D3D_NAME enum in d3dcommon.idl.
      include: Fix typo with XINPUT_DEVSUBTYPE_FLIGHT_STICK name in xinput.h.
      include: Fix typo with X3DAUDIO_EMITTER structure in x3daudio.h.

Brendan McGrath (3):
      winegstreamer: Use process affinity to calculate thread_count.
      winegstreamer: Use thread_count to determine 'max-threads' value.
      winegstreamer: Set 'max_threads' to 4 for 32-bit processors.

Connor McAdams (14):
      d3dx9/tests: Move the images used across multiple test files into a shared header.
      d3dx9/tests: Add more D3DXLoadVolumeFromFileInMemory() tests.
      d3dx9: Use shared code in D3DXLoadVolumeFromFileInMemory().
      d3dx9/tests: Add more tests for D3DXCreateVolumeTextureFromFileInMemoryEx().
      d3dx9: Refactor texture creation and cleanup in D3DXCreateVolumeTextureFromFileInMemoryEx().
      d3dx9: Cleanup texture value argument handling in D3DXCreateVolumeTextureFromFileInMemoryEx().
      d3dx9: Use d3dx_image structure inside of D3DXCreateVolumeTextureFromFileInMemoryEx().
      d3dx9: Add support for mipmap generation to D3DXCreateVolumeTextureFromFileInMemoryEx().
      d3dx9/tests: Add tests for DDS skip mip level bits.
      d3dx9: Apply the DDS skip mip level bitmask.
      d3dx9/tests: Add more DDS header tests for volume texture files.
      d3dx9: Check the proper flag for DDS files representing a volume texture.
      d3dx9/tests: Add more DDS header tests for cube texture files.
      d3dx9: Return failure if a cubemap DDS file does not contain all faces.

Dmitry Timoshkov (3):
      msv1_0: Add support for SECPKG_CRED_BOTH.
      kerberos: Add support for SECPKG_CRED_BOTH.
      crypt32: Make CertFindCertificateInStore(CERT_FIND_ISSUER_NAME) work.

Elizabeth Figura (19):
      d3dcompiler/tests: Use the correct interfaces for some COM calls.
      mfplat/tests: Use the correct interfaces for some COM calls.
      d3dx9: Use the correct interfaces for some COM calls.
      d3dx9/tests: Define COBJMACROS.
      mfplat/tests: Add more tests for compressed formats.
      winegstreamer: Check the version before calling wg_format_from_caps_video_mpeg1().
      winegstreamer: Implement MPEG-4 audio to wg_format conversion.
      winegstreamer: Implement H.264 to wg_format conversion.
      winegstreamer: Implement H.264 to IMFMediaType conversion.
      winegstreamer: Implement AAC to IMFMediaType conversion.
      winegstreamer: Implement WMV to IMFMediaType conversion.
      winegstreamer: Implement WMA to IMFMediaType conversion.
      winegstreamer: Implement MPEG-1 audio to IMFMediaType conversion.
      wined3d: Invalidate the FFP VS when diffuse presence changes.
      wined3d: Destroy the push constant buffers on device reset.
      wined3d: Feed the fragment part of WINED3D_RS_SPECULARENABLE through a push constant buffer.
      wined3d: Feed the FFP color key through a push constant buffer.
      wined3d: Reorder light application in wined3d_device_apply_stateblock().
      wined3d: Feed WINED3D_RS_AMBIENT through a push constant buffer.

Eric Pouech (49):
      cmd: Add success/failure tests for file related commands.
      cmd: Set success/failure return code for TYPE command.
      cmd: Set success/failure return code DELETE command.
      cmd: Set success/failure return code for MOVE command.
      cmd: Set success/failure return code for RENAME command.
      cmd: Set success/failure return code for COPY command.
      cmd: Add success/failure tests for dir related commands.
      cmd: Add success/failure return code for MKDIR/MD commands.
      cmd: Set success/failure return code for CD command.
      cmd: Set success/failure return code for DIR command.
      cmd: Set success/failure return code for PUSHD command.
      cmd: Add some more tests for success/failure.
      cmd: Return tri-state for WCMD_ReadParseLine().
      cmd: Improve return code / errorlevel handling.
      cmd: Set success/failure return_code for POPD command.
      cmd: Set success/failure return code for RMDIR/RD command.
      cmd: Don't set ERRORLEVEL in case of redirection error.
      cmd/tests: Test success / failure for more commands.
      cmd: Set success/failure return code for SETLOCAL/ENDLOCAL commands.
      cmd: Set success/failure return code for DATE/TIME commands.
      cmd: Set success/failure return code for VER command.
      cmd: Set success/failure return code for VERIFY command.
      cmd: Set success/failure return code for VOL command.
      cmd: Set success/failure return code for LABEL command.
      cmd/tests: Add more tests for success/failure.
      cmd: Set success/failure return code of PATH command.
      cmd: Set success/failure return code for SET command.
      cmd: Set success/failure return code for ASSOC,FTYPE commands.
      cmd: Set success/failure return code for SHIFT command.
      cmd: Set success/failure return code for HELP commands.
      cmd: Set success/failure return_code for PROMPT command.
      cmd: Add tests for screen/interactive builtin commands.
      cmd: Set success/failure return code for CLS command.
      cmd: Set success/failure return code for COLOR command.
      cmd: Set success/failure return code for TITLE command.
      cmd: Use the correct output handle in pipe command.
      cmd: Set success/failure return code for CHOICE command.
      cmd: Set success/failure return code for MORE command.
      cmd: Set success/failure return code for PAUSE command.
      cmd: Get rid of CTTY command.
      cmd: Add more tests for return codes in builtin commands.
      cmd: Set success/failure return code for MKLINK command.
      cmd: Set success/failure return code for START command.
      cmd: Move empty batch command handling to WCMD_batch() callers.
      cmd: Improve return code/errorlevel support for external commands.
      cmd: Cleanup transition bits.
      cmd: Get rid for CMD_COMMAND structure.
      cmd: When parsing, dispose created objects on error path.
      cmd: Fix a couple of issues with redirections.

Fabian Maurer (6):
      cmd: Close file opened with popen with correct function (coverity).
      mlang/tests: Add test for GetGlobalFontLinkObject allowing IID_IMultiLanguage2.
      mlang/tests: Add tests showing which interface is returned by GetGlobalFontLinkObject.
      mlang: Return the correct interface in GetGlobalFontLinkObject.
      d3dx9: Remove superflous nullcheck (coverity).
      msv1_0: Set mode in ntlm_check_version.

Hans Leidekker (25):
      msi: Avoid infinite recursion while processing the DrLocator table.
      odbc32: Turn SQLBindParam() into a stub.
      odbc32: Replicate Unix data sources to the ODBC Data Sources key.
      odbc32: Reimplement SQLDrivers() using registry functions.
      odbc32: Reimplement SQLDataSources() using registry functions.
      odbc32: Introduce a Windows driver loader and forward a couple of functions.
      odbc32: Forward more functions to the Windows driver.
      odbc32: Forward yet more functions to the Windows driver.
      odbc32: Forward the remaining functions to the Windows driver.
      odbc32/tests: Add tests.
      msi: Handle failure from MSI_RecordGetInteger().
      msi: Load DrLocator table in ITERATE_AppSearch().
      winhttp: Implement WinHttpQueryOption(WINHTTP_OPTION_URL).
      odbc32: Implement SQLSetEnvAttr(SQL_ATTR_ODBC_VERSION).
      odbc32: Implement SQLGet/SetConnectAttr(SQL_ATTR_LOGIN_TIMEOUT).
      odbc32: Implement SQLGet/SetConnectAttr(SQL_ATTR_CONNECTION_TIMEOUT).
      odbc32: Stub SQLGetEnvAttr(SQL_ATTR_CONNECTION_POOLING).
      odbc32: Handle options in SQLFreeStmt().
      odbc32: Default to ODBC version 2.
      odbc32: Implement SQLGetInfo(SQL_ODBC_VER).
      odbc32: Factor out helpers to create driver environment and connection handles.
      odbc32: Accept SQL_FETCH_NEXT in SQLDataSources/Drivers() if the key has not been opened.
      odbc32: Set parent functions before creating the environment handle.
      odbc32: Use SQLFreeHandle() instead of SQLFreeEnv/Connect().
      odbc32: Use SQLSetConnectAttrW() instead of SQLSetConnectAttr() if possible.

Ilia Docin (1):
      comctl32/rebar: Hide chevron if rebar's band is resized back to full size with gripper.

Jacek Caban (38):
      jscript: Factor out find_external_prop.
      jscript: Rename PROP_IDX to PROP_EXTERN.
      jscript: Introduce lookup_prop callback.
      jscript: Factor out lookup_dispex_prop.
      jscript: Introduce next_property callback.
      jscript: Factor out handle_dispatch_exception.
      jscript: Use to_disp in a few more places.
      mshtml: Factor out dispex_prop_put.
      mshtml: Factor out dispex_prop_get.
      mshtml: Factor out dispex_prop_call.
      jscript: Allow objects to have their own addref and release implementation.
      jscript: Introduce IWineJSDispatch insterface.
      mshtml: Allow external properties to have arbitrary names.
      jscript: Introduce HostObject implementation.
      jscript: Support converting host objects to string.
      jscript: Support host objects in disp_cmp.
      jscript: Use jsdisp_t internally for host objects that support it.
      mshtml: Implement jscript IWineJSDispatchHost.
      mshtml: Pass an optional script global window to init_dispatch.
      mshtml: Support using IWineJSDispatch for DispatchEx implementation.
      mshtml: Use IWineJSDispatch for screen object script bindings.
      jscript: Factor out native_function_string.
      jscript: Add support for host functions.
      mshtml/tests: Make todo_wine explicit in builtin_toString tests.
      mshtml: Use host object script bindings for DOMImplementation class.
      mshtml: Use host object script bindings for History class.
      mshtml: Use host object script bindings for PerformanceNavigation class.
      mshtml: Use host object script bindings for PerformanceTiming class.
      mshtml: Use host object script bindings for Performance class.
      mshtml: Store document node instead of GeckoBrowser in DOMImplementation.
      mshtml/tests: Add script context test.
      mshtml: Store script global object pointer in document object.
      mshtml: Use host object script bindings for MediaQueryList class.
      mshtml: Use host object script bindings for Navigator class.
      mshtml: Use host object script bindings for Selection class.
      mshtml: Use host object script bindings for TextRange class.
      mshtml: Use host object script bindings for Range class.
      mshtml: Use host object script bindings for Console class.

Marc-Aurel Zent (4):
      ntdll: Prefer futex for thread-ID alerts over kqueue.
      ntdll: Use USE_FUTEX to indicate futex support.
      ntdll: Simplify futex interface from futex_wake() to futex_wake_one().
      ntdll: Implement futex_wait() and futex_wake_one() on macOS.

Matthias Gorzellik (2):
      winebus.sys: Fix rotation for angles < 90deg.
      winebus.sys: Align logical max of angles to physical max defined in dinput.

Mohamad Al-Jaf (7):
      include: Add windows.data.json.idl file.
      windows.web: Add stub DLL.
      windows.web: Implement IActivationFactory::ActivateInstance().
      include: Add IJsonValueStatics interface definition.
      windows.web: Add IJsonValueStatics stub interface.
      windows.web/tests: Add IJsonValueStatics::CreateStringValue() tests.
      windows.web: Implement IJsonValueStatics::CreateStringValue().

Nikolay Sivov (2):
      winhttp/tests: Add some tests for querying string options with NULL buffer.
      winhttp: Fix error handling when returning string options.

Paul Gofman (14):
      ntdll: Report the space completely outside of reserved areas as allocated on i386.
      psapi/tests: Add tests for QueryWorkingSetEx() with multiple addresses.
      ntdll: Validate length in get_working_set_ex().
      ntdll: Factor OS-specific parts out of get_working_set_ex().
      ntdll: Iterate views instead of requested addresses in get_working_set_ex().
      ntdll: Limit vprot scan range to the needed interval in get_working_set_ex().
      ntdll: Fill range of output in fill_working_set_info().
      ntdll: Buffer pagemap reads in fill_working_set_info().
      winhttp/tests: Add test for trailing spaces in reply header.
      winhttp: Construct raw header from the parse result in read_reply().
      winhttp: Skip trailing spaces in reply header names.
      win32u: Use FT_LOAD_PEDANTIC on first load try in freetype_get_glyph_outline().
      ntdll: Better track thread pool wait's wait_pending state.
      ntdll: Make sure wakeups from already unset events are ignored in waitqueue_thread_proc().

Piotr Caban (9):
      ucrtbase: Store exception record in ExceptionInformation[6] during unwinding.
      xcopy: Exit after displaying help message.
      xcopy: Exit on invalid command line argument.
      xcopy: Strip quotes only from source and destination arguments.
      xcopy: Introduce get_arg helper that duplicates first argument to new string.
      xcopy: Handle switch options concatenated with path.
      xcopy: Add support for parsing concatenated switches.
      kernel32/tests: Fix CompareStringW test crash when linguistic compressions are used.
      ucrtbase: Fix FILE no buffering flag value.

Rémi Bernon (60):
      server: Move thread message queue masks to the shared mapping.
      win32u: Read the thread message queue masks from the shared memory.
      server: Move thread message queue bits to the shared mapping.
      win32u: Use the thread message queue shared memory in get_input_state.
      win32u: Use the thread message queue shared memory in NtUserGetQueueStatus.
      win32u: Use the thread message queue shared memory in wait_message_reply.
      mf/session: Don't update transform output type if not needed.
      mf/session: Implement D3D device manager propagation.
      winegstreamer: Translate GstCaps directly to MFVIDEOFORMAT / WAVEFORMATEX in wg_transform.
      winegstreamer: Translate MFVIDEOFORMAT / WAVEFORMATEX directly to GstCaps in wg_transform.
      winegstreamer: Create transforms from MFVIDEOFORMAT / WAVEFORMATEX.
      winegstreamer: Only use pool and set buffer meta for raw video frames.
      winegstreamer: Use a new wg_video_buffer_pool class to add buffer meta.
      winegstreamer: Keep the input caps on the transform.
      winegstreamer: Use video info stride in buffer meta rather than videoflip.
      winegstreamer: Normalize both input and output media types stride at once.
      winegstreamer: Normalize video processor and color converter apertures.
      winegstreamer: Respect video format padding for input buffers too.
      server: Move the desktop flags to the shared memory.
      win32u: Use the shared memory to read the desktop flags.
      server: Create a thread input shared mapping.
      server: Move active window to input shared memory.
      server: Move focus window to input shared memory.
      server: Move capture window to input shared memory.
      server: Move caret window and rect to input shared memory.
      win32u: Use input shared memory for NtUserGetGUIThreadInfo.
      win32u: Move offscreen window surface creation fallback.
      win32u: Split a new create_window_surface helper from apply_window_pos.
      win32u: Pass the window surface rect for CreateLayeredWindow.
      win32u: Pass whether window is shaped to drivers WindowPosChanging.
      win32u: Introduce a new window surface helper to set window shape.
      win32u: Use a 1bpp bitmap to store the window surface shape bits.
      win32u: Update the window surface shape with color key and alpha.
      win32u: Pass the shape bitmap info and bits to window_surface flush.
      mfreadwrite/reader: Look for a matching output type if setting it failed.
      winex11: Reset window shape whenever window surface is created.
      mf/tests: Remove static specifier on variables referencing other variables.
      win32u: Allocate heap in peek_message only when necessary.
      win32u: Use the thread message queue shared memory in peek_message.
      win32u: Simplify the logic for driver messages polling.
      ddraw/tests: Make sure the window is restored after some minimize tests.
      ddraw/tests: Flush messages and X11 events between some tests.
      server: Add a foreground flag to the thread input shared memory.
      server: Add cursor handle and count to desktop shared memory.
      win32u: Use the thread input shared memory for NtUserGetForegroundWindow.
      win32u: Use the thread input shared memory for NtUserGetCursorInfo.
      win32u: Use the thread input shared memory for NtUserGetGUIThreadInfo.
      server: Use a shared_object_t for the dummy object.
      win32u: Check the surface layered nature when reusing window surface.
      win32u: Update the window state when WS_EX_LAYERED window style changes.
      win32u: Update the layered surface attributes in apply_window_pos.
      winex11: Rely on win32u layered window surface attribute updates.
      wineandroid: Rely on win32u layered window surface attribute updates.
      winemac: Rely on win32u layered window surface attribute updates.
      winegstreamer/video_decoder: Generate timestamps relative to the first input sample.
      mfreadwrite/reader: Send MFT_MESSAGE_NOTIFY_START_OF_STREAM on start or seek.
      mf/tests: Reduce the mute threshold for the transform tests.
      win32u: Fix initial value when checking whether WS_EX_LAYERED changes.
      win32u: Use the dummy surface for empty layered window surfaces.
      maintainers: Remove MF GStreamer section.

Shaun Ren (1):
      dinput: Call handle_foreground_lost() synchronously in cbt_hook_proc().

Stefan Dösinger (1):
      ddraw: Set dwMaxVertexCount to 2048.

Zhiyi Zhang (1):
      winemac.drv: Remove the clear OpenGL views to black hack.

Ziqing Hui (18):
      winegstreamer/video_encoder: Implement GetInputAvailableType.
      winegstreamer/video_encoder: Implement SetInputType.
      winegstreamer/video_encoder: Implement GetInputCurrentType.
      mf/tests: Add more tests for h264 encoder type attributes.
      winegstreamer/video_encoder: Introduce create_input_type.
      winegstreamer/video_encoder: Check more attributes in SetInputType.
      winegstreamer/video_encoder: Implement GetInputStreamInfo.
      winegstreamer/video_encoder: Implement GetOutputStreamInfo.
      winegstreamer/video_encoder: Rename create_input_type to video_encoder_create_input_type.
      winegstreamer/video_encoder: Clear input type when setting output type.
      winegstreamer/video_encoder: Create wg_transform.
      winegstreamer/video_encoder: Implement ProcessInput.
      winegstreamer/video_encoder: Implement ProcessMessage.
      winegstreamer/video_encoder: Use MF_ATTRIBUTES_MATCH_INTERSECTION to compare input type.
      winegstreamer/video_encoder: Set output info cbSize in SetOutputType.
      winegstreamer/wg_transform: Introduce transform_create_decoder_elements.
      winegstreamer/wg_transform: Introduce transform_create_converter_elements.
      winegstreamer/wg_transform: Support creating encoder transform.
```
