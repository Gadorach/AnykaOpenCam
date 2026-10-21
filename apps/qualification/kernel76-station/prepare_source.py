#!/usr/bin/env python3
"""Copy only the 67 pinned public source files; refuse existing outputs."""
import datetime
import hashlib
import json
from pathlib import Path
import sys

if sys.flags.optimize or sys.argv[1:]:
    raise SystemExit("no optimized Python or arguments")
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
MANIFEST = ROOT / "work/lightbulb-camera/foundation-20261011T041148Z/analysis/native-usb76-build01/prepare76-manifest.json"
ORIGINAL = ROOT / "work/lightbulb-camera/foundation-20261011T041148Z/artifacts/native-usb76-source01/station76-final"
EVIDENCE = ROOT / "docs/qualification-plans/kernel76-transport-20261021/station-control-a1-host01"
SOURCE = HERE / "source"


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    if SOURCE.exists() or EVIDENCE.exists():
        raise SystemExit("refuse existing source/evidence destination")
    raw = MANIFEST.read_bytes()
    if sha(raw) != "686ad5da743ea3396962606f6349cf07f6d6c7454c84d572fbc5c4418bdd581d":
        raise SystemExit("preparer pin mismatch")
    pins = json.loads(raw)["paired_radio_source75_pins"]
    if len(pins) != 67:
        raise SystemExit("expected exact 67 source pins")
    files = []
    for name, pin in sorted(pins.items()):
        rel = Path(name).relative_to("station75")
        if rel.is_absolute() or ".." in rel.parts or rel.suffix not in (".c", ".h", ".md", ""):
            raise SystemExit("not an admitted source path")
        src = ORIGINAL / rel
        if src.is_symlink() or not src.is_file():
            raise SystemExit("not a regular source file")
        data = src.read_bytes()
        if sha(data) != pin or data.startswith(b"\x7fELF") or b"\0" in data:
            raise SystemExit("source mismatch or binary")
        files.append((rel, data, pin))
    SOURCE.mkdir()
    EVIDENCE.mkdir()
    entries = []
    for rel, data, pin in files:
        dest = SOURCE / rel
        dest.parent.mkdir(parents=True, exist_ok=True)
        with dest.open("xb") as output:
            output.write(data)
        entries.append({"path": str(rel), "source_path": str((ORIGINAL / rel).relative_to(ROOT)),
                        "source_extent": {"offset": 0, "bytes": len(data)},
                        "original_sha256": pin, "copied_sha256": sha(dest.read_bytes())})
    receipt = {"utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
               "scope": "PUBLIC_SOURCE_COPY_ONLY_NO_BUILD_NO_TARGET", "argv": sys.argv,
               "python": sys.version, "script_sha256": sha(Path(__file__).read_bytes()),
               "prepare_manifest_sha256": sha(raw), "count": len(entries), "files": entries}
    with (EVIDENCE / "copy-baseline.json").open("x") as output:
        json.dump(receipt, output, indent=2)
        output.write("\n")
    print("67 pinned public source files copied byte-exact; no build")


if __name__ == "__main__":
    main()
