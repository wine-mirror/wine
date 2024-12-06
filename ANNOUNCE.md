The Wine development release 10.0-rc1 is now available.

This is the first release candidate for the upcoming Wine 10.0. It
marks the beginning of the yearly code freeze period. Please give this
release a good testing and report any issue that you find, to help us
make the final 10.0 as good as possible.

What's new in this release:
  - Bundled vkd3d upgraded to version 1.14.
  - Mono engine updated to version 9.4.0.
  - Initial version of a Bluetooth driver.
  - UTF-8 support in the C runtime functions.
  - Support for split debug info using build ids.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/10.0/wine-10.0-rc1.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-10.0-rc1/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 10.0-rc1 (total 17):

 - #43172  IDirectPlay4::EnumConnections needs to support wide string in DPNAME structure
 - #56109  Broken Radiobutton navigation (Up/Down, Accelerators) in several InnoSetup installers
 - #56709  PackTouchHitTestingProximityEvaluation not located in USER32.dll when attempting to run Clip Studio Paint 3.0
 - #56838  FL Studio 21 gui problem
 - #57064  Bloodrayne 2 (legacy and Terminal Cut): graphical issue (foggy screen)
 - #57308  Cannot load split debug symbols under /usr/lib/debug
 - #57401  Dragon Unpacker crashes on both wine devel and staging
 - #57411  Heroes of the Storm: screen sporadically flickers black
 - #57431  Links 2003 Crashes
 - #57437  PStart isn't showing a menu in the tray bar
 - #57453  Regression:  TWM: Cursor position offset in *some* programs.
 - #57457  Mathcad 15 crashes when enter trace tab
 - #57463  winebus always crashing with unknown since 8b41c2cfddba1f9973246f61e39d4a4d92912da5
 - #57472  Systray support is broken in Wine 9.22
 - #57474  Windows disappear irreversibly when are not shown on a virtual desktop
 - #57477  After commit of 484c61111ef32d75dcf5cf1656b4469b4de3ec37 game could not launch with dxvk
 - #57493  Mathcad 15 crashes on startup due to unhandled domdoc MaxElementDepth property

### Changes since 9.22:
```
Aida Jonikienė (6):
      dsound: Handle NaN values in the 3D code.
      dsound: Add an angle check for SetOrientation().
      dsound/tests: Add NaN tests for floating-point 3D functions.
      dsound: Add non-NaN value tests for SetOrientation().
      winevulkan: Mirror function handling in vk_is_available_instance_function32().
      winevulkan: Use WINE_UNIX_LIB instead of WINE_VK_HOST.

Alexandre Julliard (20):
      ntdll: Initial version of NtContinueEx().
      ntdll: Always return the handle from NtCreateIoCompletion().
      server: Do not allow to open an existing mailslot in NtCreateMailslotFile.
      server: Fix a token reference leak.
      ntdll/tests: Remove some workarounds for old Windows versions.
      ntdll/tests: Add tests for opening objects with zero access.
      vkd3d: Import upstream release 1.14.
      server: Use the correct handle allocation pattern for all object types.
      win32u: Add some access rights when creating a desktop object.
      server: Make CurrentControlSet a symlink in new prefixes.
      ntdll: Make a debug channel dynamically settable only if there's no specified class.
      taskmgr: Only list dynamically settable debug channels.
      server: Only store a Unix name for regular files.
      widl: Avoid unused variable warning.
      configure: Correctly check the --enable-build-id option.
      winegcc: Remove support for .def files as import libraries.
      winebuild: Remove support for .def files as import libraries.
      wrc: Use the correct error function for syntax errors.
      server: Print signal names in traces.
      ntdll: Move update_hybrid_metadata() to the ARM64EC backend.

Alfred Agrell (2):
      dsound/tests: Add nonlooping SetNotificationPositions test.
      dsound: Fix SetNotificationPositions at end of nonlooping buffer.

Alistair Leslie-Hughes (3):
      include: Add _WIN32_WINNT_ version defines.
      include: Add DB_VARNUMERIC struct.
      include: Add SQL_C_TCHAR define.

Andrew Nguyen (1):
      msxml3: Accept the domdoc MaxElementDepth property.

Anton Baskanov (2):
      dplayx/tests: Test client side of GetMessageQueue() separately.
      dplayx: Support DPMESSAGEQUEUE_RECEIVE in GetMessageQueue().

Billy Laws (5):
      msi: Dynamically determine supported package architectures.
      ntdll: Test more ARM64 brk instruction exception behaviour.
      ntdll: Fix reported exception code for some brk immediates.
      ntdll: Add arm64ec_get_module_metadata helper.
      ntdll: Force redirect all ARM64EC indirect calls until the JIT is ready.

Brendan McGrath (2):
      mfmediaengine: Implement the Simple Video Renderer.
      mfmediaengine: Fallback to sample copy if scaling is required.

Brendan Shanks (3):
      include: Use %fs/%gs prefixes instead of a separate .byte 0x64/.byte 0x65.
      ntdll: Use %fs/%gs prefixes instead of a separate .byte 0x64/.byte 0x65.
      ntdll: Use sched_getcpu instead of the getcpu syscall.

Conor McCarthy (12):
      winegstreamer: Handle null transform in video IMediaObject::Flush().
      winegstreamer: Handle null transform in video IMFTransform::ProcessMessage() DRAIN.
      winegstreamer: Handle null transform in video IMFTransform::ProcessMessage() FLUSH.
      winegstreamer: Handle null transform in WMA IMediaObject::Flush().
      winegstreamer: Return the result code from media_source_Pause().
      mf/tests: Add tests for shutting down a media source used in a session.
      mf: Handle media source EndGetEvent() failure due to shutdown.
      mf: Handle media source BeginGetEvent() failure due to shutdown.
      mf: Handle media source event subscription failure due to source shutdown.
      mf: Handle media source Start() failure due to source shutdown.
      mf: Introduce IMFMediaShutdownNotify for notification of media source shutdown.
      winegstreamer: Send media source shutdown notification via IMFMediaShutdownNotify.

Daniel Lehman (5):
      msvcr120/tests: Add tests for _fsopen.
      msvcp120/tests: Add tests for _Fiopen.
      ucrtbase/tests: Add tests for _fsopen.
      msvcp140/tests: Add tests for _Fiopen.
      msvcp140: Call into fopen from _Fiopen.

Dmitry Timoshkov (1):
      ntdll: Add NtFlushBuffersFileEx() semi-stub.

Dāvis Mosāns (1):
      ntdll/tests: Unify APC test functions.

Elizabeth Figura (14):
      wined3d: Use wined3d_texture_download_from_texture() even if the dst texture map binding is not valid.
      wined3d: Beginnings of an HLSL FFP pixel shader implementation.
      wined3d: Implement pretransformed varyings in the HLSL FFP pipeline.
      wined3d: Take the depth buffer into account for HLSL pretransformed draws.
      wined3d: Implement lighting in the HLSL FFP pipeline.
      wined3d: Implement vertex fog in the HLSL FFP pipeline.
      quartz/dsoundrender: Always treat samples as continuous if they are late or out of order.
      quartz/dsoundrender: Remove the unused "tStop" argument to send_sample_data().
      quartz/dsoundrender: Play non-discontinuous samples consecutively.
      quartz/tests: Test whether the DirectSound renderer provides a position.
      quartz/dsoundrender: Do not provide time to the passthrough.
      quartz/dsoundrender: Do not ignore preroll samples.
      quartz/dsoundrender: Queue samples and render them on a separate thread.
      quartz/dsoundrender: Use send_sample_data() to fill the buffer with silence at EOS.

Eric Pouech (11):
      kernel32: Add tests for checking the exit code of default ctrl-c handlers.
      kernelbase: Fix exit code for default ctrl-c handler.
      ntdll/tests: Fix format warning with clang.
      configure: Properly test clang for dwarf support.
      winegcc: Remap build-id linker option for clang.
      configure: Use -Wl,--build-id unconditionally if requested.
      configure: Don't add -Wl,--build-id linker option to CFLAGS.
      dbghelp: Extend search for buildid in system directories.
      dbghelp: Search debug info with buildid for RSDS debug entry w/o filenames.
      server: Ensure in pending delete on close that path to unlink are unique.
      winedbg: Add support for dynamic debug channel.

Esme Povirk (1):
      mscoree: Update Wine Mono to 9.4.0.

Evan Tang (2):
      kernelbase: Properly return 0 from EnumSystemFirmwareTable on error.
      kernelbase: Add test for EnumSystemFirmwareTables on missing provider.

Fabian Maurer (8):
      dplayx: Add a few more locks (Coverity).
      comctl32/tests: Add tests for radio button WM_SETFOCUS.
      comctl32: Send parent BN_CLICKED notification when a radio button get focused.
      user32/tests: Add tests for radio button WM_SETFOCUS.
      user32: Send parent BN_CLICKED notification when a radio button get focused.
      oleaut32: Make OleCreateFontIndirect return error if font name is missing.
      oleaut32: Remove unneeded null checks.
      d3dx9: Remove superfluous null check (Coverity).

Gabriel Ivăncescu (38):
      mshtml: Move htmlcomment.c contents into htmltextnode.c.
      mshtml: Add an internal IWineHTMLCharacterData interface and forward text node methods to it.
      mshtml: Expose the props from the IWineHTMLCharacterData interface for CharacterDataPrototype.
      mshtml: Don't expose toString from text nodes in IE9+ mode.
      mshtml: Expose IHTMLCommentElement2 interface for comment elements.
      mshtml: Don't expose 'atomic' prop from comment nodes in IE9+ modes.
      mshtml: Don't expose element props from comment nodes in IE9+ modes.
      mshtml: Implement get_data for legacy DOCTYPE comment elements.
      mshtml: Expose ie9_char as char for KeyboardEvent.
      mshtml: Make PageTransitionEvents only available in IE11 mode.
      mshtml: Make ProgressEvent constructor only available in IE10+ modes.
      mshtml: Expose respective props from Element prototype.
      mshtml: Don't expose fireEvent from elements in IE11 mode.
      mshtml: Don't expose onmspointerhover from elements in IE11 mode.
      mshtml: Move toString from HTMLElement to HTMLAnchorElement or HTMLAreaElement in IE9+ modes.
      mshtml: Move hasAttributes from HTMLElement to HTMLDOMNode in IE9+ modes.
      mshtml: Move normalize from HTMLElement to HTMLDOMNode in IE9+ modes.
      mshtml: Don't expose onpage from elements in IE9+ modes.
      mshtml: Don't expose expression methods from elements in IE9+ modes.
      mshtml: Don't expose some props from elements in IE10+ modes.
      mshtml: Don't expose some props from elements in IE11 mode.
      mshtml: Move HTMLTableDataCellElement prototype props to the HTMLTableCellElement prototype.
      mshtml: Add IHTMLDOMNode2 in every mode in node's init_dispex_info.
      mshtml: Get rid of HTMLELEMENT_TIDS.
      mshtml: Move HTMLDocument prototype props to the Document prototype.
      mshtml: Use DocumentPrototype as the document's prototype for modes prior to IE11.
      mshtml: Expose the right props from document fragments.
      mshtml: Don't expose some props from document prototype depending on mode.
      mshtml/tests: Add more tests for the style aliased prop names.
      mshtml: Expose respective props from MSCSSPropertiesPrototype.
      mshtml: Prefer builtins for style aliases that have the same name.
      mshtml: Move 'filter' prop to MSCSSPropertiesPrototype in IE9 mode.
      mshtml: Don't expose 'behavior' prop from styles in IE11 mode.
      mshtml: Don't expose the clip* props from style declaration or properties in IE9+ modes.
      mshtml: Don't expose the *Expression methods from styles in IE9+ modes.
      mshtml: Don't expose toString from styles in IE9+ modes.
      mshtml: Expose respective props from StyleSheetPrototype.
      mshtml: Get rid of unused HTMLElement_toString_dispids.

Georg Lehmann (1):
      winevulkan: Update to VK spec version 1.4.303.

Gerald Pfeifer (3):
      win32u: Don't use bool as member of a union type.
      msi: Use mybool instead of bool as variable name.
      winhlp32: Drop unused member of struct lexret.

Giovanni Mascellani (2):
      user32/tests: Check that message-only windows ignore WS_EX_TOPMOST.
      win32u/window: Ignore changing WS_EX_TOPMOST for message-only windows.

Hans Leidekker (2):
      msi: Assume PLATFORM_INTEL if the template property is missing.
      bcrypt: Trace returned handles.

Haoyang Chen (1):
      gdiplus: Use the FormatID of the source image when cloning.

Henri Verbeet (3):
      d3dcompiler/tests: Clean up tests fixed by vkd3d merges.
      d3d10_1/tests: Clean up tests fixed by vkd3d merges.
      d3dx11/tests: Clean up tests fixed by vkd3d merges.

Jacek Caban (18):
      msvcrt/tests: Silence -Wformat-security Clang warning in test_snprintf.
      include: Apply LONG_PTR format hack only to Wine build.
      include: Use LONG_PTR format hack on Clang in MSVC mode.
      include: Use format attribute on Clang in MSVC mode.
      d3d11/tests: Always use a format string in winetest_push_context calls.
      ddraw/tests: Always use a format string in winetest_push_context calls.
      imagehlp: Cast AddressOfData to size_t in debug traces.
      mmdevapi/tests: Use %u format for unsigned int arguments.
      include: Enable format attributes for debug traces in Clang MSVC mode.
      gdiplus: Cast enums to unsigned type when validating its value.
      jscript: Avoid unused variable warning.
      msi: Avoid unused variable warning.
      msxml: Avoid unused variable warning.
      vbscript: Avoid unused variable warning.
      wbemprox: Avoid unused variable warning.
      include: Use inline assembly on Clang MSVC mode in exception helpers.
      jscript: Move property allocation to update_external_prop.
      jscript: Add support for deleting host properties.

Louis Lenders (1):
      msvcp140: Add a version resource.

Marc-Aurel Zent (8):
      ntdll: Implement NtGetCurrentProcessorNumber for macOS on x86_64.
      server: Do not suspend mach task in read_process_memory.
      server: Use mach_vm_read_overwrite in read_process_memory.
      server: Do not suspend mach task in get_selector_entry.
      server: Use mach_vm_read_overwrite in get_selector_entry.
      server: Do not suspend mach task in write_process_memory.
      server: Do not page-align address in  write_process_memory.
      server: Work around macOS W^X limitations in write_process_memory.

Matteo Bruni (5):
      d3dcompiler/tests: Clean up further tests fixed by vkd3d merges.
      d3dx9_43: Generate an import library.
      d3dx9/tests: Add d3dx9_43 tests.
      d3dx9/tests: Test the 'double' HLSL data type.
      d3dcompiler/tests: Test the 'double' HLSL data type.

Mohamad Al-Jaf (5):
      windows.networking.connectivity: Add stub dll.
      windows.networking.connectivity: Add INetworkInformationStatics stub interface.
      windows.networking.connectivity: Implement INetworkInformationStatics::GetInternetConnectionProfile().
      windows.networking.connectivity/tests: Add some INetworkInformationStatics::GetInternetConnectionProfile() tests.
      windows.networking.connectivity: Implement IConnectionProfile::GetNetworkConnectivityLevel().

Nikolay Sivov (6):
      d2d1/effect: Improve handling of blob properties.
      windowscodecs/tests: Use string literals in the metadata tests.
      windowscodecs/tests: Add some tests for CreateMetadataReader().
      windowscodecs/tests: Add a basic test for CreateComponentEnumerator().
      windowscodecs/metadata: Add a helper to iterate over components.
      windowscodecs: Implement CreateMetadataReader().

Orin Varley (3):
      msxml3/tests: Add indentation test.
      comctl32/tests: Add tests for a small number of items but big size to the combobox dropdown size tests.
      comctl32: Make CBS_NOINTEGRALHEIGHT only set minimum combobox height.

Paul Gofman (1):
      explorer: Prevent apps from showing Wine specific shell tray window with no icons.

Piotr Caban (43):
      include: Add ___lc_codepage_func() declaration.
      msvcp60: Improve wcsrtombs implementation.
      msvcp60/tests: Add wcsrtombs tests.
      msvcrt: Call _wmkdir in _mkdir function.
      msvcrt: Call _wrmdir in _rmdir function.
      msvcrt: Call _wchdir in _chdir function.
      msvcrt: Call _wgetcwd in _getcwd function.
      msvcrt: Call _wgetdcwd in _getdcwd function.
      msvcrt: Call _wfullpath in _fullpath function.
      ole32: Fix unsupported vector elements detection in PropertyStorage_ReadProperty.
      ole32/tests: Add FMTID_UserDefinedProperties property storage tests.
      ole32: Read property storage section from correct location.
      ole32/tests: Add more FMTID_UserDefinedProperties property storage tests.
      msvcrt: Prepare _fsopen to handle UTF-8 strings.
      msvcrt: Call _wunlink in _unlink function.
      msvcrt: Call _waccess in _access function.
      msvcrt: Call _wchmod in _chmod function.
      msvcrt: Call _unlink in remove function.
      msvcrt: Call _wunlink in _wremove function.
      msvcrt: Prepare _mktemp to handle UTF-8 strings.
      msvcrt: Prepare _mktemp_s to handle UTF-8 strings.
      msvcrt: Call _wstat64 in _stat64 function.
      msvcrt: Call _wrename in rename function.
      msvcrt: Call _wtempnam in _tempnam function.
      msvcrt: Don't return success on GetFullPathName error in _wsearchenv_s.
      msvcrt: Prepare _searchenv_s() for utf-8 encoded filename.
      include: Cleanup corecrt_io.h file and use it in io.h.
      msvcrt: Call _wfindfirst32 in _findfirst32 function.
      msvcrt: Call _wfindnext32 in _findnext32 function.
      msvcrt: Call _wfindfirst64 in _findfirst64 function.
      msvcrt: Call _wfindnext64 in _findnext64 function.
      msvcrt: Call _wfindfirst64i32 in _findfirst64i32 function.
      msvcrt: Call _wfindnext64i32 in _findnext64i32 function.
      msvcrt: Add putenv() utf-8 tests.
      msvcrt: Return error on NULL path parameter in _wsopen_dispatch.
      msvcrt: Prepare _sopen_dispatch to handle utf-8 encoded path.
      msvcrt: Prepare freopen to handle utf-8 encoded path.
      msvcrt: Prepare _loaddll to handle utf-8 encoded path.
      msvcrt: Prepare _spawnl to handle utf-8 encoded arguments.
      msvcrt: Prepare _execle to handle utf-8 encoded arguments.
      msvcrt: Prepare remaining process creation functions to handle utf-8 encoded arguments.
      ucrtbase: Enable utf8 support.
      ucrtbase: Always use CP_ACP when converting environment block.

Roman Pišl (1):
      kernel32: Use a proper import for HeapFree.

Rémi Bernon (45):
      winebus: Ignore reports with unexpected IDs.
      winex11: Read _NET_SUPPORTED atom list on process attach.
      winex11: Only request the supported _NET_WM_STATE atoms.
      winevulkan: Add missing wine_vkGetPhysicalDeviceSurfaceFormatsKHR manual wrapper.
      win32u: Use PFN_* typedefs for vulkan function pointers.
      winevulkan: Get rid of the instance/device funcs structs.
      winevulkan: Generate ALL_VK_(DEVICE|INSTANCE)_FUNCS in wine/vulkan.h.
      winevulkan: Move vulkan_client_object header to wine/vulkan_driver.h.
      winevulkan: Name wine_instance parameters and variables more consistently.
      winevulkan: Hoist physical device array and client instance handle.
      winevulkan: Introduce a new vulkan_instance base structure.
      winevulkan: Introduce a new vulkan_physical_device base structure.
      winevulkan: Name wine_device parameters and variables more consistently.
      winevulkan: Introduce a new vulkan_device base structure.
      winevulkan: Restore some wine_*_from_handle helpers.
      winevulkan: Introduce a new vulkan_queue base structure.
      winevulkan: Introduce a new vulkan_surface base structure.
      winevulkan: Introduce a new vulkan_swapchain base structure.
      winevulkan: Use a vulkan_object header for other wrappers.
      winevulkan: Use the result to decide if creation failed.
      winevulkan: Introduce a new vulkan_object_init helper.
      winevulkan: Fix incorrect client queue pointers.
      winevulkan: Avoid changing client command buffer pointer.
      winevulkan: Get rid of unnecessary *to_handle helpers.
      winevulkan: Use the vulkan object as the wrapper tree node.
      winevulkan: Keep the host function pointers in devices and instances.
      win32u: Move surface and swapchain wrappers from winevulkan.
      winex11: Don't update Win32 window position for offscreen windows.
      winex11: Do not use desired_state when computing state updates.
      winex11: Set a non-transparent window background pixel color.
      win32u: Let fullscreen windows cover entire monitors, keeping aspect ratio.
      winex11: Use bilinear filtering in xrender_blit.
      quartz/dsoundrender: Rename "This" to "filter".
      quartz/dsoundrender: Add missing static qualifier to IDispatch methods.
      quartz/dsoundrender: Make brace placement consistent.
      quartz/dsoundrender: Use a consistent style for method names.
      quartz/dsoundrender: Make trace messages more consistent.
      win32u: Add a force parameter to lock_display_devices.
      win32u: Implement update_display_cache with lock_display_devices.
      win32u: Remove recursive lock_display_devices calls.
      win32u: Hold the display_lock when checking the cache update time.
      mfmediaengine: Implement D3D-aware video frame sink.
      windows.networking.connectivity: Use %I64d instead of %llu.
      winex11: Move the _NET_SUPPORTED information to the thread data.
      winex11: Listen to root window _NET_SUPPORTED property changes.

Santino Mazza (2):
      mmdevapi/tests: Test for IAudioClockAdjustment.
      mmdevapi: Do not modify buffer size after sample rate change.

Sven Baars (1):
      win32u: Allow unsetting the user driver.

Tim Clem (3):
      explorer: Apply a default admin token when running for the desktop.
      Revert "win32u: Create explorer with the thread effective access token.".
      kernelbase: Improve logging of information classes in GetTokenInformation.

Tingzhong Luo (3):
      dwrite/tests: Add a test for DrawGlyphRun() bounds.
      dwrite/gdiinterop: Always return valid bounds from DrawGlyphRun on success.
      dwrite/gdiinterop: Apply dpi scaling to the whole target transform.

Torge Matthies (2):
      advapi32/tests: Add test for CurrentControlSet link.
      loader: Add Default, Failed, and LastKnownGood values to HKLM\System\Select.

Vibhav Pant (15):
      winebth.sys: Add base winebth.sys driver.
      winebth.sys: Add a basic unixlib stub using DBus.
      winebth.sys: Create radio PDOs from the list of org.bluez.Adapter1 objects on BlueZ.
      winebth.sys: Derive a unique hardware ID for radio PDOs from their corresponding BlueZ object path.
      winebth.sys: Register and enable BTHPORT_DEVICE and BLUETOOTH_RADIO interfaces for radio PDOs.
      bluetoothapis/tests: Fix potential test failure from memcmp'ing uninitialized bytes.
      bluetoothapis/tests: Add tests for BluetoothFindFirstRadio.
      bluetoothapis/tests: Add tests for BluetoothFindNextRadio.
      bluetoothapis/tests: Add tests for BluetoothFindRadioClose.
      bluetoothapis: Implement BluetoothFindFirstRadio, BluetoothFindNextRadio, BluetoothFindRadioClose.
      winebth.sys: Set radio PDO properties from the device's corresponding org.bluez.Adapter1 object properties.
      winebth.sys: Create new radio PDOs on receiving InterfacesAdded for objects that implement org.bluez.Adapter1.
      winebth.sys: Remove the corresponding radio PDO on receiving InterfacesRemoved for a org.bluez.Adapter1 object.
      winebth.sys: Update radio PDO properties on receiving PropertiesChanged for an org.bluez.Adapter1 object.
      winebth.sys: Implement IOCTL_BTH_GET_LOCAL_INFO.

Vijay Kiran Kamuju (2):
      user32: Add PackTouchHitTestingProximityEvaluation stub.
      user32: Add EvaluateProximityToRect stub.

Vladislav Timonin (1):
      comctl32/edit: Scroll caret on Ctrl+A.

Zhiyi Zhang (2):
      appwiz.cpl: Fix wine_get_version() function pointer check.
      uxtheme: Check DrawThemeEdge() content rectangle pointer.

Ziqing Hui (5):
      qasf: Return S_FALSE for flushing in dmo_wrapper_sink_Receive.
      qasf/tests: Test dmo_wrapper_sink_Receive if downstream fail to receive.
      qasf: Return failure in dmo_wrapper_sink_Receive if process_output fails.
      qasf/tests: Add more tests for dmo_wrapper_sink_Receive.
      qasf: Correctly return failure in process_output.
```
