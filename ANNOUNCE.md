The Wine development release 11.18 is now available.

What's new in this release:
  - More NTOSKRNL support for kernel drivers.
  - A wide range of fixes for various code correctness issues.
  - Some compatibility fixes in standard C headers.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/11.x/wine-11.18.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-11.18/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 11.18 (total 21):

 - #18260  QuickTime 2.x win16 installer exits silently
 - #45104  OOB read in gdiplus
 - #51499  Assassin's Creed Rogue some textures have Normal Map effect
 - #54146  Super Meat Boy: Periodic Music Corruption
 - #57980  Adobe Creative Cloud requires unimplemented function KERNEL32.dll.SetThreadpoolTimerEx
 - #59278  Adobe Express Photos installer crashes on unimplemented function KERNEL32.dll.PackageFullNameFromId
 - #59335  Screamer 4x4 v1.2 (GOG.com) crashes after 40-50 seconds in XWayland full-screen mode
 - #59908  Reading a file: When a line is longer, End of File (EOF) is triggered
 - #59934  SlingPlayer 1.5 has a broken border
 - #60288  Enclave: brightness control no longer works
 - #60290  Mouse button handling regressions in several games when launched through Steam
 - #60291  Wine application windows react to mouse clicks outside window
 - #60292  Star Wars: Knights of the Old Republic: graphical overlay/corruption in 3D scenes after starting or loading a game
 - #60296  Return to Krondor: corrupted text and graphics in several menu screens
 - #60307  The Bard's Tale IV Director's Cut: MP4 playback broken, causing black screen and hang
 - #60311  wmic: /value after get is treated as a property name
 - #60326  mfreadwrite regression causes 'In Sound Mind' to crash during startup videos
 - #60327  Regression since "ntdll: Allocate the initial TEB after the main image is loaded."
 - #60331  arm64ec regression since "ntdll: Allocate the initial TEB after the main image is loaded."
 - #60337  Wine unable to start up after "ntdll: Store the main module handle in a global variable."
 - #60338  Wine applications such as Regedit and Notepad show Arabic when the system locale is Norwegian Nynorsk

### Changes since 11.17:
```
Alex Henrie (17):
      include: Define EXIT_FAILURE as 1, not -1.
      include: Correct the size of OUTLINETEXTMETRIC16.otmsUnderscorePosition.
      mmsystem: Correct callback arguments in timeCB3216.
      combase: Fix double free on error path in dispatch_rpc.
      mshtml: Fix double free on error path in get_nsstyle_pos.
      ieframe: Fix duplication of post data in navigate_bsc.
      setupapi: Fix "DelService" search in SetupInstallServicesFromInfSectionW.
      ntdll: Fix length computation in RtlUpcaseUnicodeStringToOemString.
      wined3d: Add device information for the NVIDIA L4 GPU.
      dwrite: Correct interface name in warning in rendertarget_DrawGlyphRun.
      mshtml: Fix format specifier in warning in HTMLDOMTextNode_splitText.
      gdiplus: Fix typo in DLL name in comment above GdipAddPathArcI.
      setupx: Correct function name in comment in VCP_UI_NodeCompare.
      vbscript/tests: Fix format specifier in error message in run_tests.
      wow64: Correct comment above wow64_NtWorkerFactoryWorkerReady.
      wnaspi32: Remove duplicate definition of INQUIRY_VENDOR from aspi.h.
      mpr: Always use heap allocation in WNetGetConnectionA.

Alexandre Julliard (28):
      fluidsynth: Fix an enum conversion compiler warning.
      server: Store the image path first in the startup info.
      ntdll: Map the main exe before allocating the process parameters.
      ntdll: Allocate the initial thread data in high memory.
      ntdll: Ignore the preloader range when allocating reserved areas.
      ntdll: Use the debug info from the thread data as soon as it's allocated.
      ntdll: Store the session id in a global variable.
      ntdll: Store the cpu count in a global variable.
      ntdll: Add a separate helper to allocate the initial thread data.
      ntdll: Set the startup_info_size global variable directly in server_init_process().
      ntdll: Allocate the initial TEB after the main image is loaded.
      ntdll: Store the redirection flag in the thread data before the TEB is created.
      ntdll: Setup large address space before allocating the first TEB.
      ntdll: Set the arm64ec bitmap pointer once the PEB is allocated.
      ntdll: Only initialize the wow64 TEB in wow64 mode.
      ntdll: Only allocate the wow64 TEB in wow64 mode.
      ntdll: Allocate initial TEB and process parameters in high memory on 64-bit.
      ntdll: Store the main module handle in a global variable.
      ntdll: Setup the DOS address space after the main module is loaded.
      ntdll: Don't try to load a 32-bit main image in high memory.
      ntdll: Don't return early from load_main_exe if the file is missing.
      ntdll: Always load builtins from ARM64 directory on ARM64EC.
      ntdll: Initialize resource loading before the activation context.
      ntdll: Use en-US as last fallback for resource lookups.
      makedep: Disable the tests of a disabled module.
      makedep: Explicitly list targets instead of using `all`.
      dmloader/tests: Skip test that only works on 32-bit.
      msvfw32/tests: Skip test that only works on 32-bit.

Alfred Agrell (2):
      include/ddraw: Fix typoed vtbl macro.
      include/dsound: Fix typoed vtbl macros.

Alistair Leslie-Hughes (14):
      winedump: Corrections for generating stub dll.
      include: Convert d3d10_1shader.h to idl.
      server: Correct sizeof usage.
      include: Fix typos in vtbl macros.
      msado15: Correct len calculation in stream_ReadText.
      include: Fix WriteProfileSection macro.
      include: Fix DI_INF_IS_SORTED defined value.
      include: Remove parameter name in ScrollConsoleScreenBufferW.
      include: Fix LPFN_WSASTRINGTOADDRESS typedef.
      include: Correct WS_AF_OSI define.
      include: Correct FORWARD__WT_PROXIMITY/_INFOCHANGE defines.
      oleaut32: Don't skip characters after 'c' in VarTokenizeFormatString.
      localspl: Correct buffer size being passed to cups_start_doc.
      oleaut32: Correctly compare the last character in VarFormatPercent.

Andrey Gusev (3):
      d3dx: Check for NULL new_data in convert_dib_to_bmp().
      msi: Check for NULL prop,val in msi_parse_command_line().
      msi: Check for NULL t in append_storage_to_db().

Anton Baskanov (11):
      dsound: Get rid of committedbuff and report the mixing position as writepos instead.
      dsound: Reuse input samples from the previous resampling iteration.
      dsound: Equalize latency for all resampling ratios.
      dsound/tests: Test that the filters are applied before resampling.
      dsound: Apply the filters before resampling.
      dmband: Handle GUID_Download and GUID_Unload.
      dmime/tests: Test that the collection is not leaked when parsing a MIDI file.
      dmime: Don't leak collection in midi_parser_parse().
      dmusic/tests: Test multiple downloads of an instrument.
      dmusic: Allow downloading an instrument to multiple ports.
      dmusic: Keep track of instrument download count.

Bernhard Kölbl (1):
      webservices: Probe the next element with the desired name before staying at the current one.

Bernhard Übelacker (4):
      winex11.drv: Use memmove in strip_driver_extra.
      comctl32/tests: Add missing newline in ok statement.
      sechost: Avoid buffer-overlow with too short strings in ConvertStringSidToSid.
      lsass: Make copies of strings to make it freeable by RPC.

Brian Carbone (1):
      winewayland: Fix dummy buffer leak.

Charlotte Pabst (2):
      mfsrcsnk/tests: Test that byte streams are read in chunks of 0x40000.
      winegstreamer: Read input in chunks of 256KiB.

Connor McAdams (10):
      include/ddk: Add missing PnP minor function code definition.
      ntoskrnl/tests: Introduce new PnP driver tests for device trees.
      ntoskrnl: Set DEVPKEY_Device_FirstInstallDate for devices.
      ntoskrnl: Add support for creating unique instance ID values based on parent device.
      setupapi: Fix device instance ID string casing.
      ntoskrnl: Set SPDRP_CAPABILITIES for PnP devices.
      ntoskrnl: Handle DevicePropertyContainerID in IoGetDeviceProperty().
      ntoskrnl: Set container ID for root PnP devices.
      ntoskrnl: Assign container IDs to PnP devices that don't report their own.
      hidclass.sys: Use the PnP manager to supply the majority of the device instance ID.

Conor McCarthy (7):
      mf/tests: Test OnClockStop() is called after adding a clock state sink.
      mf/tests: Test audio renderer MEStreamSinkStopped after OnClockStop().
      mf/tests: Test video renderer stopped and paused events after OnClock*().
      mf/clock: Don't notify a sink that is also the time source in AddClockStateSink().
      mf/evr: Always send MEStreamSinkStopped in OnClockStop().
      mf/evr: Always send MEStreamSinkPaused in OnClockPaused().
      mf/session: Ignore MEStreamSinkStopped if prerolling or starting sinks.

Daniel Lehman (2):
      msxml3/tests: Add test for XPath with namespace.
      msxml3: Fix crash when collecting elements with namespace.

Dmitry Timoshkov (4):
      crypt32/tests: Add some tests for PKCS signed message attributes.
      crypt32: Properly encode CMSG_ENCODED_MESSAGE in the PKCS signed message decoder.
      crypt32/tests: Add some comments to the signed message attributes test.
      crypt32: Use public symbolic names.

Elizabeth Figura (28):
      mountmgr: Handle IRP_MJ_CREATE for disk devices.
      ntoskrnl/tests: Test failing IRP_MJ_CREATE.
      ntdll: Do not store the file handle in async_fileio.
      server: Get rid of the comp_flags argument to create_request_async().
      server: Allow open_file_object to be async.
      server: Relay the result of IRP_MJ_CREATE.
      mountmgr: Split cdrom ioctls to a separate file.
      mountmgr: Open cdrom devices.
      mountmgr: Implement IOCTL_CDROM_READ_TOC.
      mountmgr: Implement IOCTL_CDROM_SEEK_AUDIO_MSF.
      mountmgr: Implement IOCTL_CDROM_STOP_AUDIO.
      mountmgr: Implement IOCTL_CDROM_PAUSE_AUDIO.
      mountmgr: Implement IOCTL_CDROM_RESUME_AUDIO.
      mountmgr: Implement IOCTL_CDROM_GET_VOLUME.
      mountmgr: Implement IOCTL_CDROM_SET_VOLUME.
      mountmgr: Implement IOCTL_CDROM_PLAY_AUDIO_MSF.
      d3d11: Implement DiscardResource().
      d3d11: Implement DiscardView().
      mountmgr: Implement IOCTL_CDROM_READ_Q_CHANNEL.
      mountmgr: Implement IOCTL_CDROM_RAW_READ.
      mountmgr: Implement IOCTL_CDROM_DISK_TYPE.
      mountmgr: Implement IOCTL_CDROM_GET_DRIVE_GEOMETRY.
      mountmgr: Implement IOCTL_*_MEDIA_REMOVAL and IOCTL_STORAGE_EJECTION_CONTROL.
      mountmgr: Implement IOCTL_STORAGE_EJECT_MEDIA.
      mountmgr: Implement IOCTL_*_LOAD_MEDIA.
      mountmgr: Implement IOCTL_STORAGE_RESET_DEVICE.
      mountmgr: Implement IOCTL_DVD_START_SESSION.
      mountmgr: Implement IOCTL_DVD_READ_KEY.

Eric Pouech (11):
      include: Fix some typos in wincrypt.h.
      include/msvcrt: Add missing constants in corecrt.h.
      include/msvcrt: Add some missing prototypes.
      include/msvcrt: Add missing vsnprint_s implementation.
      include/msvcrt: Add missing _mkgmtime() prototype(s).
      include/msvcrt: Add some missing C++ overloaded functions.
      include/msvcrt: Add non standard tzset() prototype.
      include/msvcrt: Define C++ exceptions only if _HAS_EXCEPTIONS is set.
      include/msvcrt: Move abs C++ overload from stdlib.h to cmath.
      include: Add missing prototype.
      include: Add some missing prototypes about C++ new handler.

Esme Povirk (9):
      gphoto2.ds: Fix grayscale palette initialization.
      ole32: Fix offset update in propertystorage_read_scalar.
      ole32: Rewrite free loop on error path.
      ole32: Handle failures in StgStreamImpl_CopyTo.
      ole32: Use common error path.
      ole32: Check for read past end of stream.
      sane.ds: Fix MSG_QUERYSUPPORT for unknown capability.
      sane.ds: Fix accidentally concatenated strings.
      windowscodecs: Check for multiplication overflow in BMP decoder.

Etaash Mathamsetty (8):
      ntoskrnl.exe: Implement PsGetThreadProcess.
      ntoskrnl.exe: Implement PsGetProcessPeb.
      ntoskrnl.exe: Implement PsGetContextThread.
      ntoskrnl.exe: Implement PsReferencePrimaryToken.
      ntoskrnl.exe: Implement MmGetPhysicalMemoryRanges.
      ntoskrnl.exe: Implement SeLocateProcessImageName.
      ntoskrnl.exe: Add semi-stub for MmGetVirtualForPhysical.
      ntoskrnl.exe: Add stub for KeCapturePersistentThreadState.

Francis De Brabandere (1):
      vbscript: Free fixed-size array data on release.

Georg Lehmann (1):
      winevulkan: Update to VK spec version 1.4.362.

Giang Nguyen (4):
      wined3d: Handle buffers without a structure byte stride in wined3d_device_context_discard_resource().
      wined3d: Use the texture level count to compute the discarded sub-resource index.
      d3d11: Release the resource returned by ID3D11View::GetResource() in DiscardView().
      d3d11/tests: Test DiscardResource() and DiscardView().

Hans Leidekker (30):
      odbc32: Remove unused pSQLBindParam function pointer.
      odbc32: Consistently use KeyValueBasicInformation in get_drivers().
      odbccp32: Fix setting the driver filename in SQLWriteDSNToIniW().
      odbccp32: Check for allocation failure in write_registry_values().
      odbccp32: Fix handle leak on error in write_registry_values().
      msi: Validate localpath length in open_package().
      msi: Handle NULL usersid/prodcode in get_deferred_action().
      msi: Handle missing SourceDir property in change_media().
      msi: Check for allocation failure in MsiLoadStringA().
      msi: Check the result of MSI_RecordGetString() in is_uninstallable().
      msi: Check for allocation failure in marshal_record().
      msi: Check for MSI_CreateRecord() failure in ITERATE_FindRelatedProducts().
      msi: Fix potential use-after-free in st_find_free_entry().
      msi: Fix memory leak on error in do_msidbCustomActionTypeDll().
      msi: Clean up on failure in MsiFormatRecordA().
      msi: Avoid underflow in MSI_GetUserInfo().
      msi: Avoid underflow in UPDATE_execute().
      mscms: Allocate dest dynamically in InstallColorProfileW().
      mscms: Fix buffer size check in GetColorProfileFromHandle().
      mscms: Fix parameter validation in CreateMultiProfileTransform().
      widl: Keep a list of winmd specific data.
      wmic: Support /value parameter.
      wmic: fgetws() takes character count.
      shell32: Navigate to folder in ItemMenu_InvokeCommand() if hosted by a shell browser.
      winscard: Handle NULL states in SCardGetStatusChangeW().
      winscard: Remove duplicate assignment.
      winscard: Check names_len parameter in SCardStatusA/W().
      rsaenh: Set last error before returning in export_public_key_impl().
      rsaenh: Handle get_key_container() failure in RSAENH_CPSetKeyParam().
      ncrypt: Validate data parameter in NCryptImportKey().

Jacek Caban (12):
      ntdll: Use correct Pc offset in ARM64EC process_breakpoint_handler.
      ntdll: Avoid exposing ARM64 call_user_mode_callback context to the client side.
      winegcc: Pass target CPU to build_tool_name.
      winebuild: Use x86_64 assembly for arm64ec targets.
      configure: Disable 32-bit only crt modules on 64-bit targets.
      configure: Disable 32-bit only Direct Music modules on 64-bit targets.
      configure: Disable 32-bit only Direct Play modules on 64-bit targets.
      configure: Disable 32-bit only OLE modules on 64-bit targets.
      configure: Disable more 32-bit only modules on 64-bit targets.
      configure: Disable d3d8.dll on 64-bit targets.
      kernelbase: Remove ARM64EC assembly version of RaiseException.
      winegcc: Use -target compiler argument when targeting a different CPU than the main target.

Marc-Aurel Zent (1):
      imm32: Cancel IME composition on hkl deactivation.

Marcus Meissner (3):
      lsass: Use boolean for checking, not pointer.
      lsass: Assign result of realloc.
      msv1_0: Free cmd on exit.

Matteo Bruni (30):
      d3dx9: NULL the out pointer for unsupported interfaces in ID3DXBuffer QueryInterface().
      include: Add missing PURE decoration on CloneMesh.
      d3dx9/effect: Fix object array element release.
      d3dx: Introduce a linear_color_from_format() helper.
      d3dx: Introduce a format_from_linear_color() helper.
      d3dx9: Fix linearization in box filtering.
      d3d10: Compute dependent properties for all the state variables.
      d3dx9: Add missing error checks in wide string allocations.
      include: Add missing PURE decoration on GetFunction[ByName].
      d3dx: Get rid of a duplicated declaration.
      d3dx10: Add missing wide string allocation error check in D3DX10CreateAsyncFileLoaderA().
      d3dx11: Add missing wide string allocation error check in D3DX11CreateAsyncFileLoaderA().
      d3dx9: Fix potential leak in OptimizeInplace()'s failure path.
      d3d10: Add a comment for intentional behavior in pres_not() and pres_or().
      d3dx9: Fix sampler parameters release.
      d3dx9: Free effect on creation failure.
      d3dx9: Always initialize effect output parameter in D3DXCreateEffect*().
      d3dx9/tests: Add a couple missing effect creation checks.
      d3dx9/tests: Release effects created because of todo_wine.
      d3dx9/mesh: Validate skin weights data more strictly.
      d3dx9/effect: Validate parameter object id.
      d3dx9/effect: Downgrade OOB effect value traces from FIXME() to WARN().
      d3dx9/tests: Test binary effect with invalid pass index in resource.
      d3dx: Validate RLE-compressed TGA data more strictly.
      d3dx9/mesh: Fix normal data size validation.
      d3dx9/mesh: Don't overflow vertex declaration buffer in SetDeclaration().
      d3dx9/mesh: Don't overflow vertex declaration buffer in UpdateSemantics().
      d3dx9/mesh: Don't overflow vertex declaration buffer in D3DXCreateMesh().
      d3dx9/mesh: Add bounds checking in declaration_equals().
      d3dx9/tests: Add some tests for overflowing vertex declarations.

Michel Bernert (1):
      imm32: Fix malformed messages in ImeToAsciiEx().

Nello De Gregoris (11):
      ntoskrnl.exe/tests: Add tests for PsGetThreadProcess.
      ntoskrnl.exe/tests: Add tests for PsGetProcessPeb.
      ntoskrnl.exe/tests: Add tests for PsGetContextThread.
      server: Allow creating token kernel objects.
      ntoskrnl.exe/tests: Add tests for token kernel objects.
      ntoskrnl.exe: Implement PsDereferencePrimaryToken.
      ntoskrnl.exe/tests: Add tests for PsReferencePrimaryToken.
      ntoskrnl.exe/tests: Add tests for MmGetPhysicalMemoryRanges.
      ntoskrnl.exe/tests: Add tests for SeLocateProcessImageName.
      ntoskrnl.exe: Implement PsGetProcessImageFileName.
      ntoskrnl.exe/tests: Add tests for PsGetProcessImageFileName.

Nikola Kuburović (2):
      ntdll: Fix out-of-bounds access in NtQueryInformationToken.
      ntdll: Add missing token information classes.

Nikolay Sivov (30):
      include: Fix pointer types in qos2.h.
      include: Fix PST_RS422 constant name.
      include: Remove duplicated macros from wingdi.h.
      include: Fix typo in IOCTL_AVIO_MODIFY_STREAM definition.
      include: Fix a typo in IMAPISession_MessageOptions() macro.
      d3dx10/tests: Add some more tests for sprite state restoration.
      d3dx10/tests: Test batched sprite drawing when interleaved with immediate one.
      d3dx10/tests: Test immediate sprite drawing outside of Begin-End.
      d3dx10/sprite: Make DrawSpritesImmediate() work outside of Begin-End.
      kernel32/tests: Add some tests for PackageFullNameFromId().
      kernelbase: Partially implement PackageFullNameFromId().
      wined3d/spirv: Fix a typo in bitwise mask test.
      wined3d: Add missing mutex release in failure paths of swapchain initialization.
      wined3d: Remove duplicated code block.
      wined3d: Remove unnecessary assignment.
      d3drm: Access created objects only on successful paths.
      d3drm: Use correct interface methods macros.
      include: Fix some typos in methods macros of D3DRM interfaces.
      include: Fix PST_RS449 value.
      mfreadwrite: Fix end-of-segment notifications for MF_SINK_WRITER_ALL_STREAMS.
      mfreadwrite/reader: Fix async command objects leak.
      mshtml: Use correct method names in error messages.
      mfreadwrite/reader: Initialize operation object refcount.
      wpcap: Remove duplicated trace call.
      xmllite/writer: Improve failure handling in WriteRawChars().
      winegstreamer/aac: Fix a copy-paste mistake in SetOutputType().
      winegstreamer: Fix a copy-paste mistake in internal video type mapping helper.
      winegstreamer: Fix typos in trace messages.
      d2d1: Add Premultiply effect stub.
      d2d1: Add 3D Transform effect stub.

Paul Gofman (13):
      ddraw/tests: Add tests for _ProcessVertices with vertex blending.
      wined3d: Support vertex blending for position in process_vertices_strided().
      ddraw/tests: Add test for lighting in _ProcessVertices with vertex blending.
      wined3d: Support vertex blending for lighting in process_vertices_strided().
      ntdll/tests: Fix test failures in test_extended_context() on current up to date Win11 (25H2).
      amstream: Unblock ddraw_mem_allocator_GetBuffer() and return VFW_E_NOT_COMMITTED when stream is flushing.
      ntdll/tests: Fix test failures in test_copy_context() on current up to date Win11 (25H2).
      ntdll/tests: Fix test failures with exception reporting flags on current up to date Win11 (25H2).
      ntdll: Send debug event exceptions as exception space.
      Revert "kernelbase: Don't modify non-volatile regs in RaiseException() on x64.".
      ntdll/tests: Fix termination_handler parameter definition.
      ntdll/tests: Expect xstate in context in test_wow64_context().
      server: Consider _MOVE_NOCOALESCE and _ABSOLUTE flags when merging mouse events.

Rémi Bernon (48):
      opengl32: Move default FBO buffer restore helper around.
      opengl32: Restore the default FBOs in pop_default_fbo_buffers.
      opengl32: Restore default FBO read/draw buffers after presenting.
      win32u: Remove unnecessary FBO read / draw buffers initialization.
      winex11: Don't ignore some focus events wrt. XI2 raw mouse events.
      winex11: Keep root window XI2 event mask in x11drv_thread_data.
      winex11: Restore XI_ButtonPress event instead of raw button events.
      opengl32: Add some traces to wglChoosePixelFormatARB.
      opengl32: Set the null function table when clearing current context.
      winex11: Restore gamma ramp driver entry points.
      resampledmo: Don't use AVRational as time base.
      imm32: Explicitly link against kernel32 before kernelbase.
      win32u: Split drivers make_current into context_activate / thread_cleanup.
      win32u: Allocate opengl_context structs in the drivers.
      win32u: Set glReserved2 to the current client or internal context.
      win32u: Set the internal context drawables when activating.
      winemac: Get rid of macdrv_context specific drawable members.
      win32u: Avoid setting drawable current if not necessary.
      winemac: Use the latched client surface toplevel window.
      winemac: Flush GL commands when activating another context.
      winemac: Make sure all contexts are double buffer capable.
      win32u: Make sure glDrawArrays has an active VAO.
      win32u: Keep the size of the opengl_drawable implementation in the funcs.
      win32u: Keep the size of the client_surface implementation in the funcs.
      win32u: Keep the size of the window_surface implementation in the funcs.
      win32u: Pass the pbuffer drawable size to opengl_drawable_create.
      win32u: Pass a SIZE to pbuffer_create instead of width / height.
      win32u: Use the opengl_drawable virtual_size for the pbuffers dimension.
      winemac: Remove unnecessary type alias for WineEventQueue.
      winemac: Remove unnecessary type alias for WineStatusItem.
      winemac: Remove unnecessary type alias for CAMetalLayer.
      winemac: Remove unnecessary type alias for WineMetalView.
      winemac: Remove unnecessary type alias for WineOpenGLContext.
      winemac: Remove unnecessary type alias for id<MTLDevice>.
      winemac: Remove unnecessary type alias for id<WineMetalSwapchain>.
      winemac: Remove unnecessary type alias for WineContentView.
      winemac: Remove unnecessary type alias for WineWindow.
      win32u: Avoid a dump_extensions unused warning.
      win32u: Release previous drawables after setting the context drawables.
      win32u: Check for any internal context in context_sync_drawables.
      win32u: Simplify context_sync_drawables control flow in the noop case.
      win32u: Set the context drawables right after activation succeeds.
      opengl32/tests: Test viewport initialization with pbuffer DC.
      opengl32: Initialize context viewport in init_client_context.
      win32u: Move the flush_memory_dc helper before create_memory_pbuffer.
      win32u: Initialize memory DC directly in create_memory_pbuffer.
      win32u: Use explicit *_LEFT buffer in wglBindTexImageARB.
      opengl32: Avoid unix-side crashes without an active context.

Santino Mazza (1):
      cmd: Use wine_dbgstr_w to prevent overflows when logging enabled.

Sean Reid (1):
      winebus.sys: Truncate input reports to their declared length.

Tobiasz Laskowski (3):
      kernel32/tests: Check case for GetTempFileName.
      kernel32/tests: Check digit case in temp file hex.
      kernel32: Use upper case hex in temp files names.

Vibhav Pant (15):
      rometadata/tests: Add additional tests for IMetaDataImport::{EnumCustomAttributes, GetCustomAttributeProps}.
      rometadata: Add a varargs helper function to get multiple table columns.
      rometadata: Factor creation of TOKEN_ENUM_LIST token enumerators into token_array.
      rometadata: Implement IMetaDataImport::{EnumCustomAttributes, GetCustomAttributeProps}.
      rometadata: Implement IMetaDataImport::{EnumInterfaceImpls, GetInterfaceImplProps}.
      rometadata/tests: Add tests for IMetaDataImport::{EnumTypeRefs, GetTypeRefProps}.
      rometadata: Implement IMetaDataImport::{EnumTypeRefs, GetTypeRefProps}.
      vccorlib140/tests: Add tests for Platform::Object::Object().
      vccorlib140: Implement Platform::Object::Object().
      rometadata/tests: Add additional tests for IMetaDataImport::FindTypeRef.
      rometadata: Implement IMetaDataImport::FindTypeRef.
      rometadata/tests: Add tests for IMetaDataImport2::{EnumGenericParams, GetGenericParamProps}.
      rometadata: Set out to NULL when QueryInterface fails.
      rometadata/tests: Add tests for IMetaDataAssemblyImport::GetAssemblyRefProps.
      vccorlib140: Use the correct prefix in IPrintable RTTI descriptors for exceptions.

Yuxuan Shui (3):
      mfreadwrite: Fix callback invocation in ReadSample error path.
      mfreadwrite: Make sure ASYNC_READ op can't start until ASYNC_SEEK finishes.
      mfreadwrite/tests: Check if and how OnReadSample is called when no stream is selected.

Zhiyi Zhang (8):
      user32/tests: Add more tests for CreateCompatibleBitmap() with display DCs.
      win32u: Add some special cases for CreateCompatibleBitmap() with a display DC.
      user32/tests: Move GetDIBits() tests with display DCs from gdi32.
      win32u: Allow 8-bit DDBs with 8-bit display DCs in NtGdiGetDIBitsInternal().
      user32/tests: Test that WM_PRINT PRF_CHILDREN sets a clip region.
      win32u: Set a clip region when handling WM_PRINT PRF_CHILDREN.
      user32/tests: Test WM_PRINT with a child window that has a non-client area.
      win32u: Fix drawing the non-client area for child windows with WM_PRINT.
```
