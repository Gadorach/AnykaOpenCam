#!/usr/bin/env python3
"""Independent HOST review: frozen public sources, fake capabilities only.

Never calls a native main, live status/RPC, private handoff, build or ARM tool.
Actual controller main/handoff run only inside the inspected supplied fake tests.
Refuses overwriting this review's evidence or invoking prior evidence collectors.
"""
from pathlib import Path
import datetime
import hashlib
import io
import json
import socket
import subprocess
import sys
import types
import unittest
from unittest.mock import patch

if not __debug__:
    raise SystemExit("Assertions required")
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
OUT = HERE / "result.json"
LOG = HERE / "supplied-tests.log"
assert not OUT.exists() and not LOG.exists(), "Preserve existing review evidence"
D = "docs/qualification-plans/kernel76-transport-20261021/"
prior = json.loads((ROOT / (D + "identity-parent-host01/result.json")).read_text())
names = list(prior["input_sha256"]) + [D + n for n in (
    "identity-parent-host01/result.json", "identity-parent-host01/initial01.json",
    "identity-parent-host01/initial01.txt", "IDENTITY_CONTROLLER_CANDIDATE.md",
    "SEPARATE_IDENTITY_ENTRY_REVIEW.md", "identity-entry-review/result.json")]
pins = {name: hashlib.sha256((ROOT / name).read_bytes()).hexdigest() for name in names}
assert all(pins[name] == value for name, value in prior["input_sha256"].items())
assert pins["apps/qualification/kernel76-transport/identity21a.py"] == "c3c19a12bffb56eec2044e259793f58a7243e1ae20ac6fe8cf75ee3f3229ceeb"
assert pins["apps/qualification/kernel76-transport/test_identity21a.py"] == "8393989ac89f75100521bfca9fd55932122f7c076b2f56f367a6b0da913ef04b"
sys.path.insert(0, str(ROOT / "apps/qualification/kernel76-transport"))
# All six imports and their public-only definition-time source read were inspected.
import identity21a as i
import test_identity21a as supplied


def forbidden(*args, **kwargs):
    raise AssertionError("Unfaked external/private capability in HOST review")


checks = []
probes = []


def check(name, value):
    assert value, name
    checks.append(name)


def protected_probe(fault=None, zero=False):
    """Real Boot.send/status/check_stream/wait plus real protected_call translation.

    Only the last private_rpc capability is fake. No private envelope or process
    query is evaluated; fixture replacement raw_since/ticks/clock/HOLD are memory.
    """
    requests = []
    with supplied.guard_fixture() as (guard, session, _, raw, clock, tx, held):
        sent = [False]
        def inject():
            if fault == "deadline": clock[0] = 291
            elif fault == "coverage": session.capture_deadline = clock[0] + 299
            elif fault == "HOLD": held.seek(0); held.truncate(); held.write(b"EXTERNAL")
            elif fault == "TX": tx[0] += 1
            elif fault == "RX": raw.clear()
            elif fault == "activeEP": raw.extend(supplied.WARNING)
            elif fault == "HCD": raw.extend(b"musb-hdrc musb-hdrc.0.auto: Babble\n")
            elif fault == "topology": raw.extend(b"usb 1-1: USB disconnect, device number 4\n")
            elif fault == "kernel": raw.extend(b"Linux version unexpected\n")
            elif fault == "reset": raw.extend(b"U-Boot 2013.10\n")
        def rpc(request, expected_peer=None):
            requests.append(dict(request))
            if request["action"] == "status":
                assert request["private_status"] is True
                changed = fault is not None and (sent[0] or not zero)
                if changed: inject()
                state = {"part": i.meta.PART, "tx_bytes": tx[0], "rx_bytes": len(raw),
                         "recent_rx": raw.decode(), "tx_busy": changed and fault == "busy",
                         "abort_armed": not (changed and fault == "catch"), "conditional_send_v1": True}
                peer = (456, 1000, 1000) if changed and fault == "peer" else session.peer
                return state, peer
            assert request["action"] == "send_protected"
            assert request["expected_rx_bytes"] == len(raw) and expected_peer == session.peer
            assert request["reason"] == "Protected ROOT RAM transfer"
            if zero and not sent[0]:
                sent[0] = True
                return {"not_sent": True}, session.peer
            wire = bytes.fromhex(request["hex"])
            tx[0] += len(wire); sent[0] = True
            return {"transmitted_bytes": len(wire)}, session.peer
        session.private_rpc = rpc
        guard.original_call = types.MethodType(i.meta.private.PrivateBoot.protected_call, session)
        error = None
        try:
            guard.send(b"shell\r", "fixed public reason not a payload")
        except (AssertionError, i.b.Refused) as exc:
            error = type(exc).__name__
        sends = [r for r in requests if r["action"] == "send_protected"]
        if fault is None:
            check("protected_healthy_completed_" + str(zero), error is None and guard.expected_tx == 106 and guard.index == 1)
            check("protected_healthy_send_count_" + str(zero), len(sends) == 1 + int(zero))
            offset = len(raw); raw.extend(b"\n# ")
            check("protected_real_wait_receive_" + str(zero), guard.wait(b"# ", offset, 6) == b"\n# ")
        else:
            check(f"protected_fault_blocks_{fault}_{zero}", error is not None and guard.frozen and guard.expected_tx == 100 and guard.index == 0)
            check(f"protected_fault_no_later_send_{fault}_{zero}", len(sends) == int(zero))
        probes.append({"fault": fault, "after_one_zero_refusal": zero, "error": error,
                       "protected_send_requests": len(sends), "completed_program_slots": guard.index})


stream = io.StringIO()
# Defence in depth: a test forgetting its own fake cannot access these facilities.
with patch.object(socket, "socket", side_effect=forbidden), \
     patch.object(subprocess, "run", side_effect=forbidden), \
     patch.object(i.meta.private, "private_path", side_effect=forbidden), \
     patch.object(i.b, "ticks", side_effect=forbidden):
    suite = unittest.defaultTestLoader.loadTestsFromNames([
        "test_radio_receipt", "test_metadata21a", "test_trial21a", "test_retire21a", "test_identity21a"])
    result = unittest.TextTestRunner(stream=stream, verbosity=2).run(suite)
    assert result.wasSuccessful(), stream.getvalue()
    check("supplied_48_methods_reproduced", result.testsRun == 48)
    for zero in (False, True):
        protected_probe(zero=zero)
        for fault in ("deadline", "coverage", "HOLD", "TX", "RX", "activeEP", "HCD", "topology", "kernel", "reset", "busy", "catch", "peer"):
            protected_probe(fault, zero)
    # Independent cumulative lineage checks do not use wall-clock timestamps.
    args = supplied.history()
    for n, event in enumerate(args[-1]): event["utc"] = "reversed-" + str(-n)
    check("global_lineage_ignores_timestamp_order", i.lineage(*args) == args[5]["tx_bytes"])
    args = supplied.history(); args[-1][-2]["bytes"] = args[-1][-1]["bytes"] = 0
    try: i.lineage(*args)
    except AssertionError: check("global_lineage_zero_length_frame_refuses", True)
    else: raise AssertionError("zero-length frame accepted")
    # No need to trust the candidate manifest as an independent-review token.
    check("source_manifest_contains_candidate_and_all_frozen_helpers", all(name in prior["input_sha256"] for name in (
        "apps/qualification/kernel76-transport/identity21a.py", "apps/qualification/kernel76-transport/retire21a.py",
        "scripts/qc07f_boot.py", "scripts/qc07f_private_root_transfer.py")))

assert all(hashlib.sha256((ROOT / name).read_bytes()).hexdigest() == pin for name, pin in pins.items())
record = {"utc": datetime.datetime.now(datetime.timezone.utc).isoformat(), "python": sys.version,
          "argv": ["python3", "-B", str(Path(__file__).relative_to(ROOT))],
          "scope": "Independent SOURCE/HOST; inspected suite and in-memory real Boot/protected_call composition",
          "script_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(), "input_sha256": pins,
          "supplied_test_methods": result.testsRun, "supplied_failures": len(result.failures), "supplied_errors": len(result.errors),
          "independent_checks_passed": len(checks), "independent_checks": checks, "protected_composition_probes": probes,
          "all_inputs_rehash_unchanged": True, "private_target_RPC_status_network_build_ARM_access": False,
          "source_review_not_live_admission_or_queue": True}
with LOG.open("x") as f: f.write(stream.getvalue())
with OUT.open("x") as f: json.dump(record, f, indent=2); f.write("\n")
print(f"{result.testsRun} supplied methods + {len(checks)} independent checks PASS; no live/private capability used")
