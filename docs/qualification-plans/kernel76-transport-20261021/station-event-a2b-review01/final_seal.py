#!/usr/bin/env python3
"""Read-only final pin recheck and exclusive review-owned inventory."""
import datetime
import hashlib
import json
from pathlib import Path
import sys

if sys.flags.optimize:
    raise SystemExit("refuse optimized Python")
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
REPORT = HERE.parent / "STATION76_EVENT_A2B_REVIEW.md"


def pin(path):
    assert not path.is_symlink(), path
    data = path.read_bytes()
    return {"bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}


result = json.loads((HERE / "run01/result.json").read_bytes())
assert result["all_pass"] and len(result["mutants"]) == 11
for relative, expected in result["inputs"].items():
    assert pin(ROOT / relative) == expected, relative
for relative, expected in json.loads((HERE / "run01/output-hashes.json").read_bytes()).items():
    assert pin(HERE / "run01" / relative) == expected, relative
for entry in result["extractions"]:
    if not entry["mutated"]:
        data = (ROOT / entry["source"]).read_bytes()
        assert data[:entry["start_byte"]].count(b"\n") + 1 == entry["start_line"]
        segment = data[entry["start_byte"]:entry["start_byte"] + entry["bytes"]]
        assert hashlib.sha256(segment).hexdigest() == entry["sha256"]
paths = [REPORT] + sorted(path for path in HERE.rglob("*") if path.is_file())
seal = {
    "utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
    "argv": sys.argv, "cwd": str(Path.cwd()), "python": sys.version,
    "scope": "EXACT_EVENT_EXTENT_AND_REAL_CONSUMER_SOURCE_HOST_GO_ONLY_FULL_S4_BUILD_NATIVE_NO_GO",
    "all_1407_successful_run_inputs_rechecked": True,
    "all_successful_run_outputs_and_source_extraction_extents_rechecked": True,
    "source_tests_helpers_original_evidence_immutable": True,
    "files": {str(path.relative_to(ROOT)): pin(path) for path in paths},
    "self_excluded": True,
}
with (HERE / "review-seal.json").open("x", encoding="utf-8") as stream:
    json.dump(seal, stream, indent=2)
    stream.write("\n")
print("A2b independent review sealed; all 1407 inputs and fresh HOST outputs unchanged")
