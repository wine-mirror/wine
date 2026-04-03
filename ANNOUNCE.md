The Wine development release 11.6 is now available.

What's new in this release:
  - Beginnings of a revival of the Android driver.
  - DLL load order heuristics to better support game mods.
  - More VBScript compatibility fixes.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/11.x/wine-11.6.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-11.6/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 11.6 (total 28):

 - #7961   Bug in findfirst/findnext
 - #26389  Win3.1 Notepad crashes when opening a large file
 - #38874  Homesite+ (v5.5): Radiobutton in Find dialog not set properly when using keyboard shortcut (Alt+U)
 - #42517  StarOffice51 Crash on File open
 - #45289  Wine Crashes Some Programs If Compiled with AVX Support
 - #49776  .NET application settings are never saved
 - #51577  Google Earth Installer renders mostly black
 - #52019  DigiCertUtil wrong window size
 - #52741  Missing chakra.dll to run Minecraft Windows 10 Edition
 - #55933  NSFW game 'Creature reaction inside the ship! 1.5' videos stop after looping a few times
 - #56296  PDFSam Installer shows an empty language list
 - #58043  Multiple applications won't run on Wine 10.3 because they think that there is a debugger running when there isn't (DVDFab, DCS World updater)
 - #58146  HWiNFO 8.24 (latest): doesn't work unless winecfg is set to Windows 7
 - #58690  fw_manager_IsPortAllowed is not implemented
 - #59486  Treeview TVS_CHECKBOXES cycle all state images with non-zero index
 - #59498  pdf-xchange editor v10.8.4.409 crashes at start.
 - #59536  Missing WINHTTP_OPTION_SERVER_CERT_CHAIN_CONTEXT
 - #59546  Commit 20b34866 breaks the list of on-line multiplayer servers for the game Mount & Blade: Warband
 - #59548  Neko Project emulator no longer works with 96 kHz sampling rate (Gentoo Linux)
 - #59551  Buhl Tax 2026 shows blank preview - crash on geometry creation
 - #59553  ICU dlls cause some games to crash (Cyberpunk 2077)
 - #59554  DOAXVV (DMM version) fails to start with network error
 - #59559  Missing D3D11 Interop Functions needed for Gecko based browsers
 - #59561  VICE: GTK version of x64sc.exe crashes at startup.
 - #59563  printf does not support %Z
 - #59573  Regression introduced by decompression support in winhttp
 - #59576  EIZO ColorNavigator 6 crashes on unimplemented function mscms.dll.WcsGetCalibrationManagementState
 - #59600  Regression on 11.5 - Diablo IV in game-menu shop no more reachable

### Changes since 11.5:
```
Adam Markowski (3):
      po: Update Polish translation.
      po: Update Polish translation.
      po: Update Polish translation.

Akihiro Sagawa (5):
      winegstreamer: Flush wg_transform buffers on seek in quartz_transform.c.
      quartz/tests: Add more video format tests for MPEG-1 Splitter.
      winegstreamer: Pass MPEG-1 video codec data to the GStreamer decoder.
      winegstreamer: Increase the buffer count in the MPEG video output.
      quartz/tests: Remove todo_wine from tests that succeed now.

Alexandre Julliard (27):
      configure: Warn when the C++ compiler is missing or not working.
      gdi32: Also redirect path in RemoveFontResourceEx.
      server: Add a helper function to trace a Unicode string.
      server: Share more code between 32- and 64-bit PE data directory handling.
      ntdll: Add a structure to return information about a PE mapping.
      server: Return version resource in get_mapping_info for native dlls.
      ntdll: Add heuristics to prefer native dll based on the version resource.
      wtsapi32/tests: Skip processes that no longer exist.
      ldap: Correctly grow the buffer on incomplete message.
      configure: Require a PE compiler on 64-bit macOS.
      configure: Stop using libunwind.
      configure: Check for gradle on Android.
      configure: Pass ANDROID_HOME when building the Android APK.
      ntdll: Fix a printf format warning.
      ntoskrnl: Update the security cookie once relocation are applied.
      kernelbase: Update timezone data to version 2026a.
      zlib: Import upstream release 1.3.2.
      png: Import upstream release 1.6.56.
      lcms2: Import upstream release 2.18.
      ntdll: Move the prefix bootstrap check out of is_builtin_path().
      ntdll: Pass the full PE mapping info to get_load_order().
      ntdll: Also consider wow64 system directory for module load order.
      ntdll: Only set the version resource child when found in version_find_key.
      winecrt0: Add LGPL license exception.
      msvcrt: Add LGPL license exception.
      vcruntime140: Add LGPL license exception.
      libs: Add LGPL license exception to the uuid libraries.

Alistair Leslie-Hughes (3):
      winhttp: Support Query option WINHTTP_OPTION_SECURITY_INFO.
      ntdll: Correct last parameter of NtFlushVirtualMemory.
      comctl32/imagelist: Allow for larger initial image count.

Anton Baskanov (1):
      dsound: Make i signed in downsample.

Aurimas Fišeras (1):
      po: Update Lithuanian translation.

Bartosz Kosiorek (5):
      gdiplus: Fix GdipGetImageBounds result in case Graphics is deleted.
      gdiplus: Return InvalidParameter in GdipGetImageHorizontalResolution in case HEMF is missing.
      gdiplus: Return InvalidParameter in GdipGetImageVerticalResolution in case HEMF is missing.
      gdiplus: Return InvalidParameter in GdipGetImageWidth in case HEMF is missing.
      gdiplus: Return InvalidParameter in GdipGetImageHeight in case HEMF is missing.

Benoît Legat (2):
      crypt32: Implement CERT_NCRYPT_KEY_HANDLE_PROP_ID support.
      crypt32: Implement PFXExportCertStoreEx support.

Bernhard Übelacker (2):
      mmdevapi: Avoid copying more memory than reserved in validate_fmt (ASan).
      d2d1: Allow value NULL in d2d_effect_SetInput.

Brendan McGrath (7):
      wmvcore: Test we can open a rawvideo wmv file.
      winegstreamer: Place 'videoconvert' prior to 'deinterlace' element.
      iyuv_32: Add stub dll.
      iyuv_32/tests: Add test stub.
      iyuv_32/tests: Test which formats are supported.
      iyuv_32/tests: Add a decompression test.
      iyuv_32/tests: Add a compression test.

Brendan Shanks (4):
      ntdll/tests: In wow64, factor the SYSTEM_INFORMATION_CLASS out of test_query_architectures().
      ntdll/tests: Add tests for NtQuerySystemInformation(SystemSupportedProcessorArchitectures2).
      ntdll: Implement NtQuerySystemInformation(SystemSupportedProcessorArchitectures2).
      server: Add x86_64 as a supported machine on ARM64.

Chris Kadar (1):
      cabinet: Add stubs for compressor/decompressor.

Connor McAdams (8):
      dinput/tests: Fix occasional GameInput test crash.
      dinput/tests: Fix incorrect index when removing child device.
      include: Fix HID_STRING_ID_* definitions.
      dinput/tests: Add tests for HID device indexed strings.
      dinput/tests: Check child device serial, if provided, on remove.
      dinput/tests: Cleanup every device key with winetest vendor.
      dinput/tests: Test that guidInstance is stable across hotplugs.
      dinput/tests: Add tests for dinput joystick guidInstance values.

Conor McCarthy (6):
      mf: Trace additional topology details.
      mf/topology_loader: Do not create a new branch list containing downstreamm nodes.
      mf/topology_loader: Delete unused struct connect_context.
      mf/topology_loader: Load the branch up connection method only for source nodes.
      mf: Specify the multithreaded queue for presentation clock callbacks.
      mf: Specify the multithreaded queue for sample grabber timer callbacks.

Dmitry Timoshkov (6):
      advapi32/tests: Add some tests for LockServiceDatabase().
      services: Add proper handle management to LockServiceDatabase() and UnlockServiceDatabase().
      advapi32: Implement LockServiceDatabase() and UnlockServiceDatabase().
      winedump: Fix building with PSDK compiler.
      crypt32/tests: Add some tests for CertOpenSystemStore() with real provider handle.
      crypt32: Add CERT_STORE_NO_CRYPT_RELEASE_FLAG to CertOpenStore() flags when called from CertOpenSystemStore().

Elizabeth Figura (7):
      wined3d/vk: Handle stretching and format conversion when resolving as well.
      wined3d: Do not force the source into RB_RESOLVED for a non-colour blit.
      wined3d: Separate a raw_blitter_supported() helper.
      wined3d: Require matching sample counts for the raw blitter.
      wined3d: Move RB_RESOLVED logic from texture2d_blt() to texture2d_blt_fbo().
      wined3d: Explicitly handle raw resolve in the FBO blitter.
      wined3d: Explicitly pass the RAW flag for d3d10+ resolve.

Eric Pouech (6):
      mfplat: Add stub for MFSerializeAttributesToStream.
      mf: Add stub for MFCreateDeviceSource().
      include: Add missing extern "C" wrappers.
      include: Don't use C++ keywords.
      wevtapi: Add stub for EvtRender.
      advapi32: Add stub for AddConditionalAce().

Francis De Brabandere (24):
      vbscript: Implement IActiveScriptError::GetSourceLineText.
      vbscript: Invoke default property when calling object variable with empty parens.
      vbscript: Pass VT_DISPATCH as-is to Property Let without extracting default value.
      wscript: Implement IActiveScriptSite::OnScriptError.
      vbscript: Allow hex literals with excess leading zeros.
      vbscript: Fix dot member access after line continuation.
      vbscript: Add error code tests for compile errors.
      vbscript: Improve some error codes in the compiler.
      vbscript: Improve some error codes in the lexer.
      vbscript: Improve some error codes in the parser.
      vbscript: Implement GetRef.
      vbscript: Fix crash in ReDim on uninitialized dynamic arrays.
      vbscript: Return "Object required" error (424) instead of E_FAIL/E_NOTIMPL.
      vbscript: Return correct error for array dimension mismatch and NULL array access.
      vbscript: Return type mismatch when calling non-dispatch variable as statement.
      vbscript: Fix chained array indexing on call expression results.
      vbscript: Allow Not operator as operand of comparison expressions.
      vbscript: Implement AscW and ChrW functions.
      vbscript: Report proper errors for invalid date literals and line continuations.
      vbscript/tests: Enable Empty Mod test that now passes.
      vbscript: Implement Escape and Unescape functions.
      vbscript: Implement the Erase statement.
      vbscript: Implement Eval, Execute, and ExecuteGlobal.
      vbscript: Return error 92 for uninitialized For loops.

Georg Lehmann (1):
      winevulkan: Update to VK spec version 1.4.347.

Gijs Vermeulen (1):
      mscms: Add WcsGetCalibrationManagementState stub.

Hans Leidekker (16):
      winhttp: Fix querying available data for chunked transfers.
      winhttp: Trace more option values.
      winhttp: Accept WINHTTP_OPTION_ENABLE_HTTP_PROTOCOL on session handles.
      include: Add missing WinHTTP defines.
      winhttp: Print a fixme for unsupported flags in WinHttpOpen() and WinHttpOpenRequest().
      winhttp: Introduce struct read_buffer.
      winhttp: Always buffer response data.
      winhttp: Fix a typo.
      winhttp/tests: Get rid of workarounds for old Windows versions.
      winhttp: Ignore unexpected content encodings.
      winhttp: Don't trace content read / content length in read_data().
      winhttp: Move web socket support to a separate file.
      winhttp: Use public define for the web socket handle type.
      winhttp: Move pending_sends and pending_receives to the socket structure.
      crypt32: Also accept CERT_CHAIN_ENGINE_CONFIG without dwExclusiveFlags.
      bcrypt: Handle SHA512 in key_asymmetric_verify().

Jacek Caban (1):
      configure: Build PE modules with build ID by default.

José Luis Jiménez López (1):
      quartz/tests/avisplit: Add tests for IMediaSeeking time formats.

June Wilde (1):
      include: Add windows.graphics.directx.direct3d11.interop.idl file.

Kun Yang (2):
      taskschd/tests: Add tests for ILogonTrigger.
      taskschd: Add implementation of ILogonTrigger.

Louis Lenders (1):
      magnification: Add stub for MagInitialize.

Maotong Zhang (3):
      msi: Add ComboBox items, using value if text does not exist.
      msi: The ComboBox defaults to displaying the value.
      msi: ComboBox displays value on selection and falls back to text if missing.

Mohamad Al-Jaf (4):
      windows.devices.radios: Add stub dll.
      windows.devices.radios: Add IRadioStatics stub.
      windows.devices.radios: Implement IRadioStatics::GetRadiosAsync().
      windows.devices.radios/tests: Add IRadioStatics::GetRadiosAsync() tests.

Nikolay Sivov (24):
      msxml3/tests: Add some more DOM tests.
      msxml3/tests: Enable namespace:: axis test on Windows.
      msxml3/tests: Remove alternative variants of saved output as acceptable test results.
      msxml3/tests: Merge a test with existing get_xml() tests.
      msxml3/tests: Add a test for immediate document children list.
      d2d1/geometry: Check for null user transform when streaming.
      msxml3/tests: Add ISupportErrorInfo tests for DOCTYPE nodes.
      msxml3/tests: Add basic argument test for loadXML().
      msxml3/tests: Add parentNode() test for attributes.
      msxml3/tests: Add some cloneNode() tests for a document.
      msxml3/tests: Use string comparison in more places.
      msxml3/tests: Add another get_xml() test for namespace definitions.
      msxml3/tests: Add some more appendChild() tests.
      msxml3/tests: Remove some value testing macros.
      msxml3/tests: Skip removeAttributeNode() tests on Wine.
      msxml3/tests: Add another test for loadXML(NULL).
      msxml3/tests: Move CDATA node tests to its own function.
      msxml3/tests: Simplify setting string Variant values.
      secur32/tests: Test required buffer size in case of SEC_E_INCOMPLETE_MESSAGE.
      secur32/schannel: Fix returned required buffer size for incomplete messages in DecryptMessage().
      secur32/tests: Use existing macro for a capability flag.
      msxml3/tests: Add a test for reloading document with outstanding references.
      odbc32: Map SQLColAttributes(SQL_COLUMN_UNSIGNED) to SQLColAttribute().
      include: Fix SQLDriverConnectW() prototype.

Olivia Ryan (11):
      windows.web: Cleanup struct json_value.
      windows.web: Use json_buffer struct & helper functions.
      windows.web: Support escaped characters.
      windows.web: Additional string parsing tests.
      windows.web: Stub JsonArray runtime class.
      windows.web: Support json array parsing.
      windows.web: Add a IMap_HSTRING_IInspectable to json objects.
      windows.web: Support json object parsing.
      windows.web: Implement JsonArray element getters.
      windows.web: Support boolean & number value creation.
      windows.web: Implement JsonObject typed getters.

Paul Gofman (6):
      dwrite: Fix second chance fonts handling in fallback_map_characters().
      dwrite: Move opentype_get_font_table() up.
      dwrite: Fail loading unsupported COLRv1 fonts.
      netprofm: Check adapter addresses instead of gateway to guess ipv6 internet connectivity.
      ntdll: Do not keep next handler pointer outside of lock in call_vectored_handlers().
      server: Do not return STATUS_ACCESS_DENIED from read_process_memory_vm() for zero read.

Piotr Pawłowski (2):
      comctl32/tests: Test cycling all state images from custom image list when toggling checkboxes.
      comctl32/treeview: Cycle all state images from custom image list when toggling checkboxes.

Reece Dunham (1):
      mshtml: Doc typo fix.

Rémi Bernon (20):
      windows.web: Rename JsonObject class methods.
      windows.web: Skip whitespace in json_buffer_take.
      winex11: Update queue input time on WM_TAKE_FOCUS.
      mf: Introduce a new topology_node_get_type helper.
      winemac: Keep current user locale in the generated HKLs.
      winewayland: Activate an IME HKL before sending IME updates.
      winex11: Activate an IME HKL before sending IME updates.
      imm32: Only notify the host IME for known IME HKLs.
      include: Define _MAX_ENV in stdlib.h.
      joy.cpl: Ignore unsupported DIPROP_AUTOCENTER.
      dinput/tests: Move TLC count to the descriptor.
      dinput/tests: Support starting multiple devices.
      dinput/tests: Move fill_context call into hid_device_start.
      cfgmgr32: Implement CM_Locate_DevNode(_Ex)(A|W) and CM_Get_Device_ID(_Size)(_Ex)(A|W).
      cfgmgr32: Implement CM_Open_DevNode_Key(_Ex).
      setupapi: Forward CM_Open_DevNode_Key(_Ex) to cfgmgr32.
      cfgmgr32: Implement CM_Get_DevNode_Registry_Property(_Ex)(A|W).
      setupapi: Forward CM_Get_DevNode_Registry_Property(_Ex)(A|W) to cfgmgr32.
      winevulkan: Improve support for some recursive struct conversion.
      winevulkan: Ignore VkPhysicalDeviceVulkan14Properties returnedonly.

Stefan Dösinger (4):
      ddraw/tests: Cap test_map_synchronisation to 64k vertices.
      ddraw/tests: Fix nested designated initializers in test_caps().
      d3d9/tests: Improve yuv_color_test() on Nvidia drivers.
      ddraw/tests: Try harder to hide the window in test_window_style().

Theodoros Chatzigiannakis (3):
      msi: Handle padding bytes in table streams.
      msi: Detect mismatched bytes_per_strref in table data.
      msi/tests: Add regression tests for table stream padding and mismatched string refs.

Trung Nguyen (2):
      msvcrt: Add support for %Z printf specifier.
      msvcrt/tests: Add tests for %Z printf specifier.

Twaik Yont (5):
      wineandroid: Update Android project for modern Gradle plugin.
      wineandroid: Use BOOL for is_desktop flag.
      wineandroid: Fix APK output path for recent Gradle.
      ntdll: Disable userfaultfd support on Android.
      wineandroid: Switch to AHardwareBuffer and raise minSdkVersion to 26.

Vibhav Pant (10):
      winebth.sys: Store GATT characteristic value cached by the Unix Bluetooth service on the PE driver.
      winebth.sys: Update the cached GATT characteristic value on receiving a PropertiesChanged signal for a GATT characteristic value from BlueZ.
      winebth.sys: Implement IOCTL_WINEBTH_GATT_SERVICE_READ_CHARACTERISITIC_VALUE.
      bluetoothapis: Implement BluetoothGATTGetCharacteristicValue.
      bluetoothapis/tests: Add tests for BluetoothGATTGetCharacteristicValue.
      winebth.sys: Remove unneeded call to IoInvalidateDeviceRelations on radio removal.
      winebth.sys: Don't add Bluetooth devices/entries from Unix if they already exist.
      winebth.sys: Only make calls to BlueZ if it is available.
      winebth.sys: Initialize and remove devices on BlueZ startup and shutdown respectively.
      mountmgr.sys: Initialize threading support for DBus before calling DBus functions.

Wolfgang Hartl (1):
      oleaut32: Handle reference tables when processing SLTG records.

Yongjie Yao (2):
      user32/tests: Test the thread safety of NtUserRegisterClassExWOW and NtUserUnregisterClass.
      win32u: Fixed thread safety issues with NtUserRegistClassExWOW and NtUserUnregisterClass.

Yuxuan Shui (4):
      crypt32: Don't access CERT_CHAIN_ENGINE_CONFIG::dwExclusiveFlags without checking size.
      mfsrcsnk: Test how pixel aspect ratio metadata in an mp4 file are handled.
      winedmo: Call avformat_find_stream_info for H264 streams missing aspect ratio.
      ntoskrnl.exe/tests: Fix stack use-after-free of OVERLAPPED.

Zhao Yi (2):
      win32u: Fix deadlock when setting hdc_src to the window's self DC.
      win32u/tests: Add tests for hdc_src is window's self dc when calling UpdateLayeredWindow.

Zhiyi Zhang (15):
      ninput: Add ordinal 2502.
      include: Add some ALPC definitions.
      ntdll/tests: Add AlpcGetHeaderSize() tests.
      ntdll: Implement AlpcGetHeaderSize().
      ntdll/tests: Add AlpcInitializeMessageAttribute() tests.
      ntdll: Implement AlpcInitializeMessageAttribute().
      ntdll/tests: Add AlpcGetMessageAttribute() tests.
      ntdll: Implement AlpcGetMessageAttribute().
      iertutil: Move CreateIUriBuilder() from urlmon.
      iertutil: Move CreateUri() from urlmon.
      iertutil: Move CreateUriWithFragment() from urlmon.
      iertutil: Add PrivateCoInternetCombineIUri().
      iertutil: Add PrivateCoInternetParseIUri().
      urlmon: Add a wine_get_canonicalized_uri() helper.
      iertutil: Move CUri coclass from urlmon.
```
