# Windows 11 Pro reference installation

The VM ailinux-win11-reference has only a 112 GiB virtual SATA disk.
Windows Setup uses install.wim index 5 for the Windows 11 Pro edition
from the existing local German Windows 11 ISO. Physical host disks
are not passed through to the guest.

The private answer file lives only in the host directory
~/workspace/.wine-build-state/windows-reference/setup-secrets/.
It is not included in public source control. The answer file contains
randomly generated local guest account credentials:
- ailinux-test: standard Users group
- ailinux-admin: Administrators group, for guest maintenance

The local passwords are stored in setup-secrets/vm-users.txt (0600).
Do not paste passwords or keys into GitHub or log files.

The guest remains network-isolated; Windows activation was not bypassed.
Use Test-WindowsReference.ps1 only after Windows OOBE has completed to
confirm Windows edition, local users, account roles and PowerShell version.
The script collects no passwords or product keys.

The Win32 export probes for Windows-vs-Wine comparisons are separate.
API export presence alone does not establish behavioral compatibility.

## Windows Update target (verified October 26, 2026)

The currently supported general-release Windows 11 x64 feature line is
26H2, OS build 26300. Microsoft's October 13, 2026 quality/security
update KB5127648 targets build 26300.9711. The existing German ISO was
named Windows11_Client_x64_de-de_26300_9457.iso; check the actual installed
build with Windows PowerShell before and after applying updates.

The guest reached Windows sign-in and both ailinux-test and ailinux-admin
appeared. The original unattended-install CD carrying local credentials
was removed from BOTH active and persistent libvirt config. No secrets
are contained in the new tools CD.

Host-side libvirt default NAT was attached LIVE only via e1000e and DHCP
successfully allocated 192.168.122.185. A temporary nftables filter limits
guest access to public IP ranges and prevents routed access to private LANs
and host services other than bridge DHCP/DNS. This firewall is NOT persistent
across host reboot. The guest NIC is NOT persistent across VM restart.

As ailinux-admin, open an elevated Windows PowerShell window and run the
signed-by-Windows Windows Update Agent workflow from the tools CD:

    powershell.exe -NoProfile -File E:\Update-WindowsReference.ps1
    powershell.exe -NoProfile -File E:\Update-WindowsReference.ps1 -Install -Confirm:$false

Adjust E: to the actual tools-CD drive letter. The first command checks
only; the second downloads non-preview Microsoft software updates and installs
them using WUA. It does not automatically reboot. After reboot check
`Get-ComputerInfo | Select-Object WindowsVersion,OsBuildNumber` and confirm
`26300.9711` or later. If Windows Update cannot run, check guest internet,
Windows servicing state, and report results instead of claiming success.

No activation bypass, leaked credentials, host files, or system-Wine changes.

The reference tools CD (containing no credentials) was attached live and
persistently under `/var/lib/libvirt/images/ailinux-reference-tools-20261026.iso`.
The admin sign-in and KB5127648 installation are **NOT yet completed**.
The VMware/QEMU guest Windows build and updated OS build must be verified
from a guest PowerShell session before marking the task complete.
