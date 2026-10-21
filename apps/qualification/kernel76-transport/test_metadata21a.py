#!/usr/bin/env python3
"""Pure metadata/script parsing checks; no imports invoke target operations."""
import unittest
import hashlib
import json
import types
from unittest.mock import patch
import metadata21a as m

NONCE = "1" * 32


def fixture():
    lines = [f"K76MD_BEGIN_{NONCE}", "FDS"]
    lines += [f"lrwx------    1        64 {i} -> /dev/console" for i in range(4)]
    lines += ["FD_RC:0"]
    for key, value in zip(m.ATTRS, ("a012", "8400", "1", "480")): lines += ["ATTR:" + key, value]
    for ep, addr in (("ep_01", "01"), ("ep_81", "81")):
        for key, value in zip(m.EPATTRS, (addr, "02", "0200")): lines += [f"EP:{ep}:{key}", value]
    return "\n".join(lines + [f"K76MD_DONE_{NONCE}:0", "# "])


class Metadata(unittest.TestCase):
    def test_valid(self): self.assertTrue(m.parse(fixture(), NONCE)["no_bound_USB_interface_driver"])

    def test_script_bounds(self):
        cmd = m.script(NONCE)
        self.assertLessEqual(len(cmd.encode()), 950)
        for bad in ("x" * 32, "1" * 31, "1" * 32 + "\r"):
            with self.assertRaises(AssertionError): m.script(bad)

    def test_missing_conflicting_or_foreign_metadata(self):
        text = fixture()
        for bad in (text.replace("ATTR:idVendor\na012", "ATTR:idVendor\nffff"),
                    text.replace("EP:ep_01:wMaxPacketSize\n0200", "EP:ep_01:wMaxPacketSize\n0040"),
                    text.replace("EP:ep_81:bEndpointAddress\n81", "EP:ep_81:bEndpointAddress\n82"),
                    text.replace("FD_RC:0", "FD_RC:1"), text.replace("3 -> /dev/console", "3 -> /dev/watchdog0"),
                    text.replace(f"K76MD_DONE_{NONCE}:0", f"K76MD_DONE_{NONCE}:1"),
                    "WARNING\n" + text, text.replace("# ", "WARNING\n# "),
                    text.replace("# ", "# # "), text.replace("ATTR:speed\n480", "ATTR:speed\n480\nEXTRA")):
            with self.assertRaises(AssertionError): m.parse(bad, NONCE)

    def test_state_rejects_other_sender(self):
        class Fake:
            def check_stream(self): return {"part": m.PART, "tx_busy": False, "tx_bytes": 101}
        with self.assertRaises(AssertionError): m.state(Fake(), 100)

    def test_exact_handoff_refuses_omitted_mismatched_or_untyped_facts(self):
        queries = dict(zip(range(13), ("cat /proc/sys/kernel/random/boot_id", "cat /proc/1/comm", "cat /proc/1/status",
            "cat /proc/sys/kernel/osrelease", "cmdline", "cat /proc/modules", "cat /proc/mounts", "mem", "iomem", "isp state",
            "cat /sys/class/watchdog/watchdog0/state", "cat /sys/class/watchdog/watchdog0/nowayout", "cat /sys/class/tty/ttyAK0/rx_state")))
        queries["boot_id"] = queries.pop(0)
        module = types.SimpleNamespace(QUERIES=queries)
        source = b"PUBLIC_HOST_SOURCE_FIXTURE"; sha = hashlib.sha256(source).hexdigest()
        def base():
            return {"boot": {"state": "RAM76_BOOT_AND_INERT_MONITOR_OBSERVED", "part": m.PART, "image_sha256": m.SHA, "full_RAM_image_CRC_verified": True},
                    "post": {"state": "RAM76_NATIVE_BOOT_READONLY_IDLE_POSTCHECK_PASS", "part": m.PART, "image_sha256": m.SHA,
                             "one_empty_monitor_refresh": True, "recipe_sha256": sha},
                    "checkpoint": {"image_sha256": m.SHA, "part": m.PART, "monitor_observed": True, "shell_started": False,
                                   "tx_bytes": 100, "rx_bytes": 50, "recorder_pid": 123, "start_ticks": 42},
                    "derivation": {"inputs": [{"file": "postcheck.py", "exact_reversible_epoch_path_only_substitutions": True,
                        "original_sha256": "035c106995b566b815a670748c1c84c99e49695a5e154c8d67314bf4304fa5e9", "new_sha256": sha}]}}
        mutations = [(None, None, None), ("boot", "state", "FAILED"), ("boot", "part", "foreign"),
                     ("post", "part", "foreign"), ("checkpoint", "part", "foreign"), ("checkpoint", "image_sha256", "0" * 64),
                     ("checkpoint", "monitor_observed", False), ("checkpoint", "shell_started", True),
                     ("checkpoint", "tx_bytes", -1), ("checkpoint", "rx_bytes", True),
                     ("checkpoint", "recorder_pid", 1), ("checkpoint", "start_ticks", 0),
                     ("post", "one_empty_monitor_refresh", 1), ("post", "recipe_sha256", "0" * 64)]
        for group, key, value in mutations:
            data = base()
            if group: data[group][key] = value
            class P:
                def __init__(self, text): self.value = text
                def __truediv__(self, text): return P(self.value + "/" + text)
                def read_bytes(self): return source
                def read_text(self):
                    name = self.value.rsplit("/", 1)[-1]
                    return json.dumps(data[{"RAM76_BOOT21A01.json": "boot", "RAM76_POSTCHECK21A01.json": "post",
                        "boot-checkpoint21a01.private.json": "checkpoint", "SOURCE_DERIVATION21A01.json": "derivation"}[name]])
            loader = types.SimpleNamespace(exec_module=lambda module: None)
            with patch.object(m, "EVIDENCE", P("ev")), patch.object(m, "BASE", P("base")), \
                 patch.object(m.private, "private_path", side_effect=lambda p: p), \
                 patch.object(m.importlib.util, "spec_from_file_location", return_value=types.SimpleNamespace(loader=loader)) as imported, \
                 patch.object(m.importlib.util, "module_from_spec", return_value=module):
                if group:
                    with self.assertRaises(AssertionError): m.admitted_handoff()
                    imported.assert_not_called()
                else:
                    self.assertEqual(m.admitted_handoff()[-1], 421)
                    imported.assert_called_once()


if __name__ == "__main__": unittest.main()
