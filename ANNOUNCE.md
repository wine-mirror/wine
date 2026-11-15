The Wine development release 11.22 is now available.

What's new in this release:
  - Locale data updated to Unicode CLDR 49.
  - AVX support in x86 emulation on ARM64.
  - Address space randomization.
  - Improved glyph metrics in Tahoma font.
  - DTD validation reimplemented in MSXML.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/11.x/wine-11.22.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-11.22/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 11.22 (total 34):

 - #26037  Tahoma Bold is not bold
 - #27559  ImgBurn 2.5.5.0 and earlier needs shell32.SHGetKnownFolderIDList() implementation when run with WinVer >= Vista
 - #41438  fastone image viewer 5.9 "crop" window do not show if called by hotkey from full screen mode
 - #47837  Logos 8 Bible Software (.Net 4.7.2 app) has graphics glitches
 - #52379  Logos 9 Bible Software (.NET 4.7 app) crash when opening Faithlife assistant AI tool
 - #52380  Logos Bible Software (.NET app) menus glitching, flickering and not showing
 - #53913  Guild Wars 2 crashes on launch
 - #55405  FreeCommander is crashing immediately after it starts, when Windows version is set above Windows XP
 - #56154  Potential nullpointer dereference in wow64_NtQueryInformationToken (Coverity)
 - #56253  Perry Rhodan - The Adventure (2008 game): demo crashes on launch
 - #59749  sapi: ISpVoice::SetPriority and GetStatus return E_NOTIMPL, breaking Overwatch 2 Audio Captions (and likely any SAPI app that polls status or checks SetPriority's HRESULT)
 - #60105  Crossrunner emulator crashes on loading a hard drive image file
 - #60394  Kubios HRV Scientific installer crashes
 - #60443  ncrypt: Support importing ECDSA key blobs.
 - #60478  Wine's .winmd files have "include/" in their assembly names; .NET Framework WPF apps fail with 0x80131047 (Razer installer)
 - #60479  CloneCD shows ntoskrnl:ZwLoadDriver failed to create driver ... ElbyCDIO: c0000142
 - #60487  wbemprox: Win32_Service lacks Description, PathName, StartName and ChangeStartMode()
 - #60547  Star Citizen CrashHandler not running
 - #60564  BSG Launcher via Wine: keyboard input is not detected, not working.
 - #60594  Missing RGB565 Vulkan swapchain fallback causes crashes in multiple games
 - #60596  Black Mirror II and III: broken character models and animations with the Vulkan renderer
 - #60607  Dragon Age: Origins crashes during intro and loading screens with the Vulkan renderer due to unsupported L8 textures
 - #60626  Logos Bible 50.x fails to launch
 - #60663  ntdll: A thread terminated before the server reads its init_thread request blocks forever in read_reply_data()
 - #60664  Alan Wake and Alan Wake's American Nightmare fail to start with the Vulkan renderer due to unsupported D24X8 depth format
 - #60669  Notepad ++ shows wrong menu bar
 - #60672  wineserver: new_thread fails with STATUS_INVALID_CID after creating the thread when receive_fd() drops a terminated thread's fd
 - #60677  d2d1: SetDrawInfo arrives after SetSingleTransformNode returns during custom effect initialization
 - #60678  Swiss locales use the narrow "Fr." currency symbol instead of "CHF" since CLDR 49
 - #60685  psapi:psapi_main reports the exe header page as not shared since exe base randomization
 - #60691  Clicking "About" in WinSCP crashes since Wine 11.11
 - #60692  Samsung Magician 9 service crashes: Win32_Processor has no Status, CurrentVoltage and ExtClock
 - #60748  F.E.A.R. 3 exits on startup because FindResourceEx no longer tries the explicitly requested language ID 0x8301
 - #60750  XSLPattern queries for names starting with "not" fail

### Changes since 11.21:
```
Adam Markowski (1):
      po: Update Polish translation.

Alex Henrie (2):
      shlwapi/tests: Expand StrCmpN*/StrIsIntlEqual*/IntlStrEq* tests.
      kernelbase: Don't go past null in StrCmpN* or StrIsIntlEqual*.

Alexandre Julliard (34):
      ntdll: Grow the reserved areas when they run out of space.
      ntdll: Don't look for free memory inside reserved areas.
      nls: Add some new locales.
      ntdll: Close the reply pipe before sending the initial request.
      ntdll: Print module name if possible with exception address.
      server: Randomize dll base addresses.
      server: Also assign a random base address to executables.
      ntdll: Randomize exe base addresses.
      psapi/tests: Mark a failing test as todo.
      nls: Fix currency values for a few locales.
      server: Don't return an error when thread_get_inflight_fd() succeeds.
      ntdll: Open the global option keys before loading the main exe.
      ntdll: Load the process mitigation options.
      ntdll: Randomize virtual memory allocations.
      mpg123: Import upstream release 1.33.7.
      png: Import upstream release 1.6.59.
      tiff: Import upstream release 4.7.2.
      zlib: Import upstream release 1.4.
      lcms2: Import upstream release 2.20.
      capstone: Import upstream release 5.0.9.
      sqlite3: Import upstream release 3.54.1.
      ntdll: On i386, check actual host allocations instead of claiming everything is reserved.
      ntdll: Randomize virtual memory allocations in 32-bit builds.
      ntdll: Also try invalid locale ids in resource lookups.
      ntdll: Use the correct access rights for FileValidDataLengthInformation.
      ntdll: Add a context structure to pass around when resolving imports.
      ntdll: Return an NTSTATUS from the export lookup helpers.
      server: Pass the open file disposition to the open_fd() helper.
      server: Pass the open file disposition to the check_sharing() helper.
      server: Add a helper to open a file, falling back to read-only for directories.
      server: Don't require Unix write access to change file attributes.
      ntdll: Don't switch to 32-bit if the thread entry point is 64-bit.
      wow64win: Fix return device mapping in wow64_NtUserGetRawInputDeviceList().
      win32u: Specify zero bits when mapping a DIB section.

Ali Milhim (9):
      wined3d: Recreate stale shader resource image views in the Vulkan backend.
      d3d9/tests: Test drawing with a managed texture after evicting it.
      wined3d: Support L8, L8A8 and L16 formats in the Vulkan backend.
      wined3d: Use scaled vertex formats for integer attributes before D3D10 in the Vulkan backend.
      wined3d: Don't use a depth/stencil buffer with a mismatched sample count in the Vulkan renderer.
      wined3d: Map WINED3DFMT_X8D24_UNORM to VK_FORMAT_X8_D24_UNORM_PACK32.
      wined3d: Clip Vulkan clears to attachment bounds.
      ntdll: Follow at most 32 export forwarders.
      wined3d: Report occlusion for minimised windows.

Alistair Leslie-Hughes (10):
      windowscodecs: Extensions are comma separated.
      mpr: Remove duplicate defines.
      avifil32: Correctly compare fcc type in IAVIFile::Load.
      sechost: Correct format string in print_sid_numeric.
      quartz/tests: Test IObjectWithSite::GetSite() with no site.
      quartz: Return E_NOINTERFACE from GetSite() when there is no site.
      quartz: Allow IObjectWithSite::SetSite(NULL).
      odbc32: SQLColAttributes* accept field id SQL_COLUMN_UNSIGNED.
      odbccp32: Section name cannot be empty in SQLWritePrivateProfileStringW.
      odbccp32: Section name cannot be blank in SQLGetPrivateProfileStringW.

Andrey Gusev (1):
      mscoree: Check for NULL This in DllGetClassObject().

Aurimas Fišeras (1):
      po: Update Lithuanian translation.

Bernhard Übelacker (3):
      shell32: Improve handling of KF_FLAG_NO_ALIAS in SHGetKnownFolderIDList.
      services: Improve binary type detection with non-absolute paths.
      ntdll/tests: Test that read-only files deny write-data access combined with FILE_READ_ATTRIBUTES.

Brendan McGrath (9):
      amstream: Fix segfault when 'ipin' is NULL.
      amstream/tests: Add tests for IAMMultiMediaStream::Render.
      amstream: Handle an uninitialised stream in IAMMultiMediaStream::Render.
      amstream: Handle STREAMTYPE_READ in IAMMultiMediaStream::Render.
      amstream: Render all streams in IAMMultiMediaStream::Render.
      amstream: Reject stream if it is not STREAMTYPE_WRITE.
      amstream: Allow AMMSF_RENDERALLSTREAMS on STREAMTYPE_TRANSFORM.
      amstream: Remove unused ipin field.
      quartz: Rename parameters to match the API.

Brendan Shanks (2):
      winemac: In GetDeviceCaps(), don't double HORZ/VERTRES in retina mode.
      winemac: Only take the device_data_mutex when necessary in GetDeviceCaps().

Connor McAdams (3):
      ntoskrnl/tests: Add some tests for device interfaces with a refstr.
      setupapi/tests: Add a refstr test for CM_Get_Device_Interface_List().
      cfgmgr32: Fix paths returned by CM_Get_Device_Interface_List{A,W}.

Conor McCarthy (5):
      mfreadwrite/tests: Test sink writer creation from an audio renderer.
      mfreadwrite: Initialise the media sink writer current media type on creation if not set.
      mfreadwrite: Forward media sink writer input media type change to the media sink if current type is default.
      mfreadwrite: Initialise audio converter types in the correct order.
      mfreadwrite: Handle audio categories in GetTransformForStream().

Daniel Lehman (2):
      msxml3/tests: Add some transformNodeToObjects tests.
      msxml3: Remove existing xml declaration to avoid duplicates.

Dmitry Timoshkov (1):
      include: Add _Field_size_opt_() compatibility macro.

Elizabeth Figura (22):
      wined3d: Collapse "d3d9 or earlier" flags into a single flag.
      kernel32/tests: Test FileMailslotSetInformation.
      server: Introduce a set_file_info request.
      ntdll: Move NtSetInformationFile(FilePipeInformation) to the server.
      ntdll: Move NtSetInformationFile(FileMailslotSetInformation) to the server.
      quartz: Remove an unnecessary helper.
      quartz/vmr7: Auto-show the window on Run().
      quartz/vmr9: Auto-show the window on Run().
      ntdll: Move NtSetInformationFile(FileIoCompletionNotificationInformation) to the server.
      ntdll: Move NtSetInformationFile(FileEndOfFileInformation) to the server.
      server: Implement NtSetInformationFile() for device files.
      ntoskrnl: Handle NtSetInformationFile(FilePositionInformation).
      ntoskrnl/tests: Test how NtReadFile() offsets are translated.
      ntoskrnl: Handle FILE_USE_FILE_POINTER_POSITION.
      ntdll: Translate a NULL offset to FILE_USE_FILE_POINTER_POSITION.
      http: Use case insensitive comparison for headers.
      mountmgr: Implement IRP_MJ_READ for cdrom devices.
      mountmgr: Return the correct size from IOCTL_SCSI_PASS_THROUGH.
      ntdll: Translate IOCTL_SCSI_PASS_THROUGH_DIRECT to IOCTL_SCSI_PASS_THROUGH.
      mountmgr: Remove IOCTL_SCSI_PASS_THROUGH_DIRECT implementation.
      eject: Open the drive with read access.
      mountmgr: Open CD devices with O_NONBLOCK.

Eric Pouech (9):
      dbghelp: Fix some memory leaks.
      dbghelp: Fix a couple of missing memory releases on error path.
      dbghelp/pdb: Rewrite enumeration in linetab2.
      dbghelp/pdb: Add protection against potential bogus values in PDB files.
      dbghelp: Handle NULL Mask in SymEnumSourceFilesW.
      cmd/tests: Add tests for DIRCMD env variable.
      cmd: Fix DIRCMD usage in DIR command.
      include: Add definition for VariantCompare in propvarutil.h.
      propsys: Add stub for VariantCompare().

Esme Povirk (4):
      kernelbase: Update tzdata to 2026f.
      nls: Update locale data to CLDR version 49.
      windowscodecs: Remove unused flag.
      windowscodecs: Remove unused declaration.

Eunchan Lee (6):
      msxml3: Increment the sink count only when appending.
      po: Update fuzzy and untranslated messages in Korean translation.
      po: Use standard terms in Korean security and network messages.
      po: Use consistent terms in Korean translation.
      po: Fix typos and mistranslations in Korean translation.
      po: Rewrap Korean cmd help text to fit 80 columns.

Evan Morse (3):
      wined3d: Upload the whole buffer on writes unless it is managed.
      wined3d: Validate vertex declarations against an optional shader.
      d3d8: Reject vertex shaders that read undeclared input registers.

Fabien Romano (2):
      wpcap: Pass PCAP_CFLAGS to the Unix library build.
      dbghelp: Search .debug beside the module's real path.

Francis De Brabandere (2):
      vbscript/tests: Add ChrW tests for negative character codes.
      vbscript: Skip a BOM at the start of the script text.

Gabriel Ivăncescu (2):
      mshtml: Add IMarkupContainer2 stub for document objects.
      mshtml: Add IMarkupContainer2 stub for document nodes.

Hans Leidekker (26):
      bcrypt: Properly validate key size when importing generic ECC blobs.
      winhttp: Consistently use src_len in str_to_wire().
      winhttp: Allow 'read' to be NULL in stream_Read().
      winhttp: Remove dead code.
      winhttp: Always perform status callback on error in send_request().
      winhttp: Return an error on allocation failure in handle_redirect().
      winhttp: Return on failure from add_request_headers() in add_cookie_headers().
      winhttp: Use a single allocation in create_async_resolve().
      winhttp: Check for allocation failure in insert_header().
      winhttp: Don't touch BSTR out parameters on error.
      winhttp: Preserve errors in send_and_receive_proc().
      winhttp: Remove unused header query entries.
      winhttp: Fix string termination in handle_redirect().
      wbemprox: Validate len in fill_rawbiosdata().
      wbemprox: Handle optional fields in fill_printer().
      wbemprox: Handle NULL resource in parse_resource().
      wbemprox: Fix row index in fill_printer().
      wbemprox: Return RegCreateKeyExW() error as method result.
      wbemprox: Don't discard the context in wbem_services_OpenNamespace().
      wbemprox: Avoid assigning uninitialized process ID in process_create().
      wbemprox: Check for exact match in parse_resource().
      wbemprox: Fix memory leak on error in get_display_adapters().
      wbemprox: Increase buffer size if necessary in enum_values().
      wbemprox: Handle NULL out parameter in process_create().
      wbemprox: Add a couple of Win32_Processor properties.
      msi: Run msiexec under emulation for x86_64 custom action dlls if the host CPU doesn't match.

Ivan Ivlev (2):
      msi: Test common desktop selection for ALLUSERS.
      msi: Use the common desktop when ALLUSERS is set.

Jacek Caban (22):
      windowscodecs/tests: Use unsigned char for test data.
      include: Add missing ARM64 context defines.
      server: Rename ymm registers to vector.
      server: Store upper half of ARM64 floating point registers as vector registers.
      ntdll: Add support for CONTEXT_ARM64_FLOATING_POINT_LOW/HIGH context flags.
      ntdll/tests: Add CONTEXT_ARM64_FLOATING_POINT_LOW/HIGH tests.
      ntdll: Introduce user_xstate_config.
      ntdll: Support CONTEXT_XSTATE in context_x64_to_arm.
      ntdll: Pass EC context flags to context_arm_to_x64.
      ntdll: Setup process-local xstate config on ARM64EC.
      ntdll: Support CONTEXT_XSTATE in NtGetContextThread on ARM64EC.
      ntdll: Clear CONTEXT_XSTATE flag in consolidate_callback.
      ntdll: Support xstate in dispatch_emulation.
      ntdll: Support xstate in ARM64EC exception dispatcher.
      ntdll: Support xstate in ARM64EC APC dispatcher.
      ntdll: Support xstate in ARM64EC LdrInitializeThunk.
      ntdll: Move RtlGetEnabledExtendedFeatures implementation to signal files.
      ntdll: Initialize x86 features for ARM64EC in init_shared_data_cpuinfo.
      ntdll: Initialize XState config in user shared data on ARM64.
      ntdll: Use RtlGetEnabledExtendedFeatures in ARM64EC context functions.
      ntdll: Use shared XState config on ARM64EC.
      Revert "ntdll: Introduce user_xstate_config."

James Hawkins (6):
      cabinet: Return FDIERROR_TARGET_FILE when writing extracted data fails.
      quartz/tests: Check reference clock time after advise signal instead of racing wait timeouts.
      urlmon: Do not open cache file when binding to a file.
      urlmon/tests: Delete cached redirect entry before redirect protocol tests.
      mfplat: Wait for pending copies before mapping a d3d12 upload buffer.
      wininet: Clean up on failure when initializing cache index.

Joe Souza (1):
      cmd: Fix use of /O, /A, and /T in DIR command, each option with no parameters specified.

Joel Holdsworth (1):
      ntdll/tests: Test opening read-only files only for read/write attributes.

Marc-Aurel Zent (9):
      winemac: Also return alpha channel in macdrv_window_background_color.
      winemac: Use IOSurface to update native window layers instead of CGImage.
      winemac: Use a DIB section instead of CGDataProvider for window surfaces.
      winemac: Update the clipboard asynchronously on app activation.
      winemac: Handle windowDidMiniaturize fully on the main thread.
      winemac: Split off performing inactive window commands into its own helper.
      winemac: Add WM_MACDRV_INACTIVE_WINDOW_COMMAND to perform inactive commands.
      winemac: Remove WINDOW_MAXIMIZE_REQUESTED event.
      winemac: Remove WINDOW_MINIMIZE_REQUESTED event.

Nikolay Sivov (38):
      server: Fix copy-paste issue with monitor info traces.
      server/class: Fix missing object release on failure path.
      server/class: Remove early returns from shared section write block.
      dwrite/tests: Add some more tests for AddFontFile().
      dwrite: Ignore unsupported font files when creating custom collections.
      shell32/shellview: Handle clipboard commands via keyboard shortcuts.
      kernel32/tests: Add some tests for FileTimeToDosDateTime().
      kernel32: Improve rounding behavior in FileTimeToDosDateTime().
      msxml3/tests: Add some tests for nodeFromID() method.
      msxml6/tests: Add a validateNode() test for detached element with ID attribute.
      kerberos: Avoid use-after-free of auth data on failure paths.
      msv1_0: Avoid use-after-free of auth data on failure paths.
      msxml3: Use existing array resize helper in the namespace manager.
      msxml3: Use existing helper to resize sinks array.
      msi: Avoid use-after-free of a control text.
      include: Fix IDebugEventContextCallbacks::CreateProcess() prototype.
      d2d1/effect: Set render info when adding a node to the transform graph.
      shell32/shellview: Force list refresh on clipboard operations.
      d3dx9: Avoid file data name use-after-free on failure path.
      mfplat/buffer: Do not leave uninitialized buffer on d3d9 LockRect() failure.
      mfplat/buffer: Do not leave uninitialized buffer on DXGI surface mapping failure.
      mfplat: Fix new collection size in InsertElementAt().
      d2d1: Do not use pointers for variable data fields when recording command lists.
      d2d1: Fix potential leak on error path.
      d2d1: Fix potential factory reference leak on failure path.
      d2d1/effect: Fix potential leak of failure path in CreateVertexBuffer().
      d2d1/geometry: Use correct transform for transformed geometry's Tessellate().
      msxml3/tests: Add a validation test with duplicate ID values.
      msxml3: Add DTD validation check for duplicated ID values.
      msxml3: Implement IDREF/IDREFS validation checks.
      msxml3: Implement DTD validation of complex content models.
      msxml3/xpath: Improve not() handling in XSLPattern mode.
      msxml3/xpath: Improve 'or' operator parsing in XSLPattern.
      msxml3/xpath: Improve 'and' operator parsing in XSLPattern.
      msxml3/xpath: Check for expression parser failure before accessing its output.
      msxml3: Collect attribute ID values from the rest of the documenting, when validating a subtree.
      msxml3: Do not use libxml2 for DTD validation.
      dwrite/tests: Fix a test crash on older Windows 10 systems.

Owen Rudge (1):
      cscript: Build as ARM64X.

Paul Gofman (13):
      mfwmaaec: Implement audio capture in transform_ProcessOutput().
      preloader: Avoid rearranging ro segment.
      capabilityaccessmanagerclient: Add dll with stub IAppCapabilityStatics implementation.
      capabilityaccessmanagerclient: Return semi-stub interface from capability_statics_Create().
      msctf: Support ITextStoreACP2 callback interface.
      win32u: Set Radeon Software version in the registry.
      ddraw: Do not force render target for surface formats which do not support it.
      win32u: Only rescan rawinput devices if the devices config could have changed.
      windows.gaming.input: Reset index before enumeration in initialize_providers().
      kernelbase: Clamp year to dynamic range boundaries in GetTimeZoneInformationForYear().
      kernel32: Implement SystemTimeToTzSpecificLocalTimeEx().
      kernelbase: Do a more accurate TZ dates comparison in get_timezone_id().
      kernel32: Implement TzSpecificLocalTimeToSystemTimeEx().

Piotr Caban (5):
      propsys: Implement VariantCompare().
      propsys/tests: Add VariantCompare() tests.
      odbc32: Always pass name_len parameter to SQLDescribeCol driver function.
      odbc32: Fix NULL name parameter handling in SQLDescribeCol conversion.
      odbc32: Add ANSI conversion in SQLDescribeColW().

Rémi Bernon (11):
      win32u: Use compatibility context profile by default.
      win32u: Force recalculating menu bar height in get_menu_bar_height.
      winex11: Drop unnecessary gl_drawable functions.
      win32u: Move the NULL OpenGL surface to winex11.
      winex11: Use glXCreateContextAttribsARB unconditionally.
      win32u: Set internal context attribs and root even if creation failed.
      winex11: Ignore context format if GLX_EXT_no_config_context is supported.
      win32u: Recreate internal contexts with the drawable pixel format.
      win32u: Don't strip WGL_CONTEXT_DEBUG_BIT_ARB context flag.
      win32u: Don't try to load EGL if UseEGL option is disabled.
      win32u: Always write to NtUserToUnicodeEx output buffer.

Sky Leite (1):
      crypt32: Check CertAddEncodedCertificateToStore() return value in sync_trusted_roots_from_known_locations().

Vibhav Pant (8):
      widl: Don't sort rows for the MethodDef table.
      widl: Set metadata_name correctly when processing multiple input files.
      widl: Add initial support for merging TypeDef metadata tables.
      widl: Merge the field list column while merging TypeDefs.
      widl: Merge the method list column while merging TypeDefs.
      widl: Add support for merging InterfaceImpl metadata tables.
      widl: Add support for merging MemberRef metadata tables.
      gdiplus: Optimize pixel coverage calculation in SOFTWARE_GdipFillPathAntialiased.

Vishnunithyasoundhar S (1):
      msvcrt/scanf: Fix crash in fscanf due to null dereferencing a file.

Vitaly Lipatov (32):
      gdiplus/tests: Test GdipGetPathData() with different point counts.
      gdiplus: Fix the point count handling in GdipGetPathData().
      include: Add GdipGetEncoderParameterListSize() and GdipIsVisibleClipEmpty() declarations.
      gdiplus: Add missing NULL checks.
      configure: Check for vaExportSurfaceHandle() in libva.
      fonts: Restore the widths of composite glyphs in Tahoma Bold.
      fonts: Restore the accent positions of composite glyphs in Tahoma Bold.
      msi/tests: Test installing a package of an installed product without REINSTALLMODE=v.
      msi: Match the cached package by package code instead of product version.
      wininet/tests: Allow a pending data query to complete with no data.
      mshtml: Don't use the released document object in navigation callbacks.
      mfplat: Wait for pending copies before releasing a d3d12 buffer.
      gdiplus: Fill the WMF header of WMF metafiles in GdipGetMetafileHeaderFromMetafile().
      fonts: Make the remaining Latin letters and symbols bold in Tahoma Bold.
      po: Update Russian translation.
      crypt32: Don't close the certificate file twice.
      include: Use Status and declare CGpEffect in gdipluseffects.h.
      include: Declare the remaining effect functions in gdipluseffects.h.
      fonts: Fix the advance widths of z, U+2116 and U+2219 in Tahoma Bold.
      fonts: Fix the advance widths of Cyrillic Ya and Ka in Tahoma.
      fonts: Fix the advance widths of more glyphs in Tahoma Bold.
      fonts: Fix the advance widths of more glyphs in Tahoma.
      fonts: Make Cyrillic letters bold in Tahoma Bold.
      fonts: Fix the advance widths of some Cyrillic letters in Tahoma Bold.
      include: Add TBYTE and PTBYTE.
      include: Add some missing shellapi flags.
      include: Add ListView_ApproximateViewRect().
      include: Add the _MAX_*TOSTR_*_COUNT constants.
      gdiplus: Fix the status returned for files that can't be opened.
      user32/tests: Remove todo_wine from the system DPI checks of a DPI unaware process.
      fonts: Fix the glyph placement in the Tahoma Bold bitmap strikes.
      fonts: Fix the glyph placement in the Tahoma bitmap strikes.

Xu Zhao (2):
      quartz/tests: Test auto-show with Run() as well.
      quartz/videorenderer: Auto-show the window on Run().

Zhiyi Zhang (11):
      services/tests: Test CreateEnvironmentBlock() for service accounts.
      userenv: Add user profile environment variables for service accounts in CreateEnvironmentBlock().
      services/tests: Add tests for SHGetFolderPathW().
      shell32: Report correct %USERPROFILE% values for service accounts in _SHExpandEnvironmentStrings().
      aero.msstyles: Fix typos for taskdialog and tooltip parts.
      uxtheme/tests: Test font sizes in the theme file.
      aero.msstyles: Adjust system font sizes.
      aero.msstyles: Fix the font sizes for taskdialog and tooltips.
      aero.msstyles: Make tooltips text color darker.
      uxtheme: Fix parsing negative integers.
      winemac.drv: Set window foreground when doing unminimization.
```
