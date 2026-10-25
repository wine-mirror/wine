"""Isolated unit tests for managed portable archives; no Wine runtime needed."""
import importlib.util
import hashlib
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
import zipfile

SCRIPT = Path(__file__).resolve().parents[1] / "safe-portable.py"
spec = importlib.util.spec_from_file_location("ailinux_portable", SCRIPT)
portable = importlib.util.module_from_spec(spec)
spec.loader.exec_module(portable)


class PortableManagerTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name) / "prefixes"
        self.root.mkdir()
        self.prefix = self.root / "portable-smoke"
        (self.prefix / "drive_c/Program Files").mkdir(parents=True)
        (self.prefix / "system.reg").write_text("test")
        self.patch_root = patch.object(portable, "ROOT", self.root)
        self.patch_root.start()
        self.addCleanup(self.patch_root.stop)

    def archive(self, name, entries):
        path = Path(self.temp.name) / name
        with zipfile.ZipFile(path, "w") as z:
            for entry, content in entries.items():
                z.writestr(entry, content)
        return path, hashlib.sha256(path.read_bytes()).hexdigest()

    def test_install_audit_update_remove(self):
        a, ash = self.archive("first.zip", {"bin/app.exe": b"MZhello", "notes.txt": b"v1"})
        b, bsh = self.archive("second.zip", {"bin/app.exe": b"MZhello", "notes.txt": b"v2"})
        portable.install(self.prefix, a, ash, "Demo", "1")
        target = portable.package_base(self.prefix) / "Demo"
        self.assertTrue(portable.audit(target, portable.read_manifest(target, "Demo")))
        portable.install(self.prefix, b, bsh, "Demo", "2", is_update=True)
        self.assertEqual(portable.read_manifest(target, "Demo")["version"], "2")
        self.assertEqual((target / "notes.txt").read_bytes(), b"v2")
        portable.remove(self.prefix, "Demo")
        self.assertFalse(target.exists())

    def test_reject_modified_file_and_untracked_file(self):
        a, sha = self.archive("original.zip", {"app.exe": b"MZhello"})
        portable.install(self.prefix, a, sha, "Demo", "1")
        target = portable.package_base(self.prefix) / "Demo"
        (target / "app.exe").write_bytes(b"tampered")
        with self.assertRaises(portable.PackageError):
            portable.remove(self.prefix, "Demo")
        self.assertTrue(target.exists())
        (target / "app.exe").write_bytes(b"MZhello")
        (target / "personal.txt").write_text("do not delete")
        with self.assertRaises(portable.PackageError):
            portable.remove(self.prefix, "Demo")
        self.assertTrue((target / "personal.txt").exists())

    def test_reject_zip_traversal_and_windows_case_collision(self):
        for case, files in (
            ("traversal.zip", {"../escape.exe": b"MZbad"}),
            ("case.zip", {"App.exe": b"MZx", "app.EXE": b"MZy"}),
            ("device.zip", {"CON.exe": b"MZbad"}),
            ("noexe.zip", {"README.txt": b"text"}),
        ):
            with self.subTest(case=case):
                a, sha = self.archive(case, files)
                with self.assertRaises(portable.PackageError):
                    portable.install(self.prefix, a, sha, "Demo", "1")
                self.assertFalse((portable.package_base(self.prefix) / "Demo").exists())

    def test_checksum_refusal(self):
        a, sha = self.archive("first.zip", {"app.exe": b"MZhello"})
        with self.assertRaises(portable.PackageError):
            portable.install(self.prefix, a, "0" * 64, "Demo", "1")
        portable.install(self.prefix, a, sha, "Demo", "1")
        with self.assertRaises(portable.PackageError):
            portable.install(self.prefix, a, sha, "Demo", "1", is_update=True)


if __name__ == "__main__":
    unittest.main()
