#!/usr/bin/env python3
"""Actual retirement main under in-memory files and transport only."""
import io
import copy
import json
import types
import unittest
from unittest.mock import patch
import retire21a as r
import test_trial21a as prior
import test_metadata21a as metadata_fixture

POST = prior.POST
SNAPSHOT = b"RADIO RAM LOAD/CRC/RUN ACKNOWLEDGED; application/ABI/association still require verification\nqc07f> "
HOLD = json.dumps(r.OLD_HOLD) + "\n"


class F(io.StringIO):
    def __exit__(self, *args):
        return False

    def fileno(self):
        return 123


def simulate(fault=None):
    files = {"control/peripheral-gui-operation.lock": F("FOREIGN" if fault == "foreign_HOLD" else HOLD),
             str(r.__file__): F("PUBLIC_HOST_ONLY_RECIPE")}
    sessions = []
    clock = [0]
    class P:
        def __init__(self, value): self.value = str(value)
        def __truediv__(self, value): return P(self.value + "/" + str(value))
        def read_bytes(self): return files[self.value].getvalue().encode()
        def open(self, mode):
            if mode == "r+": files[self.value].seek(0); return files[self.value]
            assert mode == "x" and self.value not in files
            files[self.value] = F(); return files[self.value]
    checkpoint = {"recorder_pid": 123, "start_ticks": 42, "rx_bytes": 0}
    failed = {"tx_bytes": 1000, "original_preflight_rx_bytes": 0, "saved_snapshot_end_rx_bytes": len(SNAPSHOT)}
    class Session:
        def __init__(self, *args):
            self.peer = (123, 1000, 1000); self.raw = bytearray(SNAPSHOT); self.tx = 1000
            self.recent = "qc07f> late enumeration finished\n"; self.pending = b""; self.command = ""; self.calls = []; self.owners = 0
            if fault == "bad_snapshot": self.raw[0] ^= 1
            if fault == "initial_TX": self.tx += 1
            sessions.append(self)
        def state(self):
            return {"part": r.meta.PART, "tx_bytes": self.tx, "rx_bytes": len(self.raw), "tx_busy": False,
                    "abort_armed": fault != "lost_catch", "recent_rx": self.recent}
        def owner(self, **kwargs):
            self.owners += 1
            if fault == "owner_repin" and self.owners == 2: self.peer = (456, 1000, 1000)
            return self.state()
        def check_stream(self): return self.state()
        def raw_since(self, start, state): return bytes(self.raw[start:state["rx_bytes"]])
        def send(self, wire, reason):
            self.tx += len(wire)
            if wire.endswith(b"\r"):
                self.command = (self.pending + wire[:-1]).decode(); self.pending = b""; self.calls.append(self.command)
            else:
                self.pending += wire; self.raw.extend(wire)
        def wait(self, marker, start, timeout, **kwargs):
            c = self.command
            if c == "": answer = b"\r\nqc07f> "
            elif c == "shell": answer = b"\r\n# "
            elif c.startswith("echo K76MD_BEGIN_"):
                if fault == "partial_shell": raise TimeoutError("synthetic incomplete command")
                nonce = c.split("K76MD_BEGIN_", 1)[1][:32]
                text = metadata_fixture.fixture().replace(metadata_fixture.NONCE, nonce)
                if fault == "bad_descriptor": text = text.replace("ATTR:speed\n480", "ATTR:speed\n12")
                answer = text.replace("\n", "\r\n").encode()
            elif c == "exit":
                answer = b"\r\nRAM-only shell returned to monitor\r\nqc07f> "
                if fault == "new_topology": answer = b"usb 1-1: USB disconnect, device number 3\n" + answer
                if fault == "HCD_error": answer = b"musb-hdrc musb-hdrc.0.auto: Babble\n" + answer
                if fault == "new_kernel": answer = b"Linux version new-boot\n" + answer
                if fault == "deadline": clock[0] = 200
                if fault == "unexpected_TX": self.tx += 4
            else:
                key = next(key for key, command in POST.QUERIES.items() if command == c)
                value = prior.BASELINE[key]
                if fault == "changed_UUID" and key == "boot_id": value = "11234567-89ab-4cde-8f01-23456789abcd"
                if fault == "watchdog_active" and key == "watchdog": value = "active"
                answer = (value + "\r\nqc07f> ").encode()
            self.raw.extend(answer); self.recent = "# " if marker == b"# " else "qc07f> "
            return answer
        def log(self, *args, **kwargs): pass
    error = None
    with patch.multiple(r, BASE=P("base"), EVIDENCE=P("ev"), Path=P,
                        handoff=lambda: (POST, checkpoint, failed, 1000, prior.BASELINE, SNAPSHOT)), \
         patch.object(r.meta.private, "PrivateBoot", Session), patch.object(r.trial, "load_post", return_value=POST), \
         patch.object(r.b, "CONTROL", P("control")), patch.object(r.b, "ticks", return_value=42), \
         patch.object(r.os, "umask"), patch.object(r.os, "getuid", return_value=1000), patch.object(r.os, "getgid", return_value=1000), \
         patch.object(r.fcntl, "flock"), patch.object(r.time, "sleep"), patch.object(r.time, "monotonic", side_effect=lambda: clock[0]), \
         patch.object(r.sys, "argv", ["retire21a.py", "--one-readonly-retirement-preserve-HOLD21a"]), patch("builtins.print"):
        try: r.main()
        except (AssertionError, TimeoutError, ValueError, KeyError) as exc: error = type(exc).__name__
    return {"error": error, "HOLD": files["control/peripheral-gui-operation.lock"].getvalue(),
            "PASS": "ev/RADIO_READONLY_RETIREMENT21A01.json" in files,
            "calls": sessions[0].calls if sessions else [], "files": files}


class Retirement(unittest.TestCase):
    def test_complete_keeps_original_HOLD(self):
        result = simulate()
        self.assertIsNone(result["error"]); self.assertTrue(result["PASS"]); self.assertEqual(result["HOLD"], HOLD)
        self.assertEqual(len(result["calls"]), 18)  # refresh, shell/script/exit,14 reads
        record = json.loads(result["files"]["ev/RADIO_READONLY_RETIREMENT21A01.json"].getvalue())
        self.assertFalse(record["all_URB_DMA_electrical_watchdog_or_original_RUN_ACK_proved"])

    def test_faults_retain_HOLD_and_no_radio_or_reset(self):
        for fault in ("initial_TX", "bad_snapshot", "lost_catch", "partial_shell", "bad_descriptor", "new_topology",
                      "HCD_error", "new_kernel", "deadline", "unexpected_TX", "owner_repin", "changed_UUID", "watchdog_active"):
            with self.subTest(fault=fault):
                result = simulate(fault)
                self.assertIsNotNone(result["error"]); self.assertFalse(result["PASS"]); self.assertEqual(result["HOLD"], HOLD)
                self.assertFalse(any(c.startswith(("radio ", "wdt ")) for c in result["calls"]))
                self.assertLessEqual(result["calls"].count(""), 1)
                if fault == "partial_shell": self.assertNotIn("exit", result["calls"])
                if fault == "bad_descriptor": self.assertNotIn("cat /proc/1/comm", result["calls"])

    def test_foreign_HOLD_is_untouched(self):
        result = simulate("foreign_HOLD")
        self.assertIsNotNone(result["error"]); self.assertFalse(result["PASS"])
        self.assertEqual(result["HOLD"], "FOREIGN"); self.assertEqual(result["calls"], [])

    def test_wire_derivation(self):
        lengths = r.wire_lengths(POST)
        self.assertEqual(sum(lengths), 877)
        self.assertEqual(len(lengths), 27)

    def test_failed_counter_binding_and_complete_intents(self):
        for rp in (False, True):
            for rt in (False, True):
                checkpoint = {"tx_bytes": 100}
                idle = {"one_empty_monitor_refresh": rp}
                post_tx = 420 + int(rp)
                failed = {"optional_initial_monitor_refresh": rt, "checkpoint_tx_bytes": post_tx,
                          "tx_bytes": 1297 + int(rp) + int(rt), "completed_sends_count": 27 + int(rt),
                          "approved_recipe_send_lengths": r.wire_lengths(POST), "original_preflight_rx_bytes": 50}
                pre = {"event": "parent_before_trial_admission", "tx_bytes": post_tx, "target_TX": False, "rx_bytes": 50, "utc": "2026-10-21T05:23:32"}
                lengths = ([1] if rt else []) + r.wire_lengths(POST)
                events = [{"utc": pre["utc"], "kind": kind, "bytes": length} for length in lengths for kind in ("tx_protected_intent", "tx_complete")]
                args = [POST, idle, checkpoint, failed, post_tx, pre, events]
                self.assertEqual(r.reconcile_counter(*args), failed["tx_bytes"])
                for field, value in (("tx_bytes", failed["tx_bytes"] + 1), ("checkpoint_tx_bytes", post_tx + 1),
                                     ("optional_initial_monitor_refresh", int(rt)), ("completed_sends_count", 0),
                                     ("approved_recipe_send_lengths", []), ("original_preflight_rx_bytes", 0)):
                    modified = [POST, *copy.deepcopy(args[1:])]; modified[3][field] = value
                    with self.assertRaises(AssertionError): r.reconcile_counter(*modified)
                for changed in (events[:-1], events + [dict(events[-1])],
                                events + [{"utc": pre["utc"], "kind": "tx_abort", "bytes": 1}]):
                    with self.assertRaises(AssertionError): r.reconcile_counter(*args[:-1], changed)


if __name__ == "__main__": unittest.main()
