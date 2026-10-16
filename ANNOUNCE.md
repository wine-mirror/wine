The Wine development release 11.20 is now available.

What's new in this release:
  - Relay debugging for ARM64EC.
  - Improvements in XML validation.
  - CDROM support moved to the Mount Manager.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/11.x/wine-11.20.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-11.20/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 11.20 (total 24):

 - #7054   Pure Pinball game crashes trying to access freed texture objects
 - #20777  Multiple applications and games fail to load SHDocVw.dll interop dll (Wine's 'shdocvw.dll' is preferred over native)(Unigine Heaven benchmark v2.0, Toad for SQL Server 5.7, QuickBooks Help)
 - #30639  Audio stuttering and performance drops in Star Wolves 3
 - #34028  Intuit QuickBooks 2008-2010 fails to run, reporting 'Could not initialize license properties'
 - #34040  QuickBooks SS 2008 R1 crashes multiple times on second run
 - #42877  SumatraPDF 3.1.2 tabs are covered by wine title bar
 - #46568  64-bit msxml6.dll from Microsoft Core XML Services 6.0 redist package fails to load (Wine doesn't respect 44-bit user-mode VA limitation from Windows < 8.1)
 - #58418  InternetQueryOptionW(proxy_username) crashes with NULL buffer but valid size pointer.
 - #59134  UFO: Aftershock, UFO: Afterlight - problem starting the games with Nvidia binary drivers (EGL backend)
 - #59356  Imperium Great Battles of Rome doesn't reproduce music and audio
 - #59770  Broken Input under Neko Project 21/W
 - #59877  Tagscanner: Server-Side Decorations being drawn on top of the Client-Side Decorations.
 - #60253  Incoming (1998) crashes after the intro animation
 - #60328  Opening a menu in FL Studio instantly selects the menu entry the mouse is hovering
 - #60383  cmd: stack buffer overflow in search_command with a quoted /c command line longer than MAX_PATH
 - #60384  Unable to log in to Medicalc4
 - #60399  C++ exception reproducer (ARM64): crashes when a DLL function ends with a call to a noreturn throw helper
 - #60405  ssbs-probe (2026-09-29 source snapshot, ARM64): SSBS cleared after RtlRestoreContext
 - #60408  mfplat: MFCreateDXGISurfaceBuffer fails silently for DXGI_FORMAT_R8_UINT textures, so UE4 ElectraPlayer shows no video
 - #60410  MusicBee: System.OutOfMemoryException in WoW64 mode
 - #60421  Incomplete implementation of cldapi.dll causes OneDrive to crash.
 - #60424  Office 2013 Click-to-Run installer fails with builtin msxml6 when DOMDocument::save() uses "unicode" encoding
 - #60466  Battlefield 1942 cannot copy secdrv.sys into c:\windows\syswow64\drivers because directory does not exist.
 - #60484  wine 11.19 dnsapi parsing a large /etc/hosts stalls fresh prefix initialization

### Changes since 11.19:
```
Adam Markowski (1):
      po: Update Polish translation.

Alex Henrie (6):
      wined3d: Fix comment at end of include guard in wined3d_gl.h.
      user32: Correct the number of bytes that CharToOemBuffW writes.
      winsqlite3: Correct spelling of "Downloading" in make_sqlite.py.
      cryptnet: Don't overallocate in build_request_url.
      comsvcs: Fix memory leak on error path in holder_AllocResource.
      ntdll: Remove stray newline from default case of io2str.

Alexandre Julliard (36):
      makefiles: Support disabling modules for specific archs through the makefile.
      kerberos: Link directly to the krb5 libraries.
      configure: Remove a couple of no longer used config.h defines.
      ntdll: Remove special handling of areas above the user limit.
      ntdll: Store the lowest reserved address as start of the DOS range.
      preloader: Round WINEPRELOADRESERVE values to the granularity boundary.
      ntdll: Align reserved areas to the allocation granularity.
      win32u: Display the full client area if it extends into the window decorations.
      ntdll: Also determine the host address space limit on 32-bit.
      ntdll: Only reserve high 32-bit memory after the main exe is loaded.
      ntdll: Allocate the virtual heap with mmap() also on 32-bit.
      ntdll: Reserve a larger range for the top-down area.
      include: Remove some types that don't exist on Windows.
      include: Remove strcasecmp/strncasecmp declarations.
      ntdll: Use calloc to allocate the page protection pointers.
      ntdll: Use an array instead of a list for reserved memory areas.
      ntdll: Add support for blocking some memory ranges from allocation.
      ntdll: Reserve the entire low 8G on 64-bit.
      ntdll: Merge the reserved areas and free ranges arrays.
      wpnapps: Import library is not needed.
      ntoskrnl: Reject 0 affinity in KeSetSystemAffinityThreadEx().
      user32/tests: Fix a couple of test failures.
      msvfw32/tests: Mark a failing test as todo.
      ntdll: Reserve a top-down area even with a limited 64-bit address space.
      include: Remove workarounds for old MSVC compiler versions.
      include: Don't use a full barrier in Acquire/Release functions on ARM64.
      include: Implement more Acquire/Release functions.
      dplay: Make it a data-only dll.
      dpnaddr: Make it a data-only dll.
      dpnlobby: Make it a data-only dll.
      iprop: Make it a data-only dll.
      rsabase: Make it a data-only dll.
      shfolder: Make it a data-only dll.
      twain_32: Make it a data-only dll.
      vga: Make it a data-only dll.
      winegcc: Support setting the subsystem of data-only dlls.

Alistair Leslie-Hughes (14):
      msado15: Trace more recordset DBPROP id's.
      fusion: Remove duplicate assignment in IAssemblyName::GetName.
      prntvpt: Correctly compare length/width bits in devmode_to_ticket.
      prntvpt: Initialize ticket before parsing in PTGetPrintCapabilities.
      crypt32: Don't return from within a __TRY block.
      wintypes: Correctly release interface in map_entry_clear.
      dbgeng: Correct output parameter for GetCurrentTimeDate/GetCurrentSystemUpTime.
      include: Correct various commctrl.h macros.
      include: Correct spelling of D3DXBONECOMBINATION.VertexCount.
      include: Correct szOID_RSA_unstructAddr value.
      include: Correct CERT_RDN_* values.
      oledb32: Pass the guid to create_datasource.
      hnetcfg: Only assign output on success in NetFwPolicy2_create.
      wintab32: Increase the default queue size.

Andrey Gusev (1):
      d3dx: Trace the correct argument in d3dx9_line_GetDevice().

Anna (navi) Figueiredo Gomes (1):
      d3dx10: Use the correct compiler version in D3DX10CompileFromMemory and D3DX10CreateEffect*.

Anton Baskanov (27):
      dmime/tests: Explicitly specify the segment start time in test_midi.
      dmime/tests: Test setting GUID_StandardMIDIFile on a MIDI segment.
      dmusic: Move bandtrack.c to dmusic.
      dmusic: Factor out adding a band to a band track.
      dmime: Add default bands when parsing a MIDI file.
      dmusic: Correctly handle adding a band at the beginning of a track.
      dmime/tests: Test segment looping.
      dmime/tests: Stop playback before the next test in test_wave_pmsg.
      dmime: Store the length and the loop points in the segment state.
      dmime: Set rtLength for wave segments.
      dmime: Use rtLength to determine a wave segment duration.
      dmime: Don't extend the current chunk to the loop end point.
      dmime: Also use rtLength for the first loop iteration.
      dmime: Use start_time as a reference when converting rtLength to music time.
      dmime: Update start_time before end_point is overwritten.
      dmime: Ignore the loop points if DMUS_SEGIOF_REFLENGTH is set.
      dmime: Don't modify the value reported by IDirectMusicState::GetStartTime.
      dmime: Only use the start point on the first loop iteration.
      dmime: Loop at the segment end if the loop end point is zero.
      dmime: Take the loop start point into account when updating start_time.
      dmime: Play the segment to the end on the last loop iteration.
      dmime/tests: Use WORD instead of struct expected_note.
      dsound: Apply volume in put_samples.
      dsound: Perform mixing in put_samples.
      dsound: Multiply vols by firgain instead of applying it in downsample.
      winmm: Use wine_midi_stream_free() to free MIDI stream.
      ntoskrnl: Set Problem for devices in HKEY_DYN_DATA\Config Manager\Enum.

Aurimas Fišeras (1):
      po: Update Lithuanian translation.

Bernhard Kölbl (2):
      kernel32: Add GetNamedPipeClientComputerName(A|W) stubs.
      kernel32/tests: Add basic tests for GetNamedPipeClientComputerNameA.

Bernhard Übelacker (3):
      wininet: Improve invalid parameter handling in APPINFO_QueryOption.
      wininet: Improve invalid parameter handling in HTTPREQ_QueryOption.
      wine.inf: Create the syswow64/drivers directory.

Brendan McGrath (16):
      quartz/tests: Compare all fields within AM_MEDIA_TYPE.
      quartz/tests: Test source media types for different sink media types.
      quartz/tests: Test diff of RGB and X values separately.
      quartz/tests: Test all combinations of input to output.
      quartz: Add color converter stub.
      quartz: Add the source and sink pins.
      quartz: Add the IMediaSeeking source interface.
      quartz: Add the IMemInputPin sink interface.
      quartz: Implement QueryAccept on sink.
      quartz: Implement ReceiveConnection on sink.
      quartz: Implement media type enum for source.
      quartz: Implement QueryAccept for source.
      quartz: Make sure cbBuffer is large enough to fit image.
      quartz: Implement the Receive function.
      quartz: Handle dynamic format change.
      quartz: Implement the ReceiveCanBlock function.

Brendan Shanks (13):
      winemac: Add autorelease pools to WineMetalSwapChain C wrappers.
      amstream/tests: Fix tests that are faulty because of == vs ?: operator precedence.
      comctl32/tests: Fix tests that are faulty because of == vs ?: operator precedence.
      compstui/tests: Fix tests that are faulty because of == vs ?: operator precedence.
      d3d9/tests: Fix tests that are faulty because of == vs ?: operator precedence.
      dbghelp/tests: Fix tests that are faulty because of == vs ?: operator precedence.
      dxgi/tests: Fix tests that are faulty because of == vs ?: operator precedence.
      iphlpapi/tests: Fix tests that are faulty because of == vs ?: operator precedence.
      kernel32/tests: Fix tests that are faulty because of == vs ?: operator precedence.
      ntdll/tests: Fix tests that are faulty because of == vs ?: operator precedence.
      quartz/tests: Fix tests that are faulty because of == vs ?: operator precedence.
      configure: Fix check for MoltenVK.
      tools: Create temp files in TMPDIR rather than the current directory.

Christoph Neuhauser (1):
      winevulkan: Update to VK spec version 1.4.363.

Cong Zhang (2):
      msvcrt: Implement _aligned_offset_recalloc and _aligned_recalloc.
      ucrtbase/tests: Add _aligned_recalloc and _aligned_offset_recalloc tests.

Connor McAdams (11):
      winebus: Hash the unix-side device parent in instance ID.
      winebus: Use the serial number string as the instance ID.
      winebus: Create instance IDs with a format that more closely matches native.
      winebus: Use the udev parent USB device path and sysnum or interface.
      winebus: Create a container ID for devices with multiple interfaces.
      winebus: Potentially use the iInterface string as the product string for USB HID devices.
      setupapi/tests: Add tests for incomplete device instance registry keys.
      setupapi/tests: Add tests for device instance ID validation.
      setupapi: Validate device instance ID when creating or opening device info.
      setupapi: Be less picky about device registry keys in SetupDiOpenDeviceInfo().
      ntoskrnl: Try to fix device registry key if both creation and opening fail.

Conor McCarthy (10):
      winegstreamer: Check for null wg_transform in video encoder ProcessMessage().
      winegstreamer: Do not set input_format twice in mpeg_video_codec_source_get_media_type().
      winegstreamer: Do not set stream pointer twice in reader_GetAllocateForOutput().
      winegstreamer: Return error codes from wg_transform_create_quartz().
      winegstreamer: Do not ignore failure in wma decoder SetOutputType().
      winegstreamer: Hold only one reference to the async result in media_sink_begin_finalize().
      winegstreamer: Avoid using uninitialised count in media_source_start().
      winegstreamer: Check for allocation failure in media_object_GetOutputType().
      dinput: Only wait for the input thread to complete cleanup.
      mfplat/tests: Do not create COM objects on the stack.

Elizabeth Figura (9):
      wined3d: Pass a wined3d_adapter to profile enumeration functions.
      wined3d: Allow disabling video decoding support.
      mountmgr: Implement IOCTL_STORAGE_GET_MEDIA_TYPES_EX.
      server: Move volume unmounting to a separate request.
      mountmgr: Implement FSCTL_DISMOUNT_VOLUME.
      mountmgr: Stub some FSCTLs.
      ntdll: No longer open Unix devices directly.
      ntdll: Remove cdrom ioctls.
      ntdll: Remove FSCTLs only used on a volume.

Eric Pouech (24):
      include: Guard PROCESS_POWER_INFORMATION as Wine-specific.
      include: cVal field is of type CHAR.
      cmd: Dispose of resources on error path.
      cmd/tests: Add more tests about quotes handling in FOR commands.
      cmd: Use word_iterator to parse FOR execute sets.
      cmd: Use word_iterator to parse FOR numeric args.
      cmd: Use word_iterator to execute FOR set arguments.
      cmd: Use word_iterator to handle variable setting in FOR loops.
      cmd: No longer use WCMD_parameter* when parsing SET /A expressions.
      include: Add the 32/64 variant of MEMORY_BASIC_INFORMATION.
      include: Add some new definitions to winnt.h.
      include: Update winnt.h with xstate definitions.
      include: Add missing alignment attributes.
      cmd: Properly set quotes when rebuilding commands.
      cmd: Use word iterator for HELP builtin command.
      cmd: Use word iterator for CHOICE builtin command.
      cmd: Use word iterator for MKDIR builtin command.
      cmd: Use word iterator for DEL builtin command.
      cmd: Use word iterator for RMDIR builtin command.
      cmd: Use word iterator for SETLOCAL builtin command.
      cmd: Use word iterator for TYPE builtin command.
      cmd: Use word iterator for MKLINK builtin command.
      cmd: Use word iterator for MORE builtin command.
      cmd: Use word_iterator for CLS builtin command.

Esme Povirk (6):
      gdiplus: Cache the clip HRGN on Graphics objects.
      wminet_utils: Implement Put.
      wminet_utils: Implement Delete.
      wminet_utils: Implement BeginEnumeration.
      wminet_utils: Implement Next.
      wminet_utils: Implement GetObjectText.

Etaash Mathamsetty (1):
      win32u: Don't return early if no monitor id is present.

Evan Morse (2):
      d3d8: Do not decrease texture refcount when it is already 0.
      ddraw: Stage execute buffer vertices at execute time.

Francis De Brabandere (5):
      vbscript: Allow member access on a literal or New expression.
      vbscript: Reject arguments after a call with empty parentheses.
      vbscript: Reject whitespace between a with-statement dot and the member name.
      vbscript: Allow default and property as class member variable names.
      vbscript: Use IProvideClassInfo for TypeName() of COM objects.

Gabriel Ivăncescu (6):
      mshtml: Move cycle collector IIDs to jscript.idl.
      jscript: Traverse external host objects' jsdisp props.
      jscript: Traverse external host objects' non-jsdisp edges.
      jscript: Traverse raw external objects that have gecko CC participants in the jscript GC.
      mshtml: Unlink the mshtml node from the gecko node during unlink.
      mshtml/tests: Test that the WeakMap can handle external/host objects.

Hans Leidekker (1):
      dnsapi: Use a Unix call to retrieve host information.

Herman Semenoff (7):
      wined3d: Clear lowest set bit in wined3d_bit_scan() without shifting.
      wined3d: Align cs_queue fields to cache line boundaries to avoid false sharing.
      wined3d: Use WriteRelease() instead of InterlockedExchange() for CS queue pointers.
      wined3d: Use switch statement for FOURCC format index lookups in get_format_idx().
      wined3d: Compare pixel shader ID earlier in glsl_program_key_compare().
      wined3d: Avoid calling strlen() in parse_channel_desc().
      winebuild: Avoid calling strlen() in a loop in build_windows_import_lib().

Jacek Caban (7):
      ntdll: Ignore attempts to set reserved CPSR bits.
      winebuild: Simplify __wine_spec_exports_end handling.
      winebuild: Move more code to output_relay_debug.
      ntdll: Delay ntdll relay setup until after arm64ec module is initialized.
      ntdll: Allow exposing relay descriptors with load config extension.
      ntdll: Add ARM64EC relay support.
      msvcrt: Disable relay traces for problematic functions on ARM64EC.

Jactry Zeng (1):
      winemac.drv: Fix a typo to avoid usage of an uninitialized variable.

Janne Kekkonen (1):
      krnl386.exe16: Format volume labels in 8.3 layout for INT 21h FindFirst.

Kyle S. Cooper (2):
      ddraw/tests: Add tests for depth fills landing in system memory on ddraw1.
      ddraw: Fill system memory depth buffers on the CPU.

Louis Lenders (1):
      cldapi: Add stub for CfGetPlatformInfo.

Marc-Aurel Zent (2):
      winemac: Derive IME open status from the native input source.
      winemac: Replace atomic builtins with Interlocked helpers.

Matteo Bruni (16):
      d3dx11: Always fix up texture dimensions.
      d3dx11/tests: Add a test for load info dimensions smaller than the block size.
      d3dx11: WARN() when texture creation fails.
      d3dx11: Trace load_info in D3DXCreateTexture*() functions.
      d3dx10: Always fix up texture dimensions.
      d3dx10/tests: Add a test for load info dimensions smaller than the block size.
      d3dx10: WARN() when texture creation fails.
      d3dx10: Trace load_info in D3DXCreateTexture*() functions.
      dsound: Introduce a put_samples() helper.
      dsound: Interleave channels after mixing.
      d3dx10_42: Fix the version number.
      d3dx11_42: Fix the version number.
      wine.inf: Update VC 14 runtime version registry keys.
      msvcr100: Bump the version number.
      msvcr110: Add a version number.
      msvcp140: Bump the version number to 14.51.

Michael Stefaniuc (7):
      dmusic: Error out if the Band Form has no instruments.
      dmusic: Propagate the error in parse_lbdl_list().
      dmusic: Avoid adding an unitialized band ptr in a Band Track.
      dmusic: MultiByteToWideChar() dest size is in WCHARs and not bytes.
      dmusic: Use the correct COM macros matching the iface type.
      dmime: Fix a copy&paste issue from a TRACE().
      dmusic: Remove old dead code.

Nikolay Sivov (27):
      msxml3/tests: Add some tests for save() with "unicode" encoding.
      msxml3: Accept "unicode" encoding name as an alias for UTF-16.
      dwrite/tests: Remove unreliable refcount tests.
      mfplat/tests: Do not check for exact failure codes in CreateObjectFromURL().
      mfplat/tests: Fix device manager test failures on newer systems.
      msxml6/tests: Add some tests for validateNode().
      msxml6: Fix argument validation in validateNode().
      msxml3: Move validated node type check to the helper.
      msxml6: Explicitly check validated node type in validateNode().
      msxml3: Improve check for a root element when validating.
      msxml3: Add a helper to get DTD node.
      msxml3: Validate root element name vs DTD name.
      comctl32/tests: Add a test for LVIS_CUT state in LVS_OWNERDATA lists.
      msxml3: Simplify parse error object creation.
      msxml3: Handle undeclared elements during DTD validation.
      msxml3: Handle undefined attributes during DTD validation.
      msxml3/tests: Add some supported interfaces checks.
      msxml3: Validate #FIXED attribute values during DTD validation.
      msxml3: Validate enumerated attribute values with DTD.
      msxml6/tests: Add some tests for multiple validation errors reporting.
      msxml3/tests: Add some validation tests for elements declared EMPTY.
      msxml3: Validate EMPTY elements content with DTD.
      msxml3: Validate mixed content elements with DTD.
      dwrite/tests: Add some HitTestPoint() tests.
      msxml3/tests: Add some more tests for empty elements formatting.
      msxml3: Add a SAX reader extension to report empty elements separately.
      msxml3: Preserve parsed formatting for empty elements.

Paul Gofman (12):
      bcrypt/tests: Test RSA encryption with imprecise sizes.
      bcrypt: Use block size instead of output_len in encrypt_rsa().
      bcrypt: Refactor error return in decrypt_rsa().
      bcrypt: Use block size instead of app length in decrypt_rsa().
      bcrypt: Return STATUS_BUFFER_TOO_SMALL on SYMCRYPT_BUFFER_TOO_SMALL and STATUS_INVALID_PARAMETER as default error in decrypt_rsa().
      kernel32/tests: Test setting thread group affinity with process affinity.
      ntdll: Correct handling of process affinity when setting thread group affinity.
      d3d11/tests: Test D3D11_CREATE_DEVICE_VIDEO_SUPPORT.
      dinput/tests: Add a test for winmm joystick replugging.
      winmm: Avoid duplicating the same joystick devices.
      winmm: Reacquire joystick device if it was reconnected.
      ntdll: Make sure entry is removed from queue if unlocked entry.addr precheck in RtlWaitOnAddress() suggests so.

Piotr Caban (18):
      vcruntime140_1: Don't depent on DLL_THREAD_ATTACH for CxxFrameHandler4 trylevel initialization.
      jscript: Pass invoke version to GetNextDispID.
      jscript: Use fdexEnumAll in interp_forin in html mode.
      jscript: Ignore GetNextDispID failure in interp_forin in html mode.
      lsass: Make packages lookup case insensitive.
      msvcp60: Fix mbsrtowcs() implementation.
      msvcp90: Use correct locale in codecvt_short__Getcat.
      msvcp90: Fix FMTFLAG_showpoint handling in num_put_wchar__Ffmt.
      msvcp90: Fix typos in time_put<> RTTI data.
      msvcp90: Fix error path handling in locale_ctor_locale_cstr.
      msvcp90: Fix incorrect len usage in _Strxfrm.
      msvcp90: Fix off-by-one error in codecvt_char16_do_in.
      msvcp60: Fix crash when writing NULL-terminator in basic_string::replace().
      msvcp60: Use correct locale in basic_ios::narrow and basic_ios::widen.
      msvcp90: Don't crash on NULL rdbuf in basic_ostream::operator<< functions.
      msvcp90: Don't crash on NULL rdbuf in basic_istream::operator>> functions.
      msvcp60: Don't crash on NULL rdbuf in basic_ostream::operator<< functions.
      msvcp60: Don't crash on NULL rdbuf in basic_istream::operator>> functions.

Rémi Bernon (23):
      win32u: Introduce a framebuffer_image_count helper.
      win32u: Keep the framebuffer blit programs per surface.
      win32u: Avoid using a static vbo in the framebuffer blit programs.
      win32u: Avoid using a uniform block for the gamma ramp.
      win32u: Use GLSL 1.20 shaders for program in compatibility contexts.
      win32u: Use parse_current_extensions to parse EGL device GL extensions.
      winebus: Compute device index based on parent hash and index hint.
      opengl32/tests: Load functions using a loop.
      opengl32/tests: Test rendering to another process window.
      opengl32/tests: Test rendering to another process child window.
      opengl32/tests: Test rendering to a child of another process window.
      win32u: Only check pixel formats for current process windows.
      win32u: Merge the EGL display extensions with its device extensions.
      win32u: Add dedicated helpers to allocate / free adapter objects.
      win32u: Add dedicated helpers to allocate / free device objects.
      win32u: Add dedicated helpers to allocate / free allocation objects.
      win32u: Add dedicated helpers to allocate / free mutex objects.
      win32u: Add dedicated helpers to allocate / free sync objects.
      win32u: Add dedicated helpers to allocate / free resource objects.
      winemac: Get rid of unnecessary pixel_format macdrv_win_data member.
      win32u: Don't assume that every D3DKMT sync has an unix fd.
      win32u: Don't assume every D3DKMT resource has an unix fd.
      win32u: Pass runtime data in d3dkmt keyed mutex acquire and release.

Stian Low (1):
      dxgi: Implement D3D11_CREATE_DEVICE_VIDEO_SUPPORT.

Sven Baars (1):
      wined3d: Define VKD3D_UTILS_NO_WIN32_TYPES.

Symon Wang (1):
      kernelbase: Support desktop app policy in process/thread attributes list.

Thomas Ritter (5):
      comdlg32: Size custom controls for their wrapped text.
      include: Add SmoothingModeAntiAlias8x4 and SmoothingModeAntiAlias8x8.
      gdiplus: Antialias filled paths.
      gdiplus: Draw antialiased thin lines one pixel wide.
      gdiplus: Widen antialiased lines with the default flatness.

Vasiliy Stelmachenok (4):
      win32u: Fill extensions for the display device.
      opengl32: Generate entry points for EGL_EXT_device_type extension.
      win32u: Add support for EGL_EXT_device_type.
      win32u: Add proper support for WGL_RENDERER_UNIFIED_MEMORY_ARCHITECTURE_WINE.

Vibhav Pant (3):
      include: Add notificationactivationcallback.idl.
      wpnapps: Add stub dll.
      wpnapps/tests: Add tests for displaying text-only ToastNotifications through ToastNotificationManager.

Vishnunithyasoundhar S (1):
      cmd: Fix quoted redirection targets in pipes.

Zhiyi Zhang (2):
      include: Correct the value of SECURITY_LOCAL_LOGON_RID.
      include: Add logon types and providers for LogonUserA/W().
```
