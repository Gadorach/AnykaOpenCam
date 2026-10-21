#!/usr/bin/env python3
"""Retain small public static-runtime notices; no package/source-tree install."""
import argparse
import datetime
import hashlib
import json
from pathlib import Path
import urllib.request

URLS = {
    "musl-arm-memcpy.S": "https://git.musl-libc.org/cgit/musl/plain/src/string/arm/memcpy.S?h=v1.2.5",
    "gcc14-COPYING.RUNTIME": "https://raw.githubusercontent.com/gcc-mirror/gcc/releases/gcc-14.3.0/COPYING.RUNTIME",
    "gcc14-COPYING3": "https://raw.githubusercontent.com/gcc-mirror/gcc/releases/gcc-14.3.0/COPYING3",
}

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    receipts = []
    for name, url in URLS.items():
        with urllib.request.urlopen(url, timeout=30) as response:
            data = response.read(131073)
        assert len(data) <= 131072
        with (args.output / name).open("xb") as output:
            output.write(data)
        receipts.append({"file": name, "url": url, "utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
                         "bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()})
    with (args.output / "ACQUISITION.json").open("x") as output:
        json.dump(receipts, output, indent=2)
        output.write("\n")
    print("Retained3 small public runtime licensing/source-notice files; no execution.")
