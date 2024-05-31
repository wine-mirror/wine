The Wine development release 9.10 is now available.

What's new in this release:
  - Bundled vkd3d upgraded to version 1.12.
  - DPI Awareness support improvements.
  - C++ RTTI support on ARM platforms.
  - More obsolete features removed in WineD3D.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/9.x/wine-9.10.tar.xz>

Binary packages for various distributions will be available
from <https://www.winehq.org/download>

You will find documentation on <https://www.winehq.org/documentation>

Wine is available thanks to the work of many people.
See the file [AUTHORS][1] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/raw/wine-9.10/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 9.10 (total 18):

 - #23434  Race management software hangs & jumps up to 100% processor load
 - #34708  Silent Hill 4: The Room crashes after first videoscene when trying to go to the door.
 - #45493  SRPG Studio games need proper DISPATCH_PROPERTYPUTREF implementation
 - #46039  Paint.NET 4.1 (.NET 4.7 app) installer tries to run MS .NET Framework 4.7 installer (Wine-Mono only advertises as .NET 4.5)
 - #46787  Notepad++ rather slow (GetLocaleInfoEx)
 - #50196  can not copy words between wine apps and ubuntu apps
 - #50789  Multiple .NET applications crash with unimplemented 'System.Security.Principal.WindowsIdentity.get_Owner' using Wine-Mono (Affinity Photo 1.9.1, Pivot Animator 4.2)
 - #52691  FL Studio 20.9.1 Freezes on start-up
 - #54992  EA app launcher does not render correctly
 - #56548  reMarkable crashes on start
 - #56582  vb3 combobox regression: single click scrolls twice
 - #56602  DualShock 4 controller behaves incorrectly on Darwin with hidraw enabled
 - #56666  BExAnalyzer from SAP 7.30 does not work correctly
 - #56674  Multiple games fail to launch (Far Cry 3, Horizon Zero Dawn CE, Metro Exodus)
 - #56718  Compilation fails on Ubuntu 20.04 with bison 3.5.1
 - #56724  New chromium versions don't start under wine anymore
 - #56730  Access violation in riched20.dll when running EditPad
 - #56736  App packager from Windows SDK (MakeAppx.exe) 'pack' command crashes on unimplemented function ntdll.dll.RtlLookupElementGenericTableAvl

### Changes since 9.9:
```
Aida Jonikienė (2):
      dxdiagn: Add bIsD3DDebugRuntime property.
      dxdiagn: Add AGP properties.

Alexandre Julliard (52):
      msvcrt: Add helpers to abstract RVA accesses to RTTI data.
      msvcrt: Unify __RTtypeid implementation.
      msvcrt: Unify __RTDynamicCast implementation.
      msvcrt: Unify _CxxThrowException implementation.
      msvcrt: Unify _is_exception_typeof implementation.
      msvcrt: Unify __ExceptionPtrCopyException implementation.
      msvcrt: Unify exception_ptr_from_record implementation.
      msvcrt: Unify call_copy_ctor/call_dtor implementations.
      msvcp: Unify __ExceptionPtrCopyException implementation.
      msvcp: Unify __ExceptionPtrCurrentException implementation.
      msvcp: Unify call_copy_ctor/call_dtor implementations.
      msvcrt: Use RVAs in rtti and exception data on all platforms except i386.
      msvcp: Use RVAs in rtti and exception data on all platforms except i386.
      msvcrt/tests: Use function pointers to bypass builtin malloc/realloc.
      msvcp/tests: Fix mangled names on ARM.
      winecrt0: Initialize the Unix call dispatcher on first use.
      ntdll: Make __wine_unix_call() an inline function.
      wbemprox: Avoid unused function warning.
      kernelbase: Fix the name of the default system locale.
      wbemprox: Don't reference yysymbol_name on older bisons.
      conhost: Fix a printf format warning.
      winegcc: Don't print a potentially reallocated pointer.
      adsldpc: Add correct C++ mangled names for all platforms.
      dxtrans: Add correct C++ mangled names for all platforms.
      msmpeg2vdec: Add correct C++ mangled names for all platforms.
      vssapi: Add correct C++ mangled names for all platforms.
      msvcrt: Fix bad_cast_copy_ctor spec entry on ARM.
      msvcrt: Export all _ConcRT functions also on ARM.
      msvcp120_app: Fix a typo in a C++ mangled name.
      msvcp: Replace some stubs by exported functions that already exist for other platforms.
      msvcp: Export the thiscall version of ios_base_Tidy.
      msvcp140: Sort entry points by function instead of platform.
      msvcp: Only export thiscall functions on i386.
      msvcp: Only export stubs of thiscall functions on i386.
      msvcrt: Only export thiscall functions on i386.
      msvcrt: Only export stubs of thiscall functions on i386.
      msvcrt: Add missing C++ mangled names for ARM.
      msvcirt: Add missing C++ mangled names for ARM.
      msvcp60: Add missing C++ mangled names for ARM.
      msvcp70: Add missing C++ mangled names for ARM.
      msvcp71: Add missing C++ mangled names for ARM.
      msvcp80: Add missing C++ mangled names for ARM.
      msvcp90: Add missing C++ mangled names for ARM.
      msvcp100: Add missing C++ mangled names for ARM.
      winedump: Print exported function names in the exception data.
      winedump: Print the export or import name of exception handlers.
      winedump: Dump exception data for known exception handlers.
      vkd3d: Import upstream release 1.12.
      msvcrt: Move common exception handling types to the header.
      msvcrt: Share the find_caught_type() helper between platforms.
      msvcrt: Share the copy_exception() helper between platforms.
      msvcrt: Share a helper to find a catch block handler.

Alexandros Frantzis (15):
      opengl32: Remove the wglDescribePixelFormat driver entry point.
      win32u: Emit number characters for numpad virtual keys.
      win32u: Allow drivers to send only the scan code for keyboard events.
      win32u: Store the full KBD vkey information in kbd_tables_init_vsc2vk.
      server: Send numpad virtual keys if NumLock is active.
      user32/tests: Add tests for SendInput with numpad scancodes.
      winewayland.drv: Populate vkey to wchar entry for VK_DECIMAL.
      server: Fix handling of KEYEVENTF_UNICODE inputs with a non-zero vkey.
      user32/tests: Add more test for unicode input with vkey.
      user32/tests: Add tests for raw keyboard messages.
      server: Use right-left modifier vkeys for hooks.
      server: Apply modifier vkey transformations regardless of unicode flag.
      server: Don't send raw input events for unicode inputs.
      user32/tests: Check async key state in raw nolegacy tests.
      server: Set VK_PACKET async state in raw input legacy mode.

Alistair Leslie-Hughes (10):
      include: Complete __wine_uuidof for C++.
      include: Add C++ support for IUnknown.
      odbc32: Correct SQLSetConnectOptionW length parameter type.
      include: Added sqlucode.h to sql.h.
      include: Correct ListView_GetItemIndexRect macro.
      include: Add IFACEMETHOD macros.
      include: Add LOGFONTA/W typedef in shtypes.idl.
      include: Add IPreviewHandler* interfaces.
      include: Correct IRowsetNotify HROW parameter type.
      include: Add missing TreeView_* defines.

Anton Baskanov (4):
      quartz/tests: Use unaligned width in AVIDec tests to expose incorrect stride calculation.
      quartz: Get output format from source, not sink in AVIDec.
      quartz: Use the correct stride when calculating image size in AVIDec.
      quartz: Hold the streaming lock while calling ICDecompressEnd.

Brendan Shanks (1):
      ntdll: Don't warn on macOS and FreeBSD when xattr doesn't exist.

Connor McAdams (1):
      uiautomationcore: NULL initialize SAFEARRAY variable passed to IRawElementProviderFragment::GetRuntimeId().

Daniel Lehman (1):
      odbc32: Allow null handle for SQLSetEnvAttr.

Danyil Blyschak (3):
      win32u: Remove external fonts from the registry before writing to it.
      shcore: Check optional pointer in filestream_CopyTo() before writing to it.
      wineps.drv: Only merge dmDefaultSource member of devmodes when a slot is found.

Davide Beatrici (6):
      winealsa: Return minimum period in get_device_period if requested.
      mmdevapi: Adjust timing in AudioClient_Initialize.
      winealsa: Remove superfluous timing adjustment.
      winecoreaudio: Remove superfluous timing adjustment.
      wineoss: Remove superfluous timing adjustment.
      winepulse: Remove superfluous timing adjustment.

Dmitry Timoshkov (2):
      comctl32/tests: Create a fully updated ListView window.
      user32/tests: Add a test to show that SendMessage(LB_SETCOUNT) adds a scrollbar.

Elizabeth Figura (21):
      wined3d: Remove the no longer used STATE_SAMPLER.
      wined3d: Remove the no longer used STATE_POINTSPRITECOORDORIGIN.
      wined3d: Remove the FFP blitter.
      wined3d: Remove some obsolete state invalidations.
      wined3d: Remove the no longer needed fragment_caps.proj_control flag.
      d3d9/tests: Remove leftover debugging code.
      wined3d: Remove the no longer needed fragment_caps.srgb_texture flag.
      wined3d: Remove the no longer needed fragment_caps.color_key flag.
      wined3d: Remove the no longer needed wined3d_vertex_caps.xyzrhw flag.
      wined3d: Remove the no longer needed wined3d_vertex_caps.ffp_generic_attributes flag.
      wined3d: Remove the no longer used buffer conversion code.
      wined3d: Remove the no longer used wined3d_context.fog_coord field.
      wined3d: Remove the no longer used wined3d_context_gl.untracked_material_count field.
      wined3d: Remove the no longer used wined3d_context.use_immediate_mode_draw field.
      wined3d: Remove the no longer used WINED3D_SHADER_CAP_VS_CLIPPING flag.
      wined3d: Remove the no longer used wined3d_context.namedArraysLoaded field.
      wined3d: Remove the no longer used WINED3D_SHADER_CAP_SRGB_WRITE flag.
      wined3d: Move the GL_EXTCALL() definition to wined3d_gl.h.
      wined3d: Remove some no longer used wined3d_context fields.
      wined3d: Remove no longer used "exponent" and "cutoff" precomputed fields.
      wined3d: Remove the no longer used ignore_textype argument of wined3d_ffp_get_fs_settings().

Eric Pouech (4):
      conhost: Fix display of font preview in 64-bit mode.
      winedump: Dump correct handle information for minidump.
      winedump: Don't dump twice.
      winedump: Dump Memory64List streams in minidumps.

Esme Povirk (14):
      gdiplus: Replace HDC check in GdipFlush.
      gdiplus: Bracket HDC use in GdipMeasureCharacterRanges.
      gdiplus: Bracket HDC use in GdipMeasureString.
      gdiplus: Bracket HDC use in GdipDrawString.
      gdiplus: Bracket HDC use in GDI32_GdipDrawDriverString.
      gitlab: Add unzip to build image.
      gdiplus: Replace HDC use in draw_driver_string.
      gdiplus: Bracket HDC use in get_path_hrgn.
      gdiplus: Bracket HDC use in gdi_transform_acquire/release.
      gdiplus: Do not store HDC on HWND Graphics objects.
      gdiplus: Don't call GetDeviceCaps for NULL dc.
      user32/tests: Rename winevent_hook_todo to msg_todo.
      user32/tests: Mark some Wine-todo messages.
      win32u: Implement EVENT_SYSTEM_FOREGROUND.

Fabian Maurer (5):
      oleaut32: Add test for invoking a dispatch get-only property with DISPATCH_PROPERTYPUT.
      oleaut32: Handle cases where invoking a get-only property with INVOKE_PROPERTYPUT returns DISP_E_BADPARAMCOUNT.
      userenv: Add CreateAppContainerProfile stub.
      riched20: In para_set_fmt protect against out of bound cTabStop values.
      user32/tests: Fix ok_sequence succeeding in todo block not giving a test failure.

Francis De Brabandere (1):
      vbscript/tests: Fix error clear call.

Ilia Docin (1):
      sane.ds: Add missing color modes setting support.

Jacek Caban (30):
      mshtml: Use DISPEX_IDISPATCH_IMPL macro in htmlobject.c.
      mshtml: Use DISPEX_IDISPATCH_IMPL macro in htmlscript.c.
      mshtml: Use DISPEX_IDISPATCH_IMPL macro in htmlselect.c.
      mshtml: Use DISPEX_IDISPATCH_IMPL macro in htmlstorage.c.
      mshtml: Use DISPEX_IDISPATCH_IMPL macro in htmlstyle.c.
      mshtml: Use DISPEX_IDISPATCH_IMPL macro in htmlstyleelem.c.
      mshtml: Use DISPEX_IDISPATCH_IMPL macro in htmlstylesheet.c.
      mshtml: Use DISPEX_IDISPATCH_IMPL macro in htmltable.c.
      mshtml: Use DISPEX_IDISPATCH_IMPL macro in htmltextarea.c.
      mshtml: Use DISPEX_IDISPATCH_IMPL macro in htmltextnode.c.
      mshtml: Use DISPEX_IDISPATCH_IMPL macro in mutation.c.
      mshtml: Use DISPEX_IDISPATCH_IMPL macro in omnavigator.c.
      mshtml: Use DISPEX_IDISPATCH_IMPL macro in range.c.
      mshtml: Use DISPEX_IDISPATCH_IMPL macro in selection.c.
      mshtml: Use DISPEX_IDISPATCH_IMPL macro in svg.c.
      mshtml: Use DISPEX_IDISPATCH_IMPL macro in xmlhttprequest.c.
      ntdll: Use assembly wrapper for unixlib calls on ARM64EC.
      d3d9/tests: Use GNU assembly syntax on clang x86_64 MSVC target.
      d3d8/tests: Use GNU assembly syntax on clang x86_64 MSVC target.
      d3d9: Use GNU assembly syntax on clang x86_64 MSVC target.
      d3d8: Use GNU assembly syntax on clang x86_64 MSVC target.
      ddraw: Use GNU assembly syntax on clang x86_64 MSVC target.
      gitlab: Use --enable-werror for Clang builds.
      mshtml: Return success in IHTMLWindow2::get_closed stub.
      mshtml/tests: Add more custom properties tests.
      mshtml: Factor out alloc_dynamic_prop.
      mshtml: Use DispatchEx vtbl for elements as window property lookups.
      mshtml: Use DispatchEx vtbl for all window properties.
      mshtml: Use macro for window object IDispatch functions implementation.
      mshtml: Move IDispatchEx forwarding implementation to outer window object.

Krzysztof Bogacki (7):
      win32u: Use separate variable for inner loop.
      win32u: Log Vulkan GPU's PCI IDs when matching against them.
      win32u: Log Vulkan UUIDs when adding GPUs.
      win32u: Remove unused variable from add_vulkan_only_gpus.
      win32u: Use common name for fake GPUs and prefer Vulkan name over it.
      win32u: Prefer Vulkan PCI IDs over empty ones.
      win32u: Prefer Vulkan UUIDs over empty ones.

Marcus Meissner (1):
      shell32/tests: Fixed sizeof to GetModuleFileName.

Myah Caron (1):
      msvcrt: Fix _kbhit ignoring the last event.

Nikolay Sivov (3):
      gdi32/text: Make GetTextExtentExPointW() return sizes consistent with ExtTextOutW().
      d3dcompiler/fx: Write empty buffers for compiler versions 33-39.
      d3dcompiler: Enable D3DCOMPILE_EFFECT_CHILD_EFFECT option.

Paul Gofman (4):
      wine.inf: Add InstallationType field to CurrentVersion.
      wine.inf: Add Explorer\Advanced registry key.
      mf/tests: Add a test for MFEnumDeviceSources().
      mf: Implement audio capture device enumeration in MFEnumDeviceSources().

Piotr Caban (1):
      ntdll: Fix UNC path handling in alloc_module.

Rémi Bernon (57):
      win32u: Fix default_update_display_devices return type to NTSTATUS.
      win32u: Load the graphics driver vulkan functions lazily.
      win32u: Keep a list of vulkan GPUS in the device manager context.
      win32u: Match driver GPUs with vulkan GPUS from their ids, or index.
      win32u: Query GPU memory from vulkan physical device.
      win32u: Enumerate offscreen vulkan devices as GPU devices.
      dinput: Dynamically allocate the internal device / event arrays.
      server: Pass the adjusted vkey to send_hook_ll_message.
      winegstreamer: Use DMO_MEDIA_TYPE in the WMA decoder.
      winegstreamer: Implement WMA DMO Get(Input|Output)CurrentType.
      win32u/tests: Introduce a new run_in_process helper.
      win32u/tests: Add NtUser(Get|Set)ProcessDpiAwarenessContext tests.
      win32u: Fix NtUserSetProcessDpiAwarenessContext.
      user32/tests: Add some SetProcessDpiAwarenessContext tests.
      user32/tests: Add more SetThreadDpiAwarenessContext tests.
      user32: Fix SetProcessDpiAwarenessContext.
      winegstreamer: Use a GstCaps for wg_parser current_format.
      winegstreamer: Use a GstCaps instead of preferred_format.
      winegstreamer: Rename get_preferred_format to get_current_format.
      winegstreamer: Use a GstCaps for wg_parser_stream codec format.
      win32u: Get rid of the drivers force_display_devices_refresh flag.
      win32u: Update the display device cache after loading the driver.
      win32u: Get rid of the UpdateDisplayDevices force parameter.
      user32: Test and fix IsValidDpiAwarenessContext.
      user32: Test and implement GetDpiFromDpiAwarenessContext.
      user32/tests: Add more AreDpiAwarenessContextsEqual tests.
      user32/tests: Add more GetAwarenessFromDpiAwarenessContext tests.
      win32u: Use NtUserCallOnParam for SetThreadDpiAwarenessContext.
      win32u: Return UINT from NtUserGetWindowDpiAwarenessContext.
      win32u: Use NtGdiDdDDICreateDCFromMemory for gdi16 DIBDRV.
      wineandroid: Fix NtUserSendHardwareInput parameter order.
      wineandroid: Use DWORD for pixel pointers.
      win32u: Flush window surface when it is fully unlocked.
      win32u: Remove surface recursive locking requirement.
      win32u: Stop using a recursive mutex for the offscreen surface.
      wineandroid: Stop using a recursive mutex for the window surfaces.
      winemac: Stop using a recursive mutex for the window surfaces.
      winewayland: Stop using a recursive mutex for the window surfaces.
      winex11: Stop using a recursive mutex for the window surfaces.
      win32u: Get the thread DPI context instead of the awareness.
      win32u: Pass the DPI awareness context in win_proc_params.
      win32u: Fix SetThreadDpiAwarenessContext.
      win32u: Only keep DPI awareness context with window objects.
      win32u: Introduce a new window_surface_init helper.
      win32u: Move the window surface mutex to the surface header.
      win32u: Use helpers to lock/unlock window surfaces.
      win32u: Move window surface bounds to the window_surface base struct.
      winemac: Get rid of unnecessary blit_data / drawn surface members.
      wineandroid: Hold the lock while reading window surface bits.
      win32u: Use a helper to flush window surface, factor locking and bounds reset.
      win32u: Initialize window surfaces with a hwnd.
      win32u: Split update_surface_region into get_window_region helper.
      server: Merge get_surface_region / get_window_region requests together.
      win32u: Intersect the clipping region with the window shape region.
      server: Update window surface regions when the window is shaped.
      wineandroid: Remove now unnecessary set_surface_region calls.
      win32u: Use a helper to set the window surface clipping, within the lock.

Vijay Kiran Kamuju (1):
      ntdll: Add stub RtlLookupGenericTableAvl function.

Yuxuan Shui (2):
      shell32: Make sure array passed to PathResolve is big enough.
      shell32: Fix ShellExecute for non-filespec paths.

Zhiyi Zhang (4):
      comctl32/tests: Add WM_SETFONT tests.
      comctl32/syslink: Don't delete font when destroying the control.
      comctl32/tooltips: Don't duplicate font when handling WM_SETFONT.
      comctl32/ipaddress: Delete font when destroying the control.
```
