#!/usr/bin/env python3
"""Exclusive P2 v2 source inventory and seal; public integrity checks, no tests."""
import datetime
import difflib
import hashlib
import json
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
DOC = ROOT / "docs/qualification-plans/kernel76-transport-20261021"
EVIDENCE = DOC / "station-attach-init-p2-v2-source01"
REPORT = DOC / "STATION76_ATTACH_INIT_P2_V2_CANDIDATE.md"
OLD = ROOT / "apps/qualification/kernel76-station-attach-init/source"


def pin(path):
    data = path.read_bytes()
    return {"bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}


def dump(path, data):
    with path.open("x") as stream:
        json.dump(data, stream, indent=2)
        stream.write("\n")


def verify_custody():
    baseline = json.loads((EVIDENCE / "copy-baseline.json").read_bytes())
    for rel, expected in baseline["frozen_custody"].items():
        if pin(ROOT / rel) != expected:
            raise SystemExit("frozen v1/review changed: " + rel)
    for item in baseline["accepted"].values():
        for rel, expected in item["source"].items():
            if pin(ROOT / rel) != expected:
                raise SystemExit("accepted source changed: " + rel)
    for rel, expected in json.loads((EVIDENCE / "kernel-anchors.json").read_bytes()).items():
        if pin(ROOT / rel) != {k: expected[k] for k in ("bytes", "sha256")}:
            raise SystemExit("kernel anchor changed: " + rel)
    return baseline


def inventory():
    baseline = verify_custody()
    source = HERE / "source"
    files = {str(p.relative_to(source)): pin(p) for p in sorted(source.rglob("*")) if p.is_file()}
    if set(files) != set(baseline["copied_source"]) or len(files) != 71:
        raise SystemExit("source set changed")
    changed = {k: {"v1": v, "v2": files[k]} for k, v in baseline["copied_source"].items() if files[k] != v}
    if set(changed) != {"attach_init.h", "hgic_smac/attach_init.c", "hgic_smac/attach_status.c", "utils/fwctrl.c"}:
        raise SystemExit("outside bounded R1-R3 source changes")
    syntax = json.loads((EVIDENCE / "syntax01/receipt.json").read_bytes())
    syntax_sources = {str((source / rel).relative_to(ROOT)): expected for rel, expected in files.items()}
    if syntax["source"] != syntax_sources or not syntax["pass"] or len(syntax["commands"]) != 3:
        raise SystemExit("syntax evidence does not match final source")
    if any(row["returncode"] != 0 for row in syntax["commands"]):
        raise SystemExit("syntax command failed")
    for rel in ("LICENSE", "hgic_smac/rx.c", "hgic_smac/event.c", "hgic_smac/core.c",
                "hgic_smac/procfs.c", "utils/control_receipt.h", "hgic.h"):
        if files[rel] != baseline["copied_source"][rel]:
            raise SystemExit("immutable source changed: " + rel)
    aggregate = hashlib.sha256(json.dumps(files, sort_keys=True, separators=(",", ":")).encode()).hexdigest()
    patch = []
    for rel in sorted(changed):
        patch.extend(difflib.unified_diff((OLD / rel).read_text().splitlines(keepends=True),
                                          (source / rel).read_text().splitlines(keepends=True),
                                          fromfile="p2-v1/" + rel, tofile="p2-v2/" + rel))
    with (EVIDENCE / "R1_R2_R3_DELTA.patch").open("x") as stream:
        stream.writelines(patch)
    receipt = {"utc": datetime.datetime.now(datetime.timezone.utc).isoformat(), "argv": sys.argv,
               "python": sys.version, "inventory_tool": pin(Path(__file__)), "source": files,
               "source_sha256": aggregate, "aggregate_format": "SHA256 UTF-8 compact sorted-key JSON source map",
               "changed": changed, "unchanged_v1": 67, "accepted_sources_verified": 204,
               "frozen_v1_and_review_verified": len(baseline["frozen_custody"]),
               "syntax_receipt": pin(EVIDENCE / "syntax01/receipt.json"),
               "limitations": "R1-R3 source correction only; no behavior/source/native/product GO; P1/P3-P5 unmet"}
    dump(EVIDENCE / "source-inventory.json", receipt)
    print(json.dumps({"source_files": len(files), "changed": sorted(changed),
                      "unchanged_v1": 67, "source_sha256": aggregate}))


def seal():
    baseline = verify_custody()
    inventory_data = json.loads((EVIDENCE / "source-inventory.json").read_bytes())
    source = HERE / "source"
    actual = {str(p.relative_to(source)): pin(p) for p in sorted(source.rglob("*")) if p.is_file()}
    if actual != inventory_data["source"]:
        raise SystemExit("source changed after inventory")
    files = {}
    for path in sorted(list(HERE.rglob("*")) + list(EVIDENCE.rglob("*")) + [REPORT]):
        if path.is_symlink():
            raise SystemExit("unexpected owned symlink")
        if path.is_file() and path.name != "candidate-seal.json":
            files[str(path.relative_to(ROOT))] = pin(path)
    receipt = {"utc": datetime.datetime.now(datetime.timezone.utc).isoformat(), "argv": sys.argv,
               "cwd": str(ROOT), "python": sys.version, "files": files,
               "source_sha256": inventory_data["source_sha256"],
               "frozen_v1_and_review_verified": len(baseline["frozen_custody"]),
               "accepted_sources_verified": 204,
               "scope": "P2 v2 best-effort R1-R3 SOURCE candidate, changed-unit HOST syntax only",
               "limitations": ["independent review missing", "ordinary functional confirmation missing",
                               "P1/P3-P5 unmet", "no ARM/module build, staging, hardware, source/native/product GO"]}
    dump(EVIDENCE / "candidate-seal.json", receipt)
    print(json.dumps({"files": len(files), "seal": pin(EVIDENCE / "candidate-seal.json")}))


if __name__ == "__main__":
    if sys.flags.optimize or len(sys.argv) != 2 or sys.argv[1] not in ("inventory", "seal"):
        raise SystemExit("usage: python3 -B freeze_candidate.py inventory|seal (fresh outputs)")
    {"inventory": inventory, "seal": seal}[sys.argv[1]]()
