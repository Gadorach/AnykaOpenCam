#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
import stat
import unittest
import cpio

if not __debug__:
    raise SystemExit("Archive safety checks require assertions; do not use Python -O")


def member(name, payload=b"", mode=stat.S_IFREG | 0o755, links=1):
    return cpio.record(name, [1, mode, 0, 0, links, 0, 0, 0, 0, 0, 0, 0, 0], payload)


class NewcTests(unittest.TestCase):
    def setUp(self):
        self.base = member(".", mode=stat.S_IFDIR | 0o755) + member("hgics.ko", b"old") + member("bin/rtspd", b"old-rtsp") + member("TRAILER!!!")

    def test_replace_and_add_preserves_other_records(self):
        new = cpio.update(self.base, {"hgics.ko": b"new-module", "bin/rtspd": b"new-rtsp"}, {"bin/bridge": b"ELF"})
        self.assertEqual(len(new) % 512, 0)
        rows = {x["path"]: x for x in cpio.parse(new)}
        self.assertEqual(rows["bin/bridge"]["payload"], b"ELF")
        self.assertEqual(rows["."]["raw"], cpio.parse(self.base)[0]["raw"])

    def test_no_change_preserves_records(self):
        new = cpio.update(self.base, {}, {})
        self.assertEqual([x["raw"] for x in cpio.parse(new)], [x["raw"] for x in cpio.parse(self.base)])

    def test_duplicate_rejected(self):
        with self.assertRaises(AssertionError): cpio.parse(member("x") + member("x") + member("TRAILER!!!"))

    def test_normalized_duplicate_rejected(self):
        with self.assertRaises(AssertionError): cpio.parse(member("x") + member("./x") + member("TRAILER!!!"))

    def test_traversal_rejected(self):
        with self.assertRaises(AssertionError): cpio.parse(member("../x") + member("TRAILER!!!"))

    def test_absolute_rejected(self):
        with self.assertRaises(AssertionError): cpio.parse(member("/x") + member("TRAILER!!!"))

    def test_truncated_rejected(self):
        with self.assertRaises(AssertionError): cpio.parse(self.base[:-1])

    def test_trailing_data_rejected(self):
        with self.assertRaises(AssertionError): cpio.parse(self.base + b"bad")

    def test_missing_replace_rejected(self):
        with self.assertRaises(AssertionError): cpio.update(self.base, {"absent": b"new"}, {})

    def test_existing_add_rejected(self):
        with self.assertRaises(AssertionError): cpio.update(self.base, {}, {"hgics.ko": b"new"})

    def test_hardlink_replace_rejected(self):
        blob = member("x", b"old", links=2) + member("TRAILER!!!")
        with self.assertRaises(AssertionError): cpio.update(blob, {"x": b"new"}, {})

    def test_symlink_replace_rejected(self):
        blob = member("x", b"target", mode=stat.S_IFLNK | 0o777) + member("TRAILER!!!")
        with self.assertRaises(AssertionError): cpio.update(blob, {"x": b"new"}, {})

    def test_trailer_replace_rejected(self):
        with self.assertRaises(AssertionError): cpio.update(self.base, {"TRAILER!!!": b"bad"}, {})

    def test_unsafe_add_rejected(self):
        with self.assertRaises(AssertionError): cpio.update(self.base, {}, {"../x": b"bad"})

    def test_repeated_separator_rejected(self):
        with self.assertRaises(AssertionError): cpio.update(self.base, {}, {"bin//x": b"bad"})

    def test_embedded_dot_rejected(self):
        with self.assertRaises(AssertionError): cpio.update(self.base, {}, {"bin/./x": b"bad"})

    def test_NUL_add_rejected(self):
        with self.assertRaises(AssertionError): cpio.update(self.base, {}, {"bin/x\0y": b"bad"})


if __name__ == "__main__":
    unittest.main()
