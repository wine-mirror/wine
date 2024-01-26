The Wine development release 9.1 is now available.

What's new in this release:
  - A number of Input Method improvements.
  - Improved Diffie-Hellman key support.
  - Better Dvorak keyboard detection.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/9.x/wine-9.1.tar.xz>

Binary packages for various distributions will be available
from <https://www.winehq.org/download>

You will find documentation on <https://www.winehq.org/documentation>

Wine is available thanks to the work of many people.
See the file [AUTHORS][1] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/raw/wine-9.1/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 9.1 (total 42):

 - #17414  user32/dde test crashes if +heap enabled
 - #25759  Polda 1: after intro picture and animation it shows black window
 - #35300  Lego Racers crashes when click on configuration commands for Player 1
 - #36007  oleaut32/vartype tests crash with WINEDEBUG=warn+heap
 - #42784  Lost Planet dx10 demo black screen after starting new game
 - #46074  Visio 2013 crashes with unimplemented function msvcr100.dll.??0_ReaderWriterLock@details@Concurrency@@QAE@XZ
 - #46904  SIMATIC WinCC V15.1 Runtime: Automation License Manager 'almapp64x.exe' crashes on unimplemented function msvcp140.dll.?_XGetLastError@std@@YAXXZ
 - #50297  Blindwrite 7 crashes with a stack overflow
 - #50475  ENM (Externes Notenmodul / external mark module) crashes on opening
 - #50893  Wine cannot see home directory (32-bit time_t overflow)
 - #51285  The bmpcoreimage test in user32:cursoricon fails on most Windows versions
 - #51471  user32:input receives unexpected WM_SYSTIMER messages in test_SendInput()
 - #51473  user32:input Some SendInput() set LastError to ERROR_ACCESS_DENIED on cw-rx460 19.11.3
 - #51474  user32:input SendInput() triggers an unexpected message 0x60 on Windows 10 1709
 - #51477  user32:input test_Input_blackbox() gets unexpected 00&41(A) keystate changes
 - #51931  Dead Rising encounters infinite loading when starting a new game (needs WMAudio Decoder DMO)
 - #52399  SIMATIC WinCC V15.1 Runtime installer: SeCon tool fails with error 5 while trying to create 'C:\\windows\Security\\SecurityController' (needs '%windir%\\security')
 - #52595  GUIDE 7.0 shows black screen on start
 - #53516  user32:input failed due to unexpected WM_TIMECHANGE message
 - #54089  user32:input - test_SendInput() sometimes gets an unexpected 0x738 message on w1064v1709
 - #54223  Unigine Heaven Benchmark 4.0 Severely Low FPS
 - #54323  user32:input - test_SendInput() sometimes gets an unexpected 0xc042 message on Windows 7
 - #54362  BurnInTest calls unimplemented function ntoskrnl.exe.ExAllocatePool2
 - #55000  wineserver crashes below save_all_subkeys after RegUnLoadKey
 - #55268  user32:cursoricon - LoadImageA() fails in test_monochrome_icon() on Windows 8+
 - #55467  MAME 0.257: mame.exe -listxml crashes
 - #55835  putenv clobbers previous getenv
 - #55883  SpeedWave can't draw Window, needs oleaut32.OleLoadPictureFile().
 - #55945  KakaoTalk crashes when opening certain profiles after calling GdipDrawImageFX stub
 - #56054  Microsoft Safety Scanner crashes on exit on unimplemented function tbs.dll.GetDeviceIDString
 - #56055  AVG Antivirus setup crashes on unimplemented function ADVAPI32.dll.TreeSetNamedSecurityInfoW
 - #56062  unimplemented function mgmtapi.dll.SnmpMgrOpen
 - #56078  LibreOffice 7.6.4 crashes on unimplemented function msvcp140_2.dll.__std_smf_hypot3
 - #56093  msys/pacman: fails with "fixup_mmaps_after_fork: VirtualQueryEx failed"
 - #56119  Emperor - Rise of the Middle Kingdom: invisible menu buttons
 - #56135  Dictionnaire Hachette Multimédia Encyclopédique 98 crashes on start
 - #56168  dbghelp hits assertion in stabs_pts_read_type_def
 - #56174  Forza Horizon 4 crashes with concrt140.dll.?_Confirm_cancel@_Cancellation_beacon@details@Concurrency@@QEAA_NXZ
 - #56195  Device name inconsistent casing between GetRawInputDeviceInfo and PnP
 - #56223  winedbg: crashes after loading gecko debug information
 - #56235  Windows Sysinternals Process Explorer 17.05 crashes showing Threads property page.
 - #56236  notepad freezes when displaying child dialog

### Changes since 9.0:
```
Aida Jonikienė (2):
      opengl32: Add a FIXME when doing a mapped buffer copy.
      localspl: Fix a maybe-uninitialized warning in fill_builtin_form_info().

Alex Henrie (8):
      uiautomationcore/tests: Use CRT allocation functions.
      tbs: Add GetDeviceIDString stub.
      advapi32: Add TreeSetNamedSecurityInfoW stub.
      include: Add mgmtapi.h and LPSNMP_MGR_SESSION.
      mgmtapi: AddSnmpMgrOpen stub.
      msvcp140_2: Implement __std_smf_hypot3.
      include: Add POOL_FLAGS and POOL_FLAG_*.
      ntoskrnl: Reimplement ExAllocatePool* on top of ExAllocatePool2.

Alexandre Julliard (33):
      ntdll/tests: Add exception test for int 2d on x86-64.
      user32: Return result through NtCallbackReturn for the DDE message callback.
      user32: Return result through NtCallbackReturn for the thunk lock callback.
      user32: Return result through NtCallbackReturn for the copy image callback.
      user32: Return result through NtCallbackReturn for the load image callback.
      user32: Return result through NtCallbackReturn for the load sys menu callback.
      user32: Return result through NtCallbackReturn for the draw text callback.
      user32: Return result through NtCallbackReturn for the enum monitors callback.
      user32: Return result through NtCallbackReturn for the window hook callback.
      winevulkan: Return result through NtCallbackReturn for the debug callbacks.
      wineandroid.drv: Return result through NtCallbackReturn for the start device callback.
      winex11.drv: Return result through NtCallbackReturn for the drag and drop callbacks.
      winemac.drv: Return result through NtCallbackReturn for the drag and drop callbacks.
      user32: Return a proper NTSTATUS in the load driver callback.
      user32: Return a proper NTSTATUS in the post DDE message callback.
      user32: Return a proper NTSTATUS in all user callbacks.
      opengl32: Return a proper NTSTATUS in the debug callback.
      ntdll: Add NtCompareTokens syscall for ARM64EC.
      msvcp: Consistently use __int64 types in number conversion functions.
      include: Add a typedef for user callback function pointers.
      ntdll: Share KiUserCallbackDispatcher implementation across platforms.
      ntdll: Report failure in KiUserCallbackDispatcher when catching an exception.
      ntdll: Export KiUserCallbackDispatcherReturn.
      ntdll: Use a .seh handler for KiUserCallbackDispatcher exceptions.
      ntdll: Move the process breakpoint to the CPU backends.
      ntdll: Use a .seh handler for the process breakpoint.
      ntdll/tests: Update todos in context tests for new wow64 mode.
      configure: Only check for libunwind on x86-64.
      ntdll: Share the nested exception handler across platforms.
      ntdll: Use a .seh handler for nested exceptions.
      ntdll: Clear CONTEXT_UNWOUND_TO_CALL in signal frames.
      ntdll/tests: Port the exception unwinding tests to ARM64.
      ntdll/tests: Port the exception unwinding tests to ARM.

Alistair Leslie-Hughes (2):
      include: Add more D3D_FEATURE_LEVEL_ defines.
      include: Correct KMTQAITYPE values.

Aurimas Fišeras (1):
      po: Update Lithuanian translation.

Bartosz Kosiorek (2):
      gdiplus/tests: Add GdipDrawImageFX tests except effects or attributes.
      gdiplus: Partially implement GdipDrawImageFX.

Bernhard Übelacker (6):
      wing32: Add tests.
      wing32: Avoid crash in WinGGetDIBPointer when called with NULL bitmap info.
      server: Allow VirtualQueryEx on "limited" handle.
      dbghelp: Return early if HeapAlloc failed.
      ntdll: Fix structure layout in RtlQueryProcessDebugInformation for 64-bit.
      server: Avoid unloading of HKU .Default registry branch.

Biswapriyo Nath (5):
      include: Add D3D12_FEATURE_DATA_VIDEO_ENCODER_RESOURCE_REQUIREMENTS in d3d12video.idl.
      include: Add D3D12_FEATURE_DATA_VIDEO_ENCODER_RESOLUTION_SUPPORT_LIMITS in d3d12video.idl.
      include: Add D3D12_VIDEO_ENCODER_ENCODE_ERROR_FLAGS in d3d12video.idl.
      include: Add D3D12_FEATURE_DATA_VIDEO_ENCODER_SUPPORT in d3d12video.idl.
      include: Add missing macros in devenum.idl.

Brendan McGrath (3):
      gdi32: Ignore Datatype when StartDoc is called.
      d2d1: Use 24-bit FP precision for triangulate.
      d2d1: Fix double free bug when d2d_geometry_sink_Close fails.

Brendan Shanks (5):
      opengl32: Make wglSwapLayerBuffers hookable.
      combase: Make RoGetActivationFactory hookable.
      wined3d: Update reported AMD driver version.
      ntdll: Remove unnecessary NtQueryVirtualMemory call.
      ntdll: Only build the main module and ntdll once on Wow64.

Daniel Hill (3):
      winex11.drv: Dvorak should use QWERTY scancodes.
      winex11.drv: Improve DetectLayout heuristics.
      winex11.drv: Add Dvorak with phantom keys layout.

Daniel Lehman (2):
      msvcp120/tests: Add some tests for _Mtx_t fields.
      msvcp140: Pad _Mtx_t struct to match Windows.

David Kahurani (1):
      gdiplus: Avoid use of temporary variable.

Dmitry Timoshkov (4):
      ntdll: Add NtCompareTokens() stub.
      oleaut32: Do not reimplement OleLoadPicture in OleLoadPicturePath.
      oleaut32: Factor out stream creation from OleLoadPicturePath.
      oleaut32: Implement OleLoadPictureFile.

Dāvis Mosāns (1):
      ntdll/tests: Test NtContinue on x86-64.

Eric Pouech (7):
      dbghelp: Support redefinition of a range statement.
      winedbg: Make some internal data 'static const'.
      winedbg: Print all pid and tid with 4 hex characters.
      appwiz.cpl: Load dynamically wine_get_version().
      user32: Load dynamically wine_get_version().
      include: Avoid defining intrinsic functions as inline.
      include: Avoid redefining _InterlockedCompareExchange128 as inline.

Etaash Mathamsetty (2):
      xinput: Implement XInputGetCapabilitiesEx.
      xinput: Reimplement XInputGetCapabilities.

Fabian Maurer (8):
      dmsynth: Leave critical section when out of memory (Coverity).
      localspl: In fpScheduleJob leave critical section in error case (Coverity).
      wmiutils: Always zero path->namespaces in parse_text (Coverity).
      winedbg: Add missing break inside fetch_value (Coverity).
      wow64win: Add missing break inside packed_result_32to64 (Coverity).
      winegstreamer: Don't check event for NULL, gstreamer already does that.
      include: Add Windows.UI.ViewManagement.InputPane definitions.
      windows.ui: Add stubs for InputPane class.

Fan WenJie (1):
      wined3d: Compile sm1 bytecode to spirv.

Gabriel Brand (3):
      ws2_32/tests: Test binding UDP socket to invalid address.
      server: Return failure in bind if the address is not found.
      kernel32: Add string for WSAEADDRNOTAVAIL error.

Gabriel Ivăncescu (6):
      msvcirt: Use proper operator_new and operator_delete types.
      jscript: Move thread_id from JScript struct to TLS data.
      jscript: Don't use atomic compare exchange when setting the script ctx.
      jscript: Make the garbage collector thread-wide rather than per-ctx.
      jscript: Allow garbage collection between different jscript contexts.
      mshtml: Implement document.lastModified.

Giovanni Mascellani (2):
      wined3d: Expose the image view usage for null views.
      wined3d: Expose the image view usage for non-default views.

Haidong Yu (1):
      loader: Associate folder with explorer.

Hans Leidekker (19):
      bcrypt: Add support for setting DH parameters.
      bcrypt: Add support for retrieving DH parameters.
      bcrypt: Allow or disallow some operations based on whether keys are finalized.
      bcrypt: Add helpers to create a public/private key pair.
      bcrypt: Make DH blob size validation more strict in key_import_pair().
      bcrypt: Reject DH keys smaller than 512 bits.
      bcrypt: Add support for generating DH keys from known parameters.
      bcrypt: Make sure key_asymmetric_derive_key() returns correct size.
      bcrypt: Assume we have a public key in key_export_dh_public().
      bcrypt: Set dh_params in key_import_dh/_public().
      bcrypt/tests: Add DH tests.
      crypt32: Pad R/S values with zeroes if smaller than their counterpart.
      sxs: Use wide character string literals.
      sxs/tests: Use wide character string literals.
      sxs/tests: Get rid of workarounds for old Windows versions.
      sxs/tests: Update QueryAssemblyInfo() test for Windows 10.
      sxs: Skip file copy when assembly is already installed.
      bcrypt: Fix private data size in wow64 thunks.
      dssenh: Finalize the hash if necessary in CPVerifySignature().

Jacek Caban (4):
      winevdm: Use char type for max length assignment.
      kernelbase: Silence -Wsometimes-uninitialized clang warning.
      kernelbase: Silence -Warray-bounds clang warning.
      devenum: Use switch statements for moniker type handling.

Jinoh Kang (2):
      ntdll/tests: Avoid misaligned load in exception handler code in run_exception_test_flags().
      ntdll/tests: Restore x86-64 #AC exception test in test_exceptions().

Martin Storsjö (6):
      ntdll: Fix KiUserCallbackDispatcher on arm.
      ntdll: Reduce fixme logging for large numbers of cores.
      ntdll: Remove libunwind support for aarch64.
      ntdll: Remove libunwind support for ARM.
      ntdll: Remove dwarf unwinding support for aarch64.
      wineps.drv: Avoid invalid unaligned accesses.

Nicholas Tay (1):
      win32u: Preserve rawinput device instance ID case in add_device().

Nikolay Sivov (13):
      mf/tests: Skip tests if video renderer can't be created.
      ntdll: Update RTL_HEAP_PARAMETERS definition.
      ntdll/tests: Add some tests for creating custom heaps.
      scrrun/dictionary: Add support for hashing VT_EMPTY keys.
      scrrun/dictionary: Add support for hashing VT_NULL keys.
      scrrun/dictionary: Handle VT_EMPTY/VT_NULL keys.
      evr/dshow: Handle YUY2 sample copy.
      mf/tests: Fully cleanup when skipping tests.
      d2d1/tests: Add some tests for minimum/maximum input count in effect description.
      d2d1/effect: Handle variable input count attributes in the description.
      d2d1/effect: Use XML description for builtin effects.
      d2d1/effect: Recreate transform graph when input count changes.
      d3d10/effect: Use bitfields for numeric type descriptions.

Paul Gofman (5):
      ntdll: Fix exception list offset in call_user_mode_callback / user_mode_callback_return.
      ntdll: Return STATUS_DEBUGGER_INACTIVE from NtSystemDebugControl() stub.
      winex11.drv: Fix wglSwapBuffers() with NULL current context with child window rendering.
      winhttp: Always return result at once if available in WinHttpQueryDataAvailable().
      winhttp: Always return result at once if available in WinHttpReadData().

Piotr Caban (8):
      msvcp140_2: Fix i386 export names.
      msvcp140_2: Implement __std_smf_hypot3f.
      msvcp140_t/tests: Add __std_smf_hypot3 tests.
      concrt140: Add _Cancellation_beacon::_Confirm_cancel() implementation.
      msvcp140: Add _XGetLastError implementation.
      msvcp140/tests: Fix _Syserror_map(0) test failure in newest msvcp140.
      msvcp140: Recognize no error case in _Syserror_map.
      winex11.drv: Fix xim_set_focus no IC condition check.

Russell Greene (1):
      powrprof: Add PowerWriteACValueIndex stub.

Rémi Bernon (59):
      user32/tests: Remove old Windows versions broken cursoricon results.
      user32/tests: Fix cursoricon tests on recent Windows versions.
      user32/tests: Add flaky_wine to some SetActiveWindow tests.
      user32/tests: Run SendInput tests in a separate desktop.
      user32/tests: Cleanup SendInput keyboard message sequence tests.
      user32/tests: Test SendInput messages with KEYEVENTF_SCANCODE flag.
      user32/tests: Test SendInput messages with other keyboard layouts.
      imm32/tests: Add todo_himc to some ImmTranslateMessage expected calls.
      dinput/tests: Make some failing keyboard test flaky_wine.
      vulkan/tests: Add gitlab Win10 VM results.
      winex11: Return STATUS_NOT_FOUND when IME update isn't found.
      win32u: Move ImeToAsciiEx implementation from winex11.
      win32u: Support posting IME updates while processing keys.
      winemac: Use the default ImeToAsciiEx implementation.
      win32u: Remove now unnecessary ImeToAsciiEx driver entry.
      winebus: Append is_gamepad to the device instance id.
      winebus: Allow specific devices to prefer hidraw backend.
      winebus: Move device identification helpers to unixlib.h.
      winebus: Prefer hidraw backends for DS4 and DS5 gamepads.
      winexinput: Demote BusContainerId FIXME message to WARN.
      winebus: Demote BusContainerId FIXME message to WARN.
      dinput: Add a description to the dinput worker thread.
      windows.gaming.input: Add a description to the monitor thread.
      include: Add HEAACWAVEINFO and HEAACWAVEFORMAT definitions.
      mfplat/tests: Test MFInitMediaTypeFromWaveFormatEx wrt MF_MT_FIXED_SIZE_SAMPLES.
      mfplat/tests: Add MFInitMediaTypeFromWaveFormatEx tests with HEAACWAVEFORMAT.
      mfplat/tests: Test MFWaveFormatExConvertFlag_ForceExtensible with HEAACWAVEFORMAT.
      mfplat: Support AAC format attributes in MFInitMediaTypeFromWaveFormatEx.
      mfplat: Support compressed WAVEFORMATEX in MFCreateWaveFormatExFromMFMediaType.
      win32u: Avoid truncating ToUnicodeEx result if there's room.
      user32/tests: Move KEYEVENTF_UNICODE to test_SendInput_keyboard_messages.
      user32/tests: Test that WH_KEYBOARD_LL are blocking SendInput.
      setupapi: Don't clobber the original filename if .inf is found.
      dinput/tests: Introduce a new helper to create a foreground window.
      dinput/tests: Enforce ordering of concurrent read IRPs.
      dinput/tests: Add a test with a virtual HID mouse.
      dinput/tests: Add a test with a virtual HID keyboard.
      include: Add more HID digitizer usage definitions.
      dinput/tests: Add a test with a virtual HID touch screen.
      winex11: Sync with gdi_display before closing the threads display.
      dinput/tests: Differentiate missing from broken HID reports.
      dinput/tests: Relax the mouse move count test.
      dinput/tests: Add some IRawGameController2 interface tests.
      windows.gaming.input: Stub IRawGameController2 interface.
      imm32/tests: Adjust todo_wine for the new Wine CJK keyboard layouts.
      imm32: Mask the scancode before passing it to ImeToAsciiEx.
      imm32/tests: Test that WM_KEYUP are passed to ImeProcessKey.
      win32u: Also pass WM_KEYUP messages to ImmProcessKey.
      imm32/tests: Test the effect of CPS_CANCEL and CPS_COMPLETE.
      imm32: Complete the composition string when the IME is closed.
      user32/tests: Add an optional hwnd to input messages tests.
      user32/tests: Cleanup the mouse input WM_NCHITTEST / SetCapture tests.
      user32/tests: Run the mouse hook tests in the separate desktop.
      user32/tests: Filter the ll-hook messages with accept_message.
      user32/tests: Test clicking through attribute-layered windows.
      user32/tests: Tests clicking through window with SetWindowRgn.
      winegstreamer: Fix reading MF_MT_USER_DATA into HEAACWAVEFORMAT.
      winegstreamer: Use MFCreateAudioMediaType in the AAC decoder.
      winegstreamer: Use an array for the audio decoder input types.

Sven Baars (2):
      advapi32/tests: Introduce a new has_wow64 helper.
      advapi32/tests: Skip WoW64 tests on 32-bit in test_reg_create_key.

Tim Clem (1):
      winemac.drv: Detect active handwriting and panel IMEs.

Tyson Whitehead (2):
      dinput/tests: Update tests for DIPROP_AUTOCENTER.
      dinput: Implement DIPROP_AUTOCENTER.

Vijay Kiran Kamuju (5):
      concrt140: Add stub for _Cancellation_beacon::_Confirm_cancel().
      wine.inf: Create security directory.
      msvcp140: Add stub for _XGetLastError.
      include: Add Windows.Storage.Streams.InMemoryRandomAccessStream runtimeclass definition.
      msvcr100: Add _ReaderWriterLock constructor implementation.

Yuxuan Shui (4):
      dmime: AudioPathConfig is not AudioPath.
      dmime: Parse AudioPathConfig.
      dmime: IDirectMusicPerformance::CreateAudioPath should fail when config is NULL.
      dmime: Semi-support creating an audio path from config.

Zebediah Figura (14):
      ddraw/tests: Add tests for map pointer coherency.
      ddraw: Sync to sysmem after performing a color fill.
      ddraw: Use the sysmem wined3d texture for sysmem surfaces if possible.
      wined3d: Hook up push constants for Vulkan.
      wined3d/spirv: Hook up sm1 interface matching.
      d3d11: Implement D3D11_FEATURE_D3D11_OPTIONS2.
      wined3d: Report VK_EXT_shader_stencil_export availability to vkd3d_shader_compile().
      wined3d: Implement shader stencil export for GL.
      d3d11: Report support for shader stencil export if available.
      d3d11/tests: Add a test for shader stencil export.
      wined3d: Check the wined3d resource type and usage in find_ps_compile_args().
      wined3d: Set the tex_type field of the FFP fragment settings from the resource's GL type.
      wined3d: Check for WINED3DUSAGE_LEGACY_CUBEMAP instead of checking the GL texture target.
      wined3d: Collapse some trivially nested ifs into a single condition.

Zhiyi Zhang (16):
      wldap32: Fix a possible memory leak (Coverity).
      msi: Fix a memory leak (Coverity).
      bcrypt: Fix an possible out-of-bounds read (Coverity).
      win32u: Fix a possible out-of-bounds write (Coverity).
      compstui: Fix a possible out-of-bounds write (Coverity).
      user32/tests: Add recursive keyboard and mouse hook tests.
      win32u: Avoid calling WH_KEYBOARD and WH_CBT HCBT_KEYSKIPPED hooks recursively.
      win32u: Avoid calling WH_CBT HCBT_CLICKSKIPPED hooks recursively.
      user32/tests: Add recursive WM_SETCURSOR message tests.
      include: Add some ncrypt definitions.
      include: Add some bcrypt definitions.
      ncrypt/tests: Test default RSA key properties.
      ncrypt: Add some missing RSA key properties.
      ncrypt/tests: Add NCryptExportKey() tests.
      user32/tests: Test keyboard layout in CJK locales.
      win32u: Don't set the high word of keyboard layout to 0xe001 in CJK locales.
```
