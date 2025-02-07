The Wine development release 10.1 is now available.

What's new in this release:
  - A wide range of changes that were deferred during code freeze.
  - Root certificates fixes for Battle.net.
  - Print Provider improvements.
  - More progress on the Bluetooth driver.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/10.x/wine-10.1.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-10.1/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 10.1 (total 35):

 - #27245  Internet Settings security zones not i18n-ed
 - #35981  Battlefield: Bad Company 2 (Russian locale) updater has missing glyphs
 - #39576  Sound in StarCraft 2 breaks after
 - #39733  OpenGL Extensions Viewer 4.x (.NET 4.0 app) fails to start with Wine-Mono
 - #41342  Build with winegcc is not reproducible
 - #46580  HoMM3 WOG: can't enter russian text speaking with sphynx
 - #46702  GNUTLS_CURVE_TO_BITS not found
 - #52221  GameMaker 8: Missing sound effects
 - #53644  vbscript can not compile classes with lists of private / public / dim declarations
 - #54752  RUN Moldex3D Viewer will Crash
 - #55155  Telegram can not be run in latest version wine, but ok in wine6.0.4
 - #56530  Final Fantasy XI Online: Memory leak when Wine is built with CFLAGS="-g -mno-avx".
 - #56559  iologo launcher cannot download setup program
 - #56658  When using Kosugi for vertical writing, some punctuation marks are not placed correctly.
 - #56703  Crash when installing Rhinoceros 8.6
 - #56876  Paint Tool SAIv2 VirtualAlloc invalid address on commit
 - #57191  Flickering image on Video-surveilance-Software
 - #57338  wine-gecko/wine-mono don't cache their installers if using a username with unicode characters
 - #57360  Wrong Combobox dropdown in 7zFM
 - #57529  reMarkable application crash on new winehq-devel 10 RC1
 - #57563  vbscript: mid() throws when passed VT_EMPTY instead of returning empty string
 - #57626  SteuerErklarung 2025 halts: windows 8 is not compatible
 - #57650  osu! stable: Insert key to minimize to tray does not hide game window (regression)
 - #57664  New problems with SudoCue under Win 10.0 rc5
 - #57675  err:virtual:virtual_setup_exception stack overflow 3072 bytes addr 0x7bd5b54c stack 0x81100400
 - #57689  Menus misplaced on X11 when using dual monitor with right monitor as primary
 - #57690  .NET Framework 4.8 installer hangs
 - #57692  No context menu in Reason (DAW)
 - #57698  Reason's (DAW) dialog windows stopped registering mouse events and open at screen's right edge
 - #57704  Compile Error since 10.0rc5+
 - #57710  Cannot open main menu via keyboard in Reason (DAW)
 - #57711  The 32-bit wpcap program has a stack leakage issue
 - #57766  Win3_BIOS most likely should be Win32_BIOS instead
 - #57787  Final Fantasy XI Online crashes with unhandled page fault on launch
 - #57794  WinHTTP implementation assumes HTTP response has a status text

### Changes since 10.0:
```
Akihiro Sagawa (2):
      gdi32/tests: Add tests for script-independent vertical glyph lookup.
      win32u: Use the first vertical alternates table regardless of script.

Alex Henrie (20):
      concrt140: Annotate allocators with __WINE_(ALLOC_SIZE|DEALLOC|MALLOC).
      msvcirt: Annotate allocators with __WINE_(ALLOC_SIZE|DEALLOC|MALLOC).
      msvcp60: Annotate allocators with __WINE_(ALLOC_SIZE|DEALLOC|MALLOC).
      msvcp90: Annotate allocators with __WINE_(ALLOC_SIZE|DEALLOC|MALLOC).
      msvcrt: Annotate allocators with __WINE_(ALLOC_SIZE|DEALLOC|MALLOC).
      ntdll: Return an error if count is zero in NtRemoveIoCompletionEx.
      shell32/tests: Add tests for StrRetToStrN null termination.
      shlwapi: Correct return value of StrRetToBuf on an invalid type.
      shell32: Use StrRetToBuf instead of reimplementing it.
      comdlg32: Use StrRetToBuf instead of reimplementing it.
      hhctrl: Fix spelling of "local" in OnTopicChange.
      tapi32: Use wide character string literals.
      winhlp32: Use wide character string literals.
      shlwapi: Use wide character string literals.
      cryptui: Use wide character string literals.
      hhctrl: Use wide character string literals.
      urlmon: Make security zone names and descriptions translatable.
      ieframe: Use wide character string literals.
      hhctrl: Fix window class name in HH_CreateHelpWindow.
      hhctrl: Make "Select Topic" window title translatable.

Alexandre Julliard (39):
      shell32: Move some function prototypes to shlwapi.h.
      winedump: Dump MUI resources.
      wrc: Remove the unused res_count structure.
      include: Mark global asm functions as hidden.
      kernel32: Implement RegisterWaitForInputIdle().
      wineps: Don't store the glyph name or encoding for individual glyphs.
      wineps: Don't parse the glyph name or encoding when loading AFM files.
      wineps: Remove some unused AFM values.
      wineps: Use simple strings for glyph names.
      win32u: Implement NtUserDestroyCaret().
      win32u: Implement NtUserReleaseCapture().
      win32u: Implement NtUserGetThreadState().
      win32u: Implement NtUserCreateMenu() and NtUserCreatePopupMenu().
      win32u: Implement NtUserEnumClipboardFormats().
      win32u: Implement NtUserMessageBeep().
      win32u: Implement NtUserPostQuitMessage().
      win32u: Implement NtUserRealizePalette().
      win32u: Implement NtUserReplyMessage().
      win32u: Implement NtUserSetCaretBlinkTime().
      win32u: Implement NtUserSetCaretPos().
      win32u: Implement NtUserSetProcessDefaultLayout().
      win32u: Implement NtUserGetClipCursor().
      win32u: Implement NtUserArrangeIconicWindows().
      win32u: Implement NtUserDrawMenuBar().
      win32u: Implement NtUserGetWindowContextHelpId().
      win32u: Implement NtUserSetProgmanWindow() and NtUserSetTaskmanWindow().
      win32u: Implement NtUserEnableWindow().
      win32u: Implement NtUserSetWindowContextHelpId().
      win32u: Implement NtUserShowOwnedPopups().
      win32u: Implement NtUserUnhookWindowsHook().
      win32u: Implement NtUserValidateRgn().
      winedbg: Remove unneeded wrap around checks.
      winegcc: Always specify the output file name when there's no spec file.
      kernel32/tests: Remove some workarounds for old Windows versions.
      kernel32/tests: Add tests for language-specific manifest lookup.
      ntdll: Implement language-specific manifest lookup.
      server: Do not allow creating mailslots with zero access.
      server: Do not allow creating named pipes with zero access.
      server: Skip non-accessible threads in NtGetNextThread().

Alexandros Frantzis (1):
      winewayland: Round the Wayland refresh rate to calculate the win32 display frequency.

Arkadiusz Hiler (1):
      jscript: Fix JSON.stringify for arrays longer than 10.

Bernhard Übelacker (1):
      kernel32/tests: Match the value in the debug message the test condition.

Billy Laws (1):
      ntdll: Detect kernel support before using ARM64 ID regs.

Brendan McGrath (2):
      mfplat/tests: Add audio tests for MFInitMediaTypeFromAMMediaType.
      mfplat: Add support for audio with NULL format to MFInitMediaTypeFromAMMediaType.

Brendan Shanks (2):
      winemac: [NSWindow setAlphaValue:] must be called from the main thread.
      winemac: [NSWindow contentView] must be called from the main thread.

Conor McCarthy (2):
      mfsrcsnk: Release object queue objects on destruction.
      mfsrcsnk: Release the async request popped sample after sending it.

Damjan Jovanovic (1):
      user32: Copy the clipboard format iterator's position when cloning it.

Daniel Lehman (4):
      msvcp140/tests: Add more tests for _Mtx_t.
      msvcp140: Fix field order in _Mtx_t.
      include: Add signbit declarations for c++.
      msvcp140: Add padding to _Cnd_t.

Dean M Greer (2):
      documentation: Mac OS X became macOS from 10.12.
      readme: Update mac info section.

Dmitry Timoshkov (31):
      wldap32: Avoid crashes in interact_callback() if defaults is NULL.
      wldap32: Add stub for ldap_get_option(LDAP_OPT_GETDSNAME_FLAGS).
      wldap32: Add stub for ldap_set_option(LDAP_OPT_GETDSNAME_FLAGS).
      wldap32: Add stub for ldap_set_option(LDAP_OPT_PROMPT_CREDENTIALS).
      wldap32: Add stub for ldap_set_option(LDAP_OPT_REFERRAL_CALLBACK).
      prntvpt: Also initialize dmDefaultSource field.
      prntvpt: Forward BindPTProviderThunk to PTOpenProviderEx.
      prntvpt: Forward UnbindPTProviderThunk to PTCloseProvider.
      prntvpt: Implement ConvertDevModeToPrintTicketThunk2.
      prntvpt: Implement GetPrintCapabilitiesThunk2.
      prntvpt: Implement ConvertPrintTicketToDevModeThunk2.
      prntvpt: Prefer builtin.
      prntvpt: Add version resource.
      kerberos: Update sizes to match modern implementations.
      secur32: Update max token size for Negotiate.
      prntvpt: Implement writing PageMediaSize capabilities.
      prntvpt: Implement writing PageImageableSize capabilities.
      crypt32: Add support for CryptMsgControl(CMSG_CTRL_ADD_CERT) to a being decoded message.
      crypt32: Add szOID_APPLICATION_CERT_POLICIES to the list of supported critical extensions.
      crypt32: Ignore CRYPT_OID_INFO_PUBKEY_ENCRYPT_KEY_FLAG and CRYPT_OID_INFO_PUBKEY_SIGN_KEY_FLAG in CryptFindOIDInfo().
      crypt32: Add support for CryptMsgControl(CMSG_CTRL_ADD_SIGNER_UNAUTH_ATTR) to a being decoded message.
      crypt32: Add support for CryptMsgGetParam(CMSG_ENCRYPTED_DIGEST) to a being decoded signed message.
      crypt32: Add support for CryptMsgControl(CMSG_CTRL_DEL_CERT) to a being decoded message.
      crypt32: CertVerifyCertificateChainPolicy() extensions are registered under "EncodingType 0" key.
      crypt32: Do not reject key usage data longer than 1 byte.
      compstui: Add more string resources.
      netapi32: Add stubs for DsGetDcOpenA/W.
      wldap32: Use correct host when connecting to Kerberos DC.
      wldap32: ldap_init() should resolve NULL hostname to default Kerberos DC.
      wldap32/tests: Add some tests for LDAP authentication to a Kerberos DC.
      prntvpt: Use Windows 10 version numbers.

Ekaterine Papava (1):
      po: Update Georgian translation.

Elizabeth Figura (21):
      msi: Fix a spelling error in the name of MigrateFeatureStates.
      wined3d: Feed WINED3D_RS_POINTSIZE through a push constant buffer.
      wined3d: Implement point size in the HLSL FFP pipeline.
      wined3d: Implement vertex blending in the HLSL FFP pipeline.
      wined3d: Feed bumpenv constants through a push constant buffer.
      wined3d: Implement bumpenv mapping in the HLSL FFP pipeline.
      wined3d: Implement colour keying in the HLSL FFP pipeline.
      wined3d: Bind the right push constant buffers when FFP is toggled.
      wined3d: Use ps_compile_args in shader_spirv_compile_arguments.
      wined3d/spirv: Implement flat shading.
      wined3d/glsl: Move legacy alpha test to shader_glsl_apply_draw_state().
      wined3d: Feed alpha ref through a push constant buffer.
      wined3d/spirv: Implement alpha test.
      d3d9: Fix IUnknown delegation in IDirect3DDevice9On12.
      wined3d: Do not disable point sprite in wined3d_context_gl_apply_blit_state().
      wined3d: Do not toggle point sprite.
      wined3d: Remove the redundant per_vertex_point_size from vs_compile_args.
      wined3d: Remove FOGVERTEXMODE handling from find_ps_compile_args().
      d3d11/tests: Remove the workaround for RTVs in test_nv12().
      wined3d: Use the correct pitch when downloading Vulkan textures.
      wined3d: Factor out a get_map_pitch() helper.

Eric Pouech (3):
      include: Add some new definitions for dbghelp.h.
      include: Add a couple of definitions to mscvpdb.h.
      include: Use flexible array-member in some structure declarations.

Esme Povirk (5):
      gdiplus: AddClosedCurve always starts a new subpath.
      gdiplus: GdipAddPathPie always starts a new figure.
      appwiz.cpl: Account for unicode characters in XDG_CACHE_HOME.
      user32/tests: Move a todo into the message sequence.
      user32/tests: Account for Wine sometimes duplicating WM_PAINT.

Etaash Mathamsetty (3):
      wine.inf: Add UBR key.
      winecfg: Add support for UBR key.
      twinapi.appcore/tests: Fix broken registry query.

Fabian Maurer (4):
      mlang: In GetFontCodePages add another null check (Coverity).
      include: Add IConnectionProfile2.
      windows.networking.connectivity: Add IConnectionProfile2 stubs.
      windows.networking.connectivity: Fake success for IsWwanConnectionProfile and IsWlanConnectionProfile.

Francis De Brabandere (1):
      vbscript: Support multiple class declarations on a single line.

Georg Lehmann (1):
      winevulkan: Update to VK spec version 1.4.307.

Hans Leidekker (9):
      xcopy: Fix handling of quoted filenames.
      wpcap: Fix callback signature.
      wine.inf: Pass command line arguments to msiexec.
      wbemprox: Read Win32_PnPEntity values from the registry.
      wbemprox: Implement Win32_PnPEntity.Service.
      wbemprox/tests: Fix typo.
      wbemprox: Fix allocation size.
      ntdll: Stub NtQuerySystemInformation(SystemLeapSecondInformation).
      winhttp: Accept server response without status text.

Herman Semenov (1):
      msvcrt: Add missing TRACE_ON check.

Jacek Caban (1):
      wdscore: Don't export C++ symbols.

Jactry Zeng (7):
      include: Don't import .idl when DO_NO_IMPORTS is defined.
      wintypes/tests: Add interface tests.
      wintypes: Reimplement Windows.Foundation.Metadata.{ApiInformation,PropertyValue} separately.
      wintypes: Stub of Windows.Storage.Streams.DataWriter runtimeclass.
      wintypes: Return S_OK from data_writer_activation_factory_ActivateInstance().
      wintypes: Stub of Windows.Storage.Streams.RandomAccessStreamReference runtimeclass.
      wintypes: Stub of IRandomAccessStreamReferenceStatics interface.

Jeff Smith (2):
      windowscodecs: Make values returned from CanConvert consistent.
      windowscodecs: Simplify png_decoder_get_metadata_blocks using realloc.

Jinoh Kang (3):
      ntoskrnl.exe: Fix IRQL mismatch between cancel spin lock acquire and release.
      kernel32/tests: Use win_skip() for missing PrefetchVirtualMemory API.
      ntdll: Fix syscall_cfa offset in user_mode_abort_thread for ARM64.

Makarenko Oleg (2):
      dinput/tests: Add more tests for force feedback.
      dinput: Clamp FFB effect report value to the field range.

Marc-Aurel Zent (1):
      server: Use setpriority to update thread niceness when safe.

Mohamad Al-Jaf (10):
      windows.networking.hostname: Guard against WindowsDuplicateString() failure.
      windows.ui: Stub IUISettings3::add_ColorValuesChanged().
      windows.ui: Stub IUISettings3::remove_ColorValuesChanged().
      windows.ui.xaml: Add stub dll.
      windows.ui.xaml: Add IColorHelperStatics stub interface.
      windows.ui.xaml/tests: Add IColorHelperStatics::FromArgb() tests.
      windows.ui.xaml: Implement IColorHelperStatics::FromArgb().
      include: Add d3d9on12.idl file.
      d3d9: Implement Direct3DCreate9On12().
      d3d9/tests: Add Direct3DCreate9On12() tests.

Nikolay Sivov (43):
      oleaut32/tests: Use correct constants for IStream::Seek().
      windowscodecs/tests: Use correct constants for IStream::Seek().
      windowscodecs/metadata: Use correct constants for IStream::Seek().
      windowscodecs/ddsformat: Use correct constants for IStream::Seek().
      d2d1: Use correct constants for IStream::Seek().
      dmloader: Use correct constants for IStream::Seek().
      kernel32/tests: Use correct constants for SetFilePointer().
      wintrust: Use correct constants for SetFilePointer().
      storage: Use correct constants for SetFilePointer().
      krnl386: Use correct constants for SetFilePointer().
      shell32: Use correct constants for SetFilePointer().
      dmloader: Use correct constants for SetFilePointer().
      include: Update with newer Direct2D types.
      include: Update with newer DirectWrite types.
      propsys/tests: Add some tests for PropVariantChangeType(VT_UI4).
      propsys: Implement PropVariantToStringAlloc(VT_UI2).
      propsys: Implement PropVariantToStringAlloc(VT_I4).
      propsys: Implement PropVariantToStringAlloc(VT_I2).
      propsys: Implement PropVariantToStringAlloc(VT_I1).
      propsys: Implement PropVariantToStringAlloc(VT_UI1).
      propsys: Implement PropVariantToStringAlloc(VT_UI4).
      propsys: Implement PropVariantToStringAlloc(VT_I8).
      propsys: Implement PropVariantToStringAlloc(VT_UI8).
      propsys: Remove FIXME() from PropVariantChangeType().
      windowscodecs/metadatahandler: Implement GetPersistOptions().
      windowscodecs/metadatahandler: Implement GetStream().
      windowscodecs/metadata: Add registration information for the Gps reader.
      windowscodecs/metadata: Add registration information for the Exif reader.
      windowscodecs/metadata: Add initial implementation of the App1 reader.
      po: Update some Russian strings.
      windowscodecs: Move component info registry key cleanup to a common failure path.
      windowscodecs: Move an hkey handle to the component info base structure.
      windowscodecs: Add a stub for IWICMetadataWriterInfo.
      windowscodecs/metadata: Add registration information for the "Unknown" writer.
      windowscodecs/metadata: Add a stub for WICUnknownMetadataWriter.
      windowscodecs/metadata: Add a stub for WICGpsMetadataWriter.
      windowscodecs/metadata: Add a stub for WICExifMetadataWriter.
      windowscodecs/metadata: Add a stub for WICIfdMetadataWriter.
      windowscodecs: Fix a typo in metadata readers registration helper.
      windowscodecs: Remove redundant guid-to-string conversion when writing readers registration entries.
      winhttp: Check for the end of the text when stripping trailing newlines from headers (ASan).
      gdiplus/tests: Extend a GdipGetPathGradientBlend() test with excessive output buffer size.
      gdiplus: Use actual blend count for output copies in GdipGetPathGradientBlend() (ASan).

Paul Gofman (37):
      crypt32: Factor out CRYPT_RegDeleteFromReg().
      crypt32: Factor out CRYPT_SerializeContextToReg().
      crypt32: Don't output the whole chains from check_and_store_certs().
      crypt32: Do not use temporary store for updating root certificates.
      crypt32: Do not delete root certs which were not imported from host in sync_trusted_roots_from_known_locations().
      crypt32/tests: Add more tests for VerifyCertChainPolicy().
      crypt32: Fix some error codes in verify_ssl_policy().
      crypt32: Check CERT_TRUST_REVOCATION_STATUS_UNKNOWN instead of CERT_TRUST_IS_OFFLINE_REVOCATION in verify_ssl_policy().
      crypt32: Favour CERT_CHAIN_POLICY_IGNORE_END_REV_UNKNOWN_FLAG in verify_ssl_policy().
      crypt32: Only mind end certificate when checking revocation status in verify_ssl_policy().
      crypt32: Favour CERT_CHAIN_POLICY_IGNORE_WRONG_USAGE_FLAG in verify_ssl_policy().
      crypt32: Use correct tag for OCSP basic response extensions.
      crypt32: Use correct tag for OCSP single response extensions.
      cryptnet: Retry OCSP request with POST if GET failed.
      cryptnet: Do not perform OCSP requests with CERT_VERIFY_CACHE_ONLY_BASED_REVOCATION flag.
      ntdll: Add NtConvertBetweenAuxiliaryCounterAndPerformanceCounter() function.
      kernelbase: Add ConvertAuxiliaryCounterToPerformanceCounter() / ConvertPerformanceCounterToAuxiliaryCounter().
      wine.inf: Add Explorer\FileExts registry key.
      win32u: Generate mouse events in the server when releasing capture.
      server: Don't send WM_MOUSEMOVE for zero movement in queue_mouse_message().
      crypt32: Release cert context in CertDeleteCertificateFromStore().
      crypt32: Factor out memstore_free_context() function.
      crypt32: Don't try to release zero-refcount context in MemStore_addContext().
      crypt32: Release existing cert context in add_cert_to_store().
      crypt32: Only remove cert from mem store list when deleting it.
      crypt32/tests: Add a test for deleting and adding certs during enumeration.
      crypt32: Don't assert in Context_Release() on invalid refcount.
      win32u: Nullify surface hwnd when detaching Vulkan surface.
      win32u: Check for NULL hwnd before calling vulkan_surface_presented() driver callback.
      win32u: Don't invalidate existing Vulkan surface when a new one is created for window.
      winex11: Attach currently active Vulkan onscreen surface in vulkan_surface_update_offscreen().
      crypt32/tests: Avoid use after free in testEmptyStore().
      win32u: Implement NtUserGetCurrentInputMessageSource().
      winex11: Update window shape before putting surface image.
      winex11.drv: Pass visual to is_wxrformat_compatible_with_visual.
      winex11.drv: Choose alpha-enabled xrender format for argb drawables.
      ntdll: Zero terminate return string for NtQueryInformationProcess( ProcessImageFileName[Win32] ).

Piotr Caban (10):
      propsys: Add PropVariantGetStringElem implementation.
      ole32: Support all PROPVARIANT vector types in propertystorage_get_elemsize.
      ole32: Update read offset in propertystorage_read_scalar helper.
      ole32: Pass MemoryAllocator class to PropertyStorage_ReadProperty.
      ole32: Support more vector datatypes when reading property storage.
      ole32: Add support for reading VT_VECTOR|VT_VARIANT property.
      ole32: Add support for reading VT_R4 property.
      ole32: Fix IPropertyStorage::ReadMultiple return value when some properties are missing.
      ole32: Set property storage clsid on creation.
      msvcr120: Remove MSVCR120_ prefix from creal().

Piotr Morgwai Kotarbinski (2):
      wined3d: Add Nvidia RTX30xx series desktop models data.
      wined3d: Add Nvidia RTX40xx series desktop models data.

Rémi Bernon (5):
      winex11: Allow Withdrawn requests to override Iconic <-> Normal transitions.
      winex11.drv: Use get_win_data directly in X11DRV_GetDC.
      server: Introduce new set_thread_priority helper.
      ntdll: Set RLIMIT_NICE to its hard limit.
      server: Check wineserver privileges on init with -20 niceness.

Sebastian Scheibner (2):
      wineboot: Add dummy entry for SystemBiosDate.
      explorerframe: Return S_OK in more ITaskbarList3 functions.

Shaun Ren (6):
      sapi: Adding missing interfaces for SpStream.
      sapi: Implement ISpStream::Set/GetBaseStream.
      sapi: Implement ISpStream::Close.
      sapi: Implement ISpStream::GetFormat.
      sapi: Implement IStream methods for SpStream.
      sapi: Remove some unnecessary traces.

Stefan Dösinger (6):
      wined3d: Support WINED3DFMT_B5G5R5A1_UNORM in the Vulkan backend.
      wined3d: Use VK_FORMAT_R4G4B4A4_UNORM_PACK16 for WINED3DFMT_B4G4R4A4_UNORM.
      gdi32: Windows adds an extra 4 bytes to EMREXTCREATEPEN.
      gdi32: Set EMREXTCREATEPEN offBmi and offBits.
      gdi32: EMREXTCREATEPEN contains a 32 bit EXTLOGPEN.
      gdi32/tests: Add an EMREXTCREATEPEN test.

Stéphane Bacri (4):
      msvcr120: Fix _Cbuild signature.
      msvcr120: Add cimag() implementation.
      msvcr120: Add _FCbuild() implementation.
      msvcr120: Add crealf() and cimagf() implementation.

Tim Clem (1):
      imm32: Always validate the IME UI window when painting.

Vibhav Pant (6):
      bluetoothapis/tests: Add tests for BluetoothGetRadioInfo.
      bluetoothapis: Implement BluetoothGetRadioInfo.
      bluetoothapis: Add stubs for BluetoothIsConnectable, BluetoothIsDiscoverable.
      bluetoothapis/tests: Add tests for BluetoothIsConnectable, BluetoothIsDiscoverable.
      bluetoothapis: Implement BluetoothIsConnectable.
      bluetoothapis: Implement BluetoothIsDiscoverable.

William Horvath (2):
      ntdll/tests: Add tests for NtDelayExecution and Sleep(Ex).
      ntdll: Fix the return value of NtDelayExecution.

Yuxuan Shui (2):
      dmime: Handle IStream EOF correctly in MIDI parser.
      dmime: Connect default collection to MIDI bandtrack.

Zhiyi Zhang (32):
      d2d1/tests: Add ID2D1Device2_GetDxgiDevice() tests.
      d2d1: Remove an unnecessary cast in d2d_device_context_init().
      d2d1: Implement d2d_device_GetDxgiDevice().
      include: Add splay link tree helpers.
      ntdll: Implement RtlSubtreePredecessor().
      ntdll/tests: Add RtlSubtreePredecessor() tests.
      ntdll: Implement RtlSubtreeSuccessor().
      ntdll/tests: Add RtlSubtreeSuccessor() tests.
      ntdll: Implement RtlRealPredecessor().
      ntdll/tests: Add RtlRealPredecessor() tests.
      ntdll: Implement RtlRealSuccessor().
      ntdll/tests: Add RtlRealSuccessor() tests.
      win32u: Allocate a separate user buffer when packing a large WM_COPYDATA message for user32.
      user32/tests: Add tests for WM_COPYDATA.
      ntdll: Implement RtlSplay().
      ntdll/tests: Add RtlSplay() tests.
      ntdll: Implement RtlDeleteNoSplay().
      ntdll/tests: Add RtlDeleteNoSplay() tests.
      ntdll: Implement RtlDelete().
      ntdll/tests: Add RtlDelete() tests.
      light.msstyles: Use light blue as hot tracking color instead of grey.
      include: Fix PRTL_GENERIC_ALLOCATE_ROUTINE prototype.
      ntdll/tests: Add RtlInitializeGenericTable() tests.
      ntdll/tests: Add RtlNumberGenericTableElements() tests.
      ntdll: Implement RtlIsGenericTableEmpty().
      ntdll/tests: Add RtlIsGenericTableEmpty() tests.
      ntdll: Implement RtlInsertElementGenericTable().
      ntdll: Implement RtlDeleteElementGenericTable().
      ntdll/tests: Add RtlInsertElementGenericTable() tests.
      ntdll/tests: Add RtlDeleteElementGenericTable() tests.
      ntdll: Implement RtlLookupElementGenericTable().
      ntdll/tests: Add RtlLookupElementGenericTable() tests.

Zsolt Vadasz (1):
      ntdll: Add a character map name for Shift JIS.
```
