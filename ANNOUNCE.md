The Wine development release 11.19 is now available.

What's new in this release:
  - Support for vertical text in GDIPlus.
  - Unicode character tables updated to Unicode 18.0.0.
  - Caching for DNS queries.
  - Improvements to the VBScript parser.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/11.x/wine-11.19.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-11.19/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 11.19 (total 23):

 - #25355  Time clock SBE 1.2 and 2.2 not working properly on ubuntu 10.04LTS
 - #45969  ArcGIS Pro crashes on start
 - #57100  Command Post fails to load
 - #58319  winevdm crashes running Quicktime 2
 - #59831  Zombie Army 4: Dead War running launcher leads to a memory leak
 - #59944  BOINC's project dialog is messed up
 - #60117  Evolution: The Game of Intelligent Life demo fails to start
 - #60276  Paint.net 5.2 (Build 9739) tool window does not get sized correctly
 - #60298  msvc-wine: freeze / deadlock during build due to a recent regression
 - #60305  Focus problem between X window and Wine window
 - #60322  wineboot blocks for ~5sec. on both macOS and Linux, slowing down the entire Wine startup process in certain situations.
 - #60343  Euro Truck Simulator 2 fails to start using OpenGL renderer with the EGL backend
 - #60345  rsaenh: CryptSetHashParam(HP_HMAC_INFO) with CALG_SHA_256 on PROV_RSA_FULL reports success but leaves the hash uninitialized; next CryptHashData() crashes (Aniimo, Steam 4126040)
 - #60371  Desperados 2: Cooper's Revenge crashes with Animated Shadows enabled on NVIDIA under X11/XWayland
 - #60374  msys2: forked process died unexpectedly, retry 0, exit code 0xE0000269, errno 11
 - #60378  _aligned_malloc returns an undersized non-NULL pointer for sizes near SIZE_MAX due to overflow
 - #60380  .NET Framework 4.8 : Decimal Math.Round always rounds up and never down
 - #60381  XInput: The controller "disable" feature is not implemented
 - #60385  Final Fantasy XI Online: Mouse/pointer cursor activates at unintended times (redux).
 - #60386  Compile errors in dlls/opencl/unix_thunks.c
 - #60392  Clicking "Replace All" crashes Wine Notepad
 - #60403  gdiplus: GdipWidenPath reads past the dash pattern of a DashStyleCustom pen without dashes
 - #60412  Hard Truck Apocalypse hangs and hits a stack overflow with builtin msvcr71/msvcp71

### Changes since 11.18:
```
Aleksei Arsenev (2):
      include: Add REPLACEFILE_IGNORE_ACL_ERRORS.
      kernelbase: Implement REPLACEFILE_IGNORE_*_ERRORS in ReplaceFileW().

Alex Henrie (38):
      riched20: Change dark green in pen_colors from dark cyan to dark green.
      riched20: Fix reversal of "objlock" and "objupdate" in rtfKey.
      riched20: Fix logging of wNumberingTab in ME_DumpParaStyleToBuf.
      riched20: Correct always-true condition in ME_AppendToHGLOBAL.
      comctl32: Check array length before dereferencing in TOOLBAR_WrapToolbar.
      shell32: Allocate enough space for the final null in build_paths_list.
      setupapi: Don't overallocate in (append|delete)_multi_sz_value.
      crypt32: Pass the correct length when querying CNGExtraAlgid.
      shell32: Remove duplicate check in IExplorerBrowser_fnSetFolderSettings.
      msvcrt: Fix typo in function name in comment above _snwscanf_s_l.
      include: Fix typo in comment at end of include guard in msvcrt/share.h.
      msvcrt: Pass buffer size in WCHARs from _wassert to _snwprintf.
      tapi32: Fix backwards string copy in tapiGetLocationInfoA.
      krnl386: Delete NE_DumpModule and NE_WalkModules.
      msi: Trace function arguments in DISTINCT_CreateView.
      dssenh: Trace the algorithm ID argument in CPDeriveKey.
      wineps: Return NULL, not FALSE, from create_print_ctx on error.
      combase: Fix leak of registry key in get_library_for_classid.
      oleaut32: Call get_ptr_size only once in TLB_size_instance.
      win32u: Call FT_Set_Charmap only once in select_charmap.
      mapi32: Allocate enough space for null terminator in load_mapi_provider.
      mshtml: Decrement the refcount in InstallCallback_Release.
      mshtml: Fix logging of IDM_UNORDERLIST in query_edit_status.
      urlmon: Close HKEYs with CloseRegistryKey in get_mime_filter.
      notepad: Always pass a DWORD to EM_GETSEL and EM_SETSEL.
      ntdll: Set IPV6_MTU_DISCOVER for IPv6, not IPv4, in sock_ioctl.
      msimtf: Call ImmGetCandidateListCountW from GetCandidateListCountW.
      advapi32: Correctly pass the domain from LogonUserA to LogonUserW.
      kernel32/tests: Test ReleaseSemaphore with an invalid release count.
      ntdll/tests: Test NtReleaseSemaphore with an invalid release count.
      ntdll: Make the release count argument to NtReleaseSemaphore signed.
      ntdll: Validate the release count argument in NtReleaseSemaphore.
      rpcrt4: Compile ndr64_(async_)client_call only on 64-bit architectures.
      hnetcfg: Fix stack corruption in fw_app_put_ProcessImageFileName.
      wmp: Also call AddRef on the first connection in EnumConnections_Next.
      quartz: Don't overallocate in FilterMapper_EnumMatchingFilters.
      ws2_32: Increase the size of the WSAAddressToString buffers.
      notepad: Use CRT allocation functions.

Alexandre Julliard (13):
      ntdll: Always reserve some top-down space.
      ntdll: Always allocate the virtual heap with mmap() on 64-bit.
      ntdll: Make sure that the first thread data is outside of the exe range.
      configure: Consistently check for both headers and libraries with pkgconfig.
      preloader: Remove i386 macOS support.
      preloader: Don't reserve the top-down addresses.
      preloader: Remove WINEPRELOADRESERVE support in the macOS preloader.
      faudio: Import upstream release 26.10.
      msvcrt: Always use explicit sized variant of time_t.
      msvcrt: Use 32-bit time_t with older msvcrt versions.
      kerberos: Don't build the dll at all if the needed libraries are missing.
      msvcrt: Fix stat() function prototypes for older msvcrt versions.
      msvcrt: Define _CORECRT_BUILD when building msvcrt modules.

Alistair Leslie-Hughes (19):
      windows.devices.enumeration: Return E_NOINTERFACE when no interface found.
      sapi: Return error on memory allocation failure.
      sapi: Handle memory allocation failure in ISpVoice::Speak.
      oleaut32: Use the correct sizeof value in VARIANT_FormatString.
      oleaut32: Assign the correct size in FmtMediumTime.
      oleaut32: Output the correct varType variable.
      msado15: Add validation checks for position in _Stream_put_Position.
      include: Add RecordStatusEnum typedef.
      include: Add missing d3d12 typedefs.
      dpnet: Remove unneeded parameters from Create Functions.
      dpnet: Implement DirectPlay8Create.
      server: Correct mode mask compare.
      msado15: Check correct pointer in parse_criteria.
      inetmib1: Correctly access the table entries.
      comdlg32: Consistently use SendDlgItemMessageW for messages.
      rsaenh: Don't finalize hash for buffer size query in RSAENH_CPGetHashParam().
      winetest: Correct CLSID format string.
      include: Add some d3d12 options structs.
      amstream: Remove duplicate assignment.

Andrii Yukhymchak (1):
      ntdll: Fixed compile time issue in socket.c for OpenBSD.

Anna (navi) Figueiredo Gomes (1):
      win32u: Load IDC_WAIT if no cursor was ever set.

Anton Baskanov (1):
      dmime: Set F_INSTRUMENT_DRUMS for the 10th channel in midi_parser_handle_program_change().

Bernhard Kölbl (6):
      windows.media.speech: Avoid leaking an HSTRING.
      windows.media.speech: Improve memory allocation handling.
      windows.media.speech: Improve memory allocation handling.
      windows.media.speech: Improve memory allocation handling.
      windows.media.speech: Improve handling of GetUserDefaultLocaleName errors.
      windows.media.speech: Downgrade GetRuntimeClassName to a FIXME.

Bernhard Übelacker (3):
      comctl32_v6: Avoid uninitialized values in szDim in SYSLINK_Render.
      dnsapi: Increase allocations by the terminating character.
      cmd: Avoid reading past string termination in WCMD_split_command_build.

Brendan Shanks (13):
      winemac: Use C string format strings in the ObjC-only ERR().
      winemac: Use C99 bool for internal functions/variables in macdrv.h.
      winemac: Move all Windows includes (by Unix files) to macdrv.h.
      winemac: Don't expose GDI driver function prototypes to ObjC code.
      winemac: In ObjC files, replace BOOL in Windows headers with WINBOOL.
      winemac: Avoid RIID redefinition errors with CFPlugIn.h.
      winemac: Include OpenGL in macdrv.h to avoid typedef conflicts.
      winemac: Include config.h and macdrv.h in all ObjC files.
      winemac: Add default debug channels to ObjC files.
      winemac: Remove winemac-specific logging functions.
      winemac: Improve debugstr_cf().
      winemac: Remove pre-macOS 10.15 workarounds.
      winemac: Use [NSAnimationContext begin/endGrouping] in place of the deprecated NSDisable/EnableScreenUpdates().

Charlotte Pabst (14):
      mfplat/tests: Add non-locking d3d12 buffer tests.
      mfplat/tests: Test that d3d12 buffer Lock() is readonly.
      mfplat/tests: Test d3d12 buffer read-write lock failures.
      mfplat/tests: Test d3d12 buffer lock behavior.
      mfplat/tests: Test d3d12 buffer sync object interactions.
      mfplat/tests: Add d3d12 buffer creation tests.
      mfplat: Allow creating d3d12-backed buffers.
      mfplat: Implement Lock()/Unlock() for d3d12 buffer.
      mfplat: Implement GetCurrentLength()/SetCurrentLength() for d3d12 buffer.
      mfplat: Implement Lock2D()/Unlock2D() for d3d12 buffer.
      mfplat: Implement GetScanline0AndPitch() for d3d12 buffer.
      mfplat: Implement Lock2DSize() for d3d12 buffer.
      mfplat: Implement GetResource() for d3d12 buffer.
      mfplat/tests: Fix use-after-free in d3d12 buffer tests.

Connor McAdams (12):
      winebus: Set proper device capabilities for winebus devices.
      winebus: Allow the PnP manager to generate a container ID for us.
      winebus: Rename input field in struct device_desc to interface.
      winebus: Get USB interface information on MacOS if possible.
      winebus: Move USB specific code out of get_device_subsystem_info() and into its own function.
      winebus: Get vid/pid/version values using their sysattr values for USB devices.
      winebus: Get interface number for USB based udev devices.
      winebus: Get interface number for USB based IOHID devices.
      winebus: Avoid setting serial number field unnecessarily.
      winebus: Don't report a serial number for USB devices that don't have one.
      hidclass.sys: Use DEVPKEY_Device_InstanceId to get PDO IDs.
      hidclass.sys: Don't query container ID from the bus PDO.

Conor McCarthy (28):
      mf/sar: Move audio renderer flush actions inside the conditional code block for active state.
      mf/tests: Test Flush() on a shutdown audio renderer stream sink.
      mf/sar: Check for null audio client before resetting during flush.
      mfplat: Return the error code from dxgi_buffer_GetUnknown().
      mfplat/tests: Test MFInitMediaTypeFromWaveFormatEx() with WAVE_FORMAT_EXTENSIBLE and zero cbSize.
      mfplat: Validate MFInitMediaTypeFromWaveFormatEx() cbSize for WAVE_FORMAT_EXTENSIBLE.
      mfplat/tests: Add tests for zero image width or height.
      mfplat: Return E_INVALIDARG for zero height in MFCalculateImageSize().
      mfplat: Check for null media type in sample_allocator_InitializeSampleAllocator().
      mfplat: Check for null media type in sample_allocator_InitializeSampleAllocatorEx().
      mfplat/tests: Test sample allocator initialisation with null media type.
      mfplat: Return the error code on failure of presentation_descriptor_init().
      mf/topology_loader: Pass the output pointer to topology_node_get_object() instead of its address.
      mf/topology_loader: Get the input at the current loop index in topology_node_set_device_manager().
      mf/topology: Return the exchanged new value in topology_generate_id().
      mf: Always reset maxlen in mf_get_handler_strings().
      mf/scheme_handler: Always release the byte stream in scheme_handler_callback_Invoke().
      mf/topology: Guard against overflow in topology_node_reserve_streams().
      mf/tests: Test SetTimer() with no time source.
      mf/clock: Do not leave a relative 'time' on error in present_clock_schedule_timer().
      mf/clock: Check time_source for null in present_clock_schedule_timer().
      mf/copier: Validate sample_copier_transform_ProcessOutput() count parameter.
      mf/session: Check topo_node for null in the METransformNeedInput handler.
      mf/sac: Check for memory allocation failure in enum_audio_capture_sources().
      mfmediaengine: Add a missing else in media_engine_GetCurrentSource().
      mfsrcsnk: Validate stream identifier is not zero.
      mfsrcsnk: Initialise all necessary sink fields before creating a wave format.
      mfplat: Set the handler's activate pointer in MFRegisterLocalByteStreamHandler().

Daniel Betz (1):
      cmd: Fix buffer overflows in search_command() for long parameters.

Daniel Lehman (4):
      ucrtbase/tests: Add strftime format tests with %O.
      msvcp140/tests: Add tests for time_put<char>.
      ucrtbase: Stub %O in strftime.
      msvcp140: Pass along %O in format to time_put.

Daniel Varga (1):
      msvcrt: Don't compute handler pointer in cxx_frame_handler if it's not needed.

Dmitry Timoshkov (2):
      winemenubuilder: Use character literals instead of hex codes.
      winemenubuilder: Simplify the code a bit.

Elizabeth Figura (10):
      mountmgr: Implement IOCTL_DVD_SEND_KEY.
      mountmgr: Implement IOCTL_DVD_END_SESSION.
      mountmgr: Implement IOCTL_DVD_GET_REGION.
      mountmgr: Implement IOCTL_DVD_READ_STRUCTURE.
      mountmgr: Implement IOCTL_CDROM_CHECK_VERIFY.
      mountmgr: Implement IOCTL_SCSI_PASS_THROUGH.
      mountmgr: Implement IOCTL_SCSI_PASS_THROUGH_DIRECT.
      mountmgr: Implement IOCTL_SCSI_GET_ADDRESS.
      mountmgr: Implement IOCTL_SCSI_GET_CAPABILITIES.
      mountmgr: Implement IOCTL_SCSI_GET_INQUIRY_DATA.

Eric Blum (2):
      user32/tests: Test toolwindow minimum width.
      win32u: Fix minimum width of WS_EX_TOOLWINDOW windows.

Eric Pouech (12):
      cmd/tests: Add some more tests.
      cmd: Introduce explicit delimiters constants.
      cmd: Introduce a delimiter set for executable.
      cmd: Remove unnecessary parameter to WCMD_parameter*().
      cmd: Add structure splitting a command line into command + args/options.
      cmd: Introduce helper to identify word inside a string.
      cmd: Introduce struct word_iterator to help builtin commands parsing.
      include: Add SCN scanf constants for byte-sized ptr.
      include/msvcrt: Default time_t to 64bit even on 32bit compilations.
      include/msvcrt: Add missing debug definitions.
      include: Add a poor's man implementation of malloca()/freea().
      include: Fix socklen_t definition.

Esme Povirk (12):
      ntdll: Stub NtOpenPrivateNamespace.
      gdiplus/tests: Add test for DriverStringOptionsVertical.
      gdiplus: Support vertical text in GdipDrawDriverString.
      gdiplus: Support vertical text in GdipMeasureDriverString.
      gdiplus: Implement vertical text in GdipDrawString.
      gdiplus/tests: Remove duplicated line.
      gdiplus: Implement vertical text in GdipMeasureString.
      gdiplus: Implement vertical text in GdipMeasureCharacterRanges.
      gdiplus/tests: Add tests for vertical string measurement.
      gdiplus: Implement vertical text in GdipAddPathString.
      windowscodecs: Check for integer overflow in DDS decoder.
      uiautomation/tests: Skip a test that crashes on Windows 11.

Etaash Mathamsetty (1):
      winewayland: Implement color management protocol for Vulkan color space.

Francis De Brabandere (13):
      vbscript: Report a syntax error for overflowing hex and octal literals.
      vbscript: Treat unused reserved words as keywords.
      vbscript: Allow keywords that are valid identifiers as ReDim variable names.
      vbscript: Fix indexing an array named default or property.
      vbscript: Implement Erase as a builtin procedure instead of a statement.
      vbscript: Don't assert on an assignment to Me.
      vbscript: Don't assert on an assignment to an expression in parentheses.
      vbscript: Report "Name redefined" for a redefined parameter name.
      vbscript: Report "Name redefined" for more local declarations.
      vbscript: Don't check class method names against locals of another method.
      vbscript: Report "Name redefined" for a class variable declared twice.
      vbscript: Report "Name redefined" for a name declared by ReDim.
      vbscript: Allow a class property after a method of the same name.

Giang Nguyen (2):
      ucrtbase/tests: Test the tmpnam() family.
      msvcrt: Place the tmpnam() names in the temporary directory for the UCRT.

Gijs Vermeulen (1):
      quartz: Check whether the pin is connected in IBasicVideo::get_VideoWidth() and get_VideoHeight().

Hans Leidekker (14):
      dnsapi: Fix record data length in DnsRecordCopyEx().
      dnsapi: Validate domain name in DnsQuery_UTF8().
      dnsapi: Introduce a helper to allocate a record.
      dnsapi: Add support for the hosts file in DnsQuery_UTF8().
      widl: Build an index for typeref and typedef tables to speed up merge lookups.
      ws2_32: Support AI_CANONNAME | AI_DNS_ONLY flags in getaddrinfo().
      wineboot: Avoid multicast DNS lookup when resolving canonical hostname.
      dnsapi: Add a cache.
      dnsapi: Implement DnsRecordListFree(DnsFreeFlat).
      dnsapi: Implement DnsGetCacheDataTable().
      dnsapi: Avoid VOID, PCSTR, PCSWTR, etc.
      dnsapi: Update option mapping.
      include: Define DNS_QUERY_DNSSEC_REQUIRED.
      kerberos: Initialize the unixlib in DllMain().

Herman Semenoff (1):
      d3dcompiler_43: Use memchr() and an end pointer in get_line().

Ivan Ivlev (1):
      winemenubuilder: Do not write empty arguments to desktop entries.

Jacek Caban (7):
      ieframe: Fix DISPID_AMBIENT_SILENT return value.
      ieframe: Handle safe array access errors in navigate_url.
      ieframe: Fix returned cookie in EnumConnections_Next.
      ieframe: Clean up VARIANT variables in on_offlineconnected_change and on_silent_change.
      ieframe: Fix error handling in get_travellog_stream.
      ieframe: Handle memory allocation failure in create_webbrowser.
      ieframe: Handle memory allocation failure in create_callback.

Janne Kekkonen (1):
      dbghelp: Initialize missing DWARF attribute names to empty strings.

Johnny Arcitec (1):
      xinput1_3: Do not enumerate "disabled" controllers.

Marc-Aurel Zent (4):
      mountmgr: Fix type in dvd_read_structure for DVD disk key descriptor.
      winemac: Move IME related functions into their own file.
      winemac: Send IME updates directly from the main thread.
      imm32: Do not classify aliased HKLs as IME.

Matteo Bruni (10):
      d3dx9/tests: Fix compare_elements() error string on declaration size mismatch.
      d3dcompiler: Fix texcoord bounds check in find_ps_builtin_semantics().
      d3dcompiler: Add error checking to GetCurrentDirectoryA() call.
      d3dx9: Handle 0 argument in make_pow2().
      d3dcompiler: Properly handle a coissue flag on the first assembly shader instruction.
      d3dcompiler: Properly handle a predicate on the first assembly shader instruction.
      d3dx9/shader: Check for NULL bytecode in get_shader_semantics().
      d3dx9/effect: Check memory allocation in BeginParameterBlock().
      d3dx9/effect: Check wide string allocation in D3DXCreateEffect[Compiler]FromFile[Ex]A().
      d3dx9/sprite: Check vertex data allocation in d3dx_sprite_flush().

Michael Stefaniuc (11):
      dmusic: Drop unused assigment.
      dmime: Add missing break for a switch case.
      dmime: Fix operator precedence issue.
      dmime: Don't use substraction to compare two signed ints.
      dmime: Don't leak mem when freeing a sequence track.
      dmime: Split the Wave track SetParam into helpers per type.
      dmime: Don't leak dsound references in the Wave track SetParam.
      dmsynth: Don't leak references when destroying a sink.
      dmsynth: Don't leak a ref to the sink when destroying the synth.
      dmime: Pass the destination buffer size to stream_chunk_get_*.
      dmime: A MIDI note is just 7 bits so mask off the high bit.

Michael Weghorn (1):
      include: Add UI Automation HeadingLevelIds.

Mikhail Zhadanov (1):
      ws2_32: Add support for SIO_UDP_NETRESET.

Nello De Gregoris (6):
      rsaenh: Return NTE_BAD_ALGID for an HMAC with no hash algorithm.
      rsaenh: Allow any implemented hash algorithm for HMAC.
      rsaenh: Check the HMAC hash algorithm in CPSetHashParam().
      rsaenh: Use the hash implementation for the HMAC key in CPSetHashParam().
      rsaenh: Pad the HMAC key with the block length of the hash algorithm.
      rsaenh/tests: Test the inner hash algorithm of an HMAC.

Nikolay Sivov (6):
      msvcp: Implement GetNextAsyncId().
      unicode: Update to Unicode 18.0.0.
      include: Add some types for the D2D1ColorManagement effect.
      d2d1: Add Color Management effect stub.
      mfplat/tests: Add some tests for DXGI_FORMAT_R8_UINT buffers.
      mfplat: Handle R8_UINT format in MFCreateDXGISurfaceBuffer().

Paul Gofman (5):
      server: Make messages mergeable by default.
      kernel32/tests: Add tests for setting zero group affinity mask.
      ntdll: Treat zero group affinity as full group affinity.
      d3dx10/sprite: Fix matrix multiplication order in d3dx10_sprite_draw().
      d3dx10/sprite: Avoid drawing extra primitives with mismatched texture in d3dx10_sprite_draw_batch().

Peter Wedder (2):
      dinput: Use link collections when reading HID joystick values.
      winebus: Put joystick hat switches in separate collections.

Piotr Caban (15):
      wmiutils: Implement IMarshal interface in WbemDefPath.
      oledb32: Support converting DBTYPE_VARIANT to DBTYPE_I8.
      oledb32: Support converting DBTYPE_VARIANT to DBTYPE_BOOL.
      ncrypt: Don't allow setting NCRYPT_PROVIDER_HANDLE_PROPERTY property in NCryptSetProperty.
      ncrypt: Make ncrypt provider handles refcounted.
      secur32: Fix LsaFreeReturnBuffer return value.
      msxml3: Get rid of no longer needed element_entry structure.
      msxml3: Return success when setting MaxXMLSize property.
      oleaut32: Don't overflow maximum date value in VariantTimeToSystemTime when handling milliseconds.
      mshtml: Add HTMLCSSStyleDeclaration:textOverflow property implementation.
      mshtml: Test VT_UI8 type in JavaScript.
      mshtml: Handle VT_I8 type in JavaScript.
      kernel32/tests: Test if LOCALE_SSORTLOCALE returned name is valid.
      nls: Fix ku-Arab sortlocale.
      nls: Generate pap-029 locale data.

Rémi Bernon (36):
      winemac: Make the cocoa id<X> typedefs consistent with vulkan headers.
      win32u: Parse extensions directly into the BOOLEAN set.
      win32u: Parse context version from GL_VERSION string.
      opengl32: Simplify make_opengl extensions enumeration.
      win32u: Parse and keep track of available EGL extensions.
      win32u: Load core and extension GL procedures using a loop.
      win32u: Load all EGL core and extension functions.
      win32u: Restore dropped return FALSE if pbuffer creation fails.
      win32u: Parse wglCreateContextAttribsARB attribs on behalf of the drivers.
      win32u: Use the opengl_context_attrs to keep opengl_context attributes.
      win32u: Move some context attribute checks out of the drivers.
      winemac: Get rid of the now unnecessary macdrv_context core field.
      winemac: Merge create_context into macdrv_context_create.
      win32u: Move internal_context_create helper around.
      win32u: Check ES2_PROFILE_BIT against the profile attributes.
      win32u: Initialize context draw / read buffers when making it current.
      win32u: Introduce get_root_context for separate context sharing.
      opengl32: Use the root contexts to decide if sharing is possible.
      opengl32: Use a specific unix call to switch root contexts.
      win32u: Recreate framebuffer surface on root context change.
      win32u: Create extra root and thread contexts with NO_ERROR flag.
      win32u: Create extra root contexts for compat contexts on macOS.
      win32u: Get rid of the now unnecessary context shared flag.
      win32u: Also notify pbuffer surfaces of internal context activation.
      win32u: Use the null surface if framebuffer surface has no target.
      win32u: Add a default pbuffer implementation using the FBO surface.
      win32u: Avoid using glDrawPixels with the FBO surface pbuffer.
      winemac: Implement pbuffers using the framebuffer surface.
      win32u: Pass both strings as a single WINE_IME_POST_UPDATE param.
      win32u: Pass the IME update return pointer to ImeToAsciiEx.
      opengl32: Add more wglChoosePixelFormat filtering traces.
      win32u: Use a structure for extra pixel format flags.
      winex11: Request sRGB colorspace for sRGB pixel formats.
      winewayland: Request sRGB colorspace for sRGB pixel formats.
      win32u: Generate extra sRGB pixel formats for onscreen EGL configs.
      winewayland: Register a color-management-v1 protocol listener.

Thibault Payet (1):
      mountmgr: Fixup build issues related to cdrom.

Thomas Ritter (1):
      gdiplus: Widen DashStyleCustom pens without dashes as solid lines.

Tobiasz Laskowski (2):
      ole32/tests: Test DoDragDrop before OleInitialize.
      ole32: Fix DoDragDrop uninitialized error code.

Vibhav Pant (7):
      wintypes/tests: Add additional tests for RoResolveNamespace.
      wintypes: Add an initial implementation for RoResolveNamespace.
      widl: Set the memberref field for all ATTR_STATIC attributes on a type.
      wintypes/tests: Fix buffer overflow while creating a HSTRING with an embedded NUL.
      wintypes: Fix incorrect heap allocation in RoResolveNamespace.
      widl: Add initial support for merging multiple WinMD files.
      widl: Accept multiple input IDLs when invoked with --winmd and --output.

Vishnunithyasoundhar S (3):
      msvcrt: Check for overflow in aligned memory allocation.
      oleaut32: Fix VarDecRound() rounding of discarded digits.
      attrib: Reject file name arguments longer than MAX_PATH.

Zhiyi Zhang (4):
      include: Add a PW_RENDERFULLCONTENT flag for PrintWindow().
      user32/tests: Add tests for PrintWindow().
      win32u: Reimplement NtUserPrintWindow() without sending WM_PRINT messages.
      ntdll: Add a ZwOpenPrivateNamespace() private export.
```
