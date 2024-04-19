The Wine development release 9.7 is now available.

What's new in this release:
  - Build system support for ARM64X.
  - Some restructuration of the Vulkan driver interface.
  - WIDL improvements for ARM support as well as SLTG typelibs.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/9.x/wine-9.7.tar.xz>

Binary packages for various distributions will be available
from <https://www.winehq.org/download>

You will find documentation on <https://www.winehq.org/documentation>

Wine is available thanks to the work of many people.
See the file [AUTHORS][1] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/raw/wine-9.7/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 9.7 (total 18):

 - #37246  Old C&C titles freeze after the map is loaded.
 - #44699  Clang 6.0 fails to run under wine
 - #44812  Multiple applications need NtQueryInformationProcess 'ProcessQuotaLimits' class support (MSYS2, ProcessHacker 2.x)
 - #48080  Oregon Trail II will not start in 32-bit mode
 - #50111  osu! crashes since 20201110 version with wine-mono (needs native -> managed byref array marshalling)
 - #54759  Notepad++: slider of vertical scrollbar is too small for long files
 - #54901  Medieval II Total War some units partly invisible with d3dx9_30 as builtin
 - #55765  The 32-bit d2d1:d2d1 frequently crashes on the GitLab CI
 - #56133  explorer.exe: Font leak when painting
 - #56361  Geovision Parashara's Light (PL9.exe) still crashes in wine
 - #56369  Advanced IP Scanner crashes on unimplemented function netapi32.dll.NetRemoteTOD
 - #56442  Totem Arts Launcher.exe garbled text
 - #56491  Videos in BURIKO visual novel engine
 - #56493  PresentationFontCache.exe crashes during .Net 3.51 SP1  installation
 - #56536  UI: Applications using ModernWPF crash, Windows.Ui.ViewManagment.InputPane.TryShow not implemented
 - #56538  Mspaint from Windows XP needs imm32.CtfImmIsCiceroEnabled
 - #56551  HP System Diagnostics crashes when clicking the Devices tab
 - #56554  ON1 photo raw installs but wont run the application

### Changes since 9.6:
```
Alex Henrie (2):
      win32u: Also reset the unnamed values in prepare_devices.
      explorer: Fix font handle leaks in virtual desktop.

Alexandre Julliard (16):
      winhttp/tests: Fix some compiler warnings with non-PE build.
      widl: Pass 16-byte structures by value on ARM64.
      widl: Only pass power of 2 structures by value on x86-64.
      widl: Correctly align stack parameters on ARM.
      widl: Don't use a structure for NdrClientCall2 parameters on ARM.
      widl: Output register parameter assignments on ARM platforms.
      widl: Clear RobustEarly flag also for data structure conformance.
      rpcrt4/tests: Add some more parameter passing tests.
      rpcrt4: Export NdrpClientCall2.
      rpcrt4: Implement NdrClientCall2 in assembly on all platforms.
      rpcrt4: Implement NdrClientCall3 in assembly on all platforms.
      rpcrt4: Implement NdrAsyncClientCall in assembly on all platforms.
      rpcrt4: Implement Ndr64AsyncClientCall in assembly on all platforms.
      rpcrt4: Support calling server functions with floating point arguments on ARM platforms.
      widl: Use x64 calling convention for ARM64EC.
      rpcrt4: Leave some space on the stack for varargs when called from ARM64EC code.

Alfred Agrell (8):
      winegstreamer: Create decodebin instead of avidemux.
      winegstreamer: Use decodebin instead of wavparse.
      winegstreamer: Delete now-meaningless wg_parser_type enum.
      winegstreamer: Reduce CLSID_decodebin_parser merit, so the MPEG splitter is chosen instead for MPEGs.
      quartz/tests: Test that compressed formats are offered for MPEGs.
      quartz: Fix error code on empty filename.
      quartz/tests: Test the new error codes.
      quartz: Fix memory leak on failure path.

Brendan Shanks (4):
      winecoreaudio: Correctly handle devices whose UID contains non-ASCII characters.
      dxgi: Add IDXGISwapChain2 stubs for D3D11.
      dxgi: Add IDXGISwapChain3 stubs for D3D11.
      dxgi: Add IDXGISwapChain4 stubs for D3D11.

Dmitry Timoshkov (4):
      widl: In old style typelibs all types are public.
      widl: Add support for recursive type references to SLTG typelib generator.
      widl: Add support for interfaces to SLTG typelib generator.
      widl: Add support for inherited interfaces to SLTG typelib generator.

Eric Pouech (10):
      msvcrt/tests: Add tests for check buffering state of standard streams.
      ucrtbase/tests: Add tests for checking buffering state of standard streams.
      ucrtbase: Let stderr be always be unbuffered.
      dbghelp: Test thread names in minidump.
      dbghelp: Store thread names in minidump.
      winedump: Add helpers to print DWORD64 integers.
      winedump: Simplify unicode string printing (minidump).
      winedump: Support a couple of 'section' options (minidump).
      winedump: Tidy up dumping minidump.
      winedump: Extend dumping of some minidump parts.

Fabian Maurer (1):
      imm32: Add CtfImmIsCiceroEnabled stub.

Hans Leidekker (4):
      msi/tests: Use the helpers from utils.h in more modules.
      msi/tests: Try restarting tests elevated.
      msi/tests: Get rid of workarounds for old Windows versions.
      msi: Install global assemblies before running deferred custom actions.

Isaac Marovitz (1):
      ntdll: Implement NtQueueApcThreadEx().

Jacek Caban (5):
      winebuild: Add -marm64x option for generating hybrid ARM64X import libraries.
      winegcc: Pass target and resources as arguments to build_spec_obj.
      winegcc: Add -marm64x option for linking ARM64X images.
      makedep: Use hybrid ARM64X images for ARM64EC.
      ntdll: Use mangled function names in ARM64EC assembly.

Jinoh Kang (3):
      kernel32/tests: Test module refcounting with forwarded exports.
      kernel32/tests: Fix argument order in subtest_export_forwarder_dep_chain() calls.
      kernel32/tests: Document which fields may be overwritten later in gen_forward_chain_testdll().

Liam Middlebrook (1):
      krnl386.exe: Start DOSVM timer on GET_SYSTEM_TIME.

Mohamad Al-Jaf (1):
      windows.ui: Return success in IInputPane2::TryShow().

Nikolay Sivov (6):
      d2d1/effect: Zero value buffer on size or type mismatch.
      d2d1/tests: Use distinct types for vector and matrix values.
      d2d1/effect: Make it possible to set render info for draw-transform nodes.
      d2d1: Implement GetEffectProperties().
      d2d1/effect: Add 'Rect' property to the Crop effect description.
      d2d1: Fix GetPropertyNameLength() return value.

Paul Gofman (9):
      winhttp: Set actual default receive response timeout to 21sec.
      winhttp/tests: Add some tests for actual connection caching.
      winhttp: Do not cache connection if suggested by request headers.
      wow64: Support generic xstate in call_user_exception_dispatcher().
      ntdll: Preserve untouched parts of xstate in set_thread_wow64_context().
      winegstreamer: Try to handle broken IStream_Stat implementation in WM reader OpenStream().
      mmdevapi: Implement SAC_IsAudioObjectFormatSupported().
      winegstreamer: Destroy wg_transform in video_decoder/transform_SetInputType().
      ntdll: Return STATUS_NO_YIELD_PERFORMED from NtYieldExecution() on Linux if no yield was performed.

Piotr Caban (1):
      gdiplus: Fix IWICBitmapFrameDecode reference leak in decode_frame_wic.

Rémi Bernon (27):
      include: Add MFTOPOLOGY_DXVA_MODE enum definition.
      mf/tests: Fix printf format warning.
      mf/tests: Split topology loader tests to a new file.
      mf/tests: Test D3D awareness handling in the topology loader.
      mf/tests: Test device manager handling in the topology loader.
      mfmediaengine/tests: Test that effects allow converters between them.
      mfmediaengine: Allow decoder / converter to be resolved between topology nodes.
      winevulkan: Add a manual vkQueuePresent wrapper.
      winevulkan: Move vkQueuePresent FPS trace out of the drivers.
      winevulkan: Return VK_SUBOPTIMAL_KHR from vkQueuePresentKHR after resize.
      winevulkan: Return VK_SUBOPTIMAL_KHR from vkAcquireNextImage(2)KHR after resize.
      winevulkan: Remove unnecessary vkGetSwapchainImagesKHR driver entry.
      winevulkan: Pass surface info for each vkQueuePresent swapchain to win32u.
      win32u: Introduce a new vulkan_surface_presented driver entry.
      winewayland: Remove now unnecessary swapchain extents checks.
      winewayland: Remove now unnecessary swapchain wrapper.
      win32u: Move vkQueuePresent implementation out of the user drivers.
      winevulkan: Remove now unnecessary vkDestroySwapchain driver entry.
      winevulkan: Remove now unnecessary vkCreateSwapchainKHR driver entry.
      win32u: Close device manager source key in write_source_to_registry.
      win32u: Remove fake source creation when adding display mode.
      win32u: Keep the primary current mode in the device manager context.
      win32u: Add all display device source modes at once.
      winegstreamer: Introduce a new set_sample_flags_from_buffer helper.
      winegstreamer: Introduce a new sample_needs_buffer_copy helper.
      winegstreamer: Split read_transform_output_data in two helpers.
      winegstreamer: Pass optional GstVideoInfo to read_transform_output_video.

Sam Joan Roque-Worcel (1):
      win32u: Make SCROLL_MIN_THUMB larger.

Tim Clem (1):
      winemac.drv: Hide app's dock icon when it wouldn't have a taskbar item on Windows.

Vijay Kiran Kamuju (4):
      mscms: Add stub for WcsGetDefaultColorProfile.
      ntdll: Add NtQueryInformationProcess(ProcessQuotaLimits) stub.
      ntdll/tests: Add NtQueryInformationProcess(ProcessQuotaLimits) tests.
      netapi32: Add NetRemoteTOD stub.

Zebediah Figura (4):
      wined3d: Enable EXT_extended_dynamic_state3.
      wined3d: Use dynamic state for multisample state if possible.
      wined3d: Use dynamic state for blend state if possible.
      wined3d: Use dynamic state for rasterizer state if possible.

Zhiyi Zhang (4):
      opengl32/tests: Add default framebuffer tests.
      winemac.drv: Update OpenGL context immediately after the window content view is visible.
      uxtheme/tests: Test that scrollbar parts should have very few #ffffff pixels.
      light.msstyles: Use #fefefe instead of #ffffff for scrollbar parts.

Ziqing Hui (3):
      winegstreamer: Merge audio_mpeg1 into audio field.
      winegstreamer: Merge audio_mpeg4 into audio field.
      winegstreamer: Merge audio_wma into audio field.
```
