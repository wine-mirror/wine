#!/usr/bin/env python3
"""Read-only Wine Windows 11 runtime inventory for a selected prefix."""
import argparse
import json
from pathlib import Path
import re
import subprocess
import os
import sys

parser = argparse.ArgumentParser()
parser.add_argument("--wine", required=True)
parser.add_argument("--prefix", required=True)
args = parser.parse_args()
home = Path.home().resolve()
wine = Path(args.wine).expanduser().resolve(strict=True)
prefix = Path(args.prefix).expanduser().resolve(strict=True)
if prefix == home / ".wine" or not prefix.is_relative_to(home) or not (prefix / "system.reg").exists():
    sys.exit("Refusing default or uninitialized Wine prefix")
env = {**os.environ, "WINEPREFIX": str(prefix), "WINEDEBUG": "-all"}
def reg(path, key):
    result = subprocess.run([str(wine), "reg", "query", path, "/v", key],
                            env=env, capture_output=True, text=True, errors="replace", timeout=25)
    if result.returncode:
        return None
    m = re.search(r"\b" + re.escape(key) + r"\s+REG_\w+\s+([^\r\n]+)", result.stdout, re.I)
    return m.group(1).strip() if m else None
def dll_file(directory, dll):
    path = prefix / "drive_c" / "windows" / directory / dll
    return {"exists": path.exists(), "symlink": path.is_symlink(), "bytes": path.stat().st_size if path.exists() else 0}
runtimes = {}
for arch in ("x86", "x64"):
    base = "HKLM\\Software\\Microsoft\\VisualStudio\\14.0\\VC\\Runtimes\\" + arch
    runtimes[arch] = {"registry_version": reg(base, "Version"),
                      "installed": reg(base, "Installed"),
                      "vcruntime140": dll_file("syswow64" if arch == "x86" else "system32", "vcruntime140.dll"),
                      "msvcp140": dll_file("syswow64" if arch == "x86" else "system32", "msvcp140.dll")}
dotnet = reg("HKLM\\Software\\Microsoft\\NET Framework Setup\\NDP\\v4\\Full", "Release")
release = int(dotnet, 0) if dotnet else None
version = ("4.8.1+" if release and release >= 533320 else
           "4.8+" if release and release >= 528040 else
           "below 4.8 or unknown")
try:
    os_version = subprocess.run([str(wine), "cmd", "/c", "ver"], env=env, capture_output=True, text=True, timeout=25).stdout.strip()
except subprocess.TimeoutExpired:
    os_version = "timeout"
result = {"wine": subprocess.run([str(wine), "--version"], capture_output=True, text=True, timeout=8).stdout.strip(),
          "prefix": str(prefix), "windows_compat_version": os_version,
          "vc14": runtimes,
          "dotnet_framework_release": release,
          "dotnet_framework_registry_interpretation": version,
          "dotnet48_winetricks_marker": (prefix / "drive_c/windows/dotnet48.installed.workaround").exists(),
          "note": "Registry data and DLL presence do not prove full functionality."}
print(json.dumps(result, indent=2, ensure_ascii=False))
