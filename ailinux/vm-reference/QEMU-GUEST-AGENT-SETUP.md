# QEMU Windows guest agent deployment for zombie-pc

Status: Host preconfigured; guest agent NOT yet active.

- The Windows VM is ailinux-win11-reference.
- Persistent libvirt domain configuration has a virtio serial channel
  named org.qemu.guest_agent.0, enabled on the NEXT guest reboot.
- Live channel hotplug failed because there is no active virtio-serial
  controller. Windows was not restarted, and its guest agent has not run.
- The new read-only tools ISO is attached as sdc, with volume AILINUX_QGA.
  Host media path: /var/lib/libvirt/images/ailinux-qga-tools.iso.
- Official virtio-win stable version 0.1.302 was downloaded from:
  https://fedorapeople.org/groups/virt/virtio-win/direct-downloads/stable-virtio/virtio-win.iso
- Upstream ISO SHA256:
  303f7ae40dad495d6ae474fdc571df58958a4dbc5c37a522d80f9a203867949d
- Windows guest agent MSI SHA256:
  19dcf8abc30f70c2eb6e87282601fd0476e4e0b19936f63d9c80be30db0796ed

## One-time guest installation

In an authorized elevated Windows PowerShell session logged in as
ailinux-admin, locate the AILINUX_QGA CD volume and execute the provided
Install-QemuGuestAgent.ps1 script from that CD. The script verifies SHA256,
installs the Windows 11 VirtIO serial driver and QEMU guest agent MSI, and
checks the installed Windows service. It does not capture passwords, enable
WinRM or Windows remote desktop, or interfere with Windows activation.

After Windows restarts normally, use the host verification command:

    virsh -c qemu:///system qemu-agent-command ailinux-win11-reference '{"execute":"guest-ping"}'

A successful guest-ping is required before claiming remote guest command
execution works. Actual Windows Update and Windows reference API measurements
have NOT yet been executed, and their results must never be fabricated.

The attempted automated credential entry was blocked by security checks.
Do not bypass those controls or expose credentials in command-line arguments,
version control, shell logs, screenshots or CI.

## Automated host-side preflight

Run `python3 -B ailinux/vm-reference/qga-preflight.py` on zombie-pc.
It verifies SHA256 of the approved Windows MSI and serial driver, checks
whether the QEMU agent channel and approved tools CD are present in BOTH
persistent and running VM configurations, and attempts the guest-ping.
It neither signs into Windows nor installs software behind the user's login.
On October 26 the results were: package hashes valid; persistent channel
present; live channel absent; guest ping unavailable. Therefore a normal
Windows guest agent installation and a guest restart remain mandatory.
