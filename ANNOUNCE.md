The Wine development release 11.0-rc3 is now available.

What's new in this release:
  - Bug fixes only, we are in code freeze.

The source is available at <https://dl.winehq.org/wine/source/11.0/wine-11.0-rc3.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-11.0-rc3/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 11.0-rc3 (total 17):

 - #45483  Installation of the DirectX 8.1 SDK fails due to msi custom action failure
 - #46033  Execution of commands that contain special symbols will be truncated.
 - #52373  Call of Cthulhu intro videos don't play correctly
 - #53751  RollerCoaster Tycoon 1-2: rename dialog is hidden behind game screen
 - #54425  user32:clibpoard - test_string_data_process() fails on Windows in mixed locales
 - #54626  Saving files in Microsoft Word/Excel 2010 creates useless shortcut files
 - #56932  Null pointer dereference in MiniDumpWriteDump
 - #57570  Syntax Error in CMD Batch Parsing
 - #58454  Regression in Ti Nspire software
 - #58483  Burnout Paradise has rendering issues
 - #58491  Flickering on video-surveilance-app is back
 - #58709  upgrade to 10.15 results in garbled fullscreen in Limelight Lemonade Jam
 - #59047  VRChat sound output is broken after video playback start when using AVPro player
 - #59096  GLX backend doesn't work with Nvidia binary drivers since Wine-10.19
 - #59099  Mahjong game again does not display when launched - appears locked or caught in an endless loop
 - #59108  Steam: Client starts up, but all window content is black or white.
 - #59123  Performance issues in Clickteam games

### Changes since 11.0-rc2:
```
Alexandre Julliard (14):
      user32/tests: Get the codepage from the user locale for CF_TEXT conversions.
      d3d11/tests: Mark some tests that fail on gitlab CI as todo_wine.
      d3d9/tests: Mark some tests that fail on gitlab CI as todo_wine.
      win32u/tests: Fix a test failure with llvmpipe on 64-bit.
      amstream/tests: Fix test failures on 64-bit Windows.
      d3d8/tests: Disable a todo_wine that succeeds in new wow64.
      d3d10core/tests: Mark some tests that fail on Gitlab CI as todo_wine.
      d3d11/tests: Mark some tests that fail on Gitlab CI as todo_wine.
      imm32/tests: Mark a test that fails on Gitlab CI as todo_wine.
      ntdll: Set status in IO_STATUS_BLOCK on success.
      ntdll/tests: Comment out one more instance of setting %fs register.
      win32u: Only compare 32 bits of wparam in WM_STYLECHANGED handling.
      kernel32/tests: Don't test volatile register values.
      kernel32/tests: Synchronize the debug thread startup with the main thread.

Bernhard Übelacker (1):
      twain_32/tests: Test with twaindsm.dll at 64-bits.

Brendan Shanks (1):
      winemac: Report correct dimensions in macdrv_get_monitors() when retina mode is enabled.

Byeong-Sik Jeon (1):
      po: Update Korean translation.

Elizabeth Figura (3):
      d3d11/tests: Use explicit todo markers for test_uint_instructions() failures.
      ddraw: Check for DDSCAPS_PRIMARYSURFACE in ddraw_gdi_is_front().
      wined3d: Only invalidate enabled clip planes.

Eric Pouech (2):
      dbghelp: Don't crash when creating minidump without exception pointers.
      dbghelp: Skip compilation units with attributes imported from .dwz.

Hans Leidekker (1):
      nsiproxy.sys: Assign the highest metric value to loopback adapters.

Jactry Zeng (2):
      urlmon: Add missing period to Internet zone description.
      po: Update Simplified Chinese translation.

Matteo Bruni (4):
      winex11: Don't try to set NULL icon data.
      mfmediaengine: Apply stream volume instead of session-wide.
      mfmediaengine: Set SAR session ID to a random GUID.
      mfmediaengine/tests: Test WASAPI audio session interaction.

Paul Gofman (3):
      ntdll: Fix a crash in server_get_name_info() when there is NULL name.
      win32u: Register WGL_ARB_pixel_format before WGL_ARB_pixel_format_float.
      winex11.drv: Register WGL_ARB_pixel_format extension.

Rémi Bernon (12):
      opengl32: Avoid crashing if WGL_WINE_query_renderer is not supported.
      win32u: Use the window fullscreen exclusive rect for vulkan swapchain.
      win32u: Don't set window pixel format if drawable creation failed.
      win32u: Fix clipping out of vulkan and other process client surfaces.
      win32u/tests: Test that wine-specific D3DKMTEscape codes are harmless.
      opengl32: Check extension function pointers before using them.
      wined3d: Update the context DC from its swapchain if it changed.
      winex11: Avoid creating window data entirely for offscreen windows.
      winex11: Unmap windows only when SWP_HIDEWINDOW is set.
      winex11: Don't request activation of withdrawn windows.
      ntdll: Allow region allocation for some non growable heaps.
      win32u: Get the context drawable pointer again after context_sync_drawables.

Stefan Dösinger (3):
      wined3d: Initialize managed textures in LOCATION_SYSMEM.
      d3d9/tests: Test initial texture dirty state.
      d3d8/tests: Test initial texture dirty state.
```
