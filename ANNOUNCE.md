The Wine development release 10.0-rc6 is now available. This is
expected to be the last release candidate before the final 10.0.

What's new in this release:
  - Bug fixes only, we are in code freeze.

The source is available at <https://dl.winehq.org/wine/source/10.0/wine-10.0-rc6.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-10.0-rc6/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 10.0-rc6 (total 18):

 - #47036  C&C Red Alert 2 Yuri's Revenge missing graphical elements
 - #48501  U.S. Naval Observatory MICA2 software has annoying error message upon closing
 - #50398  Microsoft Office XP 2002 installer shows "Error 25504. Failed to set Feature xyz to the install state of Feature xyz for mode 2." message boxes since Wine 2.12
 - #53567  The Medium crashes when starting new game
 - #53891  user32:msg - test_swp_paint_regions() fails on Windows 7
 - #55263  Boulder Remake doesn't recognise arrow keys (other keys work)
 - #56452  Wingdings font seems not to be found; regression test done
 - #57019  wineboot crashes in create_bios_processor_values() on 2013 Mac Pro
 - #57076  Harmony Assistant 9.9.8d (64 bit) reports missing stoccata.ttf font on startup
 - #57077  Micrografx Window Draw 4.0a crashes when backspacing while editing freeform text object
 - #57179  File uploads in Hotline Client 1.2.3 hang after about 200 KB
 - #57312  Rebuild 3: Broken texture filtering
 - #57579  Deformed symbols in LTSpice
 - #57610  No windows are shown when using a dual monitor setup
 - #57632  fallout 3 radio broke with gstreamer 1.24.10 (9.22 silent, 10rc4 stalls)
 - #57652  Some windows have cut bottom and right sides
 - #57657  Null pointer dereference in traces
 - #57661  Port Royale 2: black screen during intro videos

### Changes since 10.0-rc5:
```
Alexandre Julliard (3):
      ieframe/tests: Fix more property change errors for the new test.winehq.org server.
      wshom.ocx/tests: Mark a failing tests as todo.
      winebth.sys: Don't print an error when Bluetooth is not available.

Bernhard Übelacker (2):
      gdiplus/tests: Fix use-after-free of a graphics object (ASan).
      kernel32/tests: Avoid stack buffer overflow in get_com_dir_size (ASan).

Elizabeth Figura (2):
      msi/tests: Test msidbFeatureAttributesDisallowAdvertise.
      msi: Set features to absent if advertising is disallowed.

Esme Povirk (1):
      gdiplus/tests: Trace locked bitmap data on failure.

Rémi Bernon (5):
      gdi32: Avoid crashing when tracing NULL xform.
      configure: Use per-architecture cross flags if they are provided.
      winex11: Adjust requested visible rect relative to the previous position.
      winex11: Always blit offscreen over any other onscreen clients.
      mf/topology_loader: Avoid modifying downstream media type when attempting to connect.

Santino Mazza (1):
      gdiplus: Check for MapFont result in generate_font_link_info.

Zhiyi Zhang (1):
      winex11.drv: Fix display name in X11DRV_UpdateDisplayDevices().
```
