#!/usr/bin/env python3
"""Exclusive frozen P3 byte-copy / public provenance verification; no imports of recipes."""
import datetime
import hashlib
import json
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
DOC = ROOT / "docs/qualification-plans/kernel76-transport-20261021"
E = DOC / "station-descriptor-admission-p4b-source01"
OLD = ROOT / "apps/qualification/kernel76-station-control-owned/source"


def pin(p):
    if p.is_symlink():
        raise SystemExit("unexpected symlink: " + str(p))
    b = p.read_bytes()
    return {"bytes": len(b), "sha256": hashlib.sha256(b).hexdigest()}


def main():
    if sys.flags.optimize or sys.argv[1:]:
        raise SystemExit("no optimization/arguments")
    prior = json.loads((DOC / "station-control-owned-p3-source01/copy-baseline.json").read_bytes())
    custody = dict(prior["frozen_custody"])
    for path, sha in [
        (DOC / "station-control-owned-p3-source01/candidate-seal.json",
         "5d8a96a78e1b6d99f7a16f259d36f0e077644f7e7378c8ecd88b58109add66eb"),
        (DOC / "station-control-owned-p3-review01/review-seal.json",
         "c449dee74290c8bdd5d318f71816b83576d31a84a407ab05705092094f94db38")]:
        if pin(path)["sha256"] != sha:
            raise SystemExit("seal mismatch: " + str(path))
        entries = {str(path.relative_to(ROOT)): pin(path), **json.loads(path.read_bytes())["files"]}
        for rel, expected in entries.items():
            if rel in custody and custody[rel] != expected:
                raise SystemExit("custody conflict: " + rel)
            custody[rel] = expected
    for rel, expected in custody.items():
        if pin(ROOT / rel) != expected:
            raise SystemExit("custody mismatch: " + rel)
    for item in prior["accepted"].values():
        for rel, expected in item["source"].items():
            if pin(ROOT / rel) != expected:
                raise SystemExit("accepted source mismatch: " + rel)
    baseline = json.loads((DOC / "station-control-owned-p3-source01/source-inventory.json").read_bytes())["source"]
    if len(baseline) != 74 or set(baseline) != {str(p.relative_to(OLD)) for p in OLD.rglob("*") if p.is_file()}:
        raise SystemExit("unexpected P3 source set")
    for rel, expected in baseline.items():
        if pin(OLD / rel) != expected:
            raise SystemExit("P3 source mismatch: " + rel)
    E.mkdir(exist_ok=False)
    (HERE / "source").mkdir(exist_ok=False)
    for rel, expected in baseline.items():
        dest = HERE / "source" / rel
        dest.parent.mkdir(parents=True, exist_ok=True)
        with dest.open("xb") as f:
            f.write((OLD / rel).read_bytes())
        if pin(dest) != expected:
            raise SystemExit("copy mismatch: " + rel)
    receipt = {"utc": datetime.datetime.now(datetime.timezone.utc).isoformat(), "argv": sys.argv,
               "cwd": str(ROOT), "python": sys.version, "preparer": pin(Path(__file__)),
               "baseline": baseline, "frozen_custody": custody, "accepted": prior["accepted"],
               "scope": "P4b cached selected-interface subset; no P1/full P4/P5/native selection"}
    with (E / "copy-baseline.json").open("x") as f:
        json.dump(receipt, f, indent=2)
        f.write("\n")
    print(json.dumps({"copied": len(baseline), "custody_verified": len(custody), "accepted_pins": 204}))


if __name__ == "__main__":
    main()
