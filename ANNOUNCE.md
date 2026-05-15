The Wine development release 11.9 is now available.

What's new in this release:
  - Bundled SQLite library.
  - Initial support for system threads.
  - Thread suspension in emulated code on ARM64.
  - More VBScript compatibility improvements.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/11.x/wine-11.9.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-11.9/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 11.9 (total 24):

 - #36484  Lotus Notes 8.x installer aborts with SAX parser exception (line breaks not preserved)
 - #53317  Logos 9: Crash When Indexing: Invalid String Format
 - #53551  Unable to run a hello world console program: unhandled page fault
 - #53637  WinSCP UI rendering issue
 - #53877  vbscript compile_assignment assertion when assigning multidimensional array by indices
 - #54177  vbscript fails to compile sub call when argument expression contains multiplication
 - #56281  vbscript: String number converted to ascii value instead of parsed value
 - #57852  rebase (tool from the Windows SDK) crashes on unimplemented function imagehlp.dll.ReBaseImage64
 - #58125  Homesite 5.5 installer's progress bar misses its outline
 - #58925  GOM Player: UI elements not responding to mouse clicks
 - #59011  Wargaming Game Center Window not appearing / invisible
 - #59028  Graphpad Prism 9: project (.pzfx) files cannot be saved if msxml6 is not installed
 - #59409  msvcrt: scanf scanset character ranges with high bytes (\x80-\xff) broken due to signed char comparison in scanf.h
 - #59425  GXSCC crashes when a valid MIDI file is dragged in its window
 - #59611  ExamDiff Pro Fileeditor blocks on end of file
 - #59672  winhttp: DOAXVV fails with error 9003 at title screen
 - #59689  SEC_WINNT_AUTH_IDENTITY_EX support in AcquireCredentialsHandle
 - #59690  Both Command & Conquer 3 and Command & Conquer Red Alert 3 show the same kind of error
 - #59708  d3d9: missing MSVC vtable byte-pattern
 - #59718  d2d1: Three correctness issues in arc Bezier-approximation code (d2d_arc_to_bezier, d2d_figure_add_arc, d2d_arc_transform)
 - #59722  RemoveDirectoryW fails with ERROR_SHARING_VIOLATION after subprocess exit
 - #59729  Volumes do not report FILE_SUPPORTS_OPEN_BY_FILE_ID
 - #59743  regression: Wine crashes / segmentation fault's when starting Photoshop CS 2
 - #59746  SteelSeries GG 110.0 crashes on startup in .NET System.Security.Cryptography.X509Certificates.CertificateRequest..ctor

### Changes since 11.8:
```
Alexandre Julliard (36):
      ntdll: Add a helper function to create a thread on the server side.
      ntdll: Add a helper function to create a thread with pthread.
      ntdll: Move single-step handling to the SIGTRAP handler.
      ntdll: Don't allow getting/setting the context for threads without a TEB.
      ntdll: Don't try to return from syscall for threads without a TEB.
      ntdll: Handle signals in thread without a TEB.
      ntdll: Don't raise an exception for threads without a TEB.
      ntdll: Support last error functions in threads without a TEB.
      ntdll: Pass the thread data explicitly in a few more functions.
      ntdll: Store the initial suspend flag in the thread data.
      ntdll: Move all the thread initialization into server_init_thread().
      ntdll: Export PsCreateSystemThread.
      server: Add support for system threads.
      server: Don't return system threads in enumerations.
      server: Don't allow accessing the context of system threads.
      server: Don't send debug events for system threads.
      server: Fix crash on invalid thread handle.
      ntdll: Return explicit load order when checking version heuristics.
      ntdll: Add builtin heuristics for Twain dll.
      winemac: Use pthread instead of the TEB to access per-thread data.
      winex11: Use pthread instead of the TEB to access per-thread data.
      ntdll: Always get the 64-bit TEB from the 32-bit one.
      ntdll: Pass the thread data to get_cpu_area().
      ntdll: Add a helper function to initialize a CLIENT_ID structure.
      winealsa.drv: Move zero_bits initialization to the wow64 attach callback.
      winecoreaudio.drv: Move zero_bits initialization to the wow64 attach callback.
      wineoss.drv: Move zero_bits initialization to the wow64 attach callback.
      winepulse.drv: Move zero_bits initialization to the wow64 attach callback.
      kernel32/tests: Adapt sort order test for Japanese and Korean.
      kernel32/tests: Remove a sorting test that depends on Windows version.
      kernel32/tests: Fix some geo name tests on latest Windows.
      kernelbase: Don't store invalid geo names in the registry.
      kernelbase: Implement GetGeoInfo(GEO_NAME).
      kernelbase: Implement GetGeoInfo(GEO_RFC1766).
      kernelbase: Implement GetGeoInfo(GEO_LCID).
      kernelbase: Partially implement GetGeoInfo(GEO_FRIENDLYNAME).

Alistair Leslie-Hughes (5):
      propsys: Only trace Properties that cannot be found.
      include: Add struct SHARDAPPIDINFOLINK.
      include: Add JOB_STATUS_RESTART define.
      include: Add define WC_NETADDRESS.
      msado15: Correctly increase the cnt value.

Aurimas Fišeras (1):
      po: Update Lithuanian translation.

Bernhard Übelacker (4):
      windows.ui.core.textinput/tests: Move Release below check_interface.
      kernelbase: Run conhost.exe from c:\windows\system32 directory.
      ntdll: Use debugstr_wn instead of debugstr_w in nt_to_unix_file_name.
      winex11.drv: Use hwnd_drop instead of hwnd in x11drv_dnd_drop_event.

Brendan McGrath (2):
      mf: Remove periodic callback on clock shutdown.
      mf: Call GetCorrelatedTime without critical section.

Brendan Shanks (3):
      ntdll: Always synthesize ESR in SIGTRAP handler on ARM64 Linux.
      ntdll/tests: Test unaligned access on ARM64.
      ntdll: Add constants/helpers, and be more explicit when parsing the ARM64 ESR.

Conor McCarthy (12):
      mf/tests: Test optional topology nodes.
      mf/topology_loader: Resolve topologies by walking down from each source node.
      mf/topology_loader: Try to insert optional nodes after resolving the surrounding branch.
      mf/tests: Add asynchronous transform tests.
      mf/session: Support asynchronous transforms.
      mf/tests: Test for preroll support in the sample grabber.
      mf/tests: Test preroll notification when the rate has not been set.
      mf/session: Initialise the stored presentation rate.
      mf/scheme_handler: Call PathCreateFromUrlW() for URL to path conversion.
      mfplat/tests: Create the new file in the temp path in test_file_stream().
      mfplat/tests: Test leading forward slashes and colons in filename URLs.
      mfplat: Normalise URLs before passing to BeginCreateObject().

Dan Ginovker (1):
      d3d9: Add a fake d3d9 device vtbl initialization sequence.

Dmitry Timoshkov (4):
      wineboot: Run services.exe from c:\windows\system32 directory.
      adsldp: Add support for ADS_OPTION_REFERRALS.
      crypt32: Use case-insensitive comparison for registered OID names.
      adsldp: Add support for ADS_SEARCHPREF_CHASE_REFERRALS.

Esme Povirk (2):
      win32u: Support OBJID_CLASSNAMEIDX in scrollbar controls.
      wbemprox: Stub Win32_DiskDrive TotalHeads and Signature properties.

Francis De Brabandere (35):
      vbscript/tests: Add coverage for BSTR vs numeric comparison.
      vbscript/tests: Cover BSTR comparison with VT_BYREF and additional numeric VTs.
      vbscript/tests: Cover BSTR comparison literal vs non-literal dispatch.
      vbscript: Match native BSTR-vs-numeric and BSTR-vs-Boolean comparison.
      vbscript/tests: Cover BSTR comparison with dispatch-only numeric VTs.
      vbscript: Reject Automation types not supported in VBScript on compare.
      vbscript/tests: Cover bare match.SubMatches(N) indexing.
      vbscript: Implement DateValue and TimeValue functions.
      vbscript/tests: Cover non-literal numeric vs whitespace-only and control-char BSTR.
      vbscript: Treat all-whitespace and control-char BSTR as greater than numeric.
      vbscript: Return subscript-out-of-range from UBound/LBound on uninitialized array.
      vbscript: Return Null from Day/Month/Year/Hour/Minute/Second on Null operand.
      vbscript: Handle Null operands in Left().
      vbscript: Return the script class name from TypeName() on a class instance.
      oleaut32: Fix Null handling in VarAnd.
      oleaut32: Fix VT_CY Null handling in VarImp.
      oleaut32: Fix VT_DATE Null handling in VarImp.
      vbscript: Match native UI1 Imp Null behavior.
      vbscript/tests: Add Null handling smoke tests for logical operators.
      vbscript/tests: Cover Dim/Sub shadowing of host members.
      vbscript: Coerce VT_EMPTY operands before Var* calls.
      vbscript: Support assignment to chained array index expressions.
      vbscript: Implement DateDiff built-in function.
      vbscript/tests: Cover empty parens on properties, Dim variables, and arrays.
      vbscript: Raise illegal-func-call on empty parens of class variant property.
      vbscript: Delegate do_icall is_call branch to variant_call.
      vbscript/tests: Cover top-level Const behavior.
      vbscript/tests: Cover cross-parse name redefinition semantics.
      vbscript: Match native cross-parse name redefinition rules.
      vbscript: Implement IDispatch::GetTypeInfo for class instances via ICreateTypeLib2.
      vbscript: Fix crash on fixed-size Dim array inside Execute called from a local scope.
      vbscript/tests: Add tests for BSTR vs numeric/bool with neither side literal.
      vbscript: Treat non-literal BSTR as greater than non-literal numeric/bool.
      vbscript/tests: Add tests for &H8000 literal type.
      vbscript: Parse &H8000 as Long(-32768) like native.

Georg Lehmann (1):
      winevulkan: Update to VK spec version 1.4.350.

Giang Nguyen (3):
      d2d1: Fix coordinate in arc-to-bezier conversion for degenerate case.
      d2d1: Fix arc center translation in arc-to-bezier conversion.
      d2d1: Do not translate radius vectors in arc-to-bezier conversion.

Hans Leidekker (3):
      winhttp: Don't buffer more than one chunk.
      winhttp: Always queue async writes.
      msv1_0: Handle SEC_WINNT_AUTH_IDENTITY_EX.

Ivan Ivlev (3):
      winmm/tests: Use non-static struct to avoid clang compilation errors.
      vccorlib140/tests: Use non-static objects to avoid clang compilation errors.
      dmsynth/tests: Use macro instead of static function to avoid clang compilation errors.

Jacek Caban (25):
      advapi32/tests: Remove unused variable.
      kernel32/tests: Avoid unused variables.
      server: Store the client page size in the process struct.
      ntdll/tests: Avoid unused global variables in exception.c.
      ntdll/tests: Avoid unused global variables in wow64.c.
      ntdll/tests: Avoid unused global variables.
      gitlab: Update to llvm-mingw 20260505.
      msvcrt: Provide trunc in the import library.
      keyboard.drv: Remove unused global variables.
      mouse.drv: Remove unused global variable.
      win87em: Remove unused global variables.
      comctl32/tests: Avoid unused global variables.
      localui/tests: Remove unused global variables.
      secur32/tests: Directly use secur32 functions in main.c.
      kerberos: Remove unused variable.
      msv1_0: Remove unused variables.
      kernel32/tests: Avoid unused global variables on 32-bit ARM target.
      mountmgr.drv: Avoid unused global variable.
      ntdll: Avoid unused global variables.
      ntdll/tests: Avoid unused global variables in wow64.c.
      ntoskrnl/tests: Remove unused variable.
      windows.media.speech/tests: Remove unused variables.
      ntdll: Add support for ARM64EC cooperative suspend.
      ntdll: Use unwind_context values only if the caller is not EC in capture_context.
      ntdll/tests: Add SuspendDoorbell tests.

Joel Holdsworth (4):
      ntdll/tests: Add test for FILE_SUPPORTS_OPEN_BY_FILE_ID in FileFsAttributeInformation.
      ntdll: Report FILE_SUPPORTS_OPEN_BY_FILE_ID.
      kernel32/tests: Add test for FILE_SUPPORTS_OPEN_BY_FILE_ID on volume handles.
      mountmgr: Report FILE_SUPPORTS_OPEN_BY_FILE_ID.

Jon Koops (2):
      dsound/tests: Test primary buffer independence across IDirectSound objects.
      dsound: Create independent devices for each DirectSoundCreate call.

Luca Bacci (1):
      msvcrt: Set errno in failure path.

Matteo Bruni (3):
      winmm: Fix voicecom IMMDevice leak in device enumeration.
      winmm/tests: Add some DRVM_MAPPER_CONSOLEVOICECOM_GET tests.
      winmm/tests: Check that the preferred device is always at index 0.

Mykola Mykhno (2):
      cmd: Update tests for parentheses in quoted parameter.
      cmd: Remove parentheses from list of illegal characters for quote strip.

Nikolay Sivov (7):
      msxml3: Fix missing document output in save().
      msxml3/tests: Add some tests for cloning textual nodes.
      msxml3: Clone textual contents when cloning nodes.
      msxml3: Handle baseName property for entity references.
      msxml3/tests: Remove some refcount tests.
      msxml3/tests: Add a basic test for replaceChild() on attributes.
      msxml3/tests: Add some tests for selecting from detached nodes.

Paul Gofman (16):
      wininet/tests: Add a test for multiple server addresses.
      wininet: Pass proxy server addr_str if present in open_http_connection().
      wininet: Store server address related data in the new server_addr_t structure.
      wininet: Enclose ipv6 address string in square brackets.
      wininet: Introduce and use create_connect_socket().
      wininet: Try to connect to multiple server addresses.
      wininet: Do not force ipv4 addresses.
      ws2_32/tests: Properly check for loopback address in test_gethostbyname().
      ws2_32: Exclude not found and loopback addresses in get_local_ips() if others present.
      win32u: Bump D3DKMT driver version.
      ddraw/tests: Don't leak window and ddraw in test_d3d_state_reset() on ddraw2.
      ddraw/tests: Add tests for clipper in exclusive fullscreen mode.
      ddraw: Do not render to the clipping window in exclusive fullscreen mode.
      winsqlite3: Import sqlite3 from upstream release 3.51.1.
      include: Add winsqlite3.h.
      winsqlite3/tests: Add a basic test.

Piotr Pawłowski (2):
      comctl32/tests: Print id field of logged/expected messages on failure.
      uxtheme/tests: Print id field of logged/expected messages on failure.

Rémi Bernon (5):
      mf/topology_loader: Get rid of the topoloader_context.
      mf/topology_loader: Clone and validate topology in a dedicated helper.
      mf/topology_loader: Copy branches with cloned nodes before connecting.
      mf/topology_loader: Add some sanity checks for optional output nodes.
      mf/topology_loader: Force enumerate types when optional node method is set.

Sameer Singh (समीर सिंह) (2):
      gdi32/uniscribe: Perform bounds check in insert_glyph().
      gdi32/uniscribe: Resize the buffers if glyph count exceeds limit.

Sven Baars (1):
      wine: Show all dlopen() error messages instead of just the last one.

Twaik Yont (5):
      wineandroid: Add reply_fd plumbing to ioctl path.
      wineandroid: Switch ioctl dispatch to socket transport on main thread looper.
      wineandroid: Switch dequeueBuffer to socket transport.
      wineandroid: Remove unixlib/ntoskrnl ioctl path.
      wineandroid: Return ioctl errors directly.

Yuxuan Shui (1):
      preloader: Account for ld.so stack usage when reserving.

Zakaria Habri (1):
      winewayland: Use wp_pointer_warp_v1 for SetCursorPos when available.

Zhiyi Zhang (2):
      include: Add D2D1_TURBULENCE_NOISE.
      include: Add IDCompositionDevice3.
```
