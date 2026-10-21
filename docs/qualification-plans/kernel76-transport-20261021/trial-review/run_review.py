#!/usr/bin/env python3
"""Source-only review: admitted host tests and in-memory boundary injections.

No production source writes, private inputs, control sockets, device/network IO,
build recipes or ARM execution. The trial main is exercised only under the
already-reviewed unit harness's file/OS/transport fakes.
"""
import datetime
import hashlib
import inspect
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[4]
OUT = Path(__file__).resolve().parent
APP = ROOT / "apps/qualification/kernel76-transport"


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    started = datetime.datetime.now(datetime.timezone.utc).isoformat()
    sources = list(sorted(APP.glob("*.py"))) + [ROOT / p for p in (
        "scripts/qc07f_boot.py", "scripts/qc07f_private_root_transfer.py",
        "evidence/qualification/qc07f/execution-20261021/ram76-resume21a/postcheck.py",
        "evidence/qualification/qc07f/execution-20261021/ram76-resume21a/SOURCE_DERIVATION21A01.json")]
    linux = ROOT / "work/lightbulb-camera/foundation-20261011T041148Z/artifacts/native-usb76-source01/linux"
    sources += [linux / p for p in ("drivers/usb/musb/musb_debug.h", "drivers/usb/musb/musb_host.c",
                                    "drivers/usb/musb/musb_core.c")]
    before = {str(p.relative_to(ROOT)): sha(p.read_bytes()) for p in sources}
    prior = OUT.parent / "METADATA_RECEIPT_REVIEW.md"
    prior_pins = {str(prior.relative_to(ROOT)): sha(prior.read_bytes())}
    for p in sorted((OUT.parent / "metadata-review").iterdir()):
        if p.is_file(): prior_pins[str(p.relative_to(ROOT))] = sha(p.read_bytes())
    tests = []
    for name in ("test_metadata21a.py", "test_radio_receipt.py", "test_trial21a.py"):
        argv = [sys.executable, "-B", str(APP / name), "-v"]
        begin = datetime.datetime.now(datetime.timezone.utc).isoformat()
        run = subprocess.run(argv, cwd=ROOT, env=dict(os.environ, PYTHONDONTWRITEBYTECODE="1"),
                             capture_output=True, text=True, timeout=60)
        log = OUT / (name + ".log")
        with log.open("x") as f:
            f.write("argv: " + json.dumps(argv) + "\ncwd: " + str(ROOT) + "\nstart_utc: " + begin)
            f.write("\nstdout:\n" + run.stdout + "\nstderr:\n" + run.stderr + "\nexit: " + str(run.returncode) + "\n")
        tests.append({"argv": argv, "start_utc": begin, "exit": run.returncode,
                      "log": log.name, "log_sha256": sha(log.read_bytes())})
    assert all(row["exit"] == 0 for row in tests)

    sys.path.insert(0, str(APP))
    import test_trial21a as harness
    import radio_receipt as receipt
    import test_radio_receipt as samples
    # Patch only the host harness's Fake; actual trial/main/helpers are unchanged.
    original = inspect.getsource(harness.simulate)
    owner_old = "        def owner(self, **kwargs): return self.snapshot()"
    owner_new = """        def owner(self, **kwargs):
            if fault == 'changed_owner' and self.calls:
                self.peer = (456, 1000, 1000)
            return self.snapshot()"""
    exit_old = '            elif command == "exit": answer = b"\\r\\nRAM-only shell returned to monitor\\r\\nqc07f> "'
    exit_new = '''            elif command == "exit":
                answer = b"\\r\\nRAM-only shell returned to monitor\\r\\nqc07f> "
                if fault == 'missed_HCD_warning':
                    answer = b"\\r\\n[ 1.000000] musb_rx_reinit 565: rx1, packet/512 ready?\\r\\n" + answer
                if fault == 'recognized_HCD_warning':
                    answer = b"\\r\\n[ 1.000000] musb: warning synthetic\\r\\n" + answer'''
    assert original.count(owner_old) == original.count(exit_old) == 1
    instrumented = original.replace(owner_old, owner_new).replace(exit_old, exit_new)
    namespace = dict(vars(harness))
    exec(compile(instrumented, "<reviewed-in-memory-Fake-only-injections>", "exec"), namespace)
    simulate = namespace["simulate"]
    probes = {name: simulate(name) for name in ("changed_owner", "missed_HCD_warning", "recognized_HCD_warning")}
    for value in probes.values():
        value["no_256_fallback_or_reset"] = not any(c in value["calls"] for c in ("radio load", "wdt reset"))
        # Commands contain only synthetic nonces, fixed paths and public fixtures;
        # keep the evidence concise and avoid implying any real target transcript.
        value["radio_commands"] = [c for c in value.pop("calls") if c.startswith("radio ")]
    assert probes["changed_owner"]["PASS"] and probes["changed_owner"]["load_count"] == 1
    assert probes["missed_HCD_warning"]["PASS"] and probes["missed_HCD_warning"]["load_count"] == 1
    assert not probes["recognized_HCD_warning"]["PASS"]
    assert probes["recognized_HCD_warning"]["load_count"] == 1
    assert probes["recognized_HCD_warning"]["HOLD"]

    fixture = samples.loader()
    ack = samples.ack(2, 102)
    mutants = {
        "historical_shape_truncation_28_hex": fixture.replace(ack, ack[:-4], 1),
        "numeric_negative_length": fixture.replace("len0x00000010", "len0xffffffff", 1),
        "numeric_short_length": fixture.replace("len0x00000010", "len0x0000000f", 1),
        "numeric_wrong_status": fixture.replace("1a2b0700100064000000", "1a2b0700100064000001", 1),
        "missing_ACK": fixture.replace(ack + "\n", "", 1),
    }
    strict = {}
    for name, text in mutants.items():
        assert text != fixture
        try: receipt.parse_loader(text)
        except AssertionError: strict[name] = "REJECTED"
        else: raise AssertionError("unexpected accepted synthetic loader mutant")
    assert before == {str(p.relative_to(ROOT)): sha(p.read_bytes()) for p in sources}
    assert all(sha((ROOT / p).read_bytes()) == h for p, h in prior_pins.items())
    record = {
        "start_utc": started, "end_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "scope": "host-only; actual trial main under reviewed in-memory fakes; no native operation",
        "python": sys.version, "collector_sha256": sha(Path(__file__).read_bytes()),
        "source_sha256": before, "prior_report_and_evidence_unchanged": True,
        "prior_report_and_evidence_sha256": prior_pins,
        "supplied_tests": tests, "new_boundary_probes": probes, "strict_ACK_probes": strict,
        "known_bad_boundary_acceptance_observed_only_in_memory": True,
        "target_operation": False, "private_input_access": False,
    }
    with (OUT / "result.json").open("x") as f:
        json.dump(record, f, indent=2); f.write("\n")
    print("13 supplied host tests PASS; two additional in-memory acceptance gaps confirmed; one late-warning gate confirmed.")


if __name__ == "__main__":
    main()
