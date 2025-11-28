The Wine development release 10.20 is now available.

What's new in this release:
  - Bundled vkd3d upgraded to version 1.18.
  - More support for reparse points.
  - More refactoring of Common Controls after the v5/v6 split.
  - Progress dialog for document scanning.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/10.x/wine-10.20.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-10.20/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 10.20 (total 31):

 - #41034  TomTom MyDrive Connect 4.x needs implementation of KERNEL32.dll.SetVolumeMountPointW
 - #41644  Civilization v1.2: crashes on startup
 - #42792  SQL Server 2012/2014: Installer requires ChangeServiceConfig2 with SERVICE_CONFIG_SERVICE_SID_INFO support (also affects Revit installer)
 - #44948  Multiple apps need CreateSymbolicLinkA/W implementation (Spine (Mod starter for Gothic), GenLauncher (Mod manager for GeneralsZH), MS Office 365 installer)
 - #49987  Multiple GTK Applications freeze/fail to start
 - #54927  grepwinNP3 (part of Notepad3) crashes inside uxtheme
 - #56577  QuarkXPress 2024 crashes on start with assertion error "symt_check_tag(&func->symt, SymTagFunction) || symt_check_tag(&func->symt, SymTagInlineSite)"
 - #57486  The Last Stand: Aftermath: Loads infinitely
 - #57703  Mega Man X DiVE Offline throws errors on startup unless regkeys for HKCR are added.
 - #57972  Certain display modes (e.g. 1152x864) not available in virtual desktop mode
 - #58023  Meld-3.22.2 fails to start with errormessagebox (retrieves incorrect path via XDG_DATA_HOME env var)
 - #58041  PlayOnline Viewer: Black screen when running via winewayland.
 - #58107  PlayOnline Viewer: Window not activated when restoring from a minimised state.
 - #58341  Incorrect mapping of "home" button of 8bitdo Pro 2 controller (in Xbox mode)
 - #58719  Wagotabi crashes on wine-10.15.
 - #58800  tlReader 10.1.0.2004 toolbar has broken rendering
 - #58831  the commit：win32u: Don't store the window OpenGL drawables on the DCs. Causing software deadlock
 - #58833  PlayOnline Viewer: Excessive virtual memory size possibly leading into a crash.
 - #58846  Geneforge 1 - Mutagen (Geneforge 2 - Infestation): black screen issue
 - #58880  Winecfg in wine 10.17 can not create controls(buttons,links ...) in some configurations
 - #58882  When setting Client Side Graphics=N (X11 Driver), interfaces such as winecfg.exe and regedit.exe display abnormally
 - #58908  Arrow keys unresponsive/stuck in some games
 - #58932  Some comboboxes are no longer sized correctly after commit in version 10.16
 - #58971  after commit 18ce7964203b486c8236f2c16a370ae27539d2f0 wine no longer execute windows steam
 - #58973  Many games crash on launch with new WoW64 wine-10.19 and discrete Nvidia GPU
 - #58984  imhex: Constant flickering --> GUI unusable (regression)
 - #58998  cmd broken, 'echo|set /p=%LOCALAPPDATA%' returns empty string
 - #59003  StarCraft: assertion failed
 - #59017  Synchronization barrier cannot be entered multiple times
 - #59034  Client area of CLM Explorer main window is rendered completely black on startup
 - #59050  HiveMQ CE 2025.5 crashes on startup (GetProcessHeap missing parentheses in iphlpapi.GetAnycastIpAddressTable stub)

### Changes since 10.19:
```
Adam Markowski (2):
      po: Update Polish translation.
      po: Update Polish translation.

Alexandre Julliard (13):
      vkd3d: Import upstream release 1.18.
      sxs: Implement SxspGenerateManifestPathOnAssemblyIdentity().
      sxs: Truncate fields when building a manifest file name.
      setupapi: Truncate fields when building a manifest file name.
      ntdll: Truncate fields when building a manifest file name.
      include: Add some new info classes.
      png: Import upstream release 1.6.51.
      ntdll: Support more ARM64 CPU features.
      ntdll: Always rely on mprotect() to set PROT_EXEC permission.
      iphlpapi: Fix GetProcessHeap typo.
      winedump: Move string dump functions to the common code.
      winedump: Add dumping of string and version resources for 16-bit.
      server: Use standard status value instead of win32 error.

Alfred Agrell (5):
      d2d1: Add Blend effect stub.
      d2d1: Add Brightness effect stub.
      d2d1: Add Directional Blur effect stub.
      d2d1: Add Hue Rotation effect stub.
      d2d1: Add Saturation effect stub.

Alistair Leslie-Hughes (1):
      urlmon: FindMimeFromData return only the mime type.

Anton Baskanov (38):
      dmsynth/tests: Test instrument selection.
      dmsynth: Factor out instrument fallback logic from synth_preset_noteon().
      dmsynth: Don't rely on the FluidSynth bank selection logic.
      dmsynth/tests: Add DLS tests.
      dmsynth: Use 0.1% as the sustain level unit.
      dmsynth: Don't add 1000 to the sustain modulators.
      dmsynth: Handle channel pressure events.
      dmsynth: Use a factor of 1/10 for modulation LFO x channel pressure -> gain connections.
      dmsynth: Trace lAttenuation as a signed integer.
      dmsynth: Handle sample attenuation.
      dmsynth: Handle bipolar transform for LFO connections.
      dmsynth: Set GEN_EXCLUSIVECLASS to the key group.
      dmsynth: Make voice shutdown instant.
      dmsynth: Explicitly ignore CONN_DST_EG1_SHUTDOWNTIME.
      dmsynth: Factor out play_region().
      dmsynth: Move find_region() after play_region().
      dmsynth: Add layering support.
      dmusic: Fix data size calculation in wave_create_from_soundfont().
      dmusic: Remove the unused loop_resent field from struct region.
      dmusic: Determine sample loop type from SF_GEN_SAMPLE_MODES.
      dmusic: Treat SF_GEN_(START|END)LOOP_ADDRS_OFFSET as offsets from the sample loop points.
      dmusic: Take coarse loop offsets into account.
      dmusic: Compare unity note with an unsigned 16-bit constant.
      dmusic: Take coarse tune into account in instrument_add_soundfont_region().
      dmusic: Set attenuation based on SF_GEN_INITIAL_ATTENUATION.
      dmusic: Add IIR filter resonance hump compensation.
      dmusic: Add an 8 dB attenuation to normalize SF2 instrument volume.
      dmusic: Take sample correction into account.
      dmusic: Treat fine tune as a signed value.
      dmusic: Set F_INSTRUMENT_DRUMS for bank 128.
      dmusic: Use SF_GEN_EXCLUSIVE_CLASS to set the key group.
      dmusic: Add preset generator values in a separate pass.
      dmusic: Don't pass preset generators to parse_soundfont_generators().
      dmusic: Intersect ranges for SF_GEN_KEY_RANGE and SF_GEN_VEL_RANGE.
      dmusic: Convert generators to DLS connections.
      dmusic: Add default modulators.
      dmusic: Parse instrument modulators.
      dmusic: Parse preset modulators.

Bernd Herd (10):
      sane.ds: Avoid segfault with backends that have integer array options like 'test'.
      sane.ds: Replace LocalLock/LocalUnlock with GlobalLock/GlobalUnlock.
      sane.ds: Store CAP_XFERCOUNT to activeDS.capXferCount.
      sane.ds: Apply SANE_Start() and SANE_Cancel().
      sane.ds: Fill TW_IMAGEMEMXFER.YOffset in SANE_ImageMemXferGet.
      sane.ds: Implement DG_CONTROL/DAT_PENDINGXFERS/MSG_GET based on scannedImages counter.
      sane.ds: Read frame data until EOF in native transfer mode.
      sane.ds: Load last settings from registry immediatly when opening the DS.
      sane.ds: Add cancel button and progress bar to progress dialog.
      sane.ds: Display message if ADF is empty in ADF scan.

Bernhard Kölbl (2):
      dwrite: Add the Cyrillic range to the fallback data.
      dwrite: Add the Supplemental Arrows-C range to the fallback data.

Bernhard Übelacker (6):
      user32/tests: Avoid out-of-bounds access in DdeCreateDataHandle (ASan).
      user32: Avoid out-of-bounds read in DdeCreateDataHandle with offset (ASan).
      ntdll/tests: Avoid out-of-bounds read in call_virtual_unwind_x86 (ASan).
      shell32: Avoid double-free in enumerate_strings when cur is zero (ASan).
      cmd: Skip directories if they exceed MAX_PATH in WCMD_list_directory (ASan).
      ntdll/tests: Dynamically load RtlIsProcessorFeaturePresent.

Biswapriyo Nath (1):
      include: Add symbols for av1 encoder in d3d12video.idl.

Brendan Shanks (5):
      winemac: Silence OpenGL-related warnings.
      user32/tests: Add tests for DisplayConfigGetDeviceInfo( DISPLAYCONFIG_DEVICE_INFO_GET_ADVANCED_COLOR_INFO ).
      win32u: Add semi-stub for NtUserDisplayConfigGetDeviceInfo( DISPLAYCONFIG_DEVICE_INFO_GET_ADVANCED_COLOR_INFO ).
      win32u: Store whether a monitor is HDR-capable in gdi_monitor.
      winemac: Report whether monitors are HDR-capable based on NSScreen.maximumPotentialExtendedDynamicRangeColorComponentValue.

Connor McAdams (6):
      d3dx10/tests: Use check_texture{2d,3d}_desc_values helpers in check_resource_info().
      d3dx11/tests: Use check_texture{2d,3d}_desc_values helpers in check_resource_info().
      d3dx9/tests: Add some image filter tests.
      d3dx10/tests: Add some image filter tests.
      d3dx11/tests: Add some image filter tests.
      d3dx: Implement a box filter.

Derek Lesho (4):
      win32u/tests: Set GL_DEDICATED_MEMORY_OBJECT_EXT on import.
      win32u/tests: Test named Vulkan export.
      win32u/tests: Test shared handle lifetime.
      win32u/tests: Test GL_EXT_semaphore_win32.

Dmitry Timoshkov (3):
      services: Return success for ChangeServiceConfig(SERVICE_CONFIG_SERVICE_SID_INFO).
      windowscodecs: Optimize a bit reading the 3bps RGB TIFF tile.
      kerberos: Add translation of 32-bit SecPkgContext_SessionKey in wow64_query_context_attributes() thunk.

Elizabeth Figura (19):
      wined3d: Get rid of alpha-based color keying.
      server: Retain the ? suffix when renaming or linking reparse points.
      kernelbase: Open the reparse point in CreateHardLink().
      kernelbase: Open the reparse point in SetFileAttributes().
      ntdll: Handle . and .. segments in relative symlinks.
      xactengine3/tests: Add tests for properties.
      xactengine3/tests: Add many more tests for notifications.
      xactengine3/tests: Test renderer details.
      xactengine3/tests: Test variables.
      xactengine3/tests: Test SetMatrixCoefficients() channel counts.
      xactengine3/tests: Test wavebank type mismatch.
      d3d11: Use GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS in D3D11CoreCreateDevice().
      d3d11: Implement DecoderExtension().
      wined3d: Make SHADOW into a format attr.
      winex11: Flush after presenting.
      ntdll: Implement FileIdExtdBothDirectoryInformation.
      kernelbase: Report the reparse tag from FindNextFile().
      kernel32: Implement SetVolumeMountPoint().
      mountmgr.sys: Report FILE_SUPPORTS_REPARSE_POINTS for volumes we report as NTFS.

Eric Pouech (11):
      cmd: Force flushing the prompt in SET /P command.
      kernel32/tests: Test std handles in CreateProcess with pseudo console.
      kernelbase: Create std handles from passed pseudo-console.
      kernel32/tests: Skip some tests when feature isn't present on Windows.
      cmd: Fix some quirks around CALL.
      cmd: Add more tests about echo:ing commands.
      cmd: Store '@' inside CMD_NODE.
      cmd: Don't use precedence when rebuilding commands.
      cmd: Properly echo commands.
      cmd: Match native output for $H in prompt.
      cmd: Expand loop variables nested in !variables!.

Erich Hoover (2):
      kernelbase: Open the reparse point in MoveFileWithProgress().
      kernelbase: Implement CreateSymbolicLink().

Gabriel Ivăncescu (2):
      jscript: Do existing prop lookups for external props only on objects with volatile props.
      mshtml: Add the node props to document fragments in IE9+ modes.

Giovanni Mascellani (1):
      mmdevapi: Fix WoW64 structure for is_format_supported.

Hans Leidekker (5):
      widl: Use the metadata short name for parameterized types.
      widl: Make sure attributes are added for imported member interfaces.
      widl: Don't add a type reference for IInspectable.
      include: Fix size_is syntax in IPropertyValue methods.
      wine.inf: Add .crt association.

Jacek Caban (4):
      opengl32: Free buffer wrappers after driver calls.
      opengl32: Factor out use_driver_buffer_map.
      opengl32: Support persistent memory emulation on top of GL_AMD_pinned_memory.
      opengl32: Simplify wow64_unmap_buffer.

Jactry Zeng (3):
      winemac.drv: Support to get EDID from DCPAVServiceProxy.
      winemac.drv: Support to get EDID from IODisplayConnect.
      winemac.drv: Support to generate EDID from display parameters from Core Graphics APIs.

Joe Souza (7):
      cmd/tests: Add tests for COPY to self.
      cmd: Don't attempt to copy a file to itself.
      xcopy: Fix leaked resource in an error case.
      xcopy/tests: Add test for XCOPY to self.
      xcopy: Don't attempt to copy a file to itself.
      xcopy: Return proper error code in copy file to self failure.
      cmd: Add ':' to delimiters for tab-completion support.

Ken Sharp (1):
      po: Update English resource.

Kun Yang (1):
      windowscodecs: Support NULL input palette in ImagingFactory_CreateBitmapFromHBITMAP.

Louis Lenders (2):
      wbemprox: Add PartialProductKey and ApplicationId to SoftwareLicensingProduct class.
      wbemprox: Use case insensitive search in get_method.

Marcus Meissner (1):
      kernel32/tests: Fix argument size to GetVolumeNameForVolumeMountPointW.

Matteo Bruni (6):
      mmdevapi: Share NULL GUID session.
      mmdevapi/tests: Accept any digit before the process ID in the session instance identifier.
      mmdevapi/tests: Test capture session state in render:test_session_creation().
      mmdevapi/tests: Get rid of questionable error handling.
      mmdevapi/tests: Simplify handling of failure to get a capture endpoint.
      mmdevapi/tests: Conditionally apply todo_wine to SetSampleRate() tests.

Nikolay Sivov (5):
      d2d1/tests: Explicitly use WARP device for IWICBitmapLock tests.
      d2d1/tests: Print adapter information.
      dwrite: Add an alternative name for the Noto Sans Symbols font.
      msxml: Add support for user-defined functions in msxsl:script blocks.
      msxml3: Ignore UseInlineSchema property.

Paul Gofman (23):
      comctl32/tests: Add tests for WM_MEASUREITEM with combobox.
      comctl32/combo: Adjust MEASUREITEMSTRUCT.itemHeight by 2 instead of 6.
      user32/combo: Adjust MEASUREITEMSTRUCT.itemHeight by 2 instead of 6.
      win32u: Only delete subkeys when clearing DirectX key.
      ntoskrnl.exe: Open thread with MAXIMUM_ALLOWED access in KeGetCurrentThread().
      quartz/dsoundrender: Get rid of DSoundRenderer_Max_Fill buffer data queue limit.
      quartz/test: Check for EC_COMPLETE more often in test_eos().
      quartz/dsoundrender: Send all queued samples to dsound before issuing EC_COMPLETE.
      quartz/dsoundrender: Do not send EC_COMPLETE while flushing.
      ntdll: Rename waiting_thread_count to structure_lock_count in barriers implementation.
      ntdll/tests: Add tests for iterative barrier usage.
      ntdll: Support iterative barrier usage without re-initialization.
      win32u: Initialize surface with white colour on creation.
      kernelbase: Set last error in GetModuleFileNameExW().
      win32u: Ignore startup cmd show mode for owned windows.
      win32u: Fetch startup info flags during initialization.
      win32u: Implement NtUserModifyUserStartupInfoFlags().
      user32/tests: Add tests showing that MessageBox() resets STARTF_USESHOWWINDOW.
      win32u: Clear STARTF_USESHOWWINDOW in MessageBoxIndirectW().
      msvfw32/tests: Add tests for MCI window styles.
      msvfw32: Add WS_CHILD attribute for window if parent is specified.
      msvfw32: Set correct styles for popup and child windows.
      msvfw32: Set child window id for MCI child window.

Piotr Caban (33):
      msado15: Add partial IRowset::AddRefRows implementation for tables without provider.
      msado15: Add IRowset::RestartPosition implementation for tables without provider.
      msado15: Fix test GetNextRows implementation.
      msado15: Set cursor position on provider side.
      msado15: Improve _Recordset::BOF implementation.
      msado15: Improve _Recordset::EOF implementatio.
      msado15: Improve IAccessor::CreateAccessor implementation for tables without provider.
      msado15: Add partial IRowset::GetData implementation for tables without provider.
      msado15: Add partial IRowset::SetData implementation for tables without provider.
      msado15: Fix initial row loading condition in _Recordset::MoveNext.
      msado15: Fix initial row loading condition in _Recordset::MovePrevious.
      msado15: Store column ordinal number in field structure.
      msado15: Reimplement Field::get_Value and Field::put_Value.
      msado15: Fix memory leak for tables without provider.
      msado15: Don't use row index in _Recordset::get_EditMode.
      msado15: Remove _Recordset::put_Filter hack.
      msado15: Implement IRowsetLocate::GetRowsAt for tables without provider.
      msado15: Set bookmark column value in rowset_change_InsertRow.
      msado15: Handle data type conversion in rowset_GetData.
      msado15: Use IRowsetLocate to access database rows if available.
      msado15: Reimplement _Recordset::{put,get}_Bookmark.
      msado15: Add _Recordset::MoveFirst test.
      msado15: Remove unused code for storing/manipulating database data in _Recordset object.
      msado15: Remove IRowset QueryInterface checks.
      msado15: Change data type used for fetching longer bookmarks.
      msado15: Add INT_PTR bookmark type.
      msado15: Store data in column format in memory rowset provider.
      include: Add IRowsetView interface.
      include: Add IViewChapter interface.
      include: Add IViewFilter interface.
      msado15: Add IRowsetUpdate interface in tests.
      msado15: Fix crash in _Recordset::Close() when releasing uninitialized Field objects.
      msado15: Add more _Recordset::put_Filter tests.

Rémi Bernon (51):
      win32u: Make a copy of the GL_RENDERER / GL_VENDOR strings.
      win32u: Move vulkan device wrapper from winevulkan.
      winevulkan: Allocate instance static debug objects dynamically.
      win32u: Move instance wrappers from winevulkan.
      win32u: Use the vulkan instance wrappers for D3DKMT.
      winex11: Set dmDriverExtra for detached full modes.
      hid: Implement HidP_SetData.
      win32u: Set the DC pixel format too in wglSetPixelFormatWINE.
      win32u: Keep the D3D internal OpenGL surfaces on the DCs.
      win32u: Get rid of window internal pixel format.
      win32u: Use 0x20 for iconic WM_ACTIVATE message wparam.
      win32u: Avoid INT_MAX overflow in map_monitor_rect.
      win32u: Avoid crashing if Vulkan is disabled or failed to load.
      winex11: Avoid unmapping window if it only got layered style.
      winevulkan: Avoid returning innacurate extension counts.
      winex11: Track requested WM_NORMAL_HINTS to avoid unnecessary requests.
      winex11: Track requested WM_HINTS to avoid unnecessary requests.
      winex11: Track requested _NET_WM_WINDOW_STATE to avoid unnecessary requests.
      winex11: Track requested _NET_WM_ICON to avoid unnecessary requests.
      win32u: Keep devices EGL platforms in a list.
      win32u: Add the display device first in the EGL device list.
      win32u: Terminate non-display EGL devices after initialization.
      win32u: Skip non-display software EGL devices initialization.
      win32u: Fix crash in NtUserUpdateLayeredWindow if blend is NULL.
      winex11: Initialize thread data when checking _NET_WM_STATE mask.
      winex11: Don't update client maximized state if window is minimized.
      win32u: Release internal OpenGL drawables outside of the user lock.
      win32u: Avoid leaking semaphore and fence exported fds.
      include: Add some new DMO classes to wmcodecdsp.idl.
      msvdsp: Add stub dll.
      vidreszr: Add stub dll.
      mfsrcsnk: Ignore streams with unsupported media types.
      winevulkan: Move api checks out of the constructors.
      winevulkan: Simplify enum alias handling.
      winevulkan: Split get_dyn_array_len into params / member classes.
      winevulkan: Pass parent / params to get_dyn_array_len directly.
      winevulkan: Create function param and struct member lists beforehand.
      winevulkan: Precompute struct type constant from member list.
      win32u: Add some extra 4:3 resolutions to the virtual modes.
      winex11: Pass client rect to create_client_window.
      winex11: Remove some unnecessary NtUserGetClientRect calls.
      ddraw/tests: Use a dedicated window instead of the desktop window.
      user32/tests: Use a dedicated window instead of the desktop window.
      win32u: Introduce a NtUserSetForegroundWindowInternal call.
      server: Set NULL foreground input when switching to desktop window.
      server: Forbid background process window reactivation.
      winex11: Update window position in client surface update callback.
      win32u: Update client surfaces starting from toplevel window.
      winex11: Only use XRender bilinear filter with client-side graphics.
      win32u: Skip minimized windows when looking for another window to activate.
      win32u: Hide owned popups after minimizing their owner window.

Stian Low (6):
      wined3d: Move the Vulkan blitter to texture_vk.c.
      wined3d: Move Vulkan texture functions to texture_vk.c.
      wined3d: Move GL texture functions to texture_gl.c.
      wined3d: Move the FBO blitter to texture_gl.c.
      wined3d: Move the raw blitter to texture_gl.c.
      wined3d: Move the FFP blitter to texture_gl.c.

Tim Clem (1):
      winex11.drv: Set use_egl to false if it is unavailable.

Vibhav Pant (26):
      rometadata: Add stubs for IMetaDataImport.
      rometadata/tests: Add tests for IMetaDataTables::{EnumTypeDefs, GetTypeDefProps, FindTypeDefByName}.
      rometadata/tests: Add tests for IMetaDataImport::{EnumMethods, GetMethodProps, GetNativeCallConvFromSig}.
      rometadata/tests: Add tests for IMetaDataImport::{EnumFields, GetFieldProps}.
      rometadata/tests: Add tests for IMetaDataImport::GetCustomAttributeByName.
      rometadata/tests: Add tests for IMetaDataImport::{EnumProperties, GetPropertyProps}.
      rometadata: Implement IMetaDataImport::{EnumTypeDefs, CountEnum, ResetEnum}.
      rometadata: Implement IMetaDataImport::GetTypeDefProps.
      rometadata: Implement IMetaDataImport::FindTypeDefByName.
      rometadata: Implement IMetaDataImport::{EnumMethods, GetMethodProps}.
      rometadata: Implement IMetaDataImport::EnumFields.
      rometadata: Implement IMetaDataImport::GetFieldProps.
      widl: Fix MethodList value for apicontract and enum typedefs.
      rometadata/tests: Add tests for IMetaDataDispenser::OpenScopeOnMemory.
      rometadata/tests: Add additional tests for IMetaDataImport::GetCustomAttributeByName.
      rometadata: Perform bound checks before decoding blob sizes in assemblies.
      rometadata: Fix incorrect bit width calculation.
      rometadata: Implement IMetaDataDispenser::OpenScopeOnMemory.
      rometadata: Implement IMetaDataTables::{EnumProperties, GetPropertyProps}.
      rometadata: Implement IMetaDataImport::GetCustomAttributeByName.
      rometadata/tests: Add tests for IMetaDataImport::{EnumMethodsWithName, FindMethod}.
      rometadata/tests: Add tests for IMetaDataImport::{EnumFieldsWithName, FindField}.
      rometadata/tests: Add tests for IMetaDataImport::{EnumMembersWithName, FindMember}.
      rometadata: Implement IMetaDataImport::EnumMethodsWithName.
      rometadata: Implement IMetaDataImport::FindMethod.
      rometadata: Implement IMetaDataImport::{EnumFieldsWithName, FindField}.

Vijay Kiran Kamuju (2):
      vcomp: Add omp_get_wtick() implementation.
      user32: Fix loading cursor image with resource id using LR_LOADFROMFILE on older windows versions.

Yuxuan Shui (4):
      mf/tests: Test what's returned from ProcessOutput when input ran out.
      winegstreamer: Return S_FALSE from DMO when there is not enough data.
      winegstreamer: Only change DMO's output type if SetOutputType is successful.
      winegstreamer: Add missing read thread wait in SetOutputProps failure path.

Zhiyi Zhang (25):
      comctl32/tests: Add tests for toolbar WM_ERASEBKGND handling.
      comctl32/tests: Add tests for toolbar WM_PAINT handling.
      comctl32/toolbar: Erase the background in TOOLBAR_Refresh() when TBSTYLE_TRANSPARENT is present for comctl32 v6.
      comctl32/trackbar: Add a helper to get the pen color for drawing tics.
      comctl32/trackbar: Use COMCTL32_IsThemed() to check if theme is enabled.
      comctl32/trackbar: Remove theming for comctl32 v5.
      comctl32/treeview: Add a helper to draw plus and minus signs.
      comctl32/treeview: Add a helper to fill theme background.
      comctl32/treeview: Remove theming for comctl32 v5.
      comctl32/updown: Add a helper to get the buddy border size.
      comctl32/updown: Add a helper to get the buddy spacer size.
      comctl32/updown: Add helpers to get the arrow state.
      comctl32/updown: Add helpers to get the arrow theme part and state.
      comctl32/updown: Add a helper to check if buddy background is needed.
      comctl32/updown: Refactor UPDOWN_DrawBuddyBackground() to support drawing background when theming is disabled.
      comctl32/updown: Add a helper to draw the up arrow.
      comctl32/updown: Add a helper to draw the down arrow.
      comctl32/updown: Remove theming for comctl32 v5.
      comctl32: Remove theming for comctl32 v5.
      comctl32_v6/taskdialog: Fix not enough width for the expando button text.
      icu: Add stub dll.
      icuuc: Add initial dll.
      icuin: Add initial dll.
      include: Add icu.h.
      include: Add more definitions from ICU 72.1.
```
