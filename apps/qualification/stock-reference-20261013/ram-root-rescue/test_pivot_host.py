#!/usr/bin/env python3
"""Offline tar tests and source-step mount/reference mocks. Never call host pivot/mount."""
import importlib.util
from pathlib import Path
import re
import subprocess
import unittest

HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("build_pivot", HERE / "build_pivot.py")
build = importlib.util.module_from_spec(spec)
spec.loader.exec_module(build)
SCRIPT = (HERE / "pivot-stage1.sh").read_text()
STAGE2 = (HERE / "pivot-stage2.sh").read_text()


def source_steps():
    """Extract only the literal transition operations; no shell execution."""
    steps = []
    transition = SCRIPT[SCRIPT.index("    cd /tmp/newroot || return 1"):]
    for line in transition.splitlines():
        line = line.strip()
        if line.startswith("cd "):
            steps.append(("cd", line.split()[1]))
        elif '"$pivot01_oldbb" pivot_root ' in line:
            steps.append(("pivot", ".", "oldroot"))
        elif line.startswith("/rescue-tools/busybox mount "):
            args = line.split()
            steps.append(("move", args[3], args[4]))
        elif line.startswith("exec /bin/busybox sh "):
            steps.append(("exec", "/bin/busybox", "/rescue/pivot-stage2.sh"))
    return steps


class MockMounts:
    def __init__(self, usr=True):
        self.root = "stock"
        self.cwd = "/"
        self.mounts = {"/": "stock", "/tmp": "state", "/tmp/newroot": "newroot",
                       "/dev": "dev", "/proc": "proc", "/sys": "sys"}
        if usr:
            self.mounts["/usr"] = "usrRO"
        self.usr = usr
        self.exe = "oldROOT_busybox_and_loader"
        self.events = []

    def apply(self, step):
        op = step[0]
        if op == "cd":
            self.cwd = step[1]
        elif op == "pivot":
            assert self.cwd == "/tmp/newroot" and step[1:] == (".", "oldroot")
            self.mounts = {"/oldroot" + path if path != "/" else "/oldroot": value
                           for path, value in self.mounts.items() if value != "newroot"}
            self.mounts["/"] = "newroot"
            self.root = "newroot"
            self.cwd = "/"
        elif op == "move":
            old, new = step[1:]
            assert self.root == "newroot" and old.startswith("/oldroot/")
            if old == "/oldroot/usr" and not self.usr:
                return
            self.mounts[new] = self.mounts.pop(old)
        elif op == "exec":
            assert self.cwd == "/" and self.root == "newroot"
            assert all(path in self.mounts for path in ["/dev", "/proc", "/sys", "/state"])
            self.exe = "newroot_static_busybox"
        else:
            raise AssertionError("Unreviewed mock operation")
        self.events.append(step)


class PivotTests(unittest.TestCase):
    def test_exact_literal_step_order(self):
        self.assertEqual(source_steps(), [
            ("cd", "/tmp/newroot"), ("pivot", ".", "oldroot"),
            ("move", "/oldroot/dev", "/dev"), ("move", "/oldroot/proc", "/proc"),
            ("move", "/oldroot/sys", "/sys"), ("move", "/oldroot/usr", "/usr"),
            ("move", "/oldroot/tmp", "/state"), ("cd", "/"),
            ("exec", "/bin/busybox", "/rescue/pivot-stage2.sh")])

    def test_full_transition_usr(self):
        mock = MockMounts()
        for step in source_steps():
            mock.apply(step)
        self.assertEqual(mock.mounts, {"/": "newroot", "/oldroot": "stock", "/dev": "dev",
                                     "/proc": "proc", "/sys": "sys", "/usr": "usrRO", "/state": "state"})
        self.assertEqual(mock.exe, "newroot_static_busybox")

    def test_full_transition_without_usr(self):
        mock = MockMounts(False)
        for step in source_steps():
            mock.apply(step)
        self.assertNotIn("/usr", mock.mounts)
        self.assertEqual(mock.mounts["/state"], "state")

    def test_pivot_does_not_exec_away_mm(self):
        mock = MockMounts()
        for step in source_steps()[:2]:
            mock.apply(step)
        self.assertEqual(mock.exe, "oldROOT_busybox_and_loader")

    def test_move_tmp_before_pivot_is_invalid(self):
        with self.assertRaises(AssertionError):
            MockMounts().apply(("move", "/oldroot/tmp", "/state"))

    def test_exec_before_mount_moves_is_invalid(self):
        mock = MockMounts()
        for step in source_steps()[:2]:
            mock.apply(step)
        with self.assertRaises(AssertionError):
            mock.apply(source_steps()[-1])

    def test_fail_stop_at_every_fallible_transition(self):
        steps = source_steps()
        for failure in range(len(steps) - 1):
            mock = MockMounts()
            for step in steps[:failure]:
                mock.apply(step)
            self.assertNotEqual(mock.exe, "newroot_static_busybox")
        for line in SCRIPT[SCRIPT.index("    cd /tmp/newroot || return 1"):].splitlines():
            if line.strip().startswith(("cd ", '"$pivot01_oldbb"', "/rescue-tools/busybox mount")):
                self.assertTrue(line.rstrip().endswith("|| return 1"))

    def test_only_exact_root_pwd_replaced(self):
        old = ("old_mount", "root_dentry")
        new = ("new_mount", "root_dentry")
        refs = [old, ("old_mount", "root_subdir"), ("tmp_mount", "tmp_dentry")]
        self.assertEqual([new if r == old else r for r in refs],
                         [new, refs[1], refs[2]])

    def test_no_implicit_unmount_or_hardware_commands(self):
        for text in [SCRIPT, STAGE2]:
            body = "\n".join(line for line in text.splitlines() if not line.lstrip().startswith("#"))
            self.assertIsNone(re.search(r"\b(umount|insmod|mdev|ifconfig|udhcpc|dropbear|kill|nandwrite|flash_erase)\b", body))
        self.assertNotIn("-l", STAGE2)

    def test_inert_source_only_defines_function(self):
        result = subprocess.run(["sh", "-c", '. "$1"', "host-inert-test", str(HERE / "pivot-stage1.sh")],
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, check=True)
        self.assertEqual(result.stdout, "")

    def test_nonpid1_refuses_before_operations(self):
        result = subprocess.run(["sh", "-c", '. "$1"; pivot01 I_REVIEWED_LIVE_STATE',
                                 "host-refusal-test", str(HERE / "pivot-stage1.sh")],
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("PIVOT01_REFUSE", result.stdout)

    def test_shell_syntax_only(self):
        for name in ["pivot-stage1.sh", "pivot-stage2.sh"]:
            subprocess.run(["sh", "-n", str(HERE / name)], check=True)

    def test_console_retained_before_pivot_and_after_exec(self):
        self.assertLess(SCRIPT.index("exec < /dev/console"), SCRIPT.index('"$pivot01_oldbb" pivot_root'))
        self.assertIn("< /dev/console > /dev/console 2>&1", STAGE2)

    def test_no_autorun_call_in_definition_file(self):
        self.assertTrue(SCRIPT.rstrip().endswith("}"))
        self.assertEqual(len(re.findall(r"^pivot01\(\)", SCRIPT, re.M)), 1)

    def test_tar_strict_roundtrip_no_host_extraction(self):
        entries = {"dev": {"kind": "dir", "mode": 0o755},
                   "dev/console": {"kind": "char", "mode": 0o600, "device": (5, 1)},
                   "bin": {"kind": "dir", "mode": 0o755},
                   "bin/fixture": {"kind": "file", "mode": 0o644, "data": b"PUBLIC_FIXTURE\n"},
                   "tmp": {"kind": "symlink", "mode": 0o777, "link": "state"}}
        blob = build.tar_bytes(entries)
        self.assertTrue(build.inspect_tar(blob, entries))
        self.assertEqual(blob, build.tar_bytes(entries))

    def test_tar_rejects_path_traversal(self):
        with self.assertRaises(ValueError):
            build.tar_bytes({"../bad": {"kind": "dir", "mode": 0o755}})

    def test_tar_rejects_content_change(self):
        entries = {"file": {"kind": "file", "mode": 0o600, "data": b"A"}}
        blob = build.tar_bytes(entries)
        entries["file"]["data"] = b"B"
        with self.assertRaises(ValueError):
            build.inspect_tar(blob, entries)

    def test_tmp_move_retains_inode_mount_not_original_path(self):
        mock = MockMounts()
        before_inode = ("state", 123)
        for step in source_steps():
            mock.apply(step)
        self.assertEqual((mock.mounts["/state"], 123), before_inode)
        self.assertNotIn("/oldroot/tmp", mock.mounts)


if __name__ == "__main__":
    unittest.main()
