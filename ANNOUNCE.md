The Wine development release 11.0-rc1 is now available.

This is the first release candidate for the upcoming Wine 11.0. It
marks the beginning of the yearly code freeze period. Please give this
release a good testing and report any issue that you find, to help us
make the final 11.0 as good as possible.

What's new in this release:
  - Mono engine updated to version 10.4.0.
  - Locale data updated to Unicode CLDR 48.
  - TWAINDSM module for scanner support on 64-bit.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/11.0/wine-11.0-rc1.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-11.0-rc1/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 11.0-rc1 (total 17):

 - #36514  King's Quest: Mask of Eternity requires MCIWndCreate to create a child window when parent is specified
 - #38924  HeapSize(GetProcessHeap(), 0, GlobalLock(hGlobal)) must succeed [wxWidgets samples, Tapps2, DirMaster]
 - #56386  Gramps 5.2.0 displays empty windows
 - #58037  Photoshop CS 2 installation never completes
 - #58383  mmdevapi notify_thread busy waits when no midi driver can be loaded
 - #58408  win32u:input: Performance regression in Resident Evil 2 due to internal messages processing.
 - #58827  Running Mahjong with Wine ends without displaying the game screen
 - #58949  Sound test in winecfg doesn't produce audio
 - #58975  Mugen crashes after multiple matches on NVIDIA GPUs.
 - #59006  FL Studio fails to minimize on Wine 10.19
 - #59010  Window immediately restores after minimizing when a non-modal dialog is open
 - #59041  Incorrect Casting for NET 10 Runtime Apps
 - #59052  cmd doesn't handle all parameters in FOR /F command
 - #59054  Oblivion crashes with unhandled page fault: wine 10.19
 - #59061  Office 2013 File menu doesn't open
 - #59064  Mass Effect Legendary Window Surface Not Drawn To
 - #59075  various focus-related winforms tests fail in virtual desktop

### Changes since 10.20:
```
Akihiro Sagawa (1):
      msvfw32: Retry with avivideo device type if open fails.

Alexandre Julliard (12):
      Revert "icuin: Add initial dll."
      Revert "icuuc: Add initial dll."
      Revert "icu: Add stub dll."
      ntdll: Remove "experimental wow64" message.
      faudio: Import upstream release 25.12.
      nls: Update locale data to CLDR version 48.
      mmdevapi: Don't fall back to initial driver when the MIDI driver fails to load.
      makedep: Add an install-unixlib target as a subset of install-lib.
      makedep: Don't assume that .tab.h files don't contain includes.
      wow64: Build a proper wow64 IO status block for FSCTL_GET_OBJECT_ID.
      ntdll: Silence warning about non-builtin for fake dlls.
      tomcrypt: Force using 32-bit math library.

Anton Baskanov (2):
      dmsynth: Calculate latency and the derive latency clock from the master clock.
      dmsynth: Factor out waiting for the buffer event.

Bernd Herd (8):
      sane.ds: Fix ICAP_BITDEPTH semantics, allow setting it.
      sane.ds: Refuse native transfer mode with depth 16 bit.
      sane.ds: Support SANE Backends that treat resolution as TYPE_FIXED.
      sane.ds: Make UI non-modal as required by Twain specification.
      twaindsm: Put DSM code into twaindsm.dll and add twain_32.dll wrapper.
      twaindsm: Implement DG_CONTROL/DAT_ENTRYPOINT/MSG_GET.
      twaindsm: Add DG_CONTROL/DAT_CALLBACKx/MSG_REGISTER_CALLBACK to support TWAIN 2.x protocol.
      twaindsm: Recursively search in C:\Windows\twain_xx for installed data sources.

Bernhard Übelacker (1):
      faudio: Make sure at least some wavebank notifications get allocated (ASan).

Biswapriyo Nath (2):
      include: Add D3D12_FEATURE_DATA_VIDEO_ENCODER_INTRA_REFRESH_MODE in d3d12video.idl.
      include: Add missing member in D3D12_VIDEO_ENCODER_CODEC_PICTURE_CONTROL_SUPPORT.

Brendan Shanks (2):
      win32u: Use asprintf in read_drm_device_prop.
      winecoreaudio: Set the AudioChannelLayout on output units.

Charlotte Pabst (4):
      mfsrcsnk/tests: Add tests for thinning.
      winedmo: Fall back to dts for sample time if pts is not present.
      mfsrcsnk: Process SetRate asynchronously.
      mfsrcsnk: Implement thinning.

Derek Lesho (2):
      opengl32: Cleanup glGet* and fix typo in glGetDoublev.
      opengl32: Move GL error to wgl_context.

Dmitry Timoshkov (1):
      ldap: Consistently use unicode version of QueryContextAttributes().

Elizabeth Figura (5):
      qcap: Avoid inverting frame interval twice.
      qcap: Iterate over all frame intervals to get the max/min.
      qcap: Implement GetFrameRateList().
      cmd: Print reparse points in directory listings.
      cmd: Implement mklink /j.

Eric Pouech (10):
      dbghelp: Fix skipping of inline call-site information.
      dbghelp: Properly limit area of CU's symbols.
      dbghelp: Properly skip S_FRAMEPROC inside thunk.
      dbghelp: Silence a warning.
      dbghelp: Don't use checksum on ELF files.
      cmd: Enhance parsing of FOR /F options.
      dbghelp: Detect and fail decorated TPI indexes.
      dbghelp: Revamp DBI hash table.
      dbghelp: Use contribution to select compiland for line info.
      dbghelp: Use contrib to select compiland in advance_line_info.

Esme Povirk (1):
      mscoree: Update Wine Mono to 10.4.0.

Floris Renaud (1):
      po: Update Dutch translation.

Georg Lehmann (1):
      winevulkan: Update to VK spec version 1.4.335.

Gerald Pfeifer (1):
      user32/tests: Avoid compiler warnings in tests.

Giovanni Mascellani (3):
      winmm/tests: Test a few PCM and float wave formats.
      winmm/tests: Test some more exotic wave formats.
      Revert "winmm: Use AUTOCONVERTPCM when initializing the audio client.".

Hans Leidekker (9):
      cryptui: Properly initialize the filename in show_import_ui().
      cryptui: Pass the certificate context to CryptUIWizImport() from the certificate viewer.
      cryptui: Allow switching from automatic to manual certificate store selection.
      wbemprox/tests: Fix test failures on Windows 11.
      wbemprox/tests: Consistently use check_property_nullable().
      bcrypt/tests: Fix test failures on Windows 8 and 11.
      advapi32/tests: Fix test failures on Windows 11.
      dbgeng/tests: Fix test failures on Windows 11.
      fusion/tests: Run tests on .NET version 4.

Haoyang Chen (1):
      devenum: Add DevicePath field for a video input device.

Henri Verbeet (8):
      quartz/vmr7: Handle BI_BITFIELDS formats.
      quartz/vmr7: Reject BI_RGB and BI_BITFIELDS formats with different bit depth.
      quartz/vmr7: Reject unsupported FOURCC formats.
      d3d11: Add the D3D11_DECODE_FILTER_REDUCTION macro.
      d3d11/tests: Add a sampler min/max reduction filtering test.
      wined3d/vk: Implement sampler min/max reduction filtering.
      wined3d/gl: Implement sampler min/max reduction filtering.
      wined3d: Create cube views for cube textures in wined3d_texture_acquire_identity_srv().

Jacek Caban (1):
      wininet: Move dwError to http_request_t.

Martin Storsjö (7):
      ntdll/tests: Update the reference code for a changed test.
      ntdll/tests: Enable tests for save_next for float registers.
      winedump: Fix the printout of a cornercase with packed arm64 unwind info.
      ntdll: Implement handling of arm64 packed unwind for CR=01, RegI=1.
      ntdll/tests: Add a missing testcase for arm64 packed unwind info.
      ntdll: Rewrite arm64 packed unwind info handling.
      ntdll: Handle arm64 packed unwind with H=1, RegI=RegF=0, CR!=1.

Matteo Bruni (21):
      ddraw: Advertise NV12 FOURCC as supported.
      quartz/vmr7: Implement IVMRSurfaceAllocatorNotify::SetDDrawDevice().
      quartz/vmr7: Create a ddraw object if necessary to check for FourCC support.
      quartz/vmr7: Validate BITMAPINFOHEADER size.
      quartz/tests: Test allocating a surface with different bit depth from the primary.
      quartz/tests: Test allocating BI_BITFIELDS pixel format.
      quartz/tests: Test VMR7 AllocateSurface with a BITMAPV4HEADER.
      quartz/tests: Add some VMR7 tests for unsupported formats.
      ntdll: Factor out a cancel_io() function.
      server: Factor out a cancel_async() function.
      ntdll: Wait for all asyncs to handle cancel in NtCancelIoFile().
      ntdll/tests: Add more NtCancelIoFile[Ex]() tests.
      ntdll/tests: Test IOSB values of the cancel operation.
      ntoskrnl/tests: Fix tests on current Windows 10 / 11.
      ntoskrnl/tests: Add more cancellation tests.
      ntoskrnl/tests: Test the thread ID the cancellation routine runs from.
      ntoskrnl/tests: Use the 'Nt' version of the CancelIo APIs.
      quartz/vmr7: Implement IVMRSurfaceAllocator::AdviseNotify() on the default allocator.
      quartz/vmr7: Call IVMRSurfaceAllocator_AdviseNotify() on the default allocator.
      quartz/tests: Show where AllocateSurface should be called.
      include: Fix RenderPrefs_ForceOffscreen typo.

Nikolay Sivov (5):
      comdlg32/tests: Add a helper to check for supported interfaces.
      comdlg32/tests: Check for IModalWindow interface.
      comdlg32/itemdlg: Add missing IModalWindow to supported interfaces.
      d2d1: Add a Scale effect stub.
      msxml/tests: Add some tests for supported interfaces in the SAX API.

Pan Hui (1):
      wmp: Implement OLEIVERB_SHOW.

Paul Gofman (10):
      include: Add defintions related to Windows.Perception.Spatial.SpatialAnchorExporter.
      windows.perception.stub: Stub SpatialAnchorExporter class.
      windows.perception.stub: Implement exporter_statics_RequestAccessAsync().
      windows.media.speech: Synchronize IAsyncInfo implementation in async.c.
      windows.gaming.input: Synchronize IAsyncInfo implementation in async.c.
      cryptowinrt: Synchronize IAsyncInfo implementation in async.c.
      windows.devices.enumeration: Synchronize IAsyncInfo implementation in async.c.
      windows.security.credentials.ui.userconsentverifier: Synchronize IAsyncInfo implementation in async.c.
      coremessaging: Synchronize IAsyncInfo implementation in async.c.
      win32u: Don't alter memory beyond structure effective size in NtUserEnumDisplaySettings().

Piotr Caban (23):
      msado15: Set ActiveConnection on recordset created by connection_OpenSchema.
      msado15: Handle NULL values in field_get_Value.
      msado15: Fix leak in recordset_put_CacheSize.
      msado15: Support more properties in rowset_info_GetProperties.
      msado15: Add _Recordset::LockType implementation.
      msado15: Validate LockType in rsconstruction_put_Rowset.
      msado15: Implement _Recordset::Supports function.
      msado15: Add _Recordset::Supports tests.
      include: Define CursorOptionEnum values using hexadecimal constants.
      include: Add DBINDEXCOLUMNDESC definition.
      include: Add IRowsetIndex interface.
      include: Add IRowsetCurrentIndex interface.
      msado15: Add _Recordset:put_Index implementation.
      msado15: Add _Recordset:get_Index implementation.
      msado15: Request features determined by lock type when opening table directly.
      msado15: Add partial _Recordset:CancelUpdate implementation.
      msado15: Add partial _Recordset::Update implementation.
      msado15: Call _Recordset::Update when moving to new row.
      msado15: Use accessors cache in _Recordset::AddNew.
      msado15: Handle data argument in rowset_change_InsertRow.
      msado15: Support setting values in _Recordset::AddNew.
      msado15: Support setting values in _Recordset::Update.
      msado15: Request more features when opening table directly.

Rémi Bernon (28):
      win32u: Avoid a crash when drawable fails to be created.
      win32u: Check internal drawables before trying to create new ones.
      winex11: Allow client window creation on other process windows.
      win32u: Update the window client surface even with no children.
      server: Check whether window can be made foreground earlier.
      server: Always allow windows to activate after their creation.
      opengl32: Don't generate null functions for extensions.
      opengl32: Generate some missing null functions.
      opengl32: Expect core OpenGL functions to be present.
      opengl32: Get OpenGL function table on process attach.
      winemac: Avoid taking the window lock when creating client surface.
      opengl32: Don't count EGL extensions in the registry arrays.
      opengl32: List unsupported extensions rather than supported ones.
      opengl32: Generate functions and constants for more EGL extensions.
      win32u: Query EGL devices UUID with EGL_EXT_device_persistent_id.
      winewayland: Update client surface position in update callback.
      wined3d: Remove now unnecessary pixel format restoration.
      wined3d: Create / release the window DCs with the swapchains.
      win32u: Introduce a D3DKMT escape code to set fullscreen present rect.
      wined3d: Set the window present rect when entering fullscreen mode.
      wined3d: Use the backbuffer size rather than client rect when fullscreen.
      server: Introduce a find_async_from_user helper.
      mfsrcsnk: Introduce a DEFINE_MF_ASYNC_PARAMS macro.
      mfsrcsnk: Factor out a media_source_request_stream_sample helper.
      mfsrcsnk: Peek for stream token presence without removing it.
      win32u: Require VK_KHR_external_semaphore_fd for keyed mutexes.
      win32u: Expose onscreen single buffer formats with PFD_SUPPORT_GDI.
      win32u: Workaround a 32-bit llvmpipe crash on initialization.

Vibhav Pant (2):
      winebth.sys: Unify critical sections used for bluetooth_radio.
      winebth.sys: Fix potential deadlocks while performing operations that block on the DBus event loop.

Yuxuan Shui (3):
      winegstreamer: Fix SetOutputType of the wma decoder DMO.
      mf/tests: Test Get{Input,Output}CurrentType.
      winegstreamer: Implement media_object_Get{Input,Output}CurrentType.

Zhiyi Zhang (2):
      user32/tests: Test scrollbar rect when WS_HSCROLL or WS_VSCROLL is present.
      win32u: Remove scrollbar rect offsets when WS_HSCROLL or WS_VSCROLL is present.
```
