The Wine development release 9.3 is now available.

What's new in this release:
  - Improvements to Internet Proxy support.
  - New HID pointer device driver.
  - Timezone database update.
  - More exception fixes on ARM platforms.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/9.x/wine-9.3.tar.xz>

Binary packages for various distributions will be available
from <https://www.winehq.org/download>

You will find documentation on <https://www.winehq.org/documentation>

Wine is available thanks to the work of many people.
See the file [AUTHORS][1] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/raw/wine-9.3/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 9.3 (total 23):

 - #33050  FDM (Free Download Manager) crashes with page fault when any remote FTP directory opened
 - #46070  Basemark Web 3.0 Desktop Launcher crashes
 - #46263  Final Fantasy XI crashes after accepting EULA when using Ashita; World of Warships hangs at login screen
 - #46839  Happy Foto Designer Font not found "Fehler (Code 1) [Font is not supported: Roboto]"
 - #50643  IK Product Manager: Unable to download plugins
 - #51458  Western Digital SSD Dashboard displays black screen
 - #51599  cmd.exe incorrectly parses an all-whitespace line followed by a closing parenthesis
 - #51813  python fatal error redirecting stdout to file
 - #52064  Solidworks 2008 crashes on startup
 - #52642  Virtual Life 2 freezes
 - #54794  Autodesk Fusion360: New SSO login will not open web browser
 - #55282  Flutter SDK can't find "aapt" program (where.exe is a stub)
 - #55584  Possibly incorrect handling of end_c in ARM64 process_unwind_codes
 - #55630  DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 is not handled in GetAwarenessFromDpiAwarenessContext
 - #55810  Finding Nemo (Steam): window borders gone missing (virtual desktop)
 - #55897  cpython 3.12.0 crashes due to unimplemented CopyFile2
 - #56065  Missing GetAnycastIpAddressTable() implementation
 - #56139  scrrun: Dictionary does not allow storing at key Undefined
 - #56222  Microsoft Flight Simulator 2020 (steam) needs unimplemented function GDI32.dll.D3DKMTEnumAdapters2
 - #56244  SplashTop RMM client for Atera crashes on unimplemented function shcore.dll.RegisterScaleChangeNotifications
 - #56273  [Sway] winewayland.drv: cursor does not work in Dead Island 2
 - #56328  LMMS 1.2.2 SF2 soundfonts no longer work in Wine 9.1
 - #56343  Multiple applications fail to start after de492f9a34

### Changes since 9.2:
```
Alex Henrie (8):
      ntdll: Include alloc_type argument in NtMapViewOfSection(Ex) traces.
      rpcrt4/tests: Use CRT allocation functions.
      where: Implement search with default options.
      include: Annotate NdrGetBuffer with __WINE_(ALLOC_SIZE|MALLOC).
      wined3d: Use CRT allocation functions.
      include: Add debugstr_time to wine/strmbase.h.
      msxml3: Use CRT allocation functions.
      advapi32/tests: Use CRT allocation functions.

Alexandre Julliard (51):
      ntdll: Implement RtlLookupFunctionTable.
      ntdll/tests: Don't use x86-64 assembly on ARM64EC.
      include: Add some public exception handling structures.
      ntdll: Move RtlLookupFunctionEntry() to the CPU backends.
      ntdll: Move find_function_info() to the CPU backends.
      ntdll: Move RtlAddFunctionTable() to the CPU backends.
      ntdll: Support ARM64EC code in RtlLookupFunctionEntry.
      ntdll: Make APCs alertable by default on ARM platforms.
      ntdll: Move exception unwinding code to a separate file.
      ntdll: Move the dynamic unwind tables to unwind.c.
      ntdll: Move RtlUnwind to unwind.c.
      ntdll: Implement RtlVirtualUnwind for ARM64EC.
      winedump: Add dumping of the save_any_reg ARM64 unwind code.
      ntdll: Add support for the save_any_reg ARM64 unwind code.
      ntdll: Ignore end_c when processing ARM64 unwind codes.
      ntdll: Don't count custom stack frames as part of the prolog on ARM64.
      rpcrt4/tests: Fix some malloc/HeapAlloc mismatches.
      ntdll/tests: Simplify testing of unwind register values on ARM64.
      ntdll: Ignore home parameters saving when unwinding epilog on ARM64.
      ntdll: Move ARM64 context conversion functions to a new header.
      ntdll: Implement EC_CONTEXT unwinding operation on ARM64.
      ntdll: Implement CLEAR_UNWOUND_TO_CALL unwinding operation on ARM64.
      ntdll: Always set non-volatile pointers for ARM64 unwinding.
      kernelbase: Update timezone data to version 2024a.
      faudio: Import upstream release 24.02.
      mpg123: Import upstream release 1.32.5.
      png: Import upstream release 1.6.42.
      jpeg: Import upstream release 9f.
      zlib: Import upstream release 1.3.1.
      lcms2: Import upstream release 2.16.
      ldap: Import upstream release 2.5.17.
      xslt: Import upstream release 1.1.39.
      xml2: Import upstream release 2.11.7.
      zydis: Import upstream release 4.1.0.
      fluidsynth: Import upstream release 2.3.4.
      ntdll/tests: Move unwinding tests to a separate file.
      ntdll/tests: Run dynamic unwind tests on ARM platforms.
      ntdll/tests: Run RtlVirtualUnwind tests on ARM64EC.
      ntdll: Default to the SEH channel on x86-64.
      ntdll: Add a helper macro to dump a register context.
      ntdll: Share exception dispatch implementation across platforms.
      include: Add new idl files to the makefile.
      ntdll: Handle leaf functions in RtlVirtualUnwind on ARM64.
      ntdll: Handle leaf functions in RtlVirtualUnwind on ARM.
      ntdll: Handle leaf functions in RtlVirtualUnwind on x86-64.
      ntdll: Remove support for unwinding ELF dlls on ARM.
      configure: Stop passing ARM code generation options to winebuild.
      configure: Require floating point support on ARM targets.
      winegcc: Stop passing ARM code generation options to winebuild.
      winebuild: Remove ARM code generation option.
      ntdll/tests: Fix a test failure when exception information is missing.

Aurimas Fišeras (2):
      po: Update Lithuanian translation.
      po: Update Lithuanian translation.

Bernhard Übelacker (4):
      cmd: Handle lines with just spaces in bracket blocks.
      cmd: Avoid execution if block misses closing brackets.
      wininet: Add missing assignment of return value.
      wininet: Avoid crash in InternetCreateUrl with scheme unknown.

Biswapriyo Nath (5):
      include: Add IDirect3DSurface in windows.graphics.directx.direct3d11.idl.
      include: Add BitmapBuffer runtimeclass in windows.graphics.imaging.idl.
      include: Add SoftwareBitmap runtimeclass in windows.graphics.imaging.idl.
      include: Add DetectedFace runtimeclass in windows.media.faceanalysis.idl.
      include: Add FaceDetector runtimeclass in windows.media.faceanalysis.idl.

Brendan McGrath (1):
      d2d1/tests: Increase timeout from 1 sec to 5 secs.

Brendan Shanks (2):
      ntdll: Assume process-private futexes are always present on Linux.
      ntdll: On x86_64, don't access %gs in signal_start_thread().

Daniel Lehman (8):
      oleaut32: Handle exponent in VarBstrFromR[48] in non-English locales.
      oleaut32/tests: Add tests for number of digits.
      oleaut32: Use scientific notation only for larger numbers in VarBstrFromR[48].
      include: Add some msvcrt declarations.
      msvcrt: Use string sort for strncoll/wcsncoll.
      msvcrt/tests: Include locale in ok message.
      msvcrt/tests: Add tests for strcoll/wcscoll.
      msvcrt: Use string sort for strcoll/wcscoll.

David Kahurani (1):
      msi: Avoid leaking stream on DB update.

Eric Pouech (11):
      kernel32/tests: Add tests for CreateProcess with invalid handles.
      kernelbase: Don't use INVALID_HANDLE_VALUE as std handle in CreateProcess.
      msvcrt/tests: Extend test for invalid std handle on msvcrt init.
      msvcrt: Don't reset invalid std handle in init.
      kernel32/tests: Remove todo scaffolding for CreateProcess() tests.
      winedump: Don't crash on non-effective runtime function entries.
      kernel32/tests: Check if thread is suspended in debugger attachment tests.
      winedbg: Use share attributes for opening command file.
      dbghelp: Implement SymFromIndex().
      winedump: Don't expect a fixed number of substreams in DBI header (PDB).
      dbghelp: Don't expected a fixed number of substreams in DBI header (PDB).

Fabian Maurer (5):
      user32/sysparams: Only allow dpi awareness tests to return invalid on windows.
      user32/sysparams: Handle more contexts in GetAwarenessFromDpiAwarenessContext.
      wineoss: Remove superflous check.
      winealsa: Remove superflous check.
      mmdevapi/tests: Add test for invalid format with exclusive mode.

Floris Renaud (1):
      po: Update Dutch translation.

Gabriel Ivăncescu (7):
      jscript: Add initial implementation of ArrayBuffer.
      jscript: Add initial implementation of DataView.
      jscript: Implement DataView setters.
      jscript: Implement DataView getters.
      jscript: Implement ArrayBuffer.prototype.slice.
      kernelbase: Copy the read-only attribute from the source.
      explorer: Set layered style on systray icons before calling into the driver.

Geoffrey McRae (1):
      include: Add interfaces for ID3D11On12Device1 and ID3D11On12Device2.

Hans Leidekker (1):
      wbemprox: Protect tables with a critical section.

Ivo Ivanov (2):
      hidclass.sys: Use the correct string for container_id.
      hidclass.sys: Use HID_DEVICE in the mfg_section to match the recent changes.

Jacek Caban (7):
      winebuild: Introduce exports struct.
      winebuild: Use exports struct for imports handling.
      winebuild: Use exports struct for 16-bit modules handlign.
      winebuild: Use exports struct for exports handling.
      winebuild: Use exports struct in assign_ordinals.
      winebuild: Use exports struct in assign_names.
      winebuild: Move target filtering to assign_exports.

Jinoh Kang (6):
      ntdll: Remove stale comment from set_async_direct_result() documentation.
      include: Add definition for FILE_STAT_INFORMATION.
      ntdll/tests: Add tests for NtQueryInformationFile FileStatInformation.
      ntdll: Implement NtQueryInformationFile FileStatInformation.
      kernelbase: Replace FileAllInformation with FileStatInformation in GetFileInformationByHandle().
      kernel32/tests: Fix console test with odd-sized consoles.

Kartavya Vashishtha (1):
      kernelbase: Implement CopyFile2().

Krzysztof Bogacki (4):
      gdi32/tests: Add D3DKMTEnumAdapters2 tests.
      gdi32: Add D3DKMTEnumAdapters2() stub.
      win32u: Maintain a list of GPUs in cache.
      win32u: Implement NtGdiDdDDIEnumAdapters2.

Louis Lenders (1):
      imm32: Update spec file.

Marc-Aurel Zent (4):
      iphlpapi: Implement GetRTTAndHopCount().
      server: Use __pthread_kill() syscall wrapper.
      server: Use bootstrap_register2() instead of bootstrap_register().
      server: Improve formatting in mach init_tracing_mechanism().

Mohamad Al-Jaf (5):
      include: Add windows.security.authentication.onlineid.idl file.
      windows.security.authentication.onlineid: Add stub DLL.
      windows.security.authentication.onlineid: Add IOnlineIdSystemAuthenticatorStatics stub interface.
      windows.security.authentication.onlineid: Add IOnlineIdServiceTicketRequestFactory stub interface.
      windows.security.authentication.onlineid: Implement IOnlineIdSystemAuthenticatorStatics::get_Default().

Nikola Kuburović (1):
      kernelbase: Reduce FIXME to TRACE if params is null.

Paul Gofman (29):
      kernel32/tests: Add tests for critical section debug info presence.
      strmbase: Force debug info in critical sections.
      dmime: Force debug info in critical sections.
      dmsynth: Force debug info in critical sections.
      mapi32: Force debug info in critical sections.
      propsys: Force debug info in critical sections.
      rpcrt4: Force debug info in critical sections.
      vcomp: Force debug info in critical sections.
      webservices: Force debug info in critical sections.
      ntdll: Don't hardcode xstate feature mask.
      ntdll: Don't hardcode xstate size in syscall frame.
      ntdll: Don't hardcode xstate size in exception stack layout.
      powershell: Read input command from stdin.
      winhttp: Force debug info in critical sections.
      xaudio2: Force debug info in critical sections.
      kernelbase: Force debug info in critical sections.
      combase: Force debug info in critical sections.
      crypt32: Force debug info in critical sections.
      winhttp: Mind read size when skipping async read in WinHttpReadData().
      dinput: Force debug info in critical sections.
      dplayx: Force debug info in critical sections.
      dsound: Force debug info in critical sections.
      dwrite: Force debug info in critical sections.
      inetcomm: Force debug info in critical sections.
      mscoree: Force debug info in critical sections.
      ntdll: Force debug info in critical sections.
      qmgr: Force debug info in critical sections.
      windowscodecs: Force debug info in critical sections.
      wininet: Force debug info in critical sections.

Piotr Caban (20):
      wininet: Store proxy type in proxyinfo_t.
      wininet: Don't allocate global_proxy structure dynamically.
      wininet: Move reading proxy settings from registry to separate function.
      wininet: Return process-wide proxy settings from INTERNET_GetProxySettings.
      wininet: Add support for reading AutoConfigURL from registry.
      wininet: Store whole ProxyServer string so it's not lost while saving settings to registry.
      wininet: Set ProxyOverride registry key when saving proxy information.
      wininet: Set AutoConfigURL registry key when saving proxy information.
      wininet: Test INTERNET_OPTION_PER_CONNECTION_OPTION on process settings.
      wininet: Fix buffer size calculation in InternetQueryOption(INTERNET_OPTION_PER_CONNECTION_OPTION).
      wininet: Use GlobalAlloc in InternetQueryOption(INTERNET_OPTION_PER_CONNECTION_OPTION).
      wininet/tests: Cleanup INTERNET_OPTION_PER_CONNECTION_OPTION tests.
      wininet: Add support for writing connection settings binary blobs from registry.
      wininet: Add support for reading connection settings binary blobs from registry.
      wininet: Add support for INTERNET_PER_CONN_AUTOCONFIG_URL in InternetSetOption.
      wininet: Fix INTERNET_PER_CONN_AUTOCONFIG_URL option when quering global proxy settings.
      wininet: Add support for INTERNET_OPTION_PER_CONNECTION_OPTION option on session handles.
      wininet/tests: Add more INTERNET_OPTION_PER_CONNECTION_OPTION tests.
      inetcpl.cpl: Use wininet functions to set proxy settings.
      wininet/tests: Add initial PAC script tests.

Rastislav Stanik (1):
      iphlpapi: Add stub for GetAnycastIpAddressTable().

Rémi Bernon (45):
      winex11: Process XInput2 events with QS_INPUT filter.
      winex11: Advertise XInput2 version 2.2 support.
      winex11: Initialize XInput2 extension on every thread.
      winex11: Always listen to XInput2 device changes events.
      winex11: Simplify XInput2 device valuator lookup.
      hidclass: Make HID hardware ids more similar to windows.
      hidclass: Only access Tail.Overlay.OriginalFileObject when needed.
      mouhid.sys: Introduce a new HID pointer device driver.
      mouhid.sys: Request preparsed data and inspect device caps.
      mouhid.sys: Read reports from the underlying HID device.
      mouhid.sys: Parse HID reports to track contact points.
      mfreadwrite/reader: Introduce source_reader_queue_sample helper.
      mfreadwrite/reader: Pass the transform to source_reader_pull_stream_samples.
      mfreadwrite/reader: Introduce a new source_reader_allocate_stream_sample helper.
      mfreadwrite/reader: Introduce new source_reader_(drain|flush)_transform_samples helpers.
      mfreadwrite/reader: Repeat pushing / pulling samples while it succeeds.
      winevulkan: Wrap host swapchain handles.
      winevulkan: Adjust VkSurfaceCapabilitiesKHR image extents with client rect.
      winewayland: Remove now unnecessary VkSurfaceCapabilitiesKHR fixups.
      winevulkan: Implement vkGetPhysicalDeviceSurfaceCapabilities2KHR fallback.
      winevulkan: Remove now unnecessary vkGetPhysicalDeviceSurfaceCapabilities2KHR driver entry.
      winevulkan: Remove now unnecessary vkGetPhysicalDeviceSurfaceCapabilitiesKHR driver entry.
      winegstreamer: Use MFCreateVideoMediaTypeFromSubtype in GetInputAvailableType.
      winegstreamer: Use MFCreateVideoMediaTypeFromSubtype in GetOutputAvailableType.
      winegstreamer: Remove unnecessary create_output_media_type checks.
      winegstreamer: Use GUID arrays for H264 decoder media types.
      winegstreamer: Complete H264 current output type reported attributes.
      win32u: Deduce monitor device flags from their adapter.
      win32u: Only consider active monitors for clone detection.
      win32u: Only consider active monitors for virtual screen rect.
      win32u: Only consider active monitors in monitor_from_rect.
      win32u: Don't assume that the primary adapter is always first.
      winegstreamer: Use MFCalculateImageSize to compute output info size.
      ir50_32: Use the proper hr value for stream format change.
      winegstreamer: Use the H264 decoder to implement the IV50 decoder.
      winegstreamer: Rename struct h264_decoder to struct video_decoder.
      winevulkan: Handle creation of surfaces with no HWND directly.
      winex11: Remove now unnecessary create_info HWND checks.
      winevulkan: Handle invalid window in vkCreateSwapchainKHR.
      winevulkan: Handle invalid window in vkGetPhysicalDevicePresentRectanglesKHR.
      winevulkan: Remove now unnecessary vkGetPhysicalDevicePresentRectanglesKHR driver entry.
      winevulkan: Implement vkGetPhysicalDeviceSurfaceFormats2KHR fallback.
      winevulkan: Remove now unnecessary vkGetPhysicalDeviceSurfaceFormats2KHR driver entry.
      winevulkan: Remove now unnecessary vkGetPhysicalDeviceSurfaceFormatsKHR driver entry.
      server: Use the startup info to connect the process winstation.

Santino Mazza (7):
      mlang/tests: Test for GetGlobalFontLinkObject.
      mlang: Implement GetGlobalFontLinkObject.
      mlang/tests: Test codepages priority bug in GetStrCodepages.
      mlang: Fix bug with codepage priority in GetStrCodePages.
      gdiplus: Replace HDC with GpGraphics in parameters.
      gdiplus/tests: Add interactive test for font linking.
      gdiplus: Implement font linking for gdiplus.

Shaun Ren (15):
      sapi: Implement ISpeechObjectToken::GetDescription.
      sapi: Implement ISpeechObjectToken::Invoke.
      sapi: Implement ISpeechObjectToken::GetIDsOfNames.
      sapi: Implement ISpeechObjectTokens::get_Count.
      sapi: Implement IEnumVARIANT::Next for ISpeechObjectTokens.
      sapi: Implement ISpeechObjectTokens::Invoke.
      sapi: Free typelib on DLL detach.
      sapi: Implement ISpeechVoice::Speak.
      sapi: Handle zero-length attributes correctly in ISpObjectTokenCategory::EnumTokens.
      sapi: Introduce create_token_category helper in tts.
      sapi: Implement ISpeechVoice::GetVoices.
      sapi: Implement ISpeechVoice::GetTypeInfoCount.
      sapi: Implement ISpeechVoice::GetTypeInfo.
      sapi: Implement ISpeechVoice::GetIDsOfNames.
      sapi: Implement ISpeechVoice::Invoke.

Vijay Kiran Kamuju (1):
      include: Add msdelta header file.

Yuxuan Shui (6):
      dmband: Implement getting/setting GUID_BandParam on band tracks.
      dmime/tests: Add MIDI loading test.
      dmime: Parse MIDI headers.
      dmime: Don't skip chunk for MIDI files.
      dmime: Read through a MIDI file.
      dmime/tests: Improve error reporting from expect_track.

Zebediah Figura (32):
      urlmon/tests: Add basic tests for CoInternetParseUrl(PARSE_CANONICALIZE).
      shlwapi/tests: Move the UrlCanonicalize() tests into test_UrlCanonicalizeA().
      shlwapi/tests: Remove the unused "wszExpectUrl" variable from check_url_canonicalize().
      shlwapi/tests: Use winetest_push_context() in test_UrlCanonicalizeA().
      shlwapi/tests: Move UrlCombine() error tests out of the loop.
      kernelbase: Do not use isalnum() with Unicode characters.
      kernelbase: Do not canonicalize the relative part in UrlCombine().
      kernelbase: Reimplement UrlCanonicalize().
      kernelbase: Use scheme_is_opaque() in UrlIs().
      shlwapi/tests: Add many more tests for UrlCanonicalize().
      dinput/tests: Return void from test_winmm_joystick().
      wined3d: Merge shader_load_constants() into shader_select().
      wined3d: Introduce a separate vp_disable() method.
      wined3d: Introduce a separate fp_disable() method.
      wined3d: Pass a wined3d_state pointer to the vp_enable() and fp_enable() methods.
      wined3d: Pass a non-const wined3d_context pointer to the FFP *_apply_draw_state() methods.
      wined3d/arb: Move fragment program compilation from fragment_prog_arbfp() to arbfp_apply_draw_state().
      wined3d: Set the pipeline key viewport and scissor count at initialization.
      wined3d: Make the viewport state dynamic.
      wined3d: Make the scissor state dynamic.
      wined3d: Enable EXT_extended_dynamic_state.
      wined3d: Use dynamic state for depth/stencil state if possible.
      shell32: Remove two unused strings.
      shell32: Always use IContextMenu::InvokeCommand() when selecting an item from the context menu.
      shell32: Do not set the default menu item from ShellView_DoContextMenu().
      shell32: Separate a get_filetype() helper.
      shell32: Properly implement context menu verbs.
      wined3d/arb: Move SPECULARENABLE constant loading to arbfp_apply_draw_state().
      wined3d/arb: Move TEXTUREFACTOR constant loading to arbfp_apply_draw_state().
      wined3d/arb: Move color key constant loading to arbfp_apply_draw_state().
      wined3d/arb: Move texture constant loading to arbfp_apply_draw_state().
      wined3d/arb: Move FFP bumpenv constant loading to arbfp_apply_draw_state().

Zhiyi Zhang (4):
      include: Rename DF_WINE_CREATE_DESKTOP to DF_WINE_VIRTUAL_DESKTOP.
      server: Inherit internal desktop flags when creating desktops.
      dsound/tests: Test that formats with more than two channels require WAVEFORMATEXTENSIBLE.
      dsound: Reject WAVEFORMATEX formats with more than two channels.
```
