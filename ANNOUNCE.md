The Wine development release 11.11 is now available.

What's new in this release:
  - Bundled SymCrypt library, to replace TomCrypt.
  - More USER32 window information moved to shared memory.
  - Layered windows in the Wayland driver.
  - More VBScript compatibility improvements.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/11.x/wine-11.11.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-11.11/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 11.11 (total 25):

 - #27579  The appearance of uTorrent 3.x torrents list is broken
 - #32079  Dotted (.foobar) files are not displayed in file open dialog
 - #37849  Wine causing color temperature setting(redshift) not working for a short time
 - #41134  Foxit Reader 8.x service 'FoxitConnectedPDFService.exe' crashes on startup due to invalid database permissions ('ConvertStringSecurityDescriptorToSecurityDescriptor' SDDL / ACL parser must take whitespace between ACEs into account)
 - #49622  error in installing battle.net launcher
 - #49722  Space Empires V can't load certain images (.bmp) with built-in d3dxof.dll
 - #50583  Foxit PhantomPDF Business v10.0 installer crashes in MSI custom action 'SetStartMenuWindows10T' with WinVer set to 'Windows 10'
 - #50763  Wreckfest has blurry rendering with OpenGL renderer
 - #51433  Battle.Net: Installer terminates with BLZBNTBTS0000005C error screen
 - #51567  Battle.net app takes a long time to start
 - #53293  Total War: Shogun 2 crashes with builtin d3dx9_42
 - #53653  winmine.exe is not scaled properly to match the output DPI
 - #54189  Fail in SetupDiEnumDeviceInfo: err=259: No more data available.
 - #54694  Marvel's Spider-Man Remastered requires Windows 10 version 1909, build 18363 (or newer)
 - #55230  Yesterday Origins shows black screen while playing some videos
 - #56398  Think or Swim running in WINE, loads, then crashes, and need to be shut down.
 - #57806  visual and response lags in Guitar Pro v8.1.3 (build 121)
 - #58297  [Wayland-only mode] Minimize button click makes the app window buttons lock up
 - #59125  Sunlogin crashes on unimplemented function advapi32.dll.WmiCloseBlock
 - #59453  uSimmics crashes on unimplemented function msvcp100.dll._Sinh
 - #59549  MS-Money 2000 - shows errors when loading INV7.OCX
 - #59604  setupapi: CM_Get_Parent stub breaks USB HID device identification
 - #59807  Regression in multithreaded application
 - #59841  Missing audio in Gray Matter due to FAudio 26.06 import
 - #59845  Istaria: game window fails to appear since Wine 11.10

### Changes since 11.10:
```
Alexandre Julliard (35):
      configure: Re-generate with autoconf 2.73.
      libs: Import upstream code from SymCrypt 103.11.0.
      rsaenh: Use SymCrypt for hash algorithms.
      rsaenh: Use SymCrypt for AES, DES and RC2.
      symcrypt: Add pclmul target for older compilers.
      ntdll: Use SymCrypt for SHA hashes.
      ntdll: Get crc32 implementation from zlib instead of tomcrypt.
      configure: Don't try to link to external zlib.
      makefiles: Look for dependencies in assembly files.
      makefiles: Only build asm files in PE builds.
      makefiles: Use a separate directory for ARM64EC asm files.
      symcrypt: Import upstream assembly code for x86-64.
      symcrypt: Import upstream assembly code for ARM64.
      symcrypt: Import upstream assembly code for ARM.
      symcrypt: Import upstream assembly code for i386.
      faudio: Import upstream release 26.06.
      mpg123: Import upstream release 1.33.5.
      png: Import upstream release 1.6.58.
      lcms2: Import upstream release 2.19.1.
      sqlite3: Import upstream release 3.53.2.
      sqlite3: Disable debug assertions.
      rsaenh: Align allocations for SymCrypt.
      dssenh: Align allocations for SymCrypt.
      notices: Add a file to collect all third-party copyright notices.
      makedep: Add a helper function to skip an initial directory.
      makedep: Also ignore missing headers when included from external lib header.
      fluidsynth: Remove unused files.
      gsm: Remove unused files.
      icucommon: Remove unused files.
      icui18n: Remove unused files.
      ldap: Remove unused files.
      vkd3d: Remove unused files.
      xml2: Remove unused files.
      faudio: Don't fixup wma bitrate if it's already valid.
      ntdll: Support either a Unix call function table or a __wine_unix_lib_init entrypoint.

Alistair Leslie-Hughes (1):
      include: Add IInkRecognizerContext coclass.

Anton Baskanov (5):
      dsound: Use DWORD to store freqAdjustNum, freqAdjustDen and freqAccNum.
      dsound: Move cp_fields_noresample after cp_fields_resample.
      dsound: Use #define for fir.h constants.
      dsound: Add an SSE version of upsample.
      dsound: Add an SSE version of downsample.

Bernhard Kölbl (2):
      win32u: Add missing internal messages to spy.
      win32u: Add win32u internal driver messages to spy.

Bernhard Übelacker (9):
      crypt32/tests: Avoid crash in testCertProperties.
      rsaenh: Use _aligned_free instead of free for aligned allocations (ASan).
      dssenh: Use _aligned_free instead of free for aligned allocations (ASan).
      dsound/tests: Don't fail test if device cannot be opened the second time.
      webservices/tests: Improve double tests.
      user32/tests: Fix test_window_extra_data_proc.
      windows.ui.core.textinput/tests: Only check interfaces if retrieval succeeded.
      imagehlp/tests: Accept zero checksum if the compiler emitted a zero checksum.
      kernel32/tests: Fix tests with old Windows 10 versions.

Charlotte Pabst (3):
      include: Add some missing definitions in mfd3d12.idl.
      mfplat/tests: Add tests for mf d3d12 synchronization object.
      mfplat: Implement mf d3d12 synchronization object.

Conor McCarthy (6):
      quartz: Update current and elapsed positions in Stop().
      d3d8/tests: Test multisampled back buffer locking.
      d3d9/tests: Test multisampled back buffer locking.
      wined3d: Disallow locking of multisampled back buffers.
      mfplat/tests: Test CreateObjectFromByteStream() URL with unknown or no extension.
      mfplat: Do not require CONTENT_DOES_NOT_HAVE_TO_MATCH_EXTENSION_OR_MIME_TYPE if no extension is present.

Dmitry Timoshkov (9):
      msi: Catch all exceptions in the custom DLL procedure.
      msi: Return ERROR_INSTALL_FAILURE on an exception in the custom action.
      ole32/tests: Add some tests for IStorage::MoveElementTo().
      ole32: Implement IStorage::MoveElementTo() for stream element.
      ole32: Implement IStorage::MoveElementTo() for storage element.
      widl: Add support for enums to SLTG typelib generator.
      widl: Add support for unions to SLTG typelib generator.
      sane.ds: Add more traces.
      sane.ds: SANE_DisableDSUserInterface() should accept states 5 and 6.

Elizabeth Figura (11):
      srclient: No longer prefer native.
      wevtapi: No longer prefer native.
      ninput: No longer prefer native.
      msctf: No longer prefer native.
      msctfmonitor: No longer prefer native.
      quartz/tests: Add a few more seeking tests.
      directmanipulation: No longer prefer native.
      feclient: No longer prefer native.
      d3d8thk: No longer prefer native.
      shcore: No longer prefer native.
      userenv: No longer prefer native.

Esme Povirk (1):
      comdlg32: Treat empty default extension as null.

Etaash Mathamsetty (3):
      winewayland: Do not place_above on a detached client surface.
      winewayland: Treat maximized+fullscreen state as fullscreen.
      winewayland: Implement min/max size hints for non-resizeable windows.

Fatih Uzunoglu (1):
      mscms: Add stub for `WcsCreateIccProfile()`.

Francis De Brabandere (20):
      scrrun/tests: Add tests for dictionary put_Key.
      scrrun/dictionary: Rename keys in place in put_Key.
      vbscript: Bind local variables and arguments at compile time.
      vbscript: Factor out assign_local_var from assign_ident.
      vbscript: Bind local variable assignments at compile time.
      vbscript: Factor out do_for_step and do_incc from interp_step and interp_incc.
      vbscript: Bind For-loop variables at compile time.
      vbscript: Bind class properties at compile time.
      scrrun/tests: Add tests for Empty dictionary keys.
      scrrun/dictionary: Match Empty keys against zero-valued and empty-string keys.
      scrrun/tests: Add tests for Boolean dictionary keys.
      scrrun/dictionary: Match native key semantics for Boolean keys.
      vbscript/tests: Add tests for class declaration scope.
      vbscript: Handle class declaration scope in the parser.
      vbscript/tests: Add tests for sub declaration scope.
      vbscript: Lex a dot immediately followed by a digit as a numeric literal.
      vbscript: Report specific compile errors for trailing tokens and missing identifiers.
      vbscript: Allow Dim to shadow a global const from a previous compile unit.
      vbscript/tests: Add tests for one-line sub headers with multi-line bodies.
      vbscript: Keep the offending identifier out of Err.Description.

Francisco Casas (3):
      win32u: Enable VK_KHR_external_fence_capabilities in d3dkmt_init_vulkan().
      wined3d: Enable VK_KHR_external_fence_capabilities when initializing vulkan.
      wined3d: Don't use texelFetchOffset() for buffers and multi-sample textures.

Gabriel Ivăncescu (1):
      ieframe: Return S_FALSE for NULL URLs in WebBrowser_Navigate.

Georg Lehmann (1):
      winevulkan: Update to VK spec version 1.4.353.

Hans Leidekker (9):
      winhttp: Revert recursion limit to 3.
      rsaenh: Use SymCrypt for RSA and RC4.
      dssenh: Use SymCrypt.
      bcrypt: Use SymCrypt for hashes.
      tomcrypt: Remove library.
      bcrypt: Use SymCrypt for keys.
      bcrypt: Get rid of the unixlib.
      bcrypt: Add initial support for CHACHA20_POLY1305.
      bcrypt: Add support for Brainpool curves.

Ivan Lyugaev (1):
      secur32: Return SEC_E_INVALID_TOKEN for invalid TLS record headers.

Jacek Caban (12):
      jscript/tests: Remove unused variables.
      vbscript/tests: Remove unused variables.
      ucrtbase/tests: Link directly to environment functions.
      msvcp140_1/tests: Remove unused variable.
      msvcp60/tests: Remove unused variable.
      ddraw/tests: Remove unused variable.
      ddraw: Remove unused variable.
      ntdll: Synthesize caller context in ARM64EC RtlRaiseException.
      ieframe/tests: Add missing CHECK_CALLED statements.
      scrobj/tests: Remove unused variables.
      wpcap/tests: Remove unused variables.
      shcore/tests: Remove unused variables.

Jiangyi Chen (2):
      winewayland.drv: Add alpha-modifier-v1 protocol to support change alpha by WAYLAND_SetLayeredWindowAttributes.
      winewayland.drv: Add WAYLAND_UpdateLayeredWindow and add support for changing alpha using the alpha-modifier-v1 protocol.

Kellen Kopp (1):
      win32u: Fixed incorrect glyph outline size.

Marc-Aurel Zent (3):
      winemac: Remove unused includes in vulkan.c.
      winemac: Factor out Metal device and view logic into MetalViewSwapChain.
      winemac: Implement cross-process MetalViewSwapChain via CALayerHost.

Martin Storsjö (2):
      ntdll: Fix building for arm64 without HWCAP_CPUID defined.
      ntdll: Fix building for arm64 without ESR_MAGIC/esr_context.

Matteo Bruni (4):
      server: Fix numpad vkey handling.
      winex11: Return the layout index from from detect_keyboard_layout().
      winex11: Create layouts when falling back to fuzzy detection.
      configure: Add -msse2 to default i386_EXTRACFLAGS.

Nikolay Sivov (5):
      msxml3/writer: Use wchar literals in more places.
      msxml3/tests: Add more tests for the writer output.
      msxml3/tests: Use writer output test helper throughout the tests.
      msxml3/tests: Avoid string conversion in a few places.
      msxml3/writer: Normalize newlines in CDATA content.

Paul Gofman (8):
      ddraw: Renamed is_root surface attribute to is_implicit.
      ddraw: Allow attaching flip chain surfaces.
      ddraw/tests: Add tests for attaching and detaching backbuffers on ddraw1.
      ddraw: Force render target bind flags on v1 swapchain candidate surfaces.
      kernelbase: Add stub for GetIntegratedDisplaySize().
      dsound: Fail IDirectSoundCaptureBufferImpl_Lock() for invalid buffer range.
      ddraw/tests: Remove extra backbuffer1 release in test_surface_attachment().
      win32u: Always consider newly created shape updated in set_surface_shape().

Piotr Caban (3):
      msxml3: Don't add newline before first element when indentation is enabled.
      msxml3: Don't indent endElement since it changes text node content.
      include: Define more runtimeclasses in windows.storage.streams.idl.

Rose Hellsing (6):
      wineboot: Create SQMClient MachineId registry key.
      ntdll: Account for read-only cookie in loader.
      ntoskrnl.exe: Account for read-only cookies.
      kernel32/tests: Add test for read-only cookie in loader.
      riched20: Properly capture \pntext content.
      riched20/tests: Expand editor tests to verify \pntext behaviour.

Russell Greene (1):
      hhctrl: Return a nonzero cookie from HH_INITIALIZE.

Rémi Bernon (43):
      server: Keep track of builtin class FNID on registration.
      server: Introduce a get_class_fnid helper.
      user32: Set builtin class FNID for menu controls.
      user32: Set builtin class FNID for MDI client controls.
      user32: Set builtin class FNID for dialog controls.
      win32u: Set builtin class FNID on window creation.
      winex11: Move find_xkb_layout_variant helper around.
      opengl32: Allocate display lists for client-side contexts.
      opengl32: Share display lists in wglShareLists on the PE side.
      opengl32: Query the context profile mask on init.
      opengl32: Track client-side buffer names allocation.
      opengl32: Track implicit buffer names allocation.
      user32: Store a scroll_info pointer in the scrollbar control info.
      user32: Set FNID for the builtin scrollbar window class.
      opengl32: Call get_integer hook after a successful unix call.
      opengl32: Add get_integer hooks to indexed glGet functions.
      opengl32: Pass the returned unix value to get_integer hooks.
      opengl32: Hook more glGet functions with get_integer.
      opengl32: Implement client / host mapping for buffer names.
      win32u: Introduce a server_set_window_info helper.
      server: Move window extra data to the shared memory.
      win32u: Return value directly from get_window_extra.
      server: Move window id / menu to the shared memory.
      server: Move window instance to the shared memory.
      server: Move window user data to shared memory.
      server: Move class local flag to the shared memory.
      server: Keep track of class window proc ansi nature.
      win32u: Simplify winproc handle to index conversions.
      win32u: Move window procedure to the shared memory.
      opengl32: Implement client / host mapping for framebuffer names.
      opengl32: Allow framebuffer implicit allocation in some core contexts.
      opengl32: Implement client / host mapping for renderbuffer names.
      opengl32: Allow renderbuffer implicit allocation in some core contexts.
      opengl32: Implement client / host mapping for texture names.
      opengl32: Ignore more cases in make_opengl get_object_type.
      opengl32: Implement client / host mapping for sampler names.
      opengl32: Implement client / host mapping for display list names.
      opengl32: Implement client / host mapping for program names.
      opengl32: Implement client / host mapping for vertex shader names.
      opengl32: Implement client / host mapping for fragment shader names.
      opengl32: Implement client / host mapping for semaphore names.
      opengl32: Implement client / host mapping for memory names.
      opengl32: Implement client / host mapping for path names.

SeongChan Lee (1):
      winexinput.sys: Fix handling of non-32-bit axis.

Soham Nandy (1):
      ntdll: Preserve nonvolatile registers across VEH callbacks.

Steven Don (2):
      atl: Implement registering ATL window messages.
      atl/tests: Add tests for proper handling of WM_ATLGETCONTROL and WM_ATLGETHOST.

Thibault Payet (1):
      ntdll/unix: Always define get_current_thread_data.

Tim Clem (1):
      hnetcfg: Report 0 returned elements from IEnumVARIANT::Next.

Twaik Yont (1):
      wineandroid: Raise minimum SDK version to 28 (Android 9.0).

Vibhav Pant (2):
      combase/tests: Add tests for delayed marshaling of objects with apartment affinity.
      combase: Implement delayed marshaling for RoGetAgileReference.

Wolfgang Hartl (1):
      ir50_32: Export DllRegisterServer and DllUnregisterServer.

Yuxuan Shui (2):
      mf/tests: Check frame size of video processor's output type.
      winegstreamer: Video processor shouldn't have a preferred size.

Zhengyong Chen (1):
      win32u: Fix signed coordinate extraction in drag_drop_call.
```
