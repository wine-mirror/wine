The Wine development release 10.19 is now available.

What's new in this release:
  - Support for reparse points.
  - More support for WinRT exceptions.
  - Refactoring of Common Controls after the v5/v6 split.
  - Typed Arrays support in JScript.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/10.x/wine-10.19.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-10.19/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 10.19 (total 34):

 - #21483  Wine 1.1.33+ changed token security breaks .NET Framework 2.x SDK tools (debugging of managed code using 'Cordbg' and 'Mdbg')
 - #45533  Multiple games need d3dx11_43.D3DX11CreateTextureFromMemory implementation (Puyo Puyo Tetris, HighFleet, Metro 2033, Project CARS)
 - #48109  Lynx web browser hangs while starting and never shows start page
 - #51630  "Enemy Territory: Quake Wars SDK 1.5 (EditWorld)" When typing in dialog forms (values, file names, etc.) program crashes
 - #52128  Hog4PC 3.17 installer VBScript custom action needs scrrun:filesys_MoveFolder implementation
 - #52251  Airline Tycoon Demo crashes on start
 - #52371  Horizon Zero Dawn (GOG) gamepad not recognized
 - #56187  windows.ui:uisettings fails on Windows 11
 - #56935  Softube VST plugins are not drawing their UI
 - #57001  Compute shader change causes Affinity Photo 2 to crash on start up
 - #57241  Managed COM components fail to load outside of application directory
 - #57569  BeamNG.drive minimizes its window during startup, with UseTakeFocus set to false.
 - #58121  mIRC 7.81 not starting
 - #58140  ODBC using unixodbc stopped working due to regression merge between 9.0 and 10.0
 - #58320  ok() macro should not evaluate the format arguments if condition is not met.
 - #58431  Pegasus Mail Changed font (regression)
 - #58450  Total Annihilation (GOG, demo) – Wayland black screen on startup.
 - #58504  MS Office 2007 semitransparent menus
 - #58593  explorer.exe drop-down menu doesn't work
 - #58631  WINE 10.13 breaks foobar2000
 - #58650  Mouse cursor becomes invisible and unmovable
 - #58832  Grey screen in Elasto Mania II on Wine 10.17
 - #58872  Activated windows (via alt-tab) are not raised (sometimes).
 - #58876  Windowed applications cannot be minimized in virtual desktop
 - #58891  Mono's ProcessTest:Start1_FileName_Whitespace test fails
 - #58893  Window caption updates have huge delay
 - #58896  Control ultimate edition crashes with an out of vram error
 - #58906  Starcraft: Brood war is stuttering
 - #58915  Some RPG Maker MZ games experience issues with  input handling
 - #58916  winemac.drv no longer builds for i386
 - #58918  Baldur's Gate 3 : Assertion failed : "!status && "vkQueueSubmit2KHR""
 - #58930  Enemy Territory: Quake Wars SDK 1.5 launcher fails to start editor
 - #58954  Explorer has broken interface
 - #58955  Explorer has missing location bar

### Changes since 10.18:
```
Adam Markowski (1):
      po: Update Polish translation.

Aida Jonikienė (2):
      ntdll: Add SDL video driver variables to the special variables list.
      ntdll: Add SDL audio driver variables to the special variables list.

Akihiro Sagawa (3):
      quartz/tests: Add tests for 32 bpp AVI videos.
      winegstreamer: Add ARGB32 format support for 32 bpp AVI videos.
      winegstreamer: Always use bottom-up for AVI RGB videos.

Alexandre Julliard (46):
      dbghelp: Use CRT allocation functions.
      oleacc: Use CRT allocation functions.
      quartz: Use CRT allocation functions.
      shell32: Use CRT allocation functions.
      user32: Use CRT allocation functions.
      winedevice: Use CRT allocation functions.
      kernelbase: Use NtCreateThreadEx() directly in CreateRemoteThreadEx().
      ntdll: Handle the group affinity attribute in NtCreateThreadEx().
      include: Install the wine/unixlib.h header.
      schedsvc/tests: Remove Windows version check.
      taskschd/tests: Remove Windows version check.
      windows.storage: Forward some functions to shell32.
      gdi32/uniscribe: Use CRT allocation functions.
      iphlpapi: Use CRT allocation functions.
      msvcp90: Use CRT allocation functions.
      ntoskrnl: Use CRT allocation functions.
      ole32: Use CRT allocation functions.
      strmbase: Use CRT allocation functions.
      comdlg32: Use CRT allocation functions.
      faudio: Import upstream release 25.11.
      ntdll: Treat FPU_sig and FPUX_sig as void* on i386.
      kernelbase: Continue search if find_exe_file() found a directory.
      kernelbase: Avoid using wine/heap.h helpers.
      nsi: Avoid using wine/heap.h helpers.
      winspool.drv: Avoid using wine/heap.h helpers.
      winecrt0: Avoid using wine/heap.h helpers.
      include: Remove the wine/heap.h header.
      ntdll: Store special environment variables with a UNIX_ prefix.
      ntdll: Set the environment variables for Unix child processes from their UNIX_ variant.
      ntdll: Treat all the XDG_ variables as special.
      ntdll: Don't import the Unix environment variables if they are too large.
      winebuild: Don't bother to free spec file structures.
      winebuild: Don't bother to free strings built by strmake().
      winebuild: Use a generic array for apiset entries.
      winebuild: Use a generic array for entry points.
      winebuild: Use a generic array for imported functions.
      winebuild: Use a generic array for variable values.
      winebuild: Use generic arrays for resources.
      winebuild: Assign section file positions once the offset is known.
      ntdll: Don't copy WINEDLLOVERRIDES to the Windows environment.
      ntdll: Remove Wine-internal variables from the Unix environment.
      ntdll: Don't rebuild the Unix environment from the Windows one.
      ntdll: Add some new processor features definitions.
      ntdll: Use WINE_HOST_ instead of UNIX_ as environment variable prefix.
      ntdll: Don't replace WINE_HOST_ variables if they already exist in the environment.
      ntdll: Ignore some Unix variables when importing the environment.

Alistair Leslie-Hughes (1):
      wined3d: Add GPU information for AMD Radeon RX 6700 XT.

Andrew Nguyen (1):
      shell32: Retrieve shell autocompletion strings one at a time.

Anton Baskanov (12):
      dmusic: Reuse downloaded waves.
      dmsynth: Call fluid_sample_set_sound_data() with copy_data = FALSE.
      dmsynth: Implement callback support in synth_Unload().
      dmsynth: Release the waves when voices finish playing.
      dmusic: Defer releasing IDirectMusicDownload when can_free is FALSE.
      dmsynth: Allow zero-copy access to the sample data.
      dmsynth: Remove format and sample_count from struct wave.
      dmloader/tests: Add some ClearCache() tests.
      dmloader: Free the cache entries manually in loader_Release().
      dmloader: Don't remove the default collection from the cache.
      dmloader: Mark cached objects as loaded.
      dmloader: Don't use ReleaseObject() in loader_ClearCache().

Aric Stewart (1):
      mf: Return E_NOINTERFACE if service is missing.

Bernd Herd (6):
      gphoto2.ds: Progress dialog created by CreateDialog must be closed by DestroyWindow, not by EndDialog.
      sane.ds: Progress dialog created by CreateDialog must be closed by DestroyWindow, not by EndDialog.
      twain_32: Implement TWAIN feature DG_CONTROL / DAT_ENTRYPOINT / MSG_SET.
      sane.ds: When opening a DS, return the identity information of the opened device.
      sane.ds: Make comboboxes in property sheet high enough to properly drop down.
      sane.ds: Fix setting resolution in user interface.

Bernhard Übelacker (11):
      comctl32/tests: Terminate string literal by double null character (ASan).
      itss: Avoid reading beyond buffer end in ITSProtocol_Start (ASan).
      crypt32/tests: Add null character to avoid buffer overrun (ASan).
      winhttp/tests: Fix setting string lengths in WinHttpCreateUrl_test (ASan).
      iphlpapi/tests: Give GetBestRoute2 a SOCKETADDR_INET. (ASan).
      iphlpapi/tests: Remove some unneeded casts to SOCKADDR_INET.
      d3dx9_36/tests: Add end marker to the invalid tests (ASan).
      d3d9/tests: Fix value of stride passed to DrawPrimitiveUP (ASan).
      dsound: Avoid use after free in DSOUND_WaveFormat (ASan).
      d3d8/tests: Increase size of quad array to avoid buffer overflow (ASan).
      d3d9/tests: Increase size of quad array to avoid buffer overflow (ASan).

Brendan Shanks (15):
      dwrite: Stop supporting very old FreeType versions.
      win32u: Stop supporting very old FreeType versions.
      winemac: Use C99 bool instead of int for Boolean values.
      winemac: Use fallback implementation for color depth in GetDeviceCaps().
      win32u: Unique URLs instead of file paths when enumerating Mac fonts.
      opencl: Silence warnings on macOS for functions deprecated by OpenCL 1.2.
      winemac: Fix an sprintf() deprecation warning.
      winemac: Use CGDirectDisplayID in macdrv_get_monitors() and struct macdrv_adapter.
      winemac: Have convert_display_rect() return a CGRect.
      winemac: Stop using struct macdrv_display in macdrv_set_display_mode().
      winemac: Stop using macdrv_get_displays() in macdrv_get_monitors().
      winemac: Stop using macdrv_get_displays() in UpdateDisplayDevices().
      winemac: Stop using macdrv_get_displays() in ChangeDisplaySettings(), GetDeviceGammaRamp(), SetDeviceGammaRamp().
      winemac: Stop using macdrv_get_displays() in init_original_display_mode().
      winemac: Remove macdrv_get_displays().

Charlotte Pabst (2):
      ntdll/tests: Test image mapping with offset.
      ntdll: Respect offset for image mappings.

Connor McAdams (9):
      d3dx10: Downgrade invalid filter trace from an ERR to a WARN.
      d3dx10/tests: Get rid of broken() workarounds for Vista.
      d3dx10/tests: Cleanup test image definitions.
      d3dx11: Add stubs for D3DX11GetImageInfoFromResource{A,W}().
      d3dx11: Add stubs for D3DX11CreateTextureFromResource{A,W}().
      d3dx11/tests: Rearrange and reformat tests to more closely match d3dx10 tests.
      d3dx11/tests: Import test_get_image_info() from d3dx10.
      d3dx11/tests: Import test_create_texture() from d3dx10.
      d3dx11: Implement D3DX11CreateTextureFromMemory() using shared code.

Conor McCarthy (10):
      mf: Update the stream sink input type when handling an output node format change.
      mf/tests: Wait for sample delivery before checking the frame size.
      mf/tests: Synchronise media event subscription.
      mf/tests: Limit test_media_session_source_shutdown() session reuse tests.
      mf/tests: Allow WAIT_TIMEOUT after close in test_media_session_Close().
      mf/tests: Mark state comparison flaky after seek in test_media_session_seek().
      mf/tests: Check the index in test_media_sink_GetStreamSinkByIndex().
      mf/tests: Clean up at the end of test_media_session_seek().
      mf/tests: Set test media stream sample duration by default and update current time on seek.
      mf/tests: Create all MF test objects on the heap.

Elizabeth Figura (15):
      winex11.drv: Fix an inverted condition in x11drv_surface_swap().
      winex11: Trace flags with %#x.
      ntdll/tests: Fix reparse test failures.
      server: Implement FSCTL_SET_REPARSE_POINT.
      server: Implement FSCTL_DELETE_REPARSE_POINT.
      server: Implement FSCTL_GET_REPARSE_POINT.
      ntdll: Pass attr and nt_name down to lookup_unix_name().
      ntdll: Resolve IO_REPARSE_TAG_MOUNT_POINT during path lookup.
      ntdll: Implement FILE_OPEN_REPARSE_POINT.
      ntdll: Handle reparse points in NtQueryDirectoryFile().
      ntdll: Return FILE_ATTRIBUTE_REPARSE_POINT from get_file_info().
      ntdll: Fill FILE_ATTRIBUTE_REPARSE_POINT and the reparse tag in fd_get_file_info().
      dxgi/tests: Use an explicit todo flag for checking window style.
      kernelbase: Open the reparse point in CreateDirectory().
      ntdll: Resolve IO_REPARSE_TAG_SYMLINK during path lookup.

Eric Pouech (5):
      kernel32/tests: Test adding group affinity to proc/thread attributes list.
      kernelbase: Support affinity group in process/thread attributes list.
      include: Add missing process group related definitions.
      kernel32/tests: Test thread creation with group affinity attributes.
      kernelbase: Support group affinity attributes.

Erich Hoover (3):
      kernelbase: Translate FILE_FLAG_OPEN_REPARSE_POINT.
      kernelbase: Open the reparse point in DeleteFile().
      kernelbase: Open the reparse point in RemoveDirectory().

Esme Povirk (1):
      mscoree: Return S_OK from ICLRRuntimeHost_SetHostControl.

Fabian Maurer (1):
      explorer: Increase height for pathbox to show dropdown elements.

Gabriel Ivăncescu (18):
      jscript: Simplify get_flags to only check whether it's enumerable.
      jscript: Add stub implementations for typed array constructors.
      jscript: Add initial implementation of Typed Arrays.
      jscript: Expose Typed Array constructor's BYTES_PER_ELEMENT prop.
      jscript: Implement Typed Array construction on ArrayBuffers.
      jscript: Implement ArrayBuffer.isView.
      jscript: Implement Typed Array construction from objects.
      jscript: Implement 'subarray' for Typed Arrays.
      jscript: Implement 'set' for Typed Arrays.
      jscript: Implement Uint8ClampedArray.
      jscript: Expose Uint8ClampedArray only in ES6 mode.
      jscript: Return JS_E_OBJECT_EXPECTED in valueOf with NULL disps in IE10+ modes.
      mshtml: Add window.msCrypto stub.
      mshtml: Add msCrypto.subtle stub.
      mshtml: Implement msCrypto.getRandomValues.
      mshtml: Implement "arraybuffer" type response for XMLHttpRequest.
      mshtml: Use double for get_time_stamp to have sub-millisecond precision.
      mshtml: Implement performance.now().

Georg Lehmann (1):
      winevulkan: Update to VK spec version 1.4.333.

Giovanni Mascellani (19):
      mmdevapi/tests: Print the expected result code when failing a check.
      mmdevapi: Use AUTOCONVERTPCM when initializing the audio client for spatial audio.
      windows.media.speech: Use AUTOCONVERTPCM when initializing the audio client.
      winmm: Use AUTOCONVERTPCM when initializing the audio client.
      dsound: Use AUTOCONVERTPCM when initializing the capture audio client.
      dsound: Use AUTOCONVERTPCM when initializing the render audio client.
      dsound: Always require a floating-point mixing format.
      dsound: Simplify computing the mixing format.
      dsound: Do not query for mixing format support.
      dsound: Do not query for supported formats for the primary buffer.
      dxgi/tests: Add test context for D3D10 and D3D12 tests.
      dxgi/tests: Test the frame latency waitable on D3D10 too.
      dxgi/tests: Test the compatibility between the swapchain effect and the latency waitable.
      dxgi: Do not allow the frame latency waitable for non-flip swap effects.
      dxgi/tests: Test present count for D3D10, flip discard and no latency waitable.
      dxgi/tests: Test present count for D3D10, flip discard and latency waitable.
      mmdevapi/tests: Check that IsFormatSupported() sets the format to NULL when it returns E_POINTER.
      mmdevapi: Move format validation in driver-independent code.
      mmdevapi: Move generating the suggested format in the driver-independent code.

Hans Leidekker (2):
      comdlg32/tests: Force console subsystem version 5.2.
      shell32: Support SHARD_PATHW in SHAddToRecentDocs().

Haoyang Chen (5):
      user32/tests: Add EM_{SET,GET}PASSWORDCHAR tests for edit control.
      user32/edit: Allow setting password char on multiline edit controls.
      comctl32/tests: Add EM_{SET,GET}PASSWORDCHAR tests for edit control.
      comctl32/edit: Allow setting password char on multiline edit controls.
      http.sys: Support the wildcard * in the hostname.

Jacek Caban (13):
      mshtml/tests: Allow specifying compat mode and URL query string in single script mode.
      jscript: Move SCRIPTLANGUAGEVERSION_ declarations to jsdisp.idl.
      jscript: Don't expose DataView and ArrayBuffer objects in IE9 mode.
      win32u: Introduce NtUserWintabDriverCall to provide a way for wintab32 to call display drivers.
      wintab32: Attach tablet through win32u.
      wintab32: Retrieve current tablet packet through win32u.
      wintab32: Query tablet information through win32u.
      wintab32: Initialize tablet through win32u.
      winex11: Remove no longer needed unixlib.h.
      opengl32: Take GL_CLIENT_STORAGE_BIT into account when picking vk memory type in create_buffer_storage.
      opengl32: Use Vulkan-backed buffer storage only when GL_MAP_PERSISTENT_BIT is specified.
      opengl32: Don't use PTR32 for manual wow64 wrappers return type.
      opengl32: Add support for GL_NV_ES1_1_compatibility.

Jiajin Cui (4):
      winex11.drv: Implement WS_EX_TRANSPARENT mouse pass-through with ShapeInput extension.
      kernel32: Avoid some duplicate slashes in wine_get_dos_file_name.
      gdiplus: Add memory cleanup on matrix inversion failure.
      wow64win: Change parameter types from LONG to ULONG in wow64_NtUserMessageCall.

Jiangyi Chen (1):
      ole32: Add support for writing VT_R8 property.

Kun Yang (1):
      win32u: Change the stretch mode of dst hdc to ColorOnColor in TransparentBlt.

Marc-Aurel Zent (10):
      winemac: Make macdrv_ime_process_key synchronous.
      winemac: Remove QUERY_IME_CHAR_RECT and directly get ime_composition_rect.
      kernelbase: Use NT_ERROR() to check for errors in WaitForMultipleObjectsEx.
      kernelbase: Reimplement WaitForSingleObject[Ex] on top of NtWaitForSingleObject.
      ntdll: Fix an off-by-one error in the pseudo-handle check for inproc syncs.
      ntdll: Reimplement NtWaitForSingleObject without NtWaitForMultipleObjects.
      ntdll: Reject pseudo-handles in NtWaitForMultipleObjects.
      kernel32/tests: Add more tests for waits on pseudo-handles.
      server: Fix Mach vm region info datatype in write_process_memory().
      server: Correctly report partial write size in Mach write_process_memory().

Matteo Bruni (11):
      dsound/tests: Get rid of a crashing test.
      dsound: Simplify check in DSOUND_RecalcFormat().
      dsound: Print time as an unsigned value.
      dsound: Consistently trace locked byte count as an unsigned value.
      dsound: Check data size in propset methods.
      dsound: Fill all the DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE data.
      dsound: Return the mmdevapi endpoint ID as module.
      dsound/tests: Mark weird DescriptionW propset failure on the AMD testbot machine.
      wined3d: Conditionally update saved window state in fullscreen mode.
      dxgi/tests: Use test contexts in test_resize_target().
      dxgi/tests: Test window states tracking and restoration in fullscreen mode.

Mohamad Al-Jaf (8):
      windows.perception.stub/tests: Add IHolographicSpaceInterop::CreateForWindow() tests.
      windows.perception.stub: Implement IHolographicSpaceInterop::CreateForWindow().
      windows.perception.stub/tests: Add IHolographicSpace::get_PrimaryAdapterId() tests.
      windows.perception.stub: Implement IHolographicSpace_get_PrimaryAdapterId().
      windows.perception.stub: Stub some functions.
      uxtheme: Handle NULL options in DrawThemeTextEx().
      uxtheme/tests: Add some DrawThemeTextEx() tests.
      windows.media.playback.mediaplayer/tests: Skip tests when CLASS_E_CLASSNOTAVAILABLE is returned.

Nikolay Sivov (32):
      ntdll/tests: Remove some noisy traces.
      dxcore/tests: Do not test for 0 deviceID.
      rometadata: Use correct argument when unmapping a file view (Coverity).
      dwrite: Add explicit inline helper for whitespace checks.
      msxml3: Merge IXMLDocument and IXMLElement sources into a single file.
      msxml3/tests: Add some tests for root element.
      msxml3/tests: Add some tests for IXMLElement2::get_attributes().
      msxml3/tests: Add some tests for comment nodes.
      msxml3/tests: Add some tests for PI and CDATA nodes.
      include: Fix IXMLDocument method spelling.
      msxml3/tests: Add some more get_text() tests.
      d2d1/effect: Add a description for the Gaussian Blur effect.
      d2d1/effect: Add a description for the Point Specular effect.
      d2d1/effect: Add a description for the Arithmetic Composite effect.
      d2d1/tests: Reduce test run count for some tests.
      d2d1/effect: Add property definitions for the Shadow effect.
      msxml3: Rewrite legacy DOM API using SAX parser.
      d2d1: Add property descriptions for the 2D Affine Transform effect.
      d2d1/tests: Add a basic rendering test for the Flood effect.
      user32/tests: Add some tests for GetWindow(GW_ENABLEDPOPUP).
      user32: Implement GetWindow(GW_ENABLEDPOPUP).
      d2d1/tests: Add some tests for creating shared bitmaps from IWICBitmapLock.
      d2d1/effect: Add a separate factory functions for builtin effects.
      d2d1/tests: Add another effect registration test.
      gdi32: Ignore provided glyph position if ExtTextOutW() needs complex text processing.
      d2d1: Add property bindings for a few of builtin effects.
      d2d1: Add property data for the 3D Perspective Transform effect.
      d2d1: Add property data for the Composite effect.
      d2d1: Do not use property getter for standalone properties.
      d2d1: Add property data for the Crop effect.
      d2d1: Add property data for the Color Matrix effect.
      dwrite/fallback: Add the Bamum Supplement range to the fallback data.

Paul Gofman (14):
      ntdll: Implement NtAlertMultipleThreadByThreadId().
      ntdll: Use NtAlertMultipleThreadByThreadId() in RtlWakeAddressAll().
      xaudio2: Free effect chain on error return.
      xaudio2_8: Add XAudio2CreateWithVersionInfo().
      xaudio2_8: Don't crash on invalid XAPO interface.
      ntdll: Set output frame to Rsp - 8 in epilogue on x64.
      ntdll/tests: Test unwind with popping frame reg before another one on x64.
      ntdll: Fix handling jmp in epilogue unwind on x64.
      ntdll: Handle 0xff jump opcode in epilogue unwind on x64.
      winhttp/tests: Always use secure connection to ws.ifelse.io in websocket tests.
      ntdll: Fill IOSB in NtUnlockFile().
      ntdll: Don't detect epilogue in chained unwind function on x64.
      ntdll: Check for related functions in is_inside_epilog() when detecting tail call.
      win32u: Fill some GPU info in HKLM\Software\Microsoft\DirectX.

Peter Castelein (1):
      wined3d: Mark stateblock dirty in wined3d_stateblock_multiply_transform().

Piotr Caban (28):
      msvcrt: Fix WinRT exception leaks in _except_handler4_common.
      ucrtbase: Add __C_specific_handler tests.
      include: Add DBCOLUMNDESC definition.
      include: Define DBCOL_* GUIDs.
      msado15: Return error in fields:_Append if recordset is open.
      msado15: Stub object for representing tables in memory.
      msado15: Store internal field representation in fields structure.
      msado15: Support obtaining columns info from tables without provider.
      msado15: Stub IRowsetExactScroll interface for tables without provider.
      msado15: Add more _Recordset:AddNew tests.
      vccorlib140: Move exception functions to new file.
      vccorlib140: Remove platform_ prefix from exception function names.
      msvcrt: Move TYPE_FLAG* and CLASS_IS* defines to cxx.h.
      msado15: Stub IRowsetChange interface for tables without provider.
      msado15: Stub IAccessor interface for tables without provider.
      msado15: Improve Recordset::AddNew implementation.
      msado15: Improve Recordset::get_RecordCount implementation.
      vccorlib140: Fix exceptions RTTI data.
      vccorlib140: Fix exception type descriptors.
      msado15: Initilize Recordset columns lazily.
      msado15: Skip bookmark column when creating IRowset Fields.
      msado15: Stub IRowsetInfo interface for tables without provider.
      msado15: Add partial IRowsetInfo::GetProperties implementation for tables without provider.
      msado15: Improve _Recordset::CacheSize implementation.
      msado15: Implement IRowset::GetNextRows for tables without provider.
      msado15: Ignore first special column when populating Recordset.
      msado15: Improve test IRowsetInfo::GetProperties implementation.
      msado15: Fix test IRowsetLocate::GetRowsAt implementation.

Rémi Bernon (53):
      win32u: Dispatch to the KHR function pointer when appropriate.
      winex11: Flush X requests in X11DRV_SetWindowText.
      wineandroid: Check if event list is empty before checking the pipe.
      winemac: Only check if event pipe is drained with QS_ALLINPUT.
      winex11: Only check if event pipe is drained with QS_ALLINPUT.
      win32u: Don't strip semaphore / fence export info.
      win32u: Set keyed mutex semaphore stage mask value.
      win32u: Hoist get_thread_dpi in apply_window_pos.
      win32u: Use a private flag to indicate fullscreen windows.
      win32u: Get rid of is_window_rect_full_screen helper.
      winex11: Prevent larger than monitor window resizes.
      opengl32: Avoid a potential crash when flushing in wglSwapBuffers.
      win32u: Disable vsync when emulating front buffer rendering.
      winex11: Flush X requests in X11DRV_ActivateWindow.
      winex11: Flush X requests in X11DRV_SetWindowIcons.
      winex11: Flush X requests in X11DRV_SetWindowStyle.
      winex11: Flush X requests in X11DRV_SetLayeredWindowAttributes.
      winex11: Flush X requests in X11DRV_UpdateLayeredWindow.
      win32u: Merge WM_SYSCOMMAND cases in WINE_WINDOW_STATE_CHANGED.
      win32u: Don't forcefully activate windows before restoring them.
      winex11: Make set_window_cursor helper static.
      winex11: Remove unnecessary flushes from X11DRV_SetCapture.
      winex11: Flush X requests in X11DRV_ClipCursor.
      winex11: Flush X requests in X11DRV_SetCapture.
      winex11: Flush X requests in X11DRV_NotifyIMEStatus.
      winex11: Flush X requests in X11DRV_SetIMECompositionRect.
      winex11: Flush X requests in X11DRV_FlashWindowEx.
      winex11: Flush X requests in X11DRV_SystrayDockInit.
      winex11: Flush X requests in X11DRV_SystrayDockRemove.
      winex11: Flush X requests in X11DRV_SystrayDockInsert.
      winevulkan: Force copy some missing semaphore / fence structs.
      win32u: Stub NtGdiDdDDI(Signal|WaitFor)SynchronizationObjectFromCpu.
      win32u/tests: Test NtGdiDdDDI(Signal|WaitFor)SynchronizationObjectFromCpu.
      win32u/tests: Add external semaphore / fences tests.
      vulkan-1/tests: Add timeline semaphore tests.
      win32u: Consistently use "Wine Adapter" as default GPU name.
      win32u: Use the native adapter name for some common GPUs.
      win32u: Return the native OpenGL vendor name as GL_VENDOR string.
      win32u: Return the native adapter name as GL_RENDERER string.
      win32u: Add AMD Radeon RX 7600 XT device name.
      winex11: Copy XF86VidModeModeInfo to the DEVMODEW driver data.
      winex11: Always return full modes from settings handlers.
      winex11: Introduce a x11drv_mode structure for full modes.
      winex11: Retrieve full modes before applying new display settings.
      winex11: Lock and flush display in X11DRV_ChangeDisplaySettings.
      winevulkan: Move device quirks to the loader side.
      winevulkan: Enumerate instance extensions on Vulkan initialization.
      winevulkan: Check enabled instance extensions support on the PE side.
      winevulkan: Enable VK_EXT_surface_maintenance1 extension on the instance.
      winevulkan: Enumerate devices extensions on physical device initialization.
      winevulkan: Check enabled device extensions support on the PE side.
      winex11: Remove PFD_DRAW_TO_WINDOW from EGL configs not matching the display depth.
      winevulkan: Avoid using nested quotes in make_vulkan format strings.

Tim Clem (5):
      winemac.drv: Create window data for message-only windows.
      ntdll: Retrieve FP state from an xcontext in the fpe handler, rather than the ucontext.
      ntdll: Treat FPU_sig as a void* and memcpy to and from it as needed.
      winemac.drv: Explicitly add an ivar for contentViewMaskLayer.
      opengl32: Avoid null pointer dereferences when filtering extensions.

Vibhav Pant (20):
      msvcrt: Call the destructor for C++ exceptions in __C_specific_handler.
      vccorlib140/tests: Add additional tests for exception objects.
      vccorlib140: Add Platform::Exception constructor implementation.
      vccorlib140: Add Platform::Exception(HSTRING) constructor implementation.
      vccorlib140: Add Platform::COMException constructor implementation.
      vccorlib140: Add Platform::Exception::Message::get() implementation.
      vccorlib140: Add Platform::Exception::CreateException() implementation.
      vccorlib140: Throw exceptions on error paths.
      vccorlib140: Implement __abi_translateCurrentException.
      vccorlib140: Add stubs for {Get, Resolve}WeakReference.
      vccorlib140: Implement {Get, Resolve}WeakReference.
      vccorlib140: Add stub for __abi_ObjectToString.
      vccorlib140: Add ToString() stubs for well-known types.
      vccorlib140: Implement ToString() for well-known types.
      vccorlib140: Implement __abi_ObjectToString.
      vccorlib140: Implement IPrintable for Platform::Exception.
      vccorlib140: Implement IPrintable for Platform::Type.
      vccorlib140: Add stubs for delegate/EventSource helper functions.
      vccorlib140: Implement delegate helper functions.
      vccorlib140: Fix potential memory leak in allocation functions.

Wei Xie (3):
      windowscodecs: Fix potential integer overflow in PNG format reader.
      kernelbase: Allocate buffer dynamically in DefineDosDeviceW.
      kernel32/tests: Add some DefineDosDeviceW test cases.

Yeshun Ye (2):
      d2d1: Fix incorrect union member access in radial gradient brush.
      d2d1: Fix missing geometry assignment in command list recording.

Yuxuan Shui (4):
      include: Shut the compiler up about PDNS_RECORD array bounds.
      include: Don't evaluate format arguments to ok() unless we need them.
      kernel32/tests: Don't go beyond user space virtual address limit.
      winetest: Add an option for setting timeout.

Zhengyong Chen (2):
      ntdll/tests: Add NtRenameKey tests checking subkey after rename.
      windowscodecs: Fix off-by-one check in IcoDecoder_GetFrame.

Zhiyi Zhang (37):
      comctl32/tests: Test creating v5 windows after v6 manifest is deactivated.
      user32: Fix loading comctl32 v5 after comctl32 v6 is loaded.
      comctl32/tests: Test image list interoperation with comctl32 v5 and v6.
      comctl32: Use a magic value to check if an image list is valid.
      kernel32/tests: Add more GetModuleHandle() tests for WinSxS.
      ntdll: Exclude SxS DLLs when finding a DLL with its base name and no activation contexts.
      comctl32/progress: Add a helper to draw the background.
      comctl32/progress: Move the theme background rect calculation.
      comctl32/progress: Get the theme state directly from PROGRESS_INFO.
      comctl32/progress: Get the theme handle from window.
      comctl32/progress: Add a helper to check if a progress bar is smooth.
      comctl32/progress: Add a helper to get the draw procedures.
      comctl32/progress: Use COMCTL32_IsThemed() to check if theme is enabled.
      comctl32/progress: Add a helper to handle WM_THEMECHANGED.
      comctl32/progress: Remove theming for comctl32 v5.
      comctl32/propsheet: Remove theming for comctl32 v5.
      comctl32/rebar: Add a helper to draw the gripper.
      comctl32/rebar: Add a helper to draw the chevron.
      comctl32/rebar: Add a helper to draw the band separator.
      comctl32/rebar: Add a helper to draw the band background.
      comctl32/rebar: Refactor REBAR_NCPaint().
      comctl32/rebar: Remove theming for comctl32 v5.
      comctl32/status: Refactor STATUSBAR_DrawSizeGrip() to take an HWND instead.
      comctl32/status: Add a helper to draw the background for parts.
      comctl32/status: Add a helper to draw the background.
      comctl32/status: Remove theming for comctl32 v5.
      comctl32/ipaddress: Use the system window text color to draw dots.
      winex11.drv: Fix xinerama_get_fullscreen_monitors() not working correctly with non-fullscreen rects.
      comctl32/tab: Add a helper to draw the border background.
      comctl32/tab: Add a helper to draw the item theme background.
      comctl32/tab: Use COMCTL32_IsThemed() to check if theme is enabled.
      comctl32/tab: Remove theming for comctl32 v5.
      comctl32/toolbar: Add a helper to draw the separator.
      comctl32/toolbar: Add a helper to draw the button frame.
      comctl32/toolbar: Add a helper to draw the separator drop down arrow.
      comctl32/toolbar: Add a helper to check if theme is enabled.
      comctl32/toolbar: Remove theming for comctl32 v5.
```
