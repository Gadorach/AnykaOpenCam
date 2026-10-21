#!/usr/bin/env python3
"""Execute actual trial main with every transport/file capability replaced."""
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import struct
import types
import unittest
from unittest.mock import patch
import trial21a as t
import test_radio_receipt as samples

_spec = importlib.util.spec_from_file_location("public_post_source_only", t.EVIDENCE / "postcheck.py")
POST = importlib.util.module_from_spec(_spec); _spec.loader.exec_module(POST)
UUID = "01234567-89ab-4cde-8f01-23456789abcd"
BASELINE = {"pid1": "init", "release": "6.18.55-qc07f-prototype-dirty",
    "identity": "Pid:\t1\nUid:\t0\t0\t0\t0\nCapEff:\t0000000000020000\n",
    "cmdline": POST.BOOTARGS, "modules": "",
    "mounts": "rootfs / rootfs rw 0 0\ndevtmpfs /dev devtmpfs rw 0 0\nproc /proc proc rw 0 0\nsysfs /sys sysfs rw 0 0\n",
    "memory": "MemTotal: 35000 kB\nMemFree: 12000 kB\nCmaTotal: 16384 kB\nCmaFree: 10000 kB",
    "iomem": "81800000-83ffffff : System RAM", "ISP": "result=-61 readable=0 guard_errors=0 irq_mode=0 irq_frames=0 queue_mode=0 queue_started=0 queue_held=0 wb_owner_active=0",
    "watchdog": "inactive", "nowayout": "0", "boot_id": UUID,
    "UART": "rx_error_events=0 mem_busy_ticks=0 last_status=00080000 rxpos=12; cached diagnostic, lost bytes cannot be reconstructed"}


class MemoryFile(io.StringIO):
    def __exit__(self, *args): return False
    def fileno(self): return 123


def simulate(fault=None):
    files = {}; instances = []; logs = []
    class MemoryPath:
        def __init__(self, value): self.value = str(value)
        def __truediv__(self, value): return MemoryPath(self.value + "/" + str(value))
        def read_bytes(self): return files[self.value].getvalue().encode()
        def read_text(self): return files[self.value].getvalue()
        def open(self, mode):
            if mode == "r+": files[self.value].seek(0); return files[self.value]
            assert mode == "x" and self.value not in files
            value = MemoryFile(); value.name = self.value; files[self.value] = value; return value
    image = b"PUBLIC_HOST_ONLY_IMAGE_FIXTURE"
    sha = hashlib.sha256(image).hexdigest()
    def put(name, value): files[name] = MemoryFile(json.dumps(value))
    put("ev/RAM76_BOOT21A01.json", {"image_sha256": sha})
    put("ev/RAM76_POSTCHECK21A01.json", {"image_sha256": sha, "state": "RAM76_NATIVE_BOOT_READONLY_IDLE_POSTCHECK_PASS", "one_empty_monitor_refresh": False})
    put("base/boot-checkpoint21a01.private.json", {"tx_bytes": 100, "rx_bytes": 50, "recorder_pid": 123, "start_ticks": 42})
    put("base/runtime21a01.private.json", BASELINE)
    files["root/work/lightbulb-camera/foundation-20261011T041148Z/artifacts/native-usb76-source01/images/QC6U76.UIM"] = MemoryFile(image.decode())
    put("root/docs/qualification-plans/kernel76-transport-20261021/source-review/entry-bind.json", {
        "image_sha256": sha, "embedded_cpio_equals_final_selected_cpio": True,
        "embedded_init_equals_attempt50_cpio_and_retained_init": True,
        "private_firmware_admission_booleans_only": {"synthetic": True}, "source_pins": {}})
    files["control/peripheral-gui-operation.lock"] = MemoryFile("FOREIGN" if fault == "foreign_HOLD" else "")
    files[str(t.__file__)] = MemoryFile("public synthetic recipe identity")
    if fault == "missing_receipt": del files["ev/RAM76_BOOT21A01.json"]
    initial_tx = 100 + sum(len(c.encode()) + 1 for c in list(POST.QUERIES.values()) + [POST.QUERIES["boot_id"]])
    def fake_handoff():
        boot = json.loads(files["ev/RAM76_BOOT21A01.json"].getvalue())
        idle = json.loads(files["ev/RAM76_POSTCHECK21A01.json"].getvalue())
        checkpoint = json.loads(files["base/boot-checkpoint21a01.private.json"].getvalue())
        return POST, boot, idle, checkpoint, initial_tx
    class Fake:
        def __init__(self, *args):
            self.peer = (123, 1000, 1000); self.tx = initial_tx + int(fault == "entry_TX")
            self.raw = bytearray(b"x" * 50); self.recent = "qc07f> "; self.command = ""; self.pending = b""
            self.calls = []; self.load_count = 0; self.owner_calls = 0; instances.append(self)
        def snapshot(self): return {"part": t.meta.PART, "tx_busy": False, "tx_bytes": self.tx, "rx_bytes": len(self.raw), "recent_rx": self.recent, "abort_armed": True}
        def owner(self, **kwargs):
            self.owner_calls += 1
            if self.owner_calls == 2 and fault == "changed_owner": self.peer = (456, 1000, 1000)
            value = self.snapshot()
            if self.owner_calls == 2 and fault == "lost_catch": value["abort_armed"] = False
            return value
        def check_stream(self): return self.snapshot()
        def raw_since(self, offset, state): return bytes(self.raw[offset:state["rx_bytes"]])
        def send(self, wire, reason):
            self.tx += len(wire)
            if wire.endswith(b"\r"):
                self.command = (self.pending + wire[:-1]).decode(); self.pending = b""; self.calls.append(self.command)
            else: self.pending += wire; self.raw.extend(wire)
        def wait(self, marker, start, timeout, **kwargs):
            command = self.command
            if command == "shell": answer = b"\r\n# "
            elif command == "exit":
                answer = b"\r\nRAM-only shell returned to monitor\r\nqc07f> "
                if fault == "source_warning_before": answer = b"\r\n[ 1.000000] musb_rx_reinit 565: rx1, packet/512 ready?\r\n" + answer
                if fault == "recognized_warning_before": answer = b"\r\n[ 1.000000] musb: warning synthetic\r\n" + answer
                if fault == "broken_before": answer = b"\r\n[ 1.000000] musb_ep_program 878: broken !rx_reinit, ep1 csr 1234\r\n" + answer
            elif command.startswith("echo K76MD_BEGIN_"):
                nonce = command.split("K76MD_BEGIN_", 1)[1][:32]
                lines = [f"K76MD_BEGIN_{nonce}", "FDS"] + [f"lrwx------    1        64 {i} -> /dev/console" for i in range(4)] + ["FD_RC:0"]
                for key, value in zip(t.meta.ATTRS, ("a012", "8400", "1", "480")): lines += ["ATTR:" + key, value]
                for ep, addr in (("ep_01", "01"), ("ep_81", "81")):
                    for key, value in zip(t.meta.EPATTRS, (addr, "02", "0200")): lines += [f"EP:{ep}:{key}", value]
                if fault == "bad_descriptor": lines[lines.index("0200")] = "0040"
                lines += [f"K76MD_DONE_{nonce}:0", "# "]; answer = "\r\n".join(lines).encode()
            elif command == "radio native-load":
                self.load_count += 1
                if fault == "load_timeout": raise TimeoutError("synthetic")
                value = samples.loader()
                if fault == "bad_loader": value = value.replace("RESULT0x00000804", "RESULT0xffffff92", 1)
                if fault == "warning_after_load": value += "\n[ 2.000000] WARNING: CPU: 0 PID: 1 at musb"
                answer = (value + "\r\nqc07f> ").encode()
                if fault == "intervening_TX": self.tx += 5
            elif command == "radio state":
                value = "1a2b07001000020000ff0000000000fd"
                if fault == "bad_state": value = value.replace("00ff00", "000000")
                answer = ("RADIO BOOT STATE QUERY TX RESULT 0x00000014\nRADIO RX RESULT 0x00000010\nRADIO RX BYTES " + value + "\nRADIO BOOT QUERY CHECK/CMD/COOKIE MATCH, RESULT 0x000000ff\nRADIO STATE APPLICATION\nqc07f> ").encode()
                if fault == "warning_after_state": answer = answer.replace(b"qc07f> ", b"[ 3.000000] musb: timeout synthetic\nqc07f> ")
            elif command == "radio info":
                value = bytearray.fromhex("1a2b0301240001002b001800") + bytearray(24)
                struct.pack_into("<IIHH", value, 12, 0x02010001, 22118, 0x8400, 1)
                if fault == "bad_identity": value[12] ^= 1
                answer = ("RADIO READONLY FWINFO TX RESULT 0x0000000c\nRADIO RX RESULT 0x00000024\nRADIO RX BYTES " + value.hex() + "\nRADIO FWINFO COOKIE/CMD MATCH, RETURN/PAYLOAD LENGTH 0x00000018\nqc07f> ").encode()
            else:
                key = next(k for k, c in POST.QUERIES.items() if c == command)
                value = BASELINE[key]
                if fault == "active_after" and self.load_count and key == "watchdog": value = "active"
                answer = (value + "\r\nqc07f> ").encode()
            self.raw.extend(answer); self.recent = "# " if marker == b"# " else "qc07f> "
            return answer
        def log(self, event, **kwargs): logs.append(event)
    error = None
    with patch.multiple(t, ROOT=MemoryPath("root"), BASE=MemoryPath("base"), EVIDENCE=MemoryPath("ev"), Path=MemoryPath, SHA=sha, load_post=lambda: POST), \
         patch.object(t.meta, "admitted_handoff", side_effect=fake_handoff), \
         patch.object(t.private, "private_path", side_effect=lambda p: p), patch.object(t.private, "PrivateBoot", Fake), \
         patch.object(t.b, "CONTROL", MemoryPath("control")), patch.object(t.b, "ticks", return_value=42), \
         patch.object(t.sys, "argv", ["trial21a.py", "--one-whole-native-load-and-RUN21a"]), \
         patch.object(t.fcntl, "flock"), patch.object(t.os, "umask"), patch.object(t.os, "fsync"), patch.object(t.os, "chmod"), \
         patch.object(t.time, "sleep"), patch("builtins.print"):
        try: t.main()
        except (AssertionError, TimeoutError, KeyError, OSError) as exc: error = type(exc).__name__
    return {"error": error, "HOLD": files["control/peripheral-gui-operation.lock"].getvalue(),
            "PASS": "ev/RADIO_NATIVE76_21A01.json" in files,
            "load_count": instances[0].load_count if instances else 0,
            "calls": instances[0].calls if instances else []}


class Trial(unittest.TestCase):
    def test_complete(self):
        answer = simulate()
        self.assertIsNone(answer["error"]); self.assertTrue(answer["PASS"]); self.assertFalse(answer["HOLD"])
        self.assertEqual(answer["load_count"], 1)

    def test_failures_keep_HOLD_never_repeat(self):
        for fault in ("entry_TX", "missing_receipt", "bad_descriptor", "load_timeout", "bad_loader", "intervening_TX", "bad_state", "bad_identity", "active_after"):
            with self.subTest(fault=fault):
                answer = simulate(fault)
                self.assertIsNotNone(answer["error"]); self.assertFalse(answer["PASS"]); self.assertTrue(answer["HOLD"])
                self.assertLessEqual(answer["load_count"], 1)
                self.assertNotIn("radio load", answer["calls"]); self.assertNotIn("wdt reset", answer["calls"])

    def test_foreign_HOLD_unchanged(self):
        answer = simulate("foreign_HOLD")
        self.assertEqual(answer["HOLD"], "FOREIGN"); self.assertFalse(answer["PASS"]); self.assertEqual(answer["load_count"], 0)

    def test_owner_repin_and_preload_warnings_stop_before_first_effect(self):
        for fault in ("changed_owner", "lost_catch", "source_warning_before", "recognized_warning_before", "broken_before"):
            with self.subTest(fault=fault):
                answer = simulate(fault)
                self.assertIsNotNone(answer["error"]); self.assertTrue(answer["HOLD"]); self.assertFalse(answer["PASS"])
                self.assertEqual(answer["load_count"], 0)
                self.assertFalse(any(c.startswith("radio ") for c in answer["calls"]))

    def test_interquery_warnings_prevent_next_active_query(self):
        for fault, disallowed in (("warning_after_load", "radio state"), ("warning_after_state", "radio info")):
            with self.subTest(fault=fault):
                answer = simulate(fault)
                self.assertIsNotNone(answer["error"]); self.assertTrue(answer["HOLD"]); self.assertFalse(answer["PASS"])
                self.assertEqual(answer["load_count"], 1); self.assertNotIn(disallowed, answer["calls"])

    def test_source_based_diagnostic_forms(self):
        for text in ("Could not flush host TX4 fifo", "musb_rx_reinit 565: rx1, packet/512 ready?",
                     "broken !rx_reinit, ep1 csr 12", "bogus ep0 stage 3", "no URB for end 0",
                     "RX1 dma busy, csr 4", "Rx interrupt with no errors or packet!",
                     "trying to suspend as a_host while active", "WARNING: CPU: 0", "Oops:", "Kernel panic"):
            self.assertTrue(t.HCD_DIAGNOSTIC.search(text.encode()), text)


if __name__ == "__main__": unittest.main()
