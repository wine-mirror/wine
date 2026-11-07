#!/usr/bin/env python3
"""Transactional snapshots and rollback for isolated AILinux Wine prefixes.

SPDX-License-Identifier: LGPL-2.1-or-later
Uses COW/reflink copies when possible. No sudo, never touches ~/.wine.
A restore always snapshots the current prefix before switching directories.
"""
import argparse
import datetime as dt
import fcntl
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import uuid

HOME = Path.home().resolve()
PREFIX_ROOT = HOME / ".local/share/ailinux-wine/prefixes"
BACKUP_ROOT = HOME / ".local/share/ailinux-wine/backups"
SAFE_NAME = re.compile(r"^[A-Za-z0-9][A-Za-z0-9._-]{0,100}$")
CRITICAL = ("system.reg", "user.reg", "userdef.reg")
SCHEMA = 1


class PrefixError(Exception):
    pass


def check_root(directory):
    if directory.is_symlink():
        raise PrefixError("Symlinked root not allowed: " + str(directory))
    if not directory.is_dir():
        raise PrefixError("Missing directory: " + str(directory))
    if directory.resolve() != directory:
        raise PrefixError("Root resolves outside expected path")


def get_prefix(value):
    check_root(PREFIX_ROOT)
    path = Path(value).expanduser().absolute()
    if not SAFE_NAME.fullmatch(path.name) or path.parent != PREFIX_ROOT:
        raise PrefixError("Prefix must be a single directory under " + str(PREFIX_ROOT))
    if path.is_symlink() or not path.is_dir():
        raise PrefixError("Prefix missing or symlinked: " + str(path))
    if path.resolve() != path:
        raise PrefixError("Prefix resolves outside expected directory")
    if not (path / "system.reg").is_file():
        raise PrefixError("Prefix has no system.reg")
    return path


def get_backup(value):
    check_root(BACKUP_ROOT)
    path = Path(value).expanduser().absolute()
    if not SAFE_NAME.fullmatch(path.name) or path.parent != BACKUP_ROOT:
        raise PrefixError("Backup must be a direct child of " + str(BACKUP_ROOT))
    if path.is_symlink() or not path.is_dir() or path.resolve() != path:
        raise PrefixError("Missing or unsafe backup directory")
    return path


def sha_file(filename):
    h = hashlib.sha256()
    with filename.open("rb") as src:
        for block in iter(lambda: src.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def critical_hashes(prefix):
    result = {}
    for part in CRITICAL:
        item = prefix / part
        if not item.is_file() or item.is_symlink():
            raise PrefixError("Missing or unsafe registry file: " + part)
        result[part] = sha_file(item)
    # Runtime overlays must also retain their known state and backups.
    overlay_root = prefix / ".ailinux-wine/runtime-manager"
    if overlay_root.is_dir():
        for item in overlay_root.glob("*/state.json"):
            if item.is_symlink() or not item.is_file():
                raise PrefixError("Untrusted runtime state symlink")
            result[str(item.relative_to(prefix))] = sha_file(item)
    return result


def clone_dir(source, destination):
    if destination.exists() or destination.is_symlink():
        raise PrefixError("Destination exists: " + str(destination))
    result = subprocess.run(["/usr/bin/cp", "-a", "--reflink=auto", "--",
                             str(source), str(destination)], capture_output=True, text=True)
    if result.returncode:
        raise PrefixError("Reflink copy failed: " + result.stderr[:700])


def wait_for_idle(prefix, wine, timeout=20):
    engine = Path(wine).expanduser().absolute()
    allowed = HOME / ".local/opt/ailinux-wine"
    if not engine.is_file() or not engine.is_relative_to(allowed) or engine.is_symlink():
        raise PrefixError("Use an isolated AILinux Wine executable")
    server = engine.parent / "wineserver"
    if not server.is_file():
        raise PrefixError("Matching wineserver missing")
    env = dict(os.environ, WINEPREFIX=str(prefix))
    try:
        result = subprocess.run([str(server), "-w"], env=env,
                                stdout=subprocess.DEVNULL, stderr=subprocess.PIPE,
                                text=True, timeout=timeout)
    except subprocess.TimeoutExpired as exc:
        raise PrefixError("Wine still running in prefix; close programs before retrying") from exc
    if result.returncode:
        raise PrefixError("Could not verify Wine is idle: " + result.stderr[:300])


def snapshot(prefix, wine, label="snapshot"):
    wait_for_idle(prefix, wine)
    BACKUP_ROOT.mkdir(parents=True, exist_ok=True)
    check_root(BACKUP_ROOT)
    expected = critical_hashes(prefix)
    stamp = dt.datetime.now(dt.timezone.utc).strftime("%Y%m%dT%H%M%S")
    identity = f"{prefix.name}-{label}-{stamp}-{uuid.uuid4().hex[:10]}"
    staging = Path(tempfile.mkdtemp(prefix=".incomplete-", dir=BACKUP_ROOT))
    final = BACKUP_ROOT / identity
    try:
        content = staging / "snapshot"
        clone_dir(prefix, content)
        if critical_hashes(content) != expected or critical_hashes(prefix) != expected:
            raise PrefixError("Prefix changed during backup; refusing inconsistent snapshot")
        manifest = {
            "schema": SCHEMA,
            "name": prefix.name,
            "source": str(prefix),
            "created_utc": dt.datetime.now(dt.timezone.utc).isoformat(),
            "files": expected,
            "note": "Critical registry/runtime state hashes only; not a full file inventory."
        }
        (staging / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
        os.replace(staging, final)
        return final
    finally:
        if staging.exists():
            shutil.rmtree(staging)


def verify_backup(backup, expected_prefix=None):
    metadata_path = backup / "manifest.json"
    content = backup / "snapshot"
    if not metadata_path.is_file() or content.is_symlink() or not content.is_dir():
        raise PrefixError("Invalid snapshot layout")
    manifest = json.loads(metadata_path.read_text())
    if (manifest.get("schema") != SCHEMA or
        not SAFE_NAME.fullmatch(manifest.get("name", "")) or
        manifest.get("source") != str(PREFIX_ROOT / manifest["name"])):
        raise PrefixError("Invalid or forged snapshot provenance")
    if expected_prefix is not None and manifest["name"] != expected_prefix.name:
        raise PrefixError("Snapshot belongs to another Wine prefix")
    if critical_hashes(content) != manifest.get("files"):
        raise PrefixError("Snapshot critical registry/runtime hashes do not match")
    return manifest


def restore(prefix, backup, wine):
    # Never restore across prefixes or with Wine still running.
    verify_backup(backup, prefix)
    wait_for_idle(prefix, wine)
    identity = uuid.uuid4().hex
    staged = PREFIX_ROOT / (".restore-stage-" + identity)
    displaced = PREFIX_ROOT / (".restore-original-" + identity)
    fallback = snapshot(prefix, wine, "pre-restore")
    try:
        clone_dir(backup / "snapshot", staged)
        if critical_hashes(staged) != critical_hashes(backup / "snapshot"):
            raise PrefixError("New prefix content failed staged integrity check")
        os.replace(prefix, displaced)
        try:
            os.replace(staged, prefix)
        except Exception:
            os.replace(displaced, prefix)
            raise
    finally:
        if staged.exists():
            shutil.rmtree(staged)
    # Keep the displaced original if deletion cannot complete; verified
    # pre-restore snapshot always remains recoverable.
    try:
        shutil.rmtree(displaced)
    except OSError:
        pass
    return fallback


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("action", choices=("snapshot", "verify", "restore"))
    ap.add_argument("--prefix", required=True)
    ap.add_argument("--backup")
    ap.add_argument("--wine", default=str(HOME / ".local/opt/ailinux-wine/alpha-0.1.0/bin/wine"))
    args = ap.parse_args()
    prefix = get_prefix(args.prefix)
    PREFIX_ROOT.mkdir(exist_ok=True)
    lockfile = PREFIX_ROOT / ".ailinux-prefix-transaction.lock"
    with lockfile.open("a+b") as f:
        fcntl.flock(f, fcntl.LOCK_EX)
        if args.action == "snapshot":
            created = snapshot(prefix, args.wine)
            print(json.dumps({"action": "snapshot", "backup": str(created), "verified": True}))
        else:
            if not args.backup:
                raise PrefixError("--backup is required for verify/restore")
            backup = get_backup(args.backup)
            if args.action == "verify":
                metadata = verify_backup(backup, prefix)
                print(json.dumps({"action": "verify", "backup": str(backup),
                                  "verified": True, "source": metadata["source"],
                                  "critical_files": len(metadata["files"])}))
            else:
                fallback = restore(prefix, backup, args.wine)
                print(json.dumps({"action": "restore", "backup": str(backup),
                                  "pre_restore_backup": str(fallback), "verified": True}))
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (PrefixError, OSError, ValueError, json.JSONDecodeError) as error:
        print("AILinux Wine transaction error:", error, file=sys.stderr)
        sys.exit(2)
