The Wine development release 11.5 is now available.

What's new in this release:
  - C++ support in the build system.
  - Bundled ICU libraries.
  - Support for Syscall User Dispatch on Linux.
  - A number of VBScript compatibility fixes.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/11.x/wine-11.5.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-11.5/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 11.5 (total 22):

 - #31047  unable to edit gedit's preferences
 - #35904  USB connection not recognized by Axon MultiClamp Commander 700B.
 - #42687  Evernote installation fails
 - #42811  PCG Tools Application fails to install
 - #48291  Multiple applications crash due to direct use of x86_64 SYSCALL instruction (Detroit: Become Human, Red Dead Redemption 2, Arknights: Endfield)
 - #50532  get_timezone_info doing binary search in the wrong way
 - #50636  Reading indexed bitmap wrongly returns 32bit instead of 8bit
 - #55037  vbscript: Colon on new line after Then fails
 - #58806  Smart quotes in code example misleads clang
 - #58963  Stratego (1997) fails to start with error message "Unable to 'CreateScalableFontResource()'"
 - #59138  Timelapse export failing in Clip Studio Paint
 - #59223  Sony Home Memories Throws Unrecoverable error during installation
 - #59314  Swift crashes on unimplemented function ADVAPI32.dll.SaferiIsExecutableFileType
 - #59361  MDI child window creation don't respect the window attribute (not WS_MAXIMIZEBOX)
 - #59460  bcrypt: Implement RSA-OAEP padding defaults (fixes Wallpaper Engine mobile sync)
 - #59474  File dialog crashes in .NET applications like UndertaleModTool
 - #59502  VOCALOID6 crashes on startup due to unimplemented wminet_utils stubs
 - #59504  jscript: GetScriptDispatch("") returns E_INVALIDARG instead of global dispatch
 - #59514  time()/localtime() performance has regressed significantly
 - #59531  CertCreateCertificateChainEngine fails with invalid argument in rustls-platform-verifier
 - #59541  replacing bass.dll with samp.dll causes freeze/black screen under wine . samp.dll works fine under windows
 - #59542  CEF crashes in dwrite when browsing The Verge (weird font file)

### Changes since 11.4:
```
Akihiro Sagawa (3):
      po: Update Japanese translation.
      ole32/tests: Add more RegisterDragDrop tests after mismatched cleanup.
      ole32: Relax COM initialization check in RegisterDragDrop.

Alexandre Julliard (14):
      gitlab: Use Buildah instead of Kaniko to build images.
      makefiles: Add support for C++ sources.
      configure: Check for C++17 support.
      include: Import C++ headers from LLVM version 8.0.1.
      comctl32/tests: Add some optional treeview messages.
      makedep: Specify dll name when it can't be guessed from the spec file name.
      makedep: Still build asm files if the subdir doesn't match a known platform.
      makedep: Ignore internal static libraries that are disabled.
      makedep: Don't ignore disabled libs specified with the -l flag.
      libs: Import icucommon from upstream MS-ICU release 72.1.0.3.
      libs: Import icui18n from upstream MS-ICU release 72.1.0.3.
      unicode: Generate the ICU data file.
      icu: Add dll.
      ntdll: Disable syscall dispatch when loading a .so dll.

Alistair Leslie-Hughes (6):
      combase: Add RoFailFastWithErrorContext stub.
      include: Fix PromptDataSource parameter type.
      windows.ui.core.textinput: Fake success in ICoreInputView4::add_PrimaryViewHiding.
      windows.ui.core.textinput: Return S_OK in ICoreInputView::TryHidePrimaryView.
      windows.ui.core.textinput: Return S_OK in ICoreInputView::TryShowPrimaryView.
      twinapi.appcore: Support Windows.ApplicationModel.DataTransfer.DataTransferManager.

Andrew Nguyen (2):
      wminet_utils: Implement Get.
      wminet_utils: Implement CreateInstanceEnumWmi.

Anton Baskanov (8):
      dmsynth/tests: Add some polyphony tests.
      dmsynth: Restore pre-2.4.3 fluidsynth note-cut behavior.
      dmsynth: Put each channel into a separate channel group.
      dsound: Calculate required_input more accurately.
      dsound: Swap around the two nested loops in downsample.
      dsound: Calculate firgain more accurately.
      dsound: Transpose the FIR array to make the element access sequential.
      dsound: Premultiply the input value by firgain and the interpolation weights in downsample.

Bernhard Kölbl (8):
      windows.media.speech/tests: Change a value check.
      windows.media.speech/tests: Skip some voice tests on Windows 8.
      windows.media.speech: Make sure voice provider is initialized before returning default voice.
      windows.media.speech: Set pointer to NULL.
      mf: Prevent invalid topologies from being started.
      mfreadwrite/tests: Test IMFByteStream release behavior.
      mfsrcsnk/tests: Test IMFByteStream release behavior.
      mfsrcsnk: Move IMFByteStream release into IMFMediaSource_Shutdown().

Brendan McGrath (18):
      mf/tests: Test decoder node for MARKIN/MARKOUT attributes.
      mf: Add MARKIN/MARKOUT attributes to decoder node.
      mf/tests: Move logic for sending delayed samples to helper function.
      mf/tests: Modify topology to start with no resolution.
      mf/tests: Test MARKIN will result in a trimmed sample.
      mf: Drop samples from transform if MARKIN_HERE is set.
      mf: Trim samples from transform if MARKIN_HERE is set.
      mf: Replace state variable with clock_state.
      mf: Implement presentation time source for SAR.
      mf: Check provided presentation clock has a time source.
      mf: Implement Scrubbing in SAR.
      mf: Discard samples prior to current seek time.
      mf: Report seek time up until we render past this point.
      mfmediaengine: Reset the sample requested flag on Stop.
      mfmediaengine: Implement looping behaviour.
      mfmediaengine: Keep the last presentation sample on flush.
      mf: Handle ENDOFSEGMENT marker in SAR.
      mfmediaengine: Implement scrubbing.

Brendan Shanks (1):
      server: On ARM64, only list 32-bit ARM as a supported architecture if the CPU supports it.

Conor McCarthy (2):
      winegstreamer: Do not copy the incorrect buffer.
      win32u/d3dkmt: Close the wait handle with NtClose() in NtGdiDdDDIAcquireKeyedMutex2().

Daniel Lehman (3):
      mxsml3/tests: Add tests for put_text.
      msxml3/tests: Skip put_text test for now.
      msxml3: Don't add cloned doc to orphan list.

Diego Colin (1):
      comctl32: Fix some typos in the comments.

Dmitry Timoshkov (3):
      user32/tests: Make test_ShowWindow_mdichild() use real MDI child windows.
      user32/tests: Test switching from a maximized MDI child to a child without WS_MAXIMIZEBOX.
      user32: Respect WS_MAXIMIZEBOX of the MDI child being activated.

Doug Johnson (1):
      advapi32: Add SaferiIsExecutableFileType stub.

Elizabeth Figura (8):
      d3d9/tests: Test format conversion in StretchRect().
      wined3d: Implement format conversion in the Vulkan blitter.
      ddraw: Fix caps tracing in IDirectDraw7::GetCaps().
      wined3d/vk: Implement fill mode.
      wined3d/hlsl: Force branching in branches that guard pow().
      ntdll: Implement syscall emulation on Linux using Syscall User Dispatch.
      gdi32: Redirect system32 in CreateScalableFontResourceW().
      gdi32: Redirect system32 in AddFontResourceEx().

Eric Pouech (5):
      kernelbase: Fix signature of TerminateProcess().
      include: Add same guard features as SDK does.
      include: Define struct _SECURITY_DESCRIPTOR*.
      include: Move NT_TIB(32|64) definitions in winnt.h.
      dbghelp/dwarf: Properly ignore Dwarf-5 compilation units.

Francis De Brabandere (12):
      vbscript: Split() should return an empty array for empty strings.
      vbscript: Do not fail on colon-prefixed statements in If blocks.
      vbscript: Allow empty Else blocks in If statements.
      vbscript: Fix ElseIf and Else handling for inline and multi-line forms.
      vbscript: Allow Me(Idx) to compile as a call to the default property.
      vbscript: Return "Object required" error for non-dispatch types.
      vbscript: Return proper error for For Each on non-collection types.
      vbscript: Support calling named item objects with arguments.
      vbscript: Report correct error character position for invalid parenthesized call.
      vbscript: Map DISP_E_DIVBYZERO to VBScript error 11.
      vbscript: Return type mismatch error for indexed assignment to non-array variables.
      vbscript: Allow Const to be used before its declaration.

Georg Lehmann (1):
      winevulkan: Update to VK spec version 1.4.346.

Giovanni Mascellani (15):
      winmm/tests: Skip format tests if there is no available audio device.
      mmdevapi/tests: Drop some corner-case tests.
      winmm/tests: Drop some corner-case tests.
      winepulse.drv: Support 24-bit PCM formats.
      winepulse.drv: Allow cbSize to be larger than 22 bytes for extensible wave formats.
      winepulse.drv: Remove support for A-law and mu-law formats.
      winepulse.drv: Remove support for 24-in-32 bits formats.
      mmdevapi: Do not query for format support in spatial audio.
      mmdevapi: Take the wave format validation code from the tests.
      mmdevapi/tests: Further tweak some corner-case tests.
      winmm/tests: Further tweak some corner-case tests.
      winmm: Do not try to convert sample rate or channel count.
      winmm: Hard code the list of supported formats.
      wineoss.drv: Describe 24-bit samples with AFMT_S24_PACKED when not on FreeBSD.
      wineoss.drv: Reject wave formats with invalid bits.

Hans Leidekker (10):
      winhttp/tests: Don't send more data than the receive buffer can hold.
      winhttp: WinHttpQueryDataAvailable() fails before a response has been received.
      winhttp: Don't report data available for certain response statuses, even if there is data.
      winhttp: Clear response headers after do_authorization().
      winhttp: Also do NTLM autorization when credentials are set between requests.
      winhttp: Introduce a stream abstraction.
      winhttp: Add decompression support.
      include: Update definition of CERT_CHAIN_ENGINE_CONFIG.
      crypt32: Trace CERT_CHAIN_ENGINE_CONFIG fields in CertCreateCertificateChainEngine().
      hnetcfg: Improve fw_manager_IsPortAllowed() stub.

Jacek Caban (11):
      tools: Treat -windows-gnu target as an LLVM-based toolchain.
      configure: Default to mingw mode on 32-bit ARM targets.
      vcruntime140: Add new and delete operators implementation to CRT library.
      vcruntime140: Add __type_info_root_node implementation.
      vcruntime140: Add support for MSVC thread-safe constructor initialization.
      vcruntime140: Add type_info vtbl to the import library.
      libs: Import libc++ from upstream LLVM version 8.0.1.
      libs: Import libunwind from upstream LLVM version 8.0.1.
      libs: Import libc++abi from upstream LLVM version 8.0.1.
      fluidsynth: Import upstream release 2.4.3.
      winegcc: Add default C++ libraries when using msvcrt.

Kun Yang (1):
      comctl32/tests: Test creating an imagelist with an excessively large initial image count.

Louis Lenders (1):
      ole32: Add stub for CoRegisterActivationFilter.

Marc-Aurel Zent (1):
      winemac: Avoid matching scan code 0 in GetKeyNameText.

Michael Green (1):
      winhttp: Implement WINHTTP_OPTION_SERVER_CERT_CHAIN_CONTEXT.

Nello De Gregoris (4):
      server: Allow creating directory kernel objects.
      ntoskrnl.exe/tests: Add tests for directory kernel objects.
      ntoskrnl.exe: Implement FsRtlGetFileSize().
      ntoskrnl.exe/tests: Add tests for FsRtlGetFileSize().

Nikolay Sivov (4):
      mfplay: Remove incorrect fixme for MFP_EVENT_TYPE_ERROR.
      d2d1: Do not reference layer objects when recording to the command list.
      msxml3/sax: Do not attempt to add default attributes for definitions without values.
      dwrite: Handle zero-length name strings.

Paul Gofman (9):
      win32u: Set VendorId / DeviceId in DirectX registry key with correct size.
      shell32: Return S_OK from IShellLinkW_fnSetPath for nonexistent paths.
      shell32: Remove trailing backslash in IShellLinkW_fnSetPath().
      wshom.ocx: Implement WshShortcut_get_TargetPath().
      wshom.ocx: Load shell link in WshShortcut_Create().
      include: Add IMFMediaEngineClassFactoryEx and IMFMediaEngineClassFactory2 definitions.
      mfmediaengine: Add IMFMediaEngineClassFactoryEx interface.
      server: Cache timezone bias adjustment in set_user_shared_data_time().
      comctl32/static: Set HALFTONE StretchBlt mode in STATIC_PaintBitmapfn().

Piotr Caban (3):
      ntdll: Read last error from wow teb in unixlib RtlGetLastWin32Error.
      user32/tests: Test static control extra window data access.
      user32: Make static control internal data private.

Piotr Pawłowski (1):
      comctl32/treeview: Fixed missing redraw of item being unfocused.

Robert Gerigk (4):
      jscript: Treat empty string as NULL in GetScriptDispatch.
      vbscript: Treat empty string as NULL in GetScriptDispatch.
      windowscodecs: Implement IWICFormatConverter for BlackWhite pixel format.
      hidclass.sys: Report correct device type for HID device objects.

Rémi Bernon (5):
      setupapi: Cleanup SetupDiGetClassDescription(Ex)(A|W) code style.
      setupapi: Reimplement SetupDiGetClassDescriptionExW using cfgmgr32.
      setupapi: Forward CM_Get_Device_Interface_List(_Size)(_Ex)(A|W) to cfgmgr32.
      setupapi: Use CM_Locate_DevNode_ExW to allocate devnode.
      setupapi: Use CM_Get_Device_IDW to get devnode device.

Santino Mazza (2):
      msvcrt: Fastfail if _CALL_REPORTFAULT is set.
      msvcrt: Call abort at the end of _wassert.

Stefan Dösinger (5):
      d3d8: Build a valid box in d3d8_vertexbuffer_Lock.
      d3d8: Build a valid box in d3d8_indexbuffer_Lock.
      ddraw/tests: Fix vertex winding in test_ck_rgba.
      ddraw/tests: Fix vertex winding in test_zenable.
      ddraw/tests: Fix vertex winding in test_texture_wrong_caps.

Sven Baars (1):
      winegcc: Remove unused libgcc variable.

Trent Waddington (3):
      cmd: Ensure that for_ctrl->set is non-NULL at parse time.
      findstr/tests: Add tests for start-of-word at start of line.
      findstr: Support start word position \\< at start of line.

Vitaly Lipatov (1):
      taskschd: Implement IRepetitionPattern for trigger objects.

Zhao Yi (2):
      wbemprox: Correct IP address order when calling Win32_NetworkAdapterConfiguration.
      wbemprox/tests: Test the IP address order obtained by calling Win32_NetworkAdapterConfiguration.

Zhiyi Zhang (17):
      wintypes: Return S_OK for api_information_statics_IsMethodPresent().
      ninput/tests: Add tests for INTERACTION_CONTEXT_PROPERTY_INTERACTION_UI_FEEDBACK property.
      ninput: Handle INTERACTION_CONTEXT_PROPERTY_INTERACTION_UI_FEEDBACK property.
      ninput: Add BufferPointerPacketsInteractionContext() stub.
      ninput: Implement getting and setting interaction configuration.
      ninput/tests: Add tests for getting and setting interaction configuration.
      ninput: Add GetStateInteractionContext() stub.
      ninput: Add ProcessBufferedPacketsInteractionContext() stub.
      shcore: Add RecordFeatureUsage() stub.
      icuuc: Add dll.
      icuin: Add dll.
      comctl32/tests: Add more tests for TCS_OWNERDRAWFIXED.
      comctl32/tab: Use a clip region to protect the tab item background.
      icu/tests: Add initial tests.
      user32: Add DelegateInput() stub.
      user32: Add UndelegateInput() stub.
      comctl32/imagelist: Limit the initial image count.

Zhou Qiankang (1):
      ntdll: Avoid expensive inverted DST check on get_timezone_info() cache hit path.
```
