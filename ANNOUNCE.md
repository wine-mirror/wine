The Wine stable release 9.0.1 is now available.

What's new in this release:
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/9.0/wine-9.0.1.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-9.0.1/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 9.0.1 (total 74):

 - #25207  SHFileOperation does not create new directory on FO_MOVE
 - #33050  FDM (Free Download Manager) crashes with page fault when any remote FTP directory opened
 - #44863  Performance regression in Prince of Persia 3D
 - #46070  Basemark Web 3.0 Desktop Launcher crashes
 - #46074  Visio 2013 crashes with unimplemented function msvcr100.dll.??0_ReaderWriterLock@details@Concurrency@@QAE@XZ
 - #48110  Multiple .NET 4.x applications need TaskService::ConnectedUser property (Toad for MySQL Freeware 7.x, Microsoft Toolkit from MS Office 2013)
 - #49089  nProtect Anti-Virus/Spyware 4.0 'tkpl2k64.sys' crashes on unimplemented function 'fltmgr.sys.FltBuildDefaultSecurityDescriptor'
 - #49703  Ghost Recon fails to start
 - #49877  Minecraft Education Edition shows error during install: Fails to create scheduled task
 - #51285  The bmpcoreimage test in user32:cursoricon fails on most Windows versions
 - #51599  cmd.exe incorrectly parses an all-whitespace line followed by a closing parenthesis
 - #51813  python fatal error redirecting stdout to file
 - #51957  Program started via HKLM\Software\Microsoft\Windows\CurrentVersion\App Paths should also be started if extension ".exe" is missing
 - #52399  SIMATIC WinCC V15.1 Runtime installer: SeCon tool fails with error 5 while trying to create 'C:\\windows\Security\\SecurityController' (needs '%windir%\\security')
 - #52622  windows-rs 'lib' test crashes on unimplemented function d3dcompiler_47.dll.D3DCreateLinker
 - #52879  ESET SysInspector 1.4.2.0 crashes on unimplemented function wevtapi.dll.EvtCreateRenderContext
 - #53934  __unDName fails to demangle a name
 - #54759  Notepad++: slider of vertical scrollbar is too small for long files
 - #55000  wineserver crashes below save_all_subkeys after RegUnLoadKey
 - #55268  user32:cursoricon - LoadImageA() fails in test_monochrome_icon() on Windows 8+
 - #55282  Flutter SDK can't find "aapt" program (where.exe is a stub)
 - #55421  Fallout Tactics launcher has graphics glitches
 - #55619  VOCALOID AI Shared Editor v.6.1.0 crashes with System.Management.ManagementObject object construction
 - #55724  mfmediaengine:mfmediaengine sometimes gets a threadpool assertion in Wine
 - #55765  The 32-bit d2d1:d2d1 frequently crashes on the GitLab CI
 - #55810  Finding Nemo (Steam): window borders gone missing (virtual desktop)
 - #55876  Acrom Controller Updater broken due to oleaut32 install
 - #55883  SpeedWave can't draw Window, needs oleaut32.OleLoadPictureFile().
 - #55897  cpython 3.12.0 crashes due to unimplemented CopyFile2
 - #55945  KakaoTalk crashes when opening certain profiles after calling GdipDrawImageFX stub
 - #55997  Dolphin Emulator crashes from 5.0-17264
 - #56054  Microsoft Safety Scanner crashes on exit on unimplemented function tbs.dll.GetDeviceIDString
 - #56062  unimplemented function mgmtapi.dll.SnmpMgrOpen
 - #56078  LibreOffice 7.6.4 crashes on unimplemented function msvcp140_2.dll.__std_smf_hypot3
 - #56093  msys/pacman: fails with "fixup_mmaps_after_fork: VirtualQueryEx failed"
 - #56122  LANCommander won't start, prints "error code 0x8007046C" (ERROR_MAPPED_ALIGNMENT)
 - #56133  explorer.exe: Font leak when painting
 - #56135  Dictionnaire Hachette Multimédia Encyclopédique 98 crashes on start
 - #56139  scrrun: Dictionary does not allow storing at key Undefined
 - #56168  dbghelp hits assertion in stabs_pts_read_type_def
 - #56174  Forza Horizon 4 crashes with concrt140.dll.?_Confirm_cancel@_Cancellation_beacon@details@Concurrency@@QEAA_NXZ
 - #56195  Device name inconsistent casing between GetRawInputDeviceInfo and PnP
 - #56223  winedbg: crashes after loading gecko debug information
 - #56235  Windows Sysinternals Process Explorer 17.05 crashes showing Threads property page.
 - #56243  ShowSystray registry key was removed without alternative
 - #56244  SplashTop RMM client for Atera crashes on unimplemented function shcore.dll.RegisterScaleChangeNotifications
 - #56256  Windows Sysinternals Process Explorer 17.05 shows incomplete user interface (32-bit).
 - #56265  Epic Games Launcher 15.21.0 calls unimplemented function cfgmgr32.dll.CM_Get_Device_Interface_PropertyW
 - #56271  Free Download Manager no longer works after it updated (stuck at 100% CPU, no visible window)
 - #56309  Across Lite doesn't show the letters properly when typing
 - #56334  Page fault when querying dinput8_a_EnumDevices
 - #56357  Zero sized writes using WriteProcessMemory succeed on Windows, but fail on Wine.
 - #56361  Geovision Parashara's Light (PL9.exe) still crashes in wine
 - #56367  Tomb Raider 3 GOG crashes at start
 - #56369  Advanced IP Scanner crashes on unimplemented function netapi32.dll.NetRemoteTOD
 - #56372  musl based exp2() gives very inaccurate results on i686
 - #56400  SSPI authentication does not work when connecting to sql server
 - #56434  WScript.Network does not implement UserName, ComputerName, and UserDomain properties
 - #56493  PresentationFontCache.exe crashes during .Net 3.51 SP1  installation
 - #56498  Incorrect substring expansion for magic variables
 - #56503  CryptStringToBinary doesn't adds CR before pad bytes in some cases
 - #56554  ON1 photo raw installs but wont run the application
 - #56579  Setupapi fails to read correct class GUID and name from INF file containing %strkey% tokens
 - #56582  vb3 combobox regression: single click scrolls twice
 - #56598  Calling [vararg] method via ITypeLib without arguments via IDispatch fails
 - #56599  HWMonitor 1.53 needs unimplemented function pdh.dll.PdhConnectMachineA
 - #56609  vcrun2008 fails to install
 - #56653  GetLogicalProcessorInformation can be missing Cache information
 - #56666  BExAnalyzer from SAP 7.30 does not work correctly
 - #56730  Access violation in riched20.dll when running EditPad
 - #56755  White textures in EverQuest  (Unsupported Conversion in windowscodec/convert.c)
 - #56763  Firefox 126.0.1 crashes on startup
 - #56781  srcrrun: Dictionary setting item to object fails
 - #56871  The 32-bit wpcap program is working abnormally

### Changes since 9.0:
```
Alex Henrie (10):
      tbs: Add GetDeviceIDString stub.
      include: Add mgmtapi.h and LPSNMP_MGR_SESSION.
      mgmtapi: AddSnmpMgrOpen stub.
      msvcp140_2: Implement __std_smf_hypot3.
      where: Implement search with default options.
      explorer: Fix font handle leaks in virtual desktop.
      pdh: Add PdhConnectMachineA stub.
      ntdll/tests: Delete the WineTest registry key when the tests finish.
      ntdll/tests: Rewrite the RtlQueryRegistryValues tests and add more of them.
      ntdll: Succeed in RtlQueryRegistryValues on direct query of nonexistent value.

Alexandre Julliard (16):
      configure: Add /usr/share/pkgconfig to the pkg-config path.
      winsta: Start time is an input parameter in WinStationGetProcessSid.
      oleaut32: Fix IDispatch::Invoke for vararg functions with empty varargs.
      ntdll: Add default values for cache parameters.
      secur32/tests: Update count for new winehq.org certificate.
      gitlab: Add 'build' tag on Linux build jobs.
      gitlab: Remove make -j options.
      dnsapi/tests: Update tests for winehq.org DNS changes.
      urlmon/tests: Fix a test that fails after WineHQ updates.
      wininet/tests: Update issuer check for winehq.org certificate.
      urlmon/tests: Skip test if ftp connection fails.
      user32/tests: Fix some sysparams results on recent Windows.
      dnsapi/tests: Update DNS names for the new test.winehq.org server.
      wininet/tests: Update certificate for the new test.winehq.org server.
      secur32/tests: Update expected results for the new test.winehq.org server.
      winhttp/tests: Allow some more notifications for the new test.winehq.org server.

Alistair Leslie-Hughes (3):
      fltmgr.sys: Implement FltBuildDefaultSecurityDescriptor.
      fltmgr.sys: Create import library.
      ntoskrnl/tests: Add FltBuildDefaultSecurityDescriptor test.

Andrew Nguyen (2):
      oleaut32: Bump version resource to Windows 10.
      ddraw: Reserve extra space in the reference device description buffer.

Bartosz Kosiorek (2):
      gdiplus/tests: Add GdipDrawImageFX tests except effects or attributes.
      gdiplus: Partially implement GdipDrawImageFX.

Benjamin Mayes (1):
      windowscodecs: Add conversions from PixelFormat32bppBGRA->PixelFormat16bppBGRA5551.

Bernhard Übelacker (10):
      server: Avoid unloading of HKU .Default registry branch.
      server: Allow VirtualQueryEx on "limited" handle.
      wing32: Add tests.
      wing32: Avoid crash in WinGGetDIBPointer when called with NULL bitmap info.
      dbghelp: Return early if HeapAlloc failed.
      ntdll: Fix structure layout in RtlQueryProcessDebugInformation for 64-bit.
      wininet: Add missing assignment of return value.
      wininet: Avoid crash in InternetCreateUrl with scheme unknown.
      cmd: Handle lines with just spaces in bracket blocks.
      cmd: Avoid execution if block misses closing brackets.

Dmitry Timoshkov (4):
      oleaut32: Do not reimplement OleLoadPicture in OleLoadPicturePath.
      oleaut32: Factor out stream creation from OleLoadPicturePath.
      oleaut32: Implement OleLoadPictureFile.
      kerberos: Allocate memory for the output token if requested.

Eric Pouech (4):
      dbghelp: Support redefinition of a range statement.
      server: Allow 0-write length in WriteProcessMemory().
      cmd: Add test for substring handling in 'magic' variable expansion.
      cmd: Fix substring expansion for 'magic' variables.

Esme Povirk (1):
      gdiplus: Prefer Tahoma for generic sans serif font.

Fabian Maurer (11):
      win32u: Factor out scroll timer handling.
      win32u: Only set scroll timer if it's not running.
      oleaut32: Add test for invoking a dispatch get-only property with DISPATCH_PROPERTYPUT.
      oleaut32: Handle cases where invoking a get-only property with INVOKE_PROPERTYPUT returns DISP_E_BADPARAMCOUNT.
      riched20: In para_set_fmt protect against out of bound cTabStop values.
      mmdevapi/tests: Add tests for IAudioSessionControl2 GetDisplayName / SetDisplayName.
      mmdevapi/tests: Add tests for IAudioSessionControl2 GetIconPath / SetIconPath.
      mmdevapi/tests: Add tests for IAudioSessionControl2 GetGroupingParam / SetGroupingParam.
      mmdevapi: Implement IAudioSessionControl2 GetDisplayName / SetDisplayName.
      mmdevapi: Implement IAudioSessionControl2 GetIconPath / SetIconPath.
      mmdevapi: Implement IAudioSessionControl2 GetGroupingParam SetGroupingParam.

Felix Münchhalfen (2):
      ntdll: Use pagesize alignment if MEM_REPLACE_PLACEHOLDER is set in flags of NtMapViewOfSection(Ex).
      kernelbase/tests: Add a test for MapViewOfFile3 with MEM_REPLACE_PLACEHOLDER.

Hans Leidekker (13):
      wbemprox: Protect tables with a critical section.
      wbemprox: Handle implicit property in object path.
      netprofm: Support NLM_ENUM_NETWORK flags.
      netprofm: Set return pointer to NULL in networks_enum_Next().
      msi: Install global assemblies before running deferred custom actions.
      msi: Install global assemblies after install custom actions and before commit custom actions.
      wpcap: Handle different layout of the native packet header structure on 32-bit.
      winhttp/tests: Fix test failures introduced by the server upgrade.
      secur32: Handle GNUTLS_MAC_AEAD.
      secur32/tests: Switch to TLS 1.2 for connections to test.winehq.org.
      secur32/tests: Mark some test results as broken on old Windows versions.
      crypt32/tests: Fix a test failure.
      wbemprox: Use separate critical sections for tables and table list.

Helix Graziani (1):
      cfgmgr32: Add CM_Get_Device_Interface_PropertyW stub.

Jinoh Kang (4):
      include: Add definition for FILE_STAT_INFORMATION.
      ntdll/tests: Add tests for NtQueryInformationFile FileStatInformation.
      ntdll: Implement NtQueryInformationFile FileStatInformation.
      kernelbase: Replace FileAllInformation with FileStatInformation in GetFileInformationByHandle().

Kartavya Vashishtha (1):
      kernelbase: Implement CopyFile2().

Louis Lenders (3):
      shcore: Add stub for RegisterScaleChangeNotifications.
      shell32: Try appending .exe when looking up an App Paths key.
      wmic: Support interactive mode and piped commands.

Martin Storsjö (1):
      musl: Fix limiting the float precision in intermediates.

Michael Stefaniuc (2):
      gitlab: Do not run the build script on each commit.
      tools: Get the ANNOUNCE bug list from the stable-notes git notes.

Nicholas Tay (1):
      win32u: Preserve rawinput device instance ID case in add_device().

Nikolay Sivov (10):
      scrrun/dictionary: Add support for hashing VT_EMPTY keys.
      scrrun/dictionary: Add support for hashing VT_NULL keys.
      scrrun/dictionary: Handle VT_EMPTY/VT_NULL keys.
      wshom/network: Use TRACE() for implemented method.
      wshom/network: Implement GetTypeInfo().
      wshom/network: Implement ComputerName() property.
      wshom/network: Check pointer argument in get_UserName().
      wshom/network: Implement UserDomain property.
      d2d1: Fix a double free on error path (Valgrind).
      scrrun/dictionary: Implement putref_Item() method.

Paul Gofman (5):
      explorer: Don't pop start menu on "minimize all windows" systray command.
      explorer: Don't pop start menu on "undo minimize all windows" systray command.
      winhttp: Always return result at once if available in WinHttpQueryDataAvailable().
      winhttp: Always return result at once if available in WinHttpReadData().
      kernel32/tests: Add tests for critical section debug info presence.

Peter Johnson (1):
      wined3d: Added missing GTX 3080 & 1070M.

Piotr Caban (5):
      msvcp140_2: Fix i386 export names.
      msvcp140_2: Implement __std_smf_hypot3f.
      msvcp140_t/tests: Add __std_smf_hypot3 tests.
      concrt140: Add _Cancellation_beacon::_Confirm_cancel() implementation.
      msvcrt: Store _unDName function parameter backreferences in parsed_symbol structure.

Roland Häder (1):
      wined3d: Added missing GTX 1650.

Rémi Bernon (9):
      dinput/tests: Add some IRawGameController2 interface tests.
      windows.gaming.input: Stub IRawGameController2 interface.
      explorer: Restore a per-desktop ShowSystray registry setting.
      secur32/tests: Update the tests to expect HTTP/2 headers.
      imm32/tests: Add todo_himc to some ImmTranslateMessage expected calls.
      urlmon/tests: Expect "Upgrade, Keep-Alive" connection string.
      user32/tests: Add flaky_wine to some SetActiveWindow tests.
      wininet: Parse multi-token Connection strings for Keep-Alive.
      user32/tests: Fix cursoricon tests on recent Windows versions.

Sam Joan Roque-Worcel (1):
      win32u: Make SCROLL_MIN_THUMB larger.

Santino Mazza (1):
      crypt32: Fix CryptBinaryToString not adding a separator.

Tim Clem (1):
      gitlab: Update configuration for the new Mac runner.

Tuomas Räsänen (2):
      setupapi/tests: Add tests for reading INF class with %strkey% tokens.
      setupapi: Use INF parser to read class GUID and class name.

Vijay Kiran Kamuju (10):
      msvcr100: Add _ReaderWriterLock constructor implementation.
      wine.inf: Create security directory.
      concrt140: Add stub for _Cancellation_beacon::_Confirm_cancel().
      taskschd: Implement ITaskService_get_ConnectedUser.
      taskschd: Return success from Principal_put_RunLevel.
      taskschd: Implement TaskService_get_ConnectedDomain.
      d3dcompiler: Add D3DCreateLinker stub.
      netapi32: Add NetRemoteTOD stub.
      mscms: Add stub for WcsGetDefaultColorProfile.
      wevtapi: Add stub EvtCreateRenderContext().

Zebediah Figura (4):
      shell32/tests: Remove obsolete workarounds from test_move().
      ddraw: Use system memory for version 4 vertex buffers.
      ddraw: Upload only the used range of indices in d3d_device7_DrawIndexedPrimitive().
      ddraw/tests: Test GetVertexBufferDesc().

Zhenbo Li (1):
      shell32: Create nonexistent destination directories in FO_MOVE.

Zhiyi Zhang (6):
      include: Rename DF_WINE_CREATE_DESKTOP to DF_WINE_VIRTUAL_DESKTOP.
      server: Inherit internal desktop flags when creating desktops.
      rtworkq: Avoid closing a thread pool object while its callbacks are running.
      rtworkq: Avoid possible scenarios that an async callback could be called twice.
      user32/tests: Add some ReleaseCapture() tests.
      win32u: Only send mouse input in ReleaseCapture() when a window is captured.
```
