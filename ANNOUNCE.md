The Wine development release 10.0-rc2 is now available.

What's new in this release:
  - Bug fixes only, we are in code freeze.

The source is available at <https://dl.winehq.org/wine/source/10.0/wine-10.0-rc2.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-10.0-rc2/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 10.0-rc2 (total 21):

 - #28861  Final Fantasy XI hangs after character selection
 - #47640  No Man's Sky (Beyond) does not start anymore: Unable to initialize Vulkan (vkEnumerateInstanceExtensionProperties failed)
 - #51998  Unable to start CloneCD
 - #53453  Command & Conquer 3: Tiberium Wars - fails to start (splash screen not even shown)
 - #54437  ntoskrnl.exe:ntoskrnl breaks test_rawinput() [(RIM|WM)_INPUT] in user32:input for non-English locales on Windows 7
 - #55583  d3d8:device - test_wndproc() often is missing a WM_WINDOWPOSCHANGING in Wine
 - #56056  Exiting IrfanView full screen mode creates a redundant task bar "Untitled window" item which is not clickable
 - #56325  Prefix path string in wineboot dialog is cut off
 - #56940  vs_community.exe halts:"The application cannot find one of its required files, possibly because it was unable to create it in the folder."
 - #57216  Mouse wheel input in IL-2 1946 is not applied consistently to UI elements and throttle
 - #57285  Foxit Reader - maximized view don't work properly
 - #57384  The shareware installer for Daytona (16-bit) hangs at the end of installing.
 - #57418  PlayOnline Viewer throws an application error at launch.
 - #57442  Several applications: abnormal input delay with Wine
 - #57481  Prey (2016) X11 fullscreen fails in 9.22
 - #57503  World in conflict has a frozen screen -  updating only when alt-tabbing out and in
 - #57504  Possible regression with Unity3D games: Framedrops when moving cursor.
 - #57506  Wine doesn't show any window
 - #57524  Commit c9592bae7f475c1b208de0a72ed29e94e3017094 breaks VKB Gladiator HIDRAW support
 - #57527  Drop-down list appears behind the main window
 - #57530  Regression: Tiny extra form displays in Delphi programs

### Changes since 10.0-rc1:
```
Alexandre Julliard (7):
      user32: Fixup forwarded functions on 32-bit.
      ntoskrnl: Support relative driver paths.
      ntoskrnl: Fix off-by-one error in buffer size.
      wineboot: Always wrap the wait dialog text.
      wineboot: Resize the wait dialog to accommodate the text size.
      wineboot: Scale the wait dialog icon with the dialog size.
      winetest: Filter out color escapes for junit output.

Eric Pouech (2):
      dbghelp: Fix error handling in PDB/FPO unwinder.
      dbghelp: Lower vector allocation for local variables.

Esme Povirk (1):
      mscoree: Use correct variable for codebase path.

Gabriel Ivăncescu (1):
      mshtml: Remove unused MutationObserver DISPID and related hook.

Gerald Pfeifer (1):
      webservices: Rename a struct member from bool to boolean.

Jacek Caban (1):
      configure: Define _load_config_used symbol in the cross-compiler test program.

Louis Lenders (1):
      shell32: Remove trailing spaces in SHELL_execute.

Marcus Meissner (1):
      ucrtbase/tests: Use correct size to GetEnvironmentVariableW.

Nikolay Sivov (8):
      windowscodecs/tests: Use test context in a few metadata tests.
      windowscodecs/tests: Add some tests for GetContainerFormats().
      windowscodecs/tests: Use a helper instead of a macro.
      windowscodecs/tests: Remove endianess compile time checks from the tests.
      windowscodecs/tests: Move IFD data tests to a helper.
      windowscodecs/tests: Run data test on the Exif reader.
      windowscodecs/tests: Add some tests for the Gps reader.
      windowscodecs/tests: Add some tests for the App1 reader.

Piotr Caban (1):
      msvcrt: Don't leak find handle or error in _findfirst().

Rémi Bernon (17):
      win32u: Skip updating the cache on driver load if we're already updating it.
      win32u: Release the Win16 mutex when yielding in peek_message.
      win32u: Copy the shape from the old surface when surface is recreated.
      server: Force surface region update when window region is modified.
      win32u: Extend display_lock CS around winstation check.
      server: Add a winstation monitor update serial counter.
      win32u: Use the winstation monitor update serial to detect updates.
      winex11: Request window config when it needs to be raised.
      winebus: Wait until the device is started before processing reports.
      dmloader: Remove redundant flag.
      winex11: Fixup window config size back to 0x0 if we've requested 1x1.
      winex11: Always check if the GL drawable offscreen state needs to be changed.
      winex11: Skip offscreening if the children don't require clipping.
      dinput: Queue the relative wheel motion as event data.
      explorer: Avoid hiding the taskbar if it's enabled.
      server: Allow merging WM_MOUSEMOVE across internal messages.
      winex11: Fix inconsistent coordinates when reparenting host window.

Vibhav Pant (3):
      winebth.sys: Fix new bluetooth events being incorrect set due to variable shadowing.
      winebth.sys: Set the Information field in the IRP's STATUS_BLOCK after handling IOCTL_BTH_GET_LOCAL_INFO.
      winebth.sys: Use the correct byte-ordering for setting the radio's address property.

William Horvath (2):
      include: Use inline assembly on Clang MSVC mode in YieldProcessor().
      win32u: Check for driver events more often.

Zhiyi Zhang (1):
      win32u: Use width and height to check if the display mode is vertical.
```
