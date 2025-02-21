The Wine development release 10.2 is now available.

What's new in this release:
  - Bundled vkd3d upgraded to version 1.15.
  - Support for setting thread priorities.
  - New Wow64 mode can be enabled dynamically.
  - More progress on the Bluetooth driver.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/10.x/wine-10.2.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-10.2/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 10.2 (total 20):

 - #25872  Guild Wars 'test system' doesn't show test results
 - #39474  MSWT Kart 2004 does not work
 - #50152  YOU and ME and HER: Game crashes after launching from game launcher
 - #56260  16-bit Myst deadlocks when entering Book
 - #56464  vbscript: Join() builtin is not implemented
 - #56951  psql 16: unrecognized win32 error code: 127 invalid binary: Invalid argument / could not find a "psql.exe" to execute
 - #57310  wineboot failed to initialize a wine prefix
 - #57625  Regression: some fullscreen games are displayed incorrectly upon switching from and back to game window
 - #57787  Final Fantasy XI Online crashes with unhandled page fault on launch
 - #57793  Wine Wordpad started with blank screen
 - #57795  cmd lacks support for right-to-left handle redirection (2<&1)
 - #57803  Sekiro: Shadows Die Twice crashes at launch with Xbox One controller connected
 - #57804  cmd: '@echo off' gets propagated to parent on CALL instruction (CALL seems to freezes)
 - #57809  cmd: Incorrect substring expansion of last character (e.g. `!MY_STR:~1!`)
 - #57810  ITextStream::WriteBlankLines() needs to be implemented for libxml2's configure.js to work
 - #57817  mvscp90 ::std::ifstream::seekg(0) crashes
 - #57819  Wine unable to start when built with binutils 2.44
 - #57824  SetThreadPriority unexpectedly fails on terminated threads instead of no-op
 - #57834  Cyberpunk 2077 doesn't load with CyberEngine Tweaks
 - #57847  Cross-compiled Wine no longer installs a 'wine' loader binary

### Changes since 10.1:
```
Adam Markowski (1):
      po: Update Polish translation.

Aida Jonikienė (1):
      wined3d: Add DXT format mappings in the Vulkan renderer.

Alex Henrie (6):
      urlmon/tests: Use wide character string literals.
      advapi32/tests: Use wide character string literals.
      setupapi: Use wide character string literals.
      msxml3: Use wide character string literals.
      krnl386: Add a WOWCallback16Ex flag to simulate a hardware interrupt.
      mmsystem: Simulate a hardware interrupt in MMSYSTDRV_Callback3216.

Alexandre Julliard (31):
      user32: Set last error on failed window enumeration.
      server: Fix desktop access check for window enumeration.
      ntdll: Add a helper function to retrieve the .so directory.
      ntdll: Add a helper function to get the alternate 32/64-bit machine.
      ntdll: Set the WINELOADER variable on the PE side, using a Windows path.
      loader: Install the Wine loader in the Unix lib directory.
      tools: Add a simpler Wine launcher in the bin directory.
      tools: Move the loader man pages to the new Wine loader directory.
      dbghelp: Always use the WINELOADER variable for the loader name.
      ntdll: Disable file system redirection until the initial exe is loaded.
      ntdll: Look for a builtin exe corresponding to the loader basename.
      makefiles: Install binaries as symlinks to wine.
      ntdll: Allow forcing new wow64 mode by setting WINEARCH=wow64.
      ntdll: Move setting DYLD_LIBRARY_PATH on macOS to the loader.
      makefiles: Create wine as a symlink to tools/wine/wine.
      makefiles: Link to the wine loader in the tools directory if it was built.
      makefiles: Generate the wow64 symlinks from makedep.
      makefiles: Generate more of the re-configure rules from makedep.
      make_unicode: Update timezone data source to version 2025a.
      makefiles: Don't clean files in other directories.
      png: Import upstream release 1.6.45.
      lcms2: Import upstream release 2.17.
      faudio: Import upstream release 25.02.
      rsaenh: Use RtlGenRandom() directly.
      rsaenh: Don't use printf.
      vkd3d: Import upstream release 1.15.
      lcms2: Allow creating a transform without specifying input/output formats.
      server: Support an arbitrary number of PE sections.
      ntdll: Support an arbitrary number of PE sections.
      vkd3d: Update version number.
      makefiles: Fix the distclean target to clean everything.

Alexandros Frantzis (1):
      winewayland: Don't crash on text input done events without focus.

Anton Baskanov (1):
      winedbg: Make crash dialog topmost.

Attila Fidan (4):
      winewayland: Enable/disable the zwp_text_input_v3 object.
      winewayland: Post IME update for committed text.
      winewayland: Implement SetIMECompositionRect.
      winewayland: Post IME update for preedit text.

Austin English (1):
      mshtml: Add registry association for .log files.

Bernhard Übelacker (3):
      wbemprox: Increase buffer size one iteration earlier (ASan).
      ntdll/tests: Skip leap secondInformation test with older Windows versions.
      msvcrt/tests: Add missing parameter pmode to _open call (ASan).

Biswapriyo Nath (3):
      include: Add UI Automation Style ID definitions.
      include: Add IWiaDevMgr2 definition in wia_lh.idl.
      include: Add missing structures for wiadef.h in wia_lh.idl.

Brendan McGrath (3):
      winedmo: Explicitly set 'Data1' value for VP9 GUID.
      mf/tests: Add tests using a new WMV decoder MFT.
      winegstreamer: Store and use previous WMV stride value.

Brendan Shanks (2):
      configure: MacOS is always darwin*.
      winecoreaudio: Avoid duplicate Program Change or Channel Aftertouch MIDI messages.

Connor McAdams (2):
      quartz/tests: Add a test for resetting stream position after receiving EC_COMPLETE.
      quartz/filtergraph: Allow changing stream position after receiving EC_COMPLETE.

Conor McCarthy (16):
      mfplat/tests: Test putting work on an undefined queue id.
      rtworkq/tests: Test putting work on an undefined queue id.
      rtworkq: Support putting work on RTWQ_CALLBACK_QUEUE_UNDEFINED.
      rtworkq: Support putting work on any id in RTWQ_CALLBACK_QUEUE_PRIVATE_MASK.
      mfplat/tests: Introduce a helper to check the platform lock count.
      mfplat/tests: Test mixing of MF platform functions with their Rtwq equivalents.
      mfplat/tests: Test platform startup and lock counts.
      rtworkq/tests: Test work queue.
      rtworkq: Introduce a platform startup count.
      rtworkq: Introduce an async result object cache.
      mfplat: Free the inner cookie passed to resolver_create_cancel_object().
      mfplat/tests: Release callback2 in test_event_queue().
      mfplat/tests: Check for async result release after canceling work and shutting down.
      rtworkq/tests: Test scheduled items.
      rtworkq/tests: Test queue shutdown with pending items.
      rtworkq: Do not cancel pending callbacks when closing a thread pool.

Damjan Jovanovic (2):
      scrrun: Implement ITextStream::WriteBlankLines().
      cmd: Add support for right-to-left handle redirection (2<&1).

Dmitry Timoshkov (17):
      windowscodecs/tests: Add some tests for IWICBitmapFlipRotator.
      windowscodecs: Implement IWICBitmapFlipRotator(WICBitmapTransformFlipHorizontal) for bitmaps with bpp >= 8.
      windowscodecs: Implement IWICBitmapFlipRotator(WICBitmapTransformRotate90) for bitmaps with bpp >= 8.
      windowscodecs: Simplify a bit FlipRotator_CopyPixels() implementation.
      wldap32: Enable libldap tracing with +wldap32 channel.
      wldap32: Don't use win32 constant when calling into libldap.
      oleaut32: Fix logic for deciding whether type description follows the name.
      oleaut32: 'typekind' is the last field of SLTG_OtherTypeInfo structure.
      oleaut32: Implement decoding of SLTG help strings.
      oleaut32: Add support for decoding SLTG function help strings.
      oleaut32: Add support for decoding SLTG variable help strings.
      oleaut32: Make OleLoadPicture load DIBs using WIC decoder.
      server: Add support for merging WM_MOUSEWHEEL messages.
      oleaut32: Add missing fixup for SLTG typename offset.
      oleaut32: Avoid double initialization of TypeInfoCount when reading SLTG typelib.
      gdiplus: Add support for more image color formats.
      gdiplus/tests: Add some tests for loading TIFF images in various color formats.

Elizabeth Figura (9):
      shdocvw: Remove ParseURLFromOutsideSource() implementation.
      shdocvw: Remove URLSubRegQueryA() implementation.
      wined3d: Propagate a CLEARED location when blitting.
      wined3d: Explicitly check for BUFFER/SYSMEM before calling wined3d_texture_download_from_texture().
      wined3d: Introduce initial support for Direct3D 10+ 2-plane formats.
      wined3d: Forbid creating unaligned planar textures.
      wined3d: Forbid unaligned blits for planar textures.
      wined3d: Implement creating views of planar resources.
      d3d11: Implement D3D11_FEATURE_D3D9_SIMPLE_INSTANCING_SUPPORT.

Eric Pouech (4):
      cmd/tests: Add tests about echo mode persistence across batch calls.
      cmd: Preserve echo mode across interactive batch invocation.
      cmd/tests: Add tests wrt. variable expansion with substring modifition.
      cmd: Fix substring substitution in variable expansion.

Esme Povirk (4):
      comctl32: Test for an IAccessible interface on SysLink controls.
      comctl32: Further testing of SysLink IAccessible.
      comctl32: Stub IAccessible interface for SysLink controls.
      comctl32: Implement accRole for SysLink controls.

Francis De Brabandere (1):
      vbscript: Add Join implementation.

Gabriel Ivăncescu (2):
      mshtml: Clear the documents list when detaching inner window.
      jscript: Use as_jsdisp where object is known to be a jsdisp.

Gijs Vermeulen (1):
      winegstreamer: Implement IWMSyncReader2::GetMaxOutputSampleSize.

Hans Leidekker (1):
      sort: Support /+n option.

Jacek Caban (11):
      ntdll: Pass WINE_MODREF to import_dll.
      ntdll: Pass importer pointer to find_ordinal_export.
      ntdll: Pass importer pointer to find_named_export.
      ntdll: Pass importer pointer to find_forwarded_export.
      ntdll: Pass importer pointer to build_import_name.
      ntdll: Use NULL importer in LdrGetProcedureAddress.
      ntdll: Remove no longer needed current_modref.
      msvcp: Avoid explicitly aligning structs passed by value.
      msvcp60: Avoid explicitly aligning structs passed by value.
      ntdll: Use signed type for IAT offset in LdrResolveDelayLoadedAPI.
      winebuild: Avoid using .idata section for delay-load import libraries.

Jinoh Kang (11):
      server: Don't fail SetThreadPriority() on terminated threads.
      kernel32/tests: Add a basic FlushProcessWriteBuffers stress test.
      kernel32/tests: Add a store buffering litmus test involving FlushProcessWriteBuffers.
      include: Fix ReadNoFence64 on i386.
      server: Fix incorrect usage of __WINE_ATOMIC_STORE_RELEASE in SHARED_WRITE_BEGIN/SHARED_WRITE_END.
      include: Prevent misuse of __WINE_ATOMIC_* helper macros for non-atomic large accesses.
      kernel32/tests: Add basic tests for internal flags of modules loaded with DONT_RESOLVE_DLL_REFERENCES.
      kernel32/tests: Test for unexpected LDR_PROCESS_ATTACHED flag in import dependency loaded with DONT_RESOLVE_DLL_REFERENCES.
      kernel32/tests: Test for unexpected loader notification for import dependency loaded with DONT_RESOLVE_DLL_REFERENCES.
      ntdll: Skip DLL initialization and ldr notification entirely if DONT_RESOLVE_DLL_REFERENCES is set.
      ntdll: Remove redundant LDR_DONT_RESOLVE_REFS checks before calling process_attach().

Joe Souza (3):
      cmd/tests: Add tests for command DIR /O.
      cmd: Cleanup DIR /O logic.
      cmd: Implement ability to abort lengthy directory operations via Ctrl-C.

Marc-Aurel Zent (5):
      server: Also set thread priorities upon process priority change.
      kernel32/tests: Setting process priority on a terminated process should succeed.
      server: Implement apply_thread_priority on macOS for application priorities.
      server: Implement apply_thread_priority on macOS for realtime priorities.
      server: Apply Mach thread priorities after process tracing is initialized.

Martin Storsjö (2):
      ntdll: Allow running arm/aarch64 in (old) wow64 mode.
      server: Include ARMNT as one of the supported architectures on aarch64.

Michele Dionisio (2):
      timeout: Add a timeout command.
      timeout/tests: Add minimal test suite.

Mohamad Al-Jaf (14):
      windows.web/tests: Remove superfluous cast.
      windows.web/tests: Add IJsonValue::get_ValueType() tests.
      windows.web: Implement IJsonValue::get_ValueType().
      windows.web: Partially implement IJsonValueStatics::Parse().
      windows.web: Add error handling in IJsonValue::GetArray().
      windows.web: Add error handling in IJsonValue::GetObject().
      windows.web: Implement IJsonValue::GetString().
      windows.web: Implement IJsonValue::GetNumber().
      windows.web: Implement IJsonValue::GetBoolean().
      windows.web/tests: Add IJsonValueStatics::Parse() tests.
      include: Reorder windows.storage.stream classes.
      windows.storage: Add stub dll.
      windows.storage: Move RandomAccessStreamReference class from wintypes.
      windows.storage: Add error handling in IRandomAccessStreamReferenceStatics::CreateFromStream().

Nikolay Sivov (41):
      windowscodecs: Implement CreateMetadataWriter().
      windowscodecs/metadata: Create nested writer instances when loading IFD data.
      windowscodecs/metadata: Add a stub for WICApp1MetadataWriter.
      windowscodecs: Filter options in CreateMetadataWriter().
      windowscodecs/metadata: Add initial implementation of CreateMetadataWriterFromReader().
      windowscodecs/tests: Add an item enumeration test for the App1 reader.
      windowscodecs/metadata: Implement SetValue().
      maintainers: Add myself to the scrrun.dll section.
      wbemprox: Fix a memory leak while filling Win32_PnPEntity fields.
      windowscodecs/metadata: Recursively create nested writers in CreateMetadataWriterFromReader().
      windowscodecs/tests: Use generic block writer for testing.
      windowscodecs/tests: Add some tests for querying nested readers in App1 format.
      windowscodecs/tests: Add some tests for the query writer used on App1 data.
      windowscodecs/metadata: Handle nested metadata handlers lookup by CLSID.
      windowscodecs/tests: Add a top level block enumerator for the test block writer.
      windowscodecs/tests: Add some tests for handler objects enumerator.
      windowscodecs: Add missing traces to the IWICEnumMetadataItem implementation.
      msi: Avoid invalid access when handling format strings.
      windowscodecs/tests: Add some query tests with the Unknown reader.
      windowscodecs/metadata: Share implementation between query reader and writer objects.
      windowscodecs/tests: Add some tests for CreateQueryWriter().
      windowscodecs/tests: Add query reader tests for live block reader updates.
      windowscodecs/tests: Add some tests for CreateQueryWriterFromReader().
      windowscodecs/tests: Add some tests for the query reader container format.
      windowscodecs/metadata: Use VT_LPWSTR type instead of BSTRs when parsing queries.
      windowscodecs/metadata: Collect query components before assigning values.
      windowscodecs/metadata: Add a helper to parse query index syntax.
      windowscodecs/metadata: Use separate helpers to parse query items.
      windowscodecs/metadata: Handle empty items in queries.
      windowscodecs/metadata: Return query writer object from GetMetadataByName() when block writer is used.
      windowscodecs/metadata: Base returned query handlers on metadata handlers.
      windowscodecs: Implement CreateQueryWriter().
      windowscodecs/tests: Add some tests for default metadata item set.
      windowscodecs/metadata: Add flags mask to configure builtin handlers.
      windowscodecs/metadata: Implement removing items with IWICMetadataWriter.
      windowscodecs/tests: Add some tests for RemoveMetadataByName().
      windowscodecs/metadata: Implement RemoveMetadataByName().
      windowscodecs/tests: Remove WinXP workarounds from png metadata test.
      windowscodecs/tests: Move non-specific CreateQueryReaderFromBlockReader() tests to a separate function.
      windowscodecs/tests: Add a test for reader instances returned from the decoder.
      windowscodecs: Remove TGA decoder.

Paul Gofman (24):
      user32/tests: Test system generated WM_MOUSEMOVE with raw input.
      server: Ignore WM_MOUSEMOVE with raw input / RIDEV_NOLEGACY.
      dwmapi: Sleep in DwmFlush().
      ntoskrnl.exe/tests: Open directory object with nonzero access in test_permanent().
      httpapi: Don't open files with zero access.
      kernel32: Don't open reg keys with zero access mask.
      setupapi: Don't open reg keys with zero access mask.
      devenum: Don't open reg keys with zero access mask.
      quartz: Don't open reg keys with zero access mask.
      wbemprox: Don't open reg keys with zero access mask.
      shell32: Don't open reg keys with zero access mask.
      server: Check for zero access in alloc_handle().
      include: Add cfgmgr ID list filter constants.
      setupapi: Implement CM_Get_Device_ID_List[_Size]W().
      setupapi: Implement CM_Get_Device_ID_List[_Size]A().
      setupapi: Make device instance handlers persistent.
      setupapi: Implement CM_Locate_DevNode[_Ex]{A|W}().
      winex11.drv: Sort Vulkan physical devices in get_gpu_properties_from_vulkan().
      win32u: Sort Vulkan physical devices in get_vulkan_gpus().
      crypt32/tests: Add test for chain engine cache update.
      crypt32: Lock store in MemStore_deleteContext().
      crypt32: Do not temporary delete contexts in I_CertUpdateStore().
      crypt32: Only sync registry store if registry has changed.
      crypt32: Resync world store for default engines in get_chain_engine().

Rémi Bernon (11):
      win32u: Introduce a new add_virtual_mode helper.
      win32u: Pass host enumerated display modes to get_virtual_modes.
      win32u: Keep virtual screen sizes in a SIZE array.
      win32u: Introduce a new get_screen_sizes helper.
      win32u: Check generated sizes with the maximum display mode.
      win32u: Generate fake resolution list from the host modes.
      win32u: Generate modes for the host native frequency.
      server: Introduce a new find_mouse_message helper.
      winetest: Ignore success and color codes in tests output limit.
      win32u: Drop now unnecessary WM_WINE_DESKTOP_RESIZED internal message.
      winex11: Delay changing window config if win32 side is still minimized.

Sven Baars (2):
      win32u: Fix an uninitialized variable warning on GCC 10.
      winegstreamer: Fix a compiler warning with GStreamer 1.18 or lower.

Torge Matthies (2):
      ntdll: Add sys_membarrier-based implementation of NtFlushProcessWriteBuffers.
      ntdll: Add thread_get_register_pointer_values-based implementation of NtFlushProcessWriteBuffers.

Vibhav Pant (10):
      winebth.sys: Don't call dbus_pending_call_set_notify with a NULL pending call.
      winebth.sys: Implement IOCTL_WINEBTH_RADIO_SET_FLAG.
      winebth.sys: Call bluez_watcher_close as part of bluetooth_shutdown.
      bluetoothapis: Add stub for BluetoothEnableIncomingConnections.
      bluetoothapis/tests: Add tests for BluetoothEnableIncomingConnections.
      bluetoothapis: Implement BluetoothEnableIncomingConnections.
      bluetoothapis: Add stub for BluetoothEnableDiscovery.
      bluetoothapis/tests: Add tests for BluetoothEnableDiscovery.
      bluetoothapis: Implement BluetoothEnableDiscovery.
      winebth.sys: Free Unix handle to the local radio after updating its properties.

Wei Xie (1):
      http.sys: Avoid crashes in complete_irp() if http header value is space.

Zhiyi Zhang (17):
      prntvpt: Add a missing return (Coverity).
      ntdll: Implement RtlEnumerateGenericTableWithoutSplaying().
      ntdll/tests: Add RtlEnumerateGenericTableWithoutSplaying() tests.
      ntdll: Implement RtlEnumerateGenericTable().
      ntdll/tests: Add RtlEnumerateGenericTable() tests.
      ntdll: Implement RtlGetElementGenericTable().
      ntdll/tests: Add RtlGetElementGenericTable().
      user32/tests: Test TME_LEAVE when the cursor is not in the specified tracking window.
      win32u: Post WM_MOUSELEAVE immediately when the cursor is not in the specified tracking window.
      include: Add D3DKMT_DRIVERVERSION.
      gdi32/tests: Add D3DKMTQueryAdapterInfo() tests.
      win32u: Support KMTQAITYPE_CHECKDRIVERUPDATESTATUS for NtGdiDdDDIQueryAdapterInfo().
      win32u: Support KMTQAITYPE_DRIVERVERSION for NtGdiDdDDIQueryAdapterInfo().
      winex11.drv: Go though WithdrawnState when transitioning from IconicState to NormalState.
      d2d1: Add D2D1ColorMatrix effect stub.
      d2d1: Add D2D1Flood effect stub.
      d3d11/tests: Test D3D11_FEATURE_D3D9_SIMPLE_INSTANCING_SUPPORT feature.
```
