#!/usr/bin/env python3
"""Restricted, audited Windows guest-agent control for AILinux reference VM."""
# SPDX-License-Identifier: LGPL-2.1-or-later
import argparse
import base64
import json
import subprocess
import sys
import time

VM = "ailinux-win11-reference"
CONNECTION = "qemu:///system"
MAX_POLL = 45
# All operations are static, with no caller-provided command strings.
ACTIONS = {
    "build": (
        "$os=Get-CimInstance Win32_OperatingSystem;"
        "'WINDOWS_BUILD='+$os.BuildNumber;"
        "'WINDOWS_VERSION='+$os.Version;"
        "'OS_CAPTION='+$os.Caption"
    ),
    "users": (
        "$u=Get-LocalUser -Name 'ailinux-test' -ErrorAction Stop;"
        "$a=Get-LocalGroupMember -SID 'S-1-5-32-544';"
        "$isadmin=@($a|Where-Object {$_.Name -like '*\\ailinux-test'}).Count -gt 0;"
        "'STANDARD_USER_EXISTS='+($u.Enabled -and -not $isadmin)"
    ),
    "guest-agent": "'POWERSHELL_GUEST_AGENT_BRIDGE_OK'",
}

def qga(request):
    proc = subprocess.run(
        ["virsh", "-c", CONNECTION, "qemu-agent-command", VM,
         json.dumps(request, separators=(",", ":")), "--timeout", "10"],
        text=True, capture_output=True, timeout=22, check=False)
    if proc.returncode != 0:
        raise RuntimeError("Guest agent unavailable or command failed: " + proc.stderr.strip())
    result = json.loads(proc.stdout)
    if "error" in result:
        raise RuntimeError("QEMU guest agent error: " + str(result["error"]))
    return result.get("return")

def run_command(action):
    command = ACTIONS[action]
    payload = {
        "execute": "guest-exec",
        "arguments": {
            "path": "C:\\Windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe",
            "arg": ["-NoProfile", "-NonInteractive", "-Command", command],
            "capture-output": True,
        }
    }
    result = qga(payload)
    pid = result.get("pid")
    if not isinstance(pid, int) or pid <= 0:
        raise RuntimeError("QEMU agent did not return guest process ID")
    for _ in range(MAX_POLL):
        info = qga({"execute": "guest-exec-status", "arguments": {"pid": pid}})
        if info.get("exited"):
            def output(key):
                raw = info.get(key, "")
                return base64.b64decode(raw, validate=True).decode("utf-8", errors="replace") if raw else ""
            stdout, stderr = output("out-data"), output("err-data")
            if stderr:
                print("GUEST_STDERR:\n" + stderr[:4000], file=sys.stderr)
            if info.get("exitcode") != 0:
                raise RuntimeError(f"Guest PowerShell returned {info.get('exitcode')}")
            return stdout[:16000]
        time.sleep(1)
    raise TimeoutError("Guest command exceeded polling limit; process may still be active")

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=["ping", *ACTIONS.keys()])
    args = parser.parse_args()
    if args.action == "ping":
        qga({"execute": "guest-ping"})
        print("QEMU_GUEST_AGENT_OK")
    else:
        print(run_command(args.action), end="")

if __name__ == "__main__":
    try:
        main()
    except (RuntimeError, TimeoutError, OSError, ValueError, KeyError) as error:
        print("GUEST_CONTROL_UNAVAILABLE:", error, file=sys.stderr)
        sys.exit(2)
