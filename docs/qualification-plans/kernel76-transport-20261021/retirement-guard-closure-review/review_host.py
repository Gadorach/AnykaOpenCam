#!/usr/bin/env python3
"""HOST-only guard closure: public sources, mocked suite, in-memory helpers.

No transport constructor, private input, target, build or ARM execution. Fresh
exclusive outputs only; preserve old collectors/results instead of rerunning.
"""
import ast
import copy
import datetime
import difflib
import hashlib
import io
import json
from pathlib import Path
import sys
import unittest
from unittest.mock import patch

if not __debug__:
    raise SystemExit("Assertions required")
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
PKG = ROOT / "apps/qualification/kernel76-transport"
DOC = "docs/qualification-plans/kernel76-transport-20261021/"
if any((HERE / name).exists() for name in ("result.json", "supplied-tests.log", "wrapper.diff")):
    raise SystemExit("Preserve existing outputs; no rerun")
names = [str(p.relative_to(ROOT)) for p in sorted(PKG.glob("*.py"))] + [
    "scripts/qc07f_boot.py", "scripts/qc07f_private_root_transfer.py",
    "work/lightbulb-camera/implementation-20261008T205404Z/uart_capture.py",
    "evidence/qualification/qc07f/execution-20261021/ram76-resume21a/postcheck.py",
    "evidence/qualification/qc07f/execution-20261021/ram76-resume21a/RADIO_NATIVE76_21A01_HOLD.json",
    DOC + "retirement-parent-correction01/before/retire21a.py",
    DOC + "retirement-parent-correction01/before/test_retire21a.py",
    DOC + "retirement-parent-correction01/initial-NO-GO-pins.json",
    DOC + "retirement-reader-review/result.json", DOC + "retirement-reader-review/review_host.py",
    DOC + "RETIREMENT_READER_REVIEW.md", DOC + "FAILED_TRIAL_FOLLOWUP_REVIEW.md"]
sources = {name: (ROOT / name).read_bytes() for name in names}
pins = {name: hashlib.sha256(value).hexdigest() for name, value in sources.items()}
old_result = json.loads(sources[DOC + "retirement-reader-review/result.json"])
for name in ("trial21a.py", "metadata21a.py", "radio_receipt.py"):
    key = "apps/qualification/kernel76-transport/" + name
    assert pins[key] == old_result["input_sha256"][key]
for name in ("scripts/qc07f_boot.py", "scripts/qc07f_private_root_transfer.py",
             "work/lightbulb-camera/implementation-20261008T205404Z/uart_capture.py"):
    assert pins[name] == old_result["input_sha256"][name]
for name in ("retire21a.py", "test_retire21a.py"):
    assert pins[DOC + "retirement-parent-correction01/before/" + name] == old_result["input_sha256"]["apps/qualification/kernel76-transport/" + name]

before_name = DOC + "retirement-parent-correction01/before/retire21a.py"
after_name = "apps/qualification/kernel76-transport/retire21a.py"
before = ast.parse(sources[before_name].decode())
after = ast.parse(sources[after_name].decode())
identity_check = ast.dump(ast.parse("assert os.getuid() == os.geteuid() and os.getgid() == os.getegid()").body[0])


def outside_changes(tree, remove_identity=False):
    tree = copy.deepcopy(tree)
    removed = 0
    for node in tree.body:
        if isinstance(node, ast.ClassDef) and node.name == "Guard":
            node.body = [ast.Pass()]
        if remove_identity and isinstance(node, ast.FunctionDef) and node.name == "main":
            kept = []
            for stmt in node.body:
                if ast.dump(stmt) == identity_check:
                    removed += 1
                else:
                    kept.append(stmt)
            node.body = kept
    assert removed == int(remove_identity)
    return ast.dump(tree)


assert outside_changes(before) == outside_changes(after, True)
diff = "".join(difflib.unified_diff(sources[before_name].decode().splitlines(True),
                                  sources[after_name].decode().splitlines(True),
                                  fromfile=before_name, tofile=after_name))
sys.path.insert(0, str(PKG))
import retire21a as r
import test_retire21a as supplied


def forbidden(*args, **kwargs):
    raise AssertionError("HOST-only review forbids external/private capability")


log = io.StringIO()
with patch("socket.socket", side_effect=forbidden), \
     patch.object(r.b.subprocess, "run", side_effect=forbidden), \
     patch.object(r.b.subprocess, "Popen", side_effect=forbidden), \
     patch.object(r.meta.private, "private_path", side_effect=forbidden):
    suite = unittest.defaultTestLoader.discover(str(PKG))
    test_result = unittest.TextTestRunner(stream=log, verbosity=2).run(suite)
assert test_result.wasSuccessful()
checks = []


def ok(name, condition=True):
    assert condition, name
    checks.append(name)


def refuses(name, fn, *args, allowed=(AssertionError,)):
    try:
        fn(*args)
    except allowed:
        checks.append(name)
    else:
        raise AssertionError("Missing refusal: " + name)


# Same four earlier reproducer state transitions, now with expected zero sends.
for fault in ("deadline", "catch", "HCD", "HOLD"):
    clock = [0.0]; held = io.StringIO(supplied.HOLD)
    class OldShape(r.b.Boot):
        def __init__(self):
            self.peer = (123, 1000, 1000); self.transfer_cursor = None
            self.raw = b""; self.catch = True; self.checks = 0; self.tx = []
        def check_stream(self):
            self.checks += 1
            if self.checks == 2:
                if fault == "deadline": clock[0] = 151
                if fault == "catch": self.catch = False
                if fault == "HCD": self.raw = b"musb-hdrc musb-hdrc.0.auto: Babble\n"
                if fault == "HOLD": held.seek(0); held.write("FOREIGN"); held.truncate()
            return {"abort_armed": self.catch, "tx_busy": False, "rx_bytes": len(self.raw), "conditional_send_v1": True}
        def raw_since(self, offset, state): return self.raw[offset:state["rx_bytes"]]
        def call(self, request, expected_peer=None):
            assert request["action"] == "send"
            self.tx.append(bytes.fromhex(request["hex"]))
            return {"transmitted_bytes": len(self.tx[-1])}, self.peer
    session = OldShape()
    with patch.object(r.time, "monotonic", side_effect=lambda: clock[0]), patch.object(r.b, "ticks", return_value=42):
        guard = r.Guard(session, held, supplied.HOLD, session.peer, 42, 0, 0, 150)
        refuses("old_G1_reproducer_now_refuses_" + fault, guard.send, b"\r", "public HOST only")
    ok("old_G1_reproducer_zero_bytes_" + fault, not session.tx)


def boot_fixture(fault=None, refusal=False, wait=False, always_refuse=False, partial=False):
    """Real Boot.status/check_stream/send/wait/received; only RPC/raw/clock fake."""
    clock = [0.0]; held = io.StringIO(supplied.HOLD); raw = bytearray(b"qc07f> ")
    calls = []; tx = []; statuses = [0]
    session = r.b.Boot.__new__(r.b.Boot)
    session.peer = (123, 1000, 1000); session.state = {"part": "PUBLIC_FIXTURE"}
    session.transfer_fault = session.transfer_cursor = session.epoch_cursor = None
    session.raw_since = lambda offset, state: bytes(raw[offset:state["rx_bytes"]])
    def rpc(request, expected_peer=None):
        assert expected_peer == session.peer
        if request["action"] == "status":
            statuses[0] += 1
            trigger = 3 if refusal else 2
            changed = fault is not None and statuses[0] >= trigger
            if changed:
                if fault == "deadline": clock[0] = 151
                if fault == "HCD": raw.extend(b"\nmusb-hdrc musb-hdrc.0.auto: Babble\n")
                if fault == "kernel": raw.extend(b"qc07f> Linux version another-boot\n")
                if fault == "HOLD": held.seek(0); held.write("FOREIGN"); held.truncate()
                if fault == "topology": raw.extend(b"usb 1-1: USB disconnect, device number 3\n")
            return {"part": "PUBLIC_FIXTURE", "abort_armed": not (changed and fault == "catch"),
                    "tx_busy": False, "rx_bytes": len(raw), "tx_bytes": sum(len(x) for x in tx),
                    "conditional_send_v1": True}, session.peer
        assert request["action"] == "send"
        calls.append(request)
        if always_refuse or refusal and len(calls) == 1: return {"not_sent": True}, session.peer
        wire = bytes.fromhex(request["hex"])
        tx.append(wire[:-1] if partial else wire)
        return {"transmitted_bytes": len(tx[-1])}, session.peer
    session.call = rpc
    with patch.object(r.time, "monotonic", side_effect=lambda: clock[0]), \
         patch.object(r.time, "sleep"), patch.object(r.b, "ticks", return_value=42):
        guard = r.Guard(session, held, supplied.HOLD, session.peer, 42, 0, len(raw), 150)
        error = None
        try:
            if wait: guard.wait(b"qc07f> ", 0, 6, reject_reset=True)
            else: guard.send(b"mem\r", "public HOST only")
        except (AssertionError, r.b.Refused) as exc:
            error = type(exc).__name__
    return error, len(calls), tx, statuses[0]


for refusal in (False, True):
    for fault in ("deadline", "catch", "HCD", "HOLD", "kernel", "topology"):
        error, sends, tx, statuses = boot_fixture(fault, refusal)
        ok(f"real_Boot_send_fault_{fault}_zero_refusal_{refusal}", error and sends == int(refusal) and not tx)
for fault in ("deadline", "catch", "HCD", "HOLD", "kernel", "topology"):
    error, sends, tx, statuses = boot_fixture(fault, wait=True)
    ok("real_Boot_wait_received_fault_" + fault, error and not sends and not tx and statuses >= 2)
error, sends, tx, _ = boot_fixture()
ok("healthy_real_Boot_send_still_works", error is None and sends == 1 and tx == [b"mem\r"])
error, sends, tx, _ = boot_fixture(wait=True)
ok("healthy_real_Boot_wait_still_works", error is None and not sends and not tx)
error, sends, tx, _ = boot_fixture(always_refuse=True)
ok("five_zero_byte_refusals_bounded_no_transmission", error == "Refused" and sends == 5 and not tx)
error, sends, tx, _ = boot_fixture(partial=True)
ok("partial_transmission_never_replayed", error == "Refused" and sends == 1 and tx == [b"mem"])

for size in (1024, 1025):
    raw = b"x" * size
    session = type("Memory", (), {})()
    session.peer = (123, 1000, 1000); session.call = lambda *a, **k: None
    original = lambda: {"abort_armed": True, "tx_busy": False, "rx_bytes": len(raw)}
    session.check_stream = original
    session.raw_since = lambda offset, state: raw[offset:state["rx_bytes"]]
    with patch.object(r.b, "ticks", return_value=42), patch.object(r.time, "monotonic", return_value=0):
        if size == 1025:
            refuses("oversize_carry_refuses_before_hooks", r.Guard, session, io.StringIO(supplied.HOLD), supplied.HOLD, session.peer, 42, 0, size, 150)
            ok("oversize_carry_leaves_session_unhooked", session.check_stream is original)
        else:
            guard = r.Guard(session, io.StringIO(supplied.HOLD), supplied.HOLD, session.peer, 42, 0, size, 150)
            guard.check_stream(); ok("maximum_1024_carry_accepted")

# Only the selected simulate function is copied in memory, changing its initial
# recent prompt constant to cover the otherwise missing no-refresh main branch.
tree = ast.parse(sources["apps/qualification/kernel76-transport/test_retire21a.py"].decode())
simulate = copy.deepcopy(next(n for n in tree.body if isinstance(n, ast.FunctionDef) and n.name == "simulate"))
changed = [0]
class InitialPrompt(ast.NodeTransformer):
    def visit_Constant(self, node):
        if node.value == "qc07f> late enumeration finished\n":
            changed[0] += 1; return ast.copy_location(ast.Constant("qc07f> "), node)
        return node
simulate = InitialPrompt().visit(simulate)
assert changed == [1]
ns = dict(supplied.__dict__)
exec(compile(ast.fix_missing_locations(ast.Module(body=[simulate], type_ignores=[])), "PUBLIC_IN_MEMORY_no_refresh", "exec"), ns)
with patch("socket.socket", side_effect=forbidden):
    answer = ns["simulate"]()
record = json.loads(answer["files"]["ev/RADIO_READONLY_RETIREMENT21A01.json"].getvalue())
ok("actual_main_no_refresh_healthy_HOLD_unchanged", answer["error"] is None and answer["HOLD"] == supplied.HOLD and not record["one_empty_monitor_refresh"])
ok("actual_main_no_refresh_no_extra_command", len(answer["calls"]) == 17 and "" not in answer["calls"])

# Real/effective mismatch refuses before any file/transport opening in main.
for uid, euid, gid, egid in ((1000, 0, 1000, 1000), (1000, 1000, 1000, 0)):
    with patch.object(r.sys, "argv", ["retire21a.py", "--one-readonly-retirement-preserve-HOLD21a"]), \
         patch.object(r.os, "getuid", return_value=uid), patch.object(r.os, "geteuid", return_value=euid), \
         patch.object(r.os, "getgid", return_value=gid), patch.object(r.os, "getegid", return_value=egid):
        refuses("effective_identity_mismatch_refuses_" + str(len(checks)), r.main)

ok("outside_Guard_only_real_effective_identity_assert_changed")
ok("source_pins_and_old_NO_GO_sources_preserved")
ok("all_inputs_unchanged", all((ROOT / name).read_bytes() == value for name, value in sources.items()))
result = {"utc": datetime.datetime.now(datetime.timezone.utc).isoformat(), "python": sys.version,
          "argv": ["python3", "-B", str(Path(__file__).relative_to(ROOT))],
          "scope": "HOST-only reviewed suite and in-memory real helper composition; no private/target access",
          "input_sha256": pins, "script_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
          "supplied_test_methods_PASS": test_result.testsRun, "independent_checks": checks,
          "independent_count": len(checks), "all_passed": True,
          "outside_Guard_AST_unchanged_except_identity_assert": True,
          "verdict": "CONDITIONAL_SOURCE_GO_one_preserved_HOLD_retirement_only_reader",
          "fresh_parent_hardware_admission_required": True, "native_retirement_observed": False,
          "original_trial_still_FAILED_HOLD": True, "target_IO": False, "private_input_access": False}
for name, value in (("supplied-tests.log", log.getvalue()), ("wrapper.diff", diff), ("result.json", json.dumps(result, indent=2) + "\n")):
    with (HERE / name).open("x") as stream: stream.write(value)
print(json.dumps({"supplied_methods_PASS": test_result.testsRun, "independent_checks_PASS": len(checks), "verdict": result["verdict"]}))
