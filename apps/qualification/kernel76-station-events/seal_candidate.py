#!/usr/bin/env python3
"""Read-only public-input recheck plus exclusive A2b seal; no build or hardware."""
import datetime
import hashlib
import json
from pathlib import Path
import sys

if sys.flags.optimize:
    raise SystemExit("refuse optimized Python")
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
DOC = ROOT / "docs/qualification-plans/kernel76-transport-20261021"
E = DOC / "station-event-a2b-host01"
def sha(b):
    return hashlib.sha256(b).hexdigest()
def pin(p):
    b=p.read_bytes()
    return {"bytes":len(b),"sha256":sha(b)}
result=json.loads((E / "run08/result.json").read_bytes())
assert result["all_pass"] and len(result["mutants"])==10
for rel, expected in result["inputs"].items():
    assert pin(ROOT / rel)==expected, rel
for rel, expected in json.loads((E / "run08/output-hashes.json").read_bytes()).items():
    assert pin(E / "run08" / rel)==expected, rel
for e in result["extractions"]:
    if not e["mutated"]:
        b=(ROOT / e["source"]).read_bytes()[e["start_byte"]:e["start_byte"]+e["bytes"]]
        assert sha(b)==e["sha256"]
for name in ("station-control-a1-host01/candidate-seal.json",
             "station-ingress-a2a-host01/candidate-seal.json",
             "station-ingress-a2a-review01/review-seal.json"):
    for rel, expected in json.loads((DOC / name).read_bytes())["files"].items():
        assert pin(ROOT / rel)==expected, rel
baseline=json.loads((E / "copy-baseline.json").read_bytes())
changed=[]
rows=[]
for r in baseline["files"]:
    rel=r["path"]
    p=HERE / "source" / rel
    old=ROOT / "apps/qualification/kernel76-station-ingress/source" / rel
    assert pin(old)=={"bytes":r["bytes"],"sha256":r["original_sha256"]}
    now=pin(p)
    if now["sha256"]!=r["original_sha256"]:
        changed.append(rel)
    rows.append({"path":rel,"offset":0,**now,"accepted_a2a_sha256":r["original_sha256"]})
assert len(rows)==68 and changed==["hgic_smac/event.c","hgic_smac/rx.c"]
files={}
report=DOC / "STATION76_EVENT_A2B_CANDIDATE.md"
for p in [report]+sorted(HERE.rglob("*"))+sorted(E.rglob("*")):
    if p.is_file():
        assert not p.is_symlink()
        files[str(p.relative_to(ROOT))]=pin(p)
seal={"utc":datetime.datetime.now(datetime.timezone.utc).isoformat(),"argv":sys.argv,
      "cwd":str(ROOT),"python":sys.version,"preparer_sha256":pin(Path(__file__))["sha256"],
      "scope":"A2B_SOURCE_HOST_CANDIDATE_INDEPENDENT_REVIEW_REQUIRED_FULL_S4_ARM_NATIVE_NO_GO",
      "final_run":"run08","source_count":68,"unchanged_from_A2a":66,
      "changed_files":changed,"source_extents":rows,"files":files,
      "a1_a2a_original_and_independent_review_seals_rechecked":True,
      "final_inputs_outputs_and_unmutated_extraction_extents_rechecked":True}
with (E / "candidate-seal.json").open("x") as f:
    json.dump(seal,f,indent=2)
    f.write("\n")
print("A2b 68-file candidate sealed; 66 unchanged; originals rechecked; review required")
