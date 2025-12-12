The Wine development release 11.0-rc2 is now available.

This is the first release candidate for the upcoming Wine 11.0. It
marks the beginning of the yearly code freeze period. Please give this
release a good testing and report any issue that you find, to help us
make the final 11.0 as good as possible.

What's new in this release:
  - Bug fixes only, we are in code freeze.

The source is available at <https://dl.winehq.org/wine/source/11.0/wine-11.0-rc2.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-11.0-rc2/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 11.0-rc2 (total 28):

 - #12401  NET Framework 2.0, 3.0, 4.0 installers and other apps that make use of GAC API for managed assembly installation on NTFS filesystems need reparse point/junction API support (FSCTL_SET_REPARSE_POINT/FSCTL_GET_REPARSE_POINT)
 - #38652  VMR9 Owner and Clipping hwnd
 - #43110  Indycar Series crashes on start
 - #53380  After 7.13 update, components in pop-up windows, sliders and buttons, do not work. ThumbsPlus 10 Pro
 - #53737  Incoming (game): crashes after the intro video
 - #56177  The 64-bit uxtheme:system fails on Windows 10 2004+
 - #56286  Evil Under the Sun (French version) crashes when starting a new game
 - #56462  PlayOnline Viewer crashes when Wine is built without gstreamer support
 - #57393  MS Office 2013: Slide out File menu panel is behind the editing page
 - #57960  ChessBase 18 hangs / stops accepting input a few seconds after launch
 - #58171  Breath of Fire IV, Ys VI crashes on the opening video
 - #58261  Ys Origin: videos playing upside down
 - #58724  Heroes of the Storm doesn't start
 - #58811  Steam: Unable to display the intended GUI windows due to steamwebhelper being unresponsive.
 - #58824  Melonloader exit with an error code
 - #58844  Command & Conquer Tiberian Sun (C&C Red Alert II): black rectangle instead of showing menus
 - #58914  Basic Windows Combobox are randomly missing entries
 - #59012  Maximized windows cannot be minimized on kde
 - #59053  Mario Multiverse demo crashes upon launch
 - #59069  Splinter Cell: Intro video window is invisible
 - #59089  Forms in certain programs redraw very slowly
 - #59091  UbisoftConnect window completely broken
 - #59093  Various GL driver errors, OOM, and hangs on 32-bit after 4286e07f8
 - #59094  Itch.io client shows empty white window on start
 - #59097  Earth 2150 fails to play video
 - #59100  Worms Revolution displays only black screen
 - #59102  Minimizing fullscreen notepad and restoring gives a black window
 - #59110  Memory leak in lookup_unix_name

### Changes since 11.0-rc1:
```
Alexandre Julliard (4):
      gitlab: Avoid error when parsing the minor number of a .0 release.
      xactengine/tests: Remove todo_wine from tests that succeed now.
      tomcrypt: Avoid macro redefinition warnings.
      ir50_32: Import winegstreamer directly.

André Zwing (2):
      ntdll: Remove superfluous code.
      ntdll/unix: Add bti c for call dispatchers on ARM64.

Aurimas Fišeras (1):
      po: Update Lithuanian translation.

Bernhard Übelacker (5):
      msado15: Remove left over free in get_accessor (ASan).
      msado15: Add missing return when malloc failed.
      devenum/tests: Skip test if no video capture device is present.
      win32u/tests: Dynamically load function D3DKMTOpenKeyedMutexFromNtHandle.
      win32u/tests: Use dynamically loaded D3DKMTOpenKeyedMutexFromNtHandle.

Daniel Lehman (1):
      ntdll: Fix memory leak in reparse code path.

Dmitry Timoshkov (2):
      msi/tests: Add a test for MsiDecomposeDescriptor() with a product with only one feature.
      msi: MsiDecomposeDescriptor() should look up single feature of the product.

Elizabeth Figura (11):
      quartz/tests: Test avidec source media type after dynamic format change.
      quartz/avidec: Alter the current media type during dynamic format change.
      winegstreamer: Handle dynamic format change.
      amstream/tests: Test AM_MEDIA_TYPE generation for an existing sample and sub-rect.
      amstream: Set the source/target rects from the sample rect in GetMediaType().
      quartz/tests: Test using the AVI decompressor with rects.
      quartz/avidec: Support nontrivial rects.
      ir50_32/tests: Test extended decompression functions.
      ir50_32: Implement extended decompression.
      winegstreamer: Support dynamic reconnection in the DirectShow transform.
      winegstreamer: Fix a double free (Coverity).

Hans Leidekker (4):
      msi/tests: Allow registered owner and company to be unset.
      msi/tests: Fix pending renames processing on Windows 11.
      msi/tests: Fix test failures when running as an unprivileged user.
      msi/tests: Fix a test failure on ARM64 Windows 11.

Henri Verbeet (4):
      wined3d: Do not set "context_gl->needs_set" in wined3d_context_gl_release().
      wined3d: Get rid of the "restore_pf" field from struct wined3d_context_gl.
      wined3d: Expect WINED3D_VIEW_TEXTURE_CUBE for cube textures in get_texture_view_target().
      wined3d: Properly compare reduction modes in wined3d_texture_gl_apply_sampler_desc().

Huw D. M. Davies (1):
      winemac: Remove unused variable.

Jacek Caban (1):
      winegcc: Add libs/ to library search paths when --wine-objdir is used.

Jiangyi Chen (1):
      opengl32: Fix a copy error when printing the log.

Nikolay Sivov (1):
      Revert "dwrite: Use uppercase paths for local file loader keys."

Paul Gofman (4):
      kernel32/tests: Disable heap tests which are now crashing on Windows.
      kernel32/tests: Add more tests for heap total and commit sizes.
      ntdll: Align commit size in RtlCreateHeap().
      ntdll: Compute total size as aligned commit size in RtlCreateHeap().

Piotr Caban (1):
      msado15: Ignore IRowset:GetProperties return value in rsconstruction_put_Rowset (Coverity).

Rémi Bernon (15):
      win32u: Introduce a new NtUserGetPresentRect driver-specific call.
      win32u: Use the window present rect as visible rect when set.
      win32u: Fixup fullscreen cursor clipping when present rect is set.
      winex11: Ignore children clipping in exclusive fullscreen mode.
      winex11: Retrieve the whole window when creating client window for other process.
      opengl32: Return the buffers when invalid draw/read buffer are used.
      winex11: Check for events again after processing delayed events.
      win32u: Iterate the client surfaces rather than the toplevel window tree.
      opengl32: Retrieve display OpenGL functions table only when needed.
      win32u: Process every driver event in NtUserGetAsyncKeyState.
      win32u: Remove unnecessary drawable flush in context_sync_drawables.
      winex11: Use window DPI for client surface position and geometry.
      winex11: Avoid mapping points with exclusive fullscreen windows.
      win32u: Avoid setting surface layered with the dummy surface.
      win32u: Update window state after setting internal pixel format.

Sven Baars (2):
      ntdll: Use the correct CPU tick length on macOS.
      ntdll: Include idle time in kernel time on all platforms.

Yuxuan Shui (5):
      mf/tests: Test if WMA decoder DMO accept output type with a different num of channels.
      mf/tests: Test SetOutputType with a wrong formattype on WMA decoder.
      mf/tests: Test if WMV decoder DMO accepts output type with a different size.
      winegstreamer: Reject setting WMA DMO with an output type with a wrong formattype.
      winegstreamer: Reject SetOutput with a different number of channels.
```
