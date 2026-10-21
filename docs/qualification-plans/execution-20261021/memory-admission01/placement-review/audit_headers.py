#!/usr/bin/env python3
"""Compare consumed v6 headers with public originals and exact76 UAPI text only."""
import datetime
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[5]
HERE = Path(__file__).resolve().parent
V = ROOT / "evidence/delegated/20261012-native-handoff/run34"
K = ROOT / "work/lightbulb-camera/foundation-20261011T041148Z/artifacts/native-usb76-source01/linux/arch/arm/mach-ak3918ev200"


def seal(path):
    data = path.read_bytes()
    return {"path": str(path.relative_to(ROOT)), "bytes": len(data),
            "sha256": hashlib.sha256(data).hexdigest()}


def main():
    out = HERE / "header-audit.json"
    if out.exists():
        raise SystemExit("refuse overwrite")
    manifest = json.loads((V / "manifest.json").read_text())
    names = {
        "capture-receipt.h": "bringup/qc07f/userland/capture-receipt.h",
        "capture-retirement-uapi.h": "bringup/qc07f/userland/capture-retirement-uapi.h",
        "isp-stream.h": "bringup/qc07f/variants/security75/mach-ak3918ev200/isp-stream.h",
        "qc07f_hwenc.h": "bringup/qc07f/variants/security75/mach-ak3918ev200/qc07f_hwenc.h",
    }
    checks = []
    for name, source in names.items():
        # Match the source suffix: original manifest contains sealed absolute paths.
        pins = [entry["sha256"] for path, entry in manifest["source_inputs"].items()
                if path.endswith("/" + source)]
        if len(pins) != 1:
            raise ValueError("ambiguous/missing consumed header: " + source)
        for path in (ROOT / source, V / "inputs" / name):
            item = seal(path)
            item.update(expected_sha256=pins[0], matches=item["sha256"] == pins[0])
            checks.append(item)
        if name != "capture-receipt.h":
            item = seal(K / name)
            item.update(expected_sha256=pins[0], matches=item["sha256"] == pins[0])
            checks.append(item)
    result = {
        "utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "python": sys.version,
        "command": "python3 -B " + str(Path(__file__).resolve().relative_to(ROOT)),
        "script": seal(Path(__file__).resolve()),
        "manifest": seal(V / "manifest.json"),
        "scope": "public source/header text and existing manifest only; no tests/builds/binaries/target",
        "comparisons": checks,
        "comparison_count": len(checks),
        "all_match": all(item["matches"] for item in checks),
    }
    with out.open("x") as stream:
        json.dump(result, stream, indent=2)
        stream.write("\n")
    print(json.dumps({"output": str(out.relative_to(ROOT)), "comparisons": len(checks),
                      "all_match": result["all_match"]}))
    if not result["all_match"]:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
