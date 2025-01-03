The Wine development release 10.0-rc4 is now available.

What's new in this release:
  - Bug fixes only, we are in code freeze.

The source is available at <https://dl.winehq.org/wine/source/10.0/wine-10.0-rc4.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-10.0-rc4/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 10.0-rc4 (total 13):

 - #37372  Unexpected order of results in wildcard expansion
 - #48877  Melodyne crashes when using the Pitch tool
 - #51656  Gaea Installer crashes in riched when pressing enter
 - #52447  64-bit .NET framework 2.0 installer hangs while generating/installing native images of 'System.Windows.Forms' assembly into GAC
 - #53405  Into The Breach freezes when enabling fullscreen
 - #54342  ws2_32:sock - test_WSARecv() sometimes fails with "got apc_count 1." on Windows
 - #56531  Final Fantasy XI Online: Some textures are transparent, malformed, or misplaced.
 - #56533  Final Fantasy XI Online: Incorrect/corrupt textures shown on models.
 - #56885  WinCatalog has a crash at startup
 - #57248  Rhinoceros 8.11 installer crashes on start
 - #57568  Arcanum (and many other titles) crashes on start
 - #57577  Minimised applications are restored with -4 vertical pixels.
 - #57587  10.0-rc1 regression (dsoundrender): no audio or hangs in some videos

### Changes since 10.0-rc3:
```
Alexandre Julliard (3):
      Update copyright info for 2025.
      ntdll: Set the processor architecture variable from the current arch.
      xml: Disable the non-determinist schema check.

André Zwing (6):
      bluetoothapis/tests: Don't test functions directly when reporting GetLastError().
      kernel32/tests: Don't test functions directly when reporting GetLastError().
      iphlpapi/tests: Don't test functions directly when reporting GetLastError().
      msvcr120/tests: Don't test function directly when reporting GetLastError().
      msvcp140/tests: Don't test function directly when reporting GetLastError().
      msvcp120/tests: Don't test function directly when reporting GetLastError().

Bernhard Übelacker (7):
      mfplat/tests: Fix copy-paste release calls.
      dwrite: Avoid stack-buffer-overflow in arabic_setup_masks.
      comctl32/tests: Fix test array size (ASan).
      comctl32/tests: Use sufficient user data buffer in the Tab tests (ASan).
      comctl32/tests: Mark a test as broken on Windows.
      dwrite: Fix off-by-one clustermap indexing (ASan).
      uiautomationcore: Fix a double-free of advisers array (ASan).

Elizabeth Figura (1):
      qasf/dmowrapper: Acquire new output samples for each ProcessOutput() call.

Eric Pouech (1):
      kernelbase: Don't free pathname if query failed.

Etaash Mathamsetty (1):
      nsiproxy: Set rcv/xmit speed to 1000000 on linux.

Floris Renaud (1):
      po: Update Dutch translation.

Jinoh Kang (1):
      user32/tests: Force window to be visible in subtest_swp_paint_regions.

Piotr Caban (2):
      msvcr120/tests: Skip _fsopen tests if file can't be created.
      msvcp120/tests: Skip _Fiopen tests if file can't be created.

Rémi Bernon (6):
      winex11: Improve GetWindowStateUpdates traces.
      win32u: Check window state updates again after applying new state.
      win32u: Don't overwrite dummy vulkan window.
      win32u: Always update the surface regions in apply_window_pos.
      server: Remove now unnecessary needs_update member.
      winex11: Don't re-create the GL drawable if pixel format didn't change.

Zhiyi Zhang (2):
      d2d1/tests: Remove a duplicate test.
      dxgi: Support more feature levels in debug_feature_level().
```
