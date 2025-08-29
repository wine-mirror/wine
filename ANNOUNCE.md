The Wine development release 10.14 is now available.

What's new in this release:
  - Bundled vkd3d upgraded to version 1.17.
  - Mono engine updated to version 10.2.0.
  - Support for ping on IPv6.
  - Gitlab CI now running on Debian Trixie.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/10.x/wine-10.14.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-10.14/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 10.14 (total 19):

 - #18233  Approach underscore bar inactive with multiple database open
 - #27974  warn:winsock:wsaErrno errno 115, (Operation now in progress).
 - #35622  VemsTune: program crashes on switching view modes
 - #55557  wpcap:wpcap crashes on macOS
 - #56639  Phantasy Star Online: Blue Burst: various missing/black textures
 - #57027  GetFinalPathNameByHandleW does not handle paths exceeding MAX_PATH (260 chars)
 - #57835  ROCS Show Ready crashes on unimplemented function msvcp140_atomic_wait.dll.__std_tzdb_get_time_zones
 - #57946  Multiple games need maxAnisotropy values handling (GreedFall, Mafia III: Definitive Edition)
 - #58141  [MDK] [WOW64] Stack overflow
 - #58169  Trae installer fails: "Failed to expand shell folder constant userpf"
 - #58334  ShowStopper crashes on unimplemented function ntdll.dll.RtlQueryProcessHeapInformation
 - #58403  Death to Spies: intro videos show black screen (audio works)
 - #58482  Roblox Studio installer crashes on unimplemented function api-ms-win-core-memory-l1-1-3.dll.VirtualProtectFromApp
 - #58531  MemoryRegionInformation incorrectly returns STATUS_SUCCESS for freed memory regions
 - #58574  Multiple applications require gameinput.dll (Fritz Chess Coach, Le Mans Ultimate)
 - #58577  MsiGetComponentPath/MsiLocateComponent doesn't resolve a reference to .NET GAC
 - #58600  Command line tab completion works improperly with files/directories containing delimiter characters
 - #58608  SCardTransmit should work with pioSendPci=NULL
 - #58615  winepath changes behaviour and strips ending path separator now

### Changes since 10.13:
```
Adam Markowski (2):
      po: Update Polish translation.
      po: Update Polish translation.

Alexandre Julliard (39):
      tapi32: Move registry keys out of wine.inf.
      win32u: Add NtGdiCancelDC() stub.
      win32u: Implement NtUserGetCursorPos().
      win32u: Implement NtGdiGet/SetMiterLimit().
      server: Add a helper to check a process wow64 status.
      include: Always use Unicode string constants for the PE build.
      server: Use LIST_FOR_EACH_ENTRY in more places.
      cabinet: Use the correct structure for DllGetVersion().
      rsaenh: Don't reset key when nothing was encrypted.
      winebuild: Only allow thiscall functions on i386.
      server: Fix get_next_hook return value when no hook is found.
      kernel32: Preserve trailing slash for existing paths in wine_get_dos_file_name().
      kernel32: Preserve trailing slash for existing paths in wine_get_unix_file_name().
      win32u: Define all stubs as syscalls.
      vkd3d: Import upstream release 1.17.
      sxs: Add support for language in manifest names.
      sxs: Support XML escaping in manifest names.
      sxs: Take manifest language into account when building the file name.
      sxs: Install policy files the same way as normal manifests.
      win32u: Disable some unused code when EGL is missing.
      gitlab: Update CI image to debian trixie.
      setupapi: Use SetupDiGetActualSectionToInstallW instead of duplicating that logic.
      setupapi: Add support for Include directive.
      setupapi: Add support for Needs directive.
      wine.inf: Use Needs directive to reduce duplication.
      winedump: Remove const from a member that is written to.
      kernel32/tests: Clear FPU status flags before checking control word.
      ntdll/tests: Also test mxcsr register in user callbacks.
      ntdll/tests: Use a direct syscall to test xmm registers.
      gdi32/tests: Remove some workarounds for NT4.
      gdi32: Handle the default color profile on the GDI side.
      gdi32: Handle the ICM\mntr key on the GDI side.
      winex11: Create the ICM profile file at startup.
      win32u: Remove the __wine_get_icm_profile() syscall.
      win32u: Remove the GetICMProfile driver entry point.
      amstream/tests: Use nameless unions/structs.
      qcap: Use nameless unions/structs.
      qedit: Use nameless unions/structs.
      winegstreamer: Use nameless unions/structs.

Alexandros Frantzis (2):
      winewayland: Mark only windows with per-pixel alpha as layered.
      winewayland: Handle NULL values for xkb layout name and description.

Anders Kjersem (1):
      advpack: Support ADN_DEL_IF_EMPTY flag in DelNode().

Attila Fidan (1):
      winegstreamer/wma_decoder: Return S_OK from AllocateStreamingResources().

Aurimas Fišeras (2):
      po: Update Lithuanian translation.
      po: Update Lithuanian translation.

Bernhard Übelacker (8):
      ntdll/tests: Mark test as broken with old Windows versions.
      ntdll: Fix XState data initialisation with non-AVX CPUs.
      kernel32/tests: Test paths in GetFinalPathNameByHandleW exceeding MAX_PATH.
      kernel32: Handle paths in GetFinalPathNameByHandleW exceeding MAX_PATH.
      bluetoothapis/tests: Avoid crash in gatt tests with some Windows versions.
      kernelbase/tests: Fix test failing with old Windows 10.
      ntdll/tests: Add check for len and avoid CommitSize with old Windows.
      propsys/tests: Skip tests of properties not supported by old Windows.

Brendan McGrath (1):
      winegstreamer: Only add the capsfilter for avdec_h264.

Brendan Shanks (1):
      ntdll: Implement NtGetCurrentProcessorNumber() with pthread_cpu_number_np() when available on macOS.

Connor McAdams (8):
      ntoskrnl/tests: Add tests for getting IRP_MN_QUERY_DEVICE_TEXT based device properties.
      ntoskrnl: Set DEVPKEY_Device_BusReportedDeviceDesc from driver.
      winebus: Handle IRP_MN_QUERY_DEVICE_TEXT.
      hidclass: Print a warning for unhandled IRP_MN_QUERY_DEVICE_TEXT text types.
      winebth.sys: Print a warning for unhandled IRP_MN_QUERY_DEVICE_TEXT text types.
      wineusb.sys: Print a warning for unhandled IRP_MN_QUERY_DEVICE_TEXT text types.
      winebus: Generate unique container IDs when adding devices.
      winebus: Override device instance enumerator string if bus type is known.

Conor McCarthy (1):
      winegstreamer: Use a stride alignment of 2 for NV12 in align_video_info_planes().

Dmitry Timoshkov (3):
      comdlg32: PRINTDLG_UpdatePrintDlgW() should update dmCopies field in DEVMODE.
      kernel32/tests: Test FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_POSIX_SEMANTICS | FILE_ATTRIBUTE_DIRECTORY.
      kernelbase: For FILE_FLAG_BACKUP_SEMANTICS also handle FILE_ATTRIBUTE_DIRECTORY in CreateFile.

Elizabeth Figura (3):
      qcap/tests: Test subtype validation in QueryAccept() and SetFormat().
      quartz/tests: Add more tests for video window style.
      quartz: Preserve the current visibility in IVideoWindow::put_Style().

Esme Povirk (1):
      mscoree: Update Wine Mono to 10.2.0.

Gabriel Ivăncescu (4):
      jscript: Allow objects that expose "length" prop for Function.apply under certain conditions.
      jscript: Return proper error when passing wrong type to Function.apply.
      jscript: Fallback to Object's toString for Arrays when 'this' isn't an array in ES5 mode.
      jscript: Fix error value when passing non-string 'this' to String's toString.

Georg Lehmann (3):
      winevulkan: Reorder bitmasks to handle aliases correctly.
      winevulkan: Disable h265 extensions.
      winevulkan: Update to VK spec version 1.4.325.

Gerald Pfeifer (1):
      winebus.sys: Use uint16_t instead of __u16.

Hans Leidekker (11):
      msi: Make assembly caches global.
      msi: Handle .NET assemblies in MSI_GetComponentPath().
      rsaenh: Use TomCrypt for hash implementations.
      include: Comment references to undefined static interfaces.
      include: Define IRandomAccessStreamStatics.
      include: Define ISystemMediaTransportControlsStatics.
      widl: Require static interfaces to be defined.
      widl: Require activation interfaces to be defined.
      widl: Require composition interfaces to be defined.
      widl: Require runtimeclass contracts to be defined.
      winscard: Handle NULL send parameter in SCardTransmit().

Haoyang Chen (1):
      qcap/vfwcapture: Validate the subtype in find_caps().

Henri Verbeet (4):
      d3d11/tests: Test that sampler states with anisotropic filtering and zero MaxAnisotropy can be created.
      d3d10core/tests: Test that sampler states with anisotropic filtering and zero MaxAnisotropy can be created.
      d3d11: Disable anisotropic filtering for sampler states with zero MaxAnisotropy.
      wined3d: Disable anisotropic filtering for zero max_anisotropy in sampler_desc_from_sampler_states().

Ignacy Kuchciński (1):
      windows.storage: Split ApplicationData.

Jacob Czekalla (6):
      hhctrl.ocx: Check for a NULL web_browser before QueryInterface.
      wininet/tests: Add more http time test strings.
      wininet: Fix parsing order of http times.
      wininet: Fix year parsing to include millennium.
      comctl32/treeview: Return from TREEVIEW_LButtonDown when the treeview handle is invalid.
      comctl32/tests: Add a test for treeview deletion during NM_CLICK in LBUTTONDOWN.

Jiangyi Chen (1):
      opengl32: Perform cAccumBits filtering if specified in wglChoosePixelFormat().

Joe Souza (1):
      cmd: Treat most delimiters as literals if user specified quotes.

Keigo Okamoto (3):
      winealsa: Send All Notes Off and Reset Controllers.
      winecoreaudio: Send All Notes Off and Reset Controllers.
      wineoss: Send All Notes Off and Reset Controllers.

Marc-Aurel Zent (5):
      ntdll: Implement ProcessPriorityBoost class in NtQueryInformationProcess.
      ntdll: Implement ProcessPriorityBoost class in NtSetInformationProcess.
      kernelbase: Implement GetProcessPriorityBoost.
      kernelbase: Implement SetProcessPriorityBoost.
      kernel32/tests: Add tests for GetProcessPriorityBoost/SetProcessPriorityBoost.

Mike Kozelkov (1):
      winbio: Add stub DLL.

Mohamad Al-Jaf (9):
      cryptxml: Add stub dll.
      include: Add cryptxml.h.
      cryptxml: Implement CryptXmlOpenToDecode() stub.
      cryptxml: Implement CryptXmlClose().
      cryptxml: Implement CryptXmlGetDocContext().
      cryptxml: Implement CryptXmlGetSignature().
      cryptxml: Implement CryptXmlVerifySignature() stub.
      cryptxml: Implement CryptXmlGetStatus().
      cryptxml/tests: Add some signature verification tests.

Nikolay Sivov (5):
      d2d1: Add some helpers for geometry figure manipulation.
      d2d1: Implement ellipse geometry simplification.
      d2d1: Implement rounded rectangle geometry simplification.
      fonts: Fix "O" glyph in Tahoma Bold bitmap strikes.
      d3d10/tests: Compile more test effects from sources.

Paul Gofman (35):
      ntdll: Factor out chksum_add() function.
      ntdll: Support SOCK_RAW / IPPROTO_ICMPV6 fallback over SOCK_DGRAM.
      ws2_32/tests: Test ICMPv6 ping.
      ntdll/tests: Add more tests for NtQueryVirtualMemory( MemoryRegionInformation ).
      ntdll: Factor out get_memory_region_size() function.
      ntdll: Reimplement get_memory_region_info() on top of get_memory_region_size().
      ntdll: Add semi-stub for NtQueueApcThreadEx2().
      ntdll: Validate reserve handle in NtQueueApcThreadEx2().
      ntdll: Pass user APC flags to call_user_apc_dispatcher().
      server: Do not allow queueing special APCs to wow64 threads.
      ntdll: Implement QUEUE_USER_APC_CALLBACK_DATA_CONTEXT in NtQueueApcThreadEx2() on x64.
      ntdll: Use NtContinueEx in KiUserApcDispatcher on x64.
      kernelbase: Implement QueueUserAPC2().
      server: Check thread and call parameters in queue_apc() for APC_USER.
      msvcp140_atomic_wait: Semi-stub __std_tzdb_get_time_zones() / __std_tzdb_delete_time_zones().
      msvcp140_atomic_wait: Semi-stub __std_tzdb_get_current_zone() / __std_tzdb_delete_current_zone().
      msvcp140_atomic_wait: Stub __std_tzdb_get_leap_seconds() / __std_tzdb_delete_leap_seconds().
      kernelbase: Preserve last error in OutputDebugStringA().
      kernel32: Preserve last error in OutputDebugStringA().
      kernel32/tests: Test last error preservation in OutputDebugString().
      ntdll/tests: Test last error preservation in OutputDebugString() with debugger.
      winex11.drv: Only create dummy parent when needed in create_client_window().
      win32u: Avoid calling server in NtUserGetKeyState() when input keystate is in sync.
      nsiproxy.sys: Get rid of echo request thread.
      nsiproxy.sys: Bind to source address in icmp_send_echo().
      nsiproxy.sys: Store socket type in struct icmp_data.
      iphlpapi: Factor out icmp_send_echo() function.
      iphlpapi/tests: Refactor APC testing in testIcmpSendEcho().
      iphlpapi: Only supply APC routine if no event in icmp_send_echo().
      iphlpapi: Implement Icmp6ParseReplies().
      iphlpapi: Implement Icmp6CreateFile().
      nsiproxy.sys: Don't try to check for original packet for ping socket.
      iphlpapi: Implement Icmp6SendEcho2().
      iphlpapi/tests: Add tests for Icmp6SendEcho2().
      iphlpapi: Avoid leaking APC context in icmp_send_echo().

Rémi Bernon (69):
      widl: Wrap strappend parameters in a new struct strbuf.
      widl: Introduce a new append_basic_type helper.
      widl: Remove unnecessary recursion for TYPE_BITFIELD.
      widl: Move some type name construction out of write_type_left.
      joy.cpl: Read the device state when getting selected effect.
      winetest: Set winetest_mute_threshold to 4 when running tests.
      user32: Use init_class_name(_ansi) in FindWindowEx(A|W).
      win32u: Drop unnecessary NtUserCreateWindowEx version strings.
      win32u: Add a helper to add atom / strings to server requests.
      server: Simplify create_class atom validation check.
      server: Unify reading class atom / name from requests.
      widl: Introduce a new write_record_type_definition helper.
      widl: Cleanup indentation and variables in write_type_left.
      widl: Split write_type_left into a write_type_definition_left helper.
      widl: Introduce a new append_type_left helper.
      win32u: Move nulldrv client surface code from vulkan.c.
      win32u: Introduce a new framebuffer object GL surface.
      win32u: Create render buffers for double / stereo buffering.
      win32u: Create depth attachments for the FBO surface.
      widl: Remove now unnecessary write_callconv argument.
      widl: Cleanup indentation and variables in write_type_right.
      widl: Inline write_args into write_type_right.
      widl: Introduce a new append_declspec helper.
      opengl32: Hook glGet(Booleanv|Doublev|Floatv|Integer64v).
      opengl32: Share wgl_context structure definition with win32u.
      opengl32: Keep track of draw/read framebuffer binding.
      opengl32: Return the tracked FBOs when using FBO surfaces.
      include: Add gameinput.idl.
      gameinput: Introduce new DLL.
      dinput/tests: Add some gameinput tests.
      winebus: Generate unique serial numbers when adding devices.
      user32/tests: Add more GetClassInfo tests.
      win32u: Forbid setting GCW_ATOM class info.
      win32u: Introduce helpers to check desktop and message class.
      user32: Implement integral class name versioning support.
      win32u: Remove now unnecessary integral atom specific handling.
      server: Return the class base atom from create_class.
      opengl32: Keep track of default FBO read/draw buffers.
      opengl32: Return the tracked FBO buffers when using FBO surfaces.
      opengl32: Remap read / draw FBO buffers when using FBO surfaces.
      opengl32: Redirect default framebuffer when using FBO surfaces.
      wine.inf: Fix section name for DefaultInstall.ntx86 services.
      win32u: Simplify setting extra class info in set_class_long.
      win32u: Use set_class_long_size for NtUserSetClassWord.
      win32u: Use get_class_long_size for get_class_word.
      server: Split get_class_info request from set_class_info.
      server: Get rid of set_class_info request flags.
      widl: Keep track of statements source locations.
      widl: Allow explicit registration by referencing runtimeclasses.
      windows.storage.applicationdata: Register runtimeclasses explicitly.
      windows.storage: Register runtimeclasses explicitly.
      wintypes: Register runtimeclasses explicitly.
      include: Remove now unnecessary registration ifdefs.
      opengl32: Initialize viewport when using FBO surface.
      win32u: Create a separate draw FBO for multisampled formats.
      opengl32: Resolve multisample draw buffer when using FBO surfaces.
      win32u: Remove unnecessary HDC parameter from client_surface_present.
      win32u: Remove unnecessary HDC parameter from p_surface_create.
      winex11: Remove unnecessary members from struct gl_drawable.
      windows.ui: Register runtimeclasses explicitly.
      windows.ui.xaml: Register runtimeclasses explicitly.
      coremessaging: Register runtimeclasses explicitly.
      twinapi.appcore: Register runtimeclasses explicitly.
      windows.applicationmodel: Register runtimeclasses explicitly.
      windows.system.profile.systemid: Register runtimeclasses explicitly.
      windows.system.profile.systemmanufacturers: Register runtimeclasses explicitly.
      windows.globalization: Register runtimeclasses explicitly.
      appxdeploymentclient: Register runtimeclasses explicitly.
      cryptowinrt: Register runtimeclasses explicitly.

Tim Clem (3):
      ntdll: Initialize return value in fork_and_exec.
      ntdll: Zero the process and thread handles when creating a Unix process.
      kernelbase: Zero the RTL_USER_PROCESS_PARAMETERS in CreateProcessInternalW.

Tingzhong Luo (1):
      shell32: Support the UserProgramFiles folder.

Tres Finocchiaro (1):
      winebuild: Add flag to disable dynamicbase/aslr.

Tyson Whitehead (9):
      joy.cpl: Use correct interface for effect AddRef call.
      joy.cpl: Remove needless device caps retrieval.
      joy.cpl: Remove incorrect DIEP_TYPESPECIFICPARAMS flag usage.
      joy.cpl: Cleanup selected device Acquire / Unacquire logic.
      joy.cpl: Fix effect axes / direction in SetParameters call.
      joy.cpl: Avoid restarting effect while button is pressed.
      joy.cpl: Turn off autocenter for every device on creation.
      joy.cpl: Add specific parameters based on type and not effect GUID.
      winebus: SDL backend FF effect angle requires 32 bits.

Vibhav Pant (9):
      vccorlib140: Add semi-stub for Platform::Details::Heap::{Allocate, Free}.
      vccorlib140: Add stub for Platform::Details::{Allocate(ptrdiff_t, size_t), ControlBlock::ReleaseTarget}.
      vccorlib140: Implement Platform::Details::{Allocate(ptrdiff_t, size_t), ControlBlock::ReleaseTarget}.
      vccorlib140: Add stub for __abi_make_type_id, Platform::Type{Equals, GetTypeCode, ToString, FullName::get}.
      vccorlib140: Implement __abi_make_type_id.
      vccorlib140: Implement Platform::Type::{Equals, GetTypeCode, ToString, FullName::get}.
      vccorlib140: Add stub for Platform::Details::CreateValue.
      vccorlib140: Implement Platform::Details::CreateValue.
      vccorlib140: Use the correct symbol name for InitControlBlock on i386 and arm.

William Horvath (2):
      ntdll: Check for invalid gs_base in the 64-bit segv_handler.
      ntdll/tests: Re-enable a previously crashing test.

Yuxuan Shui (6):
      include: Make sure to null terminate string in wine_dbg_vsprintf.
      server: Fix use-after-free of handle entry.
      makefiles: Don't delete Makefile if makedep is interrupted.
      dmime: Fix use-after-free after performance_CloseDown.
      msi/tests: Fix wrong character counts passed to RegSetValueExA.
      ole32: Don't get metafile extent if there is no metafile.

Zhiyi Zhang (2):
      user32/tests: Add more SendMessageCallbackA/W() tests with NULL callback.
      win32u: Put the message in the queue when the callback pointer is NULL and the callback data is 1.

Ziqing Hui (1):
      dwrite: Add fallback for Dingbats.
```
