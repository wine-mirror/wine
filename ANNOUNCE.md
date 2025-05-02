The Wine development release 10.7 is now available.

What's new in this release:
  - User fault fd support to improve write watches performance.
  - Support for Float format conversions in WindowsCodecs.
  - More work on the new PDB backend.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/10.x/wine-10.7.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-10.7/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 10.7 (total 14):

 - #18803  PokerStars windows disappear on alert
 - #18926  In Winamp, the "send to..." submenu in the playlist menu does not appear
 - #20172  Button "Alt Gr" triggers access violation in Teach2000
 - #31775  Misaligned icons in icon bar
 - #33624  winhelp: Popups appear with bogus scrollbars which disappear when you click them
 - #37706  ScrollWindowEx() returns ERROR if the window is not visible (in the Windows API sense); real Windows returns NULLREGION
 - #38379  Barnham Junction fails to start "Cannot create file C\users\username\Temp\BBC*.tmp\Sim Resources\Barnham Junction\Nameboard.bmp"
 - #48792  HeidiSQL: some icons completely grayed out
 - #50226  Native Access 1.13.5 Setup PC.exe Installer installs infinitely
 - #50851  The procedure entry point RasClearConnectionStatistics could not be located in the dynamic link library RASAPI32.dll
 - #56107  Comdlg32/Color - Cross not painted
 - #57684  Games do not receive keyboard input in virtual desktop mode
 - #58072  LVSCW_AUTOSIZE does not include the size of the state imagelist
 - #58082  Race condition in GlobalMemoryStatusEx() implementation

### Changes since 10.6:
```
Adam Markowski (1):
      po: Update Polish translation.

Akihiro Sagawa (3):
      cmd: Use the OEM code page if GetConsoleOutputCP fails.
      cmd/tests: Add updated code page test in batch file.
      cmd: Use the console output code page to read batch files.

Alex Henrie (2):
      gdi32: Limit source string length in logfont_AtoW (ASan).
      gdi32: Ensure null termination in logfont_AtoW.

Alexander Morozov (3):
      ntoskrnl.exe/tests: Test some Io functions with FDO and PDO.
      ntoskrnl.exe/tests: Test that calling some Io functions does not result in receiving IRP_MN_QUERY_ID.
      ntoskrnl.exe: Fix getting DevicePropertyEnumeratorName.

Alexandre Julliard (25):
      ntdll: Round the virtual heap size to a page boundary.
      kernel32/tests: Don't use _ReadWriteBarrier on ARM platforms.
      configure: Default to MSVC mode with LLVM cross-compilers.
      configure: Move cross-compiler checks before header checks.
      configure: Make the missing PE compiler notice a warning.
      configure: Use a standard pkg-config check for Alsa.
      include: Use pragma push/pop.
      tools: Use pragma push/pop.
      dlls: Use pragma push/pop.
      programs: Use pragma push/pop.
      configure: Re-enable pragma pack warnings.
      urlmon/tests: Run the ftp tests against test.winehq.org.
      wininet/tests: Run the ftp tests against test.winehq.org.
      include: Avoid long types on the Unix side.
      ntdll: Remove redundant casts.
      win32u: Remove redundant casts.
      winex11: Remove redundant casts.
      winemac: Remove redundant casts.
      wineandroid: Remove redundant casts.
      winewayland: Remove redundant casts.
      tools: Fix tracing of empty strarray.
      tools: Use booleans where appropriate.
      tools: Generate syscall macros directly with the right offset.
      tools: Add a platform-independent name for the ALL_SYSCALLS macros.
      ntdll: Add a test for invalid syscall numbers.

Alexandros Frantzis (3):
      winewayland: Support building with older EGL headers.
      winewayland: Always check the role to determine whether a surface is a toplevel.
      winewayland: Introduce helper to check whether a surface is toplevel.

Attila Fidan (1):
      winewayland: Require wl_pointer for pointer constraints.

Aurimas Fišeras (2):
      po: Update Lithuanian translation.
      po: Update Lithuanian translation.

Bernhard Übelacker (4):
      ntoskrnl.exe/tests: Remove unused function pointers.
      spoolss: Avoid buffer-overflow when setting numentries (ASan).
      d3d11/tests: Add broken to test_nv12.
      gdi32/tests: Remove one test for NtGdiMakeFontDir.

Billy Laws (2):
      ntdll: Allow mem{cpy,move} optimisation now -fno-builtins is used.
      ntdll: Check arm64ec TEB frames are valid before popping them.

Brendan McGrath (3):
      mf: Update state and start clock for both paused and stopped.
      mf: Reset audio client on flush.
      winegstreamer: Handle the Stream Group Done event.

Brendan Shanks (2):
      win32u: Enter font_lock in NtGdiMakeFontDir.
      win32u: Raise realized font handle limit to 5000.

Charlotte Pabst (1):
      mfplat/tests: Don't assume video processor MFT can provide samples.

Dmitry Timoshkov (3):
      user32/tests: Add more ScrollWindowEx() tests.
      win32u: Fix return value of ScrollWindowEx() for invisible windows.
      windowscodecs: Also initialize FlipRotator.bpp field.

Elizabeth Figura (4):
      wined3d/glsl: Move clip distance enabling to shader_glsl_apply_draw_state().
      wined3d/glsl: Move GL_FRAMEBUFFER_SRGB application to shader_glsl_apply_draw_state().
      wined3d: Move SRGB write enable to wined3d_extra_ps_args.
      wined3d: Move the clip plane mask to wined3d_extra_vs_args.

Eric Pouech (9):
      dbghelp: Move typedef handling to the new PDB backend.
      dbghelp: Add user field to function and inline sites.
      dbghelp: Move reading inlinee line number information to PDB backend.
      dbghelp: Directly store compiland's name in symt_compiland.
      dbghelp: Use symref_t to describe a symbol's container.
      cmd: Factor out code_page when searching for a label.
      conhost: Add support for ESC in win32 edit mode.
      conhost: Handle ctrl-break unconditionally.
      conhost: Handle ctrl-c from unix console in ReadConsoleW + control.

Esme Povirk (7):
      oleaut32: Use apartment-less WIC.
      oleaut32: Copy palette from WIC source for indexed formats.
      gdiplus: Limit clip region calculation to device rectangle.
      gdiplus: Don't trace old values in GdipSetMatrixElements.
      appwiz.cpl: Report addon download failures.
      comctl32: Implement EVENT_OBJECT_STATECHANGE for progress control.
      comctl32: Implement EVENT_OBJECT_VALUECHANGE for progress bars.

Francisco Casas (1):
      d2d1: Compile shaders on device creation instead of device context creation.

Gabriel Ivăncescu (9):
      mshtml: Rename struct constructor to stub_constructor.
      mshtml: Consolidate the functional constructors into a common struct implementation.
      mshtml: Define the constructor's prototype on mshtml side.
      mshtml: Define "create" from XMLHttpRequest constructor as a jscript prop in IE9+ modes.
      mshtml: Return proper string from functional constructors' toString in IE9+ modes.
      mshtml: Store the object_id of the last object in the prototype chain that contains the needed prop.
      mshtml: Validate builtin host functions in IE9+ using prototype_id rather than tid where possible.
      mshtml: Use designated initializers for the Location dispex data.
      mshtml: Remove unused struct mutation_observer_ctor.

Hans Leidekker (3):
      odbc32: Fix replicating unixODBC data sources.
      include: Fix typos in exclusiveto attributes.
      msv1_0: Drop the ntlm_auth check.

Jinoh Kang (4):
      Revert "kernel32/tests: Don't use _ReadWriteBarrier on ARM platforms."
      kernel32/tests: Don't use _ReadWriteBarrier() on clang.
      kernel32/tests: Run store_buffer_litmus_test() in a single-iteration loop.
      kernel32/tests: Shorten time for negative half of litmus test for FlushProcessWriteBuffers().

Keno Fischer (1):
      ntdll: Make server requests robust to spurious short writes.

Louis Lenders (1):
      combase: Add stub for RoOriginateErrorW.

Marc-Aurel Zent (4):
      server: Store process base priority separately.
      server: Use process base priority in set_thread_base_priority.
      ntdll: Implement ProcessBasePriority class in NtSetInformationProcess.
      ntdll/tests: Add tests for setting process base priority.

Michael Stefaniuc (2):
      maintainers: Remove myself as the Stable maintainer.
      dmsynth: Don't report an underrun when current equals write position.

Mohamad Al-Jaf (4):
      windows.media.mediacontrol: Fix a memory leak.
      windows.devices.enumeration: Guard against WindowsDuplicateString() failure.
      windows.system.profile.systemid/tests: Add ISystemIdentificationInfo::get_Id() tests.
      windows.system.profile.systemid: Implement ISystemIdentificationInfo::get_Id().

Nikolay Sivov (23):
      include: Add newer winhttp option constants.
      comctl32/tests: Add a column width test for LVSCW_AUTOSIZE with a state imagelist.
      comctl32/listview: Use state icon width when autosizing columns.
      winedump: Fix a crash in 'dump' command.
      include: Change the schannel.h guard name.
      include: Add WINHTTP_SECURITY_INFO type.
      windowscodecs/tests: Remove A->W test data conversion.
      windowscodecs/tests: Add some tests for encoder info.
      windowscodecs: Fix JPEG encoder information strings.
      windowscodecs: Fix TIFF encoder information strings.
      shell32: Simplify error handling when FolderItemVerbs object is created.
      shell32: Fix use-after-free at FolderItemVerbs creation (ASan).
      windows.ui/tests: Remove tests for exact color values.
      uiautomationcore: Fix BSTR buffer overrun (ASan).
      comdlg32/colordlg: Fix color picker cursor painting.
      windowscodecs/png: Fix byte-swapping mode usage in the encoder.
      windowscodecs/tests: Add a test for big-endian TIFF image data handling.
      windowscodecs/tiff: Remove unnecessary image data byte-swaping.
      windowscodecs/converter: Add 24bppBGR -> 128bppRGBAFloat conversion path.
      windowscodecs/converter: Add 32bppBGRA - > 128bppRGBAFloat conversion path.
      windowscodecs/converter: Add 128bppRGBAFloat -> 32bppBGRA conversion path.
      windowscodecs/converter: Add 96bppRGBFloat -> 128bppRGBFloat conversion path.
      windowscodecs/converter: Add 96bppRGBFloat -> 32bppBGRA conversion path.

Pali Rohár (2):
      win87em: Fix __FPMATH symbol name.
      krnl386: Set carry flag for unimplemented DPMI 0800h call (Physical Address Mapping).

Paul Gofman (6):
      opengl32: Don't distinguish WGL_SWAP_EXCHANGE_ARB and WGL_SWAP_UNDEFINED_ARB when filtering in wglChoosePixelFormatARB().
      kernel32/tests: Add more tests for write watches.
      ntdll: Use UFFD for write watches support if available.
      wbemprox: Implement Win32_CacheMemory table.
      netapi32: Fix service names in NetStatisticsGet().
      ntdll: Make sure NT flag is not set before iretq in wine_syscall_dispatcher_return on x86-64.

Piotr Caban (18):
      msvcr110/tests: Link to msvcr110.
      msvcr70/tests: Link to msvcr70.
      msvcr71/tests: Link to msvcr71.
      include: Add _FCbuild() declaration.
      include: Add vsscanf declaration.
      include: Add function declarations used in msvcr120 tests.
      msvcr120/tests: Link to msvcr120.
      msvcrt: Fix memory leaks in create_locinfo.
      makefiles: Use -fno-builtin for CRT tests.
      msvcr80/tests: Link to msvcr80.
      include: Add functions used by msvcr90 tests.
      msvcr90/tests: Link to msvcr90.
      msvcrt: Avoid dynamic allocation when storing locale name.
      msvcrt: Use LC_MAX constant in create_locinfo.
      secur32/tests: Make NTLM server challenge blob human-readable.
      secur32/tests: Use one copy of server challenge reply in NTLM tests.
      secur32/tests: Fix NTLM tests on Windows 11 by accepting NTLMv2 in test server response.
      secur32/tests: Don't accept NTLMv1 type 3 message in NTLM tests.

Rastislav Stanik (1):
      kernelbase: Fix race condition in GlobalMemoryStatusEx().

Rémi Bernon (12):
      d3d9/tests: Skip some d3d12 tests instead of crashing.
      win32u: Use the driver_funcs interface for osmesa pixel formats.
      win32u: Add an opengl_driver_funcs entry to implement wglGetProcAddress.
      opengl32: Generate error messages in null functions.
      opengl32: Pass null GL funcs to __wine_get_wgl_driver.
      win32u: Add procedure loading to generic OpenGL code.
      win32u: Add a generic wglSwapBuffers implementation.
      wineandroid: Use the generic wglSwapBuffers implementation.
      winemac: Use the generic wglSwapBuffers implementation.
      winewayland: Use the generic wglSwapBuffers implementation.
      winex11: Use the generic wglSwapBuffers implementation.
      win32u: Add nulldrv swap_buffers implementation.

Stefan Dösinger (3):
      odbc32: Don't call wcslen in debugstr_sqlwstr.
      odbc32: Retlen may be NULL in SQLGetData.
      msvcrt: Add truncf to the import library.

Tim Clem (1):
      comctl32: Track initial taskdialog layout on a per-dialog basis.

Yuxuan Shui (1):
      winegstreamer: Make sure WMSyncReader never reads in the background.
```
