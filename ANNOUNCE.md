The Wine development release 11.7 is now available.

What's new in this release:
  - Beginnings of MSXML reimplementation without libxml2.
  - VBScript compatibility fixes and optimizations.
  - SRGB filter support in D3DX.
  - 7.1 speaker configuration in DirectSound.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/11.x/wine-11.7.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-11.7/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 11.7 (total 35):

 - #26226  msxml4 emits too much xmlns attributes
 - #33118  Adding bin.base64 attribute causes duplicate datatype attribute
 - #38350  FTDI Vinculum II IDE V2.0.2-SP2 gets OLE Error 80020006 and exits, native msxml3 is workaround
 - #49029  ABBYY FineReader 12 Professional crashes in trial mode
 - #52607  build on Cygwin fails, error: undefined references to _alldiv, _allmul, _allrem, _aulldiv, _aullrem (etc.) in dlls/ntdll
 - #54236  Default to Windows 10 when creating a prefix
 - #54291  vbscript stuck in endless for loop when UBound on Empty and On Error Resume Next
 - #54344  simtower (setup.exe) says could not open the file name "h:\wine64....."
 - #55031  experimental wow64 mode: crashes in some 3d graphics code
 - #55093  vbscript: if boolean condition should work without braces
 - #55196  vbscript: Trailing End If
 - #56480  vbscript: underscore line continue issues
 - #56931  vbscript: Const used before declaration fails (explicit)
 - #58026  vbscript: Script running error when Dictionary contains array
 - #58051  vbscript: Dictionary direct Keys/Items access causes parse error
 - #58056  vbscript: Directly indexing a Split returns Empty
 - #58392  Can't display background and characters in MapleStory World.
 - #58398  vbscript: For each function with Split a empty string
 - #58673  Kinco Dtools fails to start: err:msxml:doparse Unsupported encoding: gb2312
 - #58802  Inserting XML document fragment adds the fragment instead of its content.
 - #59415  In Falsus demo cannot be interacted with
 - #59446  Latest VOCALOID6 version (6.11) crashes after VOCALOID:AI track
 - #59512  HID devices report wrong DeviceType causing GetFileType() to return FILE_TYPE_DISK
 - #59516  Songbookpro crashes with RoFailFastWithErrorContext()
 - #59517  IWICFormatConverter does not support BlackWhite as destination pixel format
 - #59528  Fade In Pro: Crashes immediately after splash screen
 - #59534  Cabinet.dll missing Compressor interfaces
 - #59558  HTTP 200 response body empty since Wine 11.5 regression (worked in 11.4)
 - #59572  Stratego (1997) shows a black screen on startup
 - #59578  Act of War: Direct Action crashes while loading the main menu
 - #59613  case 64 is missing in key_asymmetric_verify function
 - #59627  Kakaowork crashes at start.
 - #59632  VC_redist fails to start (regression)
 - #59643  SHCreateStreamOnFileW (IStream::Seek) truncates 64-bit offsets to 32-bit, breaking >4GB file reads
 - #59658  Xara Designer Pro + crashes:  Call from 00006FFFFFFA4E45 to unimplemented function icuuc.dll.udata_setCommonData_70, aborting

### Changes since 11.6:
```
Adam Markowski (2):
      po: Update Polish translation.
      po: Update Polish translation.

Akihiro Sagawa (5):
      quartz/tests: Add IMediaEvent(Ex)::WaitForCompletion tests.
      quartz: Reset the completion event on paused -> running transition.
      quartz: Don't leak the completion event handle.
      quartz/tests: Add more WaitForCompletion tests.
      quartz: Fix WaitForCompletion behavior when default handling is canceled.

Alex Henrie (1):
      kernelbase: Ignore GENERIC_WRITE in CreateFile on CDs on Windows 9x.

Alexandre Julliard (9):
      configure: Stop using external libxml/libxslt.
      shell32/tests: Remove todo from test that succeeds now.
      ntdll: For the native heuristics, assume that dll without version is not from Microsoft.
      opengl32: Link to pthread library for pthread_once.
      mf/tests: Fix 64-bit printf formats.
      shcore: Store the app user model id in the process parameters.
      configure: Require the Mingw compiler on Cygwin.
      tools: Remove Cygwin platform and treat it as Mingw.
      Revert "msi/tests: Add regression tests for table stream padding and mismatched string refs."

Andrey Gusev (2):
      d3d11: Fix a memory leak in d3d_video_decoder_create().
      d3dcompiler: Fix misplaced parentheses.

Antoine Leresche (1):
      kernelbase: Add message string for ERROR_NOT_A_REPARSE_POINT.

Anton Baskanov (5):
      dsound: Replace multiplications by fir_step and fir_width with bit shifts.
      dsound: Use a 0.32 fixed point to represent the resampling ratio.
      dsound: Make rem_num signed.
      dsound: Calculate opos_num and ipos_num incrementally.
      dsound: Calculate rem and rem_inv incrementally.

Aurimas Fišeras (3):
      po: Update Lithuanian translation.
      po: Update Lithuanian translation.
      po: Update Lithuanian translation.

Bartosz Kosiorek (1):
      gdiplus/tests: Fix flaky rounding error in test_getblend.

Brendan McGrath (21):
      amstream/tests: Test when top-down image is not accepted.
      amstream: Reject filter when top-down image is not accepted.
      amstream: Always return S_OK on disconnect.
      iyuv_32/tests: Fix a read overrun.
      winegstreamer: Add winegstreamer_create_color_converter.
      iyuv_32: Implement IYUV_GetInfo.
      iyuv_32: Implement IYUV_DecompressQuery.
      iyuv_32: Implement IYUV_DecompressGetFormat.
      iyuv_32: Implement IYUV_Open.
      iyuv_32: Implement IYUV_DecompressBegin.
      iyuv_32: Implement IYUV_Decompress.
      amstream/tests: Test QueryAccept when connected.
      amstream/tests: Test negative heights in QueryAccept.
      amstream: Reject negative heights in QueryAccept.
      amstream: Accept additional subtypes in QueryAccept.
      amstream/tests: Test AcceptQuery behavior after SetFormat.
      amstream: Only accept format passed in SetFormat.
      amstream/tests: Test media types when ddraw is passed to AddMediaStream.
      amstream: Use DisplayMode to determine pixel format.
      amstream/tests: Test media type is only on the first of each sample.
      amstream: Only provide MediaType on first retrieval.

Brendan Shanks (3):
      kernelbase: In GetProcessInformation(ProcessMachineTypeInfo), use SystemSupportedProcessorArchitectures2.
      kernelbase: Implement GetMachineTypeAttributes().
      kernelbase/tests: Add tests for GetMachineTypeAttributes().

Chris Denton (1):
      bcryptprimitives: Ensure ProcessPrng fills the whole buffer.

Connor McAdams (11):
      d3dx10/tests: Add tests for decoding DXTn DDS files.
      d3dx11/tests: Add tests for decoding DXTn DDS files.
      d3dx9: Properly handle DXT textures with premultiplied alpha.
      d3dx10/tests: Add a test for invalid image load filter flags.
      d3dx11/tests: Add a test for invalid image load filter flags.
      d3dx10: Only use passed in filter flags if image scaling is necessary.
      d3dx11: Only use passed in filter flags if image scaling is necessary.
      d3dx9/tests: Add tests for D3DX_FILTER_SRGB flags.
      d3dx10/tests: Add tests for SRGB formats and filter flags.
      d3dx11/tests: Add tests for SRGB formats and filter flags.
      d3dx: Handle SRGB filter flags.

Conor McCarthy (8):
      mf/topology_loader: Return specific error on indirect connection failure.
      mf/topology_loader: Load the down connection method in topology_branch_connect().
      mf/topology_loader: Introduce a helper to clone a media type.
      mf/topology_loader: Use the color converter for video conversion by default.
      gdiplus: Refactor SOFTWARE_GdipFillRegion() to separate bitmap handling.
      gdiplus: Implement software region fill for bitmap images.
      gdiplus: Bypass span combination for intersected rects.
      winegstreamer: Free the streams in unknown_inner_Release().

Daniel Lehman (1):
      msxml3/tests: Test encoding of special characters.

Elizabeth Figura (2):
      ntdll/tests: Test a symlink that unwinds past the directory it's in.
      ntdll: Unwind the Unix path if necessary when processing relative symlinks.

Eric Pouech (5):
      winedbg: Don't proceed with some 'info' commands on non active targets.
      winedbg: Introduce helpers to fetch thread name.
      winedbg: Let 'backtrace' do something when attached to a minidump.
      winedbg: Let 'info thread' work when debugging a minidump.
      dbghelp/dwarf: Store btBool constant leaves as signed integer.

Esme Povirk (7):
      mscoree: Use coop-aware thread attach functions.
      gdiplus/tests: Make the FromGdiDib tests more thorough.
      gdiplus/tests: Test FromGdiDib with a top-down DIB.
      gdiplus/tests: Check more structure types with FromGdiDib.
      gdiplus/tests: Test more image formats with FromGdiDib.
      gdiplus: Rewrite GdipCreateBitmapFromGdiDib.
      gdiplus: Assume bitmaps are top-down in image-reading code.

Francis De Brabandere (45):
      vbscript: Fix error character positions for Exit and Dim statements.
      vbscript: Return "Illegal assignment" error for Const and function assignment.
      vbscript: Restrict identifier characters to ASCII-only.
      vbscript: Handle vertical tab and form feed as whitespace in lexer.
      vbscript: Use ASCII-only case folding in check_keyword.
      vbscript: Add vbs_wcsicmp and use it in interpreter.
      vbscript: Replace wcsicmp with vbs_wcsicmp in compiler, globals, and dispatch.
      vbscript: Fast-path stack_pop_bool for VT_BOOL.
      vbscript: Defer For loop control variable assignment until all expressions are evaluated.
      vbscript: Return proper error for undefined variables with Option Explicit.
      vbscript: Fix For loop getting stuck when expression evaluation fails with On Error Resume Next.
      vbscript: Reject arguments on Class_Initialize and Class_Terminate.
      vbscript: Add missing compiler error constants, messages, and tests.
      vbscript: Return specific errors for mismatched End keywords in blocks.
      vbscript: Return specific errors for unclosed parens, multiple defaults, and default on Property Let/Set.
      vbscript: Support chained call syntax like dict.Keys()(i).
      vbscript: Reject 'Set Me' with error 1037 instead of crashing.
      vbscript: Return error 1054 for Property Let/Set without arguments.
      vbscript: Return error 1005 for missing opening parenthesis in Sub/Function.
      vbscript: Implement Filter function.
      vbscript: Return specific error codes for integer constant and Property declarations.
      vbscript: Return DISP_E_TYPEMISMATCH when indexing non-array variables.
      vbscript: Return error 1048 for Property declaration outside a Class.
      scrrun: Fix BSTR length off-by-one in create_folder.
      scrrun: Implement IFolder::get_ParentFolder.
      scrrun: Implement IFile::get_ParentFolder.
      scrrun: Implement IFolder::get_IsRootFolder.
      scrrun: Implement IFolder::get_Attributes and IFolder::put_Attributes.
      scrrun: Implement IFolder and IFile date getters.
      vbscript: Return correct error codes for missing statement separators.
      vbscript: Silence FuncRef::QueryInterface(IID_IDispatchEx) warning.
      vbscript: Fix crash when GetRef is called as a statement.
      vbscript: Add missing runtime error constants and tests.
      vbscript: Return proper error for New on undefined or non-class identifier.
      vbscript: Reject identifiers longer than 255 characters.
      vbscript: Support bracketed identifiers like [my var].
      vbscript: Return error 1028 for invalid keyword after 'Do'.
      vbscript: Return error 1047 for wrong 'End' keyword inside class.
      vbscript: Return error 1051 for inconsistent property argument counts.
      vbscript: Return proper error for wrong number of arguments.
      wscript: Implement error messages, usage output, and //nologo banner.
      vbscript: Add call depth limit to prevent stack overflow on infinite recursion.
      vbscript: Return specific error codes for missing keywords in parser.
      vbscript: Use indexed lookup for global functions.
      vbscript: Use indexed lookup for global variables.

Hans Leidekker (11):
      winhttp/tests: Fix test failures on old Windows versions.
      winhttp/tests: Fix a test failure.
      winhttp: Rename request_state.
      winhttp: Remove unused arguments from read_more_data().
      winhttp: Start the first chunk right after receiving the headers.
      winhttp/tests: Add a fully recursive asynchronous test.
      winhttp: Increase recursion limit.
      winhttp: Stub WinHttpSetOption(WINHTTP_OPTION_IPV6_FAST_FALLBACK).
      winhttp: Stub WinHttpQueryOption(WINHTTP_OPTION_HTTP_VERSION).
      winhttp: Stub WinHttpQueryOption(WINHTTP_OPTION_CONNECT_RETRIES).
      winhttp: Read as much data as possible in WinHttpReadData().

Jacek Caban (14):
      widl: Do not override name prefix settings from pragmas in imported modules.
      include: Don't use ns_prefix in windows.graphics.directx.direct3d11.interop.idl.
      opengl32: Move GL_NUM_EXTENSIONS to client side.
      opengl32: Remove unexposed extensions for functions registry.
      opengl32: Rename ALL_GL_CLIENT_EXTS to ALL_GL_EXTS.
      opengl32: Store extension_array as opengl_extension.
      opengl32: Implement glGetStringi(GL_EXTENSIONS) on client side.
      opengl32: Initialize enabled / disabled OpenGL extensions struct.
      opengl32: Use parse_extensions in filter_extensions.
      opengl32: Store core GL version separate from extensions in the function registry.
      opengl32: Store extensions an an enum in functions registry.
      opengl32: Introduce get_function_entry helper.
      opengl32: Always check available extensions in wrap_wglGetProcAddress.
      opengl32: Implement wglGetProcAddress on the client side.

Jinoh Kang (2):
      ntdll: Do not fail with STATUS_INVALID_INFO_CLASS for anonymous files.
      Revert "kernelbase: Replace FileAllInformation with FileStatInformation in GetFileInformationByHandle().".

Katharina Bogad (7):
      makedep: Fix -Wdiscarded-qualifier warnings with recent glibc.
      winebuild: Fix -Wdiscarded-qualifier warnings with recent glibc.
      winegcc: Fix -Wdiscarded-qualifier warnings with recent glibc.
      ntdll: Fix -Wdiscarded-qualifier warnings with recent glibc.
      winebth.sys: Fix -Wdiscarded-qualifier warnings with recent glibc.
      winex11.drv: Fix -Wdiscarded-qualifier warnings with recent glibc.
      include: Fix warnings with C23 and GLIBC.

Louis Lenders (1):
      wine.inf: Add HKCU\Software\Classes key.

Marc-Aurel Zent (7):
      win32u: Introduce new ImeToAsciiEx user driver call.
      win32u: Move IME processing to ImeToAsciiEx.
      winemac: Implement and use macdrv_ImeToAsciiEx().
      winemac: Move macdrv_ImeProcessKey logic to ImeProcessKey and macdrv_ImeToAsciiEx.
      win32u: Remove builtin WINE_IME_PROCESS_KEY driver call.
      winemac: Replace OSAtomic functions with builtin __atomic counterparts.
      winemac: Relax builtin __atomic memory barriers.

Matteo Bruni (1):
      configure: Use alsa library search paths in the alsa library check.

Mohamad Al-Jaf (2):
      dsound: Support 7.1 speaker config.
      winecfg: Support 7.1 speaker config.

Nikolay Sivov (41):
      odbc32: Add SQLGetStmtOption() -> SQLGetStmtAttr() fallback for a few options.
      odbc32: Add SQLSetStmtOption() -> SQLSetStmtAttr() for a few options.
      msxml3/tests: Add some more xml() tests.
      msxml3/tests: Add more text() tests.
      msxml3/tests: Add some more tests for whitespace handling.
      msxml3/tests: Add some more tests for namespace handling.
      msxml3/tests: Add some tests for doctype node.
      msxml3/tests: Add more tests for the XmlDecl PI.
      msxml3/tests: Add a couple of SAX parsing tests.
      msxml3: Rework DOM API.
      msxml3/tests: Add some more appendChild() tests with fragments.
      msxml3/tests: Add some tests for attribute value normalization.
      msxml3/tests: Add a setAttribute() test for setting namespace definition.
      msxml3: Fix setAttribute() for elements with unspecified namespace uri.
      msxml3/tests: Add some more tests for getNamedItem().
      msxml3: Do not try to convert NULL strings when creating libxml2 document representation.
      msxml3: Remove duplicated SafeArrayUnaccessData() in load().
      msxml3/tests: Add another test for subtree serialization with default namespaces.
      msxml3/tests: Add a simple test for loading with gb2312 specified encoding.
      msxml3: Release temporary document.
      shcore: Handle 64-bit position in file stream Seek().
      msxml3: Remove unused field from a node structure.
      msxml3: Implement parentNode() property for DTD nodes.
      msxml3: Implement ownerDocument() property for DTD nodes.
      msxml3: Implement nodeTypeString() for DTD nodes.
      msxml3/tests: Add some tests for ProhibitDTD property.
      msxml3: Handle ProhibitDTD property.
      msxml3/tests: Add more splitText() tests.
      msxml3: Unify splitText() implementation.
      msxml3: Unify deleteData() implementation.
      msxml3: Unify substringData() implementation.
      msxml3: Unify length() property implementation for textual nodes.
      msxml3: Unify insertData() implementation.
      msxml3: Unify replaceData() implementation.
      msxml3/tests: Run formatted output tests on the main thread.
      msxml3: Remove redundant check in cloneNode().
      msxml3/tests: Add more tests for setAttribute().
      msxml3: Improve check for new attribute collision with element namespace.
      msxml3: Make namespace definitions added with setAttribute() read-only.
      msxml3/tests: Add another test for insertBefore().
      msxml3/tests: Check for supported interfaces on a document object.

Paul Gofman (5):
      winmm/tests: Add tests for destination scaling with AVI window resize.
      mciavi32: Update destination rect on window size change.
      mciavi32: Use msvfw32 for presentation.
      wmvcore/tests: Add test for COM initialization in async reader thread.
      wmvcore: Initialize COM in async_reader_callback_thread().

Piotr Caban (14):
      msvcrt: Add _vwprintf_l implementation.
      msvcrt: Add _wprintf_l implementation.
      odbc32: Use SQLEndTran() in SQLTransact if needed.
      odbc32: Add odbc v3 test driver and use it in SQLConnect tests.
      odbc32: Use test driver in SQLDriverConnect tests.
      odbc32: Use test driver in SQLBrowseConnect tests.
      odbc32: Use test driver in SQLExecDirect tests.
      odbc32: Fix SQLBrowseConnect tests.
      odbc32: Use SQLFreeHandle in SQLFreeStmt if possible.
      odbc32: Only set connection and login timeout if it was ever set.
      odbc32: Don't call into driver in SQLGetEnvAttr.
      odbc32: Don't call into driver in SQLSetEnvAttr.
      odbc32: Implement sharing environment handle between connections.
      odbc32: Don't leak connection object if it's reused in connect functions.

Robert Gerigk (3):
      include: Add identity string length constants to appmodel.h.
      shcore: Implement Set/GetCurrentProcessExplicitAppUserModelID.
      shcore/tests: Add tests for Set/GetCurrentProcessExplicitAppUserModelID.

Rémi Bernon (44):
      opengl32: Move some unsupported extensions to unexposed.
      opengl32: Add support for some unregistered extensions.
      opengl32: Don't expose unsupported or unknown extensions.
      mf/topology_loader: Disconnect the node after indirect connection failed.
      mf/topology_loader: Allocate indirect connection branches dynamically.
      mf/topology_loader: Cache the media type handlers on the branches.
      mf/topology_loader: Always try setting current up type when connecting directly.
      mf/topology_loader: Avoid making redundant connection method attempts.
      ntdll: Remove unused namepos variable.
      ntdll: Fix unixlib extension position.
      mf/topology_loader: Lookup best media type when connecting converter / decoders.
      mf/topology_loader: Don't force MFTEnumEx output info either for converters.
      mf/topology_loader: Move current type check into topology_branch_foreach_up_types.
      mf/topology_loader: Enumerate and select downstream type in connect_direct.
      mf/topology_loader: Only enumerate transform outputs if no current type is set.
      mf/topology_loader: Update media types while enumerating up types.
      winex11: Use unsigned long for monitor indices generation.
      cfgmgr32: Implement CM_Get_DevNode_Property(_Ex)(A|W).
      setupapi: Forward CM_Get_DevNode_Property(_Ex)(A|W) to cfgmgr32.
      cfgmgr32: Implement CM_Get_DevNode_Property_Keys(_Ex).
      cfgmgr32: Implement CM_Get_Device_ID_List(_Size)(_Ex)(A|W).
      setupapi: Forward CM_Get_Device_ID_List(_Size)(_Ex)(A|W) to cfgmgr32.
      cfgmgr32: Move remaining stubs from setupapi.
      cfgmgr32: Fix querying unnamed properties.
      opengl32: Check for GL_EXT_memory_object_fd before filtering extensions.
      mfplat: Don't set frame size or stride if missing from VIDEOINFOHEADER2.
      mfplat: Support FORMAT_MFVideoFormat in MFInitMediaTypeFromAMMediaType.
      mf/tests: Check resampler IMFTransform / IMediaObject interop.
      mf/tests: Check color converter IMFTransform / IMediaObject interop.
      opengl32: Move major / minor version to the PE side context wrapper.
      opengl32: Implement wglGetExtensionsString(ARB|EXT) on the PE side.
      win32u: Query every OpenGL functions on initialization.
      opengl32: Don't generate extra params in function pointers.
      opengl32: Get rid of make_opengl $gen_trace variable.
      opengl32: Simplify make_opengl GL_VERSION filtering.
      opengl32: Support parsing multiple registry.py APIs.
      opengl32: Simplify adding functions in make_opengl.
      mfplat/tests: Test MFInitAMMediaTypeFromMFMediaType GUID conversions.
      opengl32: Support more function suffixes for hide_default_fbo.
      opengl32: Support more function suffixes for resolve_default_fbo.
      opengl32: Support more function suffixes for map_default_fbo_thunks.
      opengl32: Use a unique WOW64 wrapper for each group of functions.
      opengl32: Remove now unnecessary wow64 buffer function lookup.
      opengl32: Use a unique wrapper for each group of wrapped functions.

Stephan Seitz (1):
      shell32: Add stub for `SHEvaluateSystemCommandTemplate`.

Steven Don (1):
      taskmgr: Fix CPU and Memory usage history graphs.

Tim Clem (8):
      mountmgr.sys: Fall back to statfs if "important" free space is 0 on macOS.
      ntdll: Fall back to statfs if "important" free space is 0 on macOS.
      sechost: Catch invalid HDEVNOTIFY arguments to I_ScUnregisterDeviceNotification.
      sechost: Don't attempt to unregister an HDEVNOTIFY with bad magic.
      winemac.drv: Handle windows becoming invalid between WM_QUERYENDSESSION and WM_ENDSESSION.
      wintab32: Make the internal window message-only.
      wintab32: Use an HINSTANCE when registering the internal window class.
      wintab32: Create the internal window on demand.

Trent Waddington (2):
      kernel32/tests: Add tests for GetModuleFileName string termination.
      kernelbase: Fix string termination of GetModuleFileName.

Twaik Yont (5):
      wineandroid: Remove redundant backend DPI scaling.
      wineandroid: Fix 64-bit crash in setCursor JNI call.
      wineandroid: Lower targetSdkVersion to avoid Android 10 W^X restrictions.
      wineandroid: Fix loader path and library lookup for modern Wine/Android.
      wineandroid: Move environment setup to Java and drop obsolete env vars.

Vitaly Lipatov (2):
      include: Fix _LIBCPP_DEFER_NEW_TO_VCRUNTIME not being set with old clang.
      include: Avoid __builtin_wmemchr with clang < 13.

Yuxuan Shui (1):
      setupapi: Fix wrong buffer size in SetupDiGetClassDescriptionExA.

Zhiyi Zhang (6):
      iertutil: Fix parsing some partial IPv4 addresses.
      iertutil: Fix parsing URIs without a port value.
      iertutil: Do not encode user info if the scheme is unknown.
      iertutil: Fix backslash processing in URIs.
      iertutil: Add initial URI parsing support for IUriRuntimeClass.
      iertutil/tests: Add URI parser tests.
```
