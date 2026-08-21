The Wine development release 11.16 is now available.

What's new in this release:
  - Mono engine updated to version 11.3.0, including ARM64 support.
  - Hardware video decoding using VA-API.
  - Improved exception handling on ARM64EC.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/11.x/wine-11.16.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-11.16/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 11.16 (total 35):

 - #19160  Orly's Draw-A-Story demo crashes on startup (dmDriverExtra is not initialized)
 - #26040  Odell Down Under Demo: Pagefault on read access when moving fish around the map
 - #29451  pmd85 emulator - toolbar icons missing
 - #32783  Siemens Automation License Manager installer (part of SIMATIC STEP 7 Lite SP4) fails to start ALM service (Microsoft Enhanced DSS and Diffie-Hellman Cryptographic Provider missing)
 - #33600  Klik and Play crash on modify / create game option
 - #35732  Adobe CC (Creative Cloud) installer crashes inside msxml3
 - #40526  "Pettersson und Findus" does not start
 - #40945  Post provider setup of WMI core 1.5 installer crashes in wbemprox
 - #42062  silhouette studio : installation frozen
 - #42226  No ingame sfx audio when CD Audio playing with builtin dsound in Ignition
 - #46392  Commandos: Behind Enemy Lines videos aren't played
 - #46559  Visual C++ Build Tools 2015 hangs on install
 - #47785  CERT_CHAIN_POLICY_IGNORE_NOT_TIME_VALID_FLAG not taken into account
 - #49792  pfx import does not works
 - #49837  Just Grandma and Me (win16) softlock. Win16Mutex wait timed out
 - #50824  ISdone.dll error 5, something releated to insufficient virtual memory
 - #52201  Backpacker crashes on launch
 - #56565  Command and Conquer 3 Tiberium Wars installer crashes
 - #56887  Total Commander ftps connection (TLS1.2)
 - #57669  Suspicious behavior of rundll32.exe
 - #58530  regedit /e should exit silently on failure
 - #58698  Application goes into an infinite loop under new wow64 but works okay under old wow64
 - #59642  Stratego (1997): Jittery mouse pointer / cursor
 - #59726  Syscall emulation on Linux using Syscall User Dispatch broken with glibc < 2.34
 - #59757  SteelSeries GG 110.0 crashes on startup in .NET System.Security.Cryptography.X509Certificates.StorePal.Export
 - #59977  Acrobat Reader DC locks up opening a PDF directly with "wine some.pdf"
 - #60029  Broken cursor in Star Citizen inventory + terminal menu
 - #60125  Yabridge overlaping windows on some WM produce dead zone for input
 - #60141  CertCreateSelfSignCertificate with default parameters makes insecure RSA 512 cert
 - #60142  CertGetCertificateContextProperty does not support CERT_SIGN_HASH_CNG_ALG_PROP_ID
 - #60152  WineWayland driver doesn't work anymore in Wine 11.15
 - #60158  Steam fails to launch
 - #60162  WPF apps crash on startup (x86 only?)
 - #60164  WebView2 mouse hit testing is vertically offset under X11
 - #60198  X11 fullscreen borderless non-decorated window's surface offset by Wine's window borders

### Changes since 11.15:
```
Alex Henrie (2):
      riched20: Set value to 0 in TextPara_GetAlignment.
      krnl386: Respect the MOVEABLE flag in AllocResource16.

Alexandre Julliard (23):
      ntdll: Export the helper function to allocate object attributes.
      server: Use standard object attributes for window stations and desktops.
      configure: Make the non-PE build an error unless --without-mingw is specified.
      user32: Pass valid security descriptors when creating user objects.
      user32: Implement Get/SetUserObjectSecurity().
      server: Don't check access rights on newly-created desktop.
      server: Set the object name parent also for desktops.
      server: Use the root directory as winstation handle in create_desktop.
      ntdll: Ignore dlls of the wrong architecture during bootstrap.
      ntdll: Force running wineboot.exe with the native architecture.
      win32u: Force running explorer.exe with the native architecture.
      wineboot: Force running services.exe and rundll32.exe with the native architecture.
      server: Return correct status on overflow in desktop_get_full_name().
      wow64: Enforce a valid low limit in extended memory parameters.
      ntdll: Don't try to allocate past the specified limits in map_free_area().
      ntdll: Reserve some space for top-down allocation in large address aware mode.
      ntdll: Avoid overflow in NtQueryVirtualMemory() in old wow64 mode.
      ntdll: Set a 32-bit limit on cross-process allocations in old wow64 mode.
      kernel32/tests: Move the large page test to virtual.c.
      kernelbase: Fetch the large page minimum size from the user shared data.
      kernel32/tests: Add some tests for MEM_PHYSICAL and MEM_LARGE_PAGES.
      ntdll: Add parameter checks for MEM_LARGE_PAGES in NtAllocateVirtualMemory().
      ntdll: Support the MEM_PHYSICAL flag in NtAllocateVirtualMemory().

Alistair Leslie-Hughes (1):
      wintrust: Add parameter check in WTHelperGetProvCertFromChain.

Aric Stewart (2):
      coreaudio: Report USB device PKEY_Device_InstanceId.
      mmdevapi: Propagate the hardware Device_InstanceId.

Bernhard Übelacker (2):
      winhttp: Avoid heap-use-after-free with WINHTTP_OPTION_SERVER_CBT.
      wow64win: Add structure conversions in wow64_NtUserGetPointerInfoList.

Brendan McGrath (18):
      windows.gaming.input: Allow gamepad added callback to be registered before initializing providers.
      quartz/tests: Add test stub for color space converter.
      quartz/tests: Add test for filter registration.
      quartz/tests: Add interface tests for colorconv.
      quartz/tests: Add aggregation test.
      quartz/tests: Add enum pin test.
      quartz/tests: Add find pin test.
      quartz/tests: Add pin info test.
      quartz/tests: Add media types test.
      quartz/tests: Add enum media types test.
      quartz/tests: Add unconnected filter state test.
      quartz/tests: Add connect pin test.
      quartz/tests: Add sink allocator test.
      quartz/tests: Add filter state test.
      quartz/tests: Add sample processing test.
      quartz/tests: Test content of image.
      quartz/tests: Test dynamic format change.
      quartz/tests: Add streaming events test.

Brendan Shanks (3):
      ntdll: NtQuerySystemInformation(SystemProcessorBrandString) is only supported on i386 and x86_64.
      ntdll: Populate the CPU name and vendor on ARM64 macOS.
      ws2_32/tests: Remove faulty inet_addr() test.

Carl-Friedrich Braun (2):
      rsaenh: Use SymCryptAesKeyCopy() when duplicating AES keys.
      rsaenh/tests: Test that a duplicated AES key encrypts identically.

Conor McCarthy (11):
      bcrypt/tests: Test non-HMAC BCryptHash() with a secret.
      bcrypt/tests: Test BCryptDeriveKey() with secret buffers.
      bcrypt: Return INVALID_PARAMETER from BCryptHash() if the input secret is not used.
      bcrypt: Handle prepended secret buffers in BCryptDeriveKey() for KDF_HASH.
      wbemdisp/tests: Test ISWbemObjectPath::DisplayName.
      wbemdisp/tests: Test ISWbemObject set enumeration.
      wbemdisp: Handle null output pointer in enumvar_Next().
      wbemdisp: Add ISWbemObjectPath stub implementation.
      wbemdisp: Partially implement get ISWbemObjectPath::DisplayName.
      wbemdisp: Implement ISWbemObject::GetTypeInfo().
      wbemdisp: Return null object on failure of QueryInterface().

Deep Agrawal (2):
      kernelbase: Implement ReadConsoleInputEx.
      kernelbase: Reimplement PeekConsoleInput and ReadConsoleInput on top of ReadConsoleInputEx.

Elizabeth Figura (25):
      xaudio2: Release the correct lock on failure in CreateSourceVoice().
      wined3d: Introduce a stub VA decoder backend.
      wined3d: Allocate the bitstream on CPU for the VA backend.
      win32u: Enable VK_EXT_physical_device_drm and VK_EXT_external_memory_dma_buf.
      wined3d: Query VA for H.264 support.
      wined3d: Create a VA decoding session.
      wined3d: Share VA surfaces with Vulkan.
      wined3d: Implement VA H.264 decoding.
      mountmgr: Pass the device type to IoCreateDevice().
      mountmgr: Pass characteristics to IoCreateDevice().
      mountmgr: Rename the "serial" field of struct disk_device to "disk_serial".
      mountmgr: Consistently use "unix_device" for variables representing the Unix block device path.
      mountmgr: Move unix_mount to struct volume.
      mountmgr: Document the difference between struct disk_device and struct volume.
      mountmgr: Rename harddisk_* to disk_*.
      mountmgr.sys: Handle IRP_MJ_CREATE.
      ndis: Handle IRP_MJ_CREATE.
      nsiproxy: Handle IRP_MJ_CREATE.
      winebth: Handle IRP_MJ_CREATE.
      mouhid: Handle IRP_MJ_CREATE.
      wined3d: Pass a VkImageCreateInfo to wined3d_context_vk_create_image().
      d3d11: Plumb resource sharing flags to wined3d.
      d3d11: Validate shared resource flags.
      wined3d: Make WINED3D_TEXTURE_GENERATE_MIPMAPS a usage flag.
      wined3d: Try to perform CPU copy in wined3d_texture_update_sub_resource().

Eric Pouech (2):
      cmd: Revert AI generated code.
      widl: Handle boolean as template parameter.

Esme Povirk (2):
      appwiz.cpl: Move architecture to addon_info_t.
      mscoree: Update Wine Mono to 11.3.0.

Francisco Casas (1):
      wined3d: Don't call vulkan functions with 0 attachments on wined3d_context_vk_update_blend_state().

Gabriel Ivăncescu (1):
      mshtml: Don't leak the attribute after forwarding in setAttributeNode.

Hans Leidekker (1):
      crypt32: Support CertGetCertificateContextProperty(CERT_SIGN_HASH_CNG_ALG_PROP_ID).

Henri Verbeet (1):
      d3dcompiler_46: Return E_NOINTERFACE from D3DReflect() for unsupported interfaces.

Hugo Osvaldo Barrera (1):
      winex11: Fix use-after-free in GL_EXTENSIONS string.

Ivan Ivlev (9):
      comctl32/tests: Test wrapping for fixed size toolbar and autosized buttons.
      comctl32/toolbar: Add TOOLBAR_AutoSizeButtonWidth from LayoutToolbar as a separate function.
      comctl32/toolbar: Use TOOLBAR_AutoSizeButtonWidth in WrapToolbar.
      comctl32/toolbar: Remove redundant toolbar wrapping logic.
      ntdll/tests: Test NtAllocateVirtualMemory user address limit.
      ntdll/tests: Test NtMapViewOfSection user address limit.
      ntdll/tests: Test NtAllocateVirtualMemoryEx user address limit.
      ntdll/tests: Test NtMapViewOfSectionEx user address limit.
      ntdll: Validate virtual memory ranges against the user address limit.

Jacek Caban (7):
      winegcc: Disable safeseh by default.
      ntdll: Fix CONTEXT_ARM64_X18 handling when setting another thread's context.
      ntdll: Move KiUserEmulationDispatcher setup to syscall dispatcher exit code.
      ntdll: Setup KiUserEmulationDispatcher in usr1_handler when needed.
      ntdll: Set InSimulation before returning to KiUserEmulationDispatcher from the kernel side.
      ntdll: Don't use entry thunk context in RtlRaiseException.
      ntdll: Preserve exception flags when converting between x64 and arm64 context flags.

Ken Sharp (3):
      regedit: /e should exit silently if key not found.
      ntdll: Remove debug_init declaration.
      ntdll: Avoid sign extension when parsing MaxVersionTested.

Matteo Bruni (1):
      win32u: Fix get_locale_data() table entry lookup.

Nikolay Sivov (19):
      oleaut32/tests: Add a test for Invoke() with typeinfo from CreateDispTypeInfo().
      oleaut32/typelib: Make it possible to Invoke() through COCLASS type.
      msxml3: Remove commented out leftover line.
      msxml3/tests: Add one more test for document dirty state.
      msxml3/tests: Add some tests for supported interfaces for XMLHTTP objects.
      comctl32/tests: Add more tests for TCM_SETCURFOCUS.
      comctl32/tests: Remove trace noise from the Tab tests.
      comctl32/tab: Ignore negative index TCM_SETCURFOCUS for TCS_BUTTONS controls.
      comctl32/tests: Add a Tab test for switching focus from -1 value.
      comctl32/tab: Fix switching focus from unset state.
      comctl32/tab: Update item pressed state on focus change.
      d3d10/tests: Add some state block capture tests with disabled fields.
      d3d10/stateblock: Fix mask checks for byte-sized fields.
      d3dx10/sprite: Check for the Begin/End state in affected methods.
      d3dx10/tests: Add some tests for the sprite state saving feature.
      d3dx10/sprite: Partially implement state restore functionality.
      d3dx10/sprite: Collect sprites on DrawSpritesBuffered().
      include: Add some matrix functions prototypes to d3dx10math.h.
      d3dx10/tests: Add some sprites rendering tests.

Paul Gofman (20):
      ntdll: Implement NtQuerySystemInformation( SystemNumaProcessorMap ).
      kernelbase: Implement GetNumaHighestNodeNumber().
      kernel32: Implement GetNumaProcessorNode[Ex]().
      kernel32: Add FIXME to GetNumaProximityNode() stub.
      kernelbase: Implement GetNumaNodeProcessorMask[Ex]().
      kernelbase: Add FIXME to GetNumaProximityNodeEx().
      ntdll/tests: Remove spurious pointer advance in test_query_numa_map().
      ntdll: Check iosb for FSCTL_GET_OBJECT_ID in open_dll_file().
      quartz/tests: Add tests for implicit fullscreen support by filter graph.
      quartz: Implement BaseControlWindowImpl_GetRestorePosition().
      quartz: Introduce fullscreen mode emulation in filtergraph.
      quartz: Implement entering and leaving emulated fullscreen mode.
      ntdll: Add WAYLAND_DISPLAY to ignored variables list.
      msvcrt: Introduce asm wrapper for _isatty on x64.
      winhttp: Support WINHTTP_QUERY_FLAG_NUMBER64 with WINHTTP_QUERY_CONTENT_LENGTH.
      msvcrt: Always set WX_TEXT flag when importing std handles.
      msvcrt: Initialize std handles in a loop in msvcrt_init_io().
      msvcrt: Don't update system std handles in msvcrt_init_io().
      msvcrt: Also inherit WX_ATEOF and WX_READNL attributes.
      ntdll: Treat 'reached_thread_count > total_thread_count' the same as equal in RtlBarrier().

Piotr Caban (18):
      include: Add SECPKG_CALL_* definitions.
      secur32: Don't convert authentication identity in lsa_AcquireCredentialsHandleA.
      secur32: Handle memory allocated with lsa_AllocateClientBuffer in FreeContextBuffer.
      msv1_0: Use GetCallInfo to obtain thread and process id.
      msv1_0: Fix client buffer handling in ntlm_SpQueryCredentialsAttributes.
      msv1_0: Fix client buffer handling in ntlm_SpQueryContextAttributes.
      kerberos: Handle SEC_WINNT_AUTH_IDENTITY_EX in SpAcquireCredentialsHandle.
      kerberos: Use LSA_SECPKG_FUNCTION_TABLE to call Lsa functions.
      kerberos: Fix client buffer handling in kerberos_SpQueryContextAttributes.
      secur32: Pass unmodified authentication data from nego_SpAcquireCredentialsHandle().
      msv1_0: Fix auth_data client buffer handling in ntlm_SpAcquireCredentialsHandle.
      kerberos: Fix auth_data client buffer handling in kerberos_SpAcquireCredentialsHandle.
      msv1_0: Map SecBuffer data before accessing it in ntlm_SpInitLsaModeContext and ntlm_SpInitLsaModeContext.
      kerberos: Map SecBuffer data before accessing it in kerberos_SpInitLsaModeContext and kerberos_SpAcceptLsaModeContext.
      secur32: Initialize user mode handle in lsa_InitializeSecurityContextW.
      secur32: Initialize user mode handle in lsa_AcceptSecurityContext.
      secur32: Don't access Lsa mode data in negotiate user mode functions.
      msv1_0: Don't access Lsa mode data in user mode functions.

Ralf Habacker (2):
      ntdll: Fix some SIMD exception codes.
      ntdll/tests: Add SIMD exception test for floating point overflow operation fault.

Rémi Bernon (37):
      win32u: Avoid a race condition when releasing / detaching a client surface.
      win32u: Keep unused client surfaces around and reuse them if possible.
      opengl32: Move wglCopyContext implementation to the PE side.
      opengl32: Keep vendor / renderer strings in opengl_client_context.
      opengl32: Keep context version string in opengl_client_context.
      opengl32: Move debug callback / user pointers to opengl_client_context.
      opengl32: Optionally return client context from get_current_context.
      opengl32: Move unix-side context wrapper allocation to win32u.
      win32u: Remove wgl functions that aren't doing anything.
      user32/tests: Cleanup AttachThreadInput tests.
      user32/tests: Test that thread input attachments are refcounted.
      server: Move desktop check out of attach_thread_input.
      server: Pass desktop instead of thread to create_thread_input.
      server: Assume thread queues are valid in attach_thread_input.
      user32/tests: Improve reparenting tests with different DPI awareness.
      server: Check DPI awareness contexts when reparenting windows.
      server: Don't change DPI awareness context when reparenting.
      win32u: Avoid calling monitor_dpi_from_rect when creating the desktop window.
      server: Return the visible rect from get_window_rectangles request.
      server: Use the receiving window thread for set_window_rect_visible.
      opengl32: Set glTable before calling init_client_context.
      win32u: Allocate a proper opengl_context struct for internal contexts.
      win32u: Use toplevel visible rect relative position for client surfaces.
      win32u: Latch the client surface sizes in the opengl_drawable struct.
      opengl32: Initialize the scissor box when making context current.
      opengl32: Restore read and draw buffers when popping default FBOs.
      win32u: Move framebuffer_surface_resize into framebuffer_surface_flush.
      win32u: Introduce helpers to resize / destroy framebuffer attachments.
      server: Update both shared input at once in assign_thread_input.
      server: Update shared input keystate in assign_thread_input.
      server: Update focus and active windows in assign_thread_input.
      server: Detach the parent window thread input when reparenting.
      server: Implement thread input attachments refcounting.
      opengl32: Wrap glGetFramebufferParameteriv separately from its extensions.
      opengl32: Ignore the currently bound FBO in glGetFramebufferParameterivEXT.
      winex11: Use the client surface monitor rect for offscreen present.
      winex11: Allow child D3D presentation on fullscreen exclusive toplevel.

Rüdiger Lenz (1):
      winex11: Preserve fractional raw mouse motion.

Santino Mazza (1):
      d2d1: Implement DrawSpriteBatch() command list recording.

Stefan Dösinger (3):
      wined3d: Don't delay-clear backbuffers except to black.
      d3d11/tests: Test delayed clear RGB/sRGB mismatch.
      d3d10/tests: Synchronize test_swapchain_views with d3d11.

Thomas Portal (1):
      winebus.sys: Report USB class compatible IDs.

Tobiasz Laskowski (10):
      jscript/tests: Ensure windows run uses jscript.dll.
      comctl32/tests: Check multicolumn listbox redraw.
      user32/tests: Check multicolumn listbox redraw.
      comctl32/listbox: Fix SetRedraw top item handling.
      user32/listbox: Fix SetRedraw top item handling.
      gdiplus/tests: Check drawimage without attributes.
      gdiplus/metafile: Allow drawimage without attributes.
      gdiplus/tests: Check drawimage corner colors.
      gdiplus/metafile: Fix emf+ image rendering.
      gdiplus/tests: Clean up converged cases.

Zhiyi Zhang (12):
      jscript: Separate keyword searching from parsing.
      jscript: Support Unicode characters in identifiers.
      jscript/tests: Add ES3 identifier tests.
      mshtml/tests: Add ES5 identifier tests.
      dinput/tests: Test setting two actions with the same control identifier.
      dinput: Check device GUID in init_object_app_data().
      winemac.drv: Hide client_view when flushing window surfaces.
      winex11.drv: Allow ConfigureRequest events when the window is maximized.
      mmdevapi: Use MulDiv() to avoid rounding errors.
      winecoreaudio: Set the requested period frame size when creating audio streams.
      winecoreaudio: Add a helper to get device sample rate.
      winecoreaudio: Return the actual Core Audio device period frame size.
```
