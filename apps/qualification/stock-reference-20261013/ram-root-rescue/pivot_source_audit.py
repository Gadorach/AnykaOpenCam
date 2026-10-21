#!/usr/bin/env python3
"""Source/stock-binary pivot reference audit only. No target or ARM execution."""
import argparse
import datetime as dt
import hashlib
import json
from pathlib import Path
import struct
import subprocess

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
INPUT = ROOT / "work/lightbulb-camera/driver-re-20261008T110053Z/inputs"
SOURCE = ROOT / ("work/lightbulb-camera/deep-bringup-20261008T055753Z/"
                 "sources/ak3918ev200/vendor/linux-3.4.35")


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main(output):
    output.mkdir(parents=True, exist_ok=False)
    elf = INPUT / "stock-kernel-sections.elf"
    raw_path = INPUT / "zimage-00003868-decompressed.bin"
    if digest(elf) != "790646be95cfde5996d6c32ec4ad0b959e1503c212db415c93d8a383cbadf800":
        raise ValueError("Kernel ELF pin changed")
    if digest(raw_path) != "a657ae44883f58fcaba702545ceaad275248bec66da5a17488e9172878653306":
        raise ValueError("Kernel raw pin changed")
    raw = raw_path.read_bytes()
    witnesses = {0xc008ea20: 0xeb002260, 0xc0097408: 0xe1510002,
                 0xc0097418: 0xe1510002, 0xc009743c: 0xe1500001,
                 0xc009744c: 0xe1500001}
    for pc, instruction in witnesses.items():
        if struct.unpack_from("<I", raw, pc - 0xc0008000)[0] != instruction:
            raise ValueError("Pivot/reference comparison witness changed")
    commands = []
    for name, start, end in [("sys_pivot_root", 0xc008e83c, 0xc008ea80),
                             ("chroot_fs_refs", 0xc00973a8, 0xc00974f0)]:
        command = ["llvm-objdump", "-d", "--start-address=" + hex(start),
                   "--stop-address=" + hex(end), str(elf)]
        data = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=True).stdout
        (output / (name + ".disassembly.txt")).write_bytes(data)
        commands.append(command)
    excerpts = []
    sources = []
    for name, start, end in [("fs/namespace.c", 2445, 2552), ("fs/fs_struct.c", 59, 96)]:
        path = SOURCE / name
        lines = path.read_text().splitlines()
        sources.append({"path": str(path.relative_to(ROOT)), "sha256": digest(path)})
        excerpts.append({"path": str(path.relative_to(ROOT)), "start_line": start,
                         "end_line": end, "lines": lines[start - 1:end]})
    (output / "source-excerpts.json").write_text(json.dumps(excerpts, indent=2) + "\n")
    report = {"utc": dt.datetime.now(dt.timezone.utc).isoformat(), "unit": "ram-root-pivot01",
              "script_sha256": digest(Path(__file__)), "kernel_elf_sha256": digest(elf),
              "kernel_raw_sha256": digest(raw_path), "commands": commands, "sources": sources,
              "witnesses": {hex(k): hex(v) for k, v in witnesses.items()},
              "finding": "Stock sys_pivot_root calls chroot_fs_refs; both mount and dentry exact-match for root/cwd; no MM/FD rewrite",
              "native": "NOT_RUN", "device_network_socket_private_access": False,
              "host_mount_or_pivot": False}
    (output / "REPORT.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"output": str(output), "finding": report["finding"]}))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    main(parser.parse_args().output)
