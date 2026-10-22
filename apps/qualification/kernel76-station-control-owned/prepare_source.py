#!/usr/bin/env python3
"""Exclusive reviewed-P4a public byte copy and immutable custody verification."""
import datetime
import hashlib
import json
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
DOC = ROOT / "docs/qualification-plans/kernel76-transport-20261021"
E = DOC / "station-control-owned-p3-source01"
OLD = ROOT / "apps/qualification/kernel76-station-owner-lifetime/source"


def pin(p):
    b = p.read_bytes()
    return {"bytes": len(b), "sha256": hashlib.sha256(b).hexdigest()}


def main():
    if sys.flags.optimize or sys.argv[1:]:
        raise SystemExit("no optimized Python or arguments")
    prior = json.loads((DOC / "station-owner-lifetime-p4a-source01/copy-baseline.json").read_bytes())
    custody = dict(prior["frozen_custody"])
    seals = [(DOC / "station-owner-lifetime-p4a-source01/candidate-seal.json",
              "49e1c0e6209371e4714e4fb1ff5adad4d8cb042014fe4508f2e5257e37f49490"),
             (DOC / "station-owner-lifetime-p4a-review01/review-seal.json",
              "c677fe73b1e28691365e31383780f12b89b797bafe6baa075f6d7ae366bdc4ff")]
    for p, expected in seals:
        if pin(p)["sha256"] != expected:
            raise SystemExit("seal mismatch: " + str(p))
        custody[str(p.relative_to(ROOT))] = pin(p)
        custody.update(json.loads(p.read_bytes())["files"])
    for rel, expected in custody.items():
        if (ROOT / rel).is_symlink() or pin(ROOT / rel) != expected:
            raise SystemExit("frozen input mismatch: " + rel)
    for item in prior["accepted"].values():
        for rel, expected in item["source"].items():
            if pin(ROOT / rel) != expected:
                raise SystemExit("accepted source mismatch: " + rel)
    s = json.loads(seals[0][0].read_bytes())
    prefix = str(OLD.relative_to(ROOT)) + "/"
    source = {k[len(prefix):]: v for k, v in s["files"].items() if k.startswith(prefix)}
    if len(source) != 72 or set(source) != {str(p.relative_to(OLD)) for p in OLD.rglob("*") if p.is_file()}:
        raise SystemExit("unexpected P4a source set")
    E.mkdir(exist_ok=False)
    (HERE / "source").mkdir(exist_ok=False)
    for rel, expected in source.items():
        p = HERE / "source" / rel
        p.parent.mkdir(parents=True, exist_ok=True)
        with p.open("xb") as f:
            f.write((OLD / rel).read_bytes())
        if pin(p) != expected:
            raise SystemExit("copy mismatch")
    with (E / "REQUEST_CONTRACT_BEFORE_IMPLEMENTATION.md").open("xb") as f:
        f.write((DOC / "STATION76_CONTROL_OWNED_P3_CANDIDATE.md").read_bytes())
    receipt = {"utc": datetime.datetime.now(datetime.timezone.utc).isoformat(), "argv": sys.argv,
               "cwd": str(ROOT), "python": sys.version, "preparer": pin(Path(__file__)),
               "baseline": source, "frozen_custody": custody, "accepted": prior["accepted"],
               "contract": pin(E / "REQUEST_CONTRACT_BEFORE_IMPLEMENTATION.md"),
               "scope": "one owned P3 request on exact P4a producer/join/poison; no P1/runtime/native"}
    with (E / "copy-baseline.json").open("x") as f:
        json.dump(receipt, f, indent=2)
        f.write("\n")
    print(json.dumps({"copied": 72, "frozen_custody_verified": len(custody), "accepted_source_pins": 204}))


if __name__ == "__main__":
    main()
