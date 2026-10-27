"""Pure Python regression tests; no VM or Windows credentials needed."""
import unittest

import importlib.util
from pathlib import Path

spec = importlib.util.spec_from_file_location("reference_compare", Path(__file__).with_name("compare-probes.py"))
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
compare, load = module.compare, module.load


class ComparisonTests(unittest.TestCase):
    def test_export_parity_and_missing(self):
        win = {"schema": 1, "arch_bits": 64, "windows_version": "10.0.26300",
               "api": {"ntdll.dll!RtlGetVersion": True, "test.dll!Missing": True}}
        wine = {"schema": 1, "arch_bits": 64, "windows_version": "10.0.22000",
                "api": {"ntdll.dll!RtlGetVersion": True, "test.dll!Missing": False}}
        result = compare(win, wine)
        self.assertEqual(result["export_matches"], 1)
        self.assertEqual(result["wine_missing_windows_exports"], ["test.dll!Missing"])

    def test_reject_cross_architecture(self):
        a = {"schema": 1, "arch_bits": 32, "api": {"x": True}}
        b = {"schema": 1, "arch_bits": 64, "api": {"x": True}}
        with self.assertRaisesRegex(ValueError, "Architecture mismatch"):
            compare(a, b)


if __name__ == "__main__":
    unittest.main()
