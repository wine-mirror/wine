The Wine development release 10.16 is now available.

What's new in this release:
  - Fast synchronization support using NTSync.
  - 16-bit apps supported in new WoW64 mode.
  - Initial support for D3DKMT objects.
  - WinMD (Windows Metadata) files generated and installed.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/10.x/wine-10.16.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-10.16/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 10.16 (total 34):

 - #7115   Need for Speed III installer fails in Win9X mode, reporting "Could not get 'HardWareKey' value" (active PnP device keys in 'HKEY_DYN_DATA\\Config Manager\\Enum' missing)
 - #21855  Lotus Word Pro 9.8: Windows pull down does not show file names
 - #27002  Shadow Company: Left for Dead fails with "No usable 3D Devices installed".
 - #32572  Multiple games have no character animation (Alpha Polaris, Face Noir, A Stroke of Fate series)
 - #38142  Approach fields box only show 3/4 of one line
 - #43124  Overwatch loses focus on respawn
 - #44817  Some software protection schemes need ntdll.NtSetLdtEntries to modify reserved LDT entries
 - #49195  Multiple 4k demoscene OpenGL demos crash on startup (failure to lookup atom 0xC019 'static' from global atom tables)
 - #49905  VbsEdit runs wscript.exe with unsupported switches /d and /u
 - #50210  Multiple games need D3DX11GetImageInfoFromMemory implementation (S.T.A.L.K.E.R.: Call of Pripyat, Metro 2033, Project CARS)
 - #53767  vbscript fails to handle ReDim when variable is not yet created
 - #54670  16-bit applications fail in wow64 mode
 - #55151  PC crashes after endlessly eating up memory
 - #57877  CMD: Parsing issue: Mismatch in parentheses provoked by trailing tab
 - #58156  Star Wars: Jedi Knight - Dark Forces II Demo & other games: won't launch, "smackw32.DLL" failed to initialize (macOS)
 - #58204  Winecfg Audio tab doesn't enumerate drivers or show output devices, but test button works.
 - #58458  Wolfenstein: The Old Blood (Wolfenstein: The New Order) fails to start with EGL opengl backend
 - #58480  winebuild ASLR breaks older DLLs
 - #58491  Flickering on video-surveilance-app is back
 - #58515  Street Chaves only displays a black screen
 - #58534  Grand Theft Auto: Vice City intros play with a black screen
 - #58602  Screen Issue in Colin McRae Rally 2
 - #58637  SimCity 2000 Windows 95 edition doesn't launch in WoW64 mode
 - #58651  Legacy of Kain: Blood Omen black screen on startup but with sound using Verok's patch
 - #58666  wine 10.14 fails to build in alpine linux x86
 - #58688  Regression: Xenia Canary crashes with STATUS_CONFLICTING_ADDRESSES when starting a game on Wine 10.13+ (works on 10.12)
 - #58699  Profi cash 12 user interface is rendered mostly black
 - #58700  Regression: Direct3D applications show a blank screen under wined3d in 10.15
 - #58705  Wolfenstein: The New Order (Wolfenstein: Old Blood) - the screen is black
 - #58710  wmic now also prints system properties since 3c8a072b52f2159e68bfd4e471faf284309201ed
 - #58716  Camerabag Pro 2025.2 crashes with unhandled exception (unimplemented function propsys.dll.PropVariantToFileTime) on loading a JPEG
 - #58730  Images in iTunes have a white background (see picture)
 - #58742  winedbg: Internal crash at 00006FFFFF8CB5E5 (pe_load_msc_debug_info)
 - #58744  Missing Type on get_type within dlls/msi/suminfo.c

### Changes since 10.15:
```
Adam Markowski (2):
      po: Update Polish translation.
      po: Update Polish translation.

Alexandre Julliard (43):
      ntdll: Delay first thread initialization until process init.
      ntdll: Use a separate bitmap to keep track of allocated LDT entries.
      ntdll: Use the virtual mutex to protect the LDT data.
      include: Use latest definitions for PEB fields to replace the old Fls fields.
      ntdll: Store the ldt_copy pointer in the PEB and access it from the client side.
      server: Remove the server-side LDT support.
      ntdll: Replace LDT flags by explicit bit fields.
      krnl386: Replace LDT flags by explicit bit fields.
      ntdll: Store LDT limit and bits in the same word.
      krnl386: Fetch the ldt copy pointer from the PEB.
      winebuild: Fetch the ldt copy pointer from the PEB.
      ntdll: Allocate the LDT copy only when needed.
      ntdll: Handle return length from ThreadDescriptorTableEntry in the common code.
      ntdll: Support NtQueryInformationThread(ThreadDescriptorTableEntry) in new wow64 mode.
      ntdll: Support setting LDT entries in new wow64 mode.
      ntdll: Keep the DOS area clear in new wow64 mode.
      tiff: Import upstream release 4.7.1.
      ntdll: Restore the macOS RLIMIT_NOFILE workaround.
      ntdll/tests: Always restore the APC dispatcher.
      ntdll: Clear alignment flag on signal entry also on x86-64.
      ntdll: Consistently output one loaddll trace per module.
      ntdll: Only return 32-bit PEB for 32-bit process.
      win32u/tests: Avoid reading uninitialized data.
      wow64cpu: Store the actual segment registers in the wow64 context.
      wow64cpu: Store the 32-bit segment registers in the context when entering 32-bit mode.
      wow64cpu: Store the 32-bit segment registers in the context on syscalls.
      ntdll: Store the actual segment registers in the wow64 context.
      ntdll: Trace some segment registers in 64-bit mode.
      makedep: Add rules to build winmd files.
      wow64cpu: Store the 32-bit segment registers in the context also on Unix calls.
      wow64: Support exceptions happening in 16-bit mode.
      ntdll: Support exceptions happening in 16-bit mode.
      shell: Fix crash in ShellAbout16() when no icon is specified.
      makedep: Don't add a trailing slash to installation directories.
      makedep: Add a helper to install a data file.
      makedep: Add a helper to install a data file from the source dir.
      makedep: Add a helper to install a header file.
      makedep: Add a helper to install a program.
      makedep: Add a helper to install a script.
      makedep: Add a helper to install a tool.
      makedep: Add a helper to install a symlink.
      makedep: Build the install destination string in the common helper.
      makefiles: Install winmd files.

Alistair Leslie-Hughes (5):
      comdlg32: Correct show parameter passed to ShowWindow.
      cryptui: Correct show parameter passed to ShowWindow.
      wordpad: Correct show parameter passed to ShowWindow.
      include: Add TABLET_DISABLE_PRESSANDHOLD define.
      include: Add TOUCH_COORD_TO_PIXEL define.

Anton Baskanov (10):
      dmsynth/tests: Check IDirectMusicSynth::Open() default parameter values.
      dmsynth/tests: Actually set parameters to zero before calling IDirectMusicSynth::Open().
      dmsynth: Remove special handling of zero parameter values in IDirectMusicSynth::Open().
      dmsynth: Don't force enable DMUS_EFFECT_REVERB.
      winecfg: Allow configuring default MIDI device.
      dmsynth/tests: Add tests for GUID_DMUS_PROP_Volume.
      dmsynth: Set gain to 6 dB.
      dmsynth: Handle GUID_DMUS_PROP_Volume.
      fluidsynth: Disable IIR filter resonance hump compensation.
      dmsynth: Pre-attenuate voices by center pan attenuation.

Aurimas Fišeras (1):
      po: Update Lithuanian translation.

Bernhard Kölbl (2):
      make_unicode: Add some halfwidth mapping exceptions.
      mf: Handle start request when session is already running.

Billy Laws (4):
      ntdll/tests: Skip broken process suspend test under ARM64 WOW64.
      ntdll/tests: Test THREAD_CREATE_FLAGS_SKIP_LOADER_INIT flag.
      ntdll: Add a default pBaseThreadInitThunk implementation.
      ntdll: Support THREAD_CREATE_FLAGS_SKIP_LOADER_INIT flag.

Christian Costa (1):
      d3dx9: Add an initial implementation of UpdateSkinnedMesh().

Connor McAdams (15):
      d3dx9/tests: Add tests for D3DFMT_CxV8U8.
      d3dx9: Add support for D3DFMT_CxV8U8.
      d3dx9: Replace D3DFMT_CxV8U8 with D3DFMT_X8L8V8U8 when creating textures.
      d3dx9/tests: Add tests for ATI{1,2} DDS files.
      d3dx9/tests: Add a test for DDS_PF_FOURCC flag handling.
      d3dx9: Ignore all other DDS pixel format flags if DDS_PF_FOURCC is set.
      d3dx10/tests: Add more DDS pixel format tests.
      d3dx11/tests: Add more DDS pixel format tests.
      d3dx9: Add a tweak to stb_dxt to more closely match native output.
      d3dx10: Use shared code in load_texture_data() when possible.
      d3dx10: Ignore alpha channel values for WICPixelFormat32bppBGR BMP images.
      d3dx10: Add support for decompressing BC4 and BC5 formats.
      d3dx10: Add support for compressing BC4 and BC5 formats.
      d3dx10: Exclusively use shared code to load DDS files in load_texture_data().
      d3dx10: Exclusively use shared code in load_texture_data().

David Kahurani (4):
      xmllite/writer: Implement WriteNmToken().
      xmllite/writer: Implement WriteQualifiedName().
      xmllite/writer: Implement WriteEntityRef().
      xmllite/writer: Implement WriteName().

Dmitry Kislyuk (1):
      wscript: Ignore /d and /u.

Dmitry Timoshkov (15):
      winex11.drv: Remove a no longer valid assert().
      ntdll/tests: Add some tests for NtSetLdtEntries.
      comdlg32: Properly translate Flags and PageRanges to PRINTDLGEX.
      runas: Add initial implementation.
      ws2_32/tests: Add a simple test for WSAProviderConfigChange().
      ws2_32: Return a socket from WSAProviderConfigChange() stub.
      ole32/tests: Add a test for IPropertySetStorage::Open() with STGM_TRANSACTED flag.
      ole32: Ignore STGM_TRANSACTED in IPropertySetStorage::Open().
      ole32: Print correct entry name in a trace when reading storage dictionary.
      ole32: Write padding to a dictionary entry only when necessary.
      ole32: Clear dirty flag in IPropertyStorage::Commit().
      gdiplus: Handle PixelFormatPAlpha separately in alpha_blend_hdc_pixels().
      secur32: Add QuerySecurityContextToken() stub implementation to the LSA wrapper.
      d2d1: Implement D2D1CreateDeviceContext().
      d2d1: Add a test for D2D1CreateDeviceContext().

Elizabeth Figura (23):
      user32/tests: Test MDI menu updating when window titles are changed.
      user32: Always refresh the MDI menu on WM_MDISETMENU.
      setupapi/tests: Test SetupGetBinaryField().
      setupapi: Parse a 0x prefix in SetupGetBinaryField().
      ntdll: Check inproc sync signal rights in signal and wait.
      wined3d: Use malloc() for the private store.
      user32/tests: Test integral resizing of listboxes.
      comctl32/tests: Test integral resizing of listboxes.
      comctl32/listbox: Ignore the horizontal scrollbar when setting integral height.
      ntdll: Introduce a helper to wait on a server-side sync object.
      ddraw/tests: Use ANSI versions of user32 functions.
      winebth.sys: Silently pass down IRP_MN_QUERY_ID to the PDO.
      winebus.sys: Silently pass down IRP_MN_QUERY_ID to the PDO.
      wineusb.sys: Silently pass down IRP_MN_QUERY_ID to the PDO.
      ntoskrnl.exe: Create keys for devices in HKEY_DYN_DATA\Config Manager\Enum.
      ddraw/tests: Do not validate that vidmem D32 is not supported.
      ddraw/tests: Fix test_caps() on Windows 98.
      ddraw/tests: Test dwZBufferBitDepths.
      ddraw: Fill dwZBufferBitDepths.
      server: Create an inproc sync for user APC signaling.
      ntdll: Validate expected inproc sync type in get_inproc_sync.
      ntdll: Use in-process synchronization objects.
      ntdll: Cache in-process synchronization objects.

Eric Pouech (22):
      certutil: Implement -decodehex command.
      cmd: Generate binary files with certutil.
      cmd: Only set console's default color when provided.
      cmd: Fix some initial env variables setup.
      cmd: Fix context detection in cmd /c.
      cmd: Don't use page mode for stderr.
      cmd: Introduce helper to push/pop i/o handles for redirections.
      cmd: Ensure that all output to STD_OUTPUT go through WCMD_output_asis.
      cmd: Let WCMD_fgets() work properly when reading from a pipe.
      cmd: Use a global input handle for console.
      cmd: Let WCMD_ask_confirm Use WCMD_fgets().
      cmd: Let WCMD_setshow_date() use WCMD_fgets().
      cmd: Let WCMD_setshow_env() use WCMD_fgets().
      cmd: Let WCMD_label() use WCMD_fgets().
      cmd: Add tests showing that MORE outputs to CONOUT$ not stdout.
      cmd: Clean up and enhance MORE command implementation.
      cmd: Let WCMD_setshow_time() use WCMD_fgets().
      cmd: Let WCMD_wait_for_input() no longer use WCMD_ReadFile().
      cmd: Get rid of WCMD_ReadFile.
      winedbg: Simplify fetching module name.
      dbghelp: Add public symbols out of export table when no debug info is present.
      dbghelp: Don't crash on stripped image without DEBUG directories.

Francis De Brabandere (1):
      vbscript: Allow redim without a prior dim.

Giovanni Mascellani (15):
      mmdevapi/tests: Remove a flaky test.
      mmdevapi/tests: Introduce a helper to read many packets when capturing.
      mmdevapi/tests: Check that GetBuffer() fails when no packet is available.
      mmdevapi/tests: Check that GetBuffer() returns a packet of the expected size.
      mmdevapi/tests: Check that the received packet isn't larger than the padding.
      mmdevapi/tests: Test releasing a buffer without consuming it and getting it again.
      mmdevapi/tests: Check that releasing a buffer of the wrong size fails.
      mmdevapi/tests: Test acquiring and releasing the buffer out of order.
      mmdevapi/tests: Check that captured packets are consecutive.
      mmdevapi/tests: Test calling GetNextPacketSize() after GetBuffer().
      mmdevapi/tests: Replace a few tests with read_packets().
      mmdevapi/tests: Check discontinuities after having started capturing.
      mmdevapi/tests: Check that the capture buffer is empty after processing packets.
      mmdevapi/tests: Sleep for 600 ms to guarantee a buffer overrun.
      mmdevapi/tests: Remove a wrong test about packet sizes.

Hans Leidekker (8):
      wbemprox: Support WBEM_FLAG_NONSYSTEM_ONLY in class_object_Next().
      wmic: Only list non-system properties.
      bcrypt: Add support for named curves.
      msi: Support PID_EDITTIME in MsiSummaryInfoSetProperty().
      wbemprox: Implement Win32_LocalTime.
      widl: Add metadata support for imported types.
      widl: Store name and namespace string index.
      widl: Fix order of exclusiveto attribute.

Jacek Caban (2):
      winebuild: Use .rdata section instead of .rodata on PE targets.
      opengl32: Propagate GL errors from wow64 wrappers.

Jiangyi Chen (2):
      ole32/tests: Add tests for StgOpenStorageOnILockBytes().
      ole32: Fix the return value for StgOpenStorageOnILockBytes().

Jinoh Kang (10):
      fluidsynth: Fix g_atomic_int_add() return value.
      fluidsynth: Fix data race in g_get_monotonic_time().
      fluidsynth: Fix g_mutex_init() and g_cond_init().
      fluidsynth: Fix double close of thread handle in g_thread_unref().
      fluidsynth: Use full memory barrier in g_atomic_int_get().
      fluidsynth: Round up sleep duration in g_usleep().
      fluidsynth: Fix argument flag handling in g_file_test().
      fluidsynth: Fix definition of g_atomic_int_dec_and_test().
      fluidsynth: Return thread return value from g_thread_join().
      fluidsynth: Use InterlockedExchangeAdd() in g_atomic_int_add().

Joe Souza (5):
      cmd/tests: Add test to check for TYPE truncation in binary mode.
      cmd: Refactor WCMD_copy_loop out of WCMD_ManualCopy, and stop copy loop at EOF for /a mode.
      cmd: Fix TYPE behavior (now uses WCMD_copy_loop).
      cmd/tests: Test that DIR /Oxxx at the command line overrides DIRCMD=/Oyyy set in the environment.
      cmd: Allow DIR /Oxxx at the command line to override DIRCMD=/Oyyy set in the environment.

Lauri Kenttä (1):
      po: Update Finnish translation.

Louis Lenders (4):
      wmic: Add "qfe" alias.
      wbemprox: Add two hotfixid's for Windows 7 to Win32_QuickFixEngineering.
      wmic: Add a basic help option "/?".
      shell32: Add explicit ordinal for SHMultiFileProperties.

Maotong Zhang (1):
      ole32/tests: Add some tests for "CurVer" handling.

Michael Müller (1):
      user32/listbox: Ignore the horizontal scrollbar when setting integral height.

Nikolay Sivov (65):
      dwrite: Always initialize 'contours' flag.
      dwrite/layout: Always initialize effective run bounding box.
      dwrite/layout: Do not shadow output parameter.
      dwrite/opentype: Use mask shifts only for non-zero masks.
      win32u/tests: Fix missing test message context pop.
      dwrite/tests: Add some more tests for ConvertFontToLOGFONT().
      dwrite: Check against local file loader in ConvertFontToLOGFONT().
      dwrite: Remove system collection marker.
      dwrite: Mark system font sets.
      dwrite: Create both WWS and typographic system collections using system font set.
      dwrite: Create custom collections using font sets.
      dwrite: Simplify collection initialization helper.
      dwrite: Remove nested structures in fontset entries.
      dwrite: Cache set elements for returned system sets.
      dwrite/tests: Add a small test for EUDC collection.
      dwrite: Reuse font set entries to return set instances for collections.
      xmllite/tests: Add some more tests for WriteNmToken().
      mf: Implement MFTranscodeGetAudioOutputAvailableTypes().
      dwrite/tests: Add more tests for CreateFontFileReference().
      dwrite: Use uppercase paths for local file loader keys.
      dwrite: Fail file reference creation when timestamp is inaccessible.
      dwrite/tests: Add another CreateFontFaceReference() test with inaccessible file.
      xmllite/tests: Add some more implicit flushing tests.
      xmllite/write: Improve error handling in WriteCharEntity().
      xmllite/writer: Improve error handling in WriteNmToken().
      xmllite/writer: Improve error handling in WriteWhitespace().
      xmllite/writer: Improve error handling in WriteSurrogateCharEntity().
      xmllite/writer: Improve error handling in WriteStartDocument().
      xmllite/writer: Improve error handling in WriteCData().
      xmllite/writer: Improve error handling in WriteComment().
      xmllite/writer: Improve error handling in WriteProcessingInstruction().
      xmllite/writer: Remove duplicate check for whitespaces.
      xmllite/writer: Improve error handling WriteDocType().
      xmllite/writer: Improve error handling in WriteEndElement().
      xmllite/writer: Improve error handling in WriteEndDocument().
      propsys: Add PropVariantToFileTime() semi-stub.
      propsys: Add PropVariantToUInt32Vector() semi-stub.
      xmllite/writer: Improve error handling in WriteChars().
      xmllite/writer: Improve error handling in WriteString().
      xmllite/writer: Improve error handling in WriteFullEndElement().
      xmllite/writer: Improve error handling in WriteElementString().
      xmllite/writer: Improve error handling in WriteRaw().
      xmllite/tests: Add a few more tests for WriteDocType().
      xmllite/writer: Improve error handling in WriteAttributeString().
      xmllite/writer: Improve error handling when writing namespace defitions.
      xmllite/writer: Improve error handling in WriteStartElement().
      xmllite/writer: Handle empty names in WriteStartElement().
      xmllite/tests: Add more tests for name validation when writing.
      xmllite/writer: Improve NCName validation.
      xmllite/writer: Remove now unnecessary helper.
      xmllite/writer: Output element stack on release.
      xmllite/writer: Output element stack on SetOutput().
      windowscodecs: Remove IWICWineDecoder interface.
      odbc32: Implement SQLErrorW() for ANSI win32 drivers.
      odbc32: Add some fixmes for SQLError() on top of driver's SQLGetDiagRec().
      odbc32: Implement SQLDriverConnectW() for ANSI win32 drivers.
      odbc32: Implement SQLGetInfoW(SQL_ODBC_API_CONFORMANCE) for ANSI win32 drivers.
      odbc32: Implement SQLGetInfoW(SQL_ACTIVE_STATEMENTS) for ANSI win32 drivers.
      odbc32: Implement SQLGetInfoW(SQL_ACTIVE_CONNECTIONS) for ANSI win32 drivers.
      odbc32: Implement SQLGetInfoW(SQL_DRIVER_NAME) for ANSI win32 drivers.
      odbc32: Implement SQLGetInfoW(SQL_TXN_CAPABLE) for ANSI win32 drivers.
      odbc32: Implement SQLGetInfoW(SQL_DBMS_NAME) for ANSI win32 drivers.
      odbc32: Implement SQLGetInfoW(SQL_DATA_SOURCE_READ_ONLY) for ANSI win32 drivers.
      odbc32: Implement SQLGetInfoW(SQL_IDENTIFIER_QUOTE_CHAR) for ANSI win32 drivers.
      odbc32: Implement SQLExecDirectW() for ANSI win32 drivers.

Patrick Hibbs (1):
      inf: Add Windows Media Player related registry keys.

Paul Gofman (24):
      iphlpapi/tests: Add more tests for GetBestRoute().
      nsi/tests: Add test for ipv4 loopback routes presence.
      nsiproxy.sys: Explicitly add loopback entries to ipv4 forward table on Linux.
      nsi: Match struct nsi_tcp_conn_dynamic size to up to date Win11.
      nsiproxy.sys: Only enumerate active routes in ipv6_forward_enumerate_all() on Linux.
      nsiproxy.sys: Improve loopback detection in ipv6_forward_enumerate_all() on Linux.
      iphlpapi: Fully zero init address in unicast_row_fill().
      iphlpapi: Implement GetBestRoute2().
      iphlpapi: Reimplement GetBestInterfaceEx() on top of GetBestRoute2().
      iphlpapi/tests: Add tests for best routes.
      iphlpapi: Try to disambiguate addresses in GetBestRoute2() by probing system assigned ones.
      user32/tests: Add test for CB size after setting font.
      user32/combo: Don't update item height on WM_SETFONT for owner drawn CB.
      comctl32/tests: Also call test_combo_setfont() with CBS_OWNERDRAWFIXED.
      comctl32/combo: Don't update item height on WM_SETFONT for owner drawn CB.
      ntdll: Initialize segments registers in the frame in call_user_mode_callback().
      ntdll: Handle invalid FP state in usr1_handler() on x86-64.
      ntdll: Handle invalid FP state in usr1_handler() on i386.
      win32u: Implement NtUserDisplayConfigGetDeviceInfo( DISPLAYCONFIG_DEVICE_INFO_GET_ADAPTER_NAME ).
      user32/tests: Add test for (no) messages during TrackMouseEvent() call.
      win32u: Use internal message to handle NtUserTrackMouseEvent() for other thread window.
      win32u: Move mouse tracking info into per-thread data.
      win32u: Track mouse events based on last mouse message data.
      user32/tests: Don't leak thread handle in test_TrackMouseEvent().

Ratchanan Srirattanamet (1):
      msi: Fix MsiEnumFeatures[AW]() by make it look at the correct registry.

Reinhold Gschweicher (1):
      msxml3/tests: Add test for IXMLDOMElement_removeAttributeNode.

Rémi Bernon (68):
      server: Create a shared object for window classes.
      server: Write class name to the shared memory object.
      win32u: Use NtUserGetClass(Long|Name)W in needs_ime_window.
      server: Keep a class object locator in the window shared object.
      win32u: Read class name from the shared memory object.
      win32u: Validate the drawable surface window before reusing.
      wineandroid: Keep the client ANativeWindow with the window data.
      wineandroid: Use detach_client_surfaces to invalidate drawables.
      winex11: Always notify the surface that it has been presented.
      server: Use a specific type for internal inproc event syncs.
      ntdll: Remove workaround for macOS RLIMIT_NOFILE.
      wineserver: Request RLIMIT_NOFILE maximum allowed value.
      server: Rename queue is_signaled to get_queue_status.
      server: Do not clear queue masks in msg_queue_satisfied.
      server: Remove skip_wait flag from set_queue_mask.
      win32u: Use a specific opengl_drawable function for context changes.
      win32u: Avoid swapping / flushing drawables with destroyed windows.
      server: Return early if there's no queue in queue requests.
      server: Move hooks_count to the end of the queue_shm_t struct.
      win32u: Move the message queue access time to shared memory.
      server: Use monotonic_time as queue access time base.
      win32u: Check the queue access time from the shared memory.
      winex11: Avoid presenting invalid offscreen window rects.
      win32u/tests: Test that global d3dkmt handles aren't leaked.
      win32u: Use array indexes for d3dkmt handles.
      server: Create server-side global D3DKMT objects.
      server: Allocate global D3DKMT handles for objects.
      win32u: Implement creation of D3DKMT sync objects.
      win32u: Implement creation of D3DKMT resource objects.
      win32u: Pass D3DKMT object runtime data to wineserver.
      win32u: Implement NtGdiDdDDIQueryResourceInfo.
      server: Update queue access time in set_queue_mask request.
      win32u: Check the queue access time before skipping set_queue_mask.
      server: Use the access time and signal state to detect hung queue.
      server: Remove the now unnecessary queue waiting flag.
      cfgmgr32: Call devprop_filters_validate recursively.
      cfgmgr32: Pass filter ranges to helper functions.
      win32u: Implement opening of D3DKMT global handles.
      win32u: Implement NT sharing of D3DKMT sync objects.
      win32u: Implement sharing of D3DKMT resource objects.
      win32u: Implement querying D3DKMT objects from shared handles.
      win32u: Implement opening D3DKMT objects from shared handles.
      win32u: Implement opening D3DKMT shared handles from names.
      server: Introduce an internal queue bits field.
      server: Use the internal bits to signal the queue sync.
      win32u: Process all driver events when waiting on queue.
      win32u: Wrap ProcessEvents calls in process_driver_events helper.
      win32u: Avoid a crash when unsetting current context.
      server: Use a dedicated internal bit for queued hardware messages.
      win32u: Check for pending hardware messages after processing events.
      win32u: Return TRUE from ProcessEvents after emptying the event queue.
      win32u: Notify wineserver after processing every driver event.
      server: Continuously poll on queue fd for driver events.
      server: Remove mostly unnecessary thread own queue check.
      server: Use an internal event sync for message queues.
      win32u/tests: Test creating shared resources with D3D9Ex.
      win32u/tests: Test importing shared resources into D3D9Ex.
      win32u/tests: Test importing shared resources into OpenGL.
      win32u/tests: Test creating shared resources with D3D10.
      win32u/tests: Test creating shared resources with D3D11.
      win32u/tests: Test creating shared resources with D3D12.
      win32u/tests: Test creating shared resources with Vulkan.
      win32u/tests: Test importing shared resources into Vulkan.
      server: Use a separate helper to create internal event syncs.
      server: Explicitly create an internal server sync for debug events.
      server: Signal event server / inproc syncs using the signal op.
      server: Use struct object pointers for object syncs.
      ntdll: Receive the user apc inproc sync fd on alertable waits.

Sebastian Lackner (1):
      user32: Refresh MDI menus when DefMDIChildProc(WM_SETTEXT) is called.

Stian Low (2):
      ntdll: Implement NtSetEventBoostPriority().
      ntdll/tests: Add tests for NtSetEventBoostPriority().

Tim Clem (1):
      wbemprox: Use setupapi to enumerate video controllers.

Vasiliy Stelmachenok (1):
      win32u: Handle errors when creating EGL context.

Vibhav Pant (9):
      cfgmgr32: Always check the DEVPROP_OPERATOR_EQUALS mask while evaluating comparison filters.
      propsys: Use VT_LPWSTR as the property type for System.Devices.DeviceInstanceId.
      windows.devices.enumeration/tests: Add tests for IDeviceInformationStatics::{FindAllAsyncAqsFilter, CreateWatcherAqsFilter}.
      windows.devices.enumeration: Support parsing AQS filters in IDeviceInformationStatics::{FindAllAsyncAqsFilter, CreateWatcherAqsFilter}.
      maintainers: Add a section for Windows.Devices.Enumeration.
      windows.devices.enumeration: Ensure that all AQS logical operators bind left to right.
      cfgmgr32/tests: Add additional tests for device query filters.
      cfgmgr32: Implement support for logical operators in DEVPROP_FILTER_EXPRESSION.
      vccorlib140: Emit RTTI for Platform::Type.

Yuxuan Shui (1):
      mf: Add a SUBMITTED command state to avoid multiple submission of the same op.

Zhengyong Chen (1):
      imm32: Ensure HIMC is unlocked in ImmGenerateMessage.
```
