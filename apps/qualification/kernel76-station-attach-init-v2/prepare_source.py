#!/usr/bin/env python3
"""Exclusive byte copy of frozen P2 v1 source and public provenance verification."""
import datetime
import hashlib
import json
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
DOC = ROOT / "docs/qualification-plans/kernel76-transport-20261021"
EVIDENCE = DOC / "station-attach-init-p2-v2-source01"
OLD = ROOT / "apps/qualification/kernel76-station-attach-init/source"
KERNEL = ROOT / "work/lightbulb-camera/foundation-20261011T041148Z/artifacts/native-usb76-source01/linux"
V1_SEAL = DOC / "station-attach-init-p2-source01/candidate-seal.json"
REVIEW_SEAL = DOC / "station-attach-init-p2-review01/review-seal.json"


def pin(path):
    content = path.read_bytes()
    return {"bytes": len(content), "sha256": hashlib.sha256(content).hexdigest()}


def main():
    if sys.flags.optimize or sys.argv[1:]:
        raise SystemExit("no optimized Python or arguments")
    if pin(V1_SEAL)["sha256"] != "c54ff57f63bec1180199a97321fbf1ea38e9b448edf0969c9135953c565b1919":
        raise SystemExit("v1 seal mismatch")
    custody = {}
    for manifest_path in (V1_SEAL, REVIEW_SEAL):
        custody[str(manifest_path.relative_to(ROOT))] = pin(manifest_path)
        manifest = json.loads(manifest_path.read_bytes())
        for rel, expected in manifest["files"].items():
            path = ROOT / rel
            if path.is_symlink() or pin(path) != expected:
                raise SystemExit("frozen public input changed: " + rel)
            custody[rel] = expected
    v1 = json.loads(V1_SEAL.read_bytes())
    prefix = str(OLD.relative_to(ROOT)) + "/"
    source = {k[len(prefix):]: v for k, v in v1["files"].items() if k.startswith(prefix)}
    if len(source) != 71 or set(source) != {str(p.relative_to(OLD)) for p in OLD.rglob("*") if p.is_file()}:
        raise SystemExit("unexpected v1 source set")
    accepted = json.loads((DOC / "station-attach-init-p2-source01/copy-baseline.json").read_bytes())["accepted"]
    for item in accepted.values():
        for rel, expected in item["source"].items():
            if pin(ROOT / rel) != expected:
                raise SystemExit("accepted source changed: " + rel)
    expected_kernel = {
        "net/mac80211/main.c": "38047a679e48375f6d8cae18b6b2746d89dac38cde4ee62602ed3e56ca957e74",
        "include/net/mac80211.h": "43b4be700a0aa3d5b4e5ac3483db02d2506ba637e2686343b053a1d05949067f",
        "net/core/dev.c": "83a1a83bdf7a979b0aa27170272748c5a71070ad52227243a8f391008ebed80c",
        "net/mac80211/iface.c": "461e696280b214723762d85aad60641d9792f57d72a8f1f3c08c90f724aaec6f",
    }
    ranges = {"net/mac80211/main.c": [(803, 832), (1595, 1636)],
              "include/net/mac80211.h": [(4763, 4777)],
              "net/core/dev.c": [(11401, 11410), (11422, 11430), (12280, 12290), (12328, 12376)],
              "net/mac80211/iface.c": [(2376, 2390), (2393, 2407), (2422, 2427)]}
    anchors = {}
    for rel, expected in expected_kernel.items():
        path = KERNEL / rel
        metadata = pin(path)
        if metadata["sha256"] != expected:
            raise SystemExit("review kernel anchor mismatch: " + rel)
        lines = path.read_bytes().splitlines(keepends=True)
        excerpts = []
        for start, end in ranges[rel]:
            content = b"".join(lines[start - 1:end])
            excerpts.append({"start_line": start, "end_line": end, "text": content.decode(),
                             "sha256": hashlib.sha256(content).hexdigest()})
        anchors[str(path.relative_to(ROOT))] = {**metadata, "excerpts": excerpts}
    EVIDENCE.mkdir(exist_ok=False)
    (HERE / "source").mkdir(exist_ok=False)
    for rel, expected in sorted(source.items()):
        path = HERE / "source" / rel
        path.parent.mkdir(parents=True, exist_ok=True)
        with path.open("xb") as stream:
            stream.write((OLD / rel).read_bytes())
        if pin(path) != expected:
            raise SystemExit("copy mismatch: " + rel)
    receipt = {"utc": datetime.datetime.now(datetime.timezone.utc).isoformat(), "argv": sys.argv,
               "cwd": str(ROOT), "python": sys.version, "preparer": pin(Path(__file__)),
               "copied_source": source, "copied_count": 71, "frozen_custody": custody,
               "accepted": accepted, "accepted_source_count": 204,
               "scope": "v1 public source byte copy, R1-R3 correction only; no P1 read/adoption or target IO"}
    for name, data in (("copy-baseline.json", receipt), ("kernel-anchors.json", anchors)):
        with (EVIDENCE / name).open("x") as stream:
            json.dump(data, stream, indent=2)
            stream.write("\n")
    print("Copied 71 v1 source files byte-exact; verified frozen v1/review and 204 accepted source pins")


if __name__ == "__main__":
    main()
