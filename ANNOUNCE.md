The Wine development release 9.6 is now available.

What's new in this release:
  - Support for advanced AVX features in register contexts.
  - More Direct2D effects work.
  - Support for RSA OAEP padding in BCrypt.
  - Interpreted mode fixes in WIDL.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/9.x/wine-9.6.tar.xz>

Binary packages for various distributions will be available
from <https://www.winehq.org/download>

You will find documentation on <https://www.winehq.org/documentation>

Wine is available thanks to the work of many people.
See the file [AUTHORS][1] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/raw/wine-9.6/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 9.6 (total 18):

 - #20694  Mozart 10/11: Cannot save gif jpg or tiff, png + bmp are empty, emf only partial.
 - #26372  TI-83 Plus Flash Debugger buttons are not visible
 - #28170  "Text Service and Input Languages" needs unimplemented function USER32.dll.LoadKeyboardLayoutEx called
 - #41650  SolidWorks 2016 crashes on launch
 - #43052  ChessBase 14 - crashes right after start
 - #46030  Trackmania Unlimiter 1.3.x for TrackMania United Forever 2.11.26 crashes at account selection screen (heap manager differences, incorrect assumptions of mod on internal game engine data structures)
 - #47170  nProtect GameGuard Personal/Anti-Virus/Spyware 3.x/4.x kernel drivers crash due to 'winedevice' PE module having no export table
 - #49089  nProtect Anti-Virus/Spyware 4.0 'tkpl2k64.sys' crashes on unimplemented function 'fltmgr.sys.FltBuildDefaultSecurityDescriptor'
 - #50296  Multiple 32-bit applications fail due to incorrect handling of 'HKLM\\Software\\Classes' key in 64-bit WINEPREFIX (shared key under Windows 7+ WOW64)(Autocad 2005)
 - #53810  [Regression] Visual Novel Shin Koihime Eiyuutan crashes after opening movie plays
 - #54378  VrtuleTree calls unimplemented function ntoskrnl.exe.ExGetPreviousMode
 - #56188  d2d1:d2d1 frequently fails on the GitLab CI
 - #56376  Nerf Arena Blast Demo only displays a black screen
 - #56394  Final Fantasy XI Online: Mouse/pointer cursor activates at unintended times.
 - #56438  Multiple games have texture glitches (Iron Harvest, The Hong Kong Massacre)
 - #56469  configure incorrectly setting ac_cv_lib_soname_vulkan value on macOS
 - #56487  wshom tests timeout on Wine
 - #56503  CryptStringToBinary doesn't adds CR before pad bytes in some cases

### Changes since 9.5:
```
Aida Jonikienė (2):
      wbemprox: Stub most of the Win32_VideoController properties.
      sapi: Only print GetStatus() FIXME once.

Alexandre Julliard (21):
      ntdll: Copy the context contents instead of the pointer on collided unwind.
      ntdll: Use a common wrapper to call exception handlers on x86-64.
      ntdll: Use a common wrapper to call unwind handlers on x86-64.
      wshom.ocx: Don't show a message box on ShellExecute error.
      widl: Don't skip a pointer level for pointers to pointers to strings.
      include: Fix a parameter type in the IEnumTfUIElements interface.
      idl: Use IPSFactoryBuffer instead of a non-existent IFactoryBuffer.
      widl: Merge interpreted stubs header and parameters output into a single function.
      widl: Don't output the explicit handle argument.
      widl: Fix method number for call_as functions.
      rpcrt4: Return the correct failure for a NULL binding handle.
      rpcrt4: Make sure that the stack is set when catching an exception.
      msi/tests: Delete the temp .msi file in all failure cases.
      widl: Always use new-style format strings in interpreted mode.
      widl: Add /robust flags in correlation descriptors.
      widl: Output more correct /robust flags.
      widl: Use the correct type for non-encapsulated union discriminants.
      widl: Always pass the top-level attributes to detect interface pointers.
      widl: Use padding instead of alignment in structure format strings.
      widl: Clear /robust flags when no descriptor is present.
      widl: Map VT_USERDEFINED to VT_I4 for default values in typelibs.

Alistair Leslie-Hughes (6):
      fltmgr.sys: Implement FltBuildDefaultSecurityDescriptor.
      fltmgr.sys: Create import library.
      ntoskrnl/tests: Add FltBuildDefaultSecurityDescriptor test.
      include: Add STORAGE_HOTPLUG_INFO structure.
      include: Add some DWMWA_* enum values.
      include: Add more DBCOLUMN_ defines.

Aurimas Fišeras (2):
      po: Update Lithuanian translation.
      po: Update Lithuanian translation.

Brendan Shanks (10):
      mmdevapi: Make IMMDeviceCollection immutable after creation.
      ntdll: Simplify creation of the server directory.
      server: Simplify creation of the server directory.
      server: Replace some malloc/sprintf/strcpy calls with asprintf.
      server: Replace sprintf with snprintf to avoid deprecation warnings on macOS.
      server: Clarify that registry files are always in the current directory, and simplify save_branch().
      win32u: Enlarge buffer size in _CDS_flags.
      win32u: Enlarge buffer size in format_date.
      win32u: Use PATH_MAX for Unix paths instead of MAX_PATH (from Win32).
      win32u: Replace sprintf with snprintf to avoid deprecation warnings on macOS.

Dmitry Timoshkov (6):
      windowscodecs: Silence fixme for IID_CMetaBitmapRenderTarget.
      widl: Use generic 'struct sltg_data' for collecting data.
      widl: Add more traces.
      widl: Add support for structures.
      widl: Properly align name table entries.
      widl: Add support for VT_USERDEFINED to SLTG typelib generator.

Eric Pouech (4):
      evr: Remove useless code.
      evr: Fix YUY2 image copy in evr_copy_sample_buffer().
      cmd: Add test for substring handling in 'magic' variable expansion.
      cmd: Fix substring expansion for 'magic' variables.

Esme Povirk (1):
      user32/tests: Revert test updates for win11.

Hans Leidekker (6):
      bcrypt: Add support for RSA OAEP padding.
      wbemprox: Zero-initialize table data.
      wmic: Make an error message more general.
      wmic: Strip spaces once.
      wmic: Handle failure from CommandLineToArgvW().
      wmic: Handle multiple properties separated by whitespace.

Jacek Caban (2):
      ntdll: Use __ASM_GLOBAL_IMPORT for RtlUnwind.
      widl: Don't use old typelib format in do_everything mode.

Louis Lenders (2):
      wmic: Support interactive mode and piped commands.
      wbemprox: Add property 'Status' to Win32_BIOS.

Nikolay Sivov (22):
      dwrite/shape: Fully initialize shaping context when getting glyph positions (Valgrind).
      d2d1: Fix a double free on error path (Valgrind).
      gdi32/emf: Zero-initialize handles array (Valgrind).
      d3d10: Use older compiler for D3D10CompileEffectFromMemory().
      d3d10/tests: Add a test for effect compilation containing empty buffers.
      d2d1/tests: Get rid of test shader blobs.
      d2d1/effect: Use effect property types identifier directly in initializers.
      d2d1/effect: Use a helper internally instead of IsShaderLoaded().
      d2d1/tests: Add a test for a custom effect using a pixel shader.
      d2d1/tests: Add a basic test for ID2D1DrawInfo instance.
      d2d1/tests: Add a test for transform graph input count.
      d2d1/effect: Keep input count as a graph property when recreating it.
      d2d1/effect: Implement offset transform object.
      d2d1/effect: Implement blend transform object.
      d2d1/effect: Implement node list for the transform graph.
      d2d1/effect: Implement ConnectToEffectInput().
      d2d1/effect: Implement SetOutputNode().
      d2d1/effect: Implement SetSingleTransformNode().
      d2d1/effect: Fix property value size for empty or missing string values.
      d2d1/effect: Implement border transform object.
      d2d1/effect: Implement bounds adjustment transform.
      d2d1/tests: Add supported interface checks for transform objects.

Paul Gofman (23):
      ntdll: Only save AVX xstate in wine_syscall_dispatcher.
      ntdll: Preserve untouched parts of xstate in NtSetContextThread().
      ntdll: Support generic xstate in Unix-side manipulation functions.
      ntdll: Factor out xstate_from_server() function.
      ntdll: Factor out context_to_server() function.
      ntdll: Mind generic xstate masks in server context conversion.
      ntdll: Support more xstate features.
      ntdll/tests: Add more tests for xstate.
      windows.perception.stub: Add stub IHolographicSpaceInterop interface.
      wintypes: Report some API contracts as present in api_information_statics_IsApiContractPresentByMajor().
      ddraw: Store material handles in the global table.
      ddraw: Store surface handles in the global table.
      ddraw: Store matrix handle in the global table.
      ddraw: Don't apply state in ddraw_surface_blt().
      wined3d: Factor out wined3d_texture_set_lod() function.
      ddraw: Store wined3d state in d3d_device.
      ddraw: Sync wined3d render target in d3d_device_sync_rendertarget().
      ddraw: Support multiple devices per ddraw object.
      ddraw/tests: Add tests for multiple devices.
      win32u: Avoid writing past allocated memory in peek_message().
      win32u: Avoid leaking previous buffer in get_buffer_space().
      ddraw: Don't demand WINED3D_BIND_SHADER_RESOURCE for making surface in vidmem.
      avifil32: Update lCurrentFrame in IGetFrame_fnGetFrame().

Piotr Caban (1):
      gdi32: Implicitly call StartPage in ExtEscape on printer DC.

Rémi Bernon (35):
      configure: Quote ac_cv_lib_soname_MoltenVK when setting SONAME_LIBVULKAN.
      mf/tests: Check that pan scan and geometric apertures are set.
      evr/tests: Split create_d3d_sample to a separate helper.
      evr/tests: Split check_presenter_output to a separate helper.
      evr/tests: Add more video mixer input media type aperture tests.
      evr/mixer: Respect input media type MF_MT_GEOMETRIC_APERTURE.
      win32u: Open adapters in NtGdiDdDDIEnumAdapters2 outside of the display devices lock.
      winex11: Initialize D3DKMT vulkan instance only once.
      winex11: Introduce a new get_vulkan_physical_device helper.
      winex11: Introduce a new find_adapter_from_handle helper.
      win32u: Move D3DKMT vulkan implementation out of winex11.
      winegstreamer: Set GST_DEBUG if not set, based on WINEDEBUG channels.
      win32u: Split writing adapter to registry to a separate helper.
      win32u: Use a symlink for the logically indexed adapter config key.
      win32u: Use named adapters instead of struct gdi_adapter.
      win32u: Rename struct adapter to struct source.
      winevulkan: Strip surface extensions in vkEnumerateInstanceExtensionProperties.
      winevulkan: Remove now unnecessary vkEnumerateInstanceExtensionProperties driver entry.
      winevulkan: Introduce a new get_host_surface_extension driver entry.
      winevulkan: Remove now unnecessary vkCreateInstance driver entry.
      winevulkan: Remove now unnecessary vkDestroyInstance driver entry.
      winegstreamer: Create the transform parsed caps from wg_format.
      winegstreamer: Fallback to input caps only when no parser was found.
      mf/session: Avoid leaking samples in transform_node_deliver_samples.
      mfreadwrite/reader: Use MFTEnumEx to enumerate stream transforms.
      mfreadwrite/reader: Make the GetTransformForStream category parameter optional.
      mfreadwrite/tests: Test the source reader stream change events.
      mfreadwrite/tests: Test the D3D awareness of source reader transforms.
      mfreadwrite/reader: Avoid accessing an invalid stream index.
      mfreadwrite/reader: Keep the output subtypes when propagating media types.
      winex11: Keep the client window colormap on the GL drawable.
      winex11: Get rid of ref held from the HWND to its Vk surface.
      winex11: Introduce a new detach_client_window helper.
      winex11: Introduce a new destroy_client_window helper.
      winex11: Introduce a new attach_client_window helper.

Santino Mazza (1):
      crypt32: Fix CryptBinaryToString not adding a separator.

Tuomas Räsänen (1):
      winebus: Ignore udev events of devices which do not have devnodes.

Zebediah Figura (11):
      d3d11/tests: Test discarding a buffer in test_high_resource_count().
      wined3d: Recreate buffer textures when renaming the existing buffer storage.
      wined3d: Skip bindless samplers with no uniform location.
      wined3d: Enable EXT_extended_dynamic_state2.
      wined3d: Use dynamic state for primitive type if possible.
      wined3d: Use dynamic state for patch vertex count if possible.
      wined3d: Require EXT_framebuffer_object.
      wined3d: Remove the OffscreenRenderingMode setting.
      wined3d: Require GLSL 1.20 support.
      wined3d: Remove the "arb" and "none" shader_backend options.
      wined3d: Require ARB_texture_non_power_of_two.

Zhiyi Zhang (6):
      mfmediaengine: Implement IMFMediaEngineEx::SetCurrentTime().
      mfmediaengine: Implement IMFMediaEngineEx::SetCurrentTimeEx().
      mfmediaengine/tests: Test IMFMediaEngineEx::SetCurrentTime/Ex().
      gdiplus/tests: Add tests for GdipPrivateAddMemoryFont().
      gdiplus: Search microsoft platform names first in load_ttf_name_id().
      winemac.drv: Show the window after setting layered attributes.
```
