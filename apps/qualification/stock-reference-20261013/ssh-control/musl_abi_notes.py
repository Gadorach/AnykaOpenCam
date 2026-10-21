#!/usr/bin/env python3
"""Save two public musl ABI-source snippets and static dispatch disassembly."""
import argparse
import datetime
import hashlib
import json
from pathlib import Path
import subprocess
import urllib.request

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    receipts = []
    for name in ["atomics.s", "__set_thread_area.c"]:
        url = f"https://git.musl-libc.org/cgit/musl/plain/src/thread/arm/{name}?h=v1.2.5"
        with urllib.request.urlopen(url, timeout=30) as response:
            data = response.read(16385)
        assert len(data) < 16385
        with (args.output / name).open("xb") as output:
            output.write(data)
        receipts.append({"name": name, "url": url, "utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
                         "bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()})
    build = args.build.resolve()
    env = json.loads((build / "ENVIRONMENT.json").read_text())
    command = ["arm-buildroot-linux-musleabi-objdump", "-d", "--disassemble=__set_thread_area",
               str(build / "arm/dropbear")]
    result = subprocess.run(command, env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=True)
    with (args.output / "binary-set-thread-area.txt").open("x") as output:
        output.write(result.stdout)
    with (args.output / "RECEIPT.json").open("x") as output:
        json.dump({"sources": receipts, "command": command, "env": env,
                   "scope": "Public musl tag snippets explain linked atomics dispatch; NOT native kuser/auxv PASS"}, output, indent=2)
        output.write("\n")
    print("Retained2 public musl ABI snippets and exact candidate dispatch disassembly; no execution.")
