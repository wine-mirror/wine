The Wine development release 11.21 is now available.

What's new in this release:
  - Better WoW64 OpenGL performance with `map_buffer_client_pointer` extension.
  - Initial version of the Voice Capture DSP library.
  - Improved support for user accounts.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/11.x/wine-11.21.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-11.21/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 11.21 (total 20):

 - #23766  (Shift) Ctrl + Tab isn't handled
 - #30969  Tron 2.0 - No music while playing game and in menu
 - #31283  I Love Science! cannot detect its CD
 - #59821  World of Tanks: Certain keys are not registered in chat (Enter, Delete...)
 - #59946  Skyrim (2011): crashes due to "rawinput... unhandled flags 0x230" with >=11.11
 - #60273  crypt32: signature verification re-sorts PKCS#7 authenticated attributes, breaking valid signatures whose SignedAttrs are not in DER order (FL Studio 26.1.5 fails to start)
 - #60283  Fate/stay night REMASTERED only shows a black screen when using EGL
 - #60314  Cinebench 2026 crashes on unimplemented function d3d12.dll.D3D12GetInterface
 - #60433  crypt32: SSL chain policy rejects certificates with an iPAddress subject alternative name
 - #60467  BioShock 2 Remastered crashes when applying display settings: NULL output in D3D11 FinishCommandList
 - #60495  NULL pointer crash in COMBOEX_NotifyEndEdit() when a ComboBoxEx item has empty/callback text
 - #60528  Wine 11.20 compiles fine but gives segmentation fault
 - #60534  Skyrim SE: Assertion `view->protect & VPROT_SYSTEM' failed.
 - #60545  Zeus: Master of Olympus reports a system-thread access violation on exit
 - #60553  .NET Framework 4.8 : Rounding error when casting a nullable float variable (0.001) to a decimal
 - #60562  winhttp silently truncates response header lines longer than 1459 bytes (breaks pre-signed S3 redirects)
 - #60575  libs/jxr: memory bugs on damaged JPEG XR files, fixed in mircomir/jxrlib 1.4.4
 - #60580  No One Lives Forever 2 has no music (64-byte wave-item headers rejected)
 - #60593  ELF builds fail after "makefiles: Use -noentry instead of data-only modules for DLLs that should be ARM64X."
 - #60601  i386 SEH unwinding broken by "ntdll: Allocate the initial thread data with mmap."

### Changes since 11.20:
```
Abdullah Ahmed (2):
      user32/tests: Test that DialogBoxParam returns a 64-bit value.
      user32: Avoid 64-bit value truncation of dialog return values.

Aleksei Arsenev (2):
      wbemprox: Use TRACE instead of FIXME in client_security AddRef/Release.
      comdlg32: Remove noisy FIXME in IServiceProvider QueryInterface/AddRef/Release.

Alex Henrie (3):
      dplayx: Pass bAnsi=FALSE from IDirectPlay4Impl_GetSessionDesc.
      win32u: Don't return a void expression from void loaderdrv functions.
      mmdevapi: Fix error check in MMDevCol_Create.

Alexandre Julliard (23):
      ntdll: Always use MAP_FIXED_NOREPLACE on Linux.
      ntdll: Don't bother to free thread data at process exit.
      server: Don't leave space between dll mappings.
      winebuild: Error out on thiscall in non-i386 build.
      jxr: Import upstream release 1.4.4.
      winebuild: Generate the CHPE metadata structure.
      winegcc: Add necessary variables for ARM64EC link checks.
      makedep: Replace fake dll by real module for noentry dlls.
      msvcrt: Remove non-i386 entry point in dlls that are only built for i386.
      ntdll: Make sure that the address still fits in the reserved area after alignment.
      ntdll: Allocate the initial thread data with mmap.
      ntdll: Reserve the 64-bit top-down area after the main exe is loaded.
      msvcrt: Remove architecture flags in dlls that are only built for i386.
      ntdll: Add a helper to allocate an aligned block with mmap.
      ntdll: Only set WINEPRELOADRESERVE for 32-bit binaries.
      wow64: Avoid crashing on optional parameters in NtCreateToken.
      kernelbase: Directly handle process attributes in create_nt_process().
      include: Add some mitigation policy definitions.
      include: Fix IMAGE_DOS_HEADER definition.
      dmime: Use the correct COM macros for the interface type.
      dmloader: Use the correct COM macros for the interface type.
      dmusic: Use the correct COM macros for the interface type.
      dmsynth/tests: Use the correct COM macros for the interface type.

Ali Milhim (4):
      d3d12: Add a D3D12GetInterface() stub.
      wined3d: Reset the Vulkan swapchain handles after destroying them.
      d3d9/tests: Test presenting after the device window is destroyed.
      d3d11: Return E_UNEXPECTED from FinishCommandList() with a NULL command list.

Alistair Leslie-Hughes (37):
      winhttp: Query_option correct sizeof usage.
      include: Correct PMEMORY_RESERVE_OBJECT_TYPE type.
      include: Add RegSetKeyValue define.
      include: Correct GetAddrInfoExW define.
      include: Correct HID_USAGE_LED_GENERIC_INDICATOR value.
      include: Correct CALG_RC5 define.
      include: Correct HANDLE_WM_DEVICECHANGE define.
      include: Add missing GetCurrentProcessId to IDebugSystemObjects.
      include: Correct IDirectMusicScript_ defines.
      include: Correct IDXCoreAdapterList_GetAdapter define.
      include: Add IsValidDevmode define.
      mfplat: Fake success in IMFPluginControl::SetDisabled.
      include: Fix attributes for IMFByteStream interface.
      include: Add missing unique attribute.
      kernelbase: Improve support for null parameters in SHRegGetUSValueW.
      rpcrt4: Correct CHAR types in NdrRangeUnmarshall.
      include: Fix PSYSTEM_ALARM_OBJECT_ACE spelling.
      include: Correct LPXACT_NOTIFICATION_WAVE spelling.
      include: Remove duplicate function prototypes.
      include: Add IRegisterServiceProvider interface.
      wmp: Avoid casting of COM pointers.
      wmp: AddRef returned object in IOleObject::GetClientSite.
      wmp: Support IUnknown in IWMPNetwork::QueryInterface.
      d3d11: Fake success in ID3D10Device1::SetExceptionMode.
      comsvcs: Correct checking a hex value in guid_from_string.
      comsvcs: AddRef for the driver reference in create_holder.
      infosoft: Correct reference count for IWordBreaker object.
      winex11/wintab: Remove duplicate assignment.
      wintab32: UnregisterClass with the instance it was created with.
      winex11/wintab: Correct copying the BTNAMES value.
      wininet: Correct sizeof usage in HttpSendRequestExA.
      wininet: Store sync time in correct field.
      advapi32: Correct debugstr_sid when SID has a count of 6.
      dplayx: Call correct interface in IDirectPlayLobby3::Connect.
      hhctrl.ocx: Correct pszIndex assignment in wintypeWtoA.
      mfplat: Correct X1R5G5B5 debug string.
      infosoft: Don't return early in QueryInterface.

Anton Baskanov (5):
      dmusic/tests: Test loading a DLS with a compressed wave.
      dmusic: Support compressed waves.
      dmusic: Free the old format in parse_wave_chunk.
      dmusic: Allocate enough space for WAVEFORMATEX in parse_wave_chunk.
      dmusic: Check the fmt chunk size in parse_wave_chunk.

Bernhard Kölbl (4):
      mf/tests: Test which source streams the session selects.
      mfsrcsnk/tests: Check if selected streams are persistent.
      winegstreamer: Don't select streams which are inactive.
      mf: Deselect source streams not covered by the topology in the session.

Bernhard Übelacker (2):
      comctl32/tests: Add test for LM_GETIDEALSIZE for syslink with empty text.
      comctl32_v6: Don't skip BreakChar in SYSLINK_Render.

Brendan Shanks (2):
      ntdll: Remove pre-macOS 10.15 workarounds.
      ntdll: Remove macOS i386-specific code.

Daniel Lehman (2):
      msxml3/tests: Add tests for cloneNode with namespace.
      msxml3: Pass qualified name to helper when cloning node.

David Schornsheim (2):
      user32/tests: Add tests for invalidated menu bar via WM_INITMENU.
      user32/tests: Add test for misplaced popup menu.

Derek Lesho (1):
      opengl32: Implement wow64 buffer mapping on MESA_map_buffer_client_pointer.

Elizabeth Figura (1):
      ntdll: Increase the threadpool worker timeout to 60 seconds.

Eric Pouech (4):
      cmd: Use word_iterator for GOTO builtin command.
      cmd: Clean-up DIR builtin command by removing useless code.
      cmd: Use word iterator for DIR builtin command.
      cmd: Fix UAF in HELP builtin command.

Fabien Romano (2):
      winebus: Preserve signal handlers when initializing SDL.
      dwmapi/tests: Handle missing display refresh rate information.

Gabriel Ivăncescu (3):
      jscript: Pass thread_data directly to gc_run.
      mshtml: Make Collection Enumerators part of the Cycle Collection graph.
      jscript: Traverse the enumerator in gc_traverse.

Hans Leidekker (15):
      bcrypt: Add support for DES/3DES encryption.
      wbemprox: Implement more Win32_Service properties.
      wbemprox: Implement Win32_Service.ChangeStartMode().
      ncrypt: Support NCRYPT_NO_PADDING_FLAG and NCRYPT_PAD_OAEP_FLAG in NCryptEncrypt().
      ncrypt: Implement NCryptDecrypt().
      ncrypt/tests: Clean up persisted keys.
      winhttp: Increase reply buffer size to 64KB.
      ncrypt: Add ECDSA_P256/P384/P521 support.
      ncrypt: Use a common object header in key and provider structures.
      widl: Reset state between parsing files.
      wbemprox: Return appropriate error on invalid service start mode.
      wbemprox: Fix "Automatic" start mode.
      ncrypt: Set algorithm group property for ECDSA keys.
      ncrypt: Don't fail on unhandled parameters in NCryptImportKey().
      widl: Strip any path from the module name.

Harrison Short (1):
      dbghelp: Write minidump on the calling thread.

Herman Semenoff (3):
      wbemprox: Use wcschr() instead of wcsstr() for single-character lookup.
      oledb32: Use wcschr() instead of wcsstr() for single-character lookup.
      wusa: Use wcschr() instead of wcsstr() for single-character lookup.

Huw D. M. Davies (1):
      mmdevapi: Retrieve the period from the device.

Ivan Ivlev (3):
      user32: Set control state flags after SetCapture().
      comctl32: Set control state flags after SetCapture().
      comctl32_v6: Set control state flags after SetCapture().

Jacek Caban (5):
      winegcc: Add support for building DLLs without an entry point.
      makefiles: Use -noentry instead of data-only modules for DLLs that should be ARM64X.
      ntdll: Fix NULL process handle support in NtTerminateProcess.
      winegcc: Always use -mlong-double-64 on x86 mingw targets.
      windowscodecs/tests: Use unsigned char for test data.

James Hawkins (9):
      crypt32: Increase the size variance from 8 to 16.
      wininet: Delete uncommitted cache file after closing its handle.
      wininet: Stop caching on cache file write error.
      configure: Report missing Wayland dependencies individually.
      setupapi: Clean up and return FALSE on failure in SetupCopyOEMInf.
      setupapi: Handle SetupCopyOEMInfW failure in SetupDiInstallDevice.
      win32u: Call cleanup_devices() before releasing display_device_init_mutex.
      appwiz.cpl: Close installer dialog on all addon download failures.
      appwiz.cpl: Delete downloaded MSI file after binding finishes.

Jason A. Donenfeld (1):
      cmd: Link against kernel32 for CreateProcessW.

Marc-Aurel Zent (1):
      winemac: Use NSTextInput instead of NSTextInputClient.

Michael Nelson (2):
      crypt32: Preserve authenticated attribute order when verifying signatures.
      crypt32/tests: Test signature verification with unsorted authenticated attributes.

Michael Stefaniuc (1):
      dmime: Support DX8 wave item chunks.

Nikolay Sivov (32):
      msxml3: Add a DTD validation check for required attributes.
      msxml6/tests: Add a test for validateNode() on a fragment node.
      msxml6: Explicitly fail validateNode() on document fragments.
      msxml3: Implement nextNode() for PI "attributes".
      msxml3/tests: Add a bit more tests for notation node attributes.
      msxml3: Implement xml() property for notation nodes.
      msxml3/tests: Add some tests for notation node child properties.
      msxml3: Implement child list related properties for notation nodes.
      dwrite/layout: Partially implement HitTestPoint().
      mfsrcsnk/tests: Test for stream refcount change instead of it being fully released.
      d2d1: Fix texture object leak on failure path during WIC target initialization.
      d2d1: Remove duplicated assignment.
      d2d1: Avoid accessing potentially reallocated memory.
      d2d1/effect: Remove duplicated property value clearing.
      d2d1/tests: Add some tests for GetPropertyNameLength() with invalid property index.
      d2d1/effect: Fix GetPropertyNameLength() for invalid property index.
      d2d1/effect: Fix inconsistent cleanup on failure path to create an effect instance.
      dwrite: Remove duplicated assignment.
      dwrite/bidi: Fix copy-paste issue in a class test.
      dwrite: Make sure property value pointer is always initialized.
      dwrite: Fix variation axis HIDDEN flag check.
      dwrite: Remove duplicated axis value data.
      include: Fix XML_ERROR structure definition.
      msxml3/xpath: Fix failure condition in the id() function stub.
      msxml3/xpath: Fix argument count check in sum().
      msxml3/xpath: Fix argument count check in string-length().
      msxml3/xpath: Fix early return condition in concat().
      msxml3/sax: Fix copy-paste typo in xmldecl-standalone getter.
      msxml3/sax: Fix xmldecl-* property values leak.
      msxml3/sax: Fix a DOCTYPE node name leak.
      msxml3/sax: Fix parsed data leak on malformed PI nodes.
      msxml3: Make string buffer cleanup safer on resize failure.

Owen Rudge (1):
      winemac.drv: Advertise PFD_SUPPORT_GDI support.

Paul Gofman (10):
      ntdll: Ignore semaphore errors in RtlpUnWaitCriticalSection().
      xinput: Only rescan controllers if device could have changed.
      mfwmaaec: Add dll with stub Voice Capture DSP MFT and DMO.
      mfwmaaec: Fill static output stream info.
      mfwmaaec: Recognize some properties.
      mfwmaaec: Implement _SetOutputType() and related info query.
      mfwmaaec: Recognize VAD feature.
      mmdevapi: Don't try to convert QPC and avoid overflow.
      mfwmaaec: Implement audio capture in media_object_ProcessOutput().
      mfwmaaec: Report voice presence when VAD mode is requested.

Piotr Caban (2):
      msvcp90: Fix structure size in basic_ostream<wchar_t>::operator<<(double).
      msvcrt: Fix _wmktemp() implementation.

Piotr Wojnarowski (1):
      gdiplus: Avoid starting a dash at the end of a segment.

Rémi Bernon (30):
      win32u: Introduce a debugstr_size helper to wrap wine_dbgstr_point casts.
      win32u: Get rid of the framebuffer surface read_desc / draw_desc.
      win32u: Keep track of the GL drawable target window directly.
      opengl32/tests: Fix some broken tests on AMD.
      opengl32/tests: Check that 3.1 context without flags supports deprecated features.
      win32u: Use a dedicated struct for d3dkmt_sync objects.
      opengl32: Update registry for GL_MESA_map_buffer_client_pointer.
      win32u: Create some d3dkmt swapchain objects for remote GL presentation.
      win32u: Create d3dkmt swapchains in the remote toplevel window process.
      win32u: Create raw client surface and drawable for remote d3dkmt swapchains.
      win32u: Check swapchain size changes when trying to acquire images.
      win32u: Create a mutex and sync object with the swapchain objects.
      win32u: Create some dummy resources for swapchain objects.
      winewayland: Remove unnecessary wayland_win_data_get call.
      win32u: Update the framebuffer surface size on swapchain resize.
      win32u: Use a timeout when sending a remote swapchain creation message.
      win32u: Post a message to present a remote swapchain image.
      server: Use a specific struct for d3dkmt sync objects.
      server: Pass the keyed_wait list to helper functions.
      server: Use unsigned __int64 for d3dkmt keyed wait value.
      win32u: Implement d3dkmt sync CPU wait / signal operations.
      user32/tests: Check that GetMenuItemRect recomputes the menu bar height.
      user32/tests: Check that MenuItemFromPoint recomputes the menu bar height.
      user32/tests: Check that HiliteMenuItem recomputes the menu bar height.
      user32/tests: Check that other functions don't recompute the menu bar height.
      win32u: Pass the origin position and width to calc_menu_bar_size.
      win32u: Use NtUserDrawMenuBarTemp to recompute menu bar height.
      win32u: Recalculate menu bar layout in find_item_by_coords / GetMenuItemRect.
      win32u: Recompute invalid menu bar height in select_item.
      win32u: Always select the item in HiliteMenuItem.

Sven Baars (3):
      dnsapi: Add a missing else (Coverity).
      gdiplus: Fix a copy-paste error (Coverity).
      gdiplus: Make sure anchor_path is initialized when calling GdipDeletePath() (Coverity).

Vasiliy Stelmachenok (1):
      win32u: Always create OpenGL 3.1 context using compatibility profile.

Vibhav Pant (3):
      wpnapps: Add stubs for IToastNotificationManagerStatics.
      wpnapps: Add stubs for IToastNotificationFactory.
      wpnapps: Add stubs for IToastNotifier.

Vishnunithyasoundhar S (3):
      crypt32: Match IP address SANs in SSL hostname check.
      oleaut32: Fix float mantissa normalization boundary in VARIANT_DI_normalize.
      comctl32/comboex: Don't pass a NULL string to the CBEN_ENDEDIT notif.

Vitaly Lipatov (10):
      po: Update Russian translation.
      gdiplus/tests: Test flattening a translated Bezier curve.
      gdiplus: Compute the flatness of Bezier curves relative to the start point.
      gdiplus: Don't loop forever on Bezier curves that can't be flattened.
      gdiplus/tests: Test strings with a negative layout rectangle.
      gdiplus: Don't draw strings clipped to an empty layout rectangle.
      gdiplus: Don't add strings to paths with a negative layout width.
      gdiplus: Don't measure character ranges in a negative layout rectangle.
      gdiplus: Reject a negative offset in GdipDrawCurve3() and GdipDrawCurve3I().
      gdiplus: Close every open figure when converting a path to edges.

Zhiyi Zhang (23):
      winecoreaudio.drv: Move some helpers.
      winecoreaudio.drv: Add a get_format_channel_mask() helper.
      winecoreaudio.drv: Add a ca_get_layout_channel_mask() helper.
      winecoreaudio.drv: Add a get_device_channel_mask() helper.
      winecoreaudio.drv: Add a SpatialMixer audio unit to mix input audio when necessary.
      advapi32: Support more privilege names in LookupPrivilegeValue().
      advapi32/tests: Add NtCreateToken() tests.
      ntdll: Create tokens with the specified owner in NtCreateToken().
      ntdll: Create tokens with the specified primary group in NtCreateToken().
      server: Create TokenElevationTypeFull tokens by default in the create_token request.
      ntdll: Rename token_id to auth_id for NtCreateToken().
      comctl32/tests: Add TTM_ADJUSTRECT tests.
      comctl32/tooltips: Implement TTM_ADJUSTRECT.
      services/tests: Add LogonUser() tests.
      advapi32: Implement LogonUserW() for service accounts.
      services/tests: Add tests for GetUserName().
      advapi32: Query WINEUSERNAME directly for the first user account in LookupAccountSidW().
      advapi32: Forward GetUserNameA() to GetUserNameW().
      advapi32: Reimplement GetUserNameW() to report the correct username.
      userenv: Fix the domain size for LookupAccountSidW() in CreateEnvironmentBlock().
      msv1_0: Fix the username and domain size when calling LookupAccountSidW() in ntlm_SpQueryCredentialsAttributes().
      advapi32: Specify a domain buffer when calling LookupAccountSidW() in GetUserNameW().
      userenv: Specify a domain buffer when calling LookupAccountSidW() in GetUserProfileDirectoryW().
```
