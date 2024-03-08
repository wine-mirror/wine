The Wine development release 9.4 is now available.

What's new in this release:
  - Bundled vkd3d upgraded to version 1.11.
  - Initial OpenGL support in the Wayland driver.
  - Support for elevating process privileges.
  - More HID pointer improvements.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/9.x/wine-9.4.tar.xz>

Binary packages for various distributions will be available
from <https://www.winehq.org/download>

You will find documentation on <https://www.winehq.org/documentation>

Wine is available thanks to the work of many people.
See the file [AUTHORS][1] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/raw/wine-9.4/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 9.4 (total 25):

 - #11629  Add optional start menu and taskbar to explorer
 - #24812  Explorer++ 1.2: right-click menu (in the main listview) degrades to nothing when opened multiple times
 - #34319  Total Commander 8.x: Context menu does not contain 'paste' entry
 - #34321  Total Commander 8.x: cut/copy/paste keyboard shortcuts don't work
 - #34322  Total Commander 8.x: 'cut' works like 'copy'
 - #44797  Visio 2003 does not read the complete list of fonts present in the system.
 - #46773  Skype 4 MSI installer fails to create trigger for task using Task Scheduler (unimplemented type 7, TASK_TRIGGER_REGISTRATION)
 - #48110  Multiple .NET 4.x applications need TaskService::ConnectedUser property (Toad for MySQL Freeware 7.x, Microsoft Toolkit from MS Office 2013)
 - #48344  Luminance HDR / qtpfsgui 2.6.0: Empty file select dialog
 - #49877  Minecraft Education Edition shows error during install: Fails to create scheduled task
 - #52213  Thread crashes when pthread_exit is called in a SIGQUIT handler
 - #55487  winpcap: pcap_dispatch doesn't capture anything with count argument -1
 - #55619  VOCALOID AI Shared Editor v.6.1.0 crashes with System.Management.ManagementObject object construction
 - #55724  mfmediaengine:mfmediaengine sometimes gets a threadpool assertion in Wine
 - #55821  Desktop Window Manager crashes when launching a WPF app
 - #56147  Real time Receiving data freezes for 1-5 seconds
 - #56271  Free Download Manager no longer works after it updated (stuck at 100% CPU, no visible window)
 - #56299  imm32.dll: CtfImmIsGuidMapEnable could not be located in the dynamic link library
 - #56334  Page fault when querying dinput8_a_EnumDevices
 - #56337  battle.net: tray icon is not displayed with wine-9.2
 - #56345  EA app installer has no text
 - #56357  Zero sized writes using WriteProcessMemory succeed on Windows, but fail on Wine.
 - #56360  FoxVox window is rendered as a blank surface instead of expected user interface
 - #56388  Regression: Fullscreen apps show on wrong monitor and don't respond to mouse events properly
 - #56401  Some ARM unwinding testcases broken by "ntdll: Use the correct structure for non-volatile registers"

### Changes since 9.3:
```
Aida Jonikienė (3):
      qwave: Add QOSCloseHandle() stub.
      qwave: Add tests for QOSCloseHandle().
      msvcrt: Handle wide specifiers option in __stdio_common_vfscanf().

Alexandre Julliard (59):
      ntdll: Add a wrapper macro for ARM64EC syscalls.
      win32u: Add a wrapper macro for ARM64EC syscalls.
      include: Add SEH information to ARM64EC syscalls.
      ntdll: Add SEH information to ARM64EC breakpoints.
      winebuild: Remove some no longer used support for ELF ARM platforms.
      wow64: Update Wow64RaiseException behavior to match i386 hardware exceptions.
      wow64: Use a .seh handler for the simulation loop.
      ntdll: Always use .seh handlers on ARM.
      ntdll: Use a .seh handler in DbgUiRemoteBreakin on ARM64EC.
      ntdll: Use a .seh handler in RtlUserThreadStart on ARM64EC.
      ntdll: Implement RtlCaptureContext on ARM64EC.
      ntdll: Use the exported structures for dll redirection data.
      ntdll: Move RtlHashUnicodeString constants to a public header.
      ntdll: Remove no longer needed definitions from the private header.
      winedump: Print the correct register names for exception info on ARM64.
      ntdll: Use the official definitions for exception flags.
      kernelbase: Use the official definitions for exception flags.
      krnl386.exe: Use the official definitions for exception flags.
      msvcp90: Use the official definitions for exception flags.
      msvcrt: Use the official definitions for exception flags.
      win32u: Use the official definitions for exception flags.
      winecrt0: Use the official definitions for exception flags.
      wow64: Use the official definitions for exception flags.
      winedbg: Use the official definitions for exception flags.
      widl: Use the official definitions for exception flags.
      include: Remove the private definitions of the exception flags.
      include: Move unwinding functions definitions to rtlsupportapi.h.
      ntdll: Implement RtlVirtualUnwind2.
      ntdll/tests: Use a proper handler in the RtlRaiseException test on x86-64.
      ntdll: Implement RtlRaiseException on ARM64EC.
      kernelbase: Implement RaiseException on ARM64EC.
      qwave/tests: Remove todo from a succeeding test.
      ntdll: Port the RtlRaiseException test to ARM64.
      ntdll: Port the RtlRaiseException test to ARM.
      ntdll: Always use SEH support on ARM.
      ntdll: Use the correct structure for non-volatile registers on ARM64.
      ntdll: Use the correct structure for non-volatile registers on ARM.
      ntdll: Also copy non-volatile regs on collided unwind.
      ntdll: Use a common wrapper to call exception handlers on ARM64.
      ntdll: Use a common wrapper to call exception handlers on ARM.
      ntdll: Use a common wrapper to call unwind handlers on ARM64.
      ntdll: Use a common wrapper to call unwind handlers on ARM.
      ntdll: Allocate the data structure and stack for the ARM64EC emulator.
      ntdll: Implement exception dispatching on ARM64EC.
      ntdll: Implement KiUserExceptionDispatcher on ARM64EC.
      vkd3d: Import upstream release 1.11.
      ntdll: Use jump buffer definitions from setjmp.h.
      ntdll: Export _setjmpex.
      ntdll: Export longjmp.
      ntdll: Implement _setjmpex on ARM64EC.
      d3d10_1/tests: Mark a failing test as todo.
      d3dx9/tests: Mark failing tests as todo.
      ntdll: Don't copy a missing context in get_thread_context().
      ntdll: Remove a misleading WARN.
      ntdll: Support the __os_arm64x_helper functions in the loader.
      winedump: Dump the __os_arm64x_helper functions.
      include: Always use _setjmpex on non-i386 platforms.
      msvcrt: Import setjmp/setjmpex from ntdll.
      msvcrt: Import longjmp from ntdll for PE builds.

Alexandros Frantzis (11):
      winex11.drv: Remove unused refresh_drawables field.
      winewayland.drv: Add skeleton OpenGL driver.
      winewayland.drv: Initialize core GL functions.
      winewayland.drv: Implement wglGetExtensionsString{ARB,EXT}.
      winewayland.drv: Implement wglGetProcAddress.
      winewayland.drv: Implement wglDescribePixelFormat.
      winewayland.drv: Implement wglSetPixelFormat(WINE).
      winewayland.drv: Implement OpenGL context creation.
      winewayland.drv: Implement wglMakeCurrent and wglMakeContextCurrentARB.
      winewayland.drv: Implement wglSwapBuffers.
      winewayland.drv: Handle resizing of OpenGL content.

Aurimas Fišeras (1):
      po: Update Lithuanian translation.

Brendan McGrath (5):
      mshtml: Pass DOMEvent instead of nsIDOMEvent during handle_event.
      mshtml: Use generic event dispatcher for DOMContentLoaded.
      mshtml/tests: Add test for document mode after InitNew and Load.
      mshtml: Always use the event target dispex.
      mshtml: Don't handle special case when doc != node->doc.

Brendan Shanks (2):
      ntdll: Add native thread renaming for FreeBSD.
      quartz: Set the name of internal threads.

Connor McAdams (6):
      webservices/tests: Fix -Warray-bounds warning on gcc 13.2.0.
      d3dx9/tests: Add more tests for misaligned compressed surface loading.
      d3dx9: Use base image pointer when decompressing source image.
      d3dx9: Split D3DXLoadSurfaceFromMemory functionality into a separate function.
      d3dx9: Split off image decompression into a helper function.
      d3dx9: Preserve the contents of unaligned compressed destination surfaces.

Daniel Lehman (5):
      oleaut32/tests: Add tests for IPersistStream::GetSizeMax.
      oleaut32: Implement GetSizeMax for empty picture.
      oleaut32: Implement GetSizeMax for BMPs.
      ole32: Do not lock storage in read-only deny-write mode.
      ole32/tests: Remove todo from lock tests.

David McFarland (5):
      mmdevapi/tests: Add test for AudioClient3_InitializeSharedAudioStream.
      mmdevapi: Implement IAudioClient3_InitializeSharedAudioStream.
      mmdevapi: Implement IAudioClient3_GetSharedModeEnginePeriod.
      mmdevapi/tests: Add test for IDeviceTopology.
      mmdevapi: Add stub for IDeviceTopology.

Eric Pouech (20):
      server: Allow 0-write length in WriteProcessMemory().
      dbghelp/tests: Add tests for image files lookup.
      dbghelp/tests: Add tests for SymFindFileInPath for pdb files.
      dbghelp/tests: Add tests about SymLoadModule and finding pdb files.
      dbghelp: Don't fail on loading 64bit modules on 32bit applications.
      dbghelp: Don't search the passed path in SymFindFileInPath.
      dbghelp/tests: Add more tests for SymLoadModule*.
      dbghelp: Always use SymGetSrvIndexFileInfo() for files lookup.
      dbghelp: Rework loading of PDB string table.
      dbghelp: Get rid of struct pdb_lookup.
      dbghelp: Return matched information for path_find_symbol_file.
      dbghelp: Search subdirectories in element path.
      dbghelp: Relax failure conditions.
      dbghelp: Change order when trying to load modules.
      dbghelp: Mimic native behavior for module name.
      dbghelp: Fix some corner case of virtual module loading.
      dbghelp/tests: Extend the tests for SymLoadModule().
      dbghelp: Support SLMFLAG_NO_SYMBOLS in SymLoadModuleEx*().
      dbghelp: Don't fail in SymAddSymbol for modules without debug information.
      dbghelp: Fixed module information when unmatched pdb file is loaded.

Esme Povirk (7):
      gdiplus: Switch to a struct for gdip_format_string callback args.
      gdiplus: Pass gdip_format_string_info to font link functions.
      gdiplus: Restore hdc argument to gdip_format_string.
      gdiplus: Fix crash in GdipAddPathString.
      gdiplus: Fix use after free in GdipAddPathString.
      gdiplus: Implement font linking in GdipAddPathString.
      mscoree: Implement CLRRuntimeHost_Start.

Fabian Maurer (7):
      win32u: Move get_awareness_from_dpi_awareness_context.
      win32u: Refactor get_thread_dpi_awareness to use get_awareness_from_dpi_awareness_context.
      user32/tests: Add exhaustive tests for Get/SetThreadDpiAwarenessContext.
      user32: Fix Set/GetThreadDpiAwarenessContext for DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2.
      user32/tests: Add tests for AreDpiAwarenessContextsEqual.
      user32: Fix AreDpiAwarenessContextsEqual behavior for DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2.
      win32u: Sync dpi awareness changes from user32.

Florian Will (3):
      include: Add TCP_KEEPCNT and TCP_KEEPINTVL definitions.
      ws2_32/tests: Test TCP_KEEP{ALIVE,CNT,INTVL} options.
      ws2_32: Implement TCP_KEEP{ALIVE,CNT,INTVL} options.

Gabriel Ivăncescu (1):
      winex11: Set the correct visual even if alpha matches.

Hans Leidekker (5):
      netprofm: Support NLM_ENUM_NETWORK flags.
      netprofm: Set return pointer to NULL in networks_enum_Next().
      wbemprox: Handle implicit property in object path.
      netprofm/tests: Mark a test result as broken on Windows 11.
      wbemprox: Use separate critical sections for tables and table list.

Henri Verbeet (2):
      wined3d: Pass "shader->function" as source to vkd3d_shader_scan() in shader_spirv_scan_shader().
      wined3d: Slightly adjust an ERR in shader_spirv_compile_shader().

Jacek Caban (9):
      configure: Disable -Wmisleading-indentation warnings on GCC.
      vcomp/tests: Use limits.h macros in for_static_i8_cb.
      winebuild: Output load config on PE targets.
      winevulkan: Update to VK spec version 1.3.278.
      winevulkan: Remove no longer needed spec workarounds.
      winevulkan: Rename wine_device_memory mapping to vm_map.
      winevulkan: Use handle map for memory objects.
      winevulkan: Refactor extra extensions handling in wine_vk_device_convert_create_info.
      winevulkan: Use VK_EXT_map_memory_placed for memory mapping on wow64.

Martin Storsjö (1):
      arm64: Expose information about more modern CPU extensions.

Michael Müller (1):
      wine.inf: Register the New menu as a directory background context menu handler.

Nikolay Sivov (3):
      d3dcompiler: Enable semantic names mapping in compatibility mode.
      d3d10_1/tests: Add a basic test for returned preferred profiles.
      d3d10_1/tests: Add an effect compilation test using 10.1 features.

Noah Berner (1):
      comctl32/tests: Fix test that fails on Feb 29th.

Paul Gofman (34):
      nsiproxy.sys: Fix ipv6 route table parsing on Linux.
      iphlpapi: Partially fill Ipv4 / Ipv6 metric in GetAdaptersAddresses().
      wbemprox: Force debug info in critical sections.
      wmwcore: Force debug info in critical sections.
      browseui: Force debug info in critical sections.
      itss: Force debug info in critical sections.
      mmdevapi: Force debug info in critical sections.
      ntdll: Mind context compaction mask in context_from_server().
      ntdll: Don't copy xstate from / to syscall frame in usr1_handler().
      ntdll: Support generic xstate config in context manipulation functions.
      msvcp: Force debug info in critical sections.
      msvcrt: Force debug info in critical sections.
      netapi32: Force debug info in critical sections.
      rsaenh: Force debug info in critical sections.
      wined3d: Force debug info in critical sections.
      kernelbase: Use KEY_WOW64_64KEY flag when 64 bit registry access is assumed.
      mciavi32: Force debug info in critical sections.
      winmm: Force debug info in critical sections.
      winebus.sys: Force debug info in critical sections.
      windows.security.credentials.ui.userconsentverifier: Force debug info in critical sections.
      amstream: Avoid leaking critical section debug info in filter_Release().
      amstream: Force debug info in critical sections.
      winexinput.sys: Force debug info in critical sections.
      comctl32: Force debug info in critical sections.
      mcicda: Force debug info in critical sections.
      ole32: Force debug info in critical sections.
      qcap: Force debug info in critical sections.
      ntdll: Respect red zone in usr1_handler() on x64.
      quartz: Force debug info in critical sections.
      urlmon: Force debug info in critical sections.
      winegstreamer: Force debug info in critical sections.
      wmiutil: Force debug info in critical sections.
      windows.gaming.input: Force debug info in critical sections.
      windows.media.speech: Force debug info in critical sections.

Philip Rebohle (1):
      winevulkan: Update to VK spec version 1.3.279.

Piotr Caban (1):
      wininet: Fix memory leak when loading proxy information.

Rémi Bernon (49):
      win32u: Introduce new NtUserSwitchDesktop syscall stub.
      server: Keep track of the winstation input desktop.
      server: Send hardware input to the visible input desktop.
      server: Keep a list of threads connected to each desktop.
      server: Keep a list of processes that can receive rawinput messages.
      server: Dispatch rawinput messages using the rawinput process list.
      winevulkan: Remove unnecessary WINEVULKAN_QUIRK_ADJUST_MAX_IMAGE_COUNT quirk.
      winevulkan: Succeed VK_KHR_win32_surface procs queries when enabled.
      winex11: Remove now unnecessary vulkan function name mapping.
      winemac: Remove now unnecessary vulkan function name mapping.
      winewayland: Remove now unnecessary vulkan function name mapping.
      winex11: Remove unnecessary X11DRV_get_vk_* helpers.
      winemac: Remove unnecessary macdrv_get_vk_* helper.
      winex11: Remove unnecessary vkDestroySurfaceKHR NULL checks.
      winemac: Remove unnecessary vkDestroySurfaceKHR NULL checks.
      winewayland: Remove unnecessary vkDestroySurfaceKHR NULL checks.
      mfreadwrite/reader: Split source_reader_create_decoder_for_stream helper.
      mf/topology_loader: Only propagate some media type attributes.
      mfreadwrite/reader: Call SetOutputType directly on the decoder transform.
      mfreadwrite/reader: Keep the stream transforms in a list.
      mfreadwrite/reader: Create and append a converter transform.
      mfreadwrite/reader: Implement IMFSourceReaderEx_GetTransformForStream.
      mfreadwrite/reader: Adjust min_buffer_size to be 1s of audio data.
      win32u: Use NtUserCallHwndParam interface for __wine_send_input.
      win32u: Use a custom struct hid_input for NtUserSendHardwareInput.
      dinput/tests: Test the WM_POINTER* message parameter values.
      mouhid.sys: Send WM_POINTER* messages on contact updates.
      server: Add support for sending and receiving WM_POINTER* messages.
      win32u: Add support for sending and receiving WM_POINTER* messages.
      win32u: Use char array for the device manager context gpuid.
      win32u: Simplify adapter key path creation from gpu_guid.
      win32u: Remove unused wine_devpropkey_monitor_adapternameW property.
      win32u: Use REG_SZ instead of REG_BINARY for some adapter keys.
      win32u: Remove unnecessary class_guidW double check.
      win32u: Use set_reg_ascii_value whenever possible.
      win32u: Introduce and use new reg_(open|create)_ascii_key helpers.
      winex11: Support XInput2 events on individual windows.
      winex11: Select XI_Touch* input and translate it to WM_POINTER*.
      server: Stop waiting on LL-hooks for non-injected input.
      server: Generate WM_POINTERENTER / WM_POINTERLEAVE messages.
      server: Continuously send pointer update messages while it's down.
      server: Send emulated mouse messages on primary pointer updates.
      win32u: Keep a reference on the adapters gpu.
      win32u: Load gpus from registry before adapters.
      win32u: Lookup adapter gpus from their device paths.
      win32u: Enumerate devices with a dedicated helper.
      win32u: Split read / write of gpu to registry to separate helpers.
      win32u: Keep the vulkan GUID on the gpu structure.
      server: Remove desktop from their winstation list before looking for another input desktop.

Shaun Ren (2):
      sapi: Implement ISpeechVoice::{get/put}_Volume.
      sapi: Implement ISpeechVoice::{get/putref}_Voice.

Sven Baars (1):
      oleaut32: Use scientific notation if it prevents a loss of accuracy.

Tim Clem (3):
      winemac.drv: Exclude the emoji Touch Bar when looking for input methods.
      win32u: Don't mask keyboard scan codes when processing them for IME.
      winebus.sys: Only attempt to open joysticks and gamepads in the IOHID backend.

Vijay Kiran Kamuju (7):
      taskschd: Implement ITaskService_get_ConnectedUser.
      taskschd: Return success from Principal_put_RunLevel.
      include: Add IRegistrationTrigger definition.
      taskschd: Add IRegistrationTrigger stub implementation.
      taskschd: Implement IRegistrationTrigger_putEnabled.
      taskschd: Implement IRegistrationTrigger_getEnabled.
      taskschd: Implement TaskService_get_ConnectedDomain.

Yuxuan Shui (10):
      dmband: Move band.c to dmusic.
      dmime: Better MIDI parsing interface.
      dmime: Parse MIDI program change events and generate a bandtrack.
      dmime: Add a stub chordtrack for MIDI segments.
      dmime: Use linked list for tempotrack.
      dmime: Implement setting TempoParam for tempotracks.
      dmime: Parse MIDI Set Tempo meta events and generate a tempotrack.
      dmime/tests: Call the correct QueryInterface function for DirectMusic track.
      dmime: Parse note on/off events and generate a seqtrack.
      dmime: Handle MIDI control events in MIDI files.

Zebediah Figura (47):
      shell32: Also zero-initialize the background menu.
      wined3d/atifs: Move TEXTUREFACTOR constant loading to arbfp_apply_draw_state().
      wined3d/atifs: Move texture constant loading to arbfp_apply_draw_state().
      wined3d/atifs: Move FFP bumpenv constant loading to atifs_apply_draw_state().
      wined3d/atifs: Move fragment program compilation from set_tex_op_atifs() to atifs_apply_draw_state().
      shell32: Stub CLSID_NewMenu.
      shell32: Stub IContextMenu3 on the New menu.
      shell32: Stub IObjectWithSite on the New menu.
      shell32: Return an initial "New" menu.
      wine.inf: Add ShellNew registry entries for Folder.
      shell32: Enumerate the ShellNew key for Folder.
      shell32: Implement InvokeCommand() for the new menu.
      shell32/tests: Add a few more tests for the New menu.
      msi/tests: Expand costing tests.
      msi: Round costs up to 4096 bytes instead of clamping.
      msi: Store component cost in 512-byte units.
      msi: Multiply by 512 in dialog_vcl_add_drives().
      shell32: Elevate the child process for the "runas" verb.
      wine.inf: Set the EnableLUA value to 1.
      msi: Create the custom action server as an elevated process.
      shell32/tests: Add tests for context menu copy/paste.
      shell32: Remove useless and commented out code.
      shell32: Move DoPaste() up.
      shell32: Add a get_data_format() helper.
      shell32: Reimplement pasting from CF_DROP directly.
      shell32: Respect the parent PIDL when pasting from CFSTR_SHELLIDLIST.
      shell32: Implement Paste in the item menu.
      wined3d/nvrc: Move TEXTUREFACTOR constant loading to nvrc_apply_draw_state().
      wined3d/nvrc: Move FFP bumpenv constant loading to nvrc_apply_draw_state().
      wined3d/nvrc: Move color ops from nvrc_colorop() to nvrc_apply_draw_state().
      wined3d/nvrc: Remove now redundant STATE_SAMPLER handlers.
      wined3d/nvrc: Move alpha op application to nvrc_apply_draw_state().
      wined3d/nvrc: Remove now redundant WINED3D_TSS_RESULT_ARG handlers.
      mciwave: Abort the playback thread regardless of state when stopping.
      shell32/tests: Add more tests for IDataObject.
      shell32: Do not interpret the direction in IDataObject::EnumFormatEtc().
      shell32: Reimplement the data object to store a generic array of HGLOBALs.
      shell32: Implement IDataObject::SetData().
      shell32: Reimplement pasting from a CIDA without ISFHelper.
      shell32: Remove the no longer used ISFHelper::CopyItems() helper.
      shell32: Set the drop effect from the context menu.
      shell32: Respect the drop effect in do_paste().
      shell32: Fix a test failure in test_DataObject().
      winetest: Elevate test processes on Wine.
      ntdll: Implement NtSetInformationProcess(ProcessAccessToken).
      ntdll: Elevate processes if requested in the manifest.
      server: Create processes using a limited administrator token by default.

Zhiyi Zhang (7):
      Revert "winex11.drv: Handle X error from vkGetRandROutputDisplayEXT()."
      rtworkq: Avoid closing a thread pool object while its callbacks are running.
      rtworkq: Avoid possible scenarios that an async callback could be called twice.
      advapi32: Check NULL return key pointers when creating registry keys.
      advapi32/tests: Test creating registry keys with a NULL return key pointer.
      user32/tests: Add some ReleaseCapture() tests.
      win32u: Only send mouse input in ReleaseCapture() when a window is captured.

Ziqing Hui (2):
      mf/tests: Test AvgTimePerFrame for WMV decoder DMO.
      winegstreamer: Set AvgTimePerFrame in GetOutputType() for WMV decoder.
```
