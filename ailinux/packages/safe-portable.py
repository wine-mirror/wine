#!/usr/bin/env python3
"""Validate and extract a local, SHA256-pinned portable Windows ZIP safely."""
# SPDX-License-Identifier: LGPL-2.1-or-later
import argparse
import hashlib
import os
from pathlib import Path
import re
import shutil
import stat
import sys
import tempfile
import zipfile

MAX_FILES = 30000
MAX_SIZE = 4 * 1024**3
MAX_RATIO = 1000
NAME = re.compile(r"^[a-zA-Z0-9][a-zA-Z0-9._-]{0,79}$")
HOME = Path.home().resolve()
ROOT = HOME / ".local/share/ailinux-wine/prefixes"


def fail(message):
    raise ValueError(message)


def run():
    p = argparse.ArgumentParser()
    p.add_argument("--prefix", required=True)
    p.add_argument("--archive", required=True)
    p.add_argument("--sha256", required=True)
    p.add_argument("--name", required=True)
    args = p.parse_args()
    prefix = Path(args.prefix).absolute()
    archive = Path(args.archive).absolute()
    if prefix.parent != ROOT or prefix.is_symlink() or not (prefix / "system.reg").is_file():
        fail("Untrusted Wine prefix")
    if archive.is_symlink() or not archive.is_file():
        fail("Archive must be a regular file")
    if not NAME.fullmatch(args.name):
        fail("Invalid portable app name")
    if not re.fullmatch("[A-Fa-f0-9]{64}", args.sha256):
        fail("Expected exact SHA256")
    digest = hashlib.sha256(archive.read_bytes()).hexdigest()
    if digest.lower() != args.sha256.lower():
        fail("SHA256 mismatch")
    base = prefix / "drive_c/Program Files/AILinuxPortable"
    target = base / args.name
    if base.is_symlink() or target.exists() or target.is_symlink():
        fail("Destination already exists")
    total = 0
    with zipfile.ZipFile(archive) as z:
        infos = z.infolist()
        if not infos or len(infos) > MAX_FILES:
            fail("Archive entry count out of bounds")
        for info in infos:
            member = info.filename
            parts = member.replace("\\", "/").split("/")
            mode = info.external_attr >> 16
            if (not member or member.startswith("/") or
                    any(x in (".", "..") for x in parts) or
                    "\x00" in member or ":" in member or
                    stat.S_ISLNK(mode) or
                    (mode and stat.S_IFMT(mode) not in (0, stat.S_IFREG, stat.S_IFDIR))):
                fail("Unsafe archive entry")
            if len(member) > 600:
                fail("Archive path too long")
            if info.file_size > MAX_SIZE:
                fail("Single file too large")
            if info.file_size > MAX_RATIO * max(1, info.compress_size):
                fail("Suspicious decompression ratio")
            total += info.file_size
            if total > MAX_SIZE:
                fail("Archive too large")
        base.mkdir(parents=True, exist_ok=True)
        staging = Path(tempfile.mkdtemp(prefix=".unverified-", dir=base))
        try:
            for entry in infos:
                destination = staging.joinpath(*entry.filename.replace("\\", "/").split("/"))
                if entry.is_dir():
                    destination.mkdir(parents=True, exist_ok=True)
                    continue
                destination.parent.mkdir(parents=True, exist_ok=True)
                with z.open(entry) as src, destination.open("xb") as dst:
                    shutil.copyfileobj(src, dst, length=1024 * 1024)
            if not any(staging.rglob("*.exe")):
                fail("Portable archive has no Windows EXE")
            os.replace(staging, target)
        finally:
            if staging.exists():
                shutil.rmtree(staging)
    print("PORTABLE_INSTALLED=" + str(target))


if __name__ == "__main__":
    try:
        run()
    except (OSError, ValueError, zipfile.BadZipFile, EOFError) as exc:
        print("Portable ZIP rejected:", exc, file=sys.stderr)
        sys.exit(2)
