#!/usr/bin/env python3
"""Exclusive A2b public source/test copy. No project execution or device access."""
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
OLD = ROOT / "apps/qualification/kernel76-station-ingress"
E = DOC / "station-event-a2b-host01"
def sha(b):
    return hashlib.sha256(b).hexdigest()
def pin(p):
    b = p.read_bytes()
    return {"bytes": len(b), "sha256": sha(b)}
inputs = {}
for name in ("station-control-a1-host01/candidate-seal.json",
             "station-ingress-a2a-host01/candidate-seal.json",
             "station-ingress-a2a-review01/review-seal.json",
             "STATION76_INGRESS_A2A_REVIEW.md", "STATION76_EVENT_ENTRY_REVIEW.md"):
    inputs[name] = pin(DOC / name)
seal = json.loads((DOC / "station-ingress-a2a-host01/candidate-seal.json").read_bytes())
prefix = str((OLD / "source").relative_to(ROOT)) + "/"
pins = {k[len(prefix):]: v for k, v in seal["files"].items() if k.startswith(prefix)}
assert len(pins) == 68
assert set(pins) == {str(p.relative_to(OLD / "source")) for p in (OLD / "source").rglob("*") if p.is_file()}
review = (DOC / "STATION76_INGRESS_A2A_REVIEW.md").read_text()
changed = ["utils/if_usb.c", "hgic_smac/rx.c", "utils/fwctrl.c", "utils/fwctrl.h",
           "hgic_def.h", "hgic_smac/hgics.h", "hgic_smac/core.c"]
for rel in changed:
    assert pins[rel]["sha256"] in review
assert pins["LICENSE"]["sha256"] == "8177f97513213526df2cf6184d8ff986c675afb514d4e68a404010521b880643"
# Check both original sets and independent review inventory, without executing it.
for sealname in ("station-control-a1-host01/candidate-seal.json",
                 "station-ingress-a2a-review01/review-seal.json"):
    s = json.loads((DOC / sealname).read_bytes())
    for rel, expected in s["files"].items():
        assert pin(ROOT / rel) == expected, rel
data = {}
for rel, expected in pins.items():
    p = OLD / "source" / rel
    assert not p.is_symlink() and pin(p) == expected
    data[rel] = p.read_bytes()
E.mkdir(exist_ok=False)
(HERE / "source").mkdir(exist_ok=False)
rows = []
for rel, b in sorted(data.items()):
    p = HERE / "source" / rel
    p.parent.mkdir(parents=True, exist_ok=True)
    with p.open("xb") as f:
        f.write(b)
    rows.append({"path": rel, "offset": 0, "bytes": len(b),
                 "original_sha256": sha(b), "copied_sha256": sha(p.read_bytes())})
testrows = []
(HERE / "tests").mkdir(exist_ok=False)
for p in sorted((OLD / "tests").glob("*")):
    b = p.read_bytes()
    with (HERE / "tests" / p.name).open("xb") as f:
        f.write(b)
    testrows.append({"path": p.name, "bytes": len(b), "sha256": sha(b)})
receipt = {"utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
           "argv": sys.argv, "cwd": str(ROOT), "python": sys.version,
           "preparer_sha256": sha(Path(__file__).read_bytes()), "inputs": inputs,
           "review_seven_pins_verified": True, "a1_review_seals_verified": True,
           "count": len(rows), "files": rows, "copied_tests": testrows}
with (E / "copy-baseline.json").open("x") as f:
    json.dump(receipt, f, indent=2)
    f.write("\n")
print("Copied 68 accepted A2a source files and separate regression test inputs")
