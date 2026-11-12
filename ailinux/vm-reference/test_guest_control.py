"""Guarded host-to-Windows QEMU agent bridge unit tests."""
import importlib.util
from pathlib import Path
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location("guestctrl", Path(__file__).with_name("guest-control.py"))
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

class GuestControlTests(unittest.TestCase):
    def test_only_allowlisted_actions(self):
        self.assertEqual(set(module.ACTIONS), {"build", "users", "guest-agent"})
        for action in module.ACTIONS.values():
            self.assertNotIn("Invoke-Expression", action)
            self.assertNotIn("DownloadString", action)
            self.assertNotIn("password", action.lower())

    def test_ping_contract(self):
        with patch.object(module, "qga", return_value={} ) as qga:
            qga({"execute": "guest-ping"})
            qga.assert_called_once_with({"execute": "guest-ping"})

    def test_guest_exit(self):
        with patch.object(module, "qga", side_effect=[
            {"pid": 123}, {"exited": True, "exitcode": 0, "out-data": "T0sK"}
        ]):
            self.assertEqual(module.run_command("guest-agent"), "OK\n")

if __name__ == "__main__":
    unittest.main()
