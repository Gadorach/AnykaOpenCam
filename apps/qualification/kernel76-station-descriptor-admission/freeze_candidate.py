#!/usr/bin/env python3
"""P4b public source inventory/custody/delta/seal, never behavioral execution."""
import datetime
import difflib
import hashlib
import json
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
DOC = ROOT / "docs/qualification-plans/kernel76-transport-20261021"
E = DOC / "station-descriptor-admission-p4b-source01"
OLD = ROOT / "apps/qualification/kernel76-station-control-owned/source"
REPORT = DOC / "STATION76_DESCRIPTOR_ADMISSION_P4B_CANDIDATE.md"
K = ROOT / "work/lightbulb-camera/foundation-20261011T041148Z/artifacts/native-usb76-source01/linux"


def pin(p):
    if p.is_symlink():
        raise SystemExit("unexpected symlink: " + str(p))
    b = p.read_bytes()
    return {"bytes": len(b), "sha256": hashlib.sha256(b).hexdigest()}


def dump(p, obj):
    with p.open("x") as f:
        json.dump(obj, f, indent=2)
        f.write("\n")


def verify():
    baseline = json.loads((E / "copy-baseline.json").read_bytes())
    reader = json.loads((DOC / "station-control-owned-p3-review01/input-seal.json").read_bytes())["inputs"]
    for mapping in (baseline["frozen_custody"], reader):
        for rel, expected in mapping.items():
            if pin(ROOT / rel) != expected:
                raise SystemExit("frozen public input mismatch: " + rel)
    for item in baseline["accepted"].values():
        for rel, expected in item["source"].items():
            if pin(ROOT / rel) != expected:
                raise SystemExit("accepted source mismatch: " + rel)
    for rel, expected in baseline["baseline"].items():
        if pin(OLD / rel) != expected:
            raise SystemExit("P3 baseline mismatch: " + rel)
    return baseline, reader


def inventory():
    baseline, reader = verify()
    source = HERE / "source"
    files = {str(p.relative_to(source)): pin(p) for p in sorted(source.rglob("*")) if p.is_file()}
    changed = {rel: {"p3": expected, "p4b": files.get(rel)} for rel, expected in baseline["baseline"].items()
               if files.get(rel) != expected}
    if set(files) != set(baseline["baseline"]) or set(changed) != {"utils/owner_usb.c", "hgic_smac/attach_status.c"}:
        raise SystemExit("unexpected source set/delta")
    before = (OLD / "utils/owner_usb.c").read_bytes()
    after = (source / "utils/owner_usb.c").read_bytes()
    blocks = {}
    oldprobe = before.index(b"static int hgic_usb_probe(")
    newhelper = after.index(b"/* Historical cached scalars,")
    segments = {
        "all_preprobe_P3_P4a_code": (before[:oldprobe], after[:newhelper]),
        "probe_allocation_and_owner_initialization": (
            before[before.index(b"    usb = kzalloc("):before.index(b"    /* Preserve P2's minimal descriptor policy.")],
            after[after.index(b"    usb = kzalloc("):after.index(b"    /* Only the exact preallocation-admitted pair;")]),
        "allocation_core_probe_publication_32RX_rollback_disconnect_module_return": (
            before[before.index(b"    error = hgic_usb_alloc_slots(usb);"):],
            after[after.index(b"    error = hgic_usb_alloc_slots(usb);"):]),
    }
    for name, (a, b) in segments.items():
        if a != b:
            raise SystemExit("preserved source block changed: " + name)
        blocks[name] = {"bytes": len(a), "sha256": hashlib.sha256(a).hexdigest()}
    status_before = (OLD / "hgic_smac/attach_status.c").read_bytes()
    status_after = (source / "hgic_smac/attach_status.c").read_bytes()
    if status_after != status_before.replace(b"policy=control-owned-p3-source01 schema=2", b"policy=descriptor-admission-p4b-source01 schema=2"):
        raise SystemExit("status changed beyond fixed policy label")
    syntax = json.loads((E / "syntax01/receipt.json").read_bytes())
    if syntax["source"] != {str((source / rel).relative_to(ROOT)): p for rel, p in files.items()} or not syntax["pass"] or len(syntax["commands"]) != 2:
        raise SystemExit("final syntax pin mismatch")
    delta = []
    for rel in sorted(changed):
        delta.extend(difflib.unified_diff((OLD / rel).read_text().splitlines(keepends=True),
                                         (source / rel).read_text().splitlines(keepends=True),
                                         fromfile="reviewed-p3/" + rel, tofile="p4b-subset/" + rel))
    with (E / "CACHED_DESCRIPTOR_DELTA.patch").open("x") as f:
        f.writelines(delta)
    ranges = {
        K / "include/linux/usb.h": [(50, 97), (182, 252), (304, 307), (340, 425)],
        K / "include/uapi/linux/usb/ch9.h": [(291, 310), (346, 366), (396, 449), (649, 658)],
        K / "drivers/usb/core/config.c": [(15, 20), (307, 367), (457, 507), (576, 605), (619, 640),
                                             (682, 704), (756, 769), (800, 838), (925, 961)],
        K / "drivers/usb/core/message.c": [(2155, 2205)],
        K / "drivers/usb/core/driver.c": [(320, 350), (390, 410), (2082, 2087)],
        K / "drivers/base/dd.c": [(1162, 1192)],
        K / "drivers/usb/core/endpoint.c": [(37, 58), (150, 170)],
        K / "drivers/usb/core/sysfs.c": [(25, 43), (145, 178), (690, 704)],
        ROOT / "apps/qualification/kernel76-transport/metadata21a.py": [(27, 28), (60, 92)],
        DOC / "METADATA_RECEIPT_REVIEW.md": [(1, 65), (120, 134)],
        DOC / "STATION76_ATTACH_REMAINING_SOURCE_PLAN.md": [(281, 330)],
        DOC / "STATION76_CONTROL_OWNED_P3_REVIEW.md": [(1, 290)],
    }
    anchors = {}
    for path, selected in ranges.items():
        lines = path.read_bytes().splitlines(keepends=True)
        excerpts = []
        for start, end in selected:
            b = b"".join(lines[start - 1:end])
            excerpts.append({"start_line": start, "end_line": end, "text": b.decode(),
                             "sha256": hashlib.sha256(b).hexdigest()})
        anchors[str(path.relative_to(ROOT))] = {**pin(path), "excerpts": excerpts}
    dump(E / "public-fact-api-anchors.json", anchors)
    dump(E / "p3-review-reader-custody.json", reader)
    sha = hashlib.sha256(json.dumps(files, sort_keys=True, separators=(",", ":")).encode()).hexdigest()
    dump(E / "source-inventory.json", {"utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
         "argv": sys.argv, "cwd": str(ROOT), "python": sys.version, "inventory_tool": pin(Path(__file__)),
         "source": files, "source_sha256": sha, "aggregate_format": "SHA256 compact sorted-key UTF-8 JSON source size/hash map",
         "changed": changed, "unchanged_p3_files": 72, "preserved_blocks": blocks,
         "frozen_custody_verified": len(baseline["frozen_custody"]), "p3_review_reader_inputs_verified": len(reader),
         "accepted_source_pins": 204, "syntax_receipt": pin(E / "syntax01/receipt.json"),
         "scope": "P4b selected cached-interface subset only; no SOURCE/native/product GO"})
    print(json.dumps({"source_files": len(files), "changed": sorted(changed), "source_sha256": sha,
                      "prior_custody": len(baseline["frozen_custody"]), "reader_custody": len(reader)}))


def seal():
    baseline, reader = verify()
    data = json.loads((E / "source-inventory.json").read_bytes())
    source = HERE / "source"
    actual = {str(p.relative_to(source)): pin(p) for p in sorted(source.rglob("*")) if p.is_file()}
    if actual != data["source"] or json.loads((E / "p3-review-reader-custody.json").read_bytes()) != reader:
        raise SystemExit("inventory/source/reader map changed")
    for rel, expected in json.loads((E / "public-fact-api-anchors.json").read_bytes()).items():
        if pin(ROOT / rel) != {k: expected[k] for k in ("bytes", "sha256")}:
            raise SystemExit("public fact/API input changed: " + rel)
    files = {}
    for p in sorted(list(HERE.rglob("*")) + list(E.rglob("*")) + [REPORT]):
        if p.is_symlink():
            raise SystemExit("unexpected owned symlink")
        if p.is_file() and p.name != "candidate-seal.json":
            files[str(p.relative_to(ROOT))] = pin(p)
    dump(E / "candidate-seal.json", {"utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
         "argv": sys.argv, "cwd": str(ROOT), "python": sys.version, "files": files,
         "source_sha256": data["source_sha256"], "frozen_custody_verified": len(baseline["frozen_custody"]),
         "p3_review_reader_inputs_verified": len(reader), "accepted_source_pins": 204,
         "scope": "P4b cached selected-interface SOURCE candidate; changed-unit HOST syntax01 2/2 only",
         "limitations": ["unknown exact configuration/interface/alternate counts and active alternate value",
                         "exclusive pair/full packet field are restrictive policy, not complete measurements",
                         "cache may have been normalized by USB core; no original wire-descriptor proof",
                         "independent review and ordinary functional confirmation absent",
                         "P1/full P4/P5/composite/ARM/module/native/product acceptance absent"]})
    print(json.dumps({"sealed_files": len(files), "seal": pin(E / "candidate-seal.json")}))


if __name__ == "__main__":
    if sys.flags.optimize or len(sys.argv) != 2 or sys.argv[1] not in ("inventory", "seal"):
        raise SystemExit("usage: python3 -B freeze_candidate.py inventory|seal (fresh outputs)")
    {"inventory": inventory, "seal": seal}[sys.argv[1]]()
