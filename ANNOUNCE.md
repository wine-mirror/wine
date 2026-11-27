The Wine development release 11.23 is now available.

What's new in this release:
  - Support for ARM64 macOS.
  - Bundled vkd3d upgraded to version 2.2.
  - AVX support in 32-bit emulation on ARM64.
  - Better support for generating C++ headers in WIDL.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/11.x/wine-11.23.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-11.23/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 11.23 (total 40):

 - #14797  Pegasus Mail 4.4 to 4.6 SSL enabled POP3 and SMTP doesn't work
 - #19800  cygwin's tar -cz crashes when run from a shell script (32-bit)
 - #19858  cygwin tar upset it can't change timestamps; breaks build of cygwin's bash (32-bit cygwin)
 - #19859  cygwin's rsync -aq --delete-before crashes; breaks cygwin bash build (32-bit cygwin)
 - #21114  Chessbase products - antialiasing has no effect
 - #23192  2D graphics in Mathematica are not antialiased while on Windows they are
 - #54418  gdiplus: SOFTWARE_GdipFillPath is not anti-aliased, making the UI interface invisible
 - #54870  Logos Bible 10 (dotnet 6) crashes trying to load image
 - #56925  msys2-64/cygwin64: unlink of read-only file fails
 - #59056  Kontakt 8: fails to install: "FALSE"
 - #59411  Arknights: Endfield crashes on unimplemented function ntoskrnl.exe.PsGetProcessImageFileName
 - #60407  FEAR 2 crashes in shader_spirv_resource_bindings_init() with the WineD3D Vulkan renderer
 - #60439  SafeDisc driver for The Sims 1 (all versions) failed to create driver L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Secdrv"
 - #60525  FindVUK breaks in Wine 11.20 - SetFilePointer fails on Blu-ray/DVD drive
 - #60527  CUERipper does not work in Wine 11.20 - Drives not detected
 - #60531  RollerCoaster Tycoon 1 and 2: some icons appear black in Load/Save Game dialogs
 - #60542  Keys that produce non-ASCII characters (2/é, 7/è...) don't work as shortcuts in Unreal Engine games
 - #60645  Unable to start Civilization 4 with the game in the drive
 - #60651  http.sys: request header names are matched case-sensitively (lowercase "host:" gives 400 Bad Request)
 - #60683  VirtualDJ 2026 crashes on startup (needs kernel32.IsEnclaveTypeSupported)
 - #60731  Ableton window size issues on 11.21
 - #60735  win32u: with EmulateModeset, some display modes abort the process in make_ratio() (assertion num / d < 65536)
 - #60760  bagpipelayer 1.02 install fails
 - #60768  Heroes of Might and Magic V (GOG) crashes after the intro videos
 - #60781  Cross-compiled gpg2 version 2.5.22 crashes
 - #60783  dbghelp: SymEnumSymbols gives folded functions the wrong name and never returns public symbols
 - #60792  LAS X Office 1.4.11 exits at startup: unimplemented function Win32_Process.GetOwnerSid
 - #60801  Processes randomly crash at startup when the randomized exe address is already used by host mappings
 - #60813  odbc32: SQLDriverConnectW fails with leading semicolon in connection string
 - #60819  Against the Storm: image is too dark with the OpenGL renderer, EGL and EmulateModeset=Y
 - #60823  White rectangles remain in fullscreen borders in multiple games with EmulateModeset=Y
 - #60828  win32u fails to enumerate the display GPU after setupapi lowercases ClassGUID values
 - #60831  Ghost Master no longer fills the screen and a white area appears after mode changes with EmulateModeset=Y
 - #60832  Civilization III: mouse cursor position does not match UI coordinates in fullscreen mode with EmulateModeset=Y
 - #60856  Alcohol 120% version 2.1.1 needs ntoskrnl.exe.ExAllocatePoolWithTagPriority
 - #60862  Mafia III: Definitive Edition doesn't get past intro
 - #60865  Star Trek: Elite Force II: corrupted screen with anti-aliasing and EmulateModeset=Y
 - #60868  wineserver: map/unmap view requests are linear in the number of mapped views
 - #60875  winscard:winscard - SCardGetStatusChangeA() with two reader states returns c0000005 on macOS
 - #60878  winscard:winscard - SCardStatusW() fails with SCARD_E_INVALID_PARAMETER when pcchReaderLen is NULL

### Changes since 11.22:
```
Adam Markowski (1):
      po: Update Polish translation.

Alex Henrie (1):
      ntoskrnl: Add basic implementation of ExAllocatePoolWithTagPriority.

Alexandre Julliard (26):
      ntdll: Implement mitigation options for relocating DLLs.
      server: Update the PE image info for relocated executables on the server side.
      ntdll: Ignore files that don't have a name in get_unix_full_path().
      wine.inf: Don't specify a full path for regedit.exe.
      kernel32/tests: Test that MEM_DECOMMIT clears the permissions completely.
      ntdll: Reset page protections to zero on decommit.
      ntdll: Support unallocated page protections in get_vprot_range_size().
      ntdll: Ignore unallocated page protections when setting protection bits.
      ntdll: Add a separate helper to reset page protections.
      ntdll: Delay allocating page protection tables only the pages are committed.
      makedep: Store arch-independent resource files as native arch.
      makedep: Don't generate rules for resource files in disabled modules.
      ntdll: Support forcing DEP through mitigation options.
      ntdll: Only force exec permissions on 32-bit addresses.
      vkd3d: Import upstream release 2.2.
      ntoskrnl: Use wide character string literals.
      winoldap.mod: Replace the server call by a special case in NtUserWaitForInputIdle.
      ntdll/tests: Don't store %rsp in the same page as the code.
      kernel32/tests: Don't allocate code page as exec+write.
      ntdll: Add workarounds for platforms that enforce W^X memory protection.
      server: Make some functions static.
      server: Store per-process memory views in an rbtree.
      ntdll: Store LDT base address and bits together.
      runas: Support specifying a user without a domain.
      wineboot: Force prefix update if ntdll is for a different architecture.
      makefiles: Don't use explicit image bases.

Alfred Agrell (1):
      kernelbase: Match Windows behavior for StrCmpLogicalW.

Ali Milhim (3):
      wined3d: Remove cached Vulkan graphics pipelines when destroying shader modules.
      wined3d: Don't tear down 3D state on device reset.
      d3d9/tests: Test using shaders after a device reset.

Alistair Leslie-Hughes (11):
      include: Add IMimeObjResolver interface.
      inetcomm/tests: Add some IInternetProtocolInfo_ParseUrl tests.
      inetcomm: Don't prepend mhtml: in MimeOleObjectFromMoniker if it already exists.
      odbc32: Strip semicolon from Connections string.
      odbccp32: Avoid possible overflow for UsageCount.
      odbccp32: Avoid possible overflow when writing registry data.
      propsys: Support marshalling of interfaces.
      wineboot: Only close registry key if it was opened.
      rtworkq: Set output variable in create_async_result.
      rtworkq: Handle memory failure in alloc_work_item.
      rtworkq: Don't assign output on failure in RtwqAddPeriodicCallback.

Aurimas Fišeras (1):
      po: Update Lithuanian translation.

Biswapriyo Nath (1):
      include: Add dxprogrammablecapture.idl.

Brendan Shanks (15):
      winedump: Replace sprintf with snprintf to avoid deprecation warnings on macOS.
      ntdll: Use correct return data type for mach_msg_send().
      server: Use correct return data type for mach_msg(), and use the MACH_PORT_VALID() macro.
      server: Use a non-complex Mach message when sending the task port to the server.
      ntdll: Only enable Rosetta workarounds on x86_64 macOS.
      makefiles: Wrap the loader in a .app bundle on ARM64 macOS.
      makefiles: Allow specifying a provisioning profile to copy into the .app bundle on ARM64 macOS.
      makefiles: Sign macOS ARM64 Wine with needed entitlements at build-time.
      loader: Reserve the low 8GB using PAGEZERO on ARM64 macOS.
      ntdll: On ARM64 macOS, exec the loader in 4k page mode.
      ntdll: In ARM64 syscall dispatcher when on kernel stack, don't use x18.
      ntdll: On ARM64 when returning from a syscall fault, don't use x18.
      ntdll: Add stub init_handler() and leave_handler() to ARM64 signal handlers.
      ntdll: Use custom x18 ABI mode on ARM64 macOS.
      documentation: Add macOS 27 requirement for ARM64.

Connor McAdams (8):
      setupapi/tests: Add a test for SPDRP_CLASSGUID string casing.
      ntoskrnl/tests: Add a test for container ID string casing.
      ntoskrnl/tests: Add a test for SPDRP_BASE_CONTAINERID ASCII behavior.
      setupapi: Lowercase the class device GUID string stored in the registry.
      setupapi: Return a wide string from SetupDiGetDeviceRegistryPropertyA for SPDRP_BASE_CONTAINERID.
      winebus: Make container ID string lowercase.
      dinput/tests: Add some tests for message queue behavior with devices.
      dinput: Use SendMessageTimeout() with SMTO_BLOCK when sending messages to the input thread in most cases.

Conor McCarthy (7):
      mf/session: Delay sample requests on the source stream until the stream is initialised.
      mf/tests: Test topology reuse.
      mfreadwrite: Drain a sample if in EOS state and no responses are available.
      mfreadwrite: Do not respond with ENDOFSTREAM if transforms are present when handling MEEndOfStream.
      mfreadwrite: Respond with the HRESULT if an unexpected error occurs during continued draining.
      mfreadwrite: Drain each transform only after emptying the upstream transform if present.
      mfreadwrite: Clarify a comment.

Dmitry Timoshkov (1):
      Revert "winex11: Set a non-transparent window background pixel color.".

Doug Logan (2):
      wined3d: Don't add buffer views to the buffer object users list when buffer textures are unsupported.
      wined3d: Mark the buffer location valid after clearing a buffer UAV.

Elizabeth Figura (9):
      ntdll/tests: Test FileRenameInformation length validation.
      wow64: Allocate at least sizeof( FILE_RENAME_INFORMATION ).
      ntdll/tests: Test using RootDirectory for files.
      ntdll/tests: Fully test ObjectNameInformation on files.
      ntdll/tests: Test some NT names.
      ntdll/tests: Add a set_rename_information() helper.
      ntdll/tests: Test FileNameInformation with some NT names.
      ntdll/tests: Add a few more path syntax tests.
      ntoskrnl/tests: Test extra backslashes at the beginning of a file name.

Eric Pouech (13):
      cmd: Correctly close handle.
      cmd: Use word iterator for COPY builtin command.
      cmd: Use word iterator for PUSHD builtin command.
      cmd: Don't store depth in pushd stack.
      cmd: Use word_iterator for POPD builtin command.
      cmd: Use word_iterator for CD builtin command.
      winedump: Simplify some code.
      dbghelp/pdb: Simplify S_PROCREF handling.
      include: Fix a method declaration in ID2D1Device interface.
      widl: Only output 'using' statement once per method.
      include: Fix IDCompositionGaussianBlurEffect interface declaration.
      widl: Support reference in type declaration.
      include: Use references in dcomp.idl as SDK does.

Fabien Romano (2):
      ntdll: Reserve space for the thread name terminator on FreeBSD.
      loader: Load ntdll.so with RTLD_NODELETE.

Francis De Brabandere (1):
      vbscript: Implement IActiveScript::InterruptScriptThread.

Georg Lehmann (1):
      winevulkan: Update to VK spec version 1.4.369.

Gopal Prasad (1):
      ntdll: Skip missing page protection tables in get_vprot_range_size.

Hans Leidekker (9):
      server: Set user SID as owner in admin token.
      wbemprox: Perform a proper lookup in process_get_owner().
      wbemprox: Implement Win32_Process.GetOwnerSid().
      comdlg32: Add support for the OpenSave MRU list.
      odbc32: Match native connection string format.
      odbc32: Fix length passed to driver connection entry points.
      winscard: Pack reader_state_macos struct.
      winscard: Accept NULL name_len in SCardStatusA/W().
      include: Remove duplicate defines.

Helen Koike (1):
      winex11: Unconditionally select clip region in client_surface_present.

Jacek Caban (22):
      ntdll/tests: Allow an additional WOW64_CPU_AREA_INFO field and allow an extended i386 context.
      include: Update XSTATE_CONFIGURATION and KUSER_SHARED_DATA declarations to current Windows version.
      include: Add ARM64 SVE and SME struct declarations.
      ntdll: Use CONTEXT_EX size and alignment matching recent Windows 11.
      ntdll: Store xstate flag in context_parameters.
      ntdll: Use context_get_parameters for more checks in RtlCopyContext.
      ntdll: Return XSTATE_CONFIGURATION from context_get_parameters.
      ntdll: Pass XSTATE_CONFIGURATION to XState size and offset helpers.
      ntdll: Use the new shared data XState config on ARM64.
      ntdll: Support ARM64 extended context in XState helper functions.
      ntdll/tests: Don't assume x86 context layout in test_copy_context.
      ntdll/tests: Run test_copy_context on ARM64.
      ntdll/tests: Add tests for copying ARM64 context.
      ntdll: Allocate space for XState in i386 wow64 CPU area.
      ntdll: Implement the new version of RtlWow64GetCpuAreaInfo.
      ntdll: Initialize XState in wow64 CPU area.
      ntdll: Support extended x86 wow64 context on ARM64.
      wow64: Use extended context from CPU area in call_user_exception_dispatcher.
      ntdll/tests: Fix context flag check on Windows with SVE support enabled.
      ntdll/tests: Skip 0xdeadbeef handle closing exception continuation test on ARM64.
      ntdll: Expose fewer cpsr bits to the user space.
      ntdll: Don't apply cpsr_user_mask to 32-bit ARM contexts.

Jason A. Donenfeld (3):
      user32/tests: Test WM_PAINT on layered windows.
      server: Preserve pending repaints for layered windows.
      ldap: Only clean up Winsock if it was initialized.

Marc-Aurel Zent (8):
      server: Stop key repeats on foreground change.
      winemac: Handle keyboard auto repeats on the server side.
      user32/tests: Test reclaiming the foreground window before processing a foreground loss.
      win32u: Ignore a foreground loss if the thread has the foreground back.
      winemac: Add a send_notify_message helper.
      winemac: Remove WINDOW_LOST_FOCUS and APP_DEACTIVATED events.
      winemac: Remove WINDOW_GOT_FOCUS event.
      ntdll: Implement NtYieldExecution() with swtch() on macOS.

Matteo Bruni (1):
      dsound: Fix mono to 7.1 mixing.

Max Artemovskiy (1):
      ntdll: Support arbitrary serial baud rates on macOS.

Nikolay Sivov (38):
      d3d10/tests: Compile annotations test effect from source.
      d3d10/tests: Compile type class test effect from source.
      d2d1/tests: Fix test crashes on older platforms.
      dwrite: Add missing IUnknown handling to fontset's QI().
      dwrite: Make sure to return null output on failing IDWriteFontResouce::QI().
      dwrite: Remove unused field from the font data structure.
      dwrite/tests: Add some edge case tests for ReadFileFragment().
      dwrite: Fix offset validation in ReadFileFragment() for the local file stream.
      dwrite: Fix offset validation in ReadFileFragment() for the in-memory file stream.
      dwrite: Fix offset validation in ReadFileFragment() for the GDI memory resource stream.
      dwrite: Improve failure handling when creating color glyph enumerator objects.
      dwrite: Fix a leak on error path when creating EUDC collections.
      dwrite: Use better return value for the IDWriteFontFaceReference::Equals() stub.
      dwrite/layout: Fix possible use-after-free on failure.
      dwrite/tests: Add a test for rendering mode validation in CreateCustomRenderingParams().
      dwrite: Validate rendering mode argument in CreateCustomRenderingParams().
      d2d1: Fix appending rounded rectangles to the geometry group path.
      d2d1: Fix simplified representation of rounded rectangles.
      d2d1: Fix incorrect transposition in arc-to-bezier conversion.
      d2d1: Always represent arcs with beziers when they have to be transformed.
      d2d1: Transform streamed output for groups.
      dwrite: Use internal helper to create fontface references.
      gdi32: Fix allocation size for the EMRI_DEVMODE handler.
      gdi32/tests: Extend SetTextCharacterExtra() tests.
      gdi32: Fix SetTextCharacterExtra() return value on failure.
      gdi32/emfdc: Use DeleteDC() to release HDC objects.
      gdi32/emfdc: Fix a typo in bounds update in PlgBlt().
      wpnapps: Add IToastNotificationManagerStatics2 stub.
      wpnapps: Add a stub toast history object.
      kernel32: Add a stub for IsEnclaveTypeSupported().
      dwrite/tests: Add some more tests for GetKerningPairAdjustments().
      dwrite: Improve argument validation in GetKerningPairAdjustments().
      dwrite/opentype: Correctly read header fields as 32-bit values for Format 10 CMAP.
      dwrite: Fail data table access validation on zero-size reads.
      dwrite/opentype: Fix off-by-one issue with last mapped codepoint in CMAP Format 6 and 10.
      dwrite/opentype: Fix SequenceLookup access in contextual lookups.
      dwrite: Remove duplicated OS/2 table access.
      dwrite/opentype: Improve Format 4 CMAP validation.

Octavia Togami (2):
      user32/tests: Test FlashWindowEx() from another thread.
      win32u: Don't wait for the window's thread in FlashWindowEx().

Olivier Cochard-Labbé (1):
      wineoss.drv: Prefer 32-bit float as the shared mode mix format.

Owen Rudge (2):
      hh: Build as ARM64EC.
      iexplore: Build as ARM64EC.

Paul Gofman (13):
      kernelbase: Cache TZ info for year.
      user32/tests: Add tests for 0xf020 space character in DrawTextExW().
      user32: Handle space in symbol charset (0xf020) in DrawTextExW().
      ntdll: Factor out handle_syscall_opcode() function.
      ntdll: Handle int 0x2e on x64 and wow64.
      opengl32: Only resolve draw framebuffer in blit_framebuffer_surface().
      opengl32: Resolve default FBO and flush context for glReadnPixels().
      opengl32: Only resolve anything in resolve_default_fbo() if current FBO is default.
      opengl32: Fix resolve_default_fbo GL_FRAMEBUFFER_SRGB restore conditions.
      opengl32: Resolve what is currently configured for read in resolve_default_fbo().
      ntdll: Move IP_RECVTOS / IPV6_RECVTCLASS handling to the server.
      ntdll: Fix message length handling in wow64_translate_control().
      ws2_32: Implement IP_RECVECN / IPV6_RECVECN socket options.

Piotr Caban (3):
      win32u: Convert LB_GETTEXT returned string in call_messageAtoW.
      win32u: Convert CB_GETLBTEXT returned string in call_messageAtoW.
      win32u/tests: Return correct length in CB_GETLBTEXT tests.

Reyka Matthies (4):
      mf/tests: Add network bytestream tests.
      mf: Add HTTP/HTTPS IMFByteStream implementation.
      mf: Implement IMFAttributes in network bytestream.
      mf: Implement and use HttpSchemePlugin and SecureHttpSchemePlugin.

Rémi Bernon (51):
      win32u: Return the GL error instead of setting it.
      win32u: Introduce an fd_desc struct for D3DKMT fd metadata.
      win32u: Check the actual fd type when importing fds into OpenGL.
      win32u: Check the actual fd type when importing fds into Vulkan.
      win32u: Avoid recreating internal GL contexts unnecessarily.
      win32u: Silent an error when resizing single sampled framebuffers.
      win32u: Export some OpenGL textures to back swapchain resources.
      win32u: Import acquired swapchain image as an OpenGL texture.
      win32u: Synchronize swapchain image acquire / present operations.
      win32u: Invalidate remote swapchains when windows are reparented.
      win32u: Blit the framebuffer to the acquired remote swapchain image if any.
      win32u: Implement remote OpenGL swapchain image presentation.
      server: Use a bool for window topmost state changes.
      server: Use helpers to check WS_(VISIBLE|MINIMIZED|DISABLED|POPUP|CHILD).
      server: Use some style access helpers for Ws_EX_(LAYERED|TRANSPARENT).
      server: Use some style access helpers for WS_EX_LAYOUTRTL.
      server: Use some style access helpers for WS_CLIP(SIBLINGS|CHILDREN).
      server: Write the window styles to the shared memory.
      winex11: Clear the window padding with display mode emulation.
      server: Fix old / new visible condition before updating cursor pos.
      win32u: Invalidate toplevel windows when changing display modes.
      server: Use unsigned int for PE-side DPI values.
      win32u: Rename raw_dpi and monitor_dpi variables to "scale".
      win32u: Check window surface scale num / den separately.
      win32u: Split map_monitor_rect into virt_to_raw / raw_to_virt.
      win32u: Always return the effective DPI from monitor_get_dpi.
      win32u: Add a separate helper to return window monitor scale.
      win32u: Use an unsigned integer for integral DPI values.
      win32u: Restore client context after destroying framebuffer.
      win32u: Always use single sampled depth buffer for the read FBO.
      server: Avoid some return void call patterns.
      server: Use the window_info struct in create_window.
      server: Use int for monitor fractional scale ratios.
      server: Sync children windows monitor DPI and scale.
      widl: Split C++ method generation to separate helpers.
      widl: Generate vtable method prefixes for base class methods.
      widl: Generate wrappers and macros for overriden methods.
      widl: Generate C++ using statements for overriden methods.
      widl: Group overloaded method statements together.
      widl: Generate unique overloaded method names.
      dwrite: Use overloaded method names and order in dwrite_3.idl.
      include: Use overloaded method names and order in dcomp.idl.
      widl: Generate C++ overloaded methods in MSVC-native order too.
      win32u: Use case-insensitive compare when checking registry paths.
      server: Cast value to __int64 to avoid unsigned promotion.
      win32u: Map driver DC coordinates to raw monitor DPI.
      win32u: Avoid leaking GDI handles when setting window icons.
      opengl32: Return early from resolve_default_fbo if there's nothing to do.
      win32u: Always call window_surface_set_clip with a rect if there's a region.
      winemac: Use the monitor rect as backing size when emulating display modes.
      winemac: Workaround modifications of the GL context viewport when setting views.

Selim Bucher (2):
      msi/tests: Test saving a database containing just under 2^16 unique strings.
      msi: Ignore unused slots at the end of the string table when saving.

Sky Leite (1):
      ntdll: Avoid redundant NtYieldExecution and select() in NtDelayExecution.

Stefan Dösinger (9):
      wined3d: Create an access helper for GL texture views.
      wined3d: Implement swapchain RTVs for the GL backend.
      wined3d: Implement swapchain RTVs for the Vulkan backend.
      wined3d: Rotate through swapchain views.
      d3d11/tests: Add a test for rgb<->sRGB CopyResource.
      d3d9/tests: Test the impact of sRGB writing on multisample resolve.
      d3d11/tests: Test sRGB conversion on presentation.
      d3d11/tests: Extend multisample resolve sRGB tests.
      wined3d: Control FRAMEBUFFER_SRGB in blits.

Stian Low (1):
      d3d11/tests: Test sRGB RTVs on swapchains with more than one buffer.

Valentin Bernard (1):
      winex11: Return unshifted characters as is with MAPVK_VK_TO_CHAR.

Vibhav Pant (3):
      widl: Fix unintended fallthrough in merge_memberrefs.
      widl: Sort metadata tables after building them.
      widl: Merge the associated Constant row (if any) while merging a Field row.

Vishnunithyasoundhar S (1):
      msvcrt/scanf: Replace _EOF_RET with -1.

Vitaly Lipatov (15):
      fonts: Remove the broken glyphs from the Tahoma Bold bitmap strikes.
      fonts: Embolden the Tahoma Bold bitmap strike glyphs copied from Tahoma.
      fonts: Fix the width of some Tahoma Bold bitmap strike glyphs.
      fonts: Remove the broken glyphs from the Tahoma bitmap strikes.
      fonts: Redraw the Cyrillic glyphs of the 8 pixel Tahoma bitmap strike.
      fonts: Narrow the Tahoma bitmap strike glyphs wider than on Windows.
      include: Declare the flat GDI+ API used by libgdiplus tests.
      gdi32/tests: Test recording SetICMMode in enhanced metafiles.
      gdi32: Record SetICMMode in enhanced metafiles.
      msi/tests: Test directory resolution of installed components in maintenance installs.
      msi: Keep installed components in their registered location.
      msi/tests: Test MsiGetComponentPathEx() with a NULL buffer.
      include: Add default and ARGB constructors for GDI+ Color.
      mpr/tests: Test that the network provider order keys exist.
      wine.inf: Create the network provider order keys.

Zhiyi Zhang (3):
      user32/tests: Test the palette of 8-bit DIBs created from a 8-bit display DC.
      win32u: Fill the palette for 8-bit DIBs created from an 8-bit display DC.
      ntdll: Pack SIDs together in NtCreateToken().
```
