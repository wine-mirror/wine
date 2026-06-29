The Wine development release 11.12 is now available.

What's new in this release:
  - Bundled libswresample and libswscale from FFmpeg.
  - Mono engine updated to version 11.2.0.
  - XSLPattern parser reimplemented in MSXML.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/11.x/wine-11.12.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-11.12/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 11.12 (total 27):

 - #3037   LTspice, Progman: broken internal windows resizing
 - #18010  Slingplayer video tuning wizard fails
 - #27240  Multiple Corel product installers report msxml:xslpattern_error syntax error / msxml:domselection_create error code 1206 (Corel Paint Shop Photo Pro, CorelDRAW Graphics Suite X3/X4)
 - #33450  .NET 3.5 Framework installation fails (.NET WorkFlow Service Registration Tool "WFServicesReg.exe" crash with libxml2 < 2.9.0)
 - #34758  Microsoft Money 97 5.0 crashes with an unhandled divide by zero
 - #36912  Taskkill does not support /T option
 - #44232  Menus in Slingplayer 1.5 do not draw until mouseover
 - #48174  wine-mono creates incorrect strings
 - #51678  Avogadro crashes when adding atoms
 - #56100  Greenshot: Error when adding a textbox
 - #56664  winecfg not exposing an option for 7.1 surround sound audio output (winepulse.drv)
 - #58632  Against the Storm: Encyclopedia doesn't display animated sequences
 - #59317  Need for Speed Most Wanted (2005): Up is triggered continuously without pressing a button on gamepad
 - #59318  Graphic problem
 - #59406  Super Hexagon: if gamepad is connected, game will ignore every input after selecting anything from title screen
 - #59590  MS-Money 2000 - cannot process TKIND_UNION when loading INV7.OCX
 - #59711  unlock_fd: UnlockFile returns ERROR_LOCK_VIOLATION (33) instead of ERROR_NOT_LOCKED (158) when no matching lock is held
 - #59820  ComicRackCE freezes when attempting to view "info" for a comic file
 - #59842  Cross builds don't install the "wine" executable anymore
 - #59855  .configure script fails on system without "ln -s"
 - #59856  "musl: Use __builtin_rint if available" breaks rounding control in rint function family on x86_64
 - #59870  VRChat PhotonEncryptorPlugin.dll crash
 - #59871  Sonic Boom (1995 Klik & Play fangame) fails to create HBITMAP over memory, cannot start
 - #59880  For games installed under a prefix that has cnc_ddraw installed using Winetricks, error message appears
 - #59886  Build failed for SymCrypt with clang-msvc on x86/i386 and x86_64
 - #59894  MS Office 2007 does no longer accept product key from config file
 - #59910  One case of i386 unwinding broken

### Changes since 11.11:
```
Alexandre Julliard (20):
      makefiles: Don't reuse an install command if it has an explicit destination name.
      makefiles: Always enable the wine tool.
      makefiles: Also skip C files in arch-specific directories.
      libs: Import ffmpeg from upstream release 8.1.1.
      user32: Don't send a message in GetComboBoxInfo().
      nsiproxy.sys: Link to pthread libs for pthread_once.
      ffmpeg: Import upstream assembly optimizations for ARM64.
      ffmpeg: Import upstream assembly optimizations for ARM.
      makefiles: Add support for nasm.
      ffmpeg: Import upstream assembly optimizations for x86.
      symcrypt: Disable asm optimizations globally for non-PE builds.
      symcrypt: Disable asm optimizations for older clang versions.
      ffmpeg: Only disable clang warnings when building ffmpeg itself.
      configure: Use the pkg-config specified libs by default in package checks.
      configure: Use the pkg-config specified libs also for MinGW checks.
      configure: Add more pkg-config checks.
      kernelbase: Implement HasFiberData thread flag.
      ntdll: Initialize fiber data to 0x1e00.
      ntdll: Set default NtGlobalFlag for debugged processes.
      server: Fix returned status for NtUnlockFile on unlocked range.

Alistair Leslie-Hughes (4):
      tiptsf: Add dll.
      tiptsf: Add ITextInputPanel interface stub.
      rtscom: Add stub dll.
      rtscom: Add RealTimeStylus stubbed interface.

André Zwing (6):
      comctl32/tests: Fix typo for ARM64EC.
      kernel32/tests: Fix typo for ARM64EC.
      oleaut32/tests: Fix typo for ARM64EC.
      user32/tests: Fix typo for ARM64EC.
      uxtheme/tests: Fix typo for ARM64EC.
      vcomp/tests: Fix typo for ARM64EC.

Anna (navi) Figueiredo Gomes (4):
      win32u: Stub NtUserGetPointerType.
      win32u: Stub NtUserGetPointerDeviceRects.
      win32u/tests: Add NtUserGetPointerDeviceRects test for INVALID_HANDLE_VALUE.
      win32u: Implement NtUserGetPointerDeviceRects for INVALID_HANDLE_VALUE.

Bartosz Kosiorek (5):
      msvcp: Use sizeof(wchar_t) instead of sizeof(char) for basic_string_wchar_replace_cstr_len.
      msvcp: Use sizeof(wchar_t) instead of sizeof(char) for basic_string_wchar_replace_ch.
      msvcp90: Add _Ctraits<float>::_Isinf(float) implementation.
      msvcp90: Add _Ctraits<double>::_Isinf(double) implementation.
      msvcp90: Add _Ctraits<long double>::_Isinf(long double) implementation.

Benoît Legat (1):
      rsaenh: Fix reporting of "Cryptographic Service Provider".

Bernhard Übelacker (9):
      mfplat/tests: Avoid crash if mediasource retrieval failed.
      mfplat/tests: Explicitly link against kernel32 before kernelbase.
      mfsrcsnk/tests: Add broken in test_mp4_extra_metadata.
      user32/tests: Fix test_class_name with old Windows versions.
      dxgi/tests: Add two missing winetest_pop_context.
      dxgi/tests: Skip tests if DXGI 1.4 is not available.
      msxml3/xpath: Fix out-of-bounds access in query string (ASan).
      oleview: Avoid heap-use-after-free when reloading (ASan).
      regedit: Avoid heap-buffer-overflow in regedit with REG_MULTI_SZ (ASan).

Connor McAdams (1):
      dinput: Lowercase device paths passed into insert_cache_entry() when reusing cache entries.

Daniel Lehman (4):
      msxml3/tests: Fix double-free of node text.
      dwrite: Add fallbacks for arrows.
      include: Move ifdef __cplusplus in rpcasync.h.
      shell32: Use DE_INVALIDFILES instead of hard-coded value.

Dmitry Timoshkov (2):
      oleaut32: Take into account that SAFEARRAY element in an SLTG typelib uses 32-bit units.
      gdi.exe: Don't use biSizeImage when calculating DIB pitch.

Esme Povirk (1):
      mscoree: Update Wine Mono to 11.2.0.

Etaash Mathamsetty (6):
      winewayland: Implement fractional scaling protocol.
      winewayland: Fix missing wayland_win_data_release.
      win32u: Always flush the window surface in expose_window_surface.
      winewayland: Assign a name to the winewayland queue.
      winewayland: Change default debug channel to cursor for pointer.
      winewayland: Rename toplevel_hwnd to owner_hwnd.

Feli Rippmann (1):
      comctl32: Clamp rebar height to cyMaxChild when cyIntegral == 0 too.

Floris Renaud (1):
      po: Update Dutch translation.

Francis De Brabandere (6):
      vbscript: Index named items by name.
      vbscript: Store global variables and functions in a single tagged tree.
      vbscript: Parse a bare '&' followed by octal digits as an octal literal.
      vbscript: Cache host DISPIDs as entries in the unified global tree.
      vbscript: Raise type mismatch when comparing an array.
      vbscript: Look up named items before builtin functions.

Giang Nguyen (1):
      d2d1: Free the vertex types array in d2d_figure_cleanup.

Hans Leidekker (4):
      bcrypt: Fix parameter validation for GCM mode.
      bcrypt: Remove unused fields.
      symcrypt: Accept RSA keys with public exponent 1.
      bcrypt: Reject RSA keys with public exponent 1.

Henri Verbeet (2):
      d3dcompiler: Retrieve signature information using vkd3d in d3dcompiler_shader_reflection_init().
      d3dcompiler: Retrieve thread group size information using vkd3d in d3dcompiler_shader_reflection_init().

Jacek Caban (12):
      dpnet: Remove unused variables.
      rpcrt4/tests: Remove unused variables.
      riched20/tests: Remove unused variables.
      msxml6/tests: Enable call checks in set_xht_site.
      msxml3/tests: Remove unused code.
      msxml3/tests: Add missing CLEAR_CALLED statement.
      mf/tests: Remove unused variables.
      wbemprox: Remove unused variable.
      winebus: Remove unused variables.
      joy.cpl: Remove unused variable.
      winevulkan: Remove unused variable.
      winex11: Remove unused variables.

Marius Kamm (2):
      windowscodecs: Fix decoding GIFs that fill the LZW code table.
      windowscodecs/tests: Test decoding a GIF that fills the LZW code table.

Martin Storsjö (2):
      ntdll: Don't validate arm64 pointer signing when unwinding.
      include: Use __msvcrt_ulong in setjmp.h.

Matteo Bruni (1):
      dsound: Get rid of non-float mixing remnants.

Nello De Gregoris (1):
      mfsrcsnk: Use SetCurrentPosition() for absolute byte stream seeks.

Nikolay Sivov (15):
      dsrole: Add a new dll.
      netapi32: Forward DsRole* functions to dsrole.dll.
      msxml3/tests: Add more selection tests.
      msxml3/xpath: Remove duplicate line typo.
      msxml3/tests: Add some tests for node value comparison in queries.
      msxml3/xpath: Fix comparison direction for node-set vs value case.
      msxml3/tests: Add some more comparison tests for XPath.
      msxml3: Add a helper to check for XPath query keywords.
      msxml3: Explicitly switch to XPath for XmlView documents.
      msxml3: Handle XSLPattern syntax directly instead of converting to XPath.
      msxml3: Fix a typo in a helper name.
      msxml3/tests: Add a query test with attribute test in the predicate.
      msxml3: Add support for nodeName() function.
      msxml3: Rename helpers to reflect their purpose better.
      msxml3: Remove now unused file.

Pan Hui (1):
      winewayland: Use process_name in wl_display_create_queue_with_name for debugging.

Paul Gofman (21):
      nsiproxy.sys: Factor out icmp_socket structure.
      nsiproxy.sys: Rename icmp_listen into icmp_get_reply.
      nsiproxy.sys: Contain all the reply data in struct icmp_reply_ctx.
      nsiproxy.sys: Manage ICMP listen threads on the Unix side.
      nsiproxy.sys: Poll all icmp sockets in a single thread.
      nsiproxy.sys: Return ICMP ids from parse_icmp_hdr() instead of matching those.
      nsiproxy.sys: Reuse icmp socket with matching parameters.
      nsiproxy.sys: Support ICMP error replies with ping sockets.
      nsiproxy.sys: Use compatible ICMP request id and seq numbers.
      winevulkan: Do not show debug assertion dialog on Unix side exceptions.
      win32u: Support D3D12_FENCE_SUBMIT_INFO in win32u_vkQueueSubmit().
      win32u: Force timeline semaphore type when importing d3d12 fence.
      ntdll/tests: Add tests for PROCESS_PARAMS_IMAGE_KEY_MISSING flag.
      ntdll: Manage PROCESS_PARAMS_IMAGE_KEY_MISSING flag.
      imm32/tests: Flush events in test_ImmSetCompositionWindow().
      imm32/tests: Add tests for resetting in-composition state.
      imm32: Strip off trailing zeros in ImeSetCompositionString().
      imm32: Clear in-composition state when empty composition string is set.
      imm32: Reset composition string when deactivating context.
      wbemprox: Use device instance id for pnpdevice_id in get_display_adapters().
      ntdll: Don't reserve exc_stack_layout twice in call_user_exception_dispatcher() on i386.

Piotr Caban (10):
      mshtml: Added IHTMLTextAreaElement::disabled property implementation.
      msv1_0: Initialize seal keys in ntlm_SpAcceptLsaModeContext.
      ucrtbase/tests: Remove no longer needed workaround from test_math_errors.
      ucrtbase: Don't use builtin_rint since it doesn't respect rounding control.
      secur32/tests: Cleanup ntlm negotiate flags tests.
      secur32/tests: Test ctxt_attrs value after initial InitializeSecurityContextA call.
      msv1_0: Fix buffer type returned by ntlm_SpAcceptLsaModeContext.
      msv1_0: Make output argument optional on second call to ntlm_SpAcceptLsaModeContext.
      msv1_0: Set ctxt_attr in ntlm_SpInitLsaModeContext.
      msv1_0: Set ctxt_attr in ntlm_SpAcceptLsaModeContext.

Rose Hellsing (4):
      gdiplus: Recognize raw WMF streams in the WMF image codec.
      gdiplus: Rasterize metafiles for encoding and report raw WMF as EMF.
      gdiplus: Store the source HMETAFILE for proper lifetime management.
      gdiplus/tests: Add tests for raw WMF load, playback and encoding.

Rémi Bernon (50):
      xinput1_3: Introduce a new open_device_at_index helper.
      mf/topology_loader: Restore original type when optional connection fails.
      opengl32: Implement client / host mapping for shader names.
      opengl32: Use a per-table lock for context / pbuffer / sync handles.
      opengl32: Move the sync handle table to the display lists.
      opengl32: Trace client side object names leaks.
      opengl32: Remove unused unix thunks.
      ffmpeg: Redirect av_log calls to Wine debug traces.
      winegstreamer: Remove unnecessary resampler inline qualifiers.
      winegstreamer/resampler: Implement IMediaObject iface using libswresample.
      winegstreamer/resampler: Translate IMFTransform calls to IMediaObject.
      resampledmo: Move resampler implementation from winegstreamer.
      win32u: Add a new driver entry to create client surfaces.
      win32u: Create vulkan client surfaces from win32u.
      win32u: Create OpenGL client surfaces from win32u.
      user32/tests: Use 1024x768 as test resolution for DPI scaling on Windows.
      user32/tests: Add more GetPointerDeviceRects tests with DPI scaling.
      opengl32: Unset current context on thread detach.
      win32u: Create a default thread context when binding NULL.
      opengl32: Ensure a context is current before display lists destruction.
      win32u: Share all unix-side GL contexts with a global context.
      opengl32: Keep a global WOW64 buffer storage map.
      opengl32: Implement wglShareLists on the PE side.
      win32u: Get rid of opengl_drawable set_context callback.
      opengl32: Allocate WOW64 buffers struct with calloc.
      opengl32: Set dummy context with wglMakeContextCurrentARB.
      gitlab: Add nasm to the CI docker images.
      xinput1_3: Move keystroke states and locks to check_for_keystroke.
      xinput1_3: Read and write current state without the controller lock.
      xinput1_3: Use a global critical section to guard device and enabled flag.
      xinput1_3: Extend the CS to guard all device open / read / destroy.
      winegstreamer/color_convert: Remove unnecessary inline qualifiers.
      winegstreamer/color_convert: Implement IMediaObject iface using libswscale.
      winegstreamer/color_convert: Translate IMFTransform calls to IMediaObject.
      colorcnv: Move color converter implementation from winegstreamer.
      winegstreamer/video_processor: Reimplement using libswscale.
      msvproc: Move video processor implementation from winegstreamer.
      opengl32: Only query GL_CONTEXT_PROFILE_MASK with GL >= 3.2.
      opengl32: Only reserve name range if range length is > 0.
      opengl32: Remove now unnecessary context creation attribs.
      winewayland: Make sure the primary monitor rect is at 0x0.
      winewayland: Use RECT / POINT to map coordinates from surface.
      winewayland: Use RECT / POINT to map coordinates to surface.
      winewayland: Use a RECT in struct wayland_surface_config.
      win32u: Use the null context instead of internal context for pbuffers.
      win32u: Notify driver of capture window reset on destruction.
      win32u: Pass current and previous toplevel windows to SetCapture.
      winex11: Also ignore warped MotionNotify if cursor isn't clipped.
      winex11: Simplify send_mouse_input when clipping cursor.
      winex11: Get rid of now unnecessary x11drv_thread_data()->grab_hwnd.

Shaun Ren (10):
      xinput1_3: Remove an unreachable return from HID_set_state().
      xinput1_3: Keep controller read events across device changes.
      xinput1_3: Return correct slot indices for existing devices in find_opened_device().
      xinput1_3: Don't wait on overlapped event if CancelIoEx failed.
      xinput1_3: Clear enabled when controller is destroyed on read error.
      xinput1_3: Return FALSE from controller_init if device enable failed.
      xinput1_3: Disable controller when removing an opened device.
      xinput1_3: Rescan controllers before reporting disconnection to callers.
      xinput1_3: Initialize state with the previous state in read_controller_state.
      xinput1_3: Initialize controller state only on success in controller_init.

Soham Nandy (1):
      ntdll: Ignore hardware breakpoint traps inside the signal stack.

Thomas Portal (1):
      user32: Add a stub for IsImmersiveProcess().

Tobias Markus (1):
      ksecdd.sys: Forward BCrypt functions to bcrypt.dll.

Vibhav Pant (1):
      winebth.sys: Ensure bus relations are invalidated after the PDO has been started by the PnP manager.

Wolfgang Hartl (1):
      oleaut32: Implement SLTG union type processing.

Zhengyong Chen (1):
      windowscodecs: Decode GIF frames on demand.

Zhiyi Zhang (1):
      server: Fix a memory leak in d3dkmt_share_objects.

Ziqing Hui (1):
      dwrite: Fix Miscellaneous Symbols font fallback.
```
