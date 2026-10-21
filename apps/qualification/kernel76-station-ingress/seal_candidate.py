#!/usr/bin/env python3
"""Exclusive A2a handback seal; only public source/synthetic HOST evidence."""
import datetime
import hashlib
import json
from pathlib import Path
import re
import sys

if sys.flags.optimize:
    raise SystemExit("refuse optimized Python")
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
P = ROOT / "docs/qualification-plans/kernel76-transport-20261021"
E = P / "station-ingress-a2a-host01"
REPORT = P / "STATION76_INGRESS_A2A_CANDIDATE.md"
FINAL = E / "run08"

def sha(b):
    return hashlib.sha256(b).hexdigest()

result = json.loads((FINAL / "result.json").read_bytes())
assert result["all_pass"] and len(result["mutants"]) == 8
for rel, pin in result["inputs"].items():
    b = (ROOT / rel).read_bytes()
    assert len(b) == pin["bytes"] and sha(b) == pin["sha256"], rel
for rel, pin in json.loads((FINAL / "output-hashes.json").read_bytes()).items():
    b = (FINAL / rel).read_bytes()
    assert len(b) == pin["bytes"] and sha(b) == pin["sha256"], rel
baseline = json.loads((E / "copy-baseline.json").read_bytes())
for row in baseline["files"]:
    b = (ROOT / "apps/qualification/kernel76-station/source" / row["path"]).read_bytes()
    assert len(b) == row["bytes"] and sha(b) == row["original_sha256"]
assert (HERE / "source/LICENSE").read_bytes() == (ROOT / "apps/qualification/kernel76-station/source/LICENSE").read_bytes()

# Inventory every actual selected-tree caller/initializer, including unlinked
# SDIO. These are static references, NOT behavioral test counts or ABI proof for
# any unselected family source.
references = []
pattern = re.compile(r"rx_packet|rx_error|rx_failed|hgic_fwctrl_init\(|hgic_fwctrl_rx\(|usb_submit_urb\(|usb_(kill|unlink|free)_urb\(|bus_priv\s*=|HGIC_USB_STATUS_(ERR|STOP)")
for row in baseline["files"]:
    path = HERE / "source" / row["path"]
    for number, line in enumerate(path.read_bytes().decode(errors="replace").splitlines(), 1):
        if pattern.search(line):
            references.append({"path": row["path"], "line": number, "text": line})
with (E / "context-audit.json").open("x") as f:
    json.dump({"scope": "static selected68 references only; not test assertions",
               "source_count": 68, "references": references}, f, indent=2)
    f.write("\n")
files = {}
for path in sorted([p for p in HERE.rglob("*") if p.is_file()] +
                   [p for p in E.rglob("*") if p.is_file()] + [REPORT]):
    b = path.read_bytes()
    files[str(path.relative_to(ROOT))] = {"bytes": len(b), "sha256": sha(b)}
receipt = {"utc": datetime.datetime.now(datetime.timezone.utc).isoformat(), "argv": sys.argv,
           "python": sys.version, "script_sha256": sha(Path(__file__).read_bytes()),
           "scope": "A2A_SOURCE_HOST_CANDIDATE_ONLY_INDEPENDENT_REVIEW_PENDING_FULL_S4_ARM_NATIVE_NO_GO",
           "final_run": "run08", "all_final_inputs_outputs_rechecked": True,
           "all68_A1_originals_rechecked": True, "source_files": 68, "unchanged_from_A1": 61,
           "changed_files": result["changed_files"], "files": files}
with (E / "candidate-seal.json").open("x") as f:
    json.dump(receipt, f, indent=2)
    f.write("\n")
print("A2a candidate frozen/sealed; independent review pending; no native GO")
