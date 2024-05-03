The Wine development release 9.8 is now available.

What's new in this release:
  - Mono engine updated to version 9.1.0.
  - IDL-generated files use fully interpreted stubs.
  - Improved RPC/COM support on ARM platforms.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/9.x/wine-9.8.tar.xz>

Binary packages for various distributions will be available
from <https://www.winehq.org/download>

You will find documentation on <https://www.winehq.org/documentation>

Wine is available thanks to the work of many people.
See the file [AUTHORS][1] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/raw/wine-9.8/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 9.8 (total 22):

 - #3689   Microsoft Office 97 installer depends on stdole32.tlb be in v1 (SLTG) format
 - #33270  Cursor disappears during Installshield install
 - #37885  Battle.net launcher fails to set permissions on WoW files
 - #38142  Approach fields box only show 3/4 of one line
 - #44388  gldriverquery.exe crash on wineboot and company of heroes says no 3d
 - #45035  Buttons of the Radiosure program are missing
 - #46455  Desktop syncing app for Remarkable devices crashes on startup
 - #47741  Lotus Approach: Initial "Welcome" dialog not shown on startup
 - #51361  SimSig with Wine 6.18 breaks after upgrading from libxml2 2.9.10 to 2.9.12
 - #54997  msys2: gpg.exe fails because "NtSetInformationFile Unsupported class (65)" / FileRenameInformationEx
 - #55736  Solid Edge crashes after a couple of minutes
 - #55844  Paper tray options missing.  Landscape orientation is ignored
 - #56041  iZotope Product Portal crashes
 - #56248  VTFEdit: Exception When Loading .VTF Files
 - #56309  Across Lite doesn't show the letters properly when typing
 - #56324  Falcon BMS launcher fails to start (native .Net 4.6.1 needed)
 - #56407  SaveToGame hangs during DWM initialization
 - #56472  Recettear opening movie blackscreen in Wine 9.5
 - #56581  Corsair iCUE 4: needs unimplemented function SHELL32.dll.SHAssocEnumHandlersForProtocolByApplication
 - #56598  Calling [vararg] method via ITypeLib without arguments via IDispatch fails
 - #56599  HWMonitor 1.53 needs unimplemented function pdh.dll.PdhConnectMachineA
 - #56609  vcrun2008 fails to install

### Changes since 9.7:
```
Aida Jonikienė (1):
      msvcp140_atomic_wait: Implement __std_*_crt().

Akihiro Sagawa (3):
      quartz/tests: Add tests to reject unsupported contents for MPEG splitter.
      winegstreamer: Reject unexpected formats on init.
      winegstreamer: Implement input media type enumeration in MPEG splitter.

Alex Henrie (3):
      setupapi: Don't set RequiredSize when SetupDiGetClassDescription* fails.
      shell32: Add SHAssocEnumHandlersForProtocolByApplication stub.
      pdh: Add PdhConnectMachineA stub.

Alexandre Julliard (57):
      include: Update a couple of RPC structures.
      rpcrt4: Add a wrapper for client calls from stubless proxies.
      rpcrt4: Move the FPU register conversion to the stubless proxy wrapper.
      rpcrt4: Remap registers to the stack for stubless proxies on ARM platforms.
      rpcrt4: Stop passing the actual FPU regs pointer to client call functions.
      rpcrt4: Remove obsolete version comments from spec file.
      rpcrt4: Fix stack alignment and by-value parameters for typelibs on ARM platforms.
      rpcrt4: Generate the parameter extension data for typelibs on ARM platforms.
      rpcrt4: Extend 8- and 16-bit parameters on ARM.
      oleaut32: Extend 8- and 16-bit parameters on ARM.
      rpcrt4: Use fully interpreted IDL stubs.
      rpcrt4/tests: Use fully interpreted IDL stubs.
      schedsvc: Use fully interpreted IDL stubs.
      schedsvc/tests: Use fully interpreted IDL stubs.
      sechost: Use fully interpreted IDL stubs.
      taskschd: Use fully interpreted IDL stubs.
      ntoskrnl.exe: Use fully interpreted IDL stubs.
      plugplay: Use fully interpreted IDL stubs.
      rpcss: Use fully interpreted IDL stubs.
      services: Use fully interpreted IDL stubs.
      ntdll/tests: Remove unnecessary shared header.
      dbghelp: Ignore a couple of Dwarf-3 opcodes.
      oleaut32: Fix IDispatch::Invoke for vararg functions with empty varargs.
      systeminfo: Pass proper Unicode strings to fwprintf.
      avifil32: Use fully interpreted IDL stubs.
      combase: Use fully interpreted IDL stubs.
      dispex: Use fully interpreted IDL stubs.
      ia2comproxy: Use fully interpreted IDL stubs.
      ieproxy: Use fully interpreted IDL stubs.
      mscftp: Use fully interpreted IDL stubs.
      makefiles: Support building files for x86-64 architecture on ARM64EC.
      rpcrt4: Move the stubless client thunks to a separate file.
      rpcrt4: Move the stubless delegating thunks to a separate file.
      rpcrt4: Move call_server_func() to a separate file.
      msdaps: Use fully interpreted IDL stubs.
      msi: Use fully interpreted IDL stubs.
      mstask: Use fully interpreted IDL stubs.
      netapi32: Use fully interpreted IDL stubs.
      oleacc: Use fully interpreted IDL stubs.
      sti: Use fully interpreted IDL stubs.
      urlmon: Use fully interpreted IDL stubs.
      windowscodecs: Use fully interpreted IDL stubs.
      actxprxy: Use fully interpreted IDL stubs.
      ole32: Use fully interpreted IDL stubs.
      oleaut32: Use fully interpreted IDL stubs.
      qmgrprxy: Use fully interpreted IDL stubs.
      quartz: Use fully interpreted IDL stubs.
      widl: Fix correlation offset for unencapsulated unions in interpreted mode.
      widl: Default to fully interpreted stubs mode.
      oleaut32: Move the call_method thunk to a separate file.
      vcomp: Move the fork wrapper to a separate file.
      vcomp: Fix stack alignment in the fork wrapper on ARM.
      ntdll: Build __chkstk as x86-64 code on ARM64EC.
      ntdll: Generate stub entry points as x86-64 code on ARM64EC.
      faudio: Import upstream release 24.05.
      fluidsynth: Import upstream release 2.3.5.
      png: Import upstream release 1.6.43.

Alexandros Frantzis (9):
      winex11.drv: Rename wgl_pixel_format to glx_pixel_format.
      opengl32: Implement wglDescribePixelFormat using new driver API get_pixel_formats.
      opengl32: Cache driver pixel format information.
      winewayland.drv: Enable wglDescribePixelFormat through p_get_pixel_formats.
      winex11.drv: Enable wglDescribePixelFormat through p_get_pixel_formats.
      winex11.drv: Remove unnecessary parameter from describe_pixel_format.
      winemac.drv: Enable wglDescribePixelFormat through p_get_pixel_formats.
      wineandroid.drv: Rename wgl_pixel_format to avoid name conflicts.
      wineandroid.drv: Enable wglDescribePixelFormat through p_get_pixel_formats.

Alfred Agrell (1):
      include: Use the correct GUID for DXFILEOBJ_PatchMesh.

Alistair Leslie-Hughes (2):
      include: Add atldef.h.
      windowscodecs: Avoid implicit cast changing value.

Anton Baskanov (3):
      user32/tests: Test that display settings are restored on process exit.
      winex11.drv: Process RRNotify events in xrandr14_get_id.
      explorer: Restore display settings on process exit.

Billy Laws (1):
      winevulkan: Allocate commited memory for placed mappings.

Brendan McGrath (4):
      winegstreamer: Pass uri to wg_parser when available.
      winegstreamer: Respond to the URI query.
      winegstreamer: Fix wow64 support for wg_parser_connect.
      winegstreamer: Log query after setting the URI.

Brendan Shanks (2):
      widl: Use hardcoded build time in TLB custom data.
      winemac.drv: Fix use-after-free in macdrv_copy_pasteboard_types.

Danyil Blyschak (1):
      mfreadwrite: Store result of object activation in stream transform.

Dmitry Timoshkov (11):
      widl: Make automatic dispid generation scheme better match what midl does.
      widl: Create library block index right after the CompObj one.
      widl: Set the lowest bit in the param name to indicate whether type description follows the name.
      widl: Add support for function parameter flags to SLTG typelib generator.
      widl: Fix calculation of the SLTG library block size.
      stdole32.tlb: Generate typelib in SLTG format.
      include: Add _Inout_cap_c_(count) macro.
      include: Move InterlockedExchangeAdd64() definition before its first usage.
      dssenh: Add CPSetKeyParam() stub implementation.
      rsaenh: Validate pbData in CPSetKeyParam().
      advapi32: CryptSetKeyParam() should accept NULL pbData.

Esme Povirk (4):
      mscoree: Update Wine Mono to 9.1.0.
      user32/tests: Add a flag for messages incorrectly sent by Wine.
      user32/tests: Remove a no-longer needed optional flag.
      windowscodecs: Check for overflow in jpeg_decoder_initialize.

Etaash Mathamsetty (2):
      user32: Fake success from RegisterTouchWindow.
      user32: Fake success from UnregisterTouchWindow.

Evan Tang (2):
      user32/tests: Check RegisterRawInputDevices RIDDEV_DEVNOTIFY posted messages.
      win32u: Post device arrival messages in NtUserRegisterRawInputDevices.

Gopal Prasad (2):
      winewayland.drv: Set wayland app-id from the process name.
      winewayland.drv: Implement SetWindowText.

Hans Leidekker (2):
      wintrust: Add support for the PE image hash in CryptCATAdminCalcHashFromFileHandle().
      msi: Install global assemblies after install custom actions and before commit custom actions.

Henri Verbeet (3):
      wined3d: Pass a shader_glsl_priv structure to shader_glsl_generate_fragment_shader().
      wined3d: Pass a shader_glsl_priv structure to shader_glsl_generate_compute_shader().
      wined3d: Introduce the "glsl-vkd3d" shader backend.

Jacek Caban (1):
      mshtml: Move iface_wrapper_t IUnknown implementation to htmlobject.c.

Kirill Zhumarin (1):
      ntdll: Use termios2 for serial when possible.

Matteo Bruni (9):
      wined3d: Rename WINED3DUSAGE_PRIVATE to WINED3DUSAGE_CS.
      d3d9: Don't do instanced draws in DrawPrimitive() and DrawPrimitiveUP().
      wined3d: Don't skip FFP projection transform update.
      wined3d: Don't override texture parameters for COND_NP2 on multisample textures.
      d3d9/tests: Skip test_sample_attached_rendertarget() without pixel shaders support.
      wined3d: Conditionally support WINED3D_FRAGMENT_CAP_SRGB_WRITE on the ffp fragment pipe.
      wined3d: Conditionally allow sRGB writes with the 'none' shader backend.
      d3d9/tests: Don't create a vertex shader in test_desktop_window() when unsupported.
      d3d9/tests: Test creating a texture on a NULL HWND device.

Michael Bond (1):
      shell32/shellpath: Fix UserPinned and QuickLaunch KnownFolderPaths.

Nikolay Sivov (4):
      d2d1: Update to ID2D1Factory7.
      d2d1: Update to ID2D1DeviceContext6.
      d2d1: Update to ID2D1Device6.
      d2d1: Implement newer CreateDeviceContext() methods.

Paul Gofman (12):
      ntdll: Remove entries from queue in RtlWakeAddressAll().
      ntdll: Pre-check entry->addr before taking a spin lock in RtlWaitOnAddress().
      crypt32: Mind constructor tag in CRYPT_AsnDecodeOCSPSignatureInfoCertEncoded().
      cryptnet: Do not use InternetCombineUrlW() in build_request_url().
      ntdll/tests: Add tests for CONTEXT_EXCEPTION_REQUEST.
      ntdll: Set exception reporting flags in NtGetContextThread().
      ntdll: Store exception reporting flags in server context.
      ntdll: Store exception reporting flags on suspend.
      ntdll: Store exception reporting flags for debug events.
      winex11.drv: Support _SHIFT_ARB attributes in X11DRV_wglGetPixelFormatAttribivARB().
      ntdll: Implement NtQuerySystemInformation(SystemProcessIdInformation).
      msvcrt: Implement _mbsncpy_s[_l]().

Peter Johnson (1):
      wined3d: Added missing GTX 3080 & 1070M.

Piotr Caban (4):
      windowscodecs: Support 32-bit ABGR bitfields bitmaps.
      winhttp: Fix parameters validation in WinHttpGetProxyForUrl.
      msvcr80/tests: Fix errno access in tests.
      winhttp: Use GlobalAlloc to allocate lpszProxy in WinHttpGetProxyForUrl.

Roland Häder (1):
      wined3d: Added missing GTX 1650.

Rémi Bernon (45):
      mfreadwrite/tests: Do not accept MFVideoFormat_RGB32 in the test transform.
      mfreadwrite/tests: Avoid using MFCreateMediaBufferFromMediaType.
      mfreadwrite/tests: Shutdown the test stream event queues on source shutdown.
      mfreadwrite/reader: Avoid leaking the stream transform service MFT.
      win32u: Introduce a distinct vulkan interface between win32u and the user drivers.
      win32u: Introduce a new VkSurfaceKHR wrapping structure.
      winevulkan: Pass win32u surface wrappers for each vkQueuePresent swapchain.
      win32u: Rename vulkan surface creation/destroy driver callbacks.
      win32u: Pass HWND directly to vulkan surface creation driver callback.
      win32u: Move host surface destruction out of the drivers.
      win32u: Destroy thread windows before calling driver ThreadDetach.
      winegstreamer: Set other aperture attributes on video media types.
      winegstreamer: Always set aperture attributes on video decoder output types.
      winegstreamer: Introduce a new wg_transform_create_quartz helper.
      winegstreamer: Use DMO_MEDIA_TYPE in the WMV decoder.
      mf/tests: Use a separate field for buffer_desc image size and compare rect.
      evr/tests: Sync compare_rgb32 / dump_rgb32 helpers with mf tests.
      mfmediaengine/tests: Sync compare_rgb32 / dump_rgb32 helpers with mf tests.
      winegstreamer/video_processor: Allow clearing input / output types.
      mf/tests: Move the video processor input bitmap names to the test list.
      mf/tests: Add more video processor tests with aperture changes.
      mf/session: Introduce new (allocate|release)_output_samples helpers.
      mf/session: Get session topo_node from their IMFTopologyNode directly.
      mf/session: Introduce new session_get_topo_node_output helper.
      mf/session: Introduce new session_get_topo_node_input helper.
      mf/session: Wrap samples in IMFMediaEvent list instead of IMFSample list.
      mf/session: Handle transform format changes and update downstream media types.
      winex11: Report all sources as detached in virtual desktop mode.
      win32u: Don't force refresh the display cache on thread desktop change.
      winex11: Let win32u decide when to force update the display cache.
      win32u: Introduce a new add_virtual_modes helper.
      win32u: Return the host surface directly from vulkan_surface_create.
      winewayland: Get rid of the now unnecessary surface wrapper.
      win32u: Introduce a per-window vulkan surface list.
      win32u: Move thread detach from winex11.
      winex11: Remove now unnecessary surface wrapper struct.
      win32u: Fix list corruption in vulkan_detach_surfaces.
      win32u: Remove now unnecessary rawinput_device_get_usages.
      win32u: Use find_device_from_handle in process_rawinput_message.
      win32u: Move rawinput device cache ticks check to rawinput_update_device_list.
      winex11: Don't call x11drv_xinput2_disable for foreign windows.
      winex11: Remove duplicated foreign window class string constant.
      winex11: Avoid leaking foreign window data if it was already created.
      mfreawrite/tests: Allow MF_E_SHUTDOWN result in test stream RequestSample.
      mf/tests: Add broken result for older Windows.

Tuomas Räsänen (2):
      setupapi/tests: Add tests for reading INF class with %strkey% tokens.
      setupapi: Use INF parser to read class GUID and class name.

Yuxuan Shui (3):
      shell32: Fix a trace log message.
      shell32: Use full path to current directory for finding executables.
      shell32: Restore the ability of running native unix programs with ShellExecute.

Zebediah Figura (1):
      kernelbase: Do not start the debugger if SEM_NOGPFAULTERRORBOX is set.

Zhiyi Zhang (1):
      win32u: Set the virtual desktop display frequency to 60Hz.

Ziqing Hui (11):
      winegstreamer: Merge video_cinepak into video field.
      winegstreamer: Merge video_h264 into video field.
      winegstreamer: Merge video_wmv into video field.
      winegstreamer: Merge video_indeo into video field.
      winegstreamer: Merge video_mpeg1 into video field.
      winegstreamer: Implement mf_media_type_to_wg_format_video_wmv.
      winegstreamer/video_decoder: Set input/output infos in h264_decoder_create.
      winegstreamer/video_decoder: Change decoder attributes.
      winegstreamer/video_decoder: Add wg_transform_attrs member.
      winegstreamer/video_decoder: Support aggregation.
      winegstreamer/video_decoder: Use video_decoder to implement wmv decoder.
```
