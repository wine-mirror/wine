The Wine development release 11.0-rc4 is now available.

What's new in this release:
  - Bug fixes only, we are in code freeze.

The source is available at <https://dl.winehq.org/wine/source/11.0/wine-11.0-rc4.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-11.0-rc4/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 11.0-rc4 (total 22):

 - #3845   parallel port access is slow due to high cpu usage (eprom burner)
 - #9906   Can't start the 4D Client
 - #51708  Bitwarden crashes on start
 - #55176  Wing Commander Secret Ops/Prophecy unable to launch
 - #55506  captvty 2.10.5.3, freeze when selecting a station
 - #55663  Buried in Time: Video files don't play, hang
 - #56045  RichTextBox crashes the WPF application
 - #56167  Unigine Heaven 4.0 DX11 benchmark performance loss on Nvidia gpu
 - #56226  Myst Masterpiece Edition won't start in Wine 4.6 and later
 - #56232  Sega Bug fails to initialize
 - #56790  wine binds dedicatedServer.exe to "lo" adapter
 - #57057  battle.net: invalid tray icon
 - #57378  Cannot start Alcohol52%
 - #57516  Heroes of Might and Magic 4 campaign editor: can't cycle through hero's portrait with left mouse button
 - #57747  Dungeon Siege (2002) rendering is broken
 - #57907  Black screen in Richard and Alice game and Ben there Dan that! Special Edition (both from GOG)
 - #58505  SSFpres search window appears behind main window since Wine 10.5
 - #59098  The application windows of certain programs (Delphi?) completely disappear while changing tabs or layouts before then reappearing
 - #59147  rthdribl performance is worse after eeb3e5597f11063aef389598b3c8408278fa8661
 - #59151  Main window content drawn twice for Ubisoft Connect
 - #59158  eRacer (using Cinepack codec in videos) hangs when the intro video ends
 - #59163  Adding debug channel +crypt makes Total Commander behave different/fail.

### Changes since 11.0-rc3:
```
Akihiro Sagawa (1):
      po: Update Japanese translation.

Alexandre Julliard (4):
      wow64: Copy the full xstate in Wow64PrepareForException.
      ntdll: Support ATL thunk emulation in new wow64 mode.
      ntdll/tests: Add a test for breakpoint exception address.
      ntdll/tests: Fix some test failures on Windows ARM64.

André Zwing (1):
      include: Disable compiler exceptions for ARM64EC MSVC targets before Clang 21.

Bernhard Kölbl (1):
      windows.media.speech/tests: Get rid of duplicated hresult.

Bernhard Übelacker (3):
      bcrypt: Avoid crash in gnutls logging when library got unloaded.
      crypt32: Avoid crash in gnutls logging when library got unloaded.
      secur32: Avoid crash in gnutls logging when library got unloaded.

Elizabeth Figura (8):
      wined3d: Add some missing states to wined3d_stateblock_invalidate_initial_states().
      wined3d: Avoid some invalidation when the vertex declaration is set redundantly.
      wined3d: Avoid some invalidation when the viewport is set redundantly.
      wined3d: Avoid some invalidation when texture states are set redundantly.
      wined3d: Invalidate fog constants only when the VS is toggled.
      wined3d: Avoid some invalidation when render states are set redundantly.
      quartz: Take top-down RGB into account in is_nontrivial_rect().
      wined3d: Invalidate FFP PS settings when FOGVERTEXMODE changes.

Esme Povirk (1):
      mscoree: Update Wine Mono to 10.4.1.

Matteo Bruni (2):
      wined3d/gl: Only check GL context when accessing onscreen resources.
      wined3d/gl: Split UBOs to separate chunks.

Rémi Bernon (4):
      winex11: Set net_active_window_serial when activating withdrawn windows.
      win32u: Add more set_(active|foreground)_window traces.
      winex11: Request drawable presentation explicitly on flush.
      winex11: Set opacity and empty input shape for the dummy parent.

Sven Baars (3):
      comctl32/tests: Run the updown tests on comctl32 version 6.
      comctl32: Set default acceleration values for up-down controls.
      comctl32: Fix the UDM_GETACCEL return value.
```
