The Wine development release 10.0-rc5 is now available.

What's new in this release:
  - Bug fixes only, we are in code freeze.

The source is available at <https://dl.winehq.org/wine/source/10.0/wine-10.0-rc5.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-10.0-rc5/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 10.0-rc5 (total 31):

 - #38975  Alpha Protocol launcher: menu options hidden behind grey boxes
 - #48737  Microsoft Golf 2.0 demo crashes on startup
 - #52542  NVIDIA GeForceNow Installer fails due to rundll32 problems
 - #53352  Redefinition of typedef ‘D2D1_PROPERTY_BINDING’ breaks compilation with gcc 4.3.4
 - #54717  dbghelp:dbghelp - SymRefreshModuleList() sometimes returns STATUS_INFO_LENGTH_MISMATCH on Windows
 - #56205  The Egyptian Prophecy: The Fate of Ramses: text displayed without transparency
 - #56474  Crowns and Pawns: Graphic bugs
 - #56523  The Dark Pictures Anthology: Man of Medan hangs/crashes after company logo
 - #56605  V-Rally 4 crashes right before starting a race
 - #56627  Direct3D applications run out of memory on Windows XP
 - #56770  Geneforge 4: stuttering in character and mouse movement
 - #56886  Wincatalog can't scan folders
 - #57207  Fallout 3: Regression spams console with errors
 - #57274  Regression causing Obduction to hang and exit
 - #57306  Multiple programs crash due to memory corruption since 5924ab4c515 (Nikon NX studio, Profit, Falcosoft's Soundfont Midi Player, IBExpert)
 - #57333  Civilization IV fails to start (XML load Error)
 - #57409  Interactivity The Interactive Experience from itch.io deadlocks (regression)
 - #57476  Methods arguments attributes are missing from dwrite.idl
 - #57522  Voltage sources have the wrong shapes in Micro-Cap 12.2.0.5 on Wine 9.21 and later
 - #57549  Fighter Factory 3: Window Graphics don't display correctly after prolonged use.
 - #57550  Geneforge 4 complains about resolution and crashes when run in virtual desktop
 - #57551  10-rc2 regression: MS Office 2007/2010: some dialogs are only ~1/4 visible
 - #57558  joy.cpl xinput joysticks circles are cropped by 1px on the bottom
 - #57566  Silent crash for application attempting to use RSA
 - #57582  Eschalon Book I: launcher menu flickering
 - #57583  Truncated popup
 - #57584  8-bit color mode is broken in Wine 9.11 and later
 - #57599  HyperBall Shareware: black screen (regression)
 - #57601  Touchscreen input broken for x11drv/mouse.c
 - #57636  Black screen in menu until button click in Age of Empires
 - #57649  call .bat doesn't propagate errorlevel

### Changes since 10.0-rc4:
```
Alexandre Julliard (7):
      shell32: Look for the file name without extension also for the path search case.
      dnsapi/tests: Update DNS names for the new test.winehq.org server.
      wininet/tests: Update certificate for the new test.winehq.org server.
      secur32/tests: Update expected results for the new test.winehq.org server.
      winhttp/tests: Allow some more notifications for the new test.winehq.org server.
      ieframe/tests: Allow more property changes with the new test.winehq.org server.
      win32u: Fix stack corruption in NtUserScrollDC.

Anton Baskanov (2):
      ddraw/tests: Test that releasing a primary surface invalidates the window.
      ddraw: Invalidate the window when the primary surface is released.

Bernhard Übelacker (2):
      d3dx9_36/tests: Fix test data buffer underflow (ASan).
      gdi32: Explicitly check for negative text length in GetTextExtentExPointW().

Billy Laws (1):
      ntdll: Emulate mrs xN, CurrentEL instructions.

Brendan McGrath (1):
      mfmediaengine: Fix maths in scaling check.

Brendan Shanks (2):
      wineboot: Correctly handle SMBIOS tables older than v3.0.
      ntdll: On macOS, only use actual SMBIOS tables if they are v2.5 or higher.

Connor McAdams (1):
      quartz/dsoundrender: Restart the render thread when clearing EOS in dsound_render_sink_end_flush().

Conor McCarthy (1):
      mf: Do not clean up a session op if it was submitted to a work queue.

Dmitry Timoshkov (4):
      rsaenh/tests: Add some tests for RC4 salt.
      rsaenh/tests: Add a test for RC4 session key.
      rsaenh/tests: Make RC4 tests more distinct.
      rsaenh: CPGenKey() shouldn't generate RC4 key salt if not requested.

Elizabeth Figura (13):
      Revert "win32u: Forward to Rectangle() if the ellipse width or height is zero.".
      Revert "win32u: Do not convert back to integer before finding intersections.".
      Revert "win32u: Correctly handle transforms which flip in get_arc_points().".
      Revert "win32u: Normalize inverted rectangles in dibdrv_RoundRect().".
      Revert "win32u: Always select the point that's closer to the ellipse.".
      Revert "win32u: Allocate the whole max_points for the top_points array.".
      Revert "win32u: Implement drawing transformed round rectangles.".
      Revert "win32u: Implement drawing transformed arcs.".
      gdi32: Trace more functions.
      gdi32/tests: Add some arc tests.
      ddraw/tests: Test preservation of the X channel when clearing.
      wined3d: Separate a cpu_blitter_clear_texture() helper.
      ddraw: Clear sysmem textures on the CPU.

Eric Pouech (4):
      cmd/tests: Add more tests.
      cmd: Skip trailing white spaces in FOR's option.
      cmd: Fix some CALL errorlevel propagation.
      cmd: Don't return syntax error code on empty lines.

Esme Povirk (1):
      gdiplus: Use font linking only for missing glyphs.

Francis De Brabandere (3):
      vbscript/tests: Refactor Mid() error tests.
      vbscript: Fix Mid() empty and null handling.
      vbscript: Remove trailing semicolon in parser.

Gabriel Ivăncescu (1):
      winex11: Respect swp_flags when syncing window position.

Jactry Zeng (1):
      po: Update Simplified Chinese translation.

Lauri Kenttä (2):
      documentation: Update Linux and Mac OS X versions.
      po: Update Finnish translation.

Nikolay Sivov (1):
      include: Fix method arguments annotations in dwrite.idl.

Paul Gofman (5):
      ddraw/tests: Test state application on multiple devices.
      ddraw: Factor out d3d_device_apply_state().
      ddraw: Correctly apply state when multiple devices are used.
      winex11: Flush display when presenting offcreen drawable from wglFlush / wglFinish.
      winex11: Call glFinish() when presenting offscreen drawable from wglFlush.

Rémi Bernon (5):
      winex11: Check window region instead of forcing offscreen on parent.
      winebus: Ignore unsupported hidraw touchscreen devices.
      winex11: Map mouse/touch event coordinates even without a hwnd.
      win32u: Initialize dibdrv info from the surface color bitmap.
      winex11: Always fill the window surface color info.

Santino Mazza (1):
      mmdevapi/tests: Fix audio clock adjustment tests failing in testbot.

William Horvath (1):
      winex11: Use the win32 client rect in needs_client_window_clipping.
```
