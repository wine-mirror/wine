The Wine development release 9.5 is now available.

What's new in this release:
  - Initial SLTG-format typelib support in widl.
  - Exception handling on ARM64EC.
  - Improvements to Minidump support.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/9.x/wine-9.5.tar.xz>

Binary packages for various distributions will be available
from <https://www.winehq.org/download>

You will find documentation on <https://www.winehq.org/documentation>

Wine is available thanks to the work of many people.
See the file [AUTHORS][1] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/raw/wine-9.5/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 9.5 (total 27):

 - #25207  SHFileOperation does not create new directory on FO_MOVE
 - #29523  CDBurnerXP hangs on right-clicking empty space in the file browser
 - #40613  Multiple applications require UAC implementation to run installer/app as a normal user instead of administrator (WhatsApp Desktop, Smartflix, Squirrel Installers, OneDrive)
 - #44514  Elder Scrolls Online (Dragon Bones update) requires more than 32 samplers in pixel shaders with D3D11 renderer
 - #45862  Multiple applications need MFCreateSinkWriterFromURL() implementation (Overwatch highlights, RadiAnt DICOM Viewer, Grand Theft Auto V Rockstar Editor)
 - #48085  Wine error trying to install Mono after a version update
 - #51957  Program started via HKLM\Software\Microsoft\Windows\CurrentVersion\App Paths should also be started if extension ".exe" is missing
 - #52352  YI Home installer crashes on unimplemented urlmon.dll.414
 - #52622  windows-rs 'lib' test crashes on unimplemented function d3dcompiler_47.dll.D3DCreateLinker
 - #53613  Visual novel RE:D Cherish! logo video and opening movie does not play
 - #53635  Alune Klient 14.03.2022 crashes on unimplemented function urlmon.dll.414
 - #54155  WeCom (aka WeChat Work) 4.x failed to launch.
 - #55421  Fallout Tactics launcher has graphics glitches
 - #55876  Acrom Controller Updater broken due to oleaut32 install
 - #56318  Totem Arts Launcher.exe does not open
 - #56367  Tomb Raider 3 GOG crashes at start
 - #56379  d2d1 unable to build
 - #56380  Rocket League crashes with Wine 9.3 after BakkesMod (trainer app) injects into the game
 - #56400  SSPI authentication does not work when connecting to sql server
 - #56406  wineserver crashes in set_input_desktop()
 - #56411  Failure to build wine 9.4 due to EGL 64-bit development files not found
 - #56433  QQ8.9.6 Installer crashes at very beginning due to a change in server/process.c
 - #56434  WScript.Network does not implement UserName, ComputerName, and UserDomain properties
 - #56435  capture mouse dont work in virtual desktop (work on wine 9.3)
 - #56445  d3d1-9 applications run out of memory after f6a1844dbe (ArmA: Cold War Assault, Final Fantasy XI Online, Far Cry 3)
 - #56450  Non-input USB HID devices stopped working in 9.1
 - #56458  ntdll tests skipped on win7 & win8: missing entry point kernel32.RtlPcToFileHeader

### Changes since 9.4:
```
Akihiro Sagawa (2):
      dsound/tests: Add tests for implicit MTA creation in IDirectSound::Initialize().
      dsound: Initialize MTA in IDirectSound::Initialize().

Alexandre Julliard (42):
      configure: Check the correct variable for the Wayland EGL library.
      ntdll: Implement RtlRestoreContext on ARM64EC.
      ntdll: Implement RtlWalkFrameChain on x86-64.
      ntdll: Implement RtlWalkFrameChain on i386.
      ntdll: Implement RtlWalkFrameChain on ARM.
      ntdll: Implement RtlWalkFrameChain on ARM64.
      ntdll: Implement RtlWalkFrameChain on ARM64EC.
      ntdll: Export RtlVirtualUnwind2 and RtlWalkFrameChain.
      ntdll: Share RtlCaptureStackBackTrace implementation across platforms.
      secur32/tests: Update count for new winehq.org certificate.
      ws2_32/tests: Fix a couple of failures on Windows.
      include: Define setjmpex prototype even when it's a builtin.
      ntdll/tests: Directly link to setjmp().
      ntdll/tests: Fix a backtrace test failure on Windows ARM64.
      ntdll: Add test for non-volatile regs in consolidated unwinds.
      ntdll: Port the RtlRestoreContext test to ARM64.
      ntdll: Port the RtlRestoreContext test to ARM.
      ntdll: Implement RtlUnwindEx on ARM64EC.
      ntdll: Move __C_specific_handler implementation to unwind.c.
      ntdll: Implement __C_specific_handler on ARM64EC.
      ntdll: Always use SEH support on ARM64.
      ntdll/tests: Skip segment register tests on ARM64EC.
      ntdll/tests: Fix debug register tests on ARM64EC.
      ntdll/tests: Fix a few more test failures on ARM64EC.
      ntdll: Implement RtlGetCallersAddress.
      kernelbase: Remove no longer needed DllMainCRTStartup function.
      include: Add some new error codes.
      include: Add some new status codes.
      ntdll: Add mappings for more status codes.
      jscript: Use the correct facility for JScript errors.
      netprofm: Use the correct symbols for error codes.
      msvcrt: Use floating point comparison builtins also in MSVC mode.
      ntdll/tests: Fix exception address checks in child process on ARM64EC.
      ntdll/tests: Update the KiUserExceptionDispatcher test for ARM64EC.
      configure: Don't build wow64 support in non-PE builds.
      wow64: Use a .seh handler in raise_exception().
      wow64: Always use a .seh handler in cpu_simulate().
      wow64: Access the BTCpuSimulate backend function pointer directly.
      wow64: Use a .seh handler for system calls.
      wow64cpu: Save non-volatile registers before switching to 32-bit code.
      wow64: Use setjmp/longjmp from ntdll.
      server: Add a helper to trace uint64 arrays.

Alexandros Frantzis (5):
      winewayland.drv: Implement wglCreateContextAttribsARB.
      winewayland.drv: Implement wglShareLists.
      winewayland.drv: Implement wgl(Get)SwapIntervalEXT.
      win32u: Cancel auto-repeat only if the repeat key is released.
      win32u: Cancel previous key auto-repeat when starting a new one.

Andrew Nguyen (3):
      ddraw: Reserve extra space in the reference device description buffer.
      oleaut32: Bump version resource to Windows 10.
      ddraw: Release only valid texture parents on ddraw_texture_init failure.

Andrew Wesie (1):
      wined3d: Use bindless textures for GLSL shaders if possible.

Brendan McGrath (5):
      comdlg32: Use values from DeviceCapabilities in combobox.
      comdlg32: Add resolutions to PRINTDLG_ChangePrinterW.
      comdlg32: Populate printer name on the print dialogs.
      comdlg32: Use ANSI functions in ANSI WMCommandA.
      comdlg32: Don't treat cmb1 as the printer list unless in PRINT_SETUP.

Brendan Shanks (6):
      ntdll: Remove support for msg_accrights FD passing.
      server: Remove support for msg_accrights FD passing.
      mountmgr: Replace sprintf with snprintf to avoid deprecation warnings on macOS.
      mountmgr: Replace some malloc/sprintf/strcpy calls with asprintf.
      opengl32: Replace sprintf with snprintf/asprintf to avoid deprecation warnings on macOS.
      secur32: Replace sprintf with snprintf to avoid deprecation warnings on macOS.

Daniel Lehman (2):
      oleaut32/tests: Add tests for GetSizeMax after dirty flag cleared.
      oleaut32: Return success from GetSizeMax if not dirty.

David Gow (1):
      evr/dshow: Support NV12 in evr_render.

David Heidelberg (2):
      d3d9/tests: Replace LPDWORD cast with float_to_int function.
      mailmap: Add a mailmap entry for myself with the proper name.

Dmitry Timoshkov (6):
      d2d1: Make some strings const.
      wineps.drv: Return default resolution if PPD doesn't provide the list of supported resolutions.
      kerberos: Allocate memory for the output token if requested.
      comctl32/tests: Add more tests for IImageList2 interface.
      comctl32: Implement IImageList2::Initialize().
      widl: Add initial implementation of SLTG typelib generator.

Eric Pouech (18):
      include: Update minidumpapiset.h.
      dbghelp/tests: Add tests for minidumps.
      dbghelp/tests: Add tests about generated memory chunks.
      dbghelp/tests: Add tests about minidump's callback.
      dbghelp/tests: Test exception information in minidump.
      dbghelp/tests: Add tests for function table lookup.
      dbghelp: Use an intermediate buffer in SymFunctionTableAccess (x86_64).
      dbghelp: Don't write minidump from running thread.
      dbghelp: Simplify thread info generation in minidump.
      dbghelp: Add support for V2 unwind info (x86_64).
      winedbg: Fallback to PE image when reading memory (minidump).
      winedbg: Reload module without virtual flag.
      dbghelp: No longer embed unwind information in minidump (x86_64).
      winedbg: Add ability to set executable name.
      winedbg: Extend 'attach' command to load minidump files.
      winedbg: Flush expr buffer upon internal exception.
      winedbg: Update the CPU information displayed when reloading a minidump.
      winedbg: Don't reload a minidump for a different machine.

Esme Povirk (7):
      user32/tests: Accept HCBT_ACTIVATE in TrackPopupMenu.
      gdiplus: Check bounding box in GdipIsVisibleRegionPoint.
      user32/tests: Add another missing message for TrackPopupMenu.
      gdiplus/tests: Region bounds aren't rounded.
      gdiplus: Calculate region bounding box without generating HRGN.
      user32/tests: Accept WM_ACTIVATE in TrackPopupMenu.
      user32/tests: Further updates for win11 TrackPopupMenu failures.

Gabriel Ivăncescu (1):
      shell32: Construct the proper path target for UnixFolder.

Giovanni Mascellani (1):
      d2d1: Compile the pixel shader with D3DCompile().

Hans Leidekker (3):
      dnsapi/tests: Skip tests if no CNAME records are returned.
      ntdll/tests: Fix a test failure.
      ntdll/tests: Load NtMakeTemporaryObject() dynamically.

Henri Verbeet (1):
      wined3d: Do not check the input signature element count for sm4+ shaders in shader_spirv_compile_arguments_init().

Jacek Caban (2):
      configure: Don't explicitly enable -Wenum-conversion on Clang.
      widl: Always close parsed input file.

Jinoh Kang (10):
      tools/gitlab: Run make_specfiles before building.
      ntdll/tests: Add tests for OBJ_PERMANENT object attribute.
      ntdll/tests: Add tests for NtMakeTemporaryObject.
      server: Generalize server request make_temporary to set_object_permanence.
      ntdll: Implement NtMakePermanentObject.
      ntdll/tests: Don't import kernel32.RtlPcToFileHeader.
      Revert "ntdll/tests: Load NtMakeTemporaryObject() dynamically."
      server: Check for DELETE access in NtMakeTemporaryObject().
      kernelbase: Open object with DELETE access for NtMakeTemporaryObject().
      gitlab: Output collapsible section markers in test stage script.

Kyrylo Babikov (1):
      dbghelp: Fix PDB processing using the FPO stream instead of FPOEXT.

Louis Lenders (1):
      shell32: Try appending .exe when looking up an App Paths key.

Marcus Meissner (1):
      ntdll/tests: Fix size passed to GetModuleFileNameW.

Mark Jansen (1):
      kernel32/tests: Add tests for job object accounting.

Nikolay Sivov (16):
      include: Add ID2D1Factory7 definition.
      include: Add ID2D1DeviceContext6 definition.
      d3dcompiler: Set correct compilation target for effects profiles.
      d3dcompiler: Wrap fx_4_x output in a dxbc container.
      d3d10/tests: Add a small effect compilation test.
      wshom/network: Use TRACE() for implemented method.
      wshom/network: Implement GetTypeInfo().
      wshom/network: Implement ComputerName() property.
      wshom/network: Check pointer argument in get_UserName().
      d3dx10/tests: Remove todo's from now passing tests.
      d3d10_1/tests: Remove todo from now passing test.
      wshom/network: Implement UserDomain property.
      msxml6/tests: Add some tests for MXWriter60.
      msxml/tests: Move some of the validation tests to their modules.
      msxml/tests: Move version-specific schema tests to corresponding modules.
      include: Remove XMLSchemaCache60 from msxml2.idl.

Noah Berner (1):
      advapi32/tests: Add todo_wine to tests that are currently failing.

Paul Gofman (11):
      imm32: Set lengths to 0 for NULL strings in ImmSetCompositionString().
      cryptowinrt: Force debug info in critical sections.
      diasymreader: Force debug info in critical sections.
      dsdmo: Force debug info in critical sections.
      qasf: Force debug info in critical sections.
      services: Force debug info in critical sections.
      explorer: Force debug info in critical sections.
      ntdll: Only allocate debug info in critical sections with RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO.
      server: Ignore some ICMP-originated socket errors for connectionless sockets.
      strmbase: Fallback to InitializeCriticalSection() if RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO is unsupported.
      ntdll: Don't use debug info presence to detect critical section global status.

Pavel Ondračka (1):
      d3d9/tests: Define enums outside of struct.

Piotr Caban (6):
      ntdll: Workaround sendmsg bug on macOS.
      winedump: Fix REG_DWORD dumping with no data.
      advapi32/tests: Merge RegLoadKey and RegUnLoadKey tests.
      advapi32/tests: Remove all files created by RegLoadKey tests.
      advapi32/tests: Test if modifications are saved in RegUnLoadKey.
      advapi32/tests: Remove all files created by RegLoadAppKey tests.

Robin Kertels (1):
      d3d9/tests: Skip desktop window tests if device creation fails.

Rémi Bernon (74):
      win32u: Move D3DKMT functions to a new d3dkmt.c source.
      win32u: Move D3DKMT VidPn* functions out of winex11.
      win32u: Add an adapter struct to the device manager context.
      win32u: Split writing monitor to registry to a separate helper.
      win32u: Get rid of the monitor display_device.
      win32u: Get rid of the monitor state_flags.
      win32u: Get rid of the adapter display_device.
      win32u: Get rid of the monitor flags.
      win32u: Enumerate monitors from their device keys.
      mfreadwrite/reader: Handle MF_E_TRANSFORM_STREAM_CHANGE results.
      win32u: Fix incorrect ascii key name in get_config_key.
      winevulkan: Use an rb_tree and allocate entries for handle mappings.
      winevulkan: Use a single allocation for device and queues.
      winevulkan: Pass VkDeviceQueueCreateInfo to wine_vk_device_init_queues.
      winevulkan: Rename wine_vk_physical_device_alloc parameters and variables.
      winevulkan: Use a single allocation for instance and physical devices.
      winevulkan: Get rid of the wine_vk_device_free helper.
      winevulkan: Simplify wine_vk_instance_free helper.
      winevulkan: Add handle mappings on creation success only.
      winevulkan: Get rid of the wine_vk_instance_free helper.
      mfplat/tests: Test each VIDEOINFOHEADER field conversion separately.
      mfplat/mediatype: Implement implicit MFInitMediaTypeFromVideoInfoHeader subtype.
      mfplat/mediatype: Implement MFInitMediaTypeFromVideoInfoHeader2.
      mfplat/tests: Add tests for MFInitMediaTypeFromVideoInfoHeader2.
      mfplat/mediatype: Implement MFInitAMMediaTypeFromMFMediaType for FORMAT_VideoInfo2.
      mfplat/tests: Test aperture to VIDEOINFOHEADER fields mapping.
      mfplat/mediatype: Support FORMAT_VideoInfo2 in MFInitMediaTypeFromAMMediaType.
      mfplat/mediatype: Set MF_MT_SAMPLE_SIZE from bmiHeader.biSizeImage.
      mfplat/mediatype: Map rcSource to MF_MT_(PAN_SCAN|MINIMUM_DISPLAY)_APERTURE.
      mfplat/mediatype: Set rcSource and rcTarget if stride differs from width.
      mfplat/tests: Add more MFAverageTimePerFrameToFrameRate tests.
      mfplat: Support flexible frame time in MFAverageTimePerFrameToFrameRate.
      mfplat/mediatype: Implement MF_MT_FRAME_RATE from VIDEOINFOHEADER2.
      mfplat/mediatype: Implement VIDEOINFOHEADER2 dwControlFlags conversion.
      mfplat/mediatype: Implement some VIDEOINFOHEADER2 dwInterlaceFlags conversion.
      mfplat/tests: Test that aperture is dropped with VIDEOINFOHEADER2.
      mfplat/tests: Test that MFCreateMFVideoFormatFromMFMediaType appends user data.
      mfplat/mediatype: Append user data in MFCreateMFVideoFormatFromMFMediaType.
      mfplat/tests: Check the conditions for the MFVideoFlag_BottomUpLinearRep flags.
      mfplat/mediatype: Stub MFInitMediaTypeFromMFVideoFormat.
      mfplat/tests: Add tests for MFInitMediaTypeFromMFVideoFormat.
      mfplat/mediatype: Implement MFInitMediaTypeFromMFVideoFormat.
      winebus: Add HID usages in the device descriptor when possible.
      winebus: Read hidraw device usages from their report descriptors.
      winebus: Prefer hidraw for everything that is not a game controller.
      winebus: Remove devices that are ignored wrt hidraw preferences.
      winevulkan: Allow only one vulkan surface at a time for an HWND.
      win32u: Avoid a crash when cleaning up a monitor without an adapter.
      winegstreamer: Release sink caps in the error path.
      winegstreamer: Append an optional parser before decoders.
      mfplat/tests: Add some broken results for Win7.
      mfplat/tests: Test initializing mediatype from AAC WAVEFORMATEXTENSIBLE.
      mfplat/tests: Check how AAC attributes are copied from user data.
      mfplat/mediatype: Check format pointers and sizes in MFInitMediaTypeFromAMMediaType.
      mfplat/mediatype: Support audio major type in MFInitMediaTypeFromAMMediaType.
      mfplat/mediatype: Force WAVEFORMATEXTENSIBLE in MFCreateWaveFormatExFromMFMediaType in some cases.
      mfplat/mediatype: Implement MFCreateMediaTypeFromRepresentation.
      mfplat/mediatype: Use MFCreateWaveFormatExFromMFMediaType in init_am_media_type_audio_format.
      ntoskrnl.exe: Open symbolic link with DELETE before making them temporary.
      server: Avoid removing thread twice from its desktop thread list.
      winex11: Accept key and mouse events with QS_RAWINPUT.
      server: Send WM_WINE_CLIPCURSOR message only when necessary.
      server: Send WM_WINE_SETCURSOR message only when necessary.
      win32u: Use a structure to pass peek_message arguments.
      server: Check for internal hardware messages before others.
      server: Process internal messages when checking queue status.
      user32/tests: Add missing winetest_pop_context.
      user32/tests: Add some LoadKeyboardLayoutEx tests.
      winemac: Use SONAME_LIBVULKAN as an alias for MoltenVK.
      win32u: Move vulkan loading and init guard out of the drivers.
      win32u: Move vkGet(Instance|Device)ProcAddr out the drivers.
      winevulkan: Stop generating the wine/vulkan_driver.h header.
      win32u: Move vkGet(Device|Instance)ProcAddr helpers inline.
      mfplat: Append MFVIDEOFORMAT user data after the structure padding.

Santino Mazza (2):
      mf/test: Check the topologies id's in topo loader.
      mf/topoloader: Preserve input topology id.

Vijay Kiran Kamuju (4):
      include: Add Windows.UI.ViewManagement.UIViewSettings definitions.
      urlmon: Add stub for ordinal 414.
      d3dcompiler: Add D3DCreateLinker stub.
      user32: Add LoadKeyboardLayoutEx stub.

Yuxuan Shui (8):
      shell32/tests: Check FindExecutable is looking in the correct current directory.
      shell32/tests: Check ShellExecute is looking in the correct current directory.
      shell32: PathResolve(file, NULL, ...) should not look in the current directory.
      shell32: Make sure PathResolve can find files in the current directory.
      shell32: PathResolve should be able to find files that already have extensions.
      shell32: PathResolve should remove trailing dot.
      shell32: Fix FindExecutable search path.
      shell32: Rely solely on SHELL_FindExecutable for ShellExecute.

Zebediah Figura (12):
      ntdll: Assign a primary token in elevate_token().
      d3d9/tests: Remove a no longer accurate comment.
      d3d11/tests: Add a test for using a large number of SRV resources.
      wined3d: Rename the shader_select_compute method to shader_apply_compute_state.
      wined3d: Move checking shader_update_mask to shader_glsl_apply_compute_state().
      shell32/tests: Remove obsolete workarounds from test_move().
      quartz: Implement SetVideoClippingWindow() and PresentImage() in the VMR7 presenter.
      quartz: Reimplement the VMR7 using the VMR7 presenter.
      quartz: Implement IVMRSurfaceAllocatorNotify::AdviseSurfaceAllocator().
      quartz: Return S_OK from IVMRSurfaceAllocator_PrepareSurface().
      quartz/tests: Add some tests for VMR7 renderless mode.
      wined3d: Avoid leaking string buffers in shader_glsl_load_bindless_samplers().

Zhenbo Li (1):
      shell32: Create nonexistent destination directories in FO_MOVE.

Zhiyi Zhang (5):
      mfreadwrite: Fix a memory leak (Coverity).
      win32u: Support HiDPI for the non-client close button in WS_EX_TOOLWINDOW windows.
      win32u: Fix a possible condition that makes EnumDisplayMonitors() not reporting any monitors.
      win32u: Don't enumerate mirrored monitor clones for GetSystemMetrics(SM_CMONITORS).
      win32u: Don't enumerate mirrored monitor clones when unnecessary.
```
