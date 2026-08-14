The Wine development release 11.15 is now available.

What's new in this release:
  - More KDF algorithms in BCrypt.
  - Support for ARM64EC build in Mingw mode.
  - More format conversions in WindowsCodecs.
  - Support for local NTLM authentication.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/11.x/wine-11.15.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-11.15/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 11.15 (total 41):

 - #4811   MSXML3: XMLDOMDocument cannot be queried for IStream interface
 - #11595  Notepad++ versions prior to 6.9 freeze if native application changes a file it has open when not in a virtual desktop (dogfood)
 - #20584  Lemmings Revolution - Crash on exit.
 - #21940  Rise of Legends Demo crashes with null pointer reference in msxml3?
 - #23319  cmd thinks shift.exe is an internal command
 - #30239  Multiple Adobe CS6 trial installers fail to initialize, reporting 'Exception caught while getting payloads data combined. Error #1090'
 - #33984  Nokia Music Player: extraneous transparent gray border in installer window
 - #35727  Tom Clancy's Rainbow Six: Lockdown Demo wrong players and weapon rendering
 - #37896  EM_SETPASSWORDCHAR ineffective on multiline edit controls
 - #43031  popen() hangs when both stdin and stdout are closed
 - #50109  conhost: write_console beep doesn't produce a beep
 - #54928  exclamation mark ! in string missing if EnableDelayedExpansion
 - #55198  QuickBooks SS 2009 installer crash
 - #55447  wine cannot parse valid javascript
 - #56399  cmd.exe's %~s modifier (short DOS path) expands to mixed short/long format
 - #57967  Wrong Z-order in Browse for Folder from MPC-HC 1.7.13
 - #58112  Unimplemented function KERNEL32.dll.InitializeSynchronizationBarrier
 - #59073  Washed out colors in OpenGL
 - #59436  Wayland/EGL: Washed-out colors caused by double sRGB conversion on default framebuffer (GL_FRAMEBUFFER_SRGB)
 - #59898  Wine on aarch64 fails on linux with CONFIG_ARM64_VA_BITS=39
 - #59926  CharPrevA/CharPrevExA crash on NULL start pointer (MapleStory 216150 fails to launch)
 - #59970  monitor_get_dpi will get zero dmPelsWidth/dmPelsHeight in both physical/current cause divide by zero
 - #59980  Final Fantasy XI Online: Unexpected Right Shift behaviour.
 - #59986  Mouse misbehaves in Steam
 - #59998  Multiple applications freeze (Mugen, Touhou 8, Final Burn Neo)
 - #59999  Application (e. g. WinSCP) freeze after resize its window
 - #60002  YNAB 4 mouse position incorrect
 - #60005  Pegasus hangs and crashes
 - #60012  HOTAS controlls by WinWing are not recognized and even aggresively mapped as some x-box controller.
 - #60023  Dialogs in some applications are rendered at a quarter of their actual size in Wine 11.13 under certain conditions, e.g. DPI = 144 (IrfanView, WinSCP)
 - #60036  WMI queries via PS Core do not work with mono.
 - #60037  Fixed resolution 4:3 fullscreen apps on Wayland have their content offset and present some undefined padding area
 - #60046  Multiple 32-bit applications including wine builtins (winecfg / regedit / etc.) crash under wine-11.13.
 - #60051  KakaoTalk intermittently hangs
 - #60057  gdiplus: GdipCloneBitmapArea drops trailing pixels for 1bpp/4bpp bitmaps with non-byte-aligned width
 - #60063  shell32: SHBrowseForFolder crashes when an application passes an invalid BROWSEINFO.pidlRoot
 - #60068  wminet_utils: implement GetErrorInfo()
 - #60069  WordPerfect 7 installation fails
 - #60088  ole32/storage: extra trailing NUL byte written into name of stream nested in a substorage (breaks MS Publisher .pub file portability)
 - #60093  Unable to combine 32 bit arm with i386 emulation on aarch64
 - #60120  Amnesia: Rebirth crashes after the splash screens

### Changes since 11.14:
```
Alex Dujardin (4):
      windowscodecs: Add 32bppBGRA, 32bppPBGRA -> 64bppPRGBA conversion.
      windowscodecs: Add 64bppPRGBA -> 128bppRGBAFloat conversion.
      windowscodecs: Add 64bppPRGBA -> 128bppPRGBAFloat conversion.
      windowscodecs: Remove incorrectly added bound check.

Alex Henrie (3):
      kernelbase: Make Char(Next|Prev)A call Char(Next|Prev)ExA.
      kernelbase: Search backwards in CharPrevExA.
      user32/tests: Add tests for CharPrev(Ex)A.

Alexandre Julliard (28):
      wmc: Remove an unused variable.
      install: Remove an unused variable.
      server: Pass unicode_str objects by value where possible.
      server: Add a structure to store object initialization parameters.
      server: Pass an object parameters structure to create_named_object().
      server: Pass an object parameters structure to open_named_object().
      server: Add an init() object operation.
      server: Implement the init() operation for events.
      server: Implement the init() operation for mutexes.
      server: Implement the init() operation for semaphores.
      server: Implement the init() operation for timers.
      server: Implement the init() operation for debug objects.
      server: Implement the init() operation for ALPC ports.
      server: Implement the init() operation for completion objects.
      server: Implement the init() operation for directories.
      server: Implement the init() operation for symlinks.
      server: Implement the init() operation for mailslots.
      server: Implement the init() operation for named pipes.
      server: Implement the init() operation for devices.
      server: Implement the init() operation for sections.
      server: Implement the init() operation for registry keys.
      server: Implement the init() operation for reserve objects.
      server: Implement the init() operation for job objects.
      server: Implement the init() operation for window stations and desktops.
      server: Move the name length check into open_named_object().
      server: Add a helper to create an object and its handle.
      symcrypt: Don't enforce any alignment in non-PE builds.
      ntdll: Redirect to sysarm32 directory on ARM wow64.

Alfred Agrell (3):
      vidreszr: Register the correct media types.
      vidreszr: Implement CResizerDMO by piggybacking on CColorConvertDMO.
      mf: Add tests for video resizer DMO.

Alistair Leslie-Hughes (5):
      include: Add more defines in wdm.h.
      include: Add more defines in fltkernel.h.
      include: Add Flag* defines.
      include: Add fltuser.h.
      include: Add fltuserstructures.h.

Allan Vester (2):
      bcrypt: Add tests for BCRYPT_INITIALIZATION_VECTOR.
      bcrypt: Support BCRYPT_INITIALIZATION_VECTOR.

Arkadiusz Hiler (1):
      winebus: Fix initial axis values for evdev gamepads.

Attila Fidan (2):
      shell32/tests: Test SHBrowseForFolderW() with CSIDL values.
      shell32: Accept CSIDL values in SHBrowseForFolderW().

Barath Kannan (1):
      conhost: Add beep functionality to write_console.

Benoît Legat (1):
      crypt32: Ignore trailing bytes past the outer SEQUENCE in PFXImportCertStore.

Bernhard Übelacker (3):
      win32u: Fix off-by-one in kbdus_tables.
      comctl32/tests: Add test of LM_GETIDEALSIZE being more greedy in width.
      comctl32_v6: Improve LM_GETIDEALSIZE to be more greedy in width.

Billy Laws (1):
      ntdll: Add RtlWow64SuspendThread semi-stub implementation.

Brendan Shanks (5):
      dinput/tests: Fix tests that are faulty because of == vs ?: operator precedence.
      msado15/tests: Fix typos.
      advapi32/tests: Fix typo.
      msvcp120/tests: Fix tests that are faulty because of == vs ?: operator precedence.
      ucrtbase/tests: Fix tests that are faulty because of == vs ?: operator precedence.

Conor McCarthy (1):
      mfreadwrite: Do not return NEED_MORE_INPUT from source_reader_push_transform_samples().

Dan Fraser (1):
      dnsapi: Add stubs for the DNS-SD service entry points.

Dean M Greer (1):
      documentation: Update macOS version.

Elizabeth Figura (11):
      d3d11/tests: Shrink NV12 test textures a bit.
      wined3d: Separate a surface_cpu_blt_plane() helper.
      wined3d: Implement planar CPU blits.
      d3d11/tests: Test planar CPU blits.
      wined3d: Separate a wined3d_decoder_vk_prepare_image() helper.
      wined3d: Separate a wined3d_decoder_vk_create_layered_image() helper.
      wined3d: Separate a wined3d_decoder_vk_destroy_images() helper.
      wined3d: Handle dynamic resizing of the H.264 stream.
      win32u: Make win32u_vkDestroySwapchainKHR() static.
      win32u: Do not use EXT_external_memory_dma_buf.
      ntdll: Search for the library containing sa_restorer.

Eric Pouech (3):
      winedbg: Support larger strings in DebugString.
      winedbg: Let 'maint module' be a bit more useful.
      winedbg: Support watch operation larger than 4 bytes.

Esme Povirk (2):
      winedump: Use the Length from the metadata header.
      winedump: Handle incremental CLR images.

Etaash Mathamsetty (1):
      winewayland: Use the visible rect for toplevel rect.

Hans Leidekker (10):
      bcrypt: Add support for BCRYPT_TLS1_{1,2}_KDF_ALGORITHM.
      bcrypt: Support retrieving hash block length.
      bcrypt: TLS1 KDF label is optional.
      bcrypt: Add support for BCRYPT_HKDF_ALGORITHM.
      msi: Close the RPC connection on dll unload.
      secur32: Use bcrypt to hash the certificate in schan_QueryContextAttributesW().
      winhttp: Support WinHttpQueryOption(WINHTTP_OPTION_SERVER_CBT).
      winhttp: Stub WinHttpSetOption(WINHTTP_OPTION_ASSURED_NON_BLOCKING_CALLBACKS).
      winhttp: Stub WinHttpSetOption(WINHTTP_OPTION_ENABLE_HTTP2_PLUS_CLIENT_CERT).
      crypt32: Use the strong provider in CertCreateSelfSignCertificate().

Henri Verbeet (1):
      wined3d: Add GPU information for AMD REMBRANDT.

Iliya Andrienko (2):
      gdiplus: Fix convert_pixels() rounding down bytes.
      gdiplus: Add sub-byte formats logic to GdipImageRotateFlip.

Jacek Caban (2):
      winegcc: Always add Clang target options to the compiler command when building ARM64X image.
      ntdll: Run native-ready .net applications as ARM64EC on ARM64.

Jannis Lübke (3):
      wined3d: Don't hold wined3d cs mutex when waiting for frame latency.
      d3dx9/tests: Add tests for D3DXIntersect() and D3DXIntersectSubset().
      d3dx9: Implement D3DXIntersect() and D3DXIntersectSubset().

Ken Sharp (1):
      po: Update English resource.

Lokesh Poovaragan (4):
      jscript: Don't insert implicit semicolon after '.' in member expressions.
      cmd: Handle caret escape in delayed expansion.
      cmd/tests: Add tests for %~s short path modifier.
      cmd: Fix %~s modifier to produce fully short paths.

Louis Lenders (7):
      wminet_utils: Add NextMethod.
      wminet_utils: Add EndMethodEnumeration.
      wminet_utils: Add GetMethod.
      wminet_utils: Add GetMethodQualifierSet.
      wminet_utils: Add QualifierSet_Get.
      wminet_utils: Add GetPropertyQualifierSet.
      wminet_utils: Add GetErrorInfo.

Maotong Zhang (2):
      comctl32/tests: Add test for RBBS_VARIABLEHEIGHT band height handling.
      comctl32: Clamp child height when cyIntegral is zero.

Martin Storsjö (2):
      include: Use __atomic_exchange_n on Clang.
      include: Fix building for arm64ec in mingw mode.

Matteo Bruni (3):
      winex11: Get rid of special handling for right shift in X11DRV_GetKeyNameText().
      win32u: Don't ignore raw mouse input.
      d3dx9/tests: Fix a flaky D3DXWeldVertices() test.

Nello De Gregoris (1):
      ntoskrnl.exe: Add stub for KeGetCurrentIrql().

Nikolay Sivov (20):
      comdlg32/filedlg: Return current path as is for CDM_GETFILEPATH if it's absolute.
      ole32/storage: Update name size field on rename.
      msxml3/tests: Add a test for the document stream.
      msxml3/tests: Add another stream read test.
      msxml3: Write out UTF-16 BOM in save().
      msxml3/tests: Extend a test with multiple writing streams.
      msxml3/tests: Adjust stream test XML data.
      msxml3/stylesheet: Improve processor output object management.
      msxml3/dom: Add IStream support for the document object.
      msxml3: Remove now unnecessary workaround in save().
      msxml3: Use ISequentialStream internally for save().
      msxml3: Explicitly handle ISequentialStream in save().
      msxml3: Add explicit messages for currently unsupported destination types in save().
      comdlg32/tests: Add a CDM_SETCONTROLTEXT(edt1) test.
      comdlg32: For CDM_SETCONTROLTEXT(edt1) always use current file control.
      msxml3: Handle null destination object in save().
      msxml3/tests: Add a test for loading from a document.
      msxml3: Remove workaround when loading from a document in load().
      include: Add IRequest definition.
      msxml3: Explicitly check and warn about currently unsupported source types in load().

Paul Gofman (10):
      winebus: Avoid spurious device recreation in process_inotify_event().
      kernel32/tests: Add tests for unhandled exception filter with BeingDebugged PEB flag set.
      kernel32/tests: Add tests for unhandled exception filter under debugger.
      kernelbase: Query debug port instead of BeingDebugged PEB flag in UnhandledExceptionFilter().
      crypt32: Avoid adding spurious line separator in encodeBase64[A/W]().
      crypt32: Fix output string tracing in quote_rdn_value_to_str_w().
      winhttp: Handle incorrect handle state when querying WINHTTP_OPTION_SERVER_CERT_CHAIN_CONTEXT.
      winhttp: Use chain established in netconn_verify_cert() for WINHTTP_OPTION_SERVER_CERT_CHAIN_CONTEXT.
      cmd/tests: Add test for console mode change.
      cmd: Set console mode in node_execute().

Piotr Caban (23):
      secur32/tests: Add NTLM QueryCredentialsAttributes(SECPKG_CRED_ATTR_NAMES) tests.
      msv1_0: Use calloc to allocate credentials handle in ntlm_SpAcquireCredentialsHandle.
      msv1_0: Add ntlm_SpQueryCredentialsAttributes stub.
      msv1_0: Add ntlm_SpQueryCredentialsAttributes implementation.
      msv1_0: Hide password in logs.
      msv1_0: Remove FLAG_NEGOTIATE_* defines and use NTLMSSP_NEGOTIATE_* instead.
      ntoskrnl: Implement IoGetRequestorProcessId.
      msv1_0: Implement in-process NTLM local authentication.
      msvcr120/tests: Restore exp() tests.
      ucrtbase: Fix exp(NAN) handling.
      msv1_0: Don't overwrite output buffer when getting session key.
      include: Declare SEC_WINNT_AUTH_IDENTITY_EX2.
      secur32/tests: Add SspiMarshalAuthIdentity tests.
      secur32/tests: Add SspiUnmarshalAuthIdentity tests.
      sspicli: Handle more auth identity formats in SspiZeroAuthIdentity.
      sspicli: Handle more auth identity formats in SspiEncodeAuthIdentityAsStrings.
      sspicli: Handle more auth identity formats in SspiPrepareForCredWrite.
      sspicli: Change auth identity format returned by SspiEncodeStringsAsAuthIdentity.
      sspicli: Use LocalAlloc/LocalFree when allocating auth identity.
      sspicli: Handle more auth identity formats in SspiFreeAuthIdentity.
      secur32: Implement SspiMarshalAuthIdentity() and SspiUnmarshalAuthIdentity().
      secur32: Pass stub LSA_SECPKG_FUNCTION_TABLE to SpInitialize.
      msv1_0: Get clients process id and thread id using LSA functions table.

Rick Rey (1):
      winebus.sys: Prefer hidraw for (some) switch 1 controllers.

Rémi Bernon (10):
      opengl32: Use unsigned handle entry pointer index.
      opengl32: Add some missing SetLastError on invalid handles.
      win32u: Use system DPI as monitor DPI for detached sources.
      user32/tests: Fix monitor DPI awareness tests when system DPI isn't 96.
      user32/tests: Test window DPI context and monitor DPI in CBT hooks.
      user32/tests: Check child window DPI awareness and monitor changes.
      win32u: Lock host window state updates when applying new state.
      opengl32: Pass NULL object array pointers through.
      dinput/tests: Fix incorrect GetOverlappedResult test expectation.
      win32u: Pass initial monitor DPI to the window creation request.

Santino Mazza (3):
      d2d1/tests: Add some tests for sprite batches.
      d2d1: Implement sprite batch object methods.
      cmd: Fix usage of uninitialized variable in node_build_parse.

Thomas Portal (2):
      dnsapi: Implement DnsServiceConstructInstance() and DnsServiceFreeInstance().
      dnsapi/tests: Add tests for DnsServiceConstructInstance().

Tobiasz Laskowski (9):
      jscript: Fix String.replace with multi-digit group.
      jscript: Fix internal state of RegExp.prototype.
      jscript/tests: Add tests for RegExp.prototype use.
      comctl32/tests: Check listbox SETTOPINDEX errors.
      user32/tests: Check listbox SETTOPINDEX errors.
      comctl32/listbox: Handle out-of-bounds SETTOPINDEX.
      user32/listbox: Handle out-of-bounds SETTOPINDEX.
      jscript: Fix zero-length regex matches.
      jscript/tests: Add more tests for 0-length regex.

Tomáš Novotný (1):
      winebus: Enable hidraw by default for Winwing Orion controllers.

Vibhav Pant (4):
      rometadata: Implement IMetaDataImport::{EnumMembers, EnumMembersWithName}.
      rometadata: Implement IMetaDataImport::FindMember.
      rometadata/tests: Add tests for IMetaDataImport::{EnumCustomAttributes, GetCustomAttributeProps}.
      maintainers: Add entry for WinRT Metadata.

Vlad Zahorodnii (1):
      winewayland: Add support for wl_fixes.ack_global_remove.

Yuxuan Shui (4):
      mfplat/tests: Test how the MPEG4 media source handles aspect ratio info.
      mfreadwrite: Don't modify the `type` parameter in src_reader_SetCurrentMediaType.
      mfreadwrite/tests: Test how aspect ratios are handled by the reader.
      mfreadwrite: Ensure output has a 1:1 pixel aspect ratio if processing is enabled.

Zhiyi Zhang (1):
      winex11.drv: Remove some noisy traces.
```
