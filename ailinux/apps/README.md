# Windows-style applications in AILinux Wine

Linux-native PowerShell 7.6.9 controls this catalog and launches Wine built-in
Windows-compatible applications by **fixed identifier**. It never treats a
user-supplied executable as a command and never copies proprietary Windows 11
system files into the fork.

    ~/.local/opt/ailinux-powershell/7.6.9/pwsh -NoProfile -File ailinux/powershell/Manage-AILinuxWineApps.ps1 -Action list
    ~/.local/opt/ailinux-powershell/7.6.9/pwsh -NoProfile -File ailinux/powershell/Manage-AILinuxWineApps.ps1 -Action check -App notepad
    ~/.local/opt/ailinux-powershell/7.6.9/pwsh -NoProfile -File ailinux/powershell/Manage-AILinuxWineApps.ps1 -Action launch -App notepad -WhatIf

The catalog is a capability inventory and launcher, not an MSIX/Store package
installer. Notepad, Explorer, Task Manager, Control Panel and cmd originate from
Wine source. They are not Microsoft Windows 11 binary equivalents.

The official PowerShell 7.6.9 x64 Windows ZIP was downloaded from
PowerShell/PowerShell GitHub Releases; SHA-256 verified against the release
asset digest, and extracted to C:\Program Files\PowerShell\7 in the dedicated
`win11-complete-alpha` prefix. However, while `pwsh.exe` returned exit code 0,
it did NOT write the expected output or marker file in a behavioral smoke test.
The Windows build is therefore marked `staged-unverified`, not launch-ready.
The separately installed native Linux PowerShell remains working.

The Windows Calculator, Photos, Terminal and recent Paint Store/MSIX apps are
marked unsupported. Their Windows packaging stack and WinRT services are not
provided by the Linux PowerShell management layer. Do not bypass licensing,
extract an installed Windows image, or pretend the Wine executable names imply
full Windows 11 functionality.

GUI smoke tests for built-in apps use an isolated Xvfb display, not the KDE
session. In October 2026, Notepad, Regedit and Wine Write opened their actual
X11 windows under the own-built Wine 11.20 in a disposable test prefix:

    WINEPREFIX="$HOME/.local/share/ailinux-wine/prefixes/gui-app-smoke-alpha" \
      ./ailinux/apps/gui-smoke.sh notepad

The WordPad app entry maps to Wine's `write.exe` because `wordpad.exe` is
present among build artifacts but is not installed under the configured
Windows paths of that prefix. The wrapper tests GUI window creation, not
editing, printing, saving documents or long-lived desktop operation.
