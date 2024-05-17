The Wine development release 9.9 is now available.

What's new in this release:
  - Support for new Wow64 mode in ODBC.
  - Improved CPU detection on ARM platforms.
  - Removal of a number of obsolete features in WineD3D.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/9.x/wine-9.9.tar.xz>

Binary packages for various distributions will be available
from <https://www.winehq.org/download>

You will find documentation on <https://www.winehq.org/documentation>

Wine is available thanks to the work of many people.
See the file [AUTHORS][1] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/raw/wine-9.9/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 9.9 (total 38):

 - #25009  Password Memory 2010 - Titlebar color rendering error
 - #26407  Shadowgrounds Survivor crashes after viewing the map
 - #26545  Crysis2: Red color on highlights of Bumpmap/Specular Highlights
 - #27745  Racer is unplayable
 - #28192  regedit: The usage message arrives too late in the wine console
 - #29417  Mouse pointer laggy/slow in Dweebs and Dweebs 2 when virtual desktop mode is enabled
 - #31665  Femap unexpected crash on rebuild database (or any command that involves it i.e. import)
 - #32346  Window is too large with Batman and Head Over Heels remakes
 - #39532  Assassin's Creed Unity doesn't run
 - #40248  Some .NET applications throw unhandled exception: System.NotImplementedException: 'System.Management.ManagementObjectSearcher.Get' when using Wine-Mono
 - #44009  Syberia Gog version: crash after cinematics
 - #44625  Cybernoid 2 exits but x window drawing updates are frozen
 - #44863  Performance regression in Prince of Persia 3D
 - #45358  Assassin's Creed Syndicate (AC Unity; AC Odyssey) broken graphics
 - #49674  Feature Request: Restoring previous resolution upon an app crashing
 - #51200  High repaint label volume causes freezing
 - #53197  Total War: Shogun 2 crashes on unimplemented function d3dx11_42.dll.D3DX11LoadTextureFromTexture
 - #55513  Paint.NET 3.5.11 runs unstable on Wine 8.x (and later) because of a bug in Mono
 - #55939  Moorhuhn Director's Cut crashes after going in-game
 - #56000  Window title is not set with winewayland
 - #56422  Exact Audio Copy installer crashes
 - #56429  Applications crash with BadWindow X error
 - #56483  ShellExecute changes in Wine 9.5 broke 64-bit Winelib loading in WoW64 builds
 - #56485  Visual novel RE:D Cherish! displays white screen instead of logo video
 - #56492  Opentrack/TrackIR head tracking broken
 - #56498  Incorrect substring expansion for magic variables
 - #56506  strmbase TRACEs occasionally fail to print floats
 - #56527  Final Fantasy XI Online: Opening movie triggers a 'GStreamer-Video-CRITICAL'.
 - #56579  Setupapi fails to read correct class GUID and name from INF file containing %strkey% tokens
 - #56588  FlatOut 1 display resolution options limited to current desktop resolution using old wow64
 - #56595  Fallout 3 is slow
 - #56607  steam: no tray icon starting with wine 9.2
 - #56615  Spelunky won't start (GLSL version 1.20 is too low; 1.20 is required)
 - #56653  GetLogicalProcessorInformation can be missing Cache information
 - #56655  X11 Driver fails to load
 - #56661  Project Diablo 2 crashes
 - #56671  Disney Ratatouille demo renders upside down on Intel graphics
 - #56682  msvcrt:locale prevents the msvcrt:* tests from running on Windows 7

### Changes since 9.8:
```
Akihiro Sagawa (2):
      quartz/tests: Test read position after MPEG splitter connection.
      winegstreamer: Seek to the end after MPEG splitter connection.

Alex Henrie (1):
      dinput: Don't include every version of DirectInputCreate in every DLL.

Alexandre Julliard (38):
      nls: Update locale data to CLDR version 45.
      winedump: Dump registry scripts resources as text.
      winedump: Dump typelib resources in structured format.
      atl: Remove empty clsid registration.
      dhtmled.ocx: Remove empty clsid registration.
      widl: Don't generate empty interface registrations.
      ntdll: Use null-terminated strings instead of explicit lengths to build SMBIOS data.
      ntdll: Cache the generated SMBIOS data.
      ntdll: Build the SMBIOS table incrementally.
      ntdll: Generate dummy SMBIOS data on non-supported platforms.
      ntdll: Store the CPU vendor and id during detection.
      ntdll: Add processor information to the SMBIOS data.
      configure: Restore warning for missing PE compiler.
      ntdll: Report the correct processor details on ARM platforms.
      ntdll: Update PROCESSOR_ARCHITECTURE variable correctly for ARM platforms.
      wineboot: Create CPU environment keys together with other hardware keys.
      wineboot: Get the CPU model details with NtQuerySystemInformation(SystemCpuInformation).
      wineboot: Report the correct model number on ARM platforms.
      wineboot: Support multiple SMBIOS entries of the same type.
      wineboot: Retrieve CPU details through the SMBIOS table.
      winemac.drv: Remove some no longer used code.
      wbemprox: Get the processor caption from the registry.
      wbemprox: Support multiple SMBIOS entries of the same type.
      wbemprox: Get the processor manufacturer from the SMBIOS table.
      wbemprox: Get the processor name from the SMBIOS table.
      wbemprox: Get the processor count from the SMBIOS table.
      wbemprox: Get the processor id from the SMBIOS table.
      wbemprox: Get a few more processor details from the SMBIOS table.
      wbemprox: Get the processor type and model with SystemCpuInformation.
      wbemprox: Use RtlGetNativeSystemInformation directly to get the correct info on ARM platforms.
      msvcrt/tests: Don't link to _setmaxstdio.
      msvcrt: Update some mangled names on ARM.
      ntdll: Add default values for cache parameters.
      msvcrt: Resync cxx.h with msvcp90.
      msvcrt: Unify __CppXcptFilter implementations.
      msvcrt: Unify __CxxQueryExceptionSize implementations.
      msvcrt: Unify __CxxDetectRethrow implementations.
      msvcrt: Unify checks for valid C++ exception.

Alexandros Frantzis (3):
      win32u: Enable dibdrv wglDescribePixelFormat through p_get_pixel_formats.
      user32/tests: Skip affected keyboard tests if a spurious layout change is detected.
      user32/tests: Remove workaround for SendInput keyboard tests on zh_CN.

Alistair Leslie-Hughes (1):
      include: Add some D3D12_FEATURE_DATA_D3D12_OPTIONS* types.

Anton Baskanov (2):
      winegstreamer: Recognize MFAudioFormat_MPEG and MFAudioFormat_MP3.
      winegstreamer: Add missing format fields to WMA support check.

Billy Laws (2):
      ntdll: Allocate wow64 environment block within the 32-bit address space.
      ntdll: Don't restore PC from LR after unwinding through frame switches.

Biswapriyo Nath (1):
      include: Avoid a C++ keyword.

Brendan McGrath (3):
      winex11.drv: Increment mode_idx in {xrandr10,xf86vm}_get_modes.
      winegstreamer: Pass filename to wg_parser when available.
      mplat/tests: Test Media Source is freed if immediately released.

Brendan Shanks (1):
      winemac.drv: Fix warning in macdrv_get_pixel_formats.

Connor McAdams (4):
      d3dx9/tests: Add a test for negative values in the source rectangle passed to D3DXLoadSurfaceFromMemory.
      d3dx9/tests: Add a new compressed surface loading test.
      d3dx9: Only do direct copies of full blocks for compressed formats.
      d3dx9: Fix destination rectangles passed from D3DXLoadSurfaceFromMemory() to d3dx_load_image_from_memory().

Daniel Lehman (2):
      msvcrt/tests: Add result for Turkish_Türkiye.1254.
      msvcrt/tests: Add tests for _wcsicmp_l.

Dmitry Timoshkov (2):
      kernelbase: Add support for FileDispositionInfoEx to SetFileInformationByHandle().
      user32/tests: Offset child CS_PARENTDC window from parent to make more bugs visible.

Elizabeth Figura (38):
      wined3d: Retrieve the VkCommandBuffer from wined3d_context_vk after executing RTV barriers.
      wined3d: Submit command buffers after 512 draw or dispatch commands.
      ddraw: Use system memory for version 4 vertex buffers.
      ddraw: Upload only the used range of indices in d3d_device7_DrawIndexedPrimitive().
      ddraw/tests: Test GetVertexBufferDesc().
      wined3d: Fix GLSL version comparison.
      maintainers: Change the full form of my name.
      wined3d: Remove the offscreen_rendering_mode setting.
      wined3d: Use wined3d_resource_is_offscreen() directly in ffp_blitter_clear_rendertargets().
      wined3d: Remove no longer used support for drawing to an onscreen render target.
      wined3d: Remove the no longer used render_offscreen field from struct ds_compile_args.
      wined3d: Remove the no longer used Y correction variable.
      d3d9/tests: Clarify and expand point size tests.
      d3d9/tests: Expand test_color_vertex().
      d3d9/tests: Add some tests for default diffuse values.
      d3d9/tests: Test default attribute components.
      wined3d: Remove the GL FFP vertex pipeline.
      wined3d: Remove the GL FFP fragment pipeline.
      wined3d: Remove the NV_register_combiners fragment pipeline.
      wined3d: Remove the ATI_fragment_shader fragment pipeline.
      wined3d: Remove the ARB_fragment_program blitter.
      wined3d: Remove the ARB fragment pipeline.
      wined3d: Remove the ARB shader backend.
      amstream: Link to msvcrt instead of ucrtbase.
      evr: Link to msvcrt instead of ucrtbase.
      qasf: Link to msvcrt instead of ucrtbase.
      qcap: Link to msvcrt instead of ucrtbase.
      qdvd: Link to msvcrt instead of ucrtbase.
      qedit: Link to msvcrt instead of ucrtbase.
      quartz: Link to msvcrt instead of ucrtbase.
      winegstreamer: Link to msvcrt instead of ucrtbase.
      wined3d: Remove no longer used support for emulated non-power-of-two textures.
      wined3d: Remove no longer used support for rectangle textures.
      wined3d: Remove texture non-power-of-two fixup.
      wined3d: Remove the last vestiges of ARB_texture_rectangle support.
      wined3d: Collapse together NPOT d3d_info flags.
      wined3d: Remove the redundant "pow2_width" and "pow2_height" fields.
      wined3d: Fix inversion in shader_get_position_fixup().

Eric Pouech (19):
      cmd/tests: Test using %%0-%%9 as loop variables.
      cmd/tests: Test nested loop variables expansion.
      cmd/tests: Test delayed expansion with spaces in IF and FOR.
      cmd/tests: Test calling batch files named as builtin commands.
      cmd/tests: Test success/failure of commands.
      cmd: Consistenly use the same variable to identify current command.
      cmd: No longer keep track of last element in command list.
      cmd: Introduce xrealloc helper.
      cmd: Use CRT's popen instead of rewriting it.
      cmd: Remove malloc attribute from xrealloc.
      cmd: Introduce CMD_NODE structure.
      cmd: Add helpers to handle list in degenerated binary tree.
      cmd: Move operator from CMD_COMMAND to CMD_NODE.
      msvcrt: Demangle std::nullptr_t datatype.
      winedump: Demangle std::nullptr_t datatype.
      msvcrt: Don't consider end of list encoding as real data types.
      winedump: Don't consider end of list encoding as real data types.
      msvcrt: Correctly unmangle qualified pointer to function/method.
      winedump: Correctly unmangle qualified pointer to function/method.

Esme Povirk (20):
      gdiplus: Add a function to bracket HDC use.
      gdiplus: Bracket HDC use in gdi_alpha_blend.
      gdiplus: Bracket HDC use in alpha_blend_pixels_hrgn.
      gdiplus: Bracket hdc use in brush_fill_path.
      gdiplus: Bracket HDC use in draw_cap.
      gdiplus: Bracket HDC use in draw_poly.
      gdiplus: Bracket HDC use in trace_path.
      gdiplus: Do not use hdc directly in get_graphics_bounds.
      gdiplus: Accept an HDC in get_gdi_transform.
      gdiplus: Fix background key.
      gdiplus: Bitmap stride is ignored when Scan0 is non-NULL.
      gdiplus: Support anchors on thin paths.
      gdiplus: Remove unnecessary math in add_anchor.
      gdiplus: Fix signs on custom line cap rotation in add_anchor.
      gdiplus: Reorder filled arrow cap points to match native.
      gdiplus: Add a test for GdipWidenPath with Custom linecaps.
      gdiplus: Do not create gdi32 objects for Bitmap objects.
      gdiplus: Bracket HDC use in GdipDrawImagePointsRect.
      gdiplus: Bracket HDC use in GDI32_GdipFillPath.
      gdiplus: Bracket HDC use in GDI32_GdipFillRegion.

Fabian Maurer (2):
      win32u: Factor out scroll timer handling.
      win32u: Only set scroll timer if it's not running.

Fotios Valasiadis (1):
      ntdll/unix: Fix building on musl by explicitly including asm/ioctls.h.

Georg Lehmann (1):
      winevulkan: Update to VK spec version 1.3.285.

Hans Leidekker (8):
      odbc32: Forward SQLDataSourcesA() to SQLDataSources() and GetDiagRecA() to GetDiagRec().
      odbc32: Make Unix call parameters Wow64 compatible.
      odbc32: Load_odbc() returns NTSTATUS.
      odbc32: Move replication of ODBC settings to the Unix lib.
      odbc32: Add Wow64 thunks.
      include: Add missing Unicode SQL function declarations.
      odbc32: Don't load libodbc.so dynamically.
      odbc32/tests: Add tests.

Hongxin Zhao (1):
      po: Add a missing \t in the Simplified Chinese translation.

Huw D. M. Davies (3):
      nsi/tests: Use NSI_IP_COMPARTMENT_TABLE instead of hard coding the integer.
      mmdevapi: Remove unused ACImpl typedef.
      maintainers: Remove Andrew Eikum.

Jacek Caban (25):
      winecrt0: Support __os_arm64x_dispatch_call symbol on ARM64EC.
      d3dx9/tests: Remove xfile dumping functionality.
      mshtml: Introduce DISPEX_IDISPATCH_IMPL.
      mshtml: Use DISPEX_IDISPATCH_IMPL macro in htmlnode.c.
      mshtml: Use DISPEX_IDISPATCH_IMPL macro in htmlbody.c.
      mshtml: Use DISPEX_IDISPATCH_IMPL in htmlanchor.c.
      mshtml: Use DISPEX_IDISPATCH_IMPL in htmlarea.c.
      mshtml: Use DISPEX_IDISPATCH_IMPL in htmlattribute.c.
      mshtml: Use DISPEX_IDISPATCH_IMPL in htmlcomment.c.
      mshtml: Use DISPEX_IDISPATCH_IMPL in htmlcurstyle.c.
      mshtml: Use DISPEX_IDISPATCH_IMPL for DocumentType object implementation.
      winebuild: Use .drectve section for exports on ARM64EC.
      mshtml: Use DISPEX_IDISPATCH_IMPL for HTMLElement object implementation.
      mshtml: Use DISPEX_IDISPATCH_IMPL macro in htmlelem.c.
      mshtml: Use DISPEX_IDISPATCH_IMPL macro in htmlelemcol.c.
      mshtml: Use DISPEX_IDISPATCH_IMPL for HTMLEventObj object implementation.
      mshtml: Use DISPEX_IDISPATCH_IMPL macro in htmlevent.c.
      mshtml: Use DISPEX_IDISPATCH_IMPL macro in htmlform.c.
      mshtml: Use DISPEX_IDISPATCH_IMPL macro in htmlframe.c.
      mshtml: Use DISPEX_IDISPATCH_IMPL macro in htmlgeneric.c.
      mshtml: Use DISPEX_IDISPATCH_IMPL macro in htmlhead.c.
      mshtml: Use DISPEX_IDISPATCH_IMPL macro in htmlimg.c.
      mshtml: Use DISPEX_IDISPATCH_IMPL macro in htmlinput.c.
      mshtml: Use DISPEX_IDISPATCH_IMPL macro in htmllink.c.
      mshtml: Use DISPEX_IDISPATCH_IMPL macro in htmllocation.c.

Nikolay Sivov (7):
      d2d1/tests: Add a test for effect output image interface query.
      d2d1/effect: Fix GetImageLocalBounds() prototype.
      d2d1/tests: Add some vertex buffer tests.
      d2d1/effect: Use more generic naming for loaded shaders array.
      d2d1/effect: Add a stub vertex buffer object.
      d2d1/effect: Implement SetPassthroughGraph().
      d2d1/effect: Implement ConnectNode().

Paul Gofman (7):
      xaudio2: Use FAudioVoice_DestroyVoiceSafeEXT() in destroy_voice().
      bcrypt: Support RSA/PKCS1 signatures with unspecified hash algorithm.
      win32u: Update last message time in NtUserGetRawInputBuffer().
      ntdll: Allocate crit section debug info by default for Windows versions before 8.
      opengl32: Prefer formats with depth if unspecified in wglChoosePixelFormat().
      xaudio2/tests: Fix test failures with xaudio2_8 in test_submix().
      nsiproxy.sys: Return success and zero count from ipv6_forward_enumerate_all() if IPV6 is unsupported.

Piotr Caban (4):
      ucrtbase: Handle empty list in variadic template.
      winedump: Handle empty list in variadic template.
      msvcrt/tests: Don't link to _wcsicmp_l and _create_locale.
      ntdll: Don't use gmtime concurrently.

Rémi Bernon (37):
      winex11: Only request display modes driver data when needed.
      win32u: Read / write source modes as a single registry blob.
      win32u: Remove now unnecessary reset_display_manager_ctx.
      win32u: Use struct pci_id in struct gdi_gpu.
      win32u: Remove driver-specific id from struct gdi_gpu.
      win32u: Pass gdi_gpu structure members as add_gpu parameters.
      win32u: Return STATUS_ALREADY_COMPLETE from UpdateDisplayDevices.
      mfreadwrite/tests: Add some source reader D3D11 awareness tests.
      mf/tests: Test video processor D3D11 awareness.
      winegstreamer/video_processor: Implement D3D awareness.
      mfreadwrite/reader: Pass the device manager to the stream transforms.
      winegstreamer: Introduce a new wg_transform_create_mf helper.
      winegstreamer: Introduce a new check_audio_transform_support helper.
      winegstreamer: Introduce a new check_video_transform_support helper.
      mfplat: Fix MFCreateMFVideoFormatFromMFMediaType videoInfo.VideoFlags masks.
      mfplat: Use IMFMediaType_GetBlobSize instead of IMFMediaType_GetBlob.
      mfplat: Introduce a new init_video_info_header helper.
      mfplat: Use media_type_get_uint32 in more places.
      mfplat: Factor AM_MEDIA_TYPE video format allocation.
      mfplat: Set AM_MEDIA_TYPE bTemporalCompression and bFixedSizeSamples.
      mfplat: Implement FORMAT_MPEGVideo for MFInitAMMediaTypeFromMFMediaType.
      mfplat: Implement FORMAT_MPEG2Video for MFInitAMMediaTypeFromMFMediaType.
      mfplat: Implement MFInitMediaTypeFromMPEG1VideoInfo.
      mfplat: Implement MFInitMediaTypeFromMPEG2VideoInfo.
      mfplat: Implement MFCreateVideoMediaType.
      winebus: Don't advertise hidraw devices as XInput compatible.
      winebus: Report whether devices are connected over bluetooth.
      winebus: Move Sony controllers report fixups to PE side.
      mfreadwrite/reader: Shutdown the queue when public ref is released.
      mfplat: Add MFVideoFormat_ABGR32 format information.
      mf/tests: Add video processor tests with MFVideoFormat_ABGR32 format.
      mfreadwrite/tests: Add tests with MFVideoFormat_ABGR32 output format.
      winegstreamer: Support MFVideoFormat_ABGR32 output in the video processor.
      mfreadwrite/reader: Fixup MFVideoFormat_ABGR32 subtype to enumerate the video processor.
      win32u: Set DEVMODEW dmSize field.
      wineandroid: Set DEVMODEW dmSize field.
      winemac: Set DEVMODEW dmSize field.

Tim Clem (7):
      mountmgr.sys: Do not create drive letters or volumes for unbrowsable macOS volumes.
      mountmgr.sys: Do not add drive letters or volumes for macOS volumes without mount paths.
      winemac.drv: Document mode_is_preferred.
      winemac.drv: Prefer display modes that are marked as usable for desktop.
      winemac.drv: Do not consider the "valid" and "safe" flags as making a display mode unique.
      Revert "winemac.drv: Hide app's dock icon when it wouldn't have a taskbar item on Windows.".
      winemac.drv: Exclude dictation when looking for input methods.

Yuxuan Shui (3):
      rtworkq: Avoid use-after-free.
      mfplat/tests: Validate MFCancelWorkItem releases the callback.
      rtworkq: Release cancelled work items.

Ziqing Hui (1):
      winegstreamer/video_decoder: Make output_plane_align specific to h264.
```
