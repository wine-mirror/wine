"""Offline regression tests for reversible AILinux Wine runtime overlays.

SPDX-License-Identifier: LGPL-2.1-or-later
"""
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import struct
import tarfile
import tempfile
import unittest
from unittest.mock import patch

MODULE = Path(__file__).resolve().parents[1] / "runtime-manager.py"
spec = importlib.util.spec_from_file_location("ailinux_runtime_manager", MODULE)
manager = importlib.util.module_from_spec(spec)
spec.loader.exec_module(manager)


def mock_pe(arch):
    machine = 0x014C if arch == "x86" else 0x8664
    buf = bytearray(256)
    buf[:2] = b"MZ"
    struct.pack_into("<I", buf, 0x3C, 128)
    buf[128:132] = b"PE\0\0"
    struct.pack_into("<H", buf, 132, machine)
    return bytes(buf)


class RuntimeManagerTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix=".ailinux-wine-unit-", dir=manager.HOME)
        self.addCleanup(self.tmp.cleanup)
        self.base = Path(self.tmp.name)
        self.prefix = self.base / "prefix"
        (self.prefix / "drive_c/windows/system32").mkdir(parents=True)
        (self.prefix / "drive_c/windows/syswow64").mkdir(parents=True)
        (self.prefix / "system.reg").write_text("#arch=win64\n", encoding="utf-8")
        self.old64 = self.prefix / "drive_c/windows/system32/d3d11.dll"
        self.old32 = self.prefix / "drive_c/windows/syswow64/d3d11.dll"
        self.old64.write_bytes(b"original 64-bit Wine DLL")
        (self.prefix / "original-32").write_bytes(b"Wine 32-bit builtin")
        self.old32.symlink_to(self.prefix / "original-32")
        self.manifest = {
            "schema": 1,
            "components": {
                "dxvk": {
                    "version": "test-1",
                    "file": "mock.tar.gz",
                    "url": "https://invalid.example/mock.tar.gz",
                    "archive_root": "mock",
                    "source_dirs": {"x64": "x64", "x86": "x32"},
                    "dlls": ["d3d11"],
                    "sha256": ""
                }
            }
        }
        self.cache = self.base / "cache"
        self.cache.mkdir()
        archive = self.cache / "mock.tar.gz"
        with tarfile.open(archive, "w:gz") as tar:
            for arch, subdir in [("x64", "x64"), ("x86", "x32")]:
                data = mock_pe(arch)
                info = tarfile.TarInfo("mock/" + subdir + "/d3d11.dll")
                info.size = len(data)
                tar.addfile(info, io.BytesIO(data))
        self.manifest["components"]["dxvk"]["sha256"] = manager.digest_file(archive)
        self.lock = self.base / "lock.json"
        self.lock.write_text(json.dumps(self.manifest), encoding="utf-8")
        patcher = patch.object(manager, "LOCKFILE", self.lock)
        patcher.start()
        self.addCleanup(patcher.stop)

    def test_install_status_remove_restores_original_and_symlink(self):
        prefix = manager.prefix_path(str(self.prefix))
        self.assertEqual(manager.supported_architectures(prefix)["x86"], "drive_c/windows/syswow64")
        manager.install(prefix, "dxvk", self.cache)
        state = manager.load_state(prefix, "dxvk")
        self.assertIsNotNone(state)
        manager.verify_state(prefix, "dxvk", state)
        self.assertEqual(self.old64.read_bytes(), mock_pe("x64"))
        self.assertEqual(self.old32.read_bytes(), mock_pe("x86"))
        manager.remove(prefix, "dxvk")
        self.assertEqual(self.old64.read_bytes(), b"original 64-bit Wine DLL")
        self.assertTrue(self.old32.is_symlink())
        self.assertEqual(self.old32.readlink(), self.prefix / "original-32")
        self.assertIsNone(manager.load_state(prefix, "dxvk"))

    def test_tamper_guard_prevents_destructive_uninstall(self):
        prefix = manager.prefix_path(str(self.prefix))
        manager.install(prefix, "dxvk", self.cache)
        self.old64.write_bytes(b"external modification")
        with self.assertRaisesRegex(manager.RuntimeErrorSafe, "has changed"):
            manager.remove(prefix, "dxvk")
        self.assertEqual(self.old64.read_bytes(), b"external modification")
        self.assertIsNotNone(manager.load_state(prefix, "dxvk"))

    def test_reinstall_refused_without_losing_backup(self):
        prefix = manager.prefix_path(str(self.prefix))
        manager.install(prefix, "dxvk", self.cache)
        with self.assertRaisesRegex(manager.RuntimeErrorSafe, "already installed"):
            manager.install(prefix, "dxvk", self.cache)
        manager.remove(prefix, "dxvk")
        self.assertEqual(self.old64.read_bytes(), b"original 64-bit Wine DLL")

    def test_wrong_archive_digest_refused(self):
        archive = self.cache / "mock.tar.gz"
        archive.write_bytes(b"corrupted")
        with self.assertRaisesRegex(manager.RuntimeErrorSafe, "wrong SHA256"):
            manager.get_archive("dxvk", self.cache)
        self.assertEqual(self.old64.read_bytes(), b"original 64-bit Wine DLL")

    def test_default_wine_prefix_rejected(self):
        with patch.object(manager, "HOME", self.base):
            (self.base / ".wine").mkdir()
            with self.assertRaises(manager.RuntimeErrorSafe):
                manager.prefix_path(str(self.base / ".wine"))

    def test_missing_prefix_registry_rejected(self):
        (self.prefix / "system.reg").unlink()
        with self.assertRaisesRegex(manager.RuntimeErrorSafe, "system.reg absent"):
            manager.prefix_path(str(self.prefix))


if __name__ == "__main__":
    unittest.main()
