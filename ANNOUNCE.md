The Wine development release 9.12 is now available.

What's new in this release:
  - Initial support for user32 data structures in shared memory.
  - Mono engine updated to version 9.2.0.
  - Rewrite of the CMD.EXE engine.
  - Fixed handling of async I/O status in new WoW64 mode.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/9.x/wine-9.12.tar.xz>

Binary packages for various distributions will be available
from <https://www.winehq.org/download>

You will find documentation on <https://www.winehq.org/documentation>

Wine is available thanks to the work of many people.
See the file [AUTHORS][1] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/raw/wine-9.12/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 9.12 (total 24):

 - #43337  Conditional command with parentheses not working.
 - #44063  Parentheses cause cmd.exe failing to recognize '2>&1'
 - #47071  Nest or mixed "FOR" and "IF" command sometimes get different result in Wine and Windows.
 - #47798  Incorrect substring result using enableDelayedExpansion
 - #49993  CUERipper 2.1.x does not work with Wine-Mono
 - #50723  Can't recognize ... as an internal or external command, or batch script
 - #52344  Can't substitute variables as a command
 - #52879  ESET SysInspector 1.4.2.0 crashes on unimplemented function wevtapi.dll.EvtCreateRenderContext
 - #53190  cmd.exe incorrectly parses a line with nested if commands
 - #54935  Rewrite (VN): black screen videos with WMP backend and gstreamer
 - #55947  Serial port event waits should use async I/O
 - #56189  quartz:vmr7 - test_default_presenter_allocate() fails on Windows 7
 - #56389  Assassin's Creed & Assassin's Creed: Revelations do not run under new WoW64
 - #56698  SuddenStrike 3 crashes when opening the intro movie
 - #56763  Firefox 126.0.1 crashes on startup
 - #56769  Death to Spies: intro videos have audio only but no video
 - #56771  Receiving mail in BeckyInternetMail freezes
 - #56827  Window borders disappear
 - #56836  Assassin's Creed III stuck on loading screen (Vulkan renderer)
 - #56838  FL Studio 21 gui problem
 - #56839  App packager from Windows SDK (MakeAppx.exe) 'pack' command crashes due to unimplemented functions in ntdll.dll
 - #56840  Apps don't launch with wayland driver
 - #56841  virtual desktop "explorer.exe /desktop=shell,1920x1080" broken
 - #56871  The 32-bit wpcap program is working abnormally

### Changes since 9.11:
```
Aida Jonikienė (1):
      ntdll: Fix params_mask type in NtRaiseHardError().

Alex Henrie (6):
      ntdll: Fix multi-string callbacks in RtlQueryRegistryValues.
      ntdll: Double-null-terminate multi-strings when using RTL_QUERY_REGISTRY_DIRECT.
      ntdll: Fix handling of non-string types with RTL_QUERY_REGISTRY_DIRECT.
      ntdll: Don't write partial strings with RTL_QUERY_REGISTRY_DIRECT.
      ntdll: Fix type and size of expanded strings in RtlQueryRegistryValues.
      ntdll: Don't special-case default values in RtlQueryRegistryValues.

Alexandre Julliard (28):
      ntdll/tests: Add test for cross-process notifications on ARM64EC.
      kernel32/tests: Add some tests for WriteProcessMemory/NtWriteVirtualMemory.
      kernelbase: Make memory writable in WriteProcessMemory if necessary.
      kernelbase: Send cross process notifications in WriteProcessMemory on ARM64.
      kernelbase: Send cross process notifications in FlushInstructionCache on ARM64.
      kernelbase: Don't use WRITECOPY protection on anonymous mappings.
      ntdll: Add helper macros to define syscalls on ARM64EC.
      ntdll: Send cross-process notification in memory functions on ARM64EC.
      ntdll: Fix the fake 32-bit %cs value on ARM64EC.
      ntdll: Don't set the TEB ExceptionList to -1 on 64-bit.
      ntdll: Simplify preloader execution using HAVE_WINE_PRELOADER.
      ntdll: Export a proper function for RtlGetNativeSystemInformation.
      ntdll: Move RtlIsProcessorFeaturePresent implementation to the CPU backends.
      ntdll: Move the IP string conversion functions to rtlstr.c.
      makedep: Add a helper to get a root-relative directory path.
      makefiles: Hardcode the fonts directory.
      makefiles: Hardcode the nls directory.
      makefiles: Hardcode the dll directory.
      makefiles: Generate rules to build makedep.
      tools: Add helper functions to get the standard directories.
      ntdll: Build relative paths at run-time instead of depending on makedep.
      loader: Build relative paths at run-time instead of depending on makedep.
      server: Build relative paths at run-time instead of depending on makedep.
      makefiles: No longer ignore makedep.c.
      configure: Disable non-PE import libraries if compiler support is missing.
      makedep: Remove the -R option.
      makedep: Generate rules for make depend.
      makedep: Generate a compile_commands.json file.

Alexandros Frantzis (5):
      opengl32: Add default implementation for wglGetPixelFormatAttribivARB.
      opengl32: Add default implementation for wglGetPixelFormatAttribfvARB.
      winex11: Update describe_pixel_format coding style.
      winex11: Pass wgl_pixel_format to describe_pixel_format.
      winex11: Use default wglGetPixelFormatAttribivARB implementation.

Alfred Agrell (10):
      quartz: Implement AMT/WMT differences for WMV media type.
      winegstreamer: Implement AM_MEDIA_TYPE to wg_format converter for Cinepak video.
      winegstreamer: Make AVI splitter use end of previous frame if the current frame doesn't have a timestamp.
      quartz/tests: Add Cinepak test to avi splitter.
      iccvid: Reject unsupported output types.
      msvfw32/tests: Test that Cinepak rejects unsupported output types.
      quartz: Allow concurrent calls to AVI decoder qc_Notify and Receive.
      quartz/tests: Test that avi_decompressor_source_qc_Notify does not deadlock if called from a foreign thread during IMemInput_Receive.
      winegstreamer: Recalculate alignment and bytes per second, instead of copying from input.
      mf/tests: Clobber the alignment and bytes per second, to test if the DMO fixes it.

Alistair Leslie-Hughes (11):
      oledb32: Support multiple values when parsing the property Mode.
      oledb32: When creating a Data Source, handle non fatal errors.
      msado15: Use the correct version when loading the typelib.
      odbccp32: Look at the Setup key to find the driver of ODBC config functions.
      odbccp32: SQLConfigDataSource/W fix crash with passed NULL attribute parameter.
      msado15: Implement _Recordset get/put CacheSize.
      msado15: Implement _Recordset get/put MaxRecords.
      msado15: Support interface ADOCommandConstruction in _Command.
      msado15: Implement _Command::get_Parameters.
      msado15: Implement Parameters interface.
      msado15: Implement _Command::CreateParameter.

Arkadiusz Hiler (2):
      bcp47langs: Add stub dll.
      apisetschema: Add api-ms-win-appmodel-runtime-internal-l1-1-1.

Aurimas Fišeras (3):
      po: Update Lithuanian translation.
      po: Update Lithuanian translation.
      po: Update Lithuanian translation.

Biswapriyo Nath (6):
      include: Add Windows.Graphics.Capture.IGraphicsCaptureSession2 definition.
      include: Add Windows.Graphics.Capture.IGraphicsCaptureSession3 definition.
      include: Add windows.graphics.idl file.
      include: Add Windows.Graphics.Capture.GraphicsCaptureItem runtimeclass.
      include: Add Windows.Graphics.Capture.Direct3D11CaptureFrame runtimeclass.
      include: Add Windows.Graphics.Capture.Direct3D11CaptureFramePool runtimeclass.

Brendan McGrath (3):
      mf/tests: Add additional tests for MESessionClosed event.
      mf: Handle MediaSession Close when state is SESSION_STATE_RESTARTING_SOURCES.
      mf: Handle an error during Media Session Close.

Brendan Shanks (9):
      dbghelp: Add ARM/ARM64 machine types for Mach-O.
      ntdll: Make __wine_syscall_dispatcher_return a separate function to fix Xcode 16 build errors.
      configure: Don't build wineloader on macOS with '-pie'.
      configure: Remove warning when not using preloader on macOS.
      configure: Rename wine_can_build_preloader to wine_use_preloader, and also use it for Linux.
      configure: Define HAVE_WINE_PRELOADER when the preloader is being built.
      loader: Use zerofill sections instead of preloader on macOS when building with Xcode 15.3.
      winemac.drv: Fix warning in [WineWindow grabDockIconSnapshotFromWindow:force:].
      winemac.drv: Fix warning in [WineContentView viewWillDraw].

Connor McAdams (8):
      d3dx9/tests: Add tests for the source info argument of D3DXCreateTextureFromFileInMemoryEx().
      d3dx9/tests: Add more tests for loading files with multiple mip levels into textures.
      d3dx9: Refactor texture creation and cleanup in D3DXCreateTextureFromFileInMemoryEx().
      d3dx9: Use d3dx_image structure inside of D3DXCreateTextureFromFileInMemoryEx().
      d3dx9: Use struct volume inside of struct d3dx_image for storing dimensions.
      d3dx9: Add support for specifying a starting mip level when initializing a d3dx_image structure.
      d3dx9: Cleanup texture value argument handling in D3DXCreateTextureFromFileInMemoryEx().
      d3dx9: Add support for specifying which mip level to get pixel data from to d3dx_image_get_pixels().

Daniel Lehman (3):
      odbc32: Handle both directions for SQLBindParameter().
      odbc32: Numeric attribute pointer may be null if not a numeric type in SQLColAttribute().
      odbc32: StrLen_or_Ind passed to SQLBindCol can be NULL.

Danyil Blyschak (1):
      gdi32/uniscribe: Ensure the cache is initialised.

Davide Beatrici (7):
      mmdevapi: Set the default period to a minimum of 10 ms.
      winepulse: Don't set a floor for the period(s).
      mmdevapi: Return errors early in adjust_timing().
      mmdevapi: Introduce helper stream_init().
      mmdevapi: Complete IAudioClient3_InitializeSharedAudioStream.
      mmdevapi: Complete IAudioClient3_GetSharedModeEnginePeriod.
      mmdevapi: Implement IAudioClient3_GetCurrentSharedModeEnginePeriod.

Elizabeth Figura (41):
      server: Check for an existing serial wait ioctl within the ioctl handler.
      server: Directly wake up wait asyncs when the serial mask changes.
      ntdll/tests: Use NtReadFile to test WoW64 IOSB handling.
      ntdll/tests: Test IOSB handling for a synchronous write which cannot be satisfied immediately.
      ntdll/tests: Test IOSB handling with NtFlushBuffersFile.
      maintainers: Remove myself as a winegstreamer maintainer.
      ntdll: Remove the redundant filling of the IOSB in NtDeviceIoControlFile().
      ntdll: Do not fill the IOSB or signal completion on failure in cdrom_DeviceIoControl().
      ntdll: Do not fill the IOSB or signal completion on failure in serial_DeviceIoControl().
      ntdll: Do not fill the IOSB or signal completion on failure in tape_DeviceIoControl().
      ntdll: Do not fill the IOSB in NtFsControlFile() on failure.
      ntdll: Do not queue an IOCP packet in complete_async() if an APC routine is specified.
      ntdll: Move complete_async() to file.c and use it in NtWriteFileGather().
      ntdll: Use file_complete_async() in tape_DeviceIoControl().
      ntdll: Use file_complete_async() in cdrom_DeviceIoControl().
      ntdll: Use file_complete_async() in serial_DeviceIoControl().
      ntdll: Use file_complete_async() in NtFsControlFile().
      quartz/tests: Handle the case where ddraw returns system memory.
      ntdll: Do not set io->Status at the end of sock_ioctl().
      ntdll: Factor filling the IOSB into set_async_direct_result().
      ntdll: Move set_async_direct_result() to file.c.
      widl: Do not allow "lu" as an integer suffix.
      widl: Store the hexadecimal flag inside of the expr_t union.
      widl: Use struct integer for the aNUM and aHEXNUM tokens.
      widl: Respect u and l modifiers in expressions.
      quartz/tests: Test IVMRWindowlessControl::GetNativeWindowSize() on the default presenter.
      quartz: Implement IVMRWindowlessControl::GetNativeVideoSize().
      wined3d: Precompute direction and position in wined3d_light_state_set_light().
      wined3d: Pass the primary stateblock to wined3d_device_process_vertices().
      wined3d: Use the primary stateblock state in wined3d_device_process_vertices() where possible.
      wined3d: Pass the constant update mask to wined3d_device_context_push_constants().
      wined3d: Feed WINED3D_TSS_CONSTANT through a push constant buffer.
      wined3d: Feed WINED3D_RS_TEXTUREFACTOR through a push constant buffer.
      ntdll: Introduce a sync_ioctl() helper.
      ntdll: Handle WoW64 file handles in sync_ioctl().
      ntdll: Always fill the 32-bit iosb for overlapped handles, for server I/O.
      ntdll: Always fill the 32-bit iosb for overlapped handles, in file_complete_async().
      ntdll: Always fill the 32-bit iosb for overlapped handles, in set_async_direct_result().
      ntdll: Always fill the 32-bit iosb for overlapped handles, for regular read/write.
      wined3d: Recheck whether a query is active after wined3d_context_vk_get_command_buffer().
      wined3d: Implement wined3d_check_device_format_conversion().

Eric Pouech (36):
      cmd: Add more tests about FOR loops.
      cmd: Introduce helpers to handle FOR variables.
      cmd: Introduce helpers to save and restore FOR loop contexts.
      cmd: Enable '%0' through '%9' as valid FOR loop variables.
      cmd: Introduce helpers to handle directory walk.
      cmd: Split parsing from executing FOR loops for numbers (/L).
      cmd: Fix delay expansion in FOR /L loops.
      cmd: Split parsing from executing FOR loops for filesets (/F).
      cmd: Fix delay expansion in FOR loop for filesets.
      cmd: Split parsing from executing FOR loops for file walking.
      cmd: Fix delayed expansion in FOR loop on file sets.
      cmd: Remove old FOR loop related code.
      cmd: Test input has been read before using it.
      cmd: Introduce token-based syntax parser for building command nodes.
      cmd: Use kernel32's error codes instead of literals.
      cmd: Introduce return code to indicate abort of current instruction.
      cmd/tests: Add tests for delayed substitution in IF command.
      cmd: Expand delayed variables in IF operands.
      cmd: Factorize code for reading a new line for parser.
      cmd: Remove unrelated parameter to WCMD_show_prompt.
      cmd: Move prompt handling into line reading.
      cmd: Fix a couple of issues wrt. variable expansion.
      cmd: Move depth count inside builder.
      cmd: Add more tests about CALL and variable expansion.
      cmd: Fix a couple of expansions issues.
      cmd: Let redirections be handled by node instead of command.
      cmd: Move code around to avoid forward declaration.
      cmd: Let token errors be tranlatable.
      cmd: Migrate IF/FOR instructions inside CMD_NODE.
      cmd: Support help for IF and FOR commands.
      cmd: Return sub-block return code for IF and FOR.
      cmd: Use precedence in command chaining.
      cmd: Add success/failure return code tests for CALL command.
      cmd: Set success/failure return code for ECHO command.
      cmd: Set success/failure return code for CALL command.
      cmd: Implement semantic for chaining in ||, | and && operators.

Esme Povirk (2):
      user32/tests: Fix spurious "Failed sequence" reports on Windows.
      mscoree: Update Wine Mono to 9.2.0.

Fabian Maurer (1):
      wow64: In wow64_NtSetInformationToken forward TokenIntegrityLevel.

Giovanni Mascellani (6):
      wined3d: Compile the clear compute shaders at runtime.
      d3d11/tests: Check the result of compiling HLSL shaders.
      d3d11/tests: Check for NV12 texture support before testing them.
      d3d11/tests: Do not check pitches.
      d3d11/tests: Test NV12 textures without render target.
      d3d11/tests: Test UpdateSubresource() for NV12 textures.

Hans Leidekker (2):
      odbc32: Rebind parameters when the result length array is resized.
      wpcap: Handle different layout of the native packet header structure on 32-bit.

Ilia Docin (2):
      sane.ds: Add SANE option settable flag support.
      sane.ds: Improve color mode and paper source detection.

Jacek Caban (12):
      mshtml: Introduce IWineJSDispatchHost interface.
      mshtml: Rename builtin function helpers.
      mshtml: Add support for using call on builtin function objects.
      mshtml: Add support for using apply on builtin function objects.
      jscript: Fix PROP_DELETED handling in delete_prop.
      jscript: Use designated initializers for builtin_info_t.
      jscript: Use to_disp in a few more places.
      jscript: Introduce to_jsdispex.
      jscript: Consistently use jsdisp_addref and jsdisp_release.
      jscript: Use default destructor for array objects.
      jscript: Use default destructor for Object instances.
      jscript: Always free jsdisp_t in jsdisp_free.

Marc-Aurel Zent (7):
      winemac.drv: Handle length of dead keycodes in ToUnicodeEx correctly.
      winemac.drv: Do not append " dead" to dead keycodes in GetKeyNameText.
      winemac.drv: Uppercase single keys in GetKeyNameText.
      winemac.drv: Use UCCompareText in char_matches_string.
      winemac.drv: Resolve symbol vkeys first without modifiers.
      winemac.drv: Add additional German symbol vkeys.
      winemac.drv: Give dead keys a friendly name in GetKeyNameText.

Paul Gofman (8):
      mmdevapi: Store device_name as a pointer in struct audio_client.
      mmdevapi: Adjust timing after main loop start in client_Initialize().
      mmdevapi: Stub AUDCLNT_STREAMFLAGS_LOOPBACK support.
      winepulse.drv: Factor out wait_pa_operation_complete().
      winepulse.drv: Implement pulse_get_loopback_capture_device().
      mmdevapi/tests: Add test for capturing render loopback.
      ddraw/tests: Add tests for preserving d3d state during primary surface creation.
      ddraw: Preserve d3d device state in ddraw_surface_create().

Piotr Caban (3):
      kernelbase: Add GetFileMUIInfo implementation.
      kernel32/tests: Add GetFileMUIInfo tests.
      kernel32/tests: Test GetFileMUIInfo on language resource file.

Rémi Bernon (59):
      winewayland: Avoid crashing when the dummy window surface is used.
      win32u: Avoid setting the SWP_NOSIZE flag on the initial WM_DISPLAYCHANGE.
      win32u: Fix a deadlock when locking the same surface on different DCs.
      dwrite/tests: Ignore macOS specific "flip" sbix format.
      kernel32/tests: Break debugger loop on unexpected result.
      win32u: Use the vulkan functions directly from d3dkmt.
      winex11: Don't use the vulkan driver interface for xrandr.
      server: Create a global session shared mapping.
      include: Add ReadNoFence64 inline helpers.
      server: Allocate shared session object for desktops.
      server: Return the desktop object locator in (get|set)_thread_desktop.
      win32u: Open the desktop shared object in NtUserSetThreadDesktop.
      server: Move the cursor position to the desktop session object.
      server: Move the last cursor time to the desktop session object.
      win32u: Use the desktop shared data for GetCursorPos.
      win32u: Remove now unused vulkan_funcs in d3dkmt.c.
      winex11: Create a global vulkan instance for xrandr.
      include: Add a couple of CRT function declarations.
      include: Define frexpf as inline function in more cases.
      server: Mark block as writable in mark_block_uninitialized.
      server: Store the cursor clip rect in the shared data.
      server: Get rid of the global cursor structure.
      win32u: Use the shared memory for get_clip_cursor.
      server: Use a separate variable to determine the message on Alt release.
      server: Use separate functions to update the desktop and input keystates.
      server: Move the desktop keystate to shared memory.
      win32u: Use the shared data if possible for NtUserGetAsyncKeyState.
      winex11: Move window surface creation functions to bitblt.c.
      winemac: Move window surface creation functions to surface.c.
      winewayland: Move window surface creation functions to window_surface.c.
      winemac: Remove unused macdrv_get_surface_display_image copy_data parameter.
      winemac: Create a provider for the surface and a HBITMAP wrapping it.
      winemac: Create window surface CGImageRef on surface flush.
      winemac: Push window surface image updates to the main thread.
      winemac: Remove now unnecessary cocoa window surface pointer.
      winemac: Remove unnecessary surface_clip_to_visible_rect.
      server: Create a thread message queue shared mapping.
      server: Keep a reference on the desktop the hook are registered for.
      server: Move hooks struct initialization within add_hook.
      server: Update the active hooks bitmaps when hooks are added / removed.
      win32u: Read the active hooks count from the shared memory.
      win32u: Remove now unnecessary thread info active_hooks cache.
      server: Remove now unnecessary active_hooks from replies.
      winex11: Only clip huge surfaces to the virtual screen rect.
      wineandroid: Only clip huge surfaces to the virtual screen rect.
      winemac: Clip huge surfaces to the virtual screen rect.
      winewayland: Clip huge window surfaces to the virtual screen rect.
      win32u: Move the surface rect computation out of the drivers.
      win32u: Use the previous surface as default surface when compatible.
      win32u: Use the default window surface when window is not visible.
      win32u: Move layered surface attributes to the window_surface struct.
      win32u: Introduce a new helper to update layered window surface attributes.
      win32u: Pass BITMAPINFO and color bits to window surface flush.
      win32u: Introduce a new helper to get surface color info and bits.
      win32u: Get rid of the unnecessary offscreen window surface struct.
      winemac: Remove now unnecessary driver surface BITMAPINFO.
      wineandroid: Remove now unnecessary window surface BITMAPINFO.
      winewayland: Remove now unnecessary window surface BITMAPINFO.
      winex11: Remove now unnecessary window surface BITMAPINFO.

Santino Mazza (3):
      mshtml/tests: Test for IMarkupServices.
      mshtml: Implement MarkupServices_CreateMarkupContainer.
      mshtml: Implement MarkupServices_ParseString.

Torge Matthies (4):
      winegstreamer: Fix race between wg_parser_stream_en/disable and GST_EVENT_FLUSH_START/STOP.
      winegstreamer: Don't only accept segment events when streams are enabled.
      winegstreamer: Ignore an assert in wg_parser.
      winegstreamer: Handle Gstreamer pipeline flushes gracefully in the media source.

Vijay Kiran Kamuju (4):
      ntdll: Fix RtlEnumerateGenericTableWithoutSplaying function parameters.
      ntdll: Add stub RtlEnumerateGenericTableWithoutSplayingAvl function.
      ntdll: Add stub RtlNumberGenericTableElementsAvl function.
      wevtapi: Add stub EvtCreateRenderContext().

Zhiyi Zhang (9):
      profapi: Add stub dll.
      kernel32: Add AppPolicyGetWindowingModel().
      rometadata: Add initial dll.
      include: Add some windows.foundation definitions.
      include: Add some windows.system definitions.
      include: Add windows.devices.input.idl.
      include: Add windows.ui.input.idl.
      include: Add some windows.ui.core definitions.
      include: Add some windows.applicationmodel definitions.

Ziqing Hui (8):
      mf/tests: Add tests for H264 encoder types.
      winegstreamer: Implement stubs for h264 encoder.
      winegstreamer/aac_decoder: Support clearing media types.
      winegstreamer/color_converter: Support clearing media types.
      mf/tests: Add more tests for h264 encoder type attributes.
      winegstreamer/video_encoder: Implement GetOutputAvailableType.
      winegstreamer/video_encoder: Implement SetOutputType.
      winegstreamer/video_encoder: Implement GetOutputCurrentType.
```
