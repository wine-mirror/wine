#!/usr/bin/env python3
"""Isolated, reversible DXVK/VKD3D-Proton DLL manager for AILinux Wine.

SPDX-License-Identifier: LGPL-2.1-or-later
Never operates on ~/.wine or system DLLs. Does not modify the registry.
Only the "run" subcommand enables native DLL overrides for its process.
"""
import argparse
import datetime
import fcntl
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import urllib.request

LOCKFILE = Path(__file__).with_name("runtimes.lock.json")
HOME = Path.home().resolve()
MANAGER_SUBDIR = Path(".ailinux-wine/runtime-manager")
MAX_DLL_BYTES = 150 * 1024 * 1024


class RuntimeErrorSafe(Exception):
    pass


def digest_file(path):
    h = hashlib.sha256()
    with open(path, "rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def digest_bytes(data):
    return hashlib.sha256(data).hexdigest()


def prefix_path(value):
    # Require intentional explicit path, never act on Wine's default prefix.
    if not value:
        raise RuntimeErrorSafe("--prefix is required; ~/.wine is forbidden")
    supplied = Path(value).expanduser().absolute()
    if supplied.is_symlink():
        raise RuntimeErrorSafe("Wine prefix itself is a symlink")
    prefix = supplied.resolve(strict=True)
    if prefix == HOME / ".wine" or prefix == HOME or not prefix.is_relative_to(HOME):
        raise RuntimeErrorSafe("Use an isolated Wine prefix located below your home directory (not ~/.wine)")
    if not (prefix / "system.reg").is_file():
        raise RuntimeErrorSafe("Not an initialized Wine prefix: system.reg absent")
    if not (prefix / "drive_c/windows/system32").is_dir():
        raise RuntimeErrorSafe("Not an initialized Wine prefix: system32 absent")
    return prefix


def supported_architectures(prefix):
    reg_header = (prefix / "system.reg").read_bytes()[:1000]
    if b"#arch=win32" in reg_header:
        return {"x86": "drive_c/windows/system32"}
    if not (prefix / "drive_c/windows/syswow64").is_dir():
        raise RuntimeErrorSafe("64-bit prefix must have syswow64 for 32-bit PE DLLs")
    return {"x86": "drive_c/windows/syswow64", "x64": "drive_c/windows/system32"}


def validate_parent(prefix, destination):
    parent = destination.parent.resolve(strict=True)
    if not parent.is_relative_to(prefix):
        raise RuntimeErrorSafe("Destination parent escapes Wine prefix")
    if destination.is_dir():
        raise RuntimeErrorSafe("Refusing to replace a DLL directory")


def get_lock():
    with open(LOCKFILE, encoding="utf-8") as stream:
        lock = json.load(stream)
    assert lock["schema"] == 1
    return lock["components"]


def get_archive(component, cache):
    entry = get_lock()[component]
    cache.mkdir(parents=True, exist_ok=True)
    file_path = cache / entry["file"]
    expected = entry["sha256"]
    if file_path.exists() and digest_file(file_path) == expected:
        print("Verified cached archive:", file_path)
        return file_path
    if file_path.exists():
        raise RuntimeErrorSafe("Cached release archive has wrong SHA256: " + str(file_path))
    print("Downloading pinned upstream release:", entry["url"], flush=True)
    with tempfile.NamedTemporaryFile(prefix=".download-", dir=cache, delete=False) as tmp:
        temp_name = Path(tmp.name)
    try:
        with urllib.request.urlopen(entry["url"], timeout=45) as source, open(temp_name, "wb") as sink:
            shutil.copyfileobj(source, sink)
        if digest_file(temp_name) != expected:
            raise RuntimeErrorSafe("GitHub release SHA256 verification failed")
        os.replace(temp_name, file_path)
    finally:
        temp_name.unlink(missing_ok=True)
    print("SHA256 matched official pinned release digest.")
    return file_path


def read_pe_dll(archive, entry, arch, name):
    member = entry["archive_root"] + "/" + entry["source_dirs"][arch] + "/" + name + ".dll"
    command = ["tar"]
    if archive.name.endswith(".zst"):
        command.append("--zstd")
    command += ["-xOf", str(archive), member]
    proc = subprocess.run(command, capture_output=True, timeout=45)
    if proc.returncode:
        raise RuntimeErrorSafe("Missing DLL in trusted archive: " + member + ": " +
                               proc.stderr.decode(errors="replace")[:180])
    data = proc.stdout
    if len(data) < 100 or len(data) > MAX_DLL_BYTES or data[:2] != b"MZ":
        raise RuntimeErrorSafe("Invalid Windows PE DLL data: " + member)
    pe_offset = struct.unpack_from("<I", data, 0x3C)[0]
    if pe_offset + 6 > len(data) or data[pe_offset:pe_offset + 4] != b"PE\0\0":
        raise RuntimeErrorSafe("Invalid PE signature: " + member)
    machine = struct.unpack_from("<H", data, pe_offset + 4)[0]
    expected_machine = 0x014C if arch == "x86" else 0x8664
    if machine != expected_machine:
        raise RuntimeErrorSafe("Unexpected PE architecture for " + member)
    return data


def atomic_write_bytes(path, data):
    with tempfile.NamedTemporaryFile(prefix=".ailinux-wine-", dir=path.parent, delete=False) as tmp:
        name = Path(tmp.name)
        try:
            tmp.write(data)
            tmp.flush()
            os.fsync(tmp.fileno())
        except Exception:
            name.unlink(missing_ok=True)
            raise
    try:
        os.replace(name, path)
    finally:
        name.unlink(missing_ok=True)


def atomic_write_json(path, data):
    atomic_write_bytes(path, (json.dumps(data, indent=2, sort_keys=True) + "\n").encode("utf-8"))


def state_path(prefix, component):
    return prefix / MANAGER_SUBDIR / component / "state.json"


def load_state(prefix, component):
    path = state_path(prefix, component)
    if not path.is_file():
        return None
    return json.loads(path.read_text(encoding="utf-8"))


def install(prefix, component, cache):
    if load_state(prefix, component) is not None:
        raise RuntimeErrorSafe(component + " already installed; remove it first to avoid losing backups")
    entry = get_lock()[component]
    archive = get_archive(component, cache)
    records = []
    # Validate and read everything before changing the prefix.
    for arch, targetdir in supported_architectures(prefix).items():
        for name in entry["dlls"]:
            dest_relative = targetdir + "/" + name + ".dll"
            destination = prefix / dest_relative
            validate_parent(prefix, destination)
            data = read_pe_dll(archive, entry, arch, name)
            records.append({"dest": dest_relative, "sha256": digest_bytes(data), "data": data})
    manager = state_path(prefix, component).parent
    if manager.exists():
        raise RuntimeErrorSafe("Residual state folder exists. Inspect before proceeding: " + str(manager))
    backups = manager / "backup"
    manager.mkdir(parents=True)
    applied = []
    try:
        for rec in records:
            dest = prefix / rec["dest"]
            backup = backups / rec["dest"]
            backup.parent.mkdir(parents=True, exist_ok=True)
            had_original = dest.exists() or dest.is_symlink()
            rec["had_original"] = had_original
            # Rename preserves original symlinks and regular file metadata.
            if had_original:
                os.replace(dest, backup)
            applied.append(rec)
            atomic_write_bytes(dest, rec["data"])
        state = {
            "component": component,
            "version": entry["version"],
            "archive_sha256": entry["sha256"],
            "installed_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
            "files": [
                {key: rec[key] for key in ("dest", "sha256", "had_original")}
                for rec in records
            ],
        }
        atomic_write_json(manager / "state.json", state)
    except Exception:
        # Any exception restores all original files, including Wine's symlinks.
        for rec in reversed(applied):
            dest = prefix / rec["dest"]
            backup = backups / rec["dest"]
            if dest.exists() or dest.is_symlink():
                dest.unlink()
            if rec["had_original"] and (backup.exists() or backup.is_symlink()):
                os.replace(backup, dest)
        shutil.rmtree(manager, ignore_errors=True)
        raise
    print("INSTALLED", component, entry["version"], "DLL files:", len(records))


def verify_state(prefix, component, state):
    for rec in state["files"]:
        dest = prefix / rec["dest"]
        validate_parent(prefix, dest)
        if dest.is_symlink() or not dest.is_file() or digest_file(dest) != rec["sha256"]:
            raise RuntimeErrorSafe("Installed runtime DLL has changed: " + str(dest))
        backup = state_path(prefix, component).parent / "backup" / rec["dest"]
        if rec["had_original"] and not (backup.exists() or backup.is_symlink()):
            raise RuntimeErrorSafe("Original DLL backup missing: " + str(backup))


def remove(prefix, component):
    state = load_state(prefix, component)
    if not state:
        print("NOT_INSTALLED", component)
        return
    verify_state(prefix, component, state)
    folder = state_path(prefix, component).parent
    for rec in reversed(state["files"]):
        dest = prefix / rec["dest"]
        backup = folder / "backup" / rec["dest"]
        dest.unlink()
        if rec["had_original"]:
            os.replace(backup, dest)
    shutil.rmtree(folder)
    print("RESTORED", component, "original DLLs; removed runtime")


def status(prefix):
    unhealthy = 0
    components = get_lock()
    for component in components:
        state = load_state(prefix, component)
        if not state:
            print(component, "not installed")
            continue
        try:
            verify_state(prefix, component, state)
            print(component, state["version"], "healthy", len(state["files"]), "files")
        except RuntimeErrorSafe as exc:
            print(component, "BROKEN:", exc)
            unhealthy += 1
    return unhealthy


def run(prefix, wine, command):
    components = get_lock()
    active = []
    for component, entry in components.items():
        state = load_state(prefix, component)
        if state:
            verify_state(prefix, component, state)
            active.extend(entry["dlls"])
    if not command:
        raise RuntimeErrorSafe("Missing Windows program after -- separator")
    exe = Path(wine).expanduser().resolve(strict=True)
    if not exe.is_file() or not os.access(exe, os.X_OK):
        raise RuntimeErrorSafe("--wine must be the full path to an executable Wine binary")
    env = os.environ.copy()
    env["WINEPREFIX"] = str(prefix)
    overrides = [f"{name}=n" for name in sorted(set(active))]
    if overrides:
        previous = env.get("WINEDLLOVERRIDES", "")
        env["WINEDLLOVERRIDES"] = previous + (";" if previous else "") + ";".join(overrides)
    if command[0] == "--":
        command = command[1:]
    print("WINEPREFIX:", prefix, flush=True)
    print("Active runtime components:", ", ".join(c for c in components if load_state(prefix, c)), flush=True)
    return subprocess.run([str(exe), *command], env=env, check=False).returncode


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("install", "remove", "status", "run", "verify-cache"))
    parser.add_argument("--prefix", required=True, help="Explicit preinitialized isolated prefix, never ~/.wine")
    parser.add_argument("--component", choices=("dxvk", "vkd3d-proton", "all"), default="all")
    parser.add_argument("--cache", default=str(HOME / ".cache/ailinux-wine/runtimes"))
    parser.add_argument("--wine", default="/usr/bin/wine", help="Absolute Wine binary, for run only")
    parser.add_argument("command", nargs=argparse.REMAINDER)
    args = parser.parse_args()
    prefix = prefix_path(args.prefix)
    manager_dir = prefix / MANAGER_SUBDIR
    manager_dir.mkdir(parents=True, exist_ok=True)
    components = list(get_lock()) if args.component == "all" else [args.component]
    lockfile = manager_dir / "operation.lock"
    with open(lockfile, "a+b") as lock_handle:
        fcntl.flock(lock_handle, fcntl.LOCK_EX)
        if args.action == "install":
            for component in components:
                install(prefix, component, Path(args.cache).expanduser())
        elif args.action == "remove":
            for component in reversed(components):
                remove(prefix, component)
        elif args.action == "status":
            if status(prefix):
                raise RuntimeErrorSafe("One or more installed runtimes failed integrity verification")
        elif args.action == "verify-cache":
            for component in components:
                get_archive(component, Path(args.cache).expanduser())
        elif args.action == "run":
            return run(prefix, args.wine, args.command)
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (RuntimeErrorSafe, OSError, subprocess.TimeoutExpired) as exc:
        print("AILinux Wine runtime error:", exc, file=sys.stderr)
        sys.exit(2)
