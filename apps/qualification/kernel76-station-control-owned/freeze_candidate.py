#!/usr/bin/env python3
"""Exclusive owned-control P3 source inventory/seal; no behavioral execution."""
import datetime
import difflib
import hashlib
import json
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
DOC = ROOT / "docs/qualification-plans/kernel76-transport-20261021"
E = DOC / "station-control-owned-p3-source01"
REPORT = DOC / "STATION76_CONTROL_OWNED_P3_CANDIDATE.md"
OLD = ROOT / "apps/qualification/kernel76-station-owner-lifetime/source"
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
    if pin(E / "REQUEST_CONTRACT_BEFORE_IMPLEMENTATION.md") != baseline["contract"]:
        raise SystemExit("original request contract changed")
    for name in ("kernel-owner-contracts.json", "module-return-contracts.json"):
        for rel, expected in json.loads((DOC / "station-owner-lifetime-p4a-source01" / name).read_bytes()).items():
            if pin(ROOT / rel) != {k: expected[k] for k in ("bytes", "sha256")}:
                raise SystemExit("P4a return contract changed: " + rel)
    return baseline


def inventory():
    baseline = verify()
    source = HERE / "source"
    files = {str(p.relative_to(source)): pin(p) for p in sorted(source.rglob("*")) if p.is_file()}
    changed = {rel: {"p4a": expected, "p3": files.get(rel)} for rel, expected in baseline["baseline"].items()
               if files.get(rel) != expected}
    added = {rel: expected for rel, expected in files.items() if rel not in baseline["baseline"]}
    if len(files) != 74 or set(changed) != {"hgic_smac/Makefile", "hgic_smac/attach_init.c", "hgic_smac/attach_status.c",
            "utils/fwctrl.c", "utils/fwctrl.h", "utils/owner_usb.c"} or set(added) != {"utils/owned_control.c", "utils/owned_control.h"}:
        raise SystemExit("unexpected P3 source change set")
    for rel in ("LICENSE", "hgic_smac/rx.c", "hgic_smac/event.c", "hgic_smac/core.c", "hgic_smac/procfs.c",
                "utils/control_receipt.h", "utils/if_usb.c", "hgic.h", "hgic_def.h", "attach_init.h"):
        if files[rel] != baseline["baseline"][rel]:
            raise SystemExit("immutable imported source changed: " + rel)
    syntax = json.loads((E / "syntax04/receipt.json").read_bytes())
    expected = {str((source / rel).relative_to(ROOT)): p for rel, p in files.items()}
    if syntax["source"] != expected or not syntax["pass"] or len(syntax["commands"]) != 5:
        raise SystemExit("final syntax input mismatch")
    preserved = {}
    blocks = [("hgic_smac/attach_init.c", "static void attach_owner_stop(", "static int attach_core_probe("),
              ("utils/owner_usb.c", "static void hgic_usb_dispose(", "static int hgic_usb_probe("),
              ("utils/owner_usb.c", "static void hgic_usb_receive(struct urb *urb)\n{", "static void hgic_usb_tx_complete(struct urb *urb)\n{"),
              ("utils/owner_usb.c", "static void hgic_usb_reserve_slot(", "static void hgic_usb_requeue(")]
    for rel, start, end in blocks:
        before, after = (OLD / rel).read_bytes(), (source / rel).read_bytes()
        a, b = start.encode(), end.encode()
        oldblock = before[before.index(a):before.index(b, before.index(a))]
        newblock = after[after.index(a):after.index(b, after.index(a))]
        if oldblock != newblock:
            raise SystemExit("P4a owner-return/producer block altered: " + rel + start)
        preserved[rel + "::" + start] = {"bytes": len(oldblock), "sha256": hashlib.sha256(oldblock).hexdigest()}
    patch = []
    for rel in sorted(changed):
        patch.extend(difflib.unified_diff((OLD / rel).read_text().splitlines(keepends=True),
                                          (source / rel).read_text().splitlines(keepends=True),
                                          fromfile="p4a/" + rel, tofile="p3-owned/" + rel))
    for rel in sorted(added):
        patch.extend(difflib.unified_diff([], (source / rel).read_text().splitlines(keepends=True),
                                          fromfile="/dev/null", tofile="p3-owned/" + rel))
    with (E / "OWNED_CONTROL_DELTA.patch").open("x") as f:
        f.writelines(patch)
    ranges = {"kernel/locking/semaphore.c": [(158, 184)], "kernel/time/time.c": [(782, 812)],
              "include/linux/timekeeping.h": [(24, 36), (168, 176)],
              "kernel/sched/completion.c": [(158, 174)],
              "include/linux/refcount.h": [(369, 384), (429, 450)]}
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
    dump(E / "request-api-contracts.json", contracts)
    aggregate = hashlib.sha256(json.dumps(files, sort_keys=True, separators=(",", ":")).encode()).hexdigest()
    dump(E / "source-inventory.json", {"utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
         "argv": sys.argv, "cwd": str(ROOT), "python": sys.version, "inventory_tool": pin(Path(__file__)),
         "source": files, "source_sha256": aggregate, "aggregate_format": "SHA256 UTF-8 compact sorted-key JSON source map",
         "changed": changed, "added": added, "unchanged_p4a": 66, "preserved_p4a_blocks": preserved,
         "frozen_custody_verified": len(baseline["frozen_custody"]), "accepted_source_pins": 204,
         "syntax_receipt": pin(E / "syntax04/receipt.json"),
         "scope": "one owned P3 control request candidate; no runtime/source/native/product GO"})
    print(json.dumps({"source_files": 74, "changed": sorted(changed), "added": sorted(added), "source_sha256": aggregate}))


def seal():
    baseline = verify()
    inventory_data = json.loads((E / "source-inventory.json").read_bytes())
    source = HERE / "source"
    actual = {str(p.relative_to(source)): pin(p) for p in sorted(source.rglob("*")) if p.is_file()}
    if actual != inventory_data["source"]:
        raise SystemExit("source changed after inventory")
    for rel, expected in json.loads((E / "request-api-contracts.json").read_bytes()).items():
        if pin(ROOT / rel) != {k: expected[k] for k in ("bytes", "sha256")}:
            raise SystemExit("request API contract changed: " + rel)
    files = {}
    for p in sorted(list(HERE.rglob("*")) + list(E.rglob("*")) + [REPORT]):
        if p.is_symlink():
            raise SystemExit("unexpected owned symlink")
        if p.is_file() and p.name != "candidate-seal.json":
            files[str(p.relative_to(ROOT))] = pin(p)
    dump(E / "candidate-seal.json", {"utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
         "argv": sys.argv, "cwd": str(ROOT), "python": sys.version, "files": files,
         "source_sha256": inventory_data["source_sha256"], "frozen_custody_verified": len(baseline["frozen_custody"]),
         "accepted_source_pins": 204, "scope": "P3 owned-control SOURCE candidate, final syntax04 five units only",
         "limitations": ["independent review missing", "ordinary functional execution missing", "P1 absent/unmet",
                         "full P4/P5 unmet", "no paired ARM/module/native/product acceptance"]})
    print(json.dumps({"files": len(files), "seal": pin(E / "candidate-seal.json")}))


if __name__ == "__main__":
    if sys.flags.optimize or len(sys.argv) != 2 or sys.argv[1] not in ("inventory", "seal"):
        raise SystemExit("usage: python3 -B freeze_candidate.py inventory|seal (fresh outputs)")
    {"inventory": inventory, "seal": seal}[sys.argv[1]]()
