The Wine development release 10.0-rc3 is now available.

What's new in this release:
  - Bug fixes only, we are in code freeze.

The source is available at <https://dl.winehq.org/wine/source/10.0/wine-10.0-rc3.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-10.0-rc3/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 10.0-rc3 (total 15):

 - #11674  Dual-core unsupported in WoW and SC2
 - #49473  Chaos Legion videos are played upside down
 - #52738  No keyboard input in "STREET CHAVES - O LUTADOR DA VILA"
 - #56319  Parallel Port Tester won't start (fails to locate driver "System32\Drivers\inpoutx64.sys", but changing to absolute path works)
 - #56348  Bricks: moving a brick causes it to rapidly alternate positions
 - #56471  starting of native program with "start /unix ..." is broken
 - #56632  Explorer cannot run any files in Windows ME compatibility mode (or below)
 - #56714  Startopia is stuck on a black screen on launch
 - #57227  IL-2 1946 crash at startup
 - #57286  Dark Age of Camelot - camelot.exe required igd10umd32.dll but the .dll file is not found.
 - #57319  Painting in a proprietary application is broken with vulkan renderer
 - #57515  desktop mode did not show taskbar anymore
 - #57523  PokerTracker 4: cannot launch anymore
 - #57525  Systray icons cannot be interacted with
 - #57541  CMake doesn't find toolchain

### Changes since 10.0-rc2:
```
Akihiro Sagawa (1):
      po: Update Japanese translation.

Alanas Tebuev (1):
      comctl32/tests: Initialize hwnd to NULL before calling rebuild_toolbar().

Alexandre Julliard (6):
      ntdll: Align heap virtual allocations to a multiple of the page size.
      shell32: Don't call AW functions internally.
      shell32: Return the file itself without extension if it exists.
      propsys/tests: Fix a test that fails on some Windows versions.
      win32u/tests: Mark the foreground thread test as flaky.
      advapi32/tests: Use the correct key handle in the notify thread.

Alistair Leslie-Hughes (1):
      msxml3: Correct looping of Document Element node map.

Bernhard Übelacker (4):
      server: Avoid crash when handle table is allocated but not yet filled.
      crypt32: Avoid stack-use-after-scope in CSignedEncodeMsg_GetParam (ASan).
      d3dx9_36/tests: Fix logging of expected bytes in check_vertex_components. (ASan).
      xmllite/tests: Avoid buffer overflow by using LONG_PTR (ASan).

Elizabeth Figura (2):
      wined3d: Add nop state entries for states now invalidated on the client side.
      Revert "wined3d: Use bindless textures for GLSL shaders if possible.".

Eric Pouech (3):
      cmd/tests: Add more tests about variable expansion.
      cmd: Fix regression in variable search in expansion.
      winedump: Fix variable overwrite when dumping exception.

Esme Povirk (1):
      gdiplus: GdipPathAddRectangle should close the path.

Gerald Pfeifer (1):
      capstone: Avoid GCC being treated as old VisualStudio.

Jacek Caban (1):
      mshtml: Ignore Gecko events on detached nodes.

Louis Lenders (1):
      kernelbase: Don't try to print the path in the FIXME in GetTempPath2.

Nikolay Sivov (9):
      windowscodecs/tests: Added some tests for Exif and Gps IFDs embedded in App1 blob.
      windowscodecs/tests: Add some tests for CreateMetadataWriterFromReader().
      windowscodecs/tests: Add some tests for CreateMetadataWriter().
      windowscodecs/tests: Add some tests for metadata stream objects handling.
      windowscodecs/tests: Add loading tests for the writers.
      windowscodecs/tests: Check persist options after Load().
      windowscodecs: Fix a typo in interface name.
      include: Add methods arguments annotations for DirectWrite types.
      dwrite/tests: Allocate test inline objects dynamically.

Owen Rudge (2):
      odbc32: Avoid crashing if str is null in debugstr_sqlstr.
      odbc32: Add null pointer checks to update_result_lengths helpers.

Paul Gofman (2):
      winex11: Use NtUserReleaseDC() with hdc.
      server: Cleanup all the global hooks owned by terminating thread.

Rémi Bernon (5):
      winex11: Sync gl drawable outside of the win_data mutex.
      winex11: Use DCX_USESTYLE when checking DC clipping regions.
      winex11: Move GL/VK offscreen if the clipping region is NULLREGION.
      dinput: Copy the device format if the user format is a subset of it.
      dinput: Check that the device format data fits in the user format data.
```
