The Wine development release 10.15 is now available.

What's new in this release:
  - Unicode character tables updated to Unicode 17.0.0.
  - Zip64 support in Packaging services.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/10.x/wine-10.15.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-10.15/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 10.15 (total 16):

 - #51345  Regression: Visual Studio 2005 "package load failure"
 - #56278  wayland: dropdowns is rendered as toplevel
 - #57192  X11DRV_SetCursorPos breaks when xinput "Coordinate Transformation Matrix" is customized
 - #57444  Multiple games crashes with new wow64 ("pop gs" behaves differently in 64bit compatibility mode) (Exertus darkness, Claw, Bloodrayne Demo)
 - #57478  Sims 2 black-screen when running with Nvidia 470.256.02 and dxvk 1.10.3
 - #57912  cmd: not every ( is a command grouping
 - #57913  cmd: echo(abc is misparsed
 - #58027  trivial use of Win32 GNU make fails
 - #58335  Wine can fail if avx is not available
 - #58503  Resource leak in wayland_pointer_set_cursor_shape can cause mouse cursor to dissappear after exhausting gdi handles
 - #58513  Wine 10.9 completely broke (LGA775 Core2Quad)
 - #58585  unnamed keymap layout leads to null pointer dereference in find_xkb_layout_variant
 - #58614  wine cmd prints "::" style comments
 - #58619  Steam fails to launch
 - #58635  CapCut crashes upon launch, needs unimplemented function IPHLPAPI.DLL.SetPerTcp6ConnectionEStats
 - #58636  CapCut installer fails: CreateFileW with  FILE_ATTRIBUTE_DIRECTORY | FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_POSIX_SEMANTICS  should create a directory instesd of a  file.

### Changes since 10.14:
```
Alex Henrie (1):
      wineboot: Fix a memory leak in create_computer_name_keys.

Alexandre Julliard (15):
      faudio: Import upstream release 25.09.
      png: Import upstream release 1.6.50.
      tools: Upgrade the config.guess/config.sub scripts.
      makedep: Build tool names once at startup.
      ntdll: Swap the mxcsr register value between PE and Unix.
      gitlab: Merge the platform docker files back into a single file.
      gitlab: Install the conflicting i386 gstreamer packages manually.
      ntdll: Save mxcsr explicitly in the xsavec code path.
      maintainers: List gitlab usernames.
      gitlab: Add CI job to trigger winehq-bot processing.
      ntdll: Move some LDT definitions to the private header.
      ntdll: Add a helper to update the LDT copy.
      ntdll: Add helpers to build some specific LDT entries.
      ntdll: Always use 32-bit LDT base addresses.
      nls: Update character tables to Unicode 17.0.0.

Alexandros Frantzis (1):
      winewayland: Fix GDI object leak.

Alfred Agrell (1):
      preloader: Make thread_ldt reference position independent.

Alistair Leslie-Hughes (2):
      winmm: Always call MCI_UnmapMsgAtoW in mciSendCommandA.
      winmm: MCI_MapMsgAtoW return error code directly instead of a tri-value.

Bartosz Kosiorek (1):
      gdi32: From Windows 2000 FixBrushOrgEx() is a NOP.

Bernhard Übelacker (1):
      winedbg: Reserve more memory for symbol value (ASan).

Billy Laws (2):
      ntdll/tests: Add THREAD_CREATE_FLAGS_SKIP_THREAD_ATTACH test.
      ntdll: Support THREAD_CREATE_FLAGS_SKIP_THREAD_ATTACH flag.

Brendan Shanks (4):
      crypt32: Avoid syscall fault in process_detach() if GnuTLS failed to load.
      secur32: Avoid syscall fault in process_detach() if GnuTLS failed to load.
      ntdll: Fix crash in check_invalid_gsbase() on macOS.
      dbghelp: Don't try to load 64-bit ELF/Mach-O debug info in a Wow64 process.

Carlo Bramini (1):
      winmm: WaveOutGetID: Return correct id for WAVE_MAPPER.

Christian Tinauer (1):
      crypt32: Accept PKCS12_ALWAYS_CNG_KSP flag and fall back to standard import.

Connor McAdams (12):
      d3dx10/tests: Add more DDS file DXGI format mapping tests.
      d3dx10/tests: Add more DDS file header handling tests.
      d3dx10: Add support for parsing DXT10 DDS headers to shared code.
      d3dx10: Only validate header size for DDS files in d3dx10.
      d3dx10: Add support for DXGI formats in d3dx_helpers.
      d3dx10: Exclusively use shared code for parsing DDS files in get_image_info().
      d3dx10: Add support for d3dx10+ image file formats in shared code.
      d3dx10: Exclusively use shared code for parsing all files in get_image_info().
      d3dx11_42: Don't share source with d3dx11_43.
      d3dx11/tests: Add a helper function for checking image info structure values.
      d3dx11/tests: Import more image info tests from d3dx10.
      d3dx11: Implement D3DX11GetImageInfoFromMemory() using shared code.

Dmitry Timoshkov (1):
      gdiplus: Manually blend to white background if the device doesn't support alpha blending.

Elizabeth Figura (7):
      ntdll: Add some traces to synchronization methods.
      ntdll: Add stub functions for in-process synchronization.
      ntdll: Retrieve and cache an ntsync device on process init.
      server: Create inproc sync events for message queues.
      server: Add a request to retrieve the inproc sync fds.
      ntdll: Check inproc sync handle access rights on wait.
      quartz/tests: Fix a failing test in test_video_window_owner().

Eric Pouech (14):
      cmd/tests: Add tests about opening/closing parenthesis.
      cmd: Don't create binary node with NULL RHS.
      cmd: Simplify builtin ECHO implementation.
      cmd: Fix handling of '(' in echo commands.
      cmd: Fix unmatched closing parenthesis handling.
      cmd/tests: Add more tests about ERASE builtin command.
      cmd: Fix return code for ERASE builtin command.
      cmd/tests: Add tests about (not) echoing labels.
      cmd: Don't display labels when echo mode is ON.
      cmd: Simplify setting console colors.
      cmd: Reuse exiting execution related helpers in wmain().
      cmd: Finish moving command line handling in dedicated helpers.
      cmd: cmd /c or /k shall handle ctrl-c events properly.
      cmd: No longer reformat output according to console width.

Gabriel Ivăncescu (13):
      mshtml: Implement DOMParser constructor and instance object.
      mshtml: Move document dispex info initialization to create_document_node.
      mshtml: Use Gecko's responseXML to create the XML document in IE10 and up.
      mshtml: Fallback to text/xml for unknown content types ending with +xml in get_mimeType.
      mshtml: Implement anchors prop for XML documents.
      mshtml: Implement DOMParser's parseFromString.
      mshtml: Use actual prop name after looking it up case insensitively on window.
      mshtml: Set non-HTML elements' prototype to ElementPrototype.
      mshtml: Expose toJSON only in IE9+ modes.
      mshtml: Don't expose toString from Performance* objects' prototypes in IE9+ modes.
      mshtml: Implement toJSON() for PerformanceTiming.
      mshtml: Implement toJSON() for PerformanceNavigation.
      mshtml: Implement toJSON() for Performance.

Gerald Pfeifer (1):
      ntdll: Fix the build of check_invalid_gsbase() on FreeBSD.

Hans Leidekker (15):
      include: Comment references to undefined member interfaces.
      include: Define IContentTypeProvider, IInputStream and IInputStreamReference.
      include: Define IUser.
      include: Define IAcceleratorKeyEventArgs and ICoreAcceleratorKeys.
      include: Define IKeyCredential and related types.
      include: Define IRfcommServiceId.
      msi: Allow pre-2.0 assemblies to be installed using 2.0 fusion.
      include: Define more USB interfaces.
      include: Define IImageDisplayProperties and IVideoDisplayProperties.
      include: Define IMediaEncodingProperties and IAudioEncodingPropertiesWithFormatUserData.
      include: Define IHolographicCamera and related types.
      include: Define IOnlineIdServiceTicket and IOnlineIdSystemIdentity.
      include: Add missing parameterized interface declarations.
      widl: Require member interfaces to be defined.
      widl: Support additional deprecated declarations.

Haoyang Chen (1):
      ntdll: Fix a buffer overflow in wcsncpy.

Jacek Caban (8):
      opengl32/tests: Add memory mapping tests.
      opengl32: Simplify wow64 memory mapping error handling.
      opengl32: Implement wrap_wglCreateContext on top of wrap_wglCreateContextAttribsARB.
      opengl32: Factor out free_context.
      opengl32: Introduce a wow64 buffer wrapper.
      opengl32: Move copy buffer allocation to Unix lib.
      opengl32: Use generated PE thunks for memory mapping functions.
      opengl32: Use generated PE thunks for memory unmapping functions.

Joe Souza (1):
      cmd: Treat COPY from CON or CON: as ASCII operation.

Kevin Puetz (1):
      oleaut32: Fix UDT record block leak in VariantClear().

Louis Lenders (2):
      iphlpapi: Add stub for SetPerTcp6ConnectionEStats.
      iphlpapi: Add stub for GetPerTcp6ConnectionEStats.

Matteo Bruni (7):
      hidclass: Set Status for pending IRPs of removed devices to STATUS_DEVICE_NOT_CONNECTED.
      hidclass: Fix check for early IRP cancellation.
      nsiproxy: Fix check for early IRP cancellation.
      dinput/tests: Fix check for early IRP cancellation.
      http.sys: Fix check for early IRP cancellation.
      winetest: Print Windows revision (UBR).
      d3dx10_33/tests: Enable tests.

Mohamad Al-Jaf (12):
      include: Add windows.media.playback.idl.
      windows.media.playback.backgroundmediaplayer: Add stub dll.
      windows.media.playback.backgroundmediaplayer: Add IBackgroundMediaPlayerStatics stub.
      windows.media.playback.mediaplayer: Implement IActivationFactory::ActivateInstance().
      windows.media.mediacontrol: Stub ISystemMediaTransportControls::add/remove_ButtonPressed().
      windows.media.mediacontrol: Stub ISystemMediaTransportControlsDisplayUpdater::ClearAll().
      windows.media.mediacontrol: Implement ISystemMediaTransportControls::put/get_IsStopEnabled().
      windows.media.mediacontrol: Stub ISystemMediaTransportControlsDisplayUpdater::put/get_Thumbnail().
      windows.media.playback.backgroundmediaplayer: Implement IBackgroundMediaPlayerStatics::get_Current().
      include: Add IMediaPlayer2 definition.
      windows.media.playback.mediaplayer: Add IMediaPlayer2 stub.
      windows.media.playback.mediaplayer: Implement IMediaPlayer2::get_SystemMediaTransportControls().

Nikolay Sivov (17):
      opcservices/tests: Use wide-char strings.
      opcservices/tests: Remove Vista workarounds.
      opcservices/tests: Use stricter return values checks.
      opcservices: Fix MoveNext() at the collection end.
      opcservices/tests: Add more tests for MovePrevious().
      opcservice: Fix iteration with MovePrevious().
      opcservices/tests: Use message context in some tests.
      opcservices: Make it clear which structures are specific to zip32.
      opcservices: Use 64-bit file sizes.
      opcservices: Add support for writing Zip64 packages.
      opcservices: Improve error handling when writing archives.
      opcservices: Use stdint types for the file header.
      opcservices: Set compression method according to part's compression options.
      opcservices: Use explicit field for the part name.
      combase/tests: Suppress trace messages from the test dll.
      opcservices: Remove separate part name allocation.
      vccorlib: Fix out of bounds access in debug helper (Coverity).

Patrick Hibbs (2):
      mmdevapi: Set DEVPKEY_Device_Driver during MMDevice_Create().
      winecfg: Fix audio tab by fetching the default device's audio driver.

Paul Gofman (15):
      ntdll: Zero aligned size in initialize_block().
      winevulkan: Handle NULL buffer pointer in vkFreeCommandBuffers().
      nsiproxy.sys: Use a separate critical section to sync icmp echo IRP cancel.
      iphlpapi: Use IOCP callback for echo async completion.
      win32u: Bump AMD internal driver version.
      kernel32/tests: Add tests for UnhandledExceptionFilter().
      kernelbase: Avoid recursive top exception filter invocation for nested exceptions.
      kernelbase: Return EXCEPTION_CONTINUE_SEARCH from UnhandledExceptionFilter() for nested exceptions.
      dbghelp: Use GetCurrentProcess() special handle in EnumerateLoadedModulesW64() if possible.
      wbemprox: Fix CIM_UINT16 handling in to_safearray().
      wbemprox: Add MSFT_PhysicalDisk table.
      wbemprox: Return an error from class_object_Next() for non-zero flags.
      wbemprox: Enumerate system properties in class_object_Next().
      iphlpapi: Fix ipforward_row_cmp().
      iphlpapi: Fix udp_row_cmp().

Rémi Bernon (90):
      dataexchange: Register runtimeclasses explicitly.
      geolocation: Register runtimeclasses explicitly.
      graphicscapture: Register runtimeclasses explicitly.
      hvsimanagementapi: Register runtimeclasses explicitly.
      iertutil: Register runtimeclasses explicitly.
      threadpoolwinrt: Register runtimeclasses explicitly.
      windows.devices.bluetooth: Register runtimeclasses explicitly.
      windows.devices.enumeration: Register runtimeclasses explicitly.
      windows.devices.usb: Register runtimeclasses explicitly.
      windows.gaming.input: Register runtimeclasses explicitly.
      win32u: Pass struct vulkan_physical_device pointer to drivers.
      winevulkan: Simplify VkDefine and typedefs generation.
      winevulkan: Generate structs and pointers for wayland platform.
      winevulkan: Generate structs and pointers for macos platform.
      winevulkan: Generate structs and pointers for xlib platform.
      winevulkan: Simplify function pointer generation.
      winevulkan: Simplify struct generation ordering.
      windows.gaming.ui.gamebar: Register runtimeclasses explicitly.
      windows.media.devices: Register runtimeclasses explicitly.
      windows.media.mediacontrol: Register runtimeclasses explicitly.
      windows.media.speech: Register runtimeclasses explicitly.
      windows.media: Register runtimeclasses explicitly.
      windows.networking.connectivity: Register runtimeclasses explicitly.
      windows.networking.hostname: Register runtimeclasses explicitly.
      windows.perception.stub: Register runtimeclasses explicitly.
      windows.security.authentication.onlineid: Register runtimeclasses explicitly.
      windows.security.credentials.ui.userconsentverifier: Register runtimeclasses explicitly.
      windows.web: Register runtimeclasses explicitly.
      include: Remove now unnecessary DO_NO_IMPORTS ifdefs.
      winevulkan: Move physical_device memory properties to vulkan_driver.h.
      winevulkan: Move extensions to struct vulkan_physical_device.
      winevulkan: Get rid of struct wine_vk_phys_dev.
      winevulkan: Move physical device pointers to struct vulkan_instance.
      win32u: Move device memory object wrapper from winevulkan.
      win32u: Move external host memory allocation to a separate helper.
      include: Add declarations for some d3dkmt functions.
      win32u: Stub NtGdiDdDDICheckOcclusion syscall.
      win32u/tests: Move d3dkmt tests from gdi32.
      win32u/tests: Check D3DKMT local handle allocation.
      win32u: Implement d3dkmt local handle allocation.
      winevulkan: Order the win32 structs as other structs.
      winevulkan: Simplify struct conversion enumeration.
      winevulkan: Build the struct extension list lazily.
      winevulkan: Enumerate struct extensions with the registry structs.
      winevulkan: Avoid converting unexposed extensions structs.
      winevulkan: Generate structs for external memory fds.
      winevulkan: Force copy some memory, buffer and image struct chains.
      win32u: Strip unsupported structs from vkAllocateMemory chain.
      win32u: Strip unsupported structs from vkCreateBuffer chain.
      win32u: Strip unsupported structs from vkCreateImage chain.
      win32u: Hook VK_KHR_external_memory_win32 functions.
      winevulkan: Remove leftover external image functions moved to win32u.
      win32u: Pass memory type flags to allocate_external_host_memory.
      win32u: Fix missing NULL initialization of NtAllocateVirtualMemory parameter.
      wintypes/tests: Check IAgileObject interface on map objects.
      wintypes: Introduce a generic IMap stub implementation.
      wintypes: Implement a generic IMap<HSTRING, IInspectable>.
      wintypes: Introduce a locked IMap<HSTRING, IInspectable> implementation.
      winevulkan: Force copying wrapped handle arrays.
      winevulkan: Force copying the VkSubmitInfo(2) struct chains.
      winevulkan: Move command buffer wrapper to vulkan_driver.h.
      win32u: Hook vkQueueSubmit and vkQueueSubmit2 functions.
      win32u: Hook VK_KHR_win32_keyed_mutex related functions.
      win32u: Hook VK_KHR_external_semaphore_win32 related functions.
      win32u: Hook VK_KHR_external_fence_win32 related functions.
      winebus: Match match gamepad dpad buttons with XUSB / GIP.
      win32u: Stub NtGdiDdDDIOpenNtHandleFromName.
      wow64win: Initialize output NtGdiDdDDI* parameters too.
      wow64win: Fix NtGdiDdDDICreateAllocation* output conversion.
      win32u/tests: Test D3DKMT synchronization object creation.
      win32u/tests: Test D3DKMT keyed mutex object creation.
      win32u/tests: Test D3DKMT allocation object creation.
      win32u/tests: Test D3DKMT objects NT handle sharing.
      winevulkan: Simplify device enabled extension conversion.
      winevulkan: Check surface_maintenance1 before using swapchain_maintenance1.
      win32u: Assume the VkPresentInfoKHR struct is copied in the thunks.
      winevulkan: Force copy of the VkSemaphoreSubmitInfo struct chain.
      win32u: Wrap vulkan semaphore objects.
      win32u: Wrap vulkan fence objects.
      d3d8/tests: Flag some tests as todo_wine.
      d3d9/tests: Flag some tests as todo_wine.
      opengl32/tests: Test that window back buffers are shared.
      include: Use the client pointer in debugstr_opengl_drawable.
      win32u: Move memory DC pbuffer handling out of context_sync_drawables.
      win32u: Use context->draw directly when flushing context.
      win32u: Swap the last window drawable if there's no context.
      win32u: Introduce an context_exchange_drawables helper.
      win32u: Rename window opengl drawable to current_drawable.
      win32u: Keep a separate pointer for unused opengl drawable.
      win32u: Don't store the window OpenGL drawables on the DCs.

Santino Mazza (2):
      dinput: Set per monitor aware DPI awareness in the worker thread.
      win32u: Map raw coordinates to virtual screen in low-level hooks.

Spencer Wallace (1):
      msxml3: Correct looping of Document Element properties.

Tim Clem (1):
      dxcore: Use a static structure for the adapter factory.

Tyson Whitehead (2):
      winebus: Unspecified condition blocks are full strength.
      joy.cpl: Play condition effects on indicated axis too.

Vibhav Pant (15):
      opcservices/tests: Add tests for ReadPackageFromStream.
      opcservices/tests: Add some more tests for MoveNext().
      windows.applicationmodel/tests: Add tests for exposing inproc WinRT classes through the manifest.
      opcservices: Mark entries according to compression mode.
      wintypes: Make a copy of the passed string in IPropertyValueStatics::CreateString.
      combase: Add stubs for HSTRING_User{Size, Marshal, Unmarshal, Free}.
      combase/tests: Add tests for HSTRING marshaling methods.
      combase: Implement HSTRING_UserSize.
      combase: Implement HSTRING_UserMarshal.
      combase: Implement HSTRING_User{Unmarshal, Free}.
      combase/tests: Add RoGetAgileReference tests with agile objects.
      combase: Don't marshal objects that implement IAgileObject in RoGetAgileReference.
      wintypes/tests: Add tests for PropertySet::{Insert, Lookup, HasKey}.
      wintypes/tests: Add tests for IMapView.
      wintypes: Introduce a serial to track IMap state changes.

Yuxuan Shui (2):
      opcservices: IOpcPart::GetContentType takes LPWSTR, not BSTR.
      winegstreamer: Free stream buffers before wg_parser_disconnect.

Zhiyi Zhang (4):
      gdi32: Use the maximum number of colours when biClrUsed is zero.
      gdi32/tests: Test recording StretchDIBits() for bitmaps with zero biClrUsed field in EMFs.
      gdi32/tests: Test recording SetDIBitsToDevice() for bitmaps with zero biClrUsed field in EMFs.
      Revert "wineps: Use the correct colours when a monochrome bitmap without a colour table is the source.".

Ziqing Hui (2):
      mf/tests: Test GetOutputStatus for video processor.
      winegstreamer: Add semi-stub implementation for video_processor_GetOutputStatus.

Zowie van Dillen (2):
      opengl32/tests: Add 16-bit bitmap rendering tests.
      winex11: Check pbuffer bit instead of pixmap bit for bitmap rendering.
```
