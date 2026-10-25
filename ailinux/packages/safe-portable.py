#!/usr/bin/env python3
"""Safely install, inspect, update and remove SHA256-pinned portable Win32 ZIPs."""
# SPDX-License-Identifier: LGPL-2.1-or-later
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import stat
import sys
import tempfile
import uuid
import zipfile

MAX_FILES = 30000
MAX_SIZE = 4 * 1024**3
MAX_RATIO = 1000
NAME = re.compile(r"^[a-zA-Z0-9][a-zA-Z0-9._-]{0,79}$")
VERSION = re.compile(r"^[a-zA-Z0-9][a-zA-Z0-9.+_-]{0,63}$")
HEX_SHA = re.compile(r"^[0-9a-fA-F]{64}$")
HOME = Path.home().resolve()
ROOT = HOME / ".local/share/ailinux-wine/prefixes"
MARKER = ".ailinux-package.json"
SCHEMA = 1


class PackageError(ValueError):
    pass


def fail(message):
    raise PackageError(message)


def sha256_file(path):
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def validate_name(value):
    if not NAME.fullmatch(value or "") or value in (".", "..") or value.endswith("."):
        fail("Unsafe portable package name")
    reserved = ("con", "prn", "aux", "nul")
    if value.lower().split(".")[0] in reserved:
        fail("Reserved Windows package name")
    return value


def validate_prefix(value):
    path = Path(value).expanduser().absolute()
    if path.parent != ROOT or path.is_symlink() or not path.is_dir():
        fail("Prefix must be a real directory immediately under AILinux Wine prefixes")
    if path.resolve() != path or path.name.startswith("."):
        fail("Unsafe Wine prefix")
    registry = path / "system.reg"
    if registry.is_symlink() or not registry.is_file():
        fail("Wine prefix has no safe system.reg")
    return path


def package_base(prefix, create=False):
    drive = prefix / "drive_c"
    program = drive / "Program Files"
    base = program / "AILinuxPortable"
    for item in (drive, program, base):
        if item.is_symlink():
            fail("Symlink in Windows portable application directory")
    if not drive.is_dir():
        fail("Missing Wine drive_c")
    if program.exists() and not program.is_dir():
        fail("Invalid Windows Program Files directory")
    if base.exists() and not base.is_dir():
        fail("Invalid portable package directory")
    if create:
        base.mkdir(parents=True, exist_ok=True)
    return base


def valid_member(path, is_dir):
    if not path or path.startswith(("/", "\\")) or len(path) > 600:
        fail("Invalid ZIP path")
    components = path.replace("\\", "/").split("/")
    if is_dir and components[-1] == "":
        components.pop()
    if not components or any(not x or x in (".", "..") for x in components):
        fail("ZIP contains traversal or empty path components")
    for part in components:
        if (any(ord(c) < 32 or c in '<>:"|?*' for c in part) or
                part.endswith((" ", ".")) or len(part) > 255):
            fail("Invalid Windows filename in ZIP")
        if part.lower().split(".")[0] in ("con", "prn", "aux", "nul"):
            fail("Windows reserved device name")
        if re.fullmatch(r"(com|lpt)[1-9]", part.lower().split(".")[0]):
            fail("Windows reserved device name")
        if part.lower() == MARKER.lower():
            fail("ZIP cannot include package registry marker")
    return "/".join(components)


def zip_contents(path):
    if path.is_symlink() or not path.is_file():
        fail("Archive must be a regular file, not a symlink")
    with zipfile.ZipFile(path) as archive:
        entries = archive.infolist()
        if not entries or len(entries) > MAX_FILES:
            fail("ZIP entry count out of bounds")
        total = 0
        used = set()
        files = set()
        dirs = set()
        for entry in entries:
            mode = entry.external_attr >> 16
            if (stat.S_ISLNK(mode) or
                (mode and stat.S_IFMT(mode) not in (0, stat.S_IFREG, stat.S_IFDIR))):
                fail("ZIP contains special file or symlink")
            is_dir = entry.is_dir()
            relative = valid_member(entry.filename, is_dir)
            folded = relative.casefold()
            if folded in used:
                fail("ZIP has case-insensitive duplicate entries")
            used.add(folded)
            if entry.file_size > MAX_SIZE or entry.file_size > MAX_RATIO * max(1, entry.compress_size):
                fail("ZIP has excessive size or expansion ratio")
            total += entry.file_size
            if total > MAX_SIZE:
                fail("ZIP uncompressed size exceeds limit")
            (dirs if is_dir else files).add(relative)
        if any(f.casefold() in {d.casefold() for d in dirs} for f in files):
            fail("ZIP includes file-directory collision")
        known_files = {p.casefold() for p in files}
        for item in used:
            parts = item.split("/")
            for index in range(1, len(parts)):
                if "/".join(parts[:index]) in known_files:
                    fail("ZIP has a file as parent directory")
        if not any(name.lower().endswith(".exe") for name in files):
            fail("Portable Windows ZIP must contain an EXE")
        return entries


def validate_archive(path, expected_sha):
    if not HEX_SHA.fullmatch(expected_sha or ""):
        fail("SHA256 must be exactly 64 hex characters")
    if path.is_symlink() or not path.is_file():
        fail("Archive must be a regular file")
    digest = sha256_file(path)
    if digest.lower() != expected_sha.lower():
        fail("SHA256 does not match")
    entries = zip_contents(path)
    return digest, entries


def manifest_path(target):
    return target / MARKER


def read_manifest(target, name):
    if target.is_symlink() or not target.is_dir():
        fail("Missing or unsafe installed portable package")
    marker = manifest_path(target)
    if marker.is_symlink() or not marker.is_file():
        fail("Portable package was not managed by AILinux Wine")
    obj = json.loads(marker.read_text("utf-8"))
    if obj.get("schema") != SCHEMA or obj.get("name") != name:
        fail("Invalid installed package manifest")
    if not HEX_SHA.fullmatch(obj.get("sha256", "")):
        fail("Invalid package source checksum")
    if not isinstance(obj.get("files"), dict) or not obj["files"]:
        fail("No tracked files in package manifest")
    return obj


def audit(target, manifest):
    expected = manifest["files"]
    seen = set()
    for item in target.rglob("*"):
        if item.is_symlink():
            fail("Installed package contains symlink; refusing mutation")
        if item.is_dir():
            continue
        if not item.is_file():
            fail("Installed package contains special file")
        relative = item.relative_to(target).as_posix()
        if relative == MARKER:
            continue
        if relative not in expected:
            fail("Untracked user file in package; refusing mutation: " + relative)
        if sha256_file(item) != expected[relative]:
            fail("Package file was modified; refusing mutation: " + relative)
        seen.add(relative)
    if seen != set(expected):
        fail("Installed package file inventory does not match manifest")
    return True


def extract(archive_path, entries, stage):
    files = {}
    with zipfile.ZipFile(archive_path) as archive:
        for entry in entries:
            relative = valid_member(entry.filename, entry.is_dir())
            target = stage.joinpath(*relative.split("/"))
            if entry.is_dir():
                target.mkdir(parents=True, exist_ok=True)
                continue
            target.parent.mkdir(parents=True, exist_ok=True)
            digest = hashlib.sha256()
            count = 0
            with archive.open(entry) as src, target.open("xb") as dst:
                while True:
                    block = src.read(1024 * 1024)
                    if not block:
                        break
                    count += len(block)
                    if count > entry.file_size:
                        fail("ZIP decompressed beyond declared file size")
                    digest.update(block)
                    dst.write(block)
            if count != entry.file_size:
                fail("ZIP file size mismatch")
            files[relative] = digest.hexdigest()
    return files


def install(prefix, source, sha, name, version, is_update=False):
    validate_name(name)
    if not VERSION.fullmatch(version):
        fail("Invalid package version")
    digest, entries = validate_archive(source, sha)
    base = package_base(prefix, create=True)
    target = base / name
    if target.is_symlink():
        fail("Symlink at portable package destination")
    if is_update:
        old = read_manifest(target, name)
        audit(target, old)
        if old["sha256"] == digest:
            fail("Installed archive already matches candidate")
    elif target.exists():
        fail("Portable package already installed; use update only after review")
    stage = Path(tempfile.mkdtemp(dir=base, prefix=".staging-"))
    try:
        manifest = {
            "schema": SCHEMA, "name": name, "version": version,
            "type": "portable-zip", "sha256": digest,
            "verification": "payload-only", "files": extract(source, entries, stage)
        }
        manifest_path(stage).write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", "utf-8")
        if is_update:
            displaced = base / (".old-" + uuid.uuid4().hex)
            os.replace(target, displaced)
            try:
                os.replace(stage, target)
            except BaseException:
                os.replace(displaced, target)
                raise
            shutil.rmtree(displaced)
        else:
            os.replace(stage, target)
    finally:
        if stage.exists():
            shutil.rmtree(stage)
    print(("PORTABLE_UPDATED=" if is_update else "PORTABLE_INSTALLED=") + str(target))
    print("PORTABLE_MANIFEST=" + str(manifest_path(target)))


def remove(prefix, name):
    validate_name(name)
    base = package_base(prefix)
    target = base / name
    manifest = read_manifest(target, name)
    audit(target, manifest)
    displaced = base / (".removed-" + uuid.uuid4().hex)
    os.replace(target, displaced)
    shutil.rmtree(displaced)
    print("PORTABLE_REMOVED=" + name)


def list_packages(prefix):
    base = package_base(prefix)
    packages = []
    if base.is_dir():
        for directory in sorted(base.iterdir()):
            if directory.name.startswith(".") or directory.is_symlink() or not directory.is_dir():
                continue
            try:
                name = validate_name(directory.name)
                obj = read_manifest(directory, name)
                packages.append({
                    "name": name, "version": obj.get("version", "unknown"),
                    "sha256": obj["sha256"], "type": obj.get("type", "unknown"),
                    "verification": obj.get("verification", "unknown")
                })
            except (PackageError, ValueError, OSError, json.JSONDecodeError):
                # Unknown directories are not presented as managed packages.
                continue
    print(json.dumps(packages, sort_keys=True))


def check_update(prefix, name, sha):
    validate_name(name)
    if not HEX_SHA.fullmatch(sha or ""):
        fail("Candidate must have an exact SHA256")
    current = read_manifest(package_base(prefix) / name, name)
    print(json.dumps({
        "name": name, "current_sha256": current["sha256"],
        "candidate_sha256": sha.lower(),
        "update_available": current["sha256"].lower() != sha.lower(),
        "source": "local candidate checksum only",
        "verification": current.get("verification")
    }, sort_keys=True))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--action", default="install",
                        choices=("install", "update", "remove", "list", "audit", "check-update"))
    parser.add_argument("--prefix", required=True)
    parser.add_argument("--archive")
    parser.add_argument("--sha256")
    parser.add_argument("--name")
    parser.add_argument("--version", default="unknown")
    args = parser.parse_args()
    prefix = validate_prefix(args.prefix)
    if args.action == "list":
        list_packages(prefix)
        return
    name = validate_name(args.name)
    if args.action == "remove":
        remove(prefix, name)
    elif args.action == "audit":
        target = package_base(prefix) / name
        manifest = read_manifest(target, name)
        audit(target, manifest)
        print("PORTABLE_AUDIT_OK=" + name)
    elif args.action == "check-update":
        check_update(prefix, name, args.sha256)
    else:
        if not args.archive or not args.sha256:
            fail("Archive path and SHA256 required")
        install(prefix, Path(args.archive).expanduser().absolute(), args.sha256,
                name, args.version, args.action == "update")


if __name__ == "__main__":
    try:
        main()
    except (PackageError, OSError, ValueError, zipfile.BadZipFile, EOFError,
            json.JSONDecodeError) as exc:
        print("Portable ZIP manager rejected operation:", exc, file=sys.stderr)
        sys.exit(2)
