The Wine development release 11.13 is now available.

What's new in this release:
  - Better support for input pointers.
  - Improved keyboard scancode mapping on X11.
  - FFmpeg optimizations on ARM64EC.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/11.x/wine-11.13.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-11.13/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 11.13 (total 22):

 - #13088  Adobe Acrobat Pro 7 / Acrobat Reader 7 -- Comments don't work
 - #30824  msvbvm60.dll crashes Gehalt.exe
 - #36373  Requiem: Avenging Angel - Black screen in-game
 - #40231  Grand Theft Auto: Vice City crashes at start
 - #42282  Tales of Zestiria v1.2 (latest Steam provided runtime) crashes at launch
 - #45503  msiexec crashes during Office 2000 setup
 - #49435  cyrrillic text are broken in winbox64.exe
 - #50030  "Hayoola" freezes in wine 5.19 while it works great in 5.13
 - #55030  Nelogica Profitchart freezes and Crash since wine-7.12 [bisect included]
 - #55407  Virtual desktop option breaks wine creating windows
 - #58105  explorer creates extra virtual desktop when using the /desktop=shell argument
 - #58610  Sim Racing Studio fails to start: “Error Loading Prython DLL ”C:\Progam Files\RimsRacingStudio 2.0\python312.dll"
 - #59727  DrawText rendering bug when DT_WORD_ELLIPSIS / DT_PATH_ELLIPSIS is used
 - #59850  Exit unwind is broken
 - #59884  Age of Mythology errors on startup with "Initialization failed"
 - #59914  One i386 uwninding testcase fails with wine built with GCC
 - #59916  d2d1 path geometry leaks figure->vertex_types (steady heap growth under sustained Direct2D path rendering)
 - #59920  msvcrt setjmp/longjmp in x86_64 ELF builds broken by ntdll change
 - #59932  Slingplayer 2's help has an extra window
 - #59960  Sony Acid Pro 7.0 crashes on startup during initialization
 - #59969  winecfg buttons can't be clicked on dpi != 96
 - #59975  SuccubusHeaven camera breaks

### Changes since 11.12:
```
Alex Henrie (1):
      faudio: Handle odd offsets in decode_mono_adpcm_block.

Alexandre Julliard (6):
      ntdll: Fix handling of exit unwind when there are no TEB frames.
      msvcrt: Save all registers when calling the exception filter on i386.
      ffmpeg: Add support for ARM64EC assembly.
      ffmpeg: Add ARM64EC assembly in libswresample.
      ffmpeg: Add ARM64EC assembly in libswscale.
      make_announce: Use the new Bugzilla REST API.

Alexey Prokhin (1):
      win32u: Enable fullscreen clipping even if not already clipping.

Alistair Leslie-Hughes (14):
      dplayx: Add missing break.
      fltmgr.sys: Add FltAllocateContext stub.
      fltmgr.sys: Add FltReleaseContext stub.
      fltmgr.sys: Add FltGetStreamContext stub.
      fltmgr.sys: Add FltSetStreamContext stub.
      fltmgr.sys: Add FltQueryInformationFile stub.
      fltmgr.sys: Add FltGetFileNameInformationUnsafe stub.
      fltmgr.sys: Add FltReadFile stub.
      fltmgr.sys: Add FltParseFileNameInformation stub.
      fltmgr.sys: Add FltReleaseFileNameInformation stub.
      fltmgr.sys: Add FltGetFileNameInformation stub.
      fltmgr.sys: Add FltCreateCommunicationPort stub.
      fltmgr.sys: Add FltCloseCommunicationPort stub.
      fltmgr.sys: Add FltEnumerateVolumes stub.

Anna (navi) Figueiredo Gomes (9):
      win32u: Move process_pointer_message to input.c.
      win32u: Keep per-thread list of known pointers.
      win32u: Preallocate pointerId 1 for the mouse pointer.
      win32u: Keep track of pointer types.
      win32u: Implement NtUserGetPointerType.
      win32u: Hardcode pointer id 1 to PT_MOUSE in GetPointerType.
      win32u: Update pointerid 1 from mouse_in_pointer events.
      server: Mask pointer inputs for SetCursorPos requests.
      win32u: Implement NtUserGetPointerInfoList.

Anton Baskanov (7):
      dmsynth: Increase the write latency by 10 ms.
      dsound: Name the parameters of bitsgetfunc and bitsputfunc.
      dsound: Remove getieee32 from the getbpp array.
      dsound: Get rid of endianness conversion.
      dsound: Downmix to mono after resampling and remove get_mono.
      dsound: Get rid of get_aux and read the values directly.
      dsound: Get all channel samples in one go.

Benoît Legat (2):
      crypt32: Avoid conflict between unnamed containers.
      crypt32: Open MS_ENH_RSA_AES_PROV in PFXImportCertStore.

Bernhard Übelacker (1):
      secur32/tests: Update count for new test.winehq.org certificate.

Brendan Shanks (4):
      ntdll: Advertise ARM32 processor features on ARM64.
      ntdll: Detect ARM32 processor features using getauxval(AT_HWCAP).
      ntdll: Detect ARM64 processor features on macOS, and on Linux using getauxval(AT_HWCAP).
      ntdll: Define HWCAP_CPUID on ARM64.

Conor McCarthy (11):
      windows.storage/tests: Test InMemoryRandomAccessStream.
      windows.storage: Add InMemoryRandomAccessStream stubs.
      windows.storage: Implement InMemoryRandomAccessStream IClosable::Close().
      windows.storage: Implement InMemoryRandomAccessStream IOutputStream::WriteAsync().
      windows.storage: Implement InMemoryRandomAccessStream IRandomAccessStream::get_Size().
      windows.storage: Implement InMemoryRandomAccessStream IRandomAccessStream::put_Size().
      windows.storage: Implement InMemoryRandomAccessStream IRandomAccessStream::get_Position().
      windows.storage: Implement InMemoryRandomAccessStream IRandomAccessStream::Seek().
      windows.storage: Implement InMemoryRandomAccessStream IInputStream::ReadAsync().
      windows.storage: Add InMemoryRandomAccessStream to class registration.
      windows.storage: Do not limit the size to SIZE_MAX in memory_stream_random_access_put_Size().

Daniel Lehman (1):
      dwrite: Add some more fallback ranges.

Elizabeth Figura (18):
      wined3d: Store plane_formats as wined3d_format pointers.
      wined3d: Allow any typed format for planar texture views.
      wined3d: Do not alter the swapchain latency.
      wined3d: Use a semaphore for frame latency.
      dxgi: Implement DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT for d3d11.
      dxgi: Implement d3d11_swapchain_SetMaximumFrameLatency().
      dxgi: Implement d3d11_swapchain_GetMaximumFrameLatency().
      dxgi: Implement d3d11_swapchain_GetFrameLatencyWaitableObject().
      acledit: No longer prefer native.
      avrt: No longer prefer native.
      connect: No longer prefer native.
      inkobj: No longer prefer native.
      magnification: No longer prefer native.
      mscat32: No longer prefer native.
      msnet32: No longer prefer native.
      msports: No longer prefer native.
      mssip32: No longer prefer native.
      netprofm: No longer prefer native.

Erhan Bilgili (1):
      cfgmgr32: Fix CM_Get_Device_ID buffer size handling.

Eric Pouech (1):
      dbghelp/dwarf: Fix out-of-bound write when leaf name is too large.

Etaash Mathamsetty (4):
      winex11: Don't initialize win_data_mutex twice.
      winewayland: Don't initialize win_data_mutex twice.
      winewayland: Use a flag instead of checking zwp_relative_pointer_v1.
      winewayland: Don't offset client rect in client surface reconfigure.

Francis De Brabandere (1):
      vbscript: Count carriage returns as line endings when reporting error lines.

Gabriel Ivăncescu (7):
      mshtml: Don't leak gecko attr when creating attribute.
      mshtml: Don't leak ref from attribute collection when detaching attribute.
      mshtml: Don't leak gecko attribute when obtaining DISPID from collection.
      mshtml: Properly unlink the element refs of frame elements.
      mshtml: Properly unlink the element refs of iframe elements.
      mshtml/tests: Fix attribute leak when testing tabIndex.
      mshtml/tests: Properly close the view of the legacy document when testing attributes across modes.

Haoyang Chen (1):
      ole32: Add support for writing VT_BOOL properties.

Jacek Caban (4):
      dsound/tests: Remove unused variable.
      qcap/tests: Remove unused variable.
      ws2_32/tests: Remove unused variable.
      http.sys: Check thread_stop in request_thread_proc loop.

Jacob Czekalla (1):
      hhctrl.ocx: Create the index popup window hidden.

Jiajin Cui (6):
      wininet/tests: Cover long CreateUrlCacheEntryW extensions.
      urlmon/tests: Cover URLDownloadToCacheFile buffer bounds.
      wininet: Ignore overly long cache file extensions.
      urlmon: Fix URLDownloadToCacheFile buffer handling.
      urlmon: Remove temporary cache files on failure.
      winewayland: Fix y coordinate of IME cursor rectangle.

Louis Lenders (3):
      kernel32: Add stub for PssQuerySnapshot.
      kernel32: Add stub for PssFreeSnapshot.
      kernel32: Add stub for PssCaptureSnapshot.

Maotong Zhang (1):
      user32: Fix lastSlash handling in TEXT_PathEllipsify.

Martin Storsjö (1):
      configure: Reduce the fixed load address for the preloader on aarch64.

Matteo Bruni (4):
      winex11: Fix Right Shift scancode.
      user32/tests: Add a SendInput VK_PAUSE test.
      user32/tests: Add more key names tests.
      user32/tests: Add more key map tests.

Nello De Gregoris (1):
      mfsrcsnk: Ignore empty byte stream origin names.

Nikolay Sivov (4):
      msxml3/xpath: Add a strstr() variant that handles nulls.
      msxml3/xpath: Add a string comparison helper that handles nulls.
      msxml3/tests: Add a test for translate() function.
      msxml3/xpath: Fix translate() function to actually return its result.

Paul Gofman (4):
      ws2_32: Add WSCEnumProtocols32().
      kernel32/tests: Test filename case preservation when opening existing file.
      kernelbase: Add a stub for FindNextFileNameW().
      mmdevapi: Implement SAORS_Reset().

Piotr Caban (3):
      secur32: Return impersonation token from lsa_QuerySecurityContextToken.
      msv1_0: Decrypt all data buffers in ntlm_SpUnsealMessage.
      gdiplus: Return InvalidParameter error when GdipLoadImageFromStream is used with empty stream.

Piotr Pawłowski (2):
      comctl32/tests: Add tests for TVN_ITEMCHANGING and TVN_ITEMCHANGED.
      comctl32/treeview: Implemented TVN_ITEMCHANGING and TVN_ITEMCHANGED.

Ryan Houdek (1):
      wow64: Add BTCpuNotifyProcessExecuteFlagsChange interface.

Rémi Bernon (53):
      winewayland: Make window lock mutex recursive.
      winewayland: Use update_client_surfaces to detach client surfaces.
      winewayland: Remove unnecessary client surface attach / detach.
      winewayland: Move ensure_window_surface_contents into wayland_surface.c.
      winex11: Use the desired state for mapping delays.
      server: Map update_window_zorder rect from raw to virtual coordinates.
      server: Update window zorder directly in queue_mouse_message.
      wineandroid: Remove now unnecessary update_window_zorder call.
      winemac: Remove now unnecessary update_window_zorder call.
      winex11: Remove now unnecessary update_window_zorder call.
      win32u: Keep track of client surface toplevel window.
      win32u: Move client_surface_update on present from winex11.
      win32u: Move client surface rect computation out of the drivers.
      win32u: Assume that NtUserExposeWindowSurface rect is in raw monitor DPI.
      win32u: Reorder KEYEVENTF_SCANCODE handling control flow.
      win32u: Remap some scancodes to better match native behavior.
      winex11: Use scancode high bit to set KEYEVENTF_EXTENDEDKEY flag.
      winex11: Support fixed X11 keycode to scancode conversion.
      opengl32/tests: Test versioned / core / compat context creation and sharing.
      winemac: Remove unnused macdrv_context renderer_id member.
      opengl32: Remove duplicated client context member.
      winex11: Remove unnecessary window send_mouse_input argument.
      winex11: Pass individual INPUT fields to send_mouse_input.
      winex11: Pass / return a POINT to / from map_event_coords.
      winex11: Use send_mouse_input in move_resize_window.
      winex11: Use send_mouse_input in the RawMotion handler.
      win32u: Use internal helpers to get present / client rects and dpi.
      win32u: Introduce a ratio struct for unix-side DPI values.
      server: Use the ratio struct for DPI values.
      win32u: Represent raw monitor dpi as rational value.
      server: Move window monitor DPI to the shared memory.
      win32u: Ignore empty mouse input in NtUserSendHardwareInput.
      server: Skip dispatching rawinput message when it is empty.
      win32u: Accumulate mouse motion in NtUserSendHardwareInput.
      winex11: Remove now unnecessary mouse motion event merging.
      winemac: Try initializing GL3_Core and GL4_Core profile contexts.
      winemac: Create all core contexts with the best supported profile.
      winemac: Use WGL_CONTEXT_CORE_PROFILE_BIT_ARB to detect core profiles.
      win32u: Request core profile for internal OpenGL contexts.
      winemac: Ignore context sharing when profile doesn't match.
      opengl32: Flag GL_EXT_external_buffer extension as unsupported.
      opengl32: Move more wow64 wrappers out of the generated code.
      opengl32: Use a static variable for the buffer storage map.
      opengl32: Remove unused flags member from buffer struct.
      opengl32: Reduce WOW64 locking to the buffer storage map accesses.
      server: Always update window monitor DPI from its parent.
      winewayland: Remove now unnecessary implicit fullscreen clipping.
      server: Support explicit raw mouse motion in NtUserSendHardwareInput.
      winewayland: Send raw mouse motion through NtUserSendHardwareInput.
      winex11: Enable XI2 raw events when window is focused.
      winex11: Send raw mouse motion frames from XI2 RawEvents.
      winex11: Remove unnecessary ConfigureNotify event merging.
      winex11: Get rid of now unnecessary event merging logic.

Stefan Riesenberger (2):
      dnsapi: Add stub for DnsStartMulticastQuery().
      dnsapi: Add stub for DnsStopMulticastQuery().

Sven Baars (2):
      configure: Set PCSCLITE_LIBS in the AC_CHECK_FUNC error case on macOS.
      dxgi: Replace FIXMEs in swapchain latency methods by traces.

Thomas Portal (1):
      hidclass: Return the full report length from IOCTL_HID_WRITE_REPORT.

Zhiyi Zhang (14):
      dxgi/tests: Wait for destroyed windows to disappear before creating a fullscreen swapchain.
      dxgi/tests: Run test_create_swapchain() on D3D12 as well.
      dxgi/tests: Test creating a fullscreen swapchain with an invalid scanline order or scaling.
      dxgi: Report the DXGI_SWAP_CHAIN_FULLSCREEN_DESC used for creating a D3D11 swapchain.
      dxgi: Create a windowed swapchain instead when the fullscreen mode scanline order or scaling is invalid.
      dxgi: Refuse to enter fullscreen when the fullscreen mode scanline order or scaling is invalid.
      dxgi: Retrieve the fullscreen state from the swapchain state in d3d11_swapchain_GetFullscreenState().
      user32/tests: Move the test window position further away from the top-left corner of the work area.
      user32/tests: Fix test failures when the top-left corner of the work area is not (0,0).
      user32/tests: Add more window placement tests.
      win32u: Implement coordinates mapping in {Get,Set}WindowPlacement().
      notepad: Use SetWindowPlacement() to restore the saved normal window position.
      taskmgr: Use SetWindowPlacement() to restore the saved normal window position.
      wordpad: Use SetWindowPlacement() to restore the saved normal window position.
```
