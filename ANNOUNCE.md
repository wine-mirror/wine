The Wine development release 11.4 is now available.

What's new in this release:
  - SAX reader reimplemented in MSXML.
  - Resampling optimizations in DirectSound.
  - Beginnings of a proper CFGMGR32 implementation.
  - Better Unix timezone matching.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/11.x/wine-11.4.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-11.4/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 11.4 (total 17):

 - #14713  Xara Xtreme 4: Resizing handles not visible
 - #16957  CreateProcess handles are inherited even when bInheritHandles=FALSE
 - #24851  Explorer++ treeview shows plus signs even when a folder contains no subfolders
 - #36686  CodeGear RAD Studio 2009 fails on startup (ISAXXMLReader::putFeature method needs to support 'normalize line breaks' feature)
 - #42024  FL Studio 12.4 installer, Janetter 4.1.1.0 crash on missing HKCU\Software\Microsoft\Windows\CurrentVersion\Internet Settings\ProxyEnable key
 - #42027  Xilinx Vivado 2014.4 installer fails: systeminfo is a stub
 - #49393  Gag game crash
 - #52330  iZotope software authorization process fails
 - #55739  Native Access 2: permanent loading circle after entering credentials
 - #57415  ROMCenter crash after creating database > IO Exception cannot locate resource images/defaultsystemicon.png (MONO 8.1 is the problem)
 - #58505  SSFpres search window appears behind main window since Wine 10.5
 - #59140  Bug avec le logiciel pronote pendant sa mise à jour
 - #59329  C++ catch() use-after-free with std::rethrow_exception()
 - #59342  Roblox Studio needs msvcp140.dll.?_Xregex_error@std@@YAXW4error_type@regex_constants@1@@Z
 - #59449  FormatMessageW fails for well-formed function call and crashes application
 - #59455  Disappearing TreeView items on TVM_INSERTITEM+TVM_EXPAND+TVM_EDITLABEL
 - #59462  wine-staging fails to build due to unable to find -lvkd3d

### Changes since 11.3:
```
Adam Markowski (2):
      po: Update Polish translation.
      po: Update Polish translation.

Akihiro Sagawa (4):
      winmm/tests: Add end position tests for avivideo.
      mciavi32: Fix handling of the end position in the play command.
      mciavi32: Fix handling of the end position in the seek command.
      mciavi32: Fix handling of the end position in the step command.

Alexandre Julliard (14):
      makedep: Only allow non-msvcrt sources to depend on config.h.
      makedep: Remove the is_external flag.
      makefiles: Specify external modules in the main makefile.
      configure: Don't bother setting MSVCRT flags for PE-only builds.
      include: Add some C++ floating point functions.
      include: Move math defines to a new corecrt_math_defines.h file.
      include: Define the locale variants of all the printf functions.
      makefiles: Simplify compiler-rt usage.
      winecrt0: Move to the libs directory.
      include: Add __popcnt intrinsics.
      include: Add nullptr_t definition for C++.
      include: Add a few C++ exception classes.
      include: Duplicate the contents of rtlsupportapi.h inside winnt.h.
      include: Avoid redefinition warnings for STATUS_ codes.

Alistair Leslie-Hughes (5):
      propsys: Add more System Device Properties.
      include: Add IDataTransferManagerInterop interface.
      include: Windows.ui.viewmanagement add IReference<UIElementType> interface.
      include: Add windows.ui.text.idl.
      include: Add windows.ui.text.core.idl.

Andrew Nguyen (1):
      winmm: Return MMSYSERR_INVALPARAM for NULL lpFormat in waveOutOpen.

Anton Baskanov (15):
      dsound: Remove the unused freqneeded field.
      dsound: Don't use double-precision arithmetic in the resampler.
      dsound: Do the subtraction before converting to float to improve rem precision.
      dsound: Use the modulus operator instead of divide-multiply-subtract.
      dsound: Multiply by dsbfirstep after calculating the modulus.
      dsound: Get rid of fir_copy.
      dsound: Resample one channel at a time.
      dsound: Resample into a temporary buffer.
      dsound: Remove asserts from the resampling loop.
      dsound: Factor out resampling.
      dsound: Split resample into separate downsample and upsample functions.
      dsound: Don't apply firgain in upsample.
      dsound: Don't pass dsbfirstep to upsample.
      dsound: Use a fixed upsampling loop boundary.
      dsound: Don't invert the remainder twice in upsample.

Aurimas Fišeras (2):
      po: Update Lithuanian translation.
      po: Update Lithuanian translation.

Bernhard Kölbl (7):
      mmdevapi/tests: Simplify a trace.
      mmdevapi/tests: Check device friendly name pattern.
      mmdevapi: Adjust device names.
      winmm/tests: Test digital-video window behavior for audio files.
      msvfw32/tests: Add a basic audio playback test.
      msvfw32: Only attempt to set a digital-video window, when the driver has a window itself.
      windows.media.speech: Implement IVectorView<VoiceInformation> on top of a stub provider.

Bernhard Übelacker (2):
      mmdevapi/tests: Use temporary during reallocation (ASan).
      winmm/tests: Reserve additional memory in test_formats (ASan).

Brendan McGrath (5):
      mf/tests: Fix potential race conditions in test.
      mf: Drop events and end draining on MFT flush.
      mf/tests: Test presentation clock with custom time source.
      mf: Don't async notify a sink that is also the time source.
      mf: Implement IMFTimer using a periodic callback.

Brendan Shanks (1):
      kernelbase: Fix GetNativeSystemInfo() in an x86_64 process on ARM64.

Daniel Lehman (1):
      dwrite: Add locales to locale list.

Dmitry Timoshkov (1):
      win32u: When looking for duplicates in the external fonts list don't count not external fonts.

Elizabeth Figura (3):
      Revert "kernel32/tests: Test whether EA size is returned from FindFirstFile().".
      kernelbase: Copy reserved fields in FindFirstFileExA().
      ntdll/tests: Accept STATUS_SUCCESS from NtQueryDirectoryFile().

Eric Pouech (8):
      windows.media.speech/tests: Add a couple of synthesizer's options tests.
      windows.media.speech: Add basic implementation on synthesizer options.
      windows.media.speech/tests: Add more tests about IVoiceInformation.
      windows.media.speech/tests: Add tests about vector view's content.
      windows.media.speech: Add basic implementation of IVoiceInformation.
      windows.media.speech/tests: Add more tests about default voice.
      windows.media.speech: Select a default voice in synthesizer.
      windows.media.speech: Implement get/put voice on synthesizer.

Gabriel Ivăncescu (1):
      winex11.drv: Do not set the pending NET_ACTIVE_WINDOW when we're skipping it.

Giovanni Mascellani (11):
      mmdevapi/tests: Introduce a helper to format wave formats.
      mmdevapi/tests: Introduce a helper to decide whether a wave format is valid.
      mmdevapi/tests: Test a curated list of wave formats.
      mmdevapi/tests: Tweak test logging a little bit.
      mmdevapi/tests: Test capturing with RATEADJUST in exclusive mode more specifically.
      winmm/tests: Test playing 24-bit wave formats.
      Revert "winmm/tests: Test a few PCM and float wave formats."
      winmm/tests: Test a curated list of wave formats.
      winmm: Write a null HWAVE when waveOutOpen() fails.
      winmm: Initialize the audio client with AUTOCONVERTPCM.
      winmm: Interpret S_FALSE from IsFormatSupported() as success.

Herman Semenoff (1):
      msvcrt: Add overflow check in _time32().

Huw D. M. Davies (1):
      user32/tests: Add mouse in pointer dragging test.

Jacek Caban (8):
      winecrt0: Add _tls_used support.
      compiler-rt: Import emutls.c.
      msvcrt: Add __ExceptionPtrSwap implementation.
      msvcp140: Add __ExceptionPtrSwap implementation.
      gitlab: Install mingw g++ packages.
      compiler-rt: Backport ARM64EC assembly.h changes.
      compiler-rt: Import aarch64 chkstk.S.
      makedep: Use compiler-rt when explicitly importing winecrt0.

Maotong Zhang (1):
      compiler-rt: Add __ctzdi2 to fix i386 MinGW builds.

Nello De Gregoris (1):
      ntoskrnl.exe: Add stub for KdChangeOption().

Nick Kalscheuer (1):
      wined3d: Add description for NVIDIA A10G GPU.

Nikolay Sivov (24):
      msxml3/writer: Return success from putDocumentLocator().
      msxml3/tests: Add a test for newlines handling in the writer.
      msxml3/tests: Use wide-char constants in more places.
      msxml3/tests: Add a test for newline handling when writing attribute values.
      msxml6/tests: Use wide-string literals when possible.
      msxml6/tests: Add some tests for newlines handling in the writer.
      msxml3/writer: Fix newline handling in the writer.
      kernelbase: Add a message string for CRYPT_E_NO_MATCH.
      msxml/tests: Separate reader tests in a number of helpers.
      msxml/tests: Add more tests for characters events.
      msxml3/saxreader: Remove libxml2 dependency in the parser.
      msxml3/saxreader: Add support for "max-xml-size" property.
      msxml/tests: Add some tests for the "normalize-line-breaks" feature.
      msxml4/tests: Add SAX tests.
      msxml: Better match newline escaping in the writer.
      msxml/tests: Extend PI test with newlines.
      msxml3/saxreader: Normalize line breaks in PI text.
      msxml3/saxreader: Normalize space characters in attribute values to 0x20.
      msxml/saxreader: Add support for "normalize-line-breaks" feature.
      msxml/tests: Add some feature setting tests for "exhaustive-errors".
      msxml3: Add SAX error messages.
      msxml/tests: Add some more tests for the ownerDocument() property.
      msxml3/dom: Fix ownerDocument() property for documents.
      msxml/tests: Add parentNode() property test for the document nodes.

Pan Hui (1):
      comdlg32: Add the implementation of IServiceProvider_fnQueryService to obtain IFolderView2.

Paul Gofman (17):
      opengl32: Don't access drawable after wgl_context_flush() in flush_context().
      win32u: Also try to use DC own drawable in get_updated_drawable() when context is flushed.
      msvcrt: Fix no buffering detection in _fwrite_nolock().
      win32u: Better handle flags for mouse in pointer messages.
      opengl32: Recognize more glHint attributes.
      ntdll/tests: Test which MxCsr from the context is used by SetThreadContext() on x64.
      ntdll: Prefer CONTEXT.MxCsr over CONTEXT.FltSave.MxCsr in NtSetContextThread() on x64.
      winhttp: Stub WinHttpSetOption(WINHTTP_OPTION_DECOMPRESSION).
      server: Create temp files for anonymous mappings with memfd_create() if available.
      ntdll: Get current timezone bias from user shared data.
      server: FIx timezone bias for timezones defined with inverse DST on Unix.
      ntdll: Fix get_timezone_info() for timezones defined with inverse DST on Unix.
      ntdll: Factor out read_reg_tz_info().
      ntdll: Try to determine system time zone to Windows zone by name.
      ntdll: Drop additional TZ matching with match_tz_name().
      ntdll: Better search for DST change in find_dst_change().
      ntdll: Accept 30min shift in match_tz_date().

Piotr Caban (8):
      msvcr100: Fix leak in __ExceptionPtrAssign.
      msvcp140: Don't validate callback in _Schedule_chore.
      mmdevapi: Don't fail initialization when no drivers were loaded.
      winmm: Call WINMM_InitMMDevices once on systems without sound devices.
      winmm: Fix endpoint notification callback leak in WINMM_InitMMDevices.
      msvcp140: Add _Xregex_error implementation.
      ntdll: Fix threadpool timer behavior on system time change.
      ntdll: Don't fix gsbase on execution fault exception.

Piotr Pawłowski (1):
      comctl32/treeview: Fixed wrong check for currently edited item.

Reyka Matthies (7):
      wininet/tests: Skip GetUrlCacheConfigInfo and CommitUrlCacheEntry tests if the functions return ERROR_CALL_NOT_IMPLEMENTED.
      wininet/tests: Add tests for seeking beyond end of file.
      wininet: Return correct errors and put handle into invalid state when seeking beyond end of file.
      include: Add MFNETSOURCE_STATISTICS_SERVICE definition.
      wininet: Factor out writing of cache file.
      wininet: Do not reset content_pos in reset_data_stream.
      wininet: Factor request creation out of HTTP_HttpSendRequestW.

Rémi Bernon (33):
      cfgmgr32: Implement CM_Get_Class_Property_Keys(_Ex).
      cfgmgr32: Implement CM_Get_Device_Interface_List(_Size)(_Ex)(A|W).
      cfgmgr32: Reimplement CM_Get_Device_Interface_Property(_Ex)W.
      cfgmgr32: Implement CM_Get_Device_Interface_Property_Keys(_Ex)W.
      include: Add vcruntime_exception.h.
      include: Add vcruntime_typeinfo.h.
      include: Add new.h and vcruntime_new.h.
      cfgmgr32/tests: Add results for missing permissions.
      cfgmgr32/tests: Skip some localized names tests.
      cfgmgr32/tests: Skip some HID devices tests if none are present.
      winemac: Call client_surface_present when OpenGL surface gets presented.
      cfgmgr32: Fix leak of device info set in DevGetObjectPropertiesEx.
      cfgmgr32: Move qsort out of dev_object_iface_get_props.
      cfgmgr32: Introduce helpers to collect object property keys.
      cfgmgr32: Allocate the property array to its maximum size.
      cfgmgr32: Keep properties in the order they were requested.
      cfgmgr32: Remove DEV_OBJECT parameter from internal callbacks.
      cfrmgr32: Introduce a new dev_get_device_interface_property helper.
      cfgmgr32: Initialize device_interface structs in DEV_OBJECT functions.
      cfgmgr32: Return BOOL from devprop_filter_matches_properties.
      cfgmgr32: Use the new helpers to get device interface property keys.
      cfgmgr32: Use the new helpers to read device interface properties.
      cfgmgr32: Return LSTATUS from enum_dev_objects callback.
      cfgmgr32: Enumerate the registry keys directly in enum_dev_objects.
      setupapi: Cleanup SetupDiOpenClassRegKey(Ex)(A|W) code style.
      setupapi: Reimplement SetupDiOpenClassRegKeyExW using cfgmgr32.
      setupapi: Cleanup SetupDiClassGuidsFromName(Ex)(A|W) code style.
      setupapi: Reimplement SetupDiClassGuidsFromNameExW using cfgmgr32.
      setupapi: Cleanup SetupDiBuildClassInfoList(Ex)(A|W) code style.
      setupapi: Reimplement SetupDiBuildClassInfoListExW using cfgmgr32.
      winebus.sys: Prefer hidraw for all VKB (VID 231d) devices.
      winex11: Introduce a new window_set_net_wm_fullscreen_monitors helper.
      winex11: Continue requesting fullscreen monitors in PropertyNotify handlers.

Sreehari Anil (1):
      bcrypt: Improve RSA OAEP parameter handling.

Sven Baars (1):
      dsound: Fir.h is no longer a generated file.

Tatsuyuki Ishi (4):
      crypt32: Support importing cert-only PKCS blobs in PFXImportCertStore.
      crypt32/tests: Add tests for importing cert-only PKCS#12 blobs.
      crypt32: Verify MAC before parsing the store in PFXImportCertStore.
      shell32: Only compute HASSUBFOLDER if requested.

Twaik Yont (5):
      ntdll: Use __builtin_ffs instead of ffs in ldt_alloc_entry.
      wineandroid: Force --rosegment for wine-preloader on Android (API < 29, NDK r22+).
      wineandroid: Fix desktop init ordering after CreateDesktopW changes.
      wineandroid: Sanitize dequeueBuffer result and reject missing buffers.
      ntdll: Call virtual_init from JNI_OnLoad to avoid early virtual_alloc_first_teb failure.

Vibhav Pant (9):
      windows.devices.bluetooth/tests: Fix test failures on w1064v1507 and w1064v1809.
      winebth.sys: Create new GATT service entries on receiving InterfacesAdded for org.bluez.GattService1 objects from BlueZ.
      winebth.sys: Create new GATT characteristic entries on receiving InterfacesAdded for org.bluez.GattCharacteristic1 objects from BlueZ.
      winebth.sys: Create PDOs for GATT services existing on remote devices.
      winebth.sys: Implement IOCTL_WINEBTH_LE_DEVICE_GET_GATT_CHARACTERISTICS for GATT service PDOs.
      bluetoothapis/tests: Add tests for calling BluetoothGATTGetCharacteristics with a GATT service handle.
      winebth.sys: Add debug helper for DBusMessageIter values.
      bluetoothapis: Implement BluetoothGATTGetCharacteristics for GATT service handle objects.
      winebth.sys: Ignore incoming method calls that are not from BlueZ.

Yuxuan Shui (1):
      winegstreamer: Fix buffer info when returning S_FALSE from ProcessOutput.

Zhiyi Zhang (10):
      win32u: Disable keyboard cues by default.
      user32/tests: Test SPI_{GET,SET}KEYBOARDCUES.
      d2d1/tests: Test brushes with out of range opacity values.
      d2d1: Clamp opacity value for brushes.
      windows.ui/tests: Add tests for IUISettings_get_AnimationsEnabled().
      windows.ui: Implement uisettings_get_AnimationsEnabled().
      dwmapi: Add DwmShowContact() stub.
      include: Add windows.graphics.display.idl.
      windows.graphics: Add stub dll.
      windows.graphics: Add IDisplayInformationStatics stub.

Ziqing Hui (3):
      mf/tests: Add more presentation clock tests for media sink.
      winegstreamer: Implement media_sink_SetPresentationClock.
      winegstreamer: Implement media_sink_GetPresentationClock.
```
