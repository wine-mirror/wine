The Wine development release 9.17 is now available.

What's new in this release:
  - Window surface scaling on High DPI displays.
  - Bundled vkd3d upgraded to version 1.13.
  - Mono engine updated to version 9.3.0
  - Improved CPU detection on ARM64.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/9.x/wine-9.17.tar.xz>

Binary packages for various distributions will be available
from <https://www.winehq.org/download>

You will find documentation on <https://www.winehq.org/documentation>

Wine is available thanks to the work of many people.
See the file [AUTHORS][1] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/raw/wine-9.17/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 9.17 (total 29):

 - #11770  mapi32.dll.so does not support attachments for sending mail
 - #18154  cmd.exe: failure to handle file extension association
 - #18846  Anti-Grain Geometry gdiplus Demo does not render correctly
 - #26813  Multiple programs ( python-3.1.3.amd64.msi, scoop) need support for administrative install (msiexec.exe /a )
 - #42601  Foxit Reader 8.2 crashes after running for an extended period of time (>30 minutes)
 - #43472  Several apps (R-Link 2 Toolbox, Mavimplant 1.0, Kundenkartei 5) crash on startup (Wine's 'packager.dll' is preferred over native, causing failure to load app provided library with same name)
 - #48404  Can't close free game notifications from Epic Games Store
 - #49244  GdipDeleteFontFamily is declared, but does nothing
 - #50365  Starcraft Remastered black screen on launch or window only mode
 - #52687  OpenKiosk installer not working
 - #53154  Crash of EpicOnlineServicesUserHelper.exe" --setup
 - #54212  Nexus ECU Tuning Software for HALTECH ECU
 - #55322  SBCL 2.3.4: unattended msiexec in administrator mode fails
 - #55343  Game Constantine doesn't respond to key inputs.
 - #55410  msi:package fails on w8adm
 - #55804  DICOM Viewer (eFilm Workstation 2.x/3.x) aborts because libxml2 doesn't like "ISO8859-1" (builtin msxml6).
 - #55936  Sven Bømwøllen series: Several games crash after loading screen
 - #55964  wine-mono: Dlls include reference to filenames different to the files in wine-mono-8.1.0-dbgsym.tar.xz
 - #56102  Comdlg32/Color - Out of bound value is used
 - #56103  PropertySheet - CTRL+TAB and CTRL+SHIFT+TAB not processed
 - #56352  comctl32: Handle progress bar state messages
 - #57096  Hogia Hemekonomi does not start
 - #57102  Textbox-border is missing if textbox is on topmost layer of e. g. a TabControl.
 - #57103  "opengl32.dll" failed to initialize with WOW64 on Wayland
 - #57117  BsgLauncher cannot launch due to unhandled exception in System.Security.Cryptography (.NET48)
 - #57119  WPF app (VOCALOID) fails to initialize Markup
 - #57127  Dutch button labels are cut off in Wine Internet Explorer
 - #57140  ADOM fails to launch
 - #57145  Quicktime 3.02 (16-bit) setup hangs

### Changes since 9.16:
```
Aida Jonikienė (4):
      explorer: Make the driver error message more neutral.
      msvcrt: Print less FIXMEs for ThreadScheduler_ScheduleTask*().
      msvcrt: Remove FIXME for _StructuredTaskCollection_dtor().
      msvcrt: Only print FIXME once for Context_Yield().

Alex Henrie (5):
      explorer: Make the "Wine Desktop" window title translatable.
      explorer: Don't display "Default" in the virtual desktop window title.
      ieframe: Widen toolbar buttons to accommodate Dutch translation.
      explorer: Return void from show_icon and hide_icon.
      explorer: Support the NoTrayItemsDisplay registry setting.

Alexandre Julliard (3):
      vkd3d: Import upstream release 1.13.
      ntdll: Use custom x64 thunks for syscall exports.
      ntdll: Add custom x64 thunk for KiUserExceptionDispatcher.

Alexandros Frantzis (1):
      winewayland: Rename surface buffer size to content size.

Alistair Leslie-Hughes (9):
      odbccp32: Return false on empty string in SQLValidDSNW.
      odbccp32: Check for valid DSN before delete in SQLRemoveDSNFromIniW.
      odbccp32: Correctly handle config_mode in SQLWrite/RemoveDSNFromIniW.
      odbc32: Pass through field id SQL_MAX_COLUMNS_IN_TABLE in SQLColAttribute/W.
      odbc32: Correcly convert columns ID in SQLColAttribute/W for ODBC v2.0.
      odbccp32: Only append slash if required in write_registry_values.
      include: Move ColorChannelLUT outside of if __cplusplus.
      include: Add basic constructors for Rect/RectF.
      include: Add class SizeF.

Andrew Eikum (1):
      mmdevapi: Add stub IAudioClockAdjustment implementation.

Andrew Nguyen (2):
      msi/tests: Add additional test cases for validating package template summary info strings.
      msi: Allow package template summary info strings without semicolon separator.

Arkadiusz Hiler (2):
      mmdevapi/tests: Add more IAudioClock tests.
      winepulse.drv: Implement set_sample_rate.

Aurimas Fišeras (1):
      po: Update Lithuanian translation.

Bernhard Übelacker (1):
      msxml3: Allow encoding name "ISO8859-1".

Billy Laws (2):
      ntdll: Populate the SMBIOS with ARM64 ID register values.
      wineboot: Populate ARM64 ID register registry keys using SMBIOS info.

Biswapriyo Nath (3):
      include: Add http3 flag in winhttp.h.
      include: Add input assembler related constants in d3d11.idl.
      include: Add ISelectionProvider2 definition in uiautomationcore.idl.

Brendan McGrath (2):
      windows.gaming.input: Zero 'value' in GetCurrentReading until first state change.
      winegstreamer: Allow application to drain queue.

Brendan Shanks (2):
      winemac.drv: Call CGWindowListCreateImageFromArray through a dlsym-obtained pointer.
      secur32: Ensure unixlib function tables and enum stay in sync.

Dmitry Timoshkov (2):
      dssenh: Add support for enumerating algorithms.
      dssenh: Add support for CPGetProvParam(PP_NAME).

Dylan Donnell (1):
      winegstreamer: Support IYUV alias for I420.

Elizabeth Figura (12):
      cmd/tests: Save and restore the drive when performing drive change tests.
      cmd: Do not try to handle ERROR_FILE_NOT_FOUND from CreateProcessW().
      cmd: Separate a run_full_path() helper.
      cmd: Run files with ShellExecute() if CreateProcess() fails.
      wined3d: Invalidate STATE_SHADER instead of STATE_POINT_ENABLE.
      wined3d: Invalidate the PS from wined3d_device_apply_stateblock() when texture states change.
      wined3d: Invalidate the PS from wined3d_device_apply_stateblock() when WINED3D_RS_COLORKEYENABLE changes.
      wined3d: Invalidate the PS from wined3d_device_apply_stateblock() when the texture changes.
      wined3d: Invalidate the VS from wined3d_device_apply_stateblock() when WINED3D_RS_NORMALIZENORMALS changes.
      cmd: Allow deleting associations via ftype.
      cmd: Report an error from ftype or assoc if the value is empty.
      winegstreamer: Append HEAACWAVEINFO extra bytes to AAC user data.

Eric Pouech (5):
      cmd: Don't display dialog boxes.
      cmd/tests: Add some more tests.
      cmd: Minor fix to the lexer.
      cmd: Fix reading some input in CHOICE command.
      cmd: Strip leading white spaces and at-sign from command nodes.

Esme Povirk (2):
      mscoree: Update Wine Mono to 9.3.0.
      user32: Implement EVENT_OBJECT_FOCUS for listbox items.

Etaash Mathamsetty (1):
      win32u: Implement NtGdiDdDDIEnumAdapters.

Fabian Maurer (5):
      comctl32/tests: Add test for propsheet hotkey navigation.
      comclt32: Allow hotkeys for propsheet navigation.
      comdlg32: Update luminosity bar when changing hue/sat/lum manually.
      comdlg32: Prevent recursion inside CC_CheckDigitsInEdit.
      comdlg32: Properly handle out of bounds values.

Garrett Mesmer (1):
      ntdll: Determine the available address space dynamically for 64bit architectures.

Georg Lehmann (1):
      winevulkan: Update to VK spec version 1.3.295.

Hans Leidekker (14):
      rsaenh: Return an error on zero length only when decrypting the final block.
      wmiutils: Handle paths with implied key.
      msi: Remove traces from a couple of helpers.
      msi: Add support for the ADMIN top level action.
      msi: Implement the InstallAdminImage action.
      msiexec: Remove an obsolete fixme.
      msiexec: Don't remove quotes from properties passed on the command line.
      msi/tests: Test escaped double quote on the command line.
      msi: Bump version to 5.0.
      wpcap/tests: Skip tests when pcap_can_set_rfmon() returns PCAP_ERROR_PERM_DENIED.
      msiexec: Don't quote property values if already quoted.
      odbc32: Fix a memory leak.
      odbc32: Fix driver name query.
      odbc32: Load libodbc dynamically.

Haoyang Chen (1):
      gdiplus: Check if graphics is occupied in GdipDrawString.

Huw D. M. Davies (11):
      nsiproxy: Add linux guards for the IPv6 forward info.
      win32u: Use unsigned bitfields.
      widl: Remove unused variable.
      winedump: Remove unused variable.
      opengl32: Test the unix call function table sizes.
      combase: Group the post quit info in a structure.
      include: Always declare the imagelist read and write functions.
      winemapi: Don't write past the end of the argv array.
      secur32: Simplify the cred_enabled_protocols logic slightly.
      winecoreaudio: Set the synth volume to the greater of the left and right channels.
      widl: Avoid using sprintf() to add a single character.

Jacob Czekalla (4):
      user32/tests: Add test for edit control format rect size.
      user32/edit: Fix incorrect size for format rect when it is smaller than text.
      comctl32/tests: Add test for edit control format rect size.
      comctl32/edit: Fix incorrect size for format rect when it is smaller than text.

Jason Edmeades (2):
      cmd: Skip directories when looking for an openable file.
      cmd/tests: Test running a file with an association.

Jeremy White (1):
      winemapi: Directly use xdg-email if available.

Martin Storsjö (3):
      ntdll: Improve ARM feature checking from /proc/cpuinfo.
      include: Add new PF_* constants.
      arm64: Detect new processor features.

Nikolay Sivov (8):
      d3dx9/tests: Use explicit numeric values as expected test results.
      d3dx9/tests: Remove unused fields from effect values test data.
      d3dx9/tests: Add a test for matrix majority class.
      d3d10/effect: Rename some variable array fields to better reflect their meaning.
      d3d10/effect: Simplify setting GlobalVariables value.
      d3d10/tests: Compile some of test effects.
      d3d10/tests: Fully check matrix types.
      d3d10/tests: Fully check scalar and vector types.

Paul Gofman (10):
      concrt140: Don't forward _IsSynchronouslyBlocked functions.
      dxcore: Prefer native.
      kernel32/tests: Add a test for TLS links.
      ntdll: Reserve space for some TLS directories at once.
      ntdll: Iterate TEBs only once in alloc_tls_slot().
      ntdll: Do not use TlsLinks for enumerating TEBs.
      ntdll: Ignore HW breakpoints on the Unix side.
      uxtheme: Define a constant for default transparent colour.
      uxtheme: Try to avoid TransparentBlt() when possible.
      ntdll: Allow sending to port 0 on UDP socket to succeed.

Rémi Bernon (69):
      win32u: Only allow a custom visible rect for toplevel windows.
      win32u: Add missing thunk lock parameters callback.
      opengl32: Add missing WOW64 process_attach unixlib entry.
      opengl32: Remove unnecessary function addresses.
      winex11: Wrap x11drv_dnd_drop_event params in a struct.
      winex11: Wrap x11drv_dnd_enter_event params in a struct.
      winex11: Wrap x11drv_dnd_post_drop params in a struct.
      winex11: Use a UINT64 for the foreign_window_proc parameter.
      winex11: Route kernel callbacks through user32.
      mfmediaengine: Remove duplicate classes IDL.
      mfplat: Fix pointer dereference when caching buffer data.
      win32u: Map window region DPI before calling into the drivers.
      win32u: Map window rects DPI before calling into the drivers.
      win32u: Move window_surface creation helper to dce.c.
      win32u: Implement DPI scaled window surface.
      explorer: Use the EnableShell option to show or hide the taskbar.
      mfreadwrite: Always try inserting a converter for non-video streams.
      winewayland: Require the wp_viewporter protocol.
      winewayland: Set the window viewport source rectangle.
      winewayland: Create the window surface buffer queue unconditionally.
      winewayland: Avoid recreating window surface buffer queues.
      mfsrcsnk: Refactor sink class factory helpers.
      mfsrcsnk: Register the AVI Byte Stream Handler class.
      mfsrcsnk: Register the WAV Byte Stream Handler class.
      mfmp4srcsnk: Register the MPEG4 Byte Stream Handler class.
      mfasfsrcsnk: Register the Asf Byte Stream Handler class.
      mfmp4srcsnk: Register the MP3 and MPEG4 sink factory classes.
      winewayland: Remove unnecessary logical to physical DPI mapping.
      winex11: Map message pos to physical DPI in move_resize_window.
      win32u: Map rect to window DPI in expose_window_surface.
      win32u: Pass window_from_point dpi to list_children_from_point.
      server: Pass window's per-monitor DPI in set_window_pos.
      user32: Move dpiaware_init to SYSPARAMS_Init.
      explorer: Make the desktop thread per-monitor DPI aware.
      win32u: Stop setting DPI_PER_MONITOR_AWARE by default.
      colorcnv: Register the Color Converter DMO class.
      msvproc: Register the Video Processor MFT class.
      resampledmo: Register the Resampler DMO class.
      wmadmod: Register the WMA Decoder DMO class.
      msauddecmft: Register the AAC Decoder MFT class.
      wmvdecod: Register the WMV Decoder DMO class.
      msmpeg2vdec: Register the H264 Decoder MFT class.
      mfh264enc: Register the H264 Encoder MFT class.
      gitlab: Install FFmpeg development libraries.
      winewayland: Post WM_WAYLAND_CONFIGURE outside of the surface lock.
      winewayland: Introduce a new ensure_window_surface_contents helper.
      winewayland: Introduce a new set_window_surface_contents helper.
      winewayland: Introduce a new get_window_surface_contents helper.
      winewayland: Reset the buffer damage region immediately after copy.
      winewayland: Move window contents buffer to wayland_win_data struct.
      winewayland: Get rid of wayland_surface reference from window_surface.
      winewayland: Get rid of window_surface reference from wayland_win_data.
      winewayland: Introduce a new wayland_client_surface_create helper.
      winewayland: Get rid of the window surface individual locks.
      include: Declare D3DKMT resource creation functions.
      include: Declare D3DKMT keyed mutex creation functions.
      include: Declare D3DKMT sync object creation functions.
      win32u: Stub D3DKMTShareObjects.
      win32u: Stub D3DKMT resource creation functions.
      win32u: Stub D3DKMT keyed mutex creation functions.
      win32u: Stub D3DKMT sync object creation functions.
      quartz: Simplify the filter registration code.
      quartz: Move registration code to main.c.
      quartz: Register the MPEG1 Splitter class.
      quartz: Register the AVI Splitter class.
      quartz: Register the WAVE Parser class.
      quartz: Register the MPEG Audio Decoder class.
      quartz: Register the MPEG Video Decoder class.
      l3codecx.ax: Register the MP3 Decoder class.

Sebastian Lackner (1):
      packager: Prefer native version.

Sergei Chernyadyev (2):
      comctl32/tooltip: Support large standard title icons.
      explorer: Support large tooltip icons.

Tim Clem (5):
      nsiproxy: Implement TCP table on top of a server call.
      nsiproxy: Implement UDP table on top of a server call.
      nsiproxy: Remove now unused git_pid_map and find_owning_pid.
      iphlpapi/tests: Confirm that GetExtendedTcpTable associates a socket with the correct PID.
      iphlpapi/tests: Confirm that GetExtendedUdpTable associates a socket with the correct PID.

Topi-Matti Ritala (1):
      po: Update Finnish translation.

Torge Matthies (2):
      winemac: Route kernel callbacks through user32.
      user32: Remove NtUserDriverCallback* kernel callbacks.

Vibhav Pant (2):
      ntoskrnl: Implement IoGetDevicePropertyData().
      ntoskrnl/tests: Add test for getting and setting device properties.

Ziqing Hui (4):
      mf/tests: Test h264 encoder sample processing.
      mf/tests: Test codecapi for h264 encoder.
      winegstreamer/video_encoder: Add ICodecAPI stubs.
      winegstreamer/video_encoder: Initially implement ProcessOutput.
```
