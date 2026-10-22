#!/usr/bin/env python3
"""Exclusive P4a byte copy/public custody and preimplementation contract capture."""
import datetime
import hashlib
import json
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
DOC = ROOT / "docs/qualification-plans/kernel76-transport-20261021"
E = DOC / "station-owner-lifetime-p4a-source01"
OLD = ROOT / "apps/qualification/kernel76-station-attach-init-v2/source"
K = ROOT / "work/lightbulb-camera/foundation-20261011T041148Z/artifacts/native-usb76-source01/linux"


def pin(p):
    b = p.read_bytes()
    return {"bytes": len(b), "sha256": hashlib.sha256(b).hexdigest()}


def main():
    if sys.flags.optimize or sys.argv[1:]:
        raise SystemExit("no optimized Python or arguments")
    prior = json.loads((DOC / "station-attach-tx-p3-source01/copy-baseline.json").read_bytes())
    custody = dict(prior["frozen_custody"])
    seal = DOC / "station-attach-tx-p3-source01/candidate-seal.json"
    if pin(seal)["sha256"] != "5c49d06840c74e1704eaccbf7401c30358d8117a79f802065500f3d59ac8d38b":
        raise SystemExit("P3 seal mismatch")
    custody[str(seal.relative_to(ROOT))] = pin(seal)
    custody.update(json.loads(seal.read_bytes())["files"])
    for rel, expected in custody.items():
        if (ROOT / rel).is_symlink() or pin(ROOT / rel) != expected:
            raise SystemExit("frozen input changed: " + rel)
    for item in prior["accepted"].values():
        for rel, expected in item["source"].items():
            if pin(ROOT / rel) != expected:
                raise SystemExit("accepted source mismatch: " + rel)
    source = prior["source"]
    if len(source) != 71:
        raise SystemExit("unexpected source inventory")
    E.mkdir(exist_ok=False)
    (HERE / "source").mkdir(exist_ok=False)
    for rel, expected in sorted(source.items()):
        p = HERE / "source" / rel
        if pin(OLD / rel) != expected:
            raise SystemExit("P2 v2 source mismatch: " + rel)
        p.parent.mkdir(parents=True, exist_ok=True)
        with p.open("xb") as f:
            f.write((OLD / rel).read_bytes())
    report = DOC / "STATION76_OWNER_LIFETIME_P4A_CANDIDATE.md"
    with (E / "OWNER_CONTRACT_BEFORE_IMPLEMENTATION.md").open("xb") as f:
        f.write(report.read_bytes())
    ranges = {
        K / "drivers/usb/core/urb.c": [(677, 779)],
        K / "drivers/usb/core/hcd.c": [(1519, 1569), (1634, 1672)],
        K / "kernel/workqueue.c": [(4463, 4485)],
        K / "fs/proc/inode.c": [(255, 273)],
        OLD / "utils/if_usb.c": [(29, 154), (268, 381), (485, 518)],
        OLD / "hgic_smac/attach_init.c": [(292, 316), (407, 467)],
    }
    contracts = {}
    for p, selected in ranges.items():
        lines = p.read_bytes().splitlines(keepends=True)
        rows = []
        for start, end in selected:
            b = b"".join(lines[start - 1:end])
            rows.append({"start_line": start, "end_line": end, "text": b.decode(),
                         "sha256": hashlib.sha256(b).hexdigest()})
        contracts[str(p.relative_to(ROOT))] = {**pin(p), "excerpts": rows}
    receipt = {"utc": datetime.datetime.now(datetime.timezone.utc).isoformat(), "argv": sys.argv,
               "cwd": str(ROOT), "python": sys.version, "preparer": pin(Path(__file__)),
               "baseline": source, "frozen_custody": custody, "accepted": prior["accepted"],
               "preimplementation_contract": pin(E / "OWNER_CONTRACT_BEFORE_IMPLEMENTATION.md"),
               "scope": "P4a shared-owner prerequisite selected before P3; no P1 adoption or runtime"}
    for name, data in (("copy-baseline.json", receipt), ("kernel-owner-contracts.json", contracts)):
        with (E / name).open("x") as f:
            json.dump(data, f, indent=2)
            f.write("\n")
    print(json.dumps({"copied": 71, "frozen_prior_files_verified": len(custody), "accepted_sources_verified": 204}))


if __name__ == "__main__":
    main()
