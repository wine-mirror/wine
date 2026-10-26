# Windows 11 reference VM for AILinux Wine

VM name: ailinux-win11-reference on zombie-pc, libvirt/QEMU with 4 CPUs,
8 GiB RAM, a 112 GiB sparse disk, Microsoft Secure Boot UEFI, TPM 2.0.
No network device; VNC is loopback-only. Autostart is disabled.

The local Windows 11 installation ISO is attached. The guest OS installation
and license activation must still be completed. The VM is not an installed,
validated Windows 11 system until that succeeds.

Run build-and-test.sh on Linux to build Windows PE x86 and x64 probes and
produce Wine output JSON. Use guest-media.sh to make a read-only ISO of those
PEs plus Run-WindowsReference.ps1. The Windows-side PowerShell script writes
Windows results to the user's desktop. Copy the resulting JSON files back
to a separately reviewed Linux result directory and run compare-probes.py.
Export presence is NOT proof of correct behavior or guaranteed app support.

Never redistribute Windows system files, circumvent activation, or export
proprietary binaries. No host home directory or credentials are mounted inside
the guest. A restricted transfer directory exists under
/var/lib/libvirt/ailinux-reference; Windows VirtioFS drivers and a working
bidirectional mount are not yet configured.

## Host convenience commands

- vm-control.sh status: inspect guest state and loopback-only VNC console
- vm-control.sh view: open the VM viewer on zombie-pc
- vm-control.sh start / shutdown: normal lifecycle (no force reset)
- vm-control.sh push-probes: compile and copy reviewed test programs to inbox
- vm-control.sh compare: compare Wine JSON against Windows JSON in outbox

Two VirtioFS shares are configured for the NEXT VM boot:
ailinux-in (read-only) and ailinux-out (separate writable output directory).
They are limited to /var/lib/libvirt/ailinux-reference/inbox and outbox.
Windows VirtioFS drivers are required before these appear as guest volumes.
The read-only tools ISO is also attached on NEXT boot. Neither guest volume
mounting nor guest-side test execution has been confirmed.

The Windows setup still needs completion and valid licensing. The VM was
started but Windows installation and activation were NOT automated.
The Linux production Wine prefix was not modified.

## Viewer from SSH/automation shells

KDE Plasma may be active while SSH/MCP shells have no DISPLAY. The view
command now reads only enumerated graphics variables from the current user's
systemd session environment, validates Xwayland via xwininfo and selects
GDK_BACKEND=x11; otherwise it tries a live Wayland socket. It does not source
or eval session data. If neither display works, it exits with a diagnostic.

On zombie-pc the graphical smoke test opened a real virt-viewer window named
ailinux-win11-reference (1) on Xwayland DISPLAY=:1, while the VM remained
running. Graphical login must already exist. This does not install Windows
inside the guest automatically.

## Updated conformance comparison (October 26, 2026)

The same 32- and 64-bit native PE source sets now run two categories of tests:
- winapi-probe-i686.exe and winapi-probe-x86_64.exe: JSON API export inventory.
- win11-api-probe-i686.exe and win11-api-probe-x86_64.exe: five behavioral
  probes for AppModel, WinRT HSTRING, Media Foundation, DPI and WinHTTP.

Both behavioral executables passed 5/5 under own-built Wine 11.20.
Genuine Windows 11 results have NOT yet been obtained. The comparison tool
rejects mismatched x86/x64 data and reports API gaps, but an export match
does not prove that a function behaves correctly.

The VM has no configured QEMU guest agent. Host virsh cannot launch
PowerShell inside the Windows guest. Guest-side scripts require an authorized
Windows login and execution inside that session. No guest credentials,
proprietary Windows DLLs, activation bypass or backdoor are included here.
