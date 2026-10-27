#!/usr/bin/env python3
"""Non-invasive Windows guest agent readiness audit on the Linux host."""
import hashlib
from pathlib import Path
import subprocess
import sys
import xml.etree.ElementTree as ET

NAME = "ailinux-win11-reference"
BASE = Path.home() / "workspace/.wine-build-state/windows-reference"
MEDIA = Path("/var/lib/libvirt/images/ailinux-qga-tools.iso")
EXPECTED = {
    "guest-agent/qemu-ga-x86_64.msi": "19dcf8abc30f70c2eb6e87282601fd0476e4e0b19936f63d9c80be30db0796ed",
    "vioserial/w11/amd64/vioser.inf": "a7bb3cc2482e0e4df281b506efb85ad4c76dd8d5a1522e480e9da354568006c0",
}

def virsh(*args):
    return subprocess.run(("virsh", "-c", "qemu:///system", *args),
                          capture_output=True, text=True, timeout=20)

def inspect_domain(inactive):
    args = ("dumpxml", "--inactive", NAME) if inactive else ("dumpxml", NAME)
    result = virsh(*args)
    if result.returncode:
        raise RuntimeError("VM metadata unavailable: " + result.stderr.strip())
    root = ET.fromstring(result.stdout)
    return {
        "agent_channel": any(node.attrib.get("name") == "org.qemu.guest_agent.0"
                             for node in root.findall("./devices/channel/target")),
        "media": any(MEDIA.as_posix() == node.attrib.get("file")
                     for node in root.findall("./devices/disk/source")),
    }

def main():
    for relative, expected in EXPECTED.items():
        path = BASE / "guest-agent-payload" / relative
        if not path.is_file():
            raise RuntimeError(f"Missing official package: {relative}")
        with path.open("rb") as handle:
            digest = hashlib.file_digest(handle, "sha256").hexdigest()
        if digest != expected:
            raise RuntimeError(f"Official package digest mismatch: {relative}")
    print("OFFICIAL_PACKAGE_HASHES=OK")
    for name, inactive in (("PERSISTENT", True), ("LIVE", False)):
        state = inspect_domain(inactive)
        print(f"{name}_QGA_CHANNEL={str(state['agent_channel']).lower()}")
        print(f"{name}_QGA_MEDIA={str(state['media']).lower()}")
    reply = virsh("qemu-agent-command", NAME, '{"execute":"guest-ping"}')
    if reply.returncode:
        print("GUEST_AGENT_PING=UNAVAILABLE")
        print("REQUIRED_NEXT_STEP=Install the agent inside authorized Windows administrator session, then restart guest.")
    else:
        print("GUEST_AGENT_PING=OK")

if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, ValueError) as error:
        print("QGA_PREFLIGHT_FAILED:", error, file=sys.stderr)
        sys.exit(2)
