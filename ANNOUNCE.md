The Wine development release 10.3 is now available.

What's new in this release:
  - Clipboard support in the Wayland driver.
  - Initial Vulkan video decoder support in WineD3D.
  - Bundled Compiler-RT library for ARM builds.
  - Header fixes for Winelib C++ support.
  - More progress on the Bluetooth driver.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/10.x/wine-10.3.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-10.3/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 10.3 (total 18):

 - #3930   Miles Sound System (WAIL32.DLL) SuspendThread() deadlock in WINMM callback (silent black screen on HOMM startup)
 - #8532   JawsEditor 2.5/3.0 reports "Invalid imagesize" on startup ('IPicture::SaveAsFile' method too stubby/incorrect)
 - #38879  wbemprox fill_videocontroller calls are expensive
 - #40523  legrand xlpro3 400 : unable to insert a pictogram
 - #41427  [Game Maker Studio - Android] subst.exe is not implemented
 - #45119  Multiple applications from Google sandbox-attacksurface-analysis-tools v1.1.x (targeting native API) need 'ntdll.NtGetNextProcess' implementation
 - #50337  Roland Zenology Pro (VST3 plugin) used with carla-bridge fails to save files
 - #50929  Silver Chains (GOG) crashes and start dumping memory in console
 - #51121  HeidiSQL 11.0.0.5919 shows a blinking black screen and crashes without virtual desktop
 - #52094  IDA Pro 7.6 crashes when loading idapython3.dll
 - #56695  Unreal Engine game checks for a specific VC runtime regkey
 - #57323  Windows 7 Card Games crash on start
 - #57849  Multiple games: sleep accuracy is affected by mouse movement speed
 - #57850  Reolink fails to load dll: err:module:import_dll Loading library libvips-42.dll
 - #57854  Steam.exe fails to start (hangs upon loading) in Wine-10.2
 - #57881  Wine10.2 Noble does not open Quicken 2004, qw.exe file
 - #57899  R-Link 2 Toolbox crash
 - #57903  kernel32:loader - test_export_forwarder_dep_chain fails on Windows 7

### Changes since 10.2:
```
Alexander Morozov (1):
      wineusb.sys: Add support for URB_FUNCTION_VENDOR_ENDPOINT.

Alexandre Julliard (32):
      ntdll: Fix pointer access in read_image_directory().
      tomcrypt: Import code from upstream libtomcrypt version 1.18.2.
      tomcrypt: Import code from upstream libtommath version 1.1.0.
      bcrypt: Use the bundled tomcrypt library for hash algorithms.
      ntdll: Use the bundled tomcrypt for the crc32 implementation.
      rsaenh: Split encrypt and decrypt implementation functions.
      rsaenh: Use the bundled tomcrypt to replace the local copy.
      configure: Check that the 32- and 64-bit builds are using the same libdir.
      ntdll: Add some missing Zw exports.
      apisetschema: Add some new apisets, and old ones for recently added dlls.
      dataexchange: Don't create an empty import lib.
      makefiles: Import winecrt0 by default even with -nodefaultlibs.
      tools: Avoid const strarray pointers in all functions.
      include: Fix some invalid array definitions in PDB structures.
      makefiles: Add support for assembly source files.
      makefiles: Add support for optionally installing external libs.
      libs: Import code from upstream compiler-rt version 8.0.1.
      makefiles: Link to compiler-rt for builds in MSVC mode.
      profapi: Don't create an empty importlib.
      winedump: Fix dumping of empty values in version resources.
      make_unicode: Generate the PostScript builtin font metrics.
      wineps: Remove the old font generation code.
      wineps: Move the generated glyph list to a separate glyphs.c file.
      wineps: Update the glyph list to a more recent version.
      wineps: Generate all core fonts metrics into a single file.
      wineps: Reduce the size of the stored core fonts metrics data.
      png: Import upstream release 1.6.47.
      setupapi: Create fake dlls for missing dlls only if explicitly requested.
      include: Import mmreg.h instead of duplicating definitions.
      winegcc: PE targets always use msvcrt mode, remove redundant checks.
      winegcc: Use -nodefaultlibs also for the linker tests.
      winegcc: Merge the utility functions into the main file.

Alexandros Frantzis (8):
      winewayland: Implement zwlr_data_control_device_v1 initialization.
      winewayland: Support copying text from win32 clipboard to native apps.
      winewayland: Generalize support for exporting clipboard formats.
      winewayland: Support exporting various clipboard formats.
      winewayland: Support copying data from native clipboard to win32 apps.
      winewayland: Normalize received MIME type strings.
      winewayland: Present EGL surfaces opaquely.
      winewayland: Treat fully transparent cursors as hidden.

Alistair Leslie-Hughes (6):
      include: Add Get/SetValue for gdiplus Color class.
      include: Keep the same header order as SDK in gdiplus.h.
      include: Add gdiplusbase.h.
      include: Add a few sal.h defines.
      include: Add stscanf_s define.
      vbscript: Correct resource text for VBSE_PARAMETER_NOT_OPTIONAL.

Arkadiusz Hiler (1):
      krnl386: Silence a warning in GetSystemDirectory16().

Attila Fidan (2):
      winewayland: Update locked pointer position hint.
      winewayland: Implement SetCursorPos via pointer lock.

Bernhard Kölbl (1):
      wine.inf: Add VC runtime version key.

Bernhard Übelacker (11):
      comctl32/tests: Skip a few tests in non-english locale.
      user32/tests: Fix monitor dpi awareness test.
      ntdll/tests: Skip FileFsFullSizeInformationEx test on older windows.
      kernelbase/tests: Skip test if EnumSystemFirmwareTables is not available.
      propsys/tests: Add broken for unexpected value in windows 7.
      d2d1/tests: Adjust tolerance of a few float comparisons in windows 7.
      ws2_32/tests: Allow WSAConnectByNameA test to fail with WSAEOPNOTSUPP.
      propsys/tests: Fix check_PropVariantToBSTR2.
      setupapi: Initialize the files member in SetupDuplicateDiskSpaceListW. (ASan).
      setupapi/tests: Fix message of ok statement.
      kernel32/tests: Add broken in test_IdnToNameprepUnicode.

Brendan McGrath (24):
      mp3dmod/tests: Add tests for selecting an invalid stream.
      mp3dmod: Check for invalid stream.
      mp3dmod/tests: Add tests for GetInputCurrentType.
      mp3dmod/tests: Add tests for GetOutputCurrentType.
      mp3dmod: Implement GetInputCurrentType.
      mp3dmod: Implement GetOutputCurrentType.
      mp3dmod: Fix leak of previous outtype.
      mfplat/tests: Add additional MFCreateWaveFormatExFromMFMediaType tests.
      mfplat: Allow MF_MT_USER_DATA to be missing for all subtypes.
      winedmo: Call avformat_find_stream_info for the mp3 format.
      mp3dmod: Return S_OK in Allocate/Free Resources.
      mp3dmod/tests: Add test for 32-bit sample size.
      mp3dmod: Add support for 32-bit sample size.
      mp3dmod/tests: Test different output sample rates.
      mp3dmod: Return error if requested output format values don't agree.
      mp3dmod: Only allow 1/2 and 1/4 subsampling.
      mp3dmod/tests: Add test for nChannels = 0 on input.
      mp3dmod: Ensure nChannels is greater than zero on input.
      mf/tests: Test different length user_data against aac decoder.
      winegstreamer: Validate the value of cbSize in aac decoder.
      winegstreamer: Expand GST_ELEMENT_REGISTER* defines.
      wined3d: Interpret Y'CbCr values as being from the reduced range.
      mp3dmod: Implement an IMFTransform interface.
      mfsrcsnk: Register the MP3 Byte Stream Handler class.

Brendan Shanks (9):
      include: Add D3DKMT_WDDM_*_CAPS.
      winemac: Remove pre-macOS 10.12 workarounds.
      winemac: Always use notifications to detect window dragging.
      winecoreaudio: Remove pre-macOS 10.12 workaround.
      winegcc: Rename TOOL_ enum constants.
      tools: Use _NSGetExecutablePath to implement get_bindir on macOS.
      server: Use _NSGetExecutablePath to implement get_nls_dir on macOS.
      loader: Use _NSGetExecutablePath to implement get_self_exe on macOS.
      user32: Use the correct DPI in AdjustWindowRect(Ex) when DPI virtualization is active.

Charlotte Pabst (1):
      winex11.drv: Weaken filter_event conditions for some events.

Connor McAdams (11):
      d3dx9/tests: Add some more tests for saving surfaces as targa files.
      d3dx9/tests: Add a test for saving a surface as D3DXIFF_DIB.
      d3dx9/tests: Add tests for saving surfaces to non-DDS files.
      d3dx9: Add TGA prefix to targa specific defines.
      d3dx9: Add basic support for saving surfaces to targa files.
      d3dx9: Add support for selecting a replacement pixel format when saving pixels to a file.
      d3dx9: Add support for saving PNG files in d3dx_save_pixels_to_memory().
      d3dx9: Add support for saving JPG files in d3dx_save_pixels_to_memory().
      d3dx9: Add support for saving BMP files in d3dx_save_pixels_to_memory().
      d3dx9: Add support for saving paletted pixel formats in d3dx_pixels_save_wic().
      d3dx9: Add support for saving DIB files in d3dx_save_pixels_to_memory().

Conor McCarthy (4):
      ntdll/tests: Test NtQueryVolumeInformationFile() with FileFsFullSizeInformationEx.
      kernel32/tests: Test GetDiskSpaceInformationA().
      ntdll: Handle FileFsFullSizeInformationEx in NtQueryVolumeInformationFile().
      kernelbase: Implement GetDiskSpaceInformationA/W().

Daniel Lehman (3):
      dwrite: Fix spelling for Sundanese.
      xml2: Import upstream release 2.12.9.
      xml2: Import upstream release 2.12.10.

Dmitry Timoshkov (7):
      oleaut32/tests: Add some tests for loading and saving EMF using IPicture interface.
      oleaut32: Add support for loading and saving EMF to IPicture interface.
      oleaut32: Implement a better stub for IPicture::SaveAsFile.
      wbemprox: Add support for Win32_PhysicalMemoryArray.
      windowscodecs: Make support for WICBitmapTransformRotate270 more explicit.
      kernel32: SuspendThread() in Win9x mode should return 0 for current thread.
      advapi32: Add CreateProcessWithLogonW() semi-stub.

Elizabeth Figura (27):
      d3dx9/tests: Define D3DX_SDK_VERSION=36 for d3dx9_36.
      d3dx9/tests: Test implicit truncation warnings.
      vkd3d: Import vkd3d-utils.
      d3dcompiler: Use D3DPreprocess() from vkd3d-utils.
      d3dcompiler: Use D3DCompile2VKD3D() from vkd3d-utils.
      d3dx9: Reimplement D3DXCompileShader() for versions before 42.
      d3dx9: Link versions 42 and 43 to the corresponding d3dcompiler DLL.
      wined3d: Use a separate format value for d3d10+ NV12.
      wined3d: Separate a wined3d_texture_vk_upload_plane() helper.
      wined3d: Separate a wined3d_texture_vk_download_plane() helper.
      wined3d: Implement planar Vulkan uploads.
      wined3d: Implement planar Vulkan downloads.
      wined3d: Implement planar Vulkan blits.
      wined3d: Enable KHR_sampler_ycbcr_conversion.
      wined3d: Implement planar NV12 in the Vulkan renderer.
      d3d11/tests: Extend NV12 tests.
      setupapi/tests: Test disk space list APIs.
      setupapi: Get rid of the DISKSPACELIST typedef.
      setupapi: Implement SetupAddToDiskSpaceList().
      setupapi: Correctly implement SetupQuerySpaceRequiredOnDrive().
      d3d11: Implement ID3D11VideoDevice::GetVideoDecoderProfile[Count]().
      wined3d: Introduce a Vulkan decoder backend.
      d3d11: Create a wined3d_decoder object backing the d3d11 decoder object.
      wined3d: Look for a video decode queue.
      wined3d: Create a Vulkan video session backing the wined3d_decoder_vk.
      d3d9/tests: Expand the YUV blit tests a bit.
      ddraw/tests: Port yuv_layout_test() from d3d9.

Eric Pouech (4):
      findstr/tests: Add test for default findstr regex vs text search mode.
      findstr: Set default search mode to regex.
      findstr/tests: Always set text/regex search mode in tests.
      cmd: Open file in 'TYPE' with more sharing attributes.

Esme Povirk (4):
      gdiplus: Stub GdipGetEffectParameters.
      gdiplus/tests: Add tests for effect parameters.
      gdiplus: Split effect parameter size into helper function.
      gdiplus: Store parameters on effect objects.

Etaash Mathamsetty (2):
      ntdll: Implement NtGetNextProcess.
      ntdll/tests: Add NtGetNextProcess tests.

Eugene McArdle (2):
      ntdll/tests: Test updated NtQueryDirectoryFile mask reset behaviour.
      ntdll: Invalidate cached data in NtQueryDirectoryFile when mask is changed.

Floris Renaud (1):
      po: Update Dutch translation.

Giovanni Mascellani (8):
      dxgi: Submit Vulkan presentation as soon as possible.
      dxgi: Set the frame latency even when increasing the frame latency.
      dxgi: Do not bias the frame latency fence.
      dxgi: Directly signal the frame latency fence.
      dxgi: Remove the frame latency fence.
      dxgi: Make the frame latency waitable a semaphore.
      dxgi: Wait on the frame latency semaphore when the client doesn't do it.
      dxgi/tests: Use an explicit frame latency waitable when testing the back buffer index.

Hajo Nils Krabbenhöft (1):
      d2d1: Update DC target surface with current HDC contents on BeginDraw().

Hans Leidekker (7):
      subst: Add basic implementation.
      win32u: Also set DriverVersion for software devices such as llvmpipe.
      wbemprox: Don't use DXGI adapter values for Win32_SoundDevice.
      wbemprox: Get Win32_VideoController values from the registry.
      wmic: Treat VT_I4 values as unsigned.
      wbemprox: Fix format string.
      wbemprox/tests: Avoid test failures on Windows 7.

Haoyang Chen (2):
      comctl32: Fix Alloc/HeapAlloc mismatches.
      cryptnet: Uniform return value that is the same as _wfsopen.

Henri Verbeet (1):
      wined3d: Handle a NULL "push_constants" buffer in glsl_fragment_pipe_alpha_test_func().

Jacek Caban (13):
      winegcc: Don't include host include paths when compiling with MSVCRT.
      include: Use inline function for DeleteFile.
      include: Use wchar_t for platform types on PE targets.
      include: Add UnsignedMultiply128 and _umul128 support.
      include: Add ShiftRight128 and __shiftright128 support.
      include: Use inline functions for CopyFile and MoveFile.
      include: Use struct inheritance for MONITORINFOEX declaration in C++.
      include: Add isnormal C++ declaration.
      include: Introduce minwindef.h header file.
      include: Introduce fibersapi.h header file.
      include: Add missing d3d8types.h defines.
      include: Introduce minwinbase.h header file.
      include: Introduce sysinfoapi.h header file.

Jinoh Kang (10):
      ntdll: Sink module dependency registration towards the end of the function in find_forwarded_export().
      ntdll: Don't re-add a module dependency if it already exists.
      ntdll: Register module dependency for export forwarders regardless of whether the dependency is already loaded.
      ntdll: Properly track refcount on static imports of export forwarders.
      ntdll: Eagerly call process_attach() on dynamic imports of export forwarders.
      ntdll: Explicitly ignore dynamic (GetProcAddress) importers as relay/snoop user module.
      ntdll: Properly track refcount on dynamic imports of export forwarders.
      ntdll: Remove superflous NULL check for importer.
      kernel32/tests: Fix thread handle leak in store_buffer_litmus_test.
      kernel32/tests: Work around Windows 7 issuing DllNotification even when loading with DONT_RESOLVE_DLL_REFERENCES.

Louis Lenders (1):
      ntdll: Silence the noisy FIXME in RtlGetCurrentProcessorNumberEx.

Mark Jansen (2):
      version/tests: Add a test for an empty value in VerQueryValueW.
      kernelbase: Fix VerQueryValueW with no data.

Mohamad Al-Jaf (1):
      makecab: Add stub program.

Nikolay Sivov (24):
      windowscodecs/gif: Store Image descriptor offset when reading GIF data.
      windowscodecs/metadata: Add an option to initialize reader from a memory block.
      windowscodecs/decoder: Separate metadata block reader to a reusable structure.
      windowscodecs/decoder: Add support for metadata block reader at decoder level.
      windowscodecs/decoder: Add support for IWICBitmapDecoder::CopyPalette() in common decoder.
      windowscodecs: Use common decoder for GIF format.
      windowscodecs/decoder: Reuse metadata readers instances.
      windowscodecs/decoder: Implement metadata readers enumerator for the common decoder.
      msxml3/tests: Remove tests that already run for msxml6.
      msxml6/tests: Move some of the SAXXMLReader60 tests.
      rtworkq: Fix private queue id mask check.
      mfplat/tests: Run tests modifying process state in separate processes.
      d2d1/tests: Add some tests for device context handling in the DC target.
      include: Add some flags constants used in pipes API.
      windowscodecs: Implement query strings enumerator.
      windowscodecs/tests: Add some more tests for query enumeration.
      windowscodecs/metadata: Do not decorate 'wstr' items with a type name in returned queries.
      windowscodecs: Implement CreateQueryWriterFromReader().
      windowscodecs/tests: Add some tests for stream position handling when nested readers are used.
      windowscodecs/metadata: Restore original stream position on GetStream().
      windowscodecs/metadata: Replicate original stream position when creating writer instances from readers.
      windowscodecs/tests: Add some tests for metadata handler GetClassID().
      windowscodecs/metadata: Implement GetClassID().
      windowscodecs: Implement GetPreferredVendorGUID().

Paul Gofman (5):
      ntdll: Add stub for RtlDeriveCapabilitySidsFromName().
      ntdll/tests: Add tests for RtlDeriveCapabilitySidsFromName().
      ntdll: Implement RtlDeriveCapabilitySidsFromName().
      kernelbase: Implement DeriveCapabilitySidsFromName().
      kernelbase/tests: Add test for DeriveCapabilitySidsFromName().

Piotr Caban (9):
      msvcr120: Simplify feupdateenv implementation.
      ole32: Mark property storage dirty in PropertyStorage_ConstructEmpty.
      ole32: Mark property storage not dirty after it's saved.
      ole32: Remove end label from PropertyStorage_WriteToStream.
      ole32: Reset output stream when wrting property storage.
      whoami: Return non-zero value on error.
      whoami: Handle arguments starting with '/' and '-'.
      whoami: Parse and validate all command line arguments.
      whoami: Support format specifiers when handling /user argument.

Raphael Riemann (1):
      kernelbase: Add WerRegisterCustomMetadata stub.

Rémi Bernon (29):
      winevulkan: Generate function pointers for required funcs.
      winevulkan: Enable the VK_EXT_headless_surface extension.
      win32u: Pass a vulkan_instance pointer to vulkan_surface_create.
      win32u: Use VK_EXT_headless_surface for nulldrv surface.
      winex11: Initialize window managed flag in create_whole_window.
      winex11: Request managed/embedded in a new window_set_managed helper.
      winex11: Check managed window changes in WindowPosChanged.
      winex11: Pass fullscreen flag to is_window_managed.
      winewayland: Pass fullscreen flag to is_window_managed.
      wined3d: Avoid double-free of swapchain surface on error.
      include: Add a MB_CUR_MAX definition in ctype.h.
      include: Add some _BitScanForward(64) declarations in intrin.h.
      include: Fix InlineIsEqualGUID C++ warning.
      include: Fix wmemchr C++ warning.
      winetest: Avoid underflow when computing filtered output size.
      win32u: Move OpenGL initialization to a separate source.
      win32u: Move dibdrv OpenGL functions to opengl.c.
      win32u: Move OSMesa OpenGL functions to opengl.c.
      win32u: Remove unncessary OSMesa indirections.
      win32u: Guard OpenGL function pointers initialization.
      wineandroid: Remove now unnecessary wine_get_wgl_driver init guard.
      winemac: Remove now unnecessary wine_get_wgl_driver init guard.
      winex11: Remove now unnecessary wine_get_wgl_driver init guard.
      winewayland: Remove now unnecessary wine_get_wgl_driver init guard.
      include: Add IID_PPV_ARGS macro.
      include: Add QueryDisplayConfig declaration.
      include: Add d3d8 interface GUIDs.
      include: Add D3D9 interface GUIDs.
      include: Fix D3DDEVINFO_D3DRESOURCEMANAGER type name.

Sebastian Lackner (1):
      oleaut32: Implement SaveAsFile for PICTYPE_ENHMETAFILE.

Tim Clem (1):
      wbemprox: Add Manufacturer and Speed to Win32_PhysicalMemory.

Vadim Kazakov (1):
      include: Add definition of NEGOSSP_NAME.

Vibhav Pant (15):
      winebth.sys: Store known devices per radio from org.bluez.Device1 objects on BlueZ.
      winebth.sys: Add a basic implementation for IOCTL_BTH_GET_DEVICE_INFO.
      winebth.sys: Add connection related properties for remote devices.
      winebth.sys: Queue a DEVICE_ADDED event on receiving InterfacesAdded for objects that implement org.bluez.Device1.
      winebth.sys: Remove the corresponding device entry for Bluetooth radios on receiving InterfacesRemoved for org.bluez.Device1 objects.
      winebth.sys: Use the correct DBus property name in IOCTL_WINEBTH_RADIO_SET_FLAG.
      winebth.sys: Initially set numOfDevices to 0 in IOCTL_BTH_GET_DEVICE_INFO.
      winebth.sys: Don't iterate over the remaining radios once a local device has been removed.
      winebth.sys: Use the "Name" property of a BlueZ adapter for the local radio name.
      winebth.sys: Use the "Trusted" property from BlueZ device objects to set BDIF_PERSONAL.
      winebth.sys: Set the device class for remote devices from BlueZ's "Class" property.
      winebth.sys: Only set the updated properties for local radios on BLUETOOTH_WATCHER_EVENT_TYPE_RADIO_PROPERTIES_CHANGED.
      winebth.sys: Update properties for tracked remote devices on receiving PropertiesChanged for org.bluez.Device1 objects from BlueZ.
      ws2_32: Implement WSAAddressToString() for Bluetooth (AF_BTH) addresses.
      ws2_32/tests: Add tests for Bluetooth addresses for WSAStringToAddress().

William Horvath (2):
      server: Use a high precision timespec directly for poll timeouts on supported platforms.
      server: Use epoll_pwait2 for the main loop on Linux.

Yuxuan Shui (1):
      winegstreamer: Avoid large buffer pushes in wg_transform.

Zhiyi Zhang (8):
      imm32/tests: Test that the IME UI window shouldn't be above normal windows at creation.
      imm32: Move the IME UI window to the bottom at creation.
      d3d11/tests: Test that the fallback device window shouldn't be above normal windows at creation.
      dxgi: Move the fallback device window to the bottom at creation.
      winex11.drv: Allow MWM_FUNC_MAXIMIZE when WS_MAXIMIZE is present.
      win32u: Use the normal window rectangle to find monitor when a window is minimized.
      win32u: Remove an unused parameter.
      win32u: Don't use the current mode in the registry if it's a detached mode.

Ziqing Hui (4):
      mfreadwrite/tests: Move writer creation tests to test_sink_writer_create.
      mfreadwrite/tests: Test getting transforms and media sinks from writer.
      mfreadwrite/tests: Test AddStream and SetInputMediaType for writer.
      mfreadwrite/tests: Test sample processing for writer.
```
