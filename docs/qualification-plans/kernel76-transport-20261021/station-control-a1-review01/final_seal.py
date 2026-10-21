#!/usr/bin/env python3
"""Exclusive review-owned final pin check; public source/HOST files only."""
import datetime
import hashlib
import json
from pathlib import Path
import sys

if sys.flags.optimize or sys.argv[1:]:
    raise SystemExit("no optimized Python or arguments")
here = Path(__file__).resolve().parent
root = here.parents[3]
out = here / "review-seal.json"
if out.exists():
    raise SystemExit("exclusive output already exists")


def pin(path):
    data = path.read_bytes()
    return {"bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}


result = json.loads((here / "run01/result.json").read_text())
assert result["all_pass"] and len(result["runtime_mutants"]) == 16
assert all(x["compiled"] and x["runtime_exit"] for x in result["runtime_mutants"])
for rel, expected in result["inputs"].items():
    assert pin(root / rel) == expected, rel
for rel, expected in json.loads((here / "run01/output-hashes.json").read_text()).items():
    assert pin(here / "run01" / rel) == expected, rel
for name in ("candidate-sanitized-test", "supplement-sanitized-test"):
    assert (here / "run01" / (name + ".stderr")).read_bytes() == b""
paths = sorted(p for p in here.rglob("*") if p.is_file())
paths += [here.parent / name for name in
          ("STATION76_CONTROL_A1_REVIEW.md", "STATION76_CONTROL_A1_CANDIDATE.md", "STATION76_ENTRY_REVIEW.md")]
receipt = {"utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
           "argv": sys.argv, "python": sys.version,
           "scope": "A1_LOCAL_COPY_FWINFO_SOURCE_HOST_REVIEW_GO_ONLY_FULL_S4_ATTACH_BUILD_NATIVE_NO_GO",
           "all_run01_inputs_and_outputs_rechecked": True,
           "files": {str(p.relative_to(root)): pin(p) for p in paths}}
with out.open("x") as f:
    json.dump(receipt, f, indent=2)
    f.write("\n")
print("Independent A1 review sealed; source/original/HOST pins unchanged")
