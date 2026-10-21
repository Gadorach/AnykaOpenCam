#!/usr/bin/env python3
"""Host-only decoder tests. No ARM instructions are executed."""
import importlib.util
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location("audit", Path(__file__).with_name("audit.py"))
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


class BranchTests(unittest.TestCase):
    def test_forward_bl(self):
        self.assertEqual(audit.branch_target(0xeb000000, 0x1000), (0x1008, True))

    def test_backward_bl(self):
        self.assertEqual(audit.branch_target(0xebfffffe, 0x1000), (0x1000, True))

    def test_conditional_branch(self):
        self.assertEqual(audit.branch_target(0x1a000008, 0xc03f50b8), (0xc03f50e0, False))

    def test_not_branch(self):
        self.assertIsNone(audit.branch_target(0xe3530002, 0xc03f50b4))

    def test_blx_excluded(self):
        self.assertIsNone(audit.branch_target(0xfa000000, 0x1000))

    def test_wrong_elf(self):
        with self.assertRaises(ValueError):
            audit.elf_sections(b"not an ELF")


if __name__ == "__main__":
    unittest.main()
