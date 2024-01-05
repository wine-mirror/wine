The Wine development release 9.0-rc4 is now available.

What's new in this release:
  - Bug fixes only, we are in code freeze.

The source is available at <https://dl.winehq.org/wine/source/9.0/wine-9.0-rc4.tar.xz>

Binary packages for various distributions will be available
from <https://www.winehq.org/download>

You will find documentation on <https://www.winehq.org/documentation>

Wine is available thanks to the work of many people.
See the file [AUTHORS][1] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/raw/wine-9.0-rc4/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 9.0-rc4 (total 17):

 - #4291   The menubar & toolbar of TresED are not displayed correctly.
 - #7106   Need for Speed 3 autorun crashes when starting setup
 - #26142  Civilization 4: Screen turns black on turn end with built-in msxml
 - #38039  linux console leaves in stty echo off mode after executing bash.exe from Wine
 - #40011  git-gui crashes on start
 - #45242  winedbg Internal crash when debugging win32
 - #51738  Bioshock Infinite crashes after intro with "mmap() failed: Cannot allocate memory"
 - #52159  cygwin/msys2: Unhandled page fault in 64-bit gdb.exe and python3.8.exe
 - #54256  Compile for wine under arm macOS (for running aarch64 windows apps) fails
 - #55540  IS Defense hangs after gameplay begins or has rendering glitches
 - #55637  dmime:dmime - test_performance_pmsg() sometimes fails due to bad timing on Windows and Wine
 - #55961  Multiple VST plugins have an invisible cursor
 - #56021  Wine 9.0-rc1 Wayland: In Sway full screen games don't run on full screen
 - #56023  Wine 9.0-rc1 Wayland: DPI problems
 - #56032  winedbg --gdb: gets terminated when target process exits
 - #56047  Won't build on FreeBSD, error: 'F_GETPATH' undeclared
 - #56110  Bejeweled 3: can't enable 3D acceleration

### Changes since 9.0-rc3:
```
Alexandre Julliard (6):
      Update copyright info for 2024.
      server: Remove WINESERVER documentation from the man page.
      loader: Remove absolute paths references from the man page.
      readme: Convert to Markdown.
      announce: Convert to Markdown.
      ntdll: Determine the available address space dynamically on ARM64.

André Zwing (2):
      mscoree/tests: Don't test function directly when reporting GetLastError().
      ntoskrnl/tests: Use RtlNtStatusToDosErrorNoTeb() for stateless conversion.

Bernhard Kölbl (1):
      windows.media.speech/tests: Remove obsolete workarounds.

Brendan Shanks (2):
      include: Assert that the debug channel name will be null-terminated and is not too long.
      server: Fix compile error on FreeBSD/NetBSD.

Byeong-Sik Jeon (1):
      po: Update Korean translation.

Eric Pouech (3):
      winedump: Better align fields in EXPORT table.
      quartz: Delay import ddraw.
      winedbg: Wait for gdb to terminate before exiting (proxy mode).

Esme Povirk (1):
      mscoree/tests: Add debug code for RemoveDirectory failure.

Fan WenJie (1):
      wineandroid: Fix incorrect checking reason.

Gabriel Ivăncescu (1):
      winex11: Use the correct root window for virtual desktops.

Lauri Kenttä (2):
      po: Update Finnish translation.
      readme: Update Finnish translation.

Rémi Bernon (2):
      dmime: Avoid leaking track references in segment Clone and Load.
      dmloader: Avoid caching DMUS_OBJ_STREAM objects we can't load from cache.

Yuxuan Shui (1):
      dmime: Fix handling of curve PMSG.

Zebediah Figura (3):
      wined3d: Reference FFP resources in reference_shader_resources().
      wined3d: Do not remove invalid BO users from the list when destroying views.
      wined3d: Set fixed_function_usage_map to 0 for an sm4 draw without a PS.

Zsolt Vadasz (2):
      msvcrt: Compare environment variable names case insensitively.
      msvcrt/tests: Test case insensitivity of getenv() and _wgetenv().
```
