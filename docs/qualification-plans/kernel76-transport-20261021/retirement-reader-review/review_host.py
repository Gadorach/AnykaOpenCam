#!/usr/bin/env python3
"""HOST ONLY: supplied mocked tests and actual Boot.send/Guard in-memory probes.

No private inputs, transport construction, RPC, target, build or ARM execution.
Outputs are exclusively created; preserve them instead of rerunning in place.
"""
import ast
import datetime
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
for name in ("result.json", "supplied-tests.log"):
    if (HERE / name).exists():
        raise SystemExit("Preserve existing evidence; no rerun")
names = [str(p.relative_to(ROOT)) for p in sorted(PKG.glob("*.py"))] + [
    "scripts/qc07f_boot.py", "scripts/qc07f_private_root_transfer.py",
    "work/lightbulb-camera/implementation-20261008T205404Z/uart_capture.py",
    "evidence/qualification/qc07f/execution-20261021/ram76-resume21a/postcheck.py",
    "evidence/qualification/qc07f/execution-20261021/ram76-resume21a/SOURCE_DERIVATION21A01.json",
    "evidence/qualification/qc07f/execution-20261021/ram76-resume21a/RADIO_NATIVE76_21A01_HOLD.json",
    "docs/qualification-plans/kernel76-transport-20261021/failed-followup-review/result.json"]
sources = {name: (ROOT / name).read_bytes() for name in names}
pins = {name: hashlib.sha256(value).hexdigest() for name, value in sources.items()}
sys.path.insert(0, str(PKG))
import retire21a as r

# Importing the reviewed modules defines helpers; it does not construct transport.
# Capability sentinels also catch accidental escapes in the mocked test bodies.
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


def send_probe(fault):
    """Actual Guard.send -> inherited Boot.send, never a socket or real file."""
    clock = [0.0]
    held = io.StringIO(json.dumps(r.OLD_HOLD) + "\n")
    old = held.getvalue()

    class Session(r.b.Boot):
        def __init__(self):
            # Deliberately do not call Boot.__init__: no paths/transport exist.
            self.peer = (123, 1000, 1000)
            self.transfer_cursor = None
            self.qcl_ready = False
            self.raw = b""
            self.catch = True
            self.checks = 0
            self.tx = []

        def check_stream(self):
            self.checks += 1
            if self.checks == 2:
                if fault == "deadline":
                    clock[0] = 151.0
                elif fault == "catch_consumed":
                    self.catch = False
                elif fault == "HCD_diagnostic":
                    self.raw = b"musb-hdrc musb-hdrc.0.auto: Babble\n"
                elif fault == "HOLD_changed":
                    held.seek(0); held.write("FOREIGN"); held.truncate()
            return {"part": r.meta.PART, "tx_bytes": sum(len(w) for w in self.tx),
                    "rx_bytes": len(self.raw), "tx_busy": False,
                    "abort_armed": self.catch, "conditional_send_v1": True,
                    "recent_rx": "qc07f> "}

        def raw_since(self, start, state):
            return self.raw[start:state["rx_bytes"]]

        def call(self, request, expected_peer=None):
            assert request["action"] == "send" and expected_peer == self.peer
            wire = bytes.fromhex(request["hex"])
            self.tx.append(wire)
            return {"transmitted_bytes": len(wire)}, self.peer

    session = Session()
    with patch.object(r.time, "monotonic", side_effect=lambda: clock[0]), \
         patch.object(r.b, "ticks", return_value=42):
        guard = r.Guard(session, held, old, session.peer, 42, 0, 0, 150.0)
        guard.send(b"\r", "PUBLIC HOST fixture only")
        late_refusal = False
        try:
            guard.check_stream()
        except AssertionError:
            late_refusal = True
    assert len(session.tx) == 1 and late_refusal
    return {"fault": fault, "actual_Guard_send_and_Boot_send_used": True,
            "simulated_wire_bytes_sent_after_gate_changed": len(session.tx[0]),
            "simulated_send_time_s": clock[0], "subsequent_Guard_check_refuses": late_refusal,
            "external_IO": False}


probes = [send_probe(fault) for fault in ("deadline", "catch_consumed", "HCD_diagnostic", "HOLD_changed")]

# Separate boundary-tail limitation: a topology line begins before settled_rx.
prefix = b"usb 1-1: USB discon"
raw = [prefix]
peer = (123, 1000, 1000)

class TopologySession:
    def __init__(self): self.peer = peer
    def check_stream(self):
        return {"abort_armed": True, "tx_busy": False, "rx_bytes": len(raw[0])}
    def raw_since(self, start, state): return raw[0][start:state["rx_bytes"]]

held = io.StringIO("PUBLIC HOST HOLD")
with patch.object(r.time, "monotonic", return_value=0), patch.object(r.b, "ticks", return_value=42):
    guard = r.Guard(TopologySession(), held, held.getvalue(), peer, 42, 0, len(prefix), 150)
    guard.check_stream()
    raw[0] += b"nect, device number 3\n"
    guard.check_stream()
assert r.TRANSITION.search(raw[0]) and not r.TRANSITION.search(raw[0][len(prefix):])

recorder = "work/lightbulb-camera/implementation-20261008T205404Z/uart_capture.py"
assert pins[recorder] == r.b.RECORDER_SHA
for name, expected in r.SOURCE_PINS.items():
    assert pins["apps/qualification/kernel76-transport/" + name] == expected
tree = ast.parse(sources[recorder].decode())
credential_or_exec = []
for node in ast.walk(tree):
    if isinstance(node, ast.Call):
        name = ast.unparse(node.func)
        if name == "exec" or name.startswith(("os.setuid", "os.setgid", "os.setres", "os.setre", "os.sete", "os.exec", "os.system", "subprocess.")):
            credential_or_exec.append(name)
assert not credential_or_exec
launcher = ast.parse(sources["scripts/qc07f_boot.py"].decode())
starts = [n for n in launcher.body if isinstance(n, ast.FunctionDef) and n.name == "start_receiver"]
assert len(starts) == 1
popens = [n for n in ast.walk(starts[0]) if isinstance(n, ast.Call) and ast.unparse(n.func) == "subprocess.Popen"]
assert len(popens) == 1
kwargs = {k.arg for k in popens[0].keywords}
assert not kwargs.intersection({"user", "group", "extra_groups", "preexec_fn", "shell"})
assert all((ROOT / name).read_bytes() == value for name, value in sources.items())
result = {"utc": datetime.datetime.now(datetime.timezone.utc).isoformat(), "python": sys.version,
          "argv": ["python3", "-B", str(Path(__file__).relative_to(ROOT))],
          "scope": "HOST-only mocked supplied suite + in-memory actual Guard/Boot.send probes; no private/target capability",
          "input_sha256": pins, "script_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
          "supplied_test_methods": test_result.testsRun, "supplied_suite_pass": True,
          "send_gate_bypass_probes": probes,
          "topology_boundary_tail_probe": {"full_line_matches": True, "guard_accepts_crossing_settled_cursor": True, "target_IO": False},
          "identity_source_checks": {"recorder_matches_135a694b_pin": True,
                                     "recorder_credential_or_exec_calls": credential_or_exec,
                                     "launcher_Popen_keyword_names": sorted(kwargs),
                                     "launcher_sets_user_group_or_preexec": False,
                                     "historical_UID_GID_not_read_or_invented": True},
          "all_reviewed_inputs_unchanged": True, "verdict": "NO_GO_guard_send_internal_checks_bypass_frozen_gates",
          "target_IO": False, "private_input_access": False, "original_trial_still_failed_HOLD": True}
with (HERE / "supplied-tests.log").open("x") as f:
    f.write(log.getvalue())
with (HERE / "result.json").open("x") as f:
    json.dump(result, f, indent=2); f.write("\n")
print(json.dumps({"supplied_methods_PASS": test_result.testsRun, "send_gate_bypass_probes": probes,
                  "topology_boundary_tail_accepted": True, "verdict": result["verdict"]}))
