#!/usr/bin/env python3
"""Exclusive public-text A1 derivative; no device, binary or build-tree access."""
import datetime
import hashlib
import json
from pathlib import Path
import sys

if sys.flags.optimize:
    raise SystemExit("refuse optimized Python")
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
A1 = ROOT / "apps/qualification/kernel76-station/source"
E = ROOT / "docs/qualification-plans/kernel76-transport-20261021"
SEAL = E / "station-control-a1-host01/candidate-seal.json"
REVIEW = E / "STATION76_CONTROL_A1_REVIEW.md"

def sha(b):
    return hashlib.sha256(b).hexdigest()

seal_bytes = SEAL.read_bytes()
seal = json.loads(seal_bytes)
review = REVIEW.read_bytes()
pins = {k.removeprefix("apps/qualification/kernel76-station/source/"): v
        for k, v in seal["files"].items()
        if k.startswith("apps/qualification/kernel76-station/source/")}
assert len(pins) == 68
assert set(pins) == {str(p.relative_to(A1)) for p in A1.rglob("*") if p.is_file()}
review_pins = {
    "utils/fwctrl.c": "67e95c81ad1d044475eb521f90f95ffc43702b50d2a6d4c8e4be6d6026750243",
    "utils/fwctrl.h": "e35102bd358117fb02104e17894d053abc7a31a4940953d849c02b06e6fb27fc",
    "utils/control_receipt.h": "3b80f917e989fd3587f61e94d9dea713bb26059d7243de5cf0c7bbaccc869073",
}
for k, v in review_pins.items():
    assert v.encode() in review and pins[k]["sha256"] == v
data = {}
for rel, pin in pins.items():
    b = (A1 / rel).read_bytes()
    assert not (A1 / rel).is_symlink()
    assert len(b) == pin["bytes"] and sha(b) == pin["sha256"]
    data[rel] = b
out = E / "station-ingress-a2a-host01"
out.mkdir(exist_ok=False)
(HERE / "source").mkdir(exist_ok=False)
files = []
for rel, b in sorted(data.items()):
    p = HERE / "source" / rel
    p.parent.mkdir(parents=True, exist_ok=True)
    with p.open("xb") as f:
        f.write(b)
    files.append({"path": rel, "offset": 0, "bytes": len(b),
                  "original_sha256": sha(b), "copied_sha256": sha(p.read_bytes())})
receipt = {"utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
           "argv": sys.argv, "python": sys.version, "script_sha256": sha(Path(__file__).read_bytes()),
           "a1_seal_sha256": sha(seal_bytes), "a1_review_sha256": sha(review),
           "review_three_pins": review_pins, "count": len(files), "files": files}
with (out / "copy-baseline.json").open("x") as f:
    json.dump(receipt, f, indent=2)
    f.write("\n")
print("Copied 68 sealed A1 public source files byte-exact; no build")
