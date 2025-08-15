The Wine development release 10.13 is now available.

What's new in this release:
  - Windows.Gaming.Input configuration tab in the Joystick Control Panel.
  - ECDSA_P521 and ECDH_P521 algorithms in BCrypt.
  - OpenGL WoW64 thunks are all generated.
  - Still more support for Windows Runtime metadata in WIDL.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/10.x/wine-10.13.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-10.13/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 10.13 (total 32):

 - #21864  Default paper size A4 instead of my printers default
 - #32334  Microsoft SQL Server Management Studio Express 2005: Connection window is too narrow
 - #44066  mintty/msys2 doesn't work since wine 2.5.0 (named pipes)
 - #50174  Microsoft Office 365 login page for activating Office is blank
 - #52844  Multiple games stuck/crash with a black screen after/before intro (Call of Duty: Black Ops II, Nioh 2 - The Complete Edition)
 - #54157  dir command of cmd fails on Z: on Ubuntu under WSL
 - #56246  Regarding the color depth of BMP in the SavePicture method: the value is unstable.
 - #56697  _kbhit ignores the last event in the queue
 - #56754  Amazing Adventures 2 CD: bundled demo launchers fail to launch game
 - #56883  DualSense bumpers registering as two buttons on wine 9.9 and later
 - #57115  CEF sample application "Draggable" test fails
 - #57116  Crash during codecs test on CEF sample application in 64-bit wineprefix (widevinecdm)
 - #57130  CEF sample application WebGL test fails
 - #57131  Info and profile buttons in CEF sample application instantly close
 - #57458  FL Studio logo appears on the top left of the screen and with a black background
 - #57648  Wrong Cursor on Wayland
 - #57783  Approach will not run in a virgin 10.0 wineprefix
 - #58122  WSAENOTSOCK error when calling winsock.Send() on duplicated socket
 - #58389  Wrong path, app fails to start (regression): err:environ:init_peb starting L"\\\\?\\unix\\home\\user\\krita-x64-5.2.9-setup.exe
 - #58393  FlexiPDFfails to run in wine-10.10
 - #58396  virtual terminal captures mouse
 - #58425  video/x-h264 alignment=au caps causes artifacts and crashes with streams that have NALUs split across buffers
 - #58448  Metasequoia 3.1.6 OpenGL regression
 - #58459  Doom 3: BFG Edition fails to start
 - #58477  Some VST plugins fail
 - #58488  Bejeweled 3: black screen on start
 - #58493  Gothic and Gothic II crash with Access Violation
 - #58497  Strings are a confusing mix of US and British English
 - #58500  64-bit "Plain Vanilla Compiling" fails
 - #58528  CRYPT_AcquirePrivateKeyFromProvInfo does not check machine store for private key
 - #58549  Call of Duty: Black Ops II has no sound
 - #58571  On NetBSD, the case-insensitive mechanism for filenames appears to be broken

### Changes since 10.12:
```
Adam Markowski (2):
      po: Update Polish translation.
      po: Update Polish translation.

Akihiro Sagawa (1):
      ntdll: Remove redundant fusefs detection for NetBSD.

Alexandre Julliard (39):
      winebrowser: Use wine_get_unix_file_name() instead of wine_nt_to_unix_file_name().
      makefiles: Don't try to install symlinks for programs if not supported.
      ntdll: Share more filename string constants.
      ntdll: Move the helper to build NT pathnames to file.c.
      ntdll: Add a ntdll_get_unix_file_name() helper.
      win32u: Use the ntdll_get_unix_file_name() helper.
      winemac.drv: Use the ntdll_get_unix_file_name() helper.
      winex11.drv: Use the ntdll_get_unix_file_name() helper.
      winspool: Use the ntdll_get_unix_file_name() helper.
      mountmgr: Perform the filename conversion to the Unix side when setting shell folders.
      mountmgr: Perform the filename conversion to the Unix side when querying shell folders.
      ntdll: Add a private info class in NtQueryInformationFile() to return the Unix file name.
      kernel32: Reimplement wine_get_unix_file_name() using WineFileUnixNameInformation.
      kernel32: Remove leftover debug traces.
      ntdll: Get rid of the wine_nt_to_unix_file_name syscall.
      ntdll: Remove some commented stubs that no longer exist in recent Windows.
      ntdll: Add NtAccessCheckByTypeAndAuditAlarm() and NtCloseObjectAuditAlarm() stubs.
      ntdll: Add more LPC stubs.
      kernel32/tests: Fix a couple of test failures.
      winebuild: Unify the get_stub_name() and get_link_name() helpers.
      winebuild: Support -syscall flag for stubs.
      ntdll: Add stubs for some syscalls that need explicit ids.
      kernel32/tests: Fix some test failures on Windows.
      msvcrt: Remove __GNUC__ checks.
      vcomp: Remove __GNUC__ checks.
      krnl386: Remove __i386__ checks.
      mmsystem: Remove __i386__ checks.
      system.drv: Remove __i386__ checks.
      win87em: Remove __i386__ checks.
      ntdll: Remove trailing backslashes from NT names.
      kernelbase: Get the CPU count from SYSTEM_CPU_INFORMATION.
      light.msstyles: Update generated bitmaps.
      joy.cpl: Use aligned double type to avoid compiler warnings.
      server: Add a helper to check if a thread is suspended.
      wbemprox: Get the CPU count from the PEB.
      taskmgr: Get the CPU count from the PEB.
      ntdll/tests: Get the CPU count from the PEB.
      kernel32/tests: Get the CPU count from the PEB.
      cmd/tests: Fix cleanup of created files.

Alfred Agrell (2):
      dsound/tests: Add tests for IDirectSoundBuffer_Lock.
      dsound: Improve IDirectSoundBufferImpl_Lock handling of invalid arguments.

Alistair Leslie-Hughes (7):
      msado15: Support all Fields interfaces.
      msado15: Support all Field interfaces.
      msado15: Support all Connection interfaces.
      msdasql/tests: Allow database tests to run as normal user.
      msado15/tests: Check return value (Coverity).
      msado15: Implement ADOConnectionConstruction15::get_DSO.
      msado15/tests: Fixup error return values for the ConnectionPoint tests.

Ally Sommers (1):
      ws2_32: Add afunix.h header.

Andrey Gusev (1):
      wined3d: Add NVIDIA GeForce RTX 4060 Mobile.

Arkadiusz Hiler (2):
      winebus: Add Logitech G920 mapping to the SDL backend.
      winebus: Don't consider wheels / flight sticks as gamepads.

Aurimas Fišeras (1):
      po: Update Lithuanian translation.

Bernhard Übelacker (1):
      cfgmgr32/tests: Load imports dynamically to allow execution on Windows 7.

Billy Laws (2):
      ntdll/tests: Add THREAD_CREATE_FLAGS_BYPASS_PROCESS_FREEZE test.
      ntdll: Support THREAD_CREATE_FLAGS_BYPASS_PROCESS_FREEZE flag.

Brendan McGrath (9):
      mf/tests: Test when SAR requests a new sample.
      mf/tests: Test sequence of calls during a Pause and Seek.
      mf: Restart transforms and sinks on seek.
      mf: Don't send MFT_MESSAGE_NOTIFY_START_OF_STREAM when seeking.
      mfmediaengine: Don't perform implicit flush on state change.
      mfmediaengine: Request sample if we are seeking.
      mf/tests: Test H264 decoder when duration and time are zero.
      mf/tests: Test WMV decoder when duration and time are zero.
      winegstreamer: Correct duration if provided value is zero.

Brendan Shanks (6):
      win32u: Remove Mac suitcase/resource-fork font support.
      xinput1_3: Correctly handle a NULL GUID parameter in XInputGetDSoundAudioDeviceGuids().
      xinput9_1_0: Implement by dynamically loading and calling xinput1_4.dll.
      ntdll: Ensure %cs is correct in sigcontext on x86_64 macOS.
      winevulkan: Enable VK_EXT_swapchain_maintenance1 when available.
      win32u: Create Vulkan swapchains with VkSwapchainPresentScalingCreateInfoEXT when the surface will be scaled.

Connor McAdams (7):
      d3dx9: Replace D3DFORMAT constants with enum d3dx_pixel_format_id constants.
      d3dx9: Introduce d3dx_resource_type enumeration.
      d3dx9: Introduce d3dx_image_file_format enumeration.
      d3dx9: Move functions intended for code sharing into a separate source file.
      d3dx9: Get rid of ID3DXBuffer usage in d3dx_helpers.
      d3dx9: Don't include d3dx9 header in d3dx_helpers.
      d3dx10: Use shared d3dx code in get_image_info when possible.

Conor McCarthy (3):
      mfplat/tests: Add NV12 650 x 850 to image_size_tests.
      mf/tests: Add a video processor NV12 test with a width alignment of 2.
      winegstreamer: Do not pass a sample size to wg_transform_read_mf().

Csányi István (1):
      winebus.sys: Fix DualSense BT quirk.

Daniel Lehman (4):
      ucrtbase/tests: Move cexp tests from msvcr120.
      ucrtbase: Add carg implementation.
      ucrtbase: Add cargf implementation.
      msvcp140_atomic_wait: Add __std_execution_* functions.

Dmitry Timoshkov (1):
      windowscodecs: Propagate ::CopyPixels() return value.

Elizabeth Figura (18):
      quartz/tests: Create separate IEnumPins instances.
      qasf/tests: Test AllocateStreamingResources() error propagation.
      mf/tests: Test IMediaObject::AllocateStreamingResources().
      winegstreamer: Return S_OK from AllocateStreamingResources().
      wined3d/glsl: Transpose the bump environment matrix.
      Revert "wined3d/glsl: Transpose the bump environment matrix.".
      d3d9/tests: Add comprehensive D3DTSS_TEXTURETRANSFORMFLAGS tests.
      wined3d: Handle all invalid values in compute_texture_matrix().
      wined3d: Pass the attribute coordinate count to get_texture_matrix().
      wined3d: Pass 3 as the attribute count for generated texcoords.
      wined3d: Alter the texture matrix even for non-projected textures.
      wined3d: Copy the projective divisor in the FFP vertex pipeline.
      wined3d: Initialize all remaining FFP texture coordinates to zero.
      wined3d: Always divide 1.x projected textures by W for shaders.
      wined3d: Always divide 1.x projected textures by W in the FFP.
      dxcore: Reset the factory object on destruction.
      dxcore: Separate a dxcore_adapter_create() helper.
      maintainers: Add dxcore to the D3D section.

Eric Pouech (7):
      winedump: Fix crash while dumping CLR blobs.
      cmd/tests: Add a couple of tests about return code propagation.
      cmd: Fix exit code when run with /C command line option.
      cmd: Factorize some code.
      cmd: Use a context when handling input from command line (/c, /k).
      cmd: Separate command file handling from external commands.
      cmd: Fix exit code in cmd /c when leaving nested command files.

Esme Povirk (9):
      comctl32/tests: Add general tests for OBJID_QUERYCLASSNAMEIDX.
      comctl32: Implement OBJID_QUERYCLASSNAMEIDX for Animate controls.
      comctl32: Implement OBJID_QUERYCLASSNAMEIDX for hotkey controls.
      comctl32: Implement OBJID_QUERYCLASSNAMEIDX for listviews.
      comctl32: Implement OBJID_QUERYCLASSNAMEIDX for tooltips.
      comctl32: Implement OBJID_QUERYCLASSNAMEIDX for trackbar controls.
      comctl32: Implement OBJID_QUERYCLASSNAMEIDX for treeviews.
      comctl32: Implement OBJID_QUERYCLASSNAMEIDX for updown controls.
      comctl32/tests: Remove individual OBJID_QUERYCLASSNAMEIDX tests.

Gabriel Ivăncescu (40):
      mshtml: Use HasAttribute instead of GetAttributeNode when checking if specified attribute.
      mshtml: Clone name properly from attached attribute nodes.
      mshtml: Use a BSTR to store the detached attribute's name.
      mshtml: Use a helper function to find an attribute in the collection's list.
      mshtml: Detach attribute nodes when removing the attribute from the element.
      mshtml: Implement 'specified' for detached attributes.
      mshtml/tests: Add tests for more element prototype props.
      mshtml/tests: Test frame and iframe element props.
      mshtml: Only allow a specific set of builtin props as attributes for elements.
      mshtml: Only allow a specific set of builtin props as attributes for button elements.
      mshtml: Only allow a specific set of builtin props as attributes for form elements.
      mshtml: Only allow a specific set of builtin props as attributes for frame elements.
      mshtml: Only allow a specific set of builtin props as attributes for iframe elements.
      mshtml: Only allow a specific set of builtin props as attributes for img elements.
      mshtml: Only allow a specific set of builtin props as attributes for input elements.
      mshtml: Only allow a specific set of builtin props as attributes for label elements.
      mshtml: Only allow a specific set of builtin props as attributes for link elements.
      mshtml: Only allow a specific set of builtin props as attributes for meta elements.
      mshtml: Only allow a specific set of builtin props as attributes for object elements.
      mshtml: Only allow a specific set of builtin props as attributes for option elements.
      mshtml: Only allow a specific set of builtin props as attributes for script elements.
      mshtml: Only allow a specific set of builtin props as attributes for select elements.
      mshtml: Only allow a specific set of builtin props as attributes for style elements.
      mshtml: Only allow a specific set of builtin props as attributes for table elements.
      mshtml: Only allow a specific set of builtin props as attributes for table data cell elements.
      mshtml: Only allow a specific set of builtin props as attributes for table row elements.
      mshtml: Only allow a specific set of builtin props as attributes for text area elements.
      mshtml: Handle NULL inputs in node's replaceChild.
      mshtml: Handle NULL input in node's removeChild.
      mshtml: Handle NULL input in node's appendChild.
      mshtml: Handle NULL input in node's insertBefore.
      mshtml/tests: Test IHTMLElement6::getAttributeNode in legacy modes.
      mshtml/tests: Test mixing attribute nodes and collections across modes.
      mshtml/tests: Test node hierarchy for attribute nodes in IE9+ modes.
      mshtml/tests: Don't create global variable due to typo.
      mshtml: Fix expando for IE9 attr nodes.
      mshtml: Fix gecko element leak when retrieving ownerElement.
      mshtml: Traverse the node on attribute nodes.
      mshtml: Allow custom set attributes with same name as builtin methods in legacy modes.
      mshtml: Fix 'expando' and 'specified' for attributes in legacy modes.

Giovanni Mascellani (10):
      mmdevapi/tests: Check that incompatible formats are rejected by IsFormatSupported().
      mmdevapi/tests: Remove workaround for Wine < 1.3.28.
      mmdevapi/tests: Test rendering with floating point formats.
      mmdevapi/tests: Test supported formats for capturing.
      mmdevapi: Error out if the channel count or sampling rate doesn't match the mix format.
      Revert "mmdevapi: Error out if the channel count or sampling rate doesn't match the mix format.".
      mmdevapi/tests: Mark a wrong error code by IsFormatSupported() as todo.
      mmdevapi/tests: Do not test QueryInterface() with a NULL output pointer.
      winepulse.drv: Allow 32-bit PCM audio samples.
      winecoreaudio.drv: Do not spam fixmes for unknown channels.

Hans Leidekker (20):
      crypt32: Retry with CRYPT_MACHINE_KEYSET in CRYPT_AcquirePrivateKeyFromProvInfo().
      windows.gaming.input: Turn put_Parameters() into a regular method.
      widl: Truncate identifiers that exceed the 255 character limit.
      widl: Check that retval parameters also have an out attribute.
      widl: Check eventadd method parameters.
      widl: Check eventremove method parameters.
      widl: Check propget method parameters.
      widl: Check propput method parameters.
      include: Comment reference to undefined activation interface.
      widl: Check activation method parameters.
      widl: Check composition method parameters.
      include: Add missing runtimeclass contract attributes.
      widl: Require runtimeclass contract or version attribute.
      widl: Use a structure for the version attribute.
      widl: Fix version attribute value.
      widl: Skip array size parameters.
      widl: Fix encoding of array parameters.
      widl: Only use 32-bit integers in row structures.
      widl: Add an implicit apicontract attribute.
      widl: Always store member references in attributes.

Haoyang Chen (2):
      explorer: Ignore command line character case.
      explorer: Allow /n to be followed by other arguments.

Henri Verbeet (1):
      wined3d: Add GPU information for AMD NAVI44.

Huw D. M. Davies (1):
      winemac: Define missing status variable.

Jacek Caban (78):
      krnl386: Remove __GNUC__ check.
      ntdll: Remove __GNUC__ checks.
      ntdll/tests: Remove __GNUC__ checks.
      ntoskrnl: Remove __GNUC__ check.
      oleaut32: Remove __GNUC__ check.
      advapi32: Initialize temp variable in test_incorrect_api_usage.
      crypt32/tests: Use dummySubject in CryptSIPLoad invalid parameter test.
      kernel32/tests: Initialize stackvar in test_IsBadReadPtr test.
      ole32/tests: Initialize rect in OleDraw invalid parameter test.
      shlwapi/tests: Initialize cookie before passing it to add_call.
      webservices/tests: Use valid url for WsEncodeUrl invalid argument tests.
      d3dx9/tests: Fix identity_matrix initialization.
      d3dx10/tests: Initialize data in test_D3DX10CreateAsyncMemoryLoader.
      d3dx11/tests: Initialize data in test_D3DX11CreateAsyncMemoryLoader.
      uxtheme/tests: Initialize rect in test_DrawThemeEdge.
      ntoskrnl/tests: Use initialized client dispatch in WskSocket call.
      quartz/tests: Initialize mt in test_connect_direct.
      d3d10core/tests: Initialize box in test_copy_subresource_region.
      d3d11/tests: Initialize box in test_copy_subresource_region.
      msvcp60/tests: Don't use const pointers for thiscall thunks.
      msvcp90/tests: Don't use const pointers for thiscall thunks.
      msvcr90/tests: Initialize key value in test_bsearch_s.
      include: Add _callnewh declaration.
      user32/tests: Use switch statement in test_keyboard_layout.
      opengl32: Move manual wow64 thunks declarations to generated header.
      opengl32: Move thunks declarations to generated header.
      opengl32: Use generated header for all thunk declarations.
      opengl32: Use generated header wrapper declarations.
      include: Add IHTMLAttributeCollection4 declaration.
      mshtml: Add IHTMLAttributeCollection4 stub implementation.
      opengl32: Move static keyword logic to generate_unix_thunk.
      opengl32: Use generate_unix_thunk for wow64 wgl thunks.
      opengl32: Use generate_unix_thunk for wow64 gl thunks.
      opengl32: Use generate_unix_thunk for wow64 ext thunks.
      opengl32: Remove no longer used get_func_args arguments.
      mshtml: Add create_node fallback to cloneNode.
      mshtml: Add DOM attribute node implementation.
      mshtml: Implement HTMLAttributeCollection4::get_length.
      mshtml: Implement IHTMLAttributeCollection4::item.
      mshtml: Implement IHTMLAttributeCollection4::getNamedItem.
      mshtml: Properly expose Attr and NamedNodeMap properties.
      mshtml/tests: Add more attribute nodes tests.
      opengl32: Use generated wow64 thunk for wglMakeCurrent.
      opengl32: Use generated wow64 thunk for wglMakeContextCurrentARB.
      opengl32: Use generated wow64 thunk for wglDeleteContext.
      opengl32: Use manual_win_functions for wglGetCurrentReadDCARB.
      opengl32: Avoid unneeded wrapper return type casts.
      opengl32: Use generated wow64 thunk for wglCreateContext.
      opengl32: Use generated wow64 thunk for wglCreateContextAttribsARB.
      opengl32: Don't generate wrapper declarations for functions implemented on PE side.
      opengl32: Introduce wow64 wrappers and use it for glClientWaitSync implementation.
      opengl32: Use wow64 wrapper for glFenceSync implementation.
      opengl32: Use wow64 wrapper for glDeleteSync implementation.
      opengl32: Use wow64 wrapper for glGetSynciv implementation.
      opengl32: Use wow64 wrapper for glIsSync implementation.
      opengl32: Use wow64 wrapper for glWaitSync implementation.
      opengl32: Factor out return_wow64_string.
      opengl32: Use generated thunk for glGetString.
      opengl32: Use generated thunk for glGetStringi.
      opengl32: Use generated thunk for wglGetExtensionsStringARB.
      opengl32: Use generated thunk for wglGetExtensionsStringEXT.
      opengl32: Use generated thunk for wglQueryCurrentRendererStringWINE.
      opengl32: Use generated thunk for wglQueryRendererStringWINE.
      opengl32: Improve whitespaces in wrapper declarations.
      opengl32: Use extra unix call argument to pass client buffer from glUnmapBuffer.
      opengl32: Use extra unix call argument to pass client buffer from glUnmapNamedBuffer.
      opengl32: Use wow64 wrappers for glMapBuffer and glMapBufferARB implementations.
      opengl32: Use wow64 wrappers for glMapBufferRange.
      opengl32: Use wow64 wrappers for glMapNamedBuffer and glMapNamedBufferEXT.
      opengl32: Use wow64 wrappers for glMapNamedBufferRange and glMapNamedBufferRangeEXT.
      opengl32: Use wow64 wrappers for glGetBufferPointerv and glGetBufferPointervARB.
      opengl32: Use wow64 wrappers for glGetNamedBufferPointerv and glGetNamedBufferPointervEXT.
      opengl32: Pass array arguments as pointers in unix calls.
      opengl32: Use generated thunk for glPathGlyphIndexRangeNV.
      opengl32: Pass type as a string to get_wow64_arg_type.
      opengl32: Use generated thunk for wglCreatePbufferARB.
      opengl32: Use generated thunk for wglGetPbufferDCARB.
      opengl32: Use generated thunk for wglGetProcAddress.

Jacob Czekalla (5):
      hhctrl.ocx: Add a search button to the search tab.
      hhctrl.ocx: Selection of treeview items in the content tab should reflect web browser page.
      mshtml/tests: Add call stacking to htmldoc test framework.
      mshtml/tests: Add iframe event tests in htmldoc.c.
      mshtml: Fire BeforeNavigate2 for documents in async_open.

Jake Coppinger (1):
      ntdll: Add a stub for RtlQueryProcessHeapInformation().

Jan Sikorski (1):
      maintainers: Remove myself as d3d maintainer.

Joe Souza (1):
      conhost: Implement F1 and F3 support for history retrieval.

Kareem Aladli (4):
      kernelbase: Implement VirtualProtectFromApp.
      kernelbase/tests: Add tests for VirtualProtectFromApp.
      ntdll: Set old_prot to PAGE_NOACCESS in NtProtectVirtualMemory() if the range is not mapped or committed.
      ntdll/tests: Add tests for NtProtectVirtualMemory().

Ken Sharp (3):
      configure: Do not hardcode "gcc" in message.
      po: Standardise source strings to English (United States).
      po: Update English (Default) resource.

Maotong Zhang (2):
      comdlg32/tests: Fix file type combo box selection in file dialogs.
      comdlg32: Display filter specs in itemdlg File Type combo box.

Marc-Aurel Zent (2):
      ntdll: Implement ThreadPriorityBoost class in NtQueryInformationThread.
      ntdll: Implement ThreadPriorityBoost class in NtSetInformationThread.

Michael Stefaniuc (8):
      dmcompos: Return E_NOTIMPL from the stub SignPost track Clone() method.
      dmcompos/tests: Fix the expected value in an ok() message.
      dmcompos: Simplify the DMChordMap IPersistStream_Load() method.
      include: Tag the DMUS_IO_* structs that changed between DX versions.
      dmstyle: Handle DX7 versions of 'note' and 'crve' chunks.
      dmusic: Don't open code debugstr_chunk in dmobject.c.
      dmusic: Add a helper to deal with different versions / sizes of a chunk.
      dmstyle: Support loading the DX7 version of the Style form.

Michael Stopa (2):
      kernel32/tests: SetFileInfo should accept FileRenameInfoEx.
      kernelbase: Pass FileRenameInfoEx to NtSetInformationFile.

Mike Kozelkov (2):
      urlmon: Add PersistentZoneIdentifier semi-stubs.
      urlmon/tests: Add PersistentZoneIdentifier test cases.

Mohamad Al-Jaf (5):
      include: Add windows.media.core.idl.
      include: Add windows.media.mediaproperties.idl.
      include: Add windows.media.transcoding.idl.
      windows.media: Implement IActivationFactory::ActivateInstance() for IMediaTranscoder.
      cryptowinrt: Implement ICryptographicBufferStatics::EncodeToBase64String().

Nikolay Sivov (33):
      d3d9/tests: Fix use-after-free (ASan).
      kernel32/tests: Fix double free of mutex handle.
      d3dx10/tests: Add a test for effect compiler behavior.
      windowscodecs/converter: Add 16bppGrayHalf -> 128bppRGBFloat conversion path.
      windowscodecs/converter: Add 16bppGrayHalf -> 32bppBGRA conversion path.
      windowscodecs/converter: Propagate source failure in 24bppBGR -> 128bppRGBAFloat conversion.
      windowscodecs/converter: Propagate source failure in 32bppBGRA -> 128bppRGBAFloat conversion.
      windowscodecs/converter: Propagate source failure in 48bppRGB -> 128bppRGBFloat conversion.
      windowscodecs/converter: Propagate source failure in 96bppRGBFloat -> 128bppRGBFloat conversion.
      windowscodecs/converter: Propagate source failure in 48bppRGBHalf -> 128bppRGBFloat conversion.
      d3d11: Make sure that index buffer is set for indexed draws.
      d3d11: Make sure that index buffer is set for instanced indexed draws.
      wined3d: Fix reference_graphics_pipeline_resources() argument type to match callers.
      d3d12/tests: Add a test for creating a device from dxcore adapters.
      d2d1: Implement mesh population methods.
      d2d1: Add a stub for geometry realization object.
      d2d1: Add initial implementation of CopyFromRenderTarget().
      d2d1: Improve bitmap methods traces.
      d2d1: Implement ComputeArea() for rectangles.
      d2d1: Implement ComputeArea() for transformed geometries.
      dxcore/tests: Add positive interface checks.
      dxcore/tests: Move GetProperty() tests to a separate function.
      dxcore/tests: Add some tests for InstanceLuid property.
      dxcore: Add support for InstanceLuid property.
      dxcore: Fix property size check in GetProperty().
      dxcore: Implement GetPropertySize().
      dxcore/tests: Add some tests for IsHardware property.
      dxcore: Return stub value for IsHardware property.
      dxcore/tests: Add some GetAdapterByLuid() tests.
      dxcore: Implement GetAdapterByLuid().
      d3d12: Support creating devices using dxcore adapters.
      dxcore: Add support for DriverDescription property.
      dxcore: Add support for DriverVersion property.

Paul Gofman (17):
      kernelbase: Duplicate GetOverlappedResult() implementation instead of calling GetOverlappedResultEx().
      kernelbase: Always set last error in GetOverlappedResult[Ex]().
      kernelbase: Wait in GetOverlappedResultEx() even if IOSB status is not pending.
      d2d1: Implement D2D1ComputeMaximumScaleFactor().
      opengl32: Map glCompressedTexImage2DARB to glCompressedTexImage2D if ARB_texture_compression is missing.
      shell32: Add AccountPictures known folder.
      nsiproxy.sys: Implement IP interface table.
      iphlpapi: Implement GetIpInterfaceTable().
      nsi/tests: Add tests for IP interface table.
      iphlpapi: Implement GetIpInterfaceEntry().
      bcrypt: Factor out len_from_bitlen() function.
      bcrypt: Use bit length instead of key size in key_import_pair().
      bcrypt: Handle importing ECDSA_P384 private blob.
      bcrypt: Support ECDSA_P521 algorithm.
      bcrypt/tests: Test ECDH_384 same way as ECDH_256.
      bcrypt: Support ECDH_P521 algorithm.
      bcrypt: Check output size early in key_asymmetric_encrypt() for RSA.

Piotr Caban (9):
      msado15/tests: Test functions called in ADORecordsetConstruction_put_Rowset.
      msado15/tests: Check recordset state in ADORecordsetConstruction tests.
      msado15/tests: Don't check count before running ADORecordsetConstruction field tests.
      msado15/tests: Add initial _Recordset_MoveNext tests.
      msado15/tests: Add _Recordset_get_RecordCount test.
      msado15/tests: Test IRowsetExactScroll interface in put_Rowset tests.
      vccorlib140: Add stub dll.
      vccorlib140: Add Platform::Details::InitializeData semi-stub.
      combase: Fix initialization flags in RoInitialize.

Ratchanan Srirattanamet (1):
      msi: Fix .NET assembly-related functionalities due to missed string copy.

Roman Pišl (2):
      ole32/tests: Test that cursor is preserved in DoDragDrop.
      ole32: Preserve cursor in DoDragDrop.

Rémi Bernon (56):
      win32u: Only update the window GL drawable when making one current.
      win32u: Move window drawable query out of DC drawable helpers.
      win32u: Update DC OpenGL drawable when it is acquired.
      winebus.sys: Prefer hidraw for all Virpil (VID 3344) devices.
      win32u: Also flush the GL drawable if the client surface is offscreen.
      win32u: Clear DC opengl drawable when releasing cached dce.
      joy.cpl: Initialize size before calling RegGetValueW.
      winebus: Use a single global structure for bus options.
      winebus: Support per-device/vendor hidraw registry option.
      windows.gaming.input: Forward get_NonRoamableId to Wine provider.
      windows.gaming.input: Forward get_DisplayName to Wine provider.
      joy.cpl: Use XInputGetStateEx to get guide button.
      joy.cpl: Add a new windows.gaming.input test tab.
      joy.cpl: List windows.gaming.input device interfaces.
      joy.cpl: Read windows.gaming.input device interface state.
      joy.cpl: Draw windows.gaming.input gamepad device.
      joy.cpl: Draw windows.gaming.input raw game controller.
      winemac: Create new client views with each VK/GL surface.
      winemac: Use the new client surface views for GL rendering.
      winemac: Get rid of now unnecessary child cocoa views.
      winemac: Sync current context when drawable was updated.
      win32u: Call opengl_drawable_flush even if drawables didn't change.
      winebus: Better separate hidraw from evdev in udev_add_device.
      winebus: Read evdev device info and feature bits on creation.
      winebus: Fill device mapping before report descriptor creation.
      winebus: Force the ordering of some common evdev gamepad buttons.
      winedmo: Avoid seeking past the end of stream.
      winedmo: Avoid reading past the end of stream.
      winedmo: Use the stream context to cache stream chunks.
      winedmo: Return container duration if no stream duration is found.
      winedmo: Return an integer from wave_format_tag_from_codec_id.
      winedmo: Seek to keyframes, using avformat_seek_file.
      widl: Get the version attribute from the typelib.
      winebus: Return error status if SDL is disabled.
      winebus: Introduce a new set_abs_axis_value helper.
      winebus: Emulate some gamepad buttons in the evdev backend.
      winebus: Introduce a new hid_device_add_gamepad helper.
      winebus: Use hid_device_add_gamepad in the evdev backend.
      win32u: Move nulldrv pixel format array inline.
      win32u: Allocate a global pixel formats array on the unix side.
      win32u: Keep pbuffer internal context on the wgl_context struct.
      win32u: Use eglGetConfigs rather than eglChooseConfig.
      win32u: Use surfaceless EGL platform for nulldrv.
      widl: Fix parsing of contract version.
      widl: Fix ATTR_CONTRACTVERSION output in header files.
      widl: Fix ATTR_CONTRACTVERSION in metadata files.
      widl: Write deprecated version in metadata files.
      windows.gaming.input: Only create Gamepad instances for XInput devices.
      winebus: Improve gamepad report compatibility with XUSB / GIP.
      windows.gaming.input: Use a generic dinput device data format.
      winebus: Use a vendor specific usage for gamepad guide buttons.
      winebus: Don't try to create rumble effect on device startup.
      winebus: Create dedicated threads to write evdev haptics output reports.
      win32u: Release the previous context drawables when changing contexts.
      win32u: Flush the new drawables after successful make_current.
      win32u: Notify the opengl drawables when they are (un)made current.

Shaun Ren (4):
      sapi/stream: Remove the FIXME message for unknown ISpStream interfaces.
      sapi/tests: Test resampler support in ISpVoice.
      sapi/tts: Implement TTS engine audio output resampler.
      sapi/tts: Support allow_format_changes in ISpVoice::SetOutput.

Thibault Payet (1):
      server: Always use the thread Unix id in ptrace for FreeBSD.

Tim Clem (4):
      wow64cpu: In Unix calls, always return the status from the non-Wow dispatcher.
      winebus: Quiet a log message about ignored HID devices.
      win32u: Fix an uninitialized variable warning.
      win32u: Remove a log message in get_shared_window.

Tomasz Pakuła (1):
      winebus: Do not touch autocenter on device init and device reset.

Vibhav Pant (55):
      bluetoothapis: Fix resource leak in bluetooth_auth_wizard_ask_response.
      include/ddk: Use the correct parameter types for ZwCreateEvent.
      setupapi/tests: Add tests for built-in device properties.
      ntoskrnl.exe/tests: Add tests for built-in properties for PnP device instances.
      setupapi: Support built-in properties in SetupDiGetDevicePropertyW and CM_Get_DevNode_Property_ExW.
      propsys/tests: Add conformance tests for getting PropertyDescriptions from PropertySystem.
      propsys/tests: Add conformance tests for PSGetPropertyKeyFromName.
      include: Add declaration for PSGetPropertySystem.
      propsys: Add stubs for PropertySystem.
      propsys: Add stubs for PSGetNameFromPropertyKey.
      propsys/tests: Add conformance tests for PSGetNameFromPropertyKey.
      propsys: Add IPropertyDescription stub for system defined properties.
      propsys: Implement IPropertyDescription for several known system properties.
      propsys/tests: Add some tests for PropVariantChangeType(VT_CLSID).
      propsys: Implement PropVariantChangeType(VT_CLSID) for string types.
      propsys/tests: Add test for PropVariantToGUID with VT_ARRAY | VT_UI1 values.
      winebth.sys: Only set properties for radio devices after they have been started.
      include: Add Windows.Foundation.Collections.PropertySet runtime class.
      wintypes/tests: Add conformance tests for Windows.Foundation.Collections.PropertySet.
      wintypes: Add stubs for Windows.Foundation.Collections.PropertySet.
      wintypes: Add stubs for IObservableMap<HSTRING, IInspectable *> to PropertySet.
      wintypes: Add stubs for IMap<HSTRING, IInspectable *> to PropertySet.
      wintypes: Add stubs for IIterable<IKeyValuePair<HSTRING, IInspectable *>> to PropertySet.
      windows.devices.enumeration: Implement DeviceInformationStatics::FindAllAsync using DevGetObjects.
      windows.devices.enumeration/tests: Add weak reference tests for DeviceWatcher.
      windows.devices.enumeration: Implement IWeakReferenceSource for DeviceWatcher.
      windows.devices.enumeration: Implement DeviceInformationStatics::DeviceWatcher using DevCreateObjectQuery.
      cfgmgr32: Add stubs for DevGetObjectProperties(Ex).
      cfgmgr32: Implement DevFreeObjectProperties.
      cfgmgr32: Implement DevGetObjectProperties for device interfaces.
      ntoskrnl.exe/tests: Add tests for device updates in DevCreateObjectQuery.
      cfgmgr32: Implement device updates for DevCreateObjectQuery.
      widl: Fix crash while replacing type parameters for arrays.
      cfgmgr32: Fix crash when CM_Register_Notification is called with a NULL filter.
      cfgmgr32: Add stub for DevFindProperty.
      cfgmgr32: Implement DevFindProperty.
      cfgmgr32/tests: Add some tests for calling DevGetObjects with filters.
      cfgmgr32: Validate DEVPROP_FILTER_EXPRESSION values passed to Dev{GetObjects, CreateObjectQueryEx}.
      cfgmgr32: Implement support for basic filter expressions in DevGetObjects.
      vccorlib140: Add stub for GetActivationFactoryByPCWSTR.
      vccorlib140: Implement GetActivationFactoryByPCWSTR.
      vccorlib140: Add stub for GetIidsFn.
      vccorlib140: Implement GetIidsFn.
      include: Add windows.devices.bluetooth.advertisement.idl.
      windows.devices.bluetooth/tests: Add tests for IBluetoothLEAdvertisementWatcher.
      windows.devices.bluetooth: Add stubs for BluetoothLEAdvertisementWatcher.
      windows.devices.bluetooth: Implement BluetoothLEAdvertisementWatcher::get_{Min, Max}SamplingInterval.
      windows.devices.bluetooth: Implement BluetoothLEAdvertisementWatcher::get_{Min, Max}OutOfRangeTimeout.
      winebth.sys: Remove GATT service entries when they are removed from the Unix Bluetooth service.
      winebth.sys: Enumerate and store GATT characteristics for each LE device.
      winebth.sys: Implement IOCTL_WINEBTH_LE_DEVICE_GET_GATT_CHARACTERISTICS.
      winebth.sys: Remove GATT characteristic entries when they are removed from the Unix Bluetooth service.
      bluetoothapis: Implement BluetoothGATTGetCharacteristics.
      bluetoothapis/tests: Implement tests for BluetoothGATTGetCharacteristics.
      winebth.sys: Set additional properties for remote Bluetooth devices.

Yeshun Ye (2):
      dsound: Check if 'cbPropData' for DSPROPERTY_Description1 is large enough.
      dsound/tests: Add test for DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_1.

Yongjie Yao (1):
      wbemprox: Add Status property in Win32_DesktopMonitor.

Yuxuan Shui (10):
      ntdll/tests: Check the context of a user callback.
      ntdll: Also restore rbp before calling user mode callback.
      cmd: Fix out-of-bound access when handling tilde modifiers.
      d2d1: Fix out-of-bound array access.
      cfgmgr32: Fix double-free of property buffers.
      server: Fix use-after-free in screen_buffer_destroy.
      urlmon/tests: Fix test_PersistentZoneIdentifier freeing the wrong thing.
      urlmon/tests: Fix out-of-bound write into tmp_dir.
      ntdll: Fix inconsistency in LFH block size calculation during realloc.
      rpcrt4: Don't read past the end of params in client_do_args.

Zhao Yi (1):
      wined3d: Return error code when Vulkan swapchain creation fails.

Zhiyi Zhang (23):
      user32/tests: Test WM_PRINT with an invisible parent.
      win32u: Allow PRF_CHILDREN to paint even though child windows have an invisible parent.
      include: Add windows.ui.windowmanagement.idl.
      include: Add Windows.UI.ViewManagement.ApplicationView runtime class.
      twinapi.appcore: Register some Windows.UI.ViewManagement.ApplicationView runtime classes.
      twinapi.appcore: Register Windows.UI.ViewManagement.UIViewSettings runtime classes.
      twinapi.appcore/tests: Add Windows.UI.ViewManagement.ApplicationView runtime class tests.
      twinapi.appcore: Add Windows.UI.ViewManagement.ApplicationView runtime class stub.
      include: Add AccessibilitySettings runtime class.
      windows.ui/tests: Add IAccessibilitySettings tests.
      windows.ui: Add IAccessibilitySettings stub.
      windows.ui: Implement accessibilitysettings_get_HighContrast().
      dwrite/tests: Add IDWriteFontDownloadQueue tests.
      dwrite: Implement dwritefactory3_GetFontDownloadQueue().
      d3d11: Return S_OK for d3d11_device_RegisterDeviceRemovedEvent().
      light.msstyles: Make toolbar button background transparent at the center.
      winemac.drv: Use a window level higher than kCGDockWindowLevel for WS_EX_TOPMOST windows.
      windows.globalization/tests: Add IApplicationLanguagesStatics tests.
      windows.globalization: Add IApplicationLanguagesStatics stub.
      windows.globalization/tests: Add ILanguage2 tests.
      windows.globalization: Implement ILanguage2.
      windows.applicationmodel/tests: Add Windows.ApplicationModel.DesignMode tests.
      windows.applicationmodel: Add Windows.ApplicationModel.DesignMode runtime class stub.

Ziqing Hui (10):
      fonts: Make numbers bold for WineTahomaBold.
      comctl32/tests: Check RGB value in test_alpha.
      comctl32/tests: Test image bitmap bits in test_alpha.
      comctl32/tests: Test adding 32bpp images with alpha to 24bpp image list.
      comctl32/tests: Use winetest_{push,pop}_context in test_alpha.
      comctl32/tests: Test image flags in test_alpha.
      comctl32/tests: Also test v5 with test_alpha().
      fonts: Make special ASCII characters bold for WineTahomaBold.
      fonts: Make uppercase ASCII letters bold for WineTahomaBold.
      fonts: Make lowercase ASCII letters bold for WineTahomaBold.
```
