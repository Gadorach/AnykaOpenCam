#!/usr/bin/env python3
"""Separate identity candidate. SOURCE review required; never repairs old HOLD.

Only main performs private I/O. Parsers and the ordered lineage proof are pure.
No old controller main, broad monitor allowlist, reset, or cleanup-on-error path.
"""
from pathlib import Path
import fcntl
import hashlib
import json
import os
import re
import stat
import sys
import time
import uuid
import retire21a as retire

meta, trial, b = retire.meta, retire.trial, retire.b
BASE, EVIDENCE, ROOT = meta.BASE, meta.EVIDENCE, meta.ROOT
REVIEW = ROOT / "docs/qualification-plans/kernel76-transport-20261021"
RETIRE_SHA = "aeda5136b55d3cec920e73adcd41209f52c978b526e4ba1709908a6398caa9d1"
SOURCE_PINS = dict(retire.SOURCE_PINS, **{"retire21a.py": RETIRE_SHA})
ACTIVE_ENDPOINT = re.compile(rb"Process [^\r\n]*called USBDEVFS_(?:CLEAR_HALT|RESETEP) for active endpoint", re.I)
ARG = "--one-separate-identity-preserve-HOLD21a"
OUTPUT = "radio-identity21a01"
PUBLIC = "RADIO_SEPARATE_IDENTITY21A01.json"

if not __debug__:
    raise SystemExit("Assertions required")


def integer(value):
    assert type(value) is int and value >= 0
    return value


def inventory_wires(post):
    wires = [c.encode() + b"\r" for c in list(post.QUERIES.values()) + [post.QUERIES["boot_id"]]]
    assert len(wires) == 14 and sum(map(len, wires)) == 320
    return wires


def cached_wires(nonce):
    command = meta.script(nonce).encode()
    assert len(command) == 527
    return [b"shell\r"] + [command[i:i + 64] for i in range(0, 527, 64)] + [b"\r", b"exit\r"]


def lineage(post, boot, idle, failed, pre, retirement, receipt, completion, events):
    """Length/order/counter provenance, NOT proof of the historical TX payload.

    Recorder TX starts at zero and increases ONLY on tx_complete. Every protected
    intent must have exactly one equal complete, even before Cboot. Boundaries
    may not bisect a frame or sit between its intent and completion.
    """
    rp, rt, rr = idle["one_empty_monitor_refresh"], failed["optional_initial_monitor_refresh"], receipt["one_empty_monitor_refresh"]
    assert all(type(x) is bool for x in (rp, rt, rr))
    cb = integer(boot["tx_bytes"])
    cp = cb + 320 + int(rp)
    cf = cb + 1197 + int(rp) + int(rt)
    cr = cb + 2056 + int(rp) + int(rt) + int(rr)
    assert failed["state"] == "OFFLINE_FAILED_TRIAL_TX_DERIVATION_VERIFIED"
    assert integer(failed["checkpoint_tx_bytes"]) == cp
    assert integer(failed["tx_bytes"]) == cf and integer(retirement["tx_bytes"]) == cr
    assert pre["event"] == "parent_before_trial_admission" and pre["target_TX"] is False
    assert integer(pre["tx_bytes"]) == cp
    assert failed["original_preflight_rx_bytes"] == pre["rx_bytes"]
    assert failed["approved_recipe_send_lengths"] == retire.wire_lengths(post)
    lengths = ([1] if rt else []) + retire.wire_lengths(post)
    assert failed["completed_sends_count"] == len(lengths)
    assert receipt["state"] == "READONLY_POSTFAILURE_RETIREMENT_OBSERVED_ORIGINAL_TRIAL_FAILED_HOLD"
    assert receipt["recipe_sha256"] == RETIRE_SHA
    assert receipt["original_trial_failed_HOLD_preserved_byte_exact"] is True
    assert receipt["no_radio_USB_query_reset_module_network_media_or_persistent_write"] is True
    assert retirement["original_trial_still_failed"] is True
    assert completion["event"] == "readonly_retirement_complete_original_HOLD_preserved"
    assert integer(completion["tx_bytes"]) == cr
    assert integer(completion["rx_bytes"]) == integer(retirement["rx_bytes"])
    total, pending, frames, boundaries = 0, None, [], {0}
    for row in events:
        kind = row["kind"]
        if not kind.startswith("tx"):
            continue
        assert kind in ("tx_protected_intent", "tx_complete")
        size = integer(row["bytes"]); assert size > 0
        if kind == "tx_protected_intent":
            assert pending is None
            pending = size
        else:
            assert pending == size
            frames.append((total, total + size, size))
            total += size; boundaries.add(total); pending = None
    assert pending is None and total == cr
    assert all(c in boundaries for c in (cb, cp, cf, cr))
    def segment(lo, hi):
        return [n for start, end, n in frames if lo <= start and end <= hi]
    assert segment(cb, cp) == ([1] if rp else []) + list(map(len, inventory_wires(post)))
    assert segment(cp, cf) == lengths
    assert segment(cf, cr) == ([1] if rr else []) + list(map(len, cached_wires("1" * 32) + inventory_wires(post)))
    return cr


def file_identity(s):
    assert stat.S_ISREG(s.st_mode) and s.st_nlink == 1
    return s.st_dev, s.st_ino, s.st_mode, s.st_uid, s.st_gid, s.st_nlink


def hold_gate(held, path, identity, content):
    assert file_identity(os.fstat(held.fileno())) == identity
    assert file_identity(path.lstat()) == identity
    held.seek(0); assert held.read() == content


def handoff():
    """Private reads only under main's original exclusive lock, never in tests."""
    for name, pin in SOURCE_PINS.items():
        assert hashlib.sha256(Path(__file__).with_name(name).read_bytes()).hexdigest() == pin
    # The authored HOST result freezes this candidate as well as all reviewed
    # public inputs. It is NOT an independent-review or hardware admission token.
    freeze = json.loads((REVIEW / "identity-parent-host01/result.json").read_text())
    for name, pin in freeze["input_sha256"].items():
        assert hashlib.sha256((ROOT / name).read_bytes()).hexdigest() == pin
    assert freeze["input_sha256"]["apps/qualification/kernel76-transport/identity21a.py"] == hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
    post, _, idle, boot, _ = meta.admitted_handoff()
    read = retire.read_private
    failed = read("radio-failed-trial21a01.private-checkpoint.json")
    runtime = read("runtime21a01.private.json")
    checkpoint = read("radio-retirement21a01.private-checkpoint.json")
    receipt = json.loads((EVIDENCE / "RADIO_READONLY_RETIREMENT21A01.json").read_text())
    failure = json.loads((EVIDENCE / "RADIO_NATIVE76_21A01_HOLD.json").read_text())
    assert failure["state"] == "RADIO76_STRICT_TRIAL_HOLD_RUN_RECEIPT_INTERLEAVED"
    assert failure["HOLD_retained"] is True and failure["application_state_info_queries_sent"] is False
    def rows(name):
        return [json.loads(x) for x in meta.private.private_path(BASE / name).read_text().splitlines()]
    pre = rows("radio-parent-preflight21a01/events.jsonl")
    assert len(pre) == 1
    log = rows("radio-retirement21a01/events.jsonl")
    completed = [x for x in log if x["event"] == "readonly_retirement_complete_original_HOLD_preserved"]
    assert len(completed) == 1 and log[-1] == completed[0]
    events = rows("uart-capture21a/uart-events-foundation-20261021a.jsonl")
    expected = lineage(post, boot, idle, failed, pre[0], checkpoint, receipt, completed[0], events)
    for item in (failed, checkpoint):
        assert item["part"] == meta.PART and item["image_sha256"] == meta.SHA
        assert item["start_ticks"] == boot["start_ticks"]
    assert failed["recorder_pid"] == boot["recorder_pid"]
    assert len(checkpoint["peer"]) == 3 and all(type(x) is int for x in checkpoint["peer"])
    assert checkpoint["peer"] == [boot["recorder_pid"], os.getuid(), os.getgid()]
    post.parse(checkpoint["rows"])
    assert checkpoint["rows"]["boot_id"].strip() == runtime["boot_id"].strip()
    assert checkpoint["metadata"] == receipt["owned_shell_returned_with_cached_metadata"]
    assert len(checkpoint["metadata"]) == 5 and all(v is True for v in checkpoint["metadata"].values())
    snapshot = meta.private.private_path(BASE / "radio-failure21a01.saved-private.bin").read_bytes()
    assert integer(failed["saved_snapshot_end_rx_bytes"]) == integer(pre[0]["rx_bytes"]) + len(snapshot)
    assert trial.receipt.DONE.encode() in snapshot and b"qc07f> " in snapshot
    assert b"RADIO BOOT STATE QUERY TX RESULT" not in snapshot and b"RADIO READONLY FWINFO TX RESULT" not in snapshot
    assert boot["rx_bytes"] <= pre[0]["rx_bytes"] <= failed["saved_snapshot_end_rx_bytes"] <= checkpoint["rx_bytes"]
    return post, boot, failed, checkpoint, runtime, snapshot, expected


class Guard(retire.Guard):
    """Exact next wire/phase ledger installed at inherited Boot RPC boundaries."""
    def __init__(self, session, held, content, peer, start, original_rx, retired_rx,
                 deadline, expected_tx, program, lock_path, lock_identity, ids):
        self.expected_tx, self.program, self.index = expected_tx, program, 0
        self.pending, self.frozen = None, False
        self.pre_valid = self.application = self.identity = False
        self.last_rx = retired_rx
        self.last_state = None
        self.lock_path, self.lock_identity, self.ids = lock_path, lock_identity, ids
        super().__init__(session, held, content, peer, start, original_rx, retired_rx, deadline)

    def local_gate(self, reserve=0):
        assert not self.frozen
        super().local_gate(reserve)
        assert (os.getuid(), os.geteuid(), os.getgid(), os.getegid()) == self.ids
        hold_gate(self.held, self.lock_path, self.lock_identity, self.hold_bytes)
        assert self.session.capture_deadline - time.monotonic() >= 300

    def next_gate(self):
        assert self.index < len(self.program)
        phase, wire = self.program[self.index]
        if phase in ("state", "info"):
            assert self.last_state is not None and self.last_state["recent_rx"].endswith("qc07f> ")
        if phase == "state":
            assert self.pre_valid and self.deadline - time.monotonic() >= 140
        elif phase == "info":
            assert self.application and self.deadline - time.monotonic() >= 130
        elif phase == "post":
            assert self.identity
        else:
            assert phase == "pre"
        return wire

    def validate(self, state):
        self.local_gate()
        assert state["part"] == meta.PART and state["abort_armed"] is True and state["tx_busy"] is False
        assert integer(state["tx_bytes"]) == self.expected_tx
        assert integer(state["rx_bytes"]) >= self.last_rx
        whole = self.session.raw_since(self.original_rx, state)
        assert not trial.HCD_DIAGNOSTIC.search(whole) and not retire.NEW_KERNEL.search(whole)
        assert not ACTIVE_ENDPOINT.search(whole)
        assert not retire.TRANSITION.search(self.topology_carry + self.session.raw_since(self.settled_rx, state))
        self.last_rx = state["rx_bytes"]
        self.last_state = state
        return state

    def check_stream(self):
        # Restore inherited cursors if a rejected interval would advance them.
        saved = {k: getattr(self.session, k) for k in ("epoch_cursor", "epoch_tail") if hasattr(self.session, k)}
        try:
            return self.validate(super().check_stream())
        except BaseException:
            for key, value in saved.items(): setattr(self.session, key, value)
            self.frozen = True
            raise

    def call(self, request, expected_peer=None):
        try:
            action = request["action"]
            assert action in ("status", "send")
            if action == "send":
                assert expected_peer == self.original_peer
                assert self.pending == self.next_gate() == bytes.fromhex(request["hex"])
                assert integer(request["expected_rx_bytes"]) == self.last_rx
                self.local_gate(10)
            value, peer = super().call(request, expected_peer=expected_peer)
            assert peer == self.original_peer
            if action == "send":
                if value.get("not_sent") is True:
                    zero = value.get("transmitted_bytes", 0)
                    assert type(zero) is int and zero == 0
                else:
                    assert type(value.get("transmitted_bytes")) is int and value["transmitted_bytes"] == len(self.pending)
                    self.expected_tx += len(self.pending)
                    self.index += 1
                    self.pending = None
                self.local_gate()
            else:
                self.validate(value)
            return value, peer
        except BaseException:
            self.frozen = True
            raise

    def send(self, wire, reason):
        try:
            assert self.pending is None and wire == self.next_gate()
            index = self.index; self.pending = wire
            super().send(wire, reason)
            assert self.pending is None and self.index == index + 1
            self.check_stream()
        except BaseException:
            self.frozen = True
            raise

    def owner(self, margin):
        self.local_gate()
        state = self.session.owner(margin=margin)
        self.local_gate()  # owner() is allowed to repin internally; we are not.
        assert self.session.peer == self.original_peer
        return self.validate(state)


def monitor(guard, post, command):
    assert guard.check_stream()["recent_rx"].endswith("qc07f> ")
    answer, _ = meta.fixed(guard, command.encode() + b"\r", guard.expected_tx, meta.MONITOR, 6)
    return post.body(answer, command)


def cached_round(guard, nonce):
    assert guard.check_stream()["recent_rx"].endswith("qc07f> ")
    answer, _ = meta.fixed(guard, b"shell\r", guard.expected_tx, b"# ", 6)
    assert re.search(rb"(?:^|\n)# $", answer.replace(b"\r", b""))
    command = meta.script(nonce).encode()
    start = guard.check_stream()["rx_bytes"]
    for chunk in cached_wires(nonce)[1:-2]:
        guard.send(chunk, "Fixed cached metadata echo")
        if len(chunk) == 64: time.sleep(.025)
    echo_end = min(guard.deadline, time.monotonic() + 3)
    while True:
        state = guard.check_stream()
        echo = re.sub(rb"\r+\n", b"", guard.raw_since(start, state))
        if echo == command: break
        assert command.startswith(echo) and time.monotonic() < echo_end
        time.sleep(.01)
    answer, _ = meta.fixed(guard, b"\r", guard.expected_tx, b"# ", 10)
    text = answer.decode("ascii").replace("\r", "")
    assert re.search(r"(?:^|\n)# $", text)
    assert text.splitlines().count("K76MD_BEGIN_" + nonce) == 1
    assert len(re.findall(r"(?m)^K76MD_DONE_" + nonce + r":\d+$", text)) == 1
    # Sole allowed post-refusal TX exception: fully framed executed cached
    # command exits before bad descriptor/FD values are parsed. Never on timeout.
    answer, _ = meta.fixed(guard, b"exit\r", guard.expected_tx, meta.MONITOR, 6)
    assert b"RAM-only shell returned to monitor" in answer
    assert answer.endswith(meta.MONITOR) and answer.count(meta.MONITOR) == 1
    return meta.parse(text, nonce)


def idle_round(guard, post, runtime):
    rows = {}
    for key, command in list(post.QUERIES.items()) + [("boot_id_after", post.QUERIES["boot_id"])]:
        rows[key] = monitor(guard, post, command)
    post.parse(rows)
    assert rows["boot_id"].strip() == runtime["boot_id"].strip()
    return rows


def main():
    assert sys.argv[1:] == [ARG]
    ids = os.getuid(), os.geteuid(), os.getgid(), os.getegid()
    assert ids[0] == ids[1] > 0 and ids[2] == ids[3] > 0
    os.umask(0o077)
    lock = b.CONTROL / "peripheral-gui-operation.lock"
    before = file_identity(lock.lstat())
    fd = os.open(lock, os.O_RDONLY | os.O_NOFOLLOW | os.O_CLOEXEC | os.O_NONBLOCK)
    with os.fdopen(fd, "rb") as held:
        fcntl.flock(held, fcntl.LOCK_EX | fcntl.LOCK_NB)
        content = held.read()
        assert json.loads(content) == retire.OLD_HOLD
        hold_gate(held, lock, before, content)
        # Consume the one fixed attempt BEFORE handoff/admission; refusal is
        # sticky even if the historical checkpoint is absent or malformed.
        output = meta.private.private_path(BASE / OUTPUT, directory=True, fresh=True)
        output.mkdir(mode=0o700, exist_ok=False)
        session = None
        try:
            assert not os.path.lexists(EVIDENCE / PUBLIC)
            post, boot, failed, cp, runtime, snapshot, expected = handoff()
            assert cp["original_HOLD"].encode() == content
            # PrivateBoot requires a fresh directory. Its fresh child is unique
            # within our already-consumed attempt, not a retry suffix.
            session = meta.private.PrivateBoot(output / "transport", BASE / "uart-capture21a")
            deadline = time.monotonic() + 300  # BEFORE the first live owner call
            current = session.owner(margin=600)
            assert session.capture_deadline - time.monotonic() >= 600
            peer = tuple(cp["peer"])
            assert peer == (boot["recorder_pid"], ids[0], ids[2])
            assert session.peer == peer and b.ticks(peer[0]) == cp["start_ticks"]
            assert current["tx_bytes"] == expected and current["rx_bytes"] >= cp["rx_bytes"]
            assert session.raw_since(failed["original_preflight_rx_bytes"], dict(current, rx_bytes=failed["saved_snapshot_end_rx_bytes"])) == snapshot
            session.epoch_cursor = boot["rx_bytes"]
            nonces = uuid.uuid4().hex, uuid.uuid4().hex
            # Refresh only a returned OWN monitor with a known empty command
            # buffer, witnessed by saved retirement and zero intervening TX.
            refresh = not current["recent_rx"].endswith("qc07f> ")
            assert "qc07f> " in current["recent_rx"]
            pre = ([b"\r"] if refresh else []) + cached_wires(nonces[0]) + inventory_wires(post)
            program = [("pre", w) for w in pre] + [("state", b"radio state\r"), ("info", b"radio info\r")]
            program += [("post", w) for w in cached_wires(nonces[1]) + inventory_wires(post)]
            guard = Guard(session, held, content, peer, cp["start_ticks"], boot["rx_bytes"], cp["rx_bytes"],
                          deadline, expected, program, lock, before, ids)
            guard.check_stream()
            if refresh:
                answer, _ = meta.fixed(guard, b"\r", expected, meta.MONITOR, 5)
                assert answer.replace(b"\r", b"") == b"\nqc07f> "
            cached_round(guard, nonces[0]); idle_round(guard, post, runtime)
            guard.pre_valid = True
            guard.owner(margin=300)
            state_text = monitor(guard, post, "radio state")
            assert trial.receipt.parse_application_state(state_text) is True
            guard.application = True
            guard.check_stream()
            info_text = monitor(guard, post, "radio info")
            identity = trial.receipt.parse_firmware_info(info_text)
            session.log("separate_identity_raw_private", state=state_text, info=info_text)
            guard.identity = True
            cached_round(guard, nonces[1]); rows = idle_round(guard, post, runtime)
            guard.owner(margin=300)
            final = guard.check_stream()
            assert final["recent_rx"].endswith("qc07f> ")
            assert guard.index == len(program) == 54 + int(refresh)
            assert guard.expected_tx == expected + 1741 + int(refresh)
            with (output / "checkpoint.private.json").open("x") as stream:
                json.dump({"rows": rows, "tx_bytes": guard.expected_tx, "rx_bytes": final["rx_bytes"],
                           "peer": peer, "start_ticks": cp["start_ticks"], "original_HOLD": cp["original_HOLD"]}, stream)
            guard.check_stream()
            # No UUID/counters/nonces/raw/fwinfo/error text in the public result.
            record = {"state": "SEPARATE_IDENTITY_RETIRED_ORIGINAL_TRIAL_FAILED_HOLD",
                      "strict_APPLICATION": True, **identity,
                      "same_original_boot_idle_retirement": True,
                      "original_trial_FAILED_HOLD_preserved": True,
                      "direct_MODE_NAK_toggle_MTU1500_or_product_acceptance": False}
            with (EVIDENCE / PUBLIC).open("x") as stream:
                json.dump(record, stream, indent=2); stream.write("\n")
            hold_gate(held, lock, before, content)
            session.log("separate_identity_complete_original_HOLD_preserved", tx_bytes=guard.expected_tx, rx_bytes=final["rx_bytes"])
            print("Separate identity retired; ORIGINAL TRIAL FAILED/HOLD unchanged")
        except BaseException:
            # No target cleanup, no clear/truncate/chmod, no exception repr on stdout.
            import traceback
            with (output / "error.private.txt").open("x") as stream:
                traceback.print_exc(file=stream)
            hold_gate(held, lock, before, content)
            raise


if __name__ == "__main__":
    try:
        main()
    except BaseException:
        print("Separate identity HOLD; no retry or repair", file=sys.stderr)
        raise SystemExit(1)
