The Wine development release 9.0-rc5 is now available. This is
expected to be the last release candidate before the final 9.0.

What's new in this release:
  - Bug fixes only, we are in code freeze.

The source is available at <https://dl.winehq.org/wine/source/9.0/wine-9.0-rc5.tar.xz>

Binary packages for various distributions will be available
from <https://www.winehq.org/download>

You will find documentation on <https://www.winehq.org/documentation>

Wine is available thanks to the work of many people.
See the file [AUTHORS][1] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/raw/wine-9.0-rc5/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 9.0-rc5 (total 22):

 - #38780  AArch64 platforms: register X18 (TEB) must remain reserved for Wine to run 64-bit ARM Windows applications (Distro aarch64 toolchains need '-ffixed-x18' default, loader/libc/userland)
 - #46777  Two Worlds 2 crashes on start
 - #47406  Add support for debug symbols in separate files
 - #49852  Performance regression in "msvcrt: Use correct code page in _write when outputing to console."
 - #50998  Failed to open error Word 2007 (c0000135)
 - #52491  SpaceDesk fails to connect to server
 - #52962  dinput:force_feedback breaks ntoskrnl.exe:ntoskrnl on Windows 7/8
 - #53319  robot battle game extremely slow after wine version 7.11 and above
 - #54387  Pantsylvania crashes if Windows version is set to 3.1
 - #54401  PhotoFiltre not printing
 - #55010  psapi:psapi_main - test_EnumProcessModulesEx() sometimes gets a 0 image size on Windows 8
 - #55497  Jennifer is Missing (Katjas Geheimnis) by Tivoli crashes on start
 - #55731  advapi32:eventlog - test_eventlog_start() fails on Windows 7 & 10 2004 & 2009
 - #55784  wldap32:parse - test_ldap_bind_sA() claims the server is down on w1064v2009
 - #56070  BVE trainsim doesn't show its logo in the main window.
 - #56113  Unfortunate Spacemen crashes on start
 - #56117  Celtic Kings runs out of memory in mere seconds when music is enabled
 - #56130  Wine is broken on Termux since 8.17-39-g25db1c5d49d
 - #56134  VA-11 HALL-A crashes on startup
 - #56149  Celtic Kings demo: window decorations missing in virtual desktop (VD size = desktop size)
 - #56150  Wine 8.18 - Fedora 37 - Winwing F16EX joystick - dinput only reports 10 buttons
 - #56152  "Script error: Handler not defined #FileIO" in "TKKG 1" (EN: Jennifer is Missing, DE: Katjas Geheimnis - Tivoli)

### Changes since 9.0-rc4:
```
Akihiro Sagawa (5):
      d3d8/tests: Test the presentation parameters after creating a device.
      d3d9/tests: Test the presentation parameters after creating a device.
      d3d8/tests: Test the presentation parameters after creating an additional swap chain.
      d3d9/tests: Test the presentation parameters after creating an additional swap chain.
      d3d9: Update presentation parameters when creating a swap chain.

Alex Henrie (1):
      winspool: Keep driver_9x in scope while it is in use.

Alexandre Julliard (2):
      krnl386: Use NtContinue to restore the full context.
      krnl386: Align the stack before calling the entry point.

Bernhard Übelacker (1):
      msvcrt: Protect setlocale against concurrent accesses.

Enol Puente (1):
      po: Update Asturian translation.

Gabriel Ivăncescu (4):
      Revert "winex11: Use the correct root window for virtual desktops.".
      winex11: Set MWM_FUNC_RESIZE for fullscreen desktop windows.
      winex11: Update Virtual Desktop fullscreen WM state after setting window pos.
      winex11: Move the update_desktop_fullscreen callsite to update_net_wm_states.

Hans Leidekker (2):
      wldap32/tests: Skip tests when the server can't be reached.
      wininet/tests: Update expected winehq.org certificate.

Jacek Caban (1):
      gitlab: Cache config.cache in Clang builds.

Jinoh Kang (4):
      ntdll/tests: Fix x86-32 extended context end offset in test_copy_context().
      ntdll/tests: Fix incorrect calculation of context length in test_copy_context().
      ntdll/tests: Don't hard code the maximum XState length in test_extended_context().
      ntdll/tests: Fix xstate tests failing on Windows 11 and CPU with more XSAVE features.

Nikolay Sivov (2):
      mfreadwrite/tests: Skip tests if D3D9 is unusable.
      mf/tests: Skip tests if D3D9 is unusable.

Rémi Bernon (3):
      dmusic: Clone streams instead of allocating wave data.
      ntoskrnl.exe/tests: Use SUOI_FORCEDELETE when uninstalling the driver.
      winewayland: Add missing breaks in keyboard layout switch.

Zebediah Figura (2):
      wined3d: Avoid WARN() when failing to allocate a GL BO without a context.
      wined3d: Only suballocate dynamic buffers.
```
