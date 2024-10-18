The Wine development release 9.20 is now available.

What's new in this release:
  - Bundled Capstone library for disassembly in WineDbg.
  - More formats supported in D3DX9.
  - Static analysis and JUnit test reports in Gitlab CI.
  - More support for network sessions in DirectPlay.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/9.x/wine-9.20.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-9.20/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 9.20 (total 15):

 - #39848  Victoria 2 (Steam) fails to start with Wine-Mono
 - #50850  Just Cause crashes when starting new game (D3DXCreateTexture unsupported format, fallback format crashes)
 - #56372  musl based exp2() gives very inaccurate results on i686
 - #56645  unimplemented function httpapi.dll.HttpSendResponseEntityBody
 - #56973  Building wine with mingw/gcc 14.1.1 fails with  error  '-Wimplicit-function-declaration'
 - #57233  Multiple games show black screen/window on startup (BeamNG.drive, Wargaming.net games)
 - #57245  Can't recognize executables/scripts with a dot in the name...
 - #57250  Rhinoceros installers crash with bad_alloc
 - #57269  wine-9.19 build with ffmpeg fails in winedmo in Ubuntu 20.04
 - #57271  `winetricks -q art2kmin` shows several popups -- Unable to load dll
 - #57293  Helicon Focus 8.2.0 regression: open images hangs the application
 - #57294  Wine 9.13+ freezes in some applications using WMA Lossless audio
 - #57300  KnightOfKnights crashes once entering the game
 - #57302  In Notepad++ find window gets glitched after losing and regaining focus
 - #57311  Nikon NX Studio Overlay windows incorrectly shown.

### Changes since 9.19:
```
Aida Jonikienė (1):
      winex11: Properly handle minimized windows in update_net_wm_fullscreen_monitors().

Alex Henrie (2):
      winebus: Allow any free device index to be reused immediately.
      explorer: Support the NoDesktop registry setting.

Alexandre Julliard (21):
      gitlab: Add support for static analysis using Clang.
      gitlab: Add a daily sast build.
      gitlab: Add a daily mac build.
      faudio: Import upstream release 24.10.
      mpg123: Import upstream release 1.32.7.
      png: Import upstream release 1.6.44.
      tiff: Import upstream release 4.7.0.
      ldap: Import upstream release 2.5.18.
      fluidsynth: Import upstream release 2.3.6.
      xslt: Import upstream release 1.1.42.
      libs: Import the Capstone library version 5.0.3.
      winedbg: Switch to the Capstone disassembler.
      libs: Remove the no longer used Zydis library.
      widl: Use plain inline instead of defining a macro.
      include: Remove custom stdcall/cdecl definition for ARM platforms.
      include: Assume that nameless unions/structs are supported.
      include: Stop using WINAPIV in msvcrt headers.
      xml2: Import upstream release 2.12.8.
      makefiles: Use llvm-strip in MSVC mode.
      tools: Update the install-sh script.
      wow64: Add missing ThreadIdealProcessorEx class.

Alistair Leslie-Hughes (1):
      dplayx/tests: Correct Enum tests.

Anton Baskanov (27):
      dplayx: Use DP_CreatePlayer() in DP_SecureOpen().
      dplayx: Free the old session desc in DP_SecureOpen().
      dplayx: Free resources on error paths in DP_SecureOpen().
      dplayx: Send password in player creation forward request.
      dplayx: Check REQUESTPLAYERREPLY size before access.
      dplayx: Free message header on error path in DP_MSG_SendRequestPlayerId().
      dplayx: Use the documented reply layout in DP_MSG_SendRequestPlayerId().
      dplayx: Check reply result in DP_MSG_SendRequestPlayerId().
      dplayx/tests: Test receiving REQUESTPLAYERREPLY with error result.
      dplayx: Handle ADDFORWARDREPLY and return error.
      dplayx/tests: Test receiving ADDFORWARDREPLY.
      dplayx/tests: Wait for Open() to finish when forward request is not expected.
      dplayx/tests: Correctly report lines in check_Open().
      dplayx/tests: Send correct port in requests in check_Open().
      dplayx: Don't crash on unknown command ids.
      dplayx/tests: Test that ADDFORWARDACK is sent in reply to ADDFORWARD.
      dplayx: Keep track of the connection status in bConnectionOpen.
      dplayx: Enter the critical section when accessing the session desc.
      dplayx: Enter the critical section when accessing the player list.
      dplayx: Remove const from message body and header parameters of DP_HandleMessage().
      dplayx: Handle ADDFORWARD, add player to the session and send ADDFORWARDACK.
      dplayx: Prevent multiplication overflow in DP_MSG_ReadSuperPackedPlayer().
      dplayx/tests: Use DPENUMPLAYERS_LOCAL and DPENUMPLAYERS_REMOTE to check player flags in checkPlayerList().
      dplayx: Respect enumeration flags in EnumPlayers().
      dplayx: Return DPERR_INVALIDPARAM from CreatePlayer() if session is not open.
      dplayx/tests: Test client side of CreatePlayer() separately.
      dplayx: Remove the unused lpMsgHdr parameter from DP_IF_CreatePlayer().

Bernhard Übelacker (1):
      msvcrt: Initialize locale data in new threads.

Biswapriyo Nath (7):
      include: Add windows.data.xml.dom.idl.
      include: Add windows.security.authorization.appcapabilityaccess.idl.
      include: Fix base class of ICompositorInterop interface.
      include: Add windows.ui.notifications.idl.
      include: Add IUISettings4 definition in windows.ui.viewmanagement.idl.
      include: Add IUISettings5 definition in windows.ui.viewmanagement.idl.
      include: Add IUISettings6 definition in windows.ui.viewmanagement.idl.

Charlotte Pabst (5):
      riched20/tests: Test that ScrollWindowEx and GetClientRect are only called when control is in-place active.
      riched20: Exit from editor_ensure_visible when control is not in-place active.
      riched20: Only call ME_SendRequestResize when control is in-place active.
      comdlg32: Fix buffer overflow when current_filter is longer than MAX_PATH.
      comdlg32/tests: Add tests for itemdlg filters longer than MAX_PATH.

Connor McAdams (28):
      d3dx9: Add pixel_format_desc type checking helper functions.
      d3dx9: Rework pixel_format_desc structure format type value.
      d3dx9: Always align and mask channel bits in format_to_d3dx_color().
      d3dx9: Get rid of la_{to,from}_rgba format callbacks.
      d3dx9: Get rid of index_to_rgba callback.
      d3dx9: Add support for D3DFMT_X8L8V8U8.
      d3dx9: Add support for D3DFMT_A2W10V10U10.
      d3dx9: Add support for D3DFMT_A8P8.
      d3dx9: Add support for D3DFMT_V16U16.
      d3dx9: Add support for D3DFMT_Q16W16V16U16.
      d3dx9/tests: Add tests for DDS files containing indexed pixel formats.
      d3dx9: Include color palette size when validating the size of DDS files with indexed pixel formats.
      d3dx9: Add support for retrieving pixels from DDS files with indexed pixel formats.
      d3dx9: Add support for D3DFMT_A8P8 DDS files.
      d3dx9/tests: Add more DDS pixel format tests.
      d3dx9/tests: Add file size validation tests for DDS files containing packed pixel formats.
      d3dx9: Validate the size of DDS files containing packed pixel formats.
      d3dx9: Rework conversion to/from D3DFORMAT from/to DDS pixel format.
      d3dx9: Add support for more DDS pixel formats.
      d3dx9: Do not use WIC to detect image file format.
      d3dx9/tests: Add TGA header image info tests.
      d3dx9: Use d3dx9 to get image information for targa files.
      d3dx9/tests: Remove now unused arguments from check_tga_image_info().
      d3dx9/tests: Add more tests for loading targa files.
      d3dx9: Add support for loading basic targa images without WIC.
      d3dx9: Add support for decoding targa files with different pixel orders in d3dx9.
      d3dx9: Add support for decoding targa files with run length encoding in d3dx9.
      d3dx9: Add support for decoding targa files with a color map in d3dx9.

Daniel Lehman (4):
      msvcp90/tests: Add tests for string length.
      msvcp90/tests: Add some tests for num_put on ints.
      msvcp90: Exclude sign from count in num_put.
      msvcp90/tests: Add tests for int in num_put.

Dmitry Timoshkov (1):
      gdiplus: Add support for EmfPlusRecordTypeSetRenderingOrigin record playback.

Elizabeth Figura (6):
      wined3d: Move rasterizer state invalidation to wined3d_stateblock_set_render_state().
      ddraw: Do not apply the entire stateblock when clearing.
      d3d8: Do not apply the stateblock when clearing.
      d3d9: Do not apply the stateblock when clearing.
      win32u: Allocate the whole max_points for the top_points array.
      win32u: Always select the point that's closer to the ellipse.

Eric Pouech (3):
      dbghelp: Protect against buffer overflow in traces.
      dbghelp: Add a couple of TRACE().
      dbghelp: Fix a couple a typos.

Esme Povirk (1):
      comctl32: Implement WM_GETOBJECT for buttons.

Gabriel Ivăncescu (7):
      jscript: Allow ES5 keywords as identifiers in variable declarations.
      jscript: Allow ES5 keywords as identifiers in catch statements.
      jscript: Allow ES5 keywords as identifiers in function expressions.
      jscript: Allow ES5 keywords as identifiers in function parameter lists.
      jscript: Allow ES5 keywords as identifiers in labelled statements.
      jscript: Allow ES5 keywords as identifiers in expressions.
      mshtml: Make sure disp_invoke is called before locking the document mode.

Hans Leidekker (4):
      msiexec: Remove quotes from all filenames.
      findstr: Fix codepage passed to WideCharToMultiByte().
      ntdll: Add a stub implementation of NtQueryInformationThread(ThreadIdealProcessorEx).
      sort: New program.

Haoyang Chen (1):
      mlang: Check handle validity in IMLangFontLink_GetFontCodePages.

Martin Storsjö (1):
      musl: Fix limiting the float precision in intermediates.

Paul Gofman (15):
      wininet: Validate pointers in InternetReadFile().
      user32/tests: Add tests for QueryDisplayConfig( QDC_VIRTUAL_MODE_AWARE ).
      win32u: Support QDC_VIRTUAL_MODE_AWARE in NtUserGetDisplayConfigBufferSizes().
      win32u: Support QDC_VIRTUAL_MODE_AWARE in NtUserQueryDisplayConfig().
      ntdll/tests: Add more tests for printf format.
      ntdll: Fix passing char argument to pf_handle_string_format().
      ntdll: Output unrecognized format symbol in pf_vsnprintf().
      ntdll: Make 'l' modifier also affect char wideness.
      ntdll: Make 'h' take precedence over 'l' in pf_vsnprintf().
      shlwapi: Use printf implementation from ntdll.
      windowscodecs: Implement 48bppRGB -> 64bppRGBA conversion.
      ddraw/tests: Fix texture interface IID in test_multiple_devices() for ddraw1.
      ddraw: Use global handle table in d3d_device2_SwapTextureHandles().
      ddraw: Validate handles in d3d_device2_SwapTextureHandles().
      ddraw: Update state if d3d_device2_SwapTextureHandles() results in texture change.

Piotr Caban (2):
      advapi32: Don't trace password in CreateProcessWithLogonW stub.
      conhost: Start input thread for GetNumberOfConsoleInputEvents.

Rémi Bernon (49):
      win32u: Use an internal message for XIM IME notifications.
      win32u: Add winevulkan/driver entry points to sync surfaces with the host.
      winemac: Stop mapping toplevel window rects to parent window.
      winemac: Use NtUserSetWindowPos when DPI awareness is unnecessary.
      winex11: Use NtUserSetWindowPos when DPI awareness is unnecessary.
      winex11: Use XTranslateCoordinates to compute relative coordinates.
      win32u: Introduce a new NtUserSetRawWindowPos call for the drivers.
      win32u: Pass a rect to SetIMECompositionWindowPos.
      win32u: Pass absolute rect to SetIMECompositionRect.
      winemac: Use SetIMECompositionRect to keep track of IME position.
      win32u: Notify the drivers of destroyed windows on thread detach.
      winedmo: Check and guard libavcodec/bsf.h inclusion.
      winex11: Move the XDND IDataObject implementation around.
      winex11: Cleanup XDND IDataObject methods and variables.
      winex11: Use the IDataObject interface to check for CF_HDROP format.
      winex11: Cleanup variable names in X11DRV_XDND_SendDropFiles.
      winex11: Use IDataObject to get CF_HDROP format for WM_DROPFILES.
      winex11: Allocate the XDND data object dynamically.
      winex11: Pass window_rects structs parameters to move_window_bits.
      win32u: Copy the entire window rect when the whole window is moved.
      win32u: Adjust the valid rects to handle visible rect changes.
      winex11: Only enter the CS to get a reference on the data object.
      winex11: Assume that PostMessageW WM_DROPFILES succeeds.
      winex11: Get rid of X11DRV_XDND_SendDropFiles helper.
      winex11: Clear the XDND data object on drop event.
      winex11: Use a custom IEnumFORMATETC interface implementation.
      winex11: Get rid of X11DRV_XDND_HasHDROP helper.
      win32u: Use parent-relative coordinates for old window rectangles.
      win32u: Avoid crashing when creating a new layered window surface.
      winetest: Add printf attributes to strmake.
      winetest: Add printf attributes to xprintf.
      winetest: Always use a temporary file for test output.
      winetest: Pass output file handle to xprintf.
      winetest: Introduce some test report helpers.
      winetest: Implement JUnit report output mode.
      gitlab: Use winetest JUnit output mode.
      winex11: Keep the target window on the data object.
      winex11: Keep the target window point on the data object.
      winex11: Keep the target effect on the data object.
      winex11: Keep the IDropTarget pointer instead of HWND/accepted.
      winex11: Keep the IDropTarget pointer on the data object.
      winegstreamer: Use wmaversion = 4 for MFAudioFormat_WMAudio_Lossless.
      win32u: Introduce a new NtUserDragDropCall message call.
      winex11: Compute DND drop point earlier when dropping files/urls.
      winex11: Query the DndSelection property value earlier.
      winex11: Lookup for files/urls DND target window in user32.
      win32u: Move the PE side DND callbacks to user32.
      win32u: Map points from window monitor DPI to thread DPI.
      user32/tests: Flush events after test_SetForegroundWindow.

Santino Mazza (2):
      gdiplus: Assign box height when bounding box height is larger.
      mmdevapi: Fix buffer overflow in pulse_set_sample_rate.

Vibhav Pant (1):
      bluetoothapis/tests: Redefine SDP type descriptor constants to compile with older GCC versions.

Zhiyi Zhang (4):
      gitlab: Update generated files for static analysis.
      ntdll/tests: Add reserve object tests.
      ntdll: Implement NtAllocateReserveObject().
      ntdll/tests: Add NtAllocateReserveObject() tests.

Ziqing Hui (4):
      propsys/tests: Add tests for PropVariantToVariant.
      propsys/tests: Test converting clsid to string.
      propsys: Support converting clsid to string for PropVariant.
      propsys: Initially implement PropVariantToVariant.
```
