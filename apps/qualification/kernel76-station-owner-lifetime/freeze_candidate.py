#!/usr/bin/env python3
"""Exclusive P4a source inventory/seal and public return-contract capture."""
import datetime
import difflib
import hashlib
import json
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
DOC = ROOT / "docs/qualification-plans/kernel76-transport-20261021"
E = DOC / "station-owner-lifetime-p4a-source01"
REPORT = DOC / "STATION76_OWNER_LIFETIME_P4A_CANDIDATE.md"
OLD = ROOT / "apps/qualification/kernel76-station-attach-init-v2/source"
K = ROOT / "work/lightbulb-camera/foundation-20261011T041148Z/artifacts/native-usb76-source01/linux"


def pin(p):
    b = p.read_bytes()
    return {"bytes": len(b), "sha256": hashlib.sha256(b).hexdigest()}


def dump(p, data):
    with p.open("x") as f:
        json.dump(data, f, indent=2)
        f.write("\n")


def verify():
    baseline = json.loads((E / "copy-baseline.json").read_bytes())
    for rel, expected in baseline["frozen_custody"].items():
        if pin(ROOT / rel) != expected:
            raise SystemExit("frozen input mismatch: " + rel)
    for item in baseline["accepted"].values():
        for rel, expected in item["source"].items():
            if pin(ROOT / rel) != expected:
                raise SystemExit("accepted source mismatch: " + rel)
    for rel, expected in json.loads((E / "kernel-owner-contracts.json").read_bytes()).items():
        if pin(ROOT / rel) != {k: expected[k] for k in ("bytes", "sha256")}:
            raise SystemExit("kernel/source contract mismatch: " + rel)
    if pin(E / "OWNER_CONTRACT_BEFORE_IMPLEMENTATION.md") != baseline["preimplementation_contract"]:
        raise SystemExit("preimplementation contract mismatch")
    return baseline


def inventory():
    baseline = verify()
    source = HERE / "source"
    files = {str(p.relative_to(source)): pin(p) for p in sorted(source.rglob("*")) if p.is_file()}
    changed = {rel: {"p2v2": expected, "p4a": files.get(rel)} for rel, expected in baseline["baseline"].items()
               if files.get(rel) != expected}
    added = {rel: expected for rel, expected in files.items() if rel not in baseline["baseline"]}
    if len(files) != 72 or set(changed) != {"hgic_def.h", "hgic_smac/Makefile", "hgic_smac/attach_init.c",
            "hgic_smac/attach_status.c", "utils/fwctrl.c", "utils/fwctrl.h"} or set(added) != {"utils/owner_usb.c"}:
        raise SystemExit("unexpected P4a source change set")
    for rel in ("LICENSE", "hgic_smac/rx.c", "hgic_smac/event.c", "hgic_smac/core.c", "hgic_smac/procfs.c",
                "utils/control_receipt.h", "utils/if_usb.c", "hgic.h"):
        if files[rel] != baseline["baseline"][rel]:
            raise SystemExit("immutable import changed: " + rel)
    syntax = json.loads((E / "syntax02/receipt.json").read_bytes())
    expected_inputs = {str((source / rel).relative_to(ROOT)): value for rel, value in files.items()}
    if syntax["source"] != expected_inputs or not syntax["pass"] or len(syntax["commands"]) != 4:
        raise SystemExit("final syntax metadata mismatch")
    patch = []
    for rel in sorted(changed):
        patch.extend(difflib.unified_diff((OLD / rel).read_text().splitlines(keepends=True),
                                          (source / rel).read_text().splitlines(keepends=True),
                                          fromfile="p2-v2/" + rel, tofile="p4a/" + rel))
    for rel in sorted(added):
        patch.extend(difflib.unified_diff([], (source / rel).read_text().splitlines(keepends=True),
                                          fromfile="/dev/null", tofile="p4a/" + rel))
    with (E / "P4A_SOURCE_DELTA.patch").open("x") as f:
        f.writelines(patch)
    ranges = {"drivers/usb/core/driver.c": [(431, 461), (1104, 1124)],
              "drivers/base/driver.c": [(264, 279)], "drivers/base/bus.c": [(775, 797)],
              "drivers/base/dd.c": [(1170, 1208), (1295, 1297), (1318, 1379), (1415, 1442)],
              "net/core/dev.c": [(2002, 2037), (2220, 2244)]}
    contracts = {}
    for rel, selected in ranges.items():
        path = K / rel
        lines = path.read_bytes().splitlines(keepends=True)
        rows = []
        for start, end in selected:
            b = b"".join(lines[start - 1:end])
            rows.append({"start_line": start, "end_line": end, "text": b.decode(),
                         "sha256": hashlib.sha256(b).hexdigest()})
        contracts[str(path.relative_to(ROOT))] = {**pin(path), "excerpts": rows}
    dump(E / "module-return-contracts.json", contracts)
    aggregate = hashlib.sha256(json.dumps(files, sort_keys=True, separators=(",", ":")).encode()).hexdigest()
    receipt = {"utc": datetime.datetime.now(datetime.timezone.utc).isoformat(), "argv": sys.argv,
               "cwd": str(ROOT), "python": sys.version, "inventory_tool": pin(Path(__file__)),
               "source": files, "source_sha256": aggregate,
               "aggregate_format": "SHA256 UTF-8 compact sorted-key JSON source map",
               "changed": changed, "added": added, "unchanged_p2v2": 65,
               "frozen_prior_files_verified": len(baseline["frozen_custody"]), "accepted_sources_verified": 204,
               "syntax_receipt": pin(E / "syntax02/receipt.json"),
               "scope": "P4a owner lifetime SOURCE candidate only; no P1/P3/full P4/P5/native GO"}
    dump(E / "source-inventory.json", receipt)
    print(json.dumps({"source_files": len(files), "changed": sorted(changed), "added": sorted(added),
                      "unchanged_p2v2": 65, "source_sha256": aggregate}))


def seal():
    baseline = verify()
    inventory_data = json.loads((E / "source-inventory.json").read_bytes())
    source = HERE / "source"
    actual = {str(p.relative_to(source)): pin(p) for p in sorted(source.rglob("*")) if p.is_file()}
    if actual != inventory_data["source"]:
        raise SystemExit("source changed after inventory")
    for rel, expected in json.loads((E / "module-return-contracts.json").read_bytes()).items():
        if pin(ROOT / rel) != {k: expected[k] for k in ("bytes", "sha256")}:
            raise SystemExit("module return contract changed: " + rel)
    files = {}
    for path in sorted(list(HERE.rglob("*")) + list(E.rglob("*")) + [REPORT]):
        if path.is_symlink():
            raise SystemExit("unexpected owned symlink")
        if path.is_file() and path.name != "candidate-seal.json":
            files[str(path.relative_to(ROOT))] = pin(path)
    dump(E / "candidate-seal.json", {"utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
         "argv": sys.argv, "cwd": str(ROOT), "python": sys.version, "files": files,
         "source_sha256": inventory_data["source_sha256"],
         "accepted_sources_verified": 204, "frozen_prior_files_verified": len(baseline["frozen_custody"]),
         "scope": "P4a shared-owner prerequisite candidate; syntax02 four changed units only, no runtime/source/native/product acceptance",
         "limitations": ["independent review missing", "ordinary functional confirmation missing", "P1 unmet",
                         "P3 request/deadlines not implemented", "full P4/P5 not implemented", "no native/ARM/module build"]})
    print(json.dumps({"files": len(files), "seal": pin(E / "candidate-seal.json")}))


if __name__ == "__main__":
    if sys.flags.optimize or len(sys.argv) != 2 or sys.argv[1] not in ("inventory", "seal"):
        raise SystemExit("usage: python3 -B freeze_candidate.py inventory|seal (exclusive outputs)")
    {"inventory": inventory, "seal": seal}[sys.argv[1]]()
