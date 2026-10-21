#!/usr/bin/env python3
"""Run only the five inspected HOST unittest files; exclusive safe evidence.

No private input, target, build, ARM execution, or prior output collector.
An initial failing run remains verbatim; use a fresh --label after corrections.
"""
from pathlib import Path
import datetime
import hashlib
import io
import json
import sys
import unittest

ROOT = Path(__file__).resolve().parents[4]
HERE = Path(__file__).resolve().parent
APPS = ROOT / "apps/qualification/kernel76-transport"


def main():
    assert len(sys.argv) == 2 and sys.argv[1] in ("initial01", "corrected02", "final03", "final04", "final05")
    label = sys.argv[1]
    sys.path.insert(0, str(APPS))
    modules = ["test_radio_receipt", "test_metadata21a", "test_trial21a", "test_retire21a", "test_identity21a"]
    # Read only public source paths already enumerated by the independent audit.
    old = json.loads((HERE.parent / "identity-entry-review/result.json").read_text())
    names = list(old["input_sha256"])
    names += [str((APPS / (m + ".py")).relative_to(ROOT)) for m in modules]
    names += ["apps/qualification/kernel76-transport/identity21a.py", str(Path(__file__).resolve().relative_to(ROOT))]
    names = sorted(set(names))
    pins = {name: hashlib.sha256((ROOT / name).read_bytes()).hexdigest() for name in names}
    unchanged = all(pins[name] == pin for name, pin in old["input_sha256"].items())
    assert unchanged, "Reviewed public inputs changed; stop HOST run"
    stream = io.StringIO()
    suite = unittest.defaultTestLoader.loadTestsFromNames(modules)
    result = unittest.TextTestRunner(stream=stream, verbosity=2).run(suite)
    record = {"utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
              "scope": "SOURCE/HOST ONLY; in-memory FS/transport; no native controller invocation",
              "argv": ["python3", "-B", str(Path(__file__).resolve().relative_to(ROOT)), label],
              "python": sys.version, "input_sha256": pins,
              "reviewed_public_inputs_unchanged": unchanged,
              "test_methods": result.testsRun, "failures": len(result.failures), "errors": len(result.errors),
              "passed": result.wasSuccessful(), "private_inputs_accessed": False,
              "target_RPC_status_serial_network_build_or_ARM_execution": False,
              "independent_review_or_native_GO": False}
    with (HERE / (label + ".txt")).open("x") as f: f.write(stream.getvalue())
    with (HERE / (label + ".json")).open("x") as f: json.dump(record, f, indent=2); f.write("\n")
    if result.wasSuccessful() and label.startswith("final"):
        with (HERE / "result.json").open("x") as f: json.dump(record, f, indent=2); f.write("\n")
    print(json.dumps({k: record[k] for k in ("test_methods", "failures", "errors", "passed")}))
    return 0 if result.wasSuccessful() else 1


if __name__ == "__main__": raise SystemExit(main())
