The Wine development release 10.4 is now available.

What's new in this release:
  - Improvements to PDB support in DbgHelp.
  - More Vulkan video decoder support in WineD3D.
  - Accessibility support in the SysLink control.
  - More progress on the Bluetooth driver.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/10.x/wine-10.4.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-10.4/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 10.4 (total 28):

 - #33770  Strong Bad's Episode 1 - Homestar Ruiner Demo crashes without d3dx9_27 (purist)
 - #33943  Battle.net client dropdowns do not appear until you hover its options
 - #42117  Multiple applications have windows with double caption/title bars (Chessmaster 9000, Steam when Windows >= Vista)
 - #44795  Need for Speed: Shift demo main menu has messed up rendering (needs ID3DXEffect::SetRawValue implementation)
 - #46012  Command & Conquer 3: Kane's Wrath (1.03) Invisible units and tiberium
 - #46662  absolute value of unsigned type 'unsigned int' has no effect
 - #47165  iTunes 12.9.4+ user interface is rendered black (only text visible)
 - #47278  Multiple games and applications require TGA support in D3DXSaveSurfaceToFileInMemory (Europa Universalis 4 Golden Century, ShaderMap 4.x)
 - #53103  ie8 doesn't start (race condition)
 - #54066  SysLink control shouldn't delete the HFONT it didn't create
 - #56106  Roon 2.0.23 crashes due to unable to find library: Windows.Storage.Streams.RandomAccessStreamReference
 - #56108  Edit control should stop processing characters when left mouse button is down
 - #56225  16-bit Myst deadlocks on start since Wine 3.2
 - #57540  unrecognized charset 'SHIFT_JIS' when running Wine with LC_ALL=ja_JP.SJIS
 - #57559  Chessbase 17 database table background and non-selected entries rendered in black
 - #57717  Adobe Illustrator CS6 (16), Adobe Photoshop CS6 (13), likely all CS6 apps: Main menu bar item shortcut/accelerator key underlines positioning is wrong with built-in gdiplus
 - #57746  BeckyInternetMail/VirtualListView: The ListView of the email list isn't redrawn while receiving email.
 - #57800  Fullscreen OpenGL apps have unintended literal transparency
 - #57826  Zenless Zone Zero fails to start after update to 10.1
 - #57848  Wrong alignment of GUI elements in Enterprise Architect
 - #57853  Error: makecab.exe not found
 - #57874  wineloader no longer able to find ntdll.so
 - #57889  Prntvpt (Print Ticket API): printing is cropped in landscape orientation
 - #57896  winemenubuilder crash
 - #57952  Q-Dir crashes on exit.
 - #57962  Firefox 136.0.1 fails to start
 - #57963  Firefox crashes on youtube: wine: Call  to unimplemented function ucrtbase.dll.imaxdiv, aborting
 - #57964  Firefox crashes: wine: Call to unimplemented function KERNEL32.dll.GetCurrentApplicationUserModelId, aborting

### Changes since 10.3:
```
Alex Henrie (3):
      winecfg: Use wide character string literal for "Tahoma".
      wuauserv: Use wide character string literal for "wuauserv".
      fusion: Use wide character string literals.

Alexander Morozov (2):
      ntoskrnl.exe/tests: Improve device properties test, avoid BSOD.
      ntoskrnl.exe: Implement MmMapLockedPages.

Alexandre Julliard (33):
      include: Add a number of missing TCHAR macros.
      include: Use pragma pack push/pop.
      ntdll: Update the main exe entry point when the module is relocated.
      ntdll: Disallow AT_ROUND_TO_PAGE on 64-bit.
      user32: Add some more stubs for ordinal functions that forward to win32u.
      server: Consistently use size_t for page sizes.
      server: Compute the size of the PE header that can be mapped.
      ntdll: Only copy the PE section data to a separate block when necessary.
      krnl386: Handle DOS ioctl with simulated real mode interrupt.
      krnl386: Remove the CTX_SEG_OFF_TO_LIN macro.
      ntdll: Pass an explicit mask to the ROUND_SIZE macro.
      ntdll: Pass the base address to decommit_pages().
      tools: Add is_pe_target() common helper function.
      winegcc: Make the target options global variables.
      winegcc: Move setting the compatibility defines to a separate helper function.
      winegcc: Move the initial argument array out of the options structure.
      winegcc: Move the remaining directory options out of the options structure.
      winegcc: Remove option fields that already have a corresponding output file variable.
      winegcc: Pass the files list explicitly to the various compilation functions.
      winegcc: Pass the output name explicitly to the various compilation functions.
      winegcc: Store the file processor type in a global variable.
      winegcc: Store the various search path directories in global variables.
      winegcc: Store the library search suffix in a global variable.
      winegcc: Store the winebuild path in a global variable.
      winegcc: Store the compiler arguments in global variables.
      winegcc: Pass tool names directly to build_tool_name instead of using an enum.
      winegcc: Store various string options as global variables.
      winegcc: Store all the boolean options in global variables.
      winegcc: Get rid of the options struct.
      ntdll: Round all sizes to the section alignment for PE mappings.
      ntdll: Always map files as writable and adjust permissions.
      server: Fix limit check for adding a committed range.
      ntdll: Force committed access on anonymous mappings.

Alexandros Frantzis (4):
      winewayland: Implement wl_data_device initialization.
      winewayland: Support wl_data_device for copies from win32 clipboard to native apps.
      winewayland: Support wl_data_device for copies from native apps to win32 clipboard.
      winewayland: Warn about missing clipboard functionality.

Anders Kjersem (2):
      comctl32/listview: Never use null buffer with LVN_ENDLABELEDIT on a text change.
      shcore: Implement OS_TABLETPC and OS_MEDIACENTER.

Attila Fidan (1):
      win32u: Return 0 from NtUserGetKeyNameText if there is no translation.

Bartosz Kosiorek (6):
      gdiplus: Fix widening of LineCapArrowAnchor.
      gdiplus/tests: Improve test drawing accuracy of GdiAddPath*Curve functions.
      gdiplus: Improve drawing accuracy of GdiAddPath*Curve functions.
      gdiplus/tests: Add additional test for GdipAddPathArc.
      gdiplus/tests: Add GdipFlattenPath tests with default flatness 0.25.
      gdiplus: Fix GdipFlattenPath return path precision.

Bernhard Übelacker (4):
      kernel32/tests: Flush pending APCs and close handles (ASan).
      ieframe: Enter reallocation path one position earlier (ASan).
      advapi32: Avoid buffer underrun in split_domain_account (ASan).
      msxml6/tests: Make test pass with Windows 7.

Biswapriyo Nath (1):
      include: Add mpeg2data.idl.

Brendan McGrath (3):
      mfplat: Add mp3 resolver hint.
      mf/tests: Test timestamps in H264 decoder.
      mf/tests: Test timestamps in WMV decoder.

Brendan Shanks (5):
      ntdll: Stop using chdir() in file_id_to_unix_file_name().
      ntdll: Move the dir_queue into file_id_to_unix_file_name().
      ntdll: Add a lock around the get_dir_case_sensitivity_attr() fs_cache.
      ntdll: Use *at() functions in get_dir_case_sensitivity().
      ntdll: Stop using chdir() in nt_to_unix_file_name().

Connor McAdams (8):
      d3dx9/tests: Add tests for ID3DXEffect::SetRawValue().
      d3dx9: Partially implement ID3DXEffect::SetRawValue().
      d3dx9: Add support for setting 4x4 matrices in ID3DXEffect::SetRawValue().
      d3dx9: Fixup return values for D3DXPT_BOOL parameters in ID3DXEffect::GetValue().
      d3dx9: Add stubs for D3DXSaveVolumeToFile{A,W,InMemory}().
      d3dx9/tests: Add tests for D3DXSaveVolumeToFile{A,W,InMemory}().
      d3dx9: Implement D3DXSaveVolumeToFile{A,W,InMemory}().
      d3dx9: Use D3DXSaveVolumeToFileInMemory() inside of D3DXSaveTextureToFileInMemory().

Dmitry Timoshkov (4):
      prntvpt: PageImageableSize capabilities depend on page orientation.
      wldap32: Also initialize idW.Flags field.
      kerberos: Fix imported target name leak.
      winex11.drv: Don't add MWM_DECOR_BORDER to windows without a caption.

Dylan Donnell (2):
      ntdll: Return STATUS_ACCESS_VIOLATION from NtQueryInformationThread ThreadHideFromDebugger if *ret_len is not writable.
      ntdll/tests: Add tests for ret_len on NtQueryInformationThread HideFromDebugger.

Elizabeth Figura (13):
      wined3d: Avoid indexing a 2-element array by shader type.
      wined3d: Invalidate bumpenv_constants in wined3d_stateblock_primary_dirtify_all_states().
      wined3d: Bind video session memory.
      d3d11: Implement GetDecoderBuffer() for metadata buffers.
      d3d11: Implement GetDecoderBuffer() for bitstream buffers.
      d3d11: Create a wined3d video decoder output view.
      d3d11: Implement DecoderBeginFrame() and DecoderEndFrame().
      advapi32: Move SystemFunction032 to cryptsp.
      advapi32: Move lmhash functions to cryptsp.
      advapi32: Merge crypt_lmhash.c into crypt_des.c.
      advapi32: Move the remaining SystemFunction* functions to cryptsp.
      advapi32: Move DES functions to cryptbase.
      advapi32: Move the remaining SystemFunction* functions to cryptbase.

Ellington Santos (1):
      wpcap: Implement pcap_set_immediate_mode.

Eric Pouech (28):
      cmd: Fix regression in PAUSE test.
      winedump: Use correct computation for first section out of a .DBG file.
      winedump: Use correct field when dumping CodeView symbols.
      winedump: Don't miss PDB_SYMBOL_RANGE* in PDB files.
      winedump: Don't miss hash entries in PDB files.
      winedump: Support more than 64K files in PDB DBI module source substream.
      include: Remove flexible array member from PDB JG header.
      winedump: Support dumping large PDB files (>4G).
      dbghelp: Fix potential crash for old debug formats.
      dbghelp: Support large PDB files (> 4G).
      dbghelp: Store pointer to context instead of context.
      dbghelp: Optimize vector allocation.
      dbghelp: Simplify get_line_from_addr().
      dbghelp: Support module lookup in SymEnumSourceFiles.
      dbghelp: Factorize some code between type enumeration APIs.
      dbghelp: Only store types with names in module.
      dbghelp: Factorize function signature creation {dwarf}.
      dbghelp: Introduce helper to match an ANSI string against a Unicode regex.
      winedbg: Support more integral types in VARIANT for enum value.
      dbghelp: Pass a VARIANT to add an enumeration entry.
      dbghelp: Use VARIANT for storing enum values (pdb).
      dbghelp: Store LEB128 encoded as 64bit entities (dwarf).
      dbghelp: Introduce helper to fill in VARIANT (dwarf).
      dbghelp: Fix debug information for C++ enumeration types (dwarf).
      dbghelp: Add a couple of missing basic types for PDB.
      dbghelp: Silence a couple of CodeView symbols.
      dbghelp: Uniformize the two readers for PDB line information.
      dbghelp: Only load line information when SYMOPT_LOAD_LINE is set.

Esme Povirk (10):
      gdiplus: Reset X position before drawing hotkey underlines.
      comctl32: Include only link items as IAccessible children.
      comctl32: Implement get_accState for SysLink controls.
      comctl32: Implement acc_getName for SysLink.
      comctl32: Implement get_accDefaultAction for SysLink.
      comctl32: Implement accLocation for SysLink.
      comctl32: Implement get_accChildCount for SysLink.
      comctl32: Implement accChild for SysLink.
      comctl32: Implement IOleWindow for SysLink.
      comctl32: Implement EVENT_OBJECT_VALUECHANGE for datetime control.

Hans Leidekker (7):
      odbc32: Pass through result length pointers when PE/Unix pointer sizes are equal.
      wpcap: Check for failure from pcap_dump_open().
      wpcap: Fix Unix call in pcap_dump_close().
      wpcap: Remove unneeded trace.
      wpcap: Pass the dumper handle to pcap_dump().
      wpcap: Correct params structure in wow64_dump_open().
      wpcap: Fall back to a buffer copy if 32-bit mmap support is not available.

Jacek Caban (3):
      rpcrt4: Don't validate buffer in NDR marshaler.
      kernelbase: Factor out get_process_image_file_name.
      kernelbase: Use ProcessImageFileNameWin32 in GetModuleFileNameExW.

Joe Souza (1):
      cmd: Allow any key to continue past DIR /P pauses.

Kun Yang (1):
      msvcrt: Add MSVCRT__NOBUF flag check in _filbuf to avoid dead loop in application which sets the flag.

Louis Lenders (2):
      magnification: Add stub for MagUninitialize.
      kernelbase: Add stub for GetCurrentApplicationUserModelId.

Marc-Aurel Zent (7):
      include: Fix RTL_PATH_TYPE names.
      ntdll: Implement RtlGetFullPathName_UEx.
      include: Add thread priority constants.
      ntdll/tests: Add tests for process and thread priority.
      server: Infer process priority class in set_thread_priority.
      server: Clarify between effective thread priority and class/level.
      server: Rename thread priority to base_priority.

Matteo Bruni (8):
      d3dx9: Load the D3DAssemble() function pointer from the proper DLL.
      d3dx9/tests: Fix expected asm test results on version >= 42.
      d3dcompiler/tests: Clean up temporary file after the test.
      d3dx9_42: Generate an import library.
      d3dx9/tests: Add d3dx9_42 tests.
      d3dcompiler: Fix a few version-dependent error returns in D3DReflect().
      d3dcompiler/tests: Add d3dcompiler_42 tests.
      d3dx9/tests: Skip some shader tests if we can't create a D3D object.

Mohamad Al-Jaf (17):
      include: Add robuffer.idl.
      wintypes: Add IBufferFactory stub.
      include: Add Windows.System.Profile.SystemIdentification definition.
      windows.system.profile.systemid: Add stub dll.
      windows.system.profile.systemid: Add ISystemIdentificationStatics stub.
      windows.system.profile.systemid/tests: Add ISystemIdentificationStatics::GetSystemIdForPublisher() tests.
      windows.system.profile.systemid: Partially implement ISystemIdentificationStatics::GetSystemIdForPublisher().
      windows.system.profile.systemid/tests: Add ISystemIdentificationInfo::get_Source() tests.
      windows.system.profile.systemid: Implement ISystemIdentificationInfo::get_Source().
      wintypes/tests: Add IBufferFactory::Create() tests.
      wintypes: Implement IBufferFactory::Create().
      wintypes: Implement IBuffer::get_Capacity().
      wintypes: Implement IBuffer::put_Length().
      wintypes: Implement IBuffer::get_Length().
      wintypes: Add IBufferByteAccess stub.
      wintypes/tests: Add IBufferByteAccess::Buffer() tests.
      wintypes: Implement IBufferByteAccess::Buffer().

Nikolay Sivov (12):
      d3dx9/effect: Remove explicit objects pointer from the parsing helpers.
      d3dx9/effect: Remove misleading trace message.
      comctl32/tests: Run LVM_GETORIGIN tests on v6.
      comctl32/tests: Add a test for LVM_GETORIGIN returned coordinate.
      comctl32/listview: Invert origin coordinate for LVM_GETORIGIN.
      comctl32/listview: Handle WM_VSCROLL(SB_TOP).
      d3dx9/tests: Enable tests for d3dx9_35.dll.
      comctl32/tests: Add a LVN_ENDLABELEDIT test with empty text.
      comctl32/tests: Add a test for LVM_FINDITEM with LVS_OWNERDATA.
      comctl32/listview: Use correct LVN_ODFINDITEM notification.
      user32/edit: Block key input when mouse input is captured.
      comctl32/edit: Block key input when mouse input is captured.

Paul Gofman (9):
      ntdll/tests: Test NtCreateUserProcess() with limited access rights.
      ntdll: Do not fail NtCreateUserProcess() if requested access doesn't have PROCESS_CREATE_THREAD.
      win32u: Prevent remote drawing to ULW layered window.
      setupapi: Fix buffer size passed to SetupDiGetDeviceInstanceIdW() in get_device_id_list().
      setupapi: Increase id buffer size in get_device_id_list().
      ntdll: Properly set context control registers from the other thread on wow64.
      ntdll/tests: Test first trap address when setting trap flag in various ways.
      ntdll: Fix setting trap flag with CONTEXT_CONTROL and instrumentation callback on x64.
      ntdll: Set CONTEXT_CONTROL frame restore flag in sigsys_handler().

Piotr Caban (3):
      advapi32: Fix environment parameter handling in CreateProcessWithLogonW.
      include: Add imaxdiv declaration.
      msvcr120: Add imaxdiv implementation.

Robert Lippmann (1):
      winedump: Fix grep warning.

Roman Pišl (2):
      kernel32/tests: Test ReplaceFileW with forward slashes.
      kernelbase: Handle correctly paths with forward slashes in ReplaceFileW.

Rémi Bernon (15):
      win32u: Don't set foreground window if window is minimized.
      opengl32: Generate pointer offsets in the extension registry.
      opengl32: Generate ALL_(WGL|GL|GL_EXT)_FUNCS macros and prototypes.
      opengl32: Get rid of opengl_funcs internal structures.
      opengl32: Use ALL_GL(_EXT)_FUNCS to generate opengl_funcs table.
      opengl32: Stop generating wine/wgl_driver.h.
      hidclass: Rename BASE_DEVICE_EXTENSION to struct device.
      hidclass: Use HID_DEVICE_EXTENSION as base for fdo and pdo.
      hidclass: Use a dedicated struct phys_device for PDOs.
      hidclass: Use a dedicated struct func_device for FDOs.
      hidclass: Avoid leaking input packet from the device thread.
      winexinput: Remove pending IRPs on IRP_MN_REMOVE_DEVICE.
      hidclass: Dispatch IRP_MN_SURPRISE_REMOVAL to the minidrivers.
      hidclass: Wait for the pending IRP after thread shutdown.
      winex11: Use -1 as fullscreen monitor indices to clear the property.

Sebastian Lackner (1):
      win32u: Fix alpha blending in X11DRV_UpdateLayeredWindow.

Tim Clem (2):
      imm32: Add a stub for CtfImmHideToolbarWnd.
      win32u: Add a stub for NtUserIsChildWindowDpiMessageEnabled.

Vadim Kazakov (1):
      ntdll: Print name for TOKEN_INFORMATION_CLASS.

Vibhav Pant (15):
      winebth.sys: Remove the first 2 zero bytes after byte-swapping Bluetooth addresses.
      winebth.sys: Implement IOCTL_WINEBTH_RADIO_START_DISCOVERY.
      winebth.sys: Implement IOCTL_WINEBTH_RADIO_STOP_DISCOVERY.
      bluetoothapis: Implement BluetoothFindFirstDevice and BluetoothFindDeviceClose.
      bluetoothapis/tests: Add tests for BluetoothFindFirstDevice, BluetoothFindDeviceClose.
      bluetoothapis: Implement BluetoothFindNextDevice.
      bluetoothapis/tests: Add tests for BluetoothFindNextDevice.
      bluetoothapis/tests: Use the correct file name while skipping tests when no radios are found.
      bluetoothapis: Add a basic implementation for BluetoothGetDeviceInfo.
      bluetoothapis/tests: Add tests for BluetoothGetDeviceInfo.
      cfgmgr32: Add stub for CM_Unregister_Notification.
      cfgmgr32/tests: Add basic tests for CM_(Un)Register_Notification.
      dinput/tests: Add tests for CM_Register_Notification.
      cfgmgr32: Implement CM_Register_Notification and CM_Unregister_Notification.
      user32: Remove incorrect FIXME warning while registering for DBT_DEVTYP_HANDLE notifications.

Ziqing Hui (4):
      winegstreamer: Assume stream type is always not NULL for media sink.
      winegstreamer: Implement stream_sink_type_handler_GetMajorType.
      winegstreamer: Implement stream_sink_type_handler_GetMediaTypeCount.
      winegstreamer: Implement stream_sink_type_handler_GetMediaTypeByIndex.
```
