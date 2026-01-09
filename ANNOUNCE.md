The Wine development release 11.0-rc5 is now available. This is
expected to be the last release candidate before the final 11.0.

What's new in this release:
  - Bug fixes only, we are in code freeze.

The source is available at <https://dl.winehq.org/wine/source/11.0/wine-11.0-rc5.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-11.0-rc5/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 11.0-rc5 (total 32):

 - #19063  GAP-Diveplanner : return Run time error '5'
 - #23688  iriver LDB Manager: crashes when navigating directory tree
 - #30129  Screen in 3d Studio Max v9 refresh badly
 - #32235  Soldiers heroes of World War II crashes on startup
 - #38505  sijpv12.exe crashes
 - #39742  Heroes of Might and Magic 5 does not work (empty screen)
 - #40512  Alan Wake's American Nightmare, some textures have missing transparency
 - #40656  Heartstone Deck Tracker v0.14.9 fails to start
 - #40730  Elasto Mania: Hang on launch - later works but without sound
 - #41568  MiPony 2.5.0 can't be launched
 - #46976  All games suffer from moderated to severe stuttering with mouse polling ~1000
 - #47244  Batman: Arkham Origins no shadow from cloak in DX11 mode
 - #48730  BioShock 2 crashes before entering main menu with builtin d3dcompiler_47
 - #49013  League of Legends: Exception code c0000005
 - #50724  Tomb Raider (2013) enabling tessellation leads to crash with Vulkan renderer
 - #52318  Sumatra PDF - always prints entire document
 - #52780  The Evil Within has very slow performance
 - #53422  Bully: Scholarship Edition fails to play intros
 - #55021  The Evil Within shows a black screen with OpenGL renderer
 - #55849  Wine posts WM_(SYS)KEYUP message twice for Alt (and such) key if Fcitx/IBus daemon is running
 - #56159  game blocks ~30 seconds, continues and then hangs, in "TKKG 1" (EN: Jennifer is Missing, DE: Katjas Geheimnis - Tivoli)
 - #56815  Virtualbox installer fails to install
 - #57999  "Tower Wars" game crash when start playing
 - #58498  Rocket League:  OSS driver is triggering game to crash
 - #58786  Microsoft Deadly Tide requires ICM_DECOMPRESSEX_BEGIN implementation in iccvid
 - #59165  Focus problems in SQLyog
 - #59171  Face Noir: 3D inventory way too dark
 - #59177  Earth 2150 freezes at the end of the intro video
 - #59180  DirectDraw fullscreen windows are invisible
 - #59190  Wolfenstein: The Old Blood / Wolfenstein: The New Order - black screen in the menu
 - #59203  Monster Truck Madness 2 crashes with "double free detected"
 - #59209  Monster Truck Madness 2 takes very long when loading/leaving a race

### Changes since 11.0-rc4:
```
Alexandre Julliard (5):
      winedump: Dump the contents of the embedded NE module for 16-bit builtins.
      Update copyright info for 2026.
      ntdll: Don't try to create a process for a 16-bit dll.
      ntdll: Use the sync_ioctl() helper in is_console_handle().
      ws2_32/tests: Wait a bit before checking that we didn't receive a message.

André Zwing (1):
      wow64: Only compare the valid segment selector part.

Bernhard Übelacker (1):
      win32u: Return failure from ResizePalette called with a count of zero.

Byeong-Sik Jeon (1):
      winex11: Drop SCIM workaround.

Dmitry Timoshkov (1):
      ntdll: get_non_pe_file_info() returns NTSTATUS.

Elizabeth Figura (5):
      wined3d: Invalidate lights when the view matrix changes.
      iccvid/tests: Add format tests.
      iccvid: Implement extended decompression.
      d3d8/tests: Test how lighting is affected by world and view matrices.
      d3d9/tests: Test how lighting is affected by world and view matrices.

Etaash Mathamsetty (1):
      winewayland: Separate icon buffers from toplevel role.

Gabriel Ivăncescu (1):
      mshtml/tests: Test calling external object's method with return value.

James Hawkins (2):
      user32/tests: Test DlgDirList in a temp directory.
      user32/tests: Remove unused todo_wine handling from listbox test.

Lauri Kenttä (1):
      po: Update Finnish translation.

Rémi Bernon (3):
      win32u: Update client surface state before swapping or presenting.
      opengl32: Use a GL_FLUSH_FORCE_SWAP flag instead of BOOL parameter.
      opengl32: Present on flush only when rendering to the front buffer.

Sven Baars (1):
      opengl32: Avoid leaking memory on error paths (Coverity).

Zhiyi Zhang (4):
      comctl32/tests: Add more RegisterClassNameW() tests.
      comctl32: Register window classes in RegisterClassNameW() instead of DllMain().
      winex11.drv: Don't send -1 fullscreen monitor indices to window managers.
      dxgi/tests: Add a missing return value check.
```
