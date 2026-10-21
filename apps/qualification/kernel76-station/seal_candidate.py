#!/usr/bin/env python3
"""Exclusive public source/host handback inventory; no target or project imports."""
import sys
if sys.flags.optimize or sys.argv[1:]:
    raise SystemExit("no optimized Python or arguments")
import datetime
import hashlib
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
EVIDENCE = ROOT / "docs/qualification-plans/kernel76-transport-20261021/station-control-a1-host01"
REPORT = EVIDENCE.parent / "STATION76_CONTROL_A1_CANDIDATE.md"
OUT = EVIDENCE / "candidate-seal.json"
if OUT.exists():
    raise SystemExit("refuse existing handback seal")


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


run = json.loads((EVIDENCE / "run06/result.json").read_text())
if not run["all_pass"] or len(run["mutants"]) != 16 or not all(x["killed"] for x in run["mutants"]):
    raise SystemExit("final host receipt not PASS")
for rel, pin in run["inputs"].items():
    if sha(ROOT / rel) != pin["sha256"]:
        raise SystemExit("final input drift: " + rel)
for rel, pin in json.loads((EVIDENCE / "run06/output-hashes.json").read_text()).items():
    if sha(EVIDENCE / "run06" / rel) != pin["sha256"]:
        raise SystemExit("final host output drift: " + rel)
paths = sorted(p for p in HERE.rglob("*") if p.is_file())
paths += [REPORT, EVIDENCE / "copy-baseline.json", EVIDENCE / "run06/result.json",
          EVIDENCE / "run06/output-hashes.json"]
inventory = {str(p.relative_to(ROOT)): {"bytes": p.stat().st_size, "sha256": sha(p)} for p in paths}
receipt = {"utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
           "scope": "A1_PUBLIC_SOURCE_HOST_HANDBACK_ONLY_FULL_ATTACH_NO_GO",
           "argv": sys.argv, "python": sys.version, "script_sha256": sha(Path(__file__)),
           "final_run": "run06", "final_input_and_output_pins_rechecked": True,
           "original_pinned_sources": 67, "files": inventory}
with OUT.open("x") as f:
    json.dump(receipt, f, indent=2)
    f.write("\n")
print("Candidate source/host handback sealed; no build/use authorization")
