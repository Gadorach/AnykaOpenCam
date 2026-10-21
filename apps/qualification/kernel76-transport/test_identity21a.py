#!/usr/bin/env python3
"""HOST ONLY: actual main + inherited Boot.send/wait, entirely in-memory I/O.

Import prior definitions/public postcheck source only; never run a collector or
any native main. Neither tests nor fixtures open private paths or a socket.
"""
import contextlib
import copy
import hashlib
import io
import itertools
import json
import stat
import struct
import types
import unittest
from unittest.mock import patch
import identity21a as i
import test_trial21a as prior
import test_metadata21a as md

POST = prior.POST
HOLD = (json.dumps(i.retire.OLD_HOLD) + "\n").encode()
IDS = (1000, 1000, 1000, 1000)
SNAPSHOT = (i.trial.receipt.DONE + "\nqc07f> ").encode()
METADATA = {"PID1_console_fds_only": True, "PID1_USB_watchdog_fds_absent": True,
            "owned_a012_8400_config1_speed480": True, "bulk_OUT01_IN81_maxpacket512": True,
            "no_bound_USB_interface_driver": True}
WARNING = b"Process 1 (init) called USBDEVFS_CLEAR_HALT for active endpoint 0x81\n"


def history(rp=True, rt=True, rr=True):
    boot = {"tx_bytes": 100, "rx_bytes": 0, "recorder_pid": 123, "start_ticks": 42}
    idle = {"one_empty_monitor_refresh": rp}
    cf = 1297 + int(rp) + int(rt)
    failed = {"state": "OFFLINE_FAILED_TRIAL_TX_DERIVATION_VERIFIED", "tx_bytes": cf,
              "checkpoint_tx_bytes": 420 + int(rp), "optional_initial_monitor_refresh": rt,
              "original_preflight_rx_bytes": 0, "saved_snapshot_end_rx_bytes": len(SNAPSHOT),
              "approved_recipe_send_lengths": i.retire.wire_lengths(POST),
              "completed_sends_count": 27 + int(rt), "part": i.meta.PART,
              "image_sha256": i.meta.SHA, "recorder_pid": 123, "start_ticks": 42}
    cp = {"tx_bytes": cf + 859 + int(rr), "rx_bytes": len(SNAPSHOT), "part": i.meta.PART,
          "image_sha256": i.meta.SHA, "peer": [123, 1000, 1000], "start_ticks": 42,
          "original_HOLD": HOLD.decode(), "original_trial_still_failed": True,
          "metadata": METADATA.copy(), "rows": dict(prior.BASELINE, boot_id_after=prior.UUID)}
    pre = {"event": "parent_before_trial_admission", "target_TX": False,
           "tx_bytes": 420 + int(rp), "rx_bytes": 0}
    receipt = {"state": "READONLY_POSTFAILURE_RETIREMENT_OBSERVED_ORIGINAL_TRIAL_FAILED_HOLD",
               "one_empty_monitor_refresh": rr, "recipe_sha256": i.RETIRE_SHA,
               "original_trial_failed_HOLD_preserved_byte_exact": True,
               "no_radio_USB_query_reset_module_network_media_or_persistent_write": True,
               "owned_shell_returned_with_cached_metadata": METADATA.copy()}
    completion = {"event": "readonly_retirement_complete_original_HOLD_preserved",
                  "tx_bytes": cp["tx_bytes"], "rx_bytes": cp["rx_bytes"]}
    sizes = [100] + ([1] if rp else []) + list(map(len, i.inventory_wires(POST)))
    sizes += ([1] if rt else []) + i.retire.wire_lengths(POST)
    sizes += ([1] if rr else []) + list(map(len, i.cached_wires("1" * 32) + i.inventory_wires(POST)))
    events = [{"kind": kind, "bytes": n} for n in sizes for kind in ("tx_protected_intent", "tx_complete")]
    return [POST, boot, idle, failed, pre, cp, receipt, completion, events]


def state_reply():
    return ("RADIO BOOT STATE QUERY TX RESULT 0x00000014\nRADIO RX RESULT 0x00000010\n"
            "RADIO RX BYTES 1a2b07001000020000ff0000000000fd\n"
            "RADIO BOOT QUERY CHECK/CMD/COOKIE MATCH, RESULT 0x000000ff\nRADIO STATE APPLICATION\nqc07f> ").encode()


def info_reply():
    value = bytearray.fromhex("1a2b0301240001002b001800") + bytearray(24)
    struct.pack_into("<IIHH", value, 12, 0x02010001, 22118, 0x8400, 1)
    value[24:] = b"PRIVATE_TAIL"  # public receipt/stdout must never contain this
    return ("RADIO READONLY FWINFO TX RESULT 0x0000000c\nRADIO RX RESULT 0x00000024\nRADIO RX BYTES "
            + value.hex() + "\nRADIO FWINFO COOKIE/CMD MATCH, RETURN/PAYLOAD LENGTH 0x00000018\nqc07f> ").encode()


class MemoryFile(io.BytesIO):
    def __exit__(self, *args): return False
    def fileno(self): return 123


class TextFile(io.StringIO):
    def __exit__(self, *args): return False


def simulate(fault=None, refresh=False, warning=WARNING, split=None, repeat=False):
    """No real FS, process discovery, socket, clock sleep, serial, or private I/O."""
    h = history()
    _, boot, _, failed, _, cp, _, _, _ = h
    clock = [0.0]; sessions = []; logs = []; stdout = io.StringIO()
    files = {"control/peripheral-gui-operation.lock": MemoryFile(b"FOREIGN" if fault == "foreign_HOLD" else HOLD)}
    if fault == "existing_public": files["ev/" + i.PUBLIC] = TextFile("preserved historical result")
    dirs = {"base", "control", "ev"}
    initial_stat = types.SimpleNamespace(st_mode=stat.S_IFREG | 0o600, st_dev=1, st_ino=2, st_uid=1000, st_gid=1000, st_nlink=1)
    class P:
        def __init__(self, value): self.value = str(value)
        def __str__(self): return self.value
        def __truediv__(self, value): return P(self.value + "/" + str(value))
        def lstat(self):
            if fault == "inode" and sessions: return types.SimpleNamespace(**dict(vars(initial_stat), st_ino=3))
            if fault == "mode" and sessions: return types.SimpleNamespace(**dict(vars(initial_stat), st_mode=stat.S_IFREG | 0o644))
            return initial_stat
        def mkdir(self, **kwargs):
            if self.value in dirs: raise FileExistsError("synthetic")
            dirs.add(self.value)
        def open(self, mode):
            assert mode == "x" and self.value not in files
            files[self.value] = TextFile(); return files[self.value]
    def fake_handoff():
        if fault == "handoff": raise AssertionError("SYNTHETIC_PRIVATE_ERROR")
        if fault == "cp_HOLD": cp["original_HOLD"] += " "
        return POST, boot, failed, cp, prior.BASELINE, SNAPSHOT, cp["tx_bytes"]
    class Session(i.b.Boot):
        # Inherit actual send, status, check_stream, received and wait.
        def __init__(self, output, capture_dir):
            output.mkdir(mode=0o700, exist_ok=False)
            self.output = output; self.call = self.rpc; self.peer = (123, 1000, 1000)
            self.state = {"part": i.meta.PART}
            self.epoch_cursor = self.transfer_cursor = self.transfer_fault = None
            self.epoch_tail = b""; self.capture_deadline = None
            self.tx = cp["tx_bytes"] + int(fault == "initial_TX")
            self.raw = bytearray(SNAPSHOT); self.recent = "qc07f> late passive line\n" if refresh else "qc07f> "
            self.pending_bytes = b""; self.calls = []; self.wires = []; self.attempts = []; self.owners = 0
            self.state_done = False; self.info_done = False; self.zero_done = False
            self.parsed_state = False
            self.between_reads = 0
            if fault == "snapshot": self.raw[0] ^= 1
            if fault == "gap_warning": self.raw.extend(warning)
            if fault == "gap_topology": self.raw.extend(b"usb 1-1: USB disconnect, device number 3\n")
            if fault == "gap_split":
                # The saved retirement boundary ends in a partial line. The
                # remaining bytes arrived unattended before current admission.
                self.raw.extend(warning[:split]); cp["rx_bytes"] = len(self.raw)
                self.raw.extend(warning[split:])
            if fault == "carry_oversize":
                self.raw.extend(b"x" * 1025); cp["rx_bytes"] = len(self.raw)
            sessions.append(self)
        def admit_state(self, state): pass
        def raw_since(self, start, state):
            assert 0 <= start <= state["rx_bytes"] <= len(self.raw)
            return bytes(self.raw[start:state["rx_bytes"]])
        def snapshot(self):
            return {"part": "foreign" if fault == "part" else i.meta.PART,
                    "tx_bytes": self.tx, "rx_bytes": len(self.raw), "recent_rx": self.recent,
                    "tx_busy": fault == "busy", "abort_armed": fault != "catch", "conditional_send_v1": True}
        def owner(self, margin):
            self.owners += 1
            if self.owners == 1:
                if fault == "owner_stall": clock[0] = 301
                if fault == "owner_admission_in_budget": clock[0] = 161
                self.capture_deadline = {"coverage600": 599, "coverage600exact": 600}.get(fault, 1000)
                if fault == "peer": self.peer = (456, 1000, 1000)
            elif fault == "repin": self.peer = (456, 1000, 1000)
            if self.owners == 3 and fault == "coverage300": self.capture_deadline = clock[0] + 299
            assert self.capture_deadline - clock[0] >= margin
            return self.snapshot()
        def rpc(self, request, expected_peer=None):
            if request["action"] == "status":
                if fault == "between_warning" and self.parsed_state and not self.info_done:
                    self.raw.extend(warning)
                if fault == "between_split" and self.parsed_state and not self.info_done:
                    if self.between_reads < 2:
                        part = warning[:split] if self.between_reads == 0 else warning[split:]
                        self.raw.extend(part)
                        if part: self.recent = part.decode()
                    self.between_reads += 1
                return self.snapshot(), self.peer
            assert request["action"] == "send"
            wire = bytes.fromhex(request["hex"]); self.attempts.append(wire)
            if fault in ("zero", "zero_warning", "zero_deadline", "zero_TX") and not self.zero_done:
                self.zero_done = True
                if fault == "zero_warning": self.raw.extend(warning)
                if fault == "zero_deadline": clock[0] = 291
                if fault == "zero_TX": self.tx += 1
                return {"not_sent": True}, self.peer
            if fault == "partial": return {"transmitted_bytes": len(wire) - 1}, self.peer
            if fault == "ambiguous": return {}, self.peer
            self.wires.append(wire); self.tx += len(wire)
            if not wire.endswith(b"\r"):
                self.pending_bytes += wire; self.raw.extend(wire)
            else:
                command = (self.pending_bytes + wire[:-1]).decode(); self.pending_bytes = b""; self.calls.append(command)
                self.respond(command)
            return {"transmitted_bytes": len(wire)}, self.peer
        def respond(self, command):
            if command == "": answer = b"\r\nqc07f> "
            elif command == "shell": answer = b"\r\n# "
            elif command.startswith("echo K76MD_BEGIN_"):
                nonce = command.split("K76MD_BEGIN_", 1)[1][:32]
                text = md.fixture().replace(md.NONCE, nonce)
                if fault == "FD": text = text.replace("3 -> /dev/console", "3 -> /dev/watchdog0")
                if fault == "descriptor": text = text.replace("ATTR:speed\n480", "ATTR:speed\n12")
                if fault == "post_descriptor" and self.info_done: text = text.replace("ATTR:speed\n480", "ATTR:speed\n12")
                answer = text.encode()
                if fault == "incomplete_metadata": answer = b"K76MD_BEGIN_" + nonce.encode()
            elif command == "exit":
                answer = b"RAM-only shell returned to monitor\nqc07f> "
                if fault == "state_floor" and not self.info_done: clock[0] = 161
                if fault == "reserve_after_read" and not self.info_done: clock[0] = 291
            elif command == "radio state":
                self.state_done = True; answer = state_reply()
                if fault == "state_negative": answer = answer.replace(b"00ff00", b"000000")
                if fault == "state_short": answer = answer.replace(b"0000fd", b"00fd")
                if fault == "state_extra": answer = b"extra\n" + answer
                if fault == "state_interleaved": answer = answer.replace(b"RADIO STATE", b"printk noise\nRADIO STATE")
                if fault == "state_prompt": answer += b"qc07f> "
                if fault == "state_late": answer = answer[:-7]
                if fault == "state_warning": answer = warning + answer
                if fault == "info_floor": clock[0] = 171
            elif command == "radio info":
                self.info_done = True; answer = info_reply()
                if fault == "info_bad": answer = answer.replace(b"01000102", b"00000102")
            else:
                key = next(k for k, c in POST.QUERIES.items() if command == c)
                value = prior.BASELINE[key]
                if fault == "UUID" and key == "boot_id": value = "11234567-89ab-4cde-8f01-23456789abcd"
                if fault == "idle" and key == "watchdog": value = "active"
                if fault == "post_idle" and self.info_done and key == "watchdog": value = "active"
                answer = (value + "\nqc07f> ").encode()
            self.raw.extend(answer)
            self.recent = "# " if command == "shell" or command.startswith("echo K76MD_BEGIN_") else "qc07f> "
        def log(self, event, **kwargs): logs.append((event, kwargs))
    error = None
    with contextlib.ExitStack() as stack:
        stack.enter_context(patch.multiple(i, BASE=P("base"), EVIDENCE=P("ev"), handoff=fake_handoff))
        stack.enter_context(patch.object(i.meta.private, "private_path", side_effect=lambda p, **k: p))
        stack.enter_context(patch.object(i.meta.private, "PrivateBoot", Session))
        parse_state = i.trial.receipt.parse_application_state
        def parsed_state(text):
            result = parse_state(text)
            sessions[0].parsed_state = True
            return result
        stack.enter_context(patch.object(i.trial.receipt, "parse_application_state", side_effect=parsed_state))
        stack.enter_context(patch.object(i.b, "CONTROL", P("control")))
        stack.enter_context(patch.object(i.b, "ticks", return_value=43 if fault == "start" else 42))
        for name, value in zip(("getuid", "geteuid", "getgid", "getegid"), IDS):
            if fault == "root": value = 0
            if fault == "effective_uid" and name == "geteuid": value += 1
            if fault == "effective_gid" and name == "getegid": value += 1
            stack.enter_context(patch.object(i.os, name, return_value=value))
        stack.enter_context(patch.object(i.os, "umask"))
        stack.enter_context(patch.object(i.os, "open", return_value=123))
        def fdopen(*args):
            value = files["control/peripheral-gui-operation.lock"]; value.seek(0); return value
        stack.enter_context(patch.object(i.os, "fdopen", side_effect=fdopen))
        stack.enter_context(patch.object(i.os.path, "lexists", side_effect=lambda p: str(p) in files or str(p) in dirs))
        stack.enter_context(patch.object(i.os, "fstat", return_value=initial_stat))
        stack.enter_context(patch.object(i.fcntl, "flock"))
        stack.enter_context(patch.object(i.time, "monotonic", side_effect=lambda: clock[0]))
        stack.enter_context(patch.object(i.time, "sleep", side_effect=lambda n: clock.__setitem__(0, clock[0] + n)))
        stack.enter_context(patch.object(i.sys, "argv", ["identity21a.py", i.ARG]))
        stack.enter_context(contextlib.redirect_stdout(stdout))
        for _ in range(2 if repeat else 1):
            try: i.main()
            except (Exception, KeyboardInterrupt) as exc: error = type(exc).__name__
    return {"error": error, "HOLD": files["control/peripheral-gui-operation.lock"].getvalue(),
            "files": files, "dirs": dirs, "stdout": stdout.getvalue(), "logs": logs,
            "calls": sessions[0].calls if sessions else [], "wires": sessions[0].wires if sessions else [],
            "attempts": sessions[0].attempts if sessions else [], "sessions": len(sessions),
            "PASS": "ev/" + i.PUBLIC in files}


class Lineage(unittest.TestCase):
    def test_all_eight_refresh_variants(self):
        for flags in itertools.product((False, True), repeat=3):
            args = history(*flags)
            self.assertEqual(i.lineage(*args), 2156 + sum(flags))

    def test_missing_partial_extra_interleaved_tx_refuse(self):
        for fault in ("missing", "partial", "extra", "nested", "unknown", "ordinary", "private", "pending", "boundary"):
            args = history(); events = args[-1]
            if fault == "missing": del events[-1]
            elif fault == "partial": events[-1]["bytes"] -= 1
            elif fault == "extra": events += [{"kind": "tx_protected_intent", "bytes": 1}, {"kind": "tx_complete", "bytes": 1}]
            elif fault == "nested": events.insert(-1, dict(events[-2]))
            elif fault in ("unknown", "ordinary", "private"):
                events.insert(1, {"kind": {"unknown": "tx_abort", "ordinary": "tx_intent", "private": "tx_private_intent"}[fault], "bytes": 1})
            elif fault == "pending": events.append({"kind": "tx_protected_intent", "bytes": 1})
            elif fault == "boundary":
                # Same total but a single frame straddles Cboot.
                events[0]["bytes"] += 1; events[1]["bytes"] += 1
                events[2]["bytes"] -= 1; events[3]["bytes"] -= 1
            with self.subTest(fault=fault), self.assertRaises(AssertionError): i.lineage(*args)

    def test_stale_checkpoints_completion_and_typed_refresh(self):
        for pos, key, value in ((3, "tx_bytes", 1297), (3, "checkpoint_tx_bytes", 0),
                                (5, "tx_bytes", 1299), (5, "original_trial_still_failed", False),
                                (6, "recipe_sha256", "stale"), (7, "tx_bytes", 0), (7, "rx_bytes", 0),
                                (7, "event", "started"), (2, "one_empty_monitor_refresh", 1),
                                (3, "optional_initial_monitor_refresh", 1), (6, "one_empty_monitor_refresh", 1)):
            args = history(); args[pos][key] = value
            with self.subTest(key=key, pos=pos), self.assertRaises(AssertionError): i.lineage(*args)


class Handoff(unittest.TestCase):
    def exercise(self, fault=None):
        """Actual handoff, all source/public/private FS reads in memory."""
        args = history()
        _, boot, idle, failed, pre, cp, receipt, completion, events = args
        source = b"PUBLIC_SYNTHETIC_SOURCE"
        pin = hashlib.sha256(source).hexdigest()
        source_name = "apps/qualification/kernel76-transport/identity21a.py"
        data = {
            "review/identity-parent-host01/result.json": {"input_sha256": {source_name: pin}},
            "ev/RADIO_READONLY_RETIREMENT21A01.json": receipt,
            "ev/RADIO_NATIVE76_21A01_HOLD.json": {"state": "RADIO76_STRICT_TRIAL_HOLD_RUN_RECEIPT_INTERLEAVED",
                "HOLD_retained": True, "application_state_info_queries_sent": False},
            "base/radio-parent-preflight21a01/events.jsonl": [pre],
            "base/radio-retirement21a01/events.jsonl": [completion],
            "base/uart-capture21a/uart-events-foundation-20261021a.jsonl": events,
        }
        private = {"radio-failed-trial21a01.private-checkpoint.json": failed,
                   "runtime21a01.private.json": prior.BASELINE.copy(),
                   "radio-retirement21a01.private-checkpoint.json": cp}
        if fault == "missing_completion": data["base/radio-retirement21a01/events.jsonl"] = []
        if fault == "partial_completion": completion.pop("rx_bytes")
        if fault == "duplicate_completion": data["base/radio-retirement21a01/events.jsonl"] *= 2
        if fault == "later_failure": data["base/radio-retirement21a01/events.jsonl"].append({"event": "failed"})
        if fault == "saved_rows": cp["rows"]["watchdog"] = "active"
        if fault == "saved_uuid": private["runtime21a01.private.json"]["boot_id"] = "foreign"
        if fault == "saved_metadata": cp["metadata"]["PID1_console_fds_only"] = False
        if fault == "peer": cp["peer"][1] += 1
        if fault == "start": cp["start_ticks"] += 1
        if fault == "part": cp["part"] = "foreign"
        if fault == "image": failed["image_sha256"] = "stale"
        if fault == "stale_failed": failed["tx_bytes"] += 1
        if fault == "stale_retired": cp["tx_bytes"] += 1
        if fault == "rebase": cp["tx_bytes"] = 999999
        if fault == "source": data["review/identity-parent-host01/result.json"]["input_sha256"][source_name] = "stale"
        if fault == "absent_checkpoint": del private["radio-retirement21a01.private-checkpoint.json"]
        class P:
            def __init__(self, name): self.name = str(name)
            def __truediv__(self, name): return P(self.name + "/" + str(name))
            def with_name(self, name): return P("source/" + name)
            def read_bytes(self):
                if self.name.endswith("saved-private.bin"): return SNAPSHOT
                return source
            def read_text(self):
                value = data[self.name]
                if self.name.endswith("jsonl"): return "\n".join(map(json.dumps, value))
                return json.dumps(value)
        with patch.multiple(i, BASE=P("base"), EVIDENCE=P("ev"), ROOT=P("root"), REVIEW=P("review"), Path=P,
                            SOURCE_PINS={"retire21a.py": pin}), \
             patch.object(i.meta, "admitted_handoff", return_value=(POST, {}, idle, boot, 421)), \
             patch.object(i.retire, "read_private", side_effect=lambda name: private[name]), \
             patch.object(i.meta.private, "private_path", side_effect=lambda p: p), \
             patch.object(i.os, "getuid", return_value=1000), patch.object(i.os, "getgid", return_value=1000):
            return i.handoff()

    def test_native_checkpoint_and_completion_provenance_composition(self):
        self.assertEqual(self.exercise()[-1], 2159)

    def test_foreign_stale_missing_and_partial_provenance_refuse(self):
        for fault in ("missing_completion", "partial_completion", "duplicate_completion", "later_failure", "saved_rows", "saved_uuid",
                      "saved_metadata", "peer", "start", "part", "image", "stale_failed", "stale_retired", "rebase", "source", "absent_checkpoint"):
            with self.subTest(fault=fault), self.assertRaises((AssertionError, KeyError)): self.exercise(fault)


@contextlib.contextmanager
def guard_fixture(program=None, fault=None):
    """Internal-boundary fake, retaining actual Boot.send/check_stream/wait."""
    raw = bytearray(b"qc07f> "); clock = [0]; tx = [100]; requests = []; statuses = [0]
    held = MemoryFile(HOLD)
    session = i.b.Boot.__new__(i.b.Boot)
    session.peer = (123, 1000, 1000); session.state = {"part": i.meta.PART}
    session.epoch_cursor = session.transfer_cursor = session.transfer_fault = None
    session.epoch_tail = b""; session.capture_deadline = 600
    session.raw_since = lambda start, state: bytes(raw[start:state["rx_bytes"]])
    session.admit_state = lambda state: None
    def rpc(request, expected_peer=None):
        if request["action"] == "status":
            statuses[0] += 1
            if statuses[0] >= 2:
                if fault == "blocking_status": clock[0] = 291
                if fault == "blocking_state_floor": clock[0] = 161
                if fault == "blocking_info_floor": clock[0] = 171
                if fault == "coverage": session.capture_deadline = clock[0] + 299
                if fault == "newTX": tx[0] += 1
                if fault == "blocking_partial": raw.extend(b"Process 1")
            return {"part": i.meta.PART, "tx_bytes": tx[0], "rx_bytes": len(raw), "abort_armed": True,
                    "tx_busy": False, "conditional_send_v1": True, "recent_rx": raw.decode()}, session.peer
        requests.append(request.copy())
        if fault == "zero_exhausted": return {"not_sent": True}, session.peer
        if fault == "partial": return {"transmitted_bytes": 1}, session.peer
        if fault == "ambiguous": raise OSError("synthetic ambiguous")
        size = len(bytes.fromhex(request["hex"])); tx[0] += size
        return {"transmitted_bytes": size}, session.peer
    session.call = rpc
    identity = types.SimpleNamespace(st_mode=stat.S_IFREG | 0o600, st_dev=1, st_ino=2,
                                     st_uid=1000, st_gid=1000, st_nlink=1)
    path = types.SimpleNamespace(lstat=lambda: identity)
    with contextlib.ExitStack() as stack:
        for name, value in zip(("getuid", "geteuid", "getgid", "getegid"), IDS):
            stack.enter_context(patch.object(i.os, name, return_value=value))
        stack.enter_context(patch.object(i.os, "fstat", return_value=identity))
        stack.enter_context(patch.object(i.b, "ticks", return_value=42))
        stack.enter_context(patch.object(i.time, "monotonic", side_effect=lambda: clock[0]))
        stack.enter_context(patch.object(i.time, "sleep", side_effect=lambda n: clock.__setitem__(0, clock[0] + n)))
        guard = i.Guard(session, held, HOLD, session.peer, 42, 0, len(raw), 300, 100,
                        program or [("pre", b"shell\r")], path, i.file_identity(identity), IDS)
        guard.check_stream()
        yield guard, session, requests, raw, clock, tx, held


class InternalLedger(unittest.TestCase):
    def test_direct_session_send_cannot_bypass_pending_wire(self):
        with guard_fixture() as (guard, session, requests, *_):
            with self.assertRaises(AssertionError): session.send(b"shell\r", "bypass facade")
            self.assertEqual(requests, []); self.assertTrue(guard.frozen)

    def test_internal_rpc_requires_exact_next_hex_cursor_peer_and_phase(self):
        for fault in ("wire", "cursor", "peer", "phase", "action"):
            with guard_fixture([("state", b"radio state\r")]) as (guard, session, requests, raw, *_):
                guard.pending = b"radio state\r"; guard.pre_valid = fault != "phase"
                request = {"action": "send", "hex": guard.pending.hex(), "expected_rx_bytes": len(raw)}
                peer = session.peer
                if fault == "wire": request["hex"] = b"radio native-load\r".hex()
                if fault == "cursor": request["expected_rx_bytes"] += 1
                if fault == "peer": peer = (456, 1000, 1000)
                if fault == "action": request["action"] = "arm_abort"
                with self.subTest(fault=fault), self.assertRaises(AssertionError): guard.call(request, expected_peer=peer)
                self.assertEqual(requests, [])

    def test_partial_or_ambiguous_freezes_and_no_replay(self):
        for fault in ("partial", "ambiguous"):
            with guard_fixture(fault=fault) as (guard, session, requests, *_):
                with self.assertRaises((AssertionError, OSError)): guard.send(b"shell\r", "fixed")
                self.assertTrue(guard.frozen); self.assertEqual(guard.expected_tx, 100)
                with self.assertRaises(AssertionError): guard.send(b"shell\r", "no replay")
                self.assertEqual(len(requests), 1)

    def test_exact_completed_tx_updates_before_next_stream_and_zero_exhaustion(self):
        with guard_fixture() as (guard, session, requests, *_):
            guard.send(b"shell\r", "fixed")
            self.assertEqual(guard.expected_tx, 106); self.assertEqual(guard.index, 1)
            self.assertEqual(guard.check_stream()["tx_bytes"], 106)
            with self.assertRaises(AssertionError): guard.send(b"shell\r", "no second program")
            self.assertEqual(len(requests), 1)
        with guard_fixture(fault="zero_exhausted") as (guard, session, requests, *_):
            with self.assertRaises(i.b.Refused): guard.send(b"shell\r", "fixed")
            self.assertEqual(guard.expected_tx, 100); self.assertEqual(guard.index, 0)
            self.assertEqual(len(requests), 5)

    def test_blocking_status_time_coverage_and_tx_rechecked_before_actual_send(self):
        for fault, phase, wire in (("blocking_status", "pre", b"shell\r"), ("coverage", "pre", b"shell\r"),
                                   ("newTX", "pre", b"shell\r"), ("blocking_state_floor", "state", b"radio state\r"),
                                   ("blocking_info_floor", "info", b"radio info\r"), ("blocking_partial", "info", b"radio info\r")):
            with guard_fixture([(phase, wire)], fault) as (guard, session, requests, *_):
                guard.pre_valid = guard.application = True
                with self.subTest(fault=fault), self.assertRaises(AssertionError): guard.send(wire, "fixed")
                self.assertEqual(requests, [])

    def test_rx_rollback_and_external_hold_change_refuse_without_overwrite(self):
        with guard_fixture() as (guard, session, requests, raw, clock, tx, held):
            guard.check_stream(); guard.last_rx += 1
            with self.assertRaises(AssertionError): guard.send(b"shell\r", "fixed")
            self.assertEqual(requests, [])
        with guard_fixture() as (guard, session, requests, raw, clock, tx, held):
            held.seek(0); held.truncate(); held.write(b"EXTERNAL CHANGE")
            with self.assertRaises(AssertionError): guard.send(b"shell\r", "fixed")
            self.assertEqual(requests, []); self.assertEqual(held.getvalue(), b"EXTERNAL CHANGE")

    def test_identity_coverage_deadline_and_phase_flags_are_not_rebased(self):
        for fault in ("uid", "gid", "start", "peer", "deadline", "coverage", "info", "post"):
            phase = fault if fault in ("info", "post") else "pre"
            wire = b"radio info\r" if phase == "info" else b"shell\r"
            with guard_fixture([(phase, wire)]) as (guard, session, requests, raw, clock, *_):
                with contextlib.ExitStack() as stack:
                    if fault == "uid": stack.enter_context(patch.object(i.os, "geteuid", return_value=1001))
                    if fault == "gid": stack.enter_context(patch.object(i.os, "getegid", return_value=1001))
                    if fault == "start": stack.enter_context(patch.object(i.b, "ticks", return_value=43))
                    if fault == "peer": session.peer = (456, 1000, 1000)
                    if fault == "deadline": clock[0] = 301
                    if fault == "coverage": session.capture_deadline = 299
                    with self.subTest(fault=fault), self.assertRaises(AssertionError): guard.send(wire, "fixed")
                    self.assertEqual(requests, [])

    def test_wait_receive_checks_full_span_and_does_not_advance_rejected_cursor(self):
        for text in (WARNING, b"Linux version new\n", b"U-Boot 2013.10\n",
                     b"musb-hdrc musb-hdrc.0.auto: Babble\n", b"usb 1-1: USB disconnect, device number 3\n"):
            with guard_fixture() as (guard, session, requests, raw, *_):
                session.epoch_cursor = 0
                raw.extend(text)
                with self.subTest(text=text), self.assertRaises(AssertionError): guard.wait(b"qc07f> ", 0, 6)
                self.assertEqual(session.epoch_cursor, 0); self.assertEqual(requests, [])


class Controller(unittest.TestCase):
    def test_two_complete_programs_inherit_real_send_wait(self):
        for refresh in (False, True):
            result = simulate(refresh=refresh)
            self.assertIsNone(result["error"])
            self.assertTrue(result["PASS"])
            self.assertEqual(result["HOLD"], HOLD)
            self.assertEqual(sum(map(len, result["wires"])), 1741 + int(refresh))
            self.assertEqual(len(result["wires"]), 54 + int(refresh))
            self.assertEqual(result["calls"].count("radio state"), 1)
            self.assertEqual(result["calls"].count("radio info"), 1)
            self.assertEqual(result["calls"].count(""), int(refresh))

    def test_all_pre_refusals_have_zero_active_queries(self):
        for fault in ("foreign_HOLD", "handoff", "cp_HOLD", "initial_TX", "snapshot", "coverage600", "owner_stall", "owner_admission_in_budget",
                      "root", "effective_uid", "effective_gid", "peer", "start",
                      "inode", "mode", "part", "busy", "catch", "gap_warning", "gap_topology", "carry_oversize", "FD", "descriptor",
                      "incomplete_metadata", "UUID", "idle", "repin", "state_floor", "reserve_after_read"):
            result = simulate(fault)
            with self.subTest(fault=fault):
                self.assertIsNotNone(result["error"])
                self.assertFalse(result["PASS"])
                self.assertEqual(result["HOLD"], b"FOREIGN" if fault == "foreign_HOLD" else HOLD)
                self.assertFalse(any(c.startswith("radio ") for c in result["calls"]))
                if fault in ("FD", "descriptor"):
                    self.assertEqual(result["calls"][-1], "exit")
                if fault == "incomplete_metadata": self.assertNotIn("exit", result["calls"])

    def test_every_negative_state_sends_no_info_or_cleanup(self):
        for fault in ("state_negative", "state_short", "state_extra", "state_interleaved", "state_prompt", "state_late",
                      "state_warning", "between_warning", "info_floor"):
            result = simulate(fault)
            with self.subTest(fault=fault):
                self.assertIsNotNone(result["error"]); self.assertFalse(result["PASS"])
                self.assertEqual(result["calls"][-1], "radio state")
                self.assertNotIn("radio info", result["calls"]); self.assertEqual(result["HOLD"], HOLD)

    def test_info_and_postfailures_keep_hold(self):
        for fault in ("info_bad", "post_descriptor", "post_idle", "coverage300"):
            result = simulate(fault)
            with self.subTest(fault=fault):
                self.assertIsNotNone(result["error"]); self.assertFalse(result["PASS"])
                self.assertEqual(result["HOLD"], HOLD)
                self.assertEqual(result["calls"].count("radio info"), 1)
                if fault == "info_bad": self.assertEqual(result["calls"][-1], "radio info")

    def test_zero_refusal_never_advances_or_replays_partial(self):
        result = simulate("zero")
        self.assertIsNone(result["error"]); self.assertTrue(result["PASS"])
        self.assertEqual(len(result["attempts"]), 55); self.assertEqual(len(result["wires"]), 54)
        self.assertEqual(result["attempts"][0], result["attempts"][1])
        for fault in ("zero_warning", "zero_deadline", "zero_TX", "partial", "ambiguous"):
            result = simulate(fault)
            with self.subTest(fault=fault):
                self.assertIsNotNone(result["error"]); self.assertFalse(result["PASS"])
                self.assertEqual(len(result["attempts"]), 1); self.assertEqual(result["HOLD"], HOLD)

    def test_all_warning_and_topology_splits_use_saved_retirement_rx(self):
        messages = (WARNING, WARNING.replace(b"CLEAR_HALT", b"RESETEP"),
                    b"usb 1-1: USB disconnect, device number 3\n",
                    b"usb 1-1: new high-speed USB device number 3 using musb-hdrc\n",
                    b"usb 1-1: reset high-speed USB device number 3 using musb-hdrc\n")
        for message in messages:
            for split in range(len(message)):
                result = simulate("gap_split", warning=message, split=split)
                with self.subTest(message=message, split=split):
                    self.assertIsNotNone(result["error"]); self.assertEqual(result["wires"], [])
                    self.assertEqual(result["HOLD"], HOLD)

    def test_active_warning_all_splits_between_state_and_info(self):
        for message in (WARNING, WARNING.replace(b"CLEAR_HALT", b"RESETEP")):
            for split in range(len(message) + 1):
                result = simulate("between_split", warning=message, split=split)
                with self.subTest(message=message, split=split):
                    self.assertIsNotNone(result["error"]); self.assertFalse(result["PASS"])
                    self.assertNotIn("radio info", result["calls"]); self.assertEqual(result["HOLD"], HOLD)

    def test_no_private_stdout_or_public_receipt(self):
        result = simulate()
        self.assertTrue(result["PASS"])
        public = result["files"]["ev/" + i.PUBLIC].getvalue()
        for value in (prior.UUID, "PRIVATE_TAIL", b"PRIVATE_TAIL".hex(), "tx_bytes", "rx_bytes", "RADIO RX BYTES"):
            self.assertNotIn(value, public + result["stdout"])
        self.assertTrue(any(event == "separate_identity_raw_private" for event, _ in result["logs"]))
        self.assertNotIn("SYNTHETIC_PRIVATE_ERROR", simulate("handoff")["stdout"])

    def test_attempt_consumed_on_refusal_and_success(self):
        for fault in (None, "handoff", "state_negative"):
            result = simulate(fault, repeat=True)
            self.assertEqual(result["error"], "FileExistsError")
            self.assertEqual(result["sessions"], 0 if fault == "handoff" else 1)
            self.assertEqual(result["HOLD"], HOLD)

    def test_exact_initial_coverage_and_existing_public_output(self):
        self.assertTrue(simulate("coverage600exact")["PASS"])
        result = simulate("existing_public")
        self.assertIsNotNone(result["error"]); self.assertEqual(result["wires"], [])
        self.assertEqual(result["files"]["ev/" + i.PUBLIC].getvalue(), "preserved historical result")


if __name__ == "__main__": unittest.main()
