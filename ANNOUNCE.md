The Wine development release 9.2 is now available.

What's new in this release:
  - Mono engine updated to version 9.0.0.
  - A number of system tray fixes.
  - Exception handling improvements on ARM platforms.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/9.x/wine-9.2.tar.xz>

Binary packages for various distributions will be available
from <https://www.winehq.org/download>

You will find documentation on <https://www.winehq.org/documentation>

Wine is available thanks to the work of many people.
See the file [AUTHORS][1] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/raw/wine-9.2/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 9.2 (total 15):

 - #43993  Quick3270 5.21: crashes when using the Connect function
 - #47521  digikam 6.10 crashes on start
 - #51360  vkGetDeviceProcAddr invalid behavior for functions from extensions unsupported by host Vulkan instance
 - #51770  digikam-7.1.0 freezes on start
 - #51843  dlls/ws2_32/socket.c:839:17: error: ‘IP_ADD_SOURCE_MEMBERSHIP’ undeclared here
 - #53934  __unDName fails to demangle a name
 - #55997  Dolphin Emulator crashes from 5.0-17264
 - #56122  LANCommander won't start, prints "error code 0x8007046C" (ERROR_MAPPED_ALIGNMENT)
 - #56243  ShowSystray registry key was removed without alternative
 - #56250  Elite Dangerous client gets stuck on black screen after launch
 - #56256  Windows Sysinternals Process Explorer 17.05 shows incomplete user interface (32-bit).
 - #56259  Microsoft Webview 2 installer hangs forever
 - #56265  Epic Games Launcher 15.21.0 calls unimplemented function cfgmgr32.dll.CM_Get_Device_Interface_PropertyW
 - #56291  Kodu game lab crashes (with xnafx40_redist+dotnet48 preinstalled): Object reference not set to an instance of an object.
 - #56293  user32:msg test_recursive_hook fails on Windows 7

### Changes since 9.1:
```
Aida Jonikienė (1):
      configure: Use YEAR2038 macro when it's available.

Alex Henrie (2):
      krnl386: Emulate the VGA status register.
      explorer: Handle the back and forward buttons of a 5-button mouse.

Alexandre Julliard (37):
      configure: Reset host flags in all cross-compiler error paths.
      winsta: Start time is an input parameter in WinStationGetProcessSid.
      ntdll: Use the system setjmp/longjmp for exceptions in Unix libs.
      ntdll: Use a .seh handler for the unwind exception handler.
      ntdll: Avoid calling DbgBreakPoint() in process_breakpoint().
      ntdll: Move DbgUiRemoteBreakin() to the CPU backends.
      include: Include cfg.h from cfgmgr32.h.
      ntdll: Report the correct address for breakpoint exception on ARM platforms.
      kernel32/tests: Fix some test failures on ARM platforms.
      ntdll: Use a .seh handler for DbgUiRemoteBreakin().
      dbghelp/tests: Mark failing tests as todo.
      Revert "loader: Associate folder with explorer".
      configure: Require a PE compiler for 32-bit ARM.
      ntdll: Share the is_valid_frame() helper function.
      ntdll: Only call TEB handlers for frames inside the current stack.
      winedump: Make the ARM exception information more compact.
      winedump: Handle ARM64 exception unwind info with flag==3.
      winebuild: Default to plain "clang" in find_clang_tool().
      winebuild: Remove some no longer used code for ARM platforms.
      winebuild: Add .seh annotations on ARM.
      kernel32: Move Wow64Get/SetThreadContext implementation to kernelbase.
      kernel32: Don't export RtlRaiseException on ARM64.
      ntdll: Fix stack layout for ARM syscalls.
      ntdll/tests: Add some process machine tests for ARM64X.
      server: Don't report alternate 64-bit machines as supported.
      ntdll: Update the image information when loading a builtin dll.
      ntdll: Use the correct machine when loading ntdll on ARM64EC.
      server: Don't update the machine in the image information for ARM64EC modules.
      server: Don't update the entry point in the image information for ARM64EC modules.
      ntdll: Redirect the module entry point for ARM64EC modules.
      server: Add hybrid flag in image mapping information.
      ntdll: Use the current machine by default to create an ARM64X process.
      ntdll: Fix RtlWow64GetCurrentMachine() result on ARM64EC.
      uxtheme: Use BOOLEAN instead of BOOL in ordinal functions.
      gdi32/tests: Fix the expected GetTextMetrics() results on recent Windows.
      user32/tests: Fix some sysparams results on recent Windows.
      ntdll/tests: Mark a failing test as todo.

Alexandros Frantzis (2):
      winewayland.drv: Track and apply latest window cursor on pointer enter.
      win32u: Use consistent locking order for display related mutexes.

Arkadiusz Hiler (4):
      winebus.sys: Fix units used for hat switches.
      winebus.sys: Use 4 bits for hat switches.
      wbemprox/tests: Test LIKE queries.
      wbemprox: Reimplement LIKE.

Brendan Shanks (2):
      winebuild: Refactor find_tool().
      winebuild: As a last resort, search for tools un-prefixed using clang.

Daniel Lehman (9):
      glu32/tests: Add a few tests for gluScaleImage.
      glu32: Return GL_OUT_OF_MEMORY from gluScaleImage if no context.
      glu32: Return GLU_INVALID_ENUM for illegal pixel types.
      advapi32/tests: Reduce reallocations.
      advapi32/tests: Add some more EventLog tests.
      oleaut32/tests: Add tests for VarBstrFromR8.
      oleaut32/tests: Add tests for VarBstrFromR4.
      ucrtbase/tests: Add sprintf tests.
      msvcrt/tests: Add sprintf tests.

David Kahurani (5):
      gdiplus: Use GdipCreatePath2 in GdipClonePath.
      gdiplus: Use GdipCreatePath2 when serializing paths.
      gdiplus: Use path_list to path helper in GdipFlattenPath.
      gdiplus: Use path_list to path helper in GdipWidenPath.
      msvcrt: Free previous environment variable when clearing.

Eric Pouech (18):
      dmime/tests: Fix copy & paste errors.
      dmime/tests: Add some tests for loops on wave tracks.
      dmime: Fix IDirectMusicAudioPath::GetObjectInPath() prototype.
      dmime: Remove unused fields in segment.
      dmime/tests: Add some tests about end-points.
      dbghelp/tests: Use Unicode encoding for generated PDB filenames.
      dbghelp/tests: Test SymSrvGetFileIndexInfo() on .dbg files.
      dbghelp: Implement SymSrvGetFileIndexInfo() for .dbg files.
      dbghelp: Implement SymSrvGetFileIndexInfo() for PDB/JG files.
      kernel32/tests: Added tests about std handle flags inheritance.
      kernel32/tests: Test DUPLICATE_SAME_ATTRIBUTES flag in DuplicateHandle().
      server: Implement support for DUPLICATE_SAME_ATTRIBUTES in DuplicateHandle().
      server: Preserve handle flags when inheriting a std handle.
      dmime: Fully implement IDirectMusicSegmentState::GetRepeats().
      dmime: Add tests about segment state's graph interface.
      dmime: Add IDirectMusicGraph interface to segment state.
      dmime: Generate track flags while in loop.
      dmime: Use sent duration in loop's playback.

Esme Povirk (6):
      gdiplus: Prefer Tahoma for generic sans serif font.
      mscoree: Update Wine Mono to 9.0.0.
      user32/tests: Accept EM_GETPASSWORDCHAR when edit is focused.
      gdiplus: Fix some degenerate cases combining infinite regions.
      gdiplus/tests: Thoroughly test region combines.
      user32/tests: Fix flags on expected EM_GETPASSWORDCHAR message.

Fabian Maurer (1):
      msi: Don't write past end of string when selecting parent directory.

Felix Münchhalfen (2):
      ntdll: Use pagesize alignment if MEM_REPLACE_PLACEHOLDER is set in flags of NtMapViewOfSection(Ex).
      kernelbase/tests: Add a test for MapViewOfFile3 with MEM_REPLACE_PLACEHOLDER.

Gabriel Ivăncescu (16):
      mshtml: Forward the script site's QueryService to the document's.
      mshtml: Forward SID_SInternetHostSecurityManager of the document obj to the doc node.
      mshtml: Implement IActiveScriptSite service.
      mshtml: Implement Exec for CGID_ScriptSite's CMDID_SCRIPTSITE_SECURITY_WINDOW.
      vbscript: Implement IActiveScriptSite service.
      jscript: Implement IActiveScriptSite service.
      mshtml: Use a hook to implement postMessage.
      mshtml: Implement `source` prop for MessageEvents.
      mshtml: Return E_ABORT from postMessage called without a caller ServiceProvider.
      mshtml: Implement `data` getter for MessageEvent objs.
      mshtml: Implement `origin` prop for MessageEvents.
      mshtml: Implement `initMessageEvent` for MessageEvents.
      mshtml: Expose the IHTMLEventObj5 props to scripts.
      mshtml/tests: Test builtin function default value getter with custom IOleCommandTarget.
      explorer: Don't activate the systray icon when showing it.
      explorer: Set layered style on systray icons only when it's actually layered.

Georg Lehmann (3):
      winevulkan: Prepare for VK_KHR_calibrated_timestamps.
      winevulkan: Update to VK spec version 1.3.277.
      winevulkan: Enable VK_ARM_render_pass_striped.

Giovanni Mascellani (1):
      d2d1: Compile vertex shaders with D3DCompile().

Hans Leidekker (1):
      msxml3: Enable XPath object cache.

Helix Graziani (2):
      cfgmgr32: Add CM_Get_Device_Interface_PropertyW stub.
      windows.globalization: Add IIterable_HSTRING impl to IVectorView_HSTRING.

Ivo Ivanov (1):
      winebus.sys: Prefer hidraw if it is the only backend enabled.

Jacek Caban (11):
      ncrypt/tests: Don't use uninitialized variable in test_get_property.
      windowscodecs: Pass result as void pointer to ComponentInfo_GetUINTValue.
      mf: Avoid implicit enum to int pointer casts.
      mfplat: Avoid implicit cast in IMFAttributes_GetUINT32 call.
      mfplat/tests: Use MF_ATTRIBUTE_TYPE type in IMFMediaType_GetItemType call.
      mfplat: Introduce media_type_get_uint32 helper.
      mfmediaengine/tests: Use MF_MEDIA_ENGINE_CANPLAY type in IMFMediaEngine_CanPlayType call.
      mfmediaengine: Avoid implicit casts in IMFAttributes_GetUINT32 calls.
      include: Add RtlRestoreContext declaration.
      d3d10/tests: Avoid implicit cast changing value.
      dsound: Simplify f_to_32.

Louis Lenders (5):
      ntdll: Add stub for RtlGetDeviceFamilyInfoEnum.
      shcore: Add stub for RegisterScaleChangeNotifications.
      wbemprox: Add property 'Caption' to Win32_PnPEntity.
      wbemprox: Add property 'ClassGuid' to Win32_PnPEntity.
      wbemprox: Add property 'Caption' to Win32_DiskDrive.

Marc-Aurel Zent (2):
      ntdll: Reimplement set_native_thread_name() for macOS.
      ntdll: Fix DW_OP_abs absolute value warning on labs() for clang.

Nikolay Sivov (3):
      mfplat/tests: Skip device manager test if d3d11 device can't be created.
      mfplat/tests: Skip tests that require d3d9 support.
      d3d10/effect: Clarify constant buffer flags field meaning.

Paul Gofman (6):
      ntdll/tests: Fix test_user_shared_data() for a more generic set of present features.
      server: Check if we have waiting asyncs in (send_socket) before enforcing blocking send.
      server: Check if we have waiting asyncs in sock_dispatch_asyncs() before clearing POLLOUT.
      explorer: Don't pop start menu on "minimize all windows" systray command.
      explorer: Don't pop start menu on "undo minimize all windows" systray command.
      msvcrt: Adjust _gmtime64_s() accepted time limits.

Piotr Caban (6):
      msvcrt: Store _unDName function parameter backreferences in parsed_symbol structure.
      msvcrt: Remove no longer used parameters reference arguments from _unDname helpers.
      winedump: Sync demangling code with msvcrt.
      jsproxy: Don't ignore hostname and url length in InternetGetProxyInfo.
      winhttp/tests: Add more WinHttpGetProxyForUrl tests.
      winhttp: Add support for WINHTTP_AUTOPROXY_HOST_LOWERCASE flag in WinHttpGetProxyForUrl.

Rémi Bernon (48):
      explorer: Restore a per-desktop ShowSystray registry setting.
      mf/tests: Check inserted topology loader transforms explicitly.
      mf/topology_loader: Use a local variable for the indirect connection method.
      mf/topology_loader: Ignore SetOutputType errors when doing indirect connect.
      mf/topology_loader: Initialize transform output type before adding converter.
      mf/topology_loader: Try to connect transform nodes with their current types first.
      winegstreamer: Implement H264 decoder GetInputCurrentType.
      winegstreamer: Ask GStreamer to stop messing with signal handlers.
      vulkan-1/tests: Enable VK_VERSION_1_1 as requested by validation layers.
      vulkan-1/tests: Create surface and device before calling test_null_hwnd.
      vulkan-1/tests: Test VK_KHR_win32_surface with windows in various states.
      vulkan-1/tests: Add more VK_KHR_win32_surface surface formats tests.
      vulkan-1/tests: Test VK_KHR_win32_surface WSI with swapchain functions.
      imm32/tests: Also ignore NotifyIME calls when ignoring WM_IME_NOTIFY.
      imm32/tests: Fix some spurious failures with Windows ko_KR IME.
      win32u: Only queue a single IME update during ImeProcessKey.
      winevulkan: Keep the create_info HWND on the surface wrappers.
      winevulkan: Return VK_ERROR_SURFACE_LOST_KHR from surface functions.
      winewayland: Remove now unnecessary VK_ERROR_SURFACE_LOST_KHR checks.
      winevulkan: Remove now unnecessary vkGetPhysicalDeviceSurfaceSupportKHR driver entry.
      winevulkan: Remove now unnecessary vkGetPhysicalDeviceSurfacePresentModesKHR driver entry.
      winevulkan: Remove now unnecessary vkGetDeviceGroupSurfacePresentModesKHR driver entry.
      user32/tests: Run rawinput device tests in the separate desktop.
      user32/tests: Rewrite the rawinput buffer test with keyboard input.
      dinput/tests: Add a helper to wait on HID input reads.
      dinput/tests: Add more tests for HID rawinput buffer.
      dinput/tests: Test rawinput messages with non-input desktop.
      dinput/tests: Use a polled HID touchscreen device.
      dinput/tests: Test rawinput with the virtual HID touchscreen.
      winegstreamer: Trace wg_transform input and output caps.
      winegstreamer: Handle allocation query in a separate helper.
      winegstreamer: Handle sink caps query in a separate helper.
      winegstreamer: Handle sink event caps in a separate helper.
      winegstreamer: Use GST_PTR_FORMAT to trace GStreamer objects.
      winegstreamer: Ignore wg_transform input / output video format fps.
      winegstreamer: Allow wg_transform size changes with an explicit attribute.
      mf/tests: Report more transform current type mismatches.
      mf/tests: Add some tests with video processor aperture handling.
      mfreadwrite/tests: Initialize test source stream types from descriptors.
      mfreadwrite/tests: Test source reader exposed transforms and types.
      mfreadwrite/tests: Test source reader transforms with MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING.
      mfreadwrite/tests: Test source reader transforms with MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING.
      server: Combine HID usage page and usage together.
      server: Stop using union rawinput in hw_input_t.
      server: Stop using hardware_msg_data in rawinput_message.
      server: Move rawinput message conversion from win32u.
      server: Fix rawinput buffer sizes and alignment on WoW64.
      win32u: Get rid of the rawinput thread data and buffer.

Shaun Ren (4):
      sapi: Create a new engine only when needed in ISpVoice.
      sapi: Add ISpeechObjectToken stub.
      sapi: Add ISpeechObjectTokens stub.
      sapi: Add stub implementation for ISpeechObjectTokens::get__NewEnum.

Tim Clem (1):
      winebus.sys: Do not attempt to open keyboard and mouse HID devices on macOS.

Vibhav Pant (1):
      configure: Correctly derive the target from CC if it's set to an absolute path.

Yuxuan Shui (2):
      dmusic: Fix loading wave data from soundfont.
      mf: Only preroll when starting from stopped state.

Zebediah Figura (8):
      wined3d: Move state objects from state.c to device.c.
      wined3d: Rename state.c to ffp_gl.c.
      wined3d: Move sampler_texdim() and texture_activate_dimensions() to ffp_gl.c.
      d3d8/tests: Add more tests for SPECULARENABLE.
      wined3d/glsl: Always set WINED3D_SHADER_CONST_FFP_LIGHTS in FFP constant update masks.
      wined3d/glsl: Pass through the specular varying when SPECULARENABLE is FALSE.
      wined3d/arb: Always disable the fragment pipeline in shader_arb_select().
      wined3d/arb: Compare the fragment pipe ops in shader_arb_select() instead of using an extra field.

Zhiyi Zhang (7):
      winex11.drv: Translate whole_rect to x11 root coordinates in set_size_hints().
      user32/tests: Fix test_recursive_messages() test failures on win7.
      mf: Make session_get_node_object() more robust.
      mf: Add a session_flush_nodes() helper.
      mf/tests: Add a create_test_topology() helper.
      mf: Support seeking while a session is started.
      mf/tests: Test IMFMediaSession::Start().
```
