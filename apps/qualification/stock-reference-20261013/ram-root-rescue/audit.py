#!/usr/bin/env python3
"""Bounded offline stock-initrd audit; never builds, executes ARM, or contacts a target."""
import argparse
import datetime as dt
import hashlib
import json
import lzma
from pathlib import Path
import platform
import struct
import subprocess
import sys
import zlib

ROOT = Path(__file__).resolve().parents[4]
INPUT = ROOT / "work/lightbulb-camera/driver-re-20261008T110053Z/inputs"
SOURCE = ROOT / ("work/lightbulb-camera/deep-bringup-20261008T055753Z/"
                 "sources/ak3918ev200/vendor/linux-3.4.35")
UIMAGE = ROOT / "firmware/stock/lightbulb-camera/session-20261008T024241Z/extracted/00080000.uimage"
PINS = {
    "uimage": "e913de9bfc7bd1b783a2a2625482fe8fcc80060167cf29935429c01dfc6d8a5b",
    "raw": "a657ae44883f58fcaba702545ceaad275248bec66da5a17488e9172878653306",
    "elf": "790646be95cfde5996d6c32ec4ad0b959e1503c212db415c93d8a383cbadf800",
}
RANGES = {
    "prepare_namespace": (0xc03f4f8c, 0xc03f5144),
    "rd_load_image": (0xc03f528c, 0xc03f5860),
    "rd_load_disk": (0xc03f5860, 0xc03f591c),
    "default_rootfs": (0xc03f591c, 0xc03f5998),
    "parse_tag_initrd2": (0xc03f6f4c, 0xc03f6f78),
    "ramdisk_size": (0xc0402390, 0xc04023bc),
    "brd_alloc": (0xc019a72c, 0xc019a898),
    "squashfs_xz": (0xc00c5984, 0xc00c5da0),
}


def sha(data):
    return hashlib.sha256(data).hexdigest()


def require(condition, message):
    if not condition:
        raise ValueError(message)


def branch_target(word, pc):
    """A32 immediate B/BL (not BLX); return destination and link flag."""
    if word >> 28 == 15 or (word >> 25) & 7 != 5:
        return None
    displacement = word & 0xffffff
    if displacement & 0x800000:
        displacement -= 0x1000000
    return ((pc + 8 + displacement * 4) & 0xffffffff, bool(word & (1 << 24)))


def elf_sections(data):
    require(data[:7] == b"\x7fELF\x01\x01\x01", "Expected ELF32 little endian")
    header = struct.unpack_from("<16sHHIIIIIHHHHHH", data)
    require(header[2] == 40, "Expected ARM ELF")
    sections = [struct.unpack_from("<10I", data, header[6] + i * header[11])
                for i in range(header[12])]
    strings_section = sections[header[13]]
    names = data[strings_section[4]:strings_section[4] + strings_section[5]]
    result = {}
    for section in sections:
        name = names[section[0]:names.index(b"\0", section[0])].decode()
        result[name] = section
    return result


def audit(output):
    output = output.absolute()
    output.mkdir(parents=True, exist_ok=False)
    started = dt.datetime.now(dt.timezone.utc).isoformat()
    image = UIMAGE.read_bytes()
    raw = (INPUT / "zimage-00003868-decompressed.bin").read_bytes()
    elf_path = INPUT / "stock-kernel-sections.elf"
    elf = elf_path.read_bytes()
    for name, data in (("uimage", image), ("raw", raw), ("elf", elf)):
        require(sha(data) == PINS[name], "Pinned public kernel input changed: " + name)
    fields = struct.unpack(">7I4B32s", image[:64])
    require(fields[0] == 0x27051956 and fields[3] == len(image) - 64,
            "Unexpected legacy uImage header/length")
    header = bytearray(image[:64])
    header[4:8] = bytes(4)
    require(zlib.crc32(header) == fields[1], "Header CRC mismatch")
    require(zlib.crc32(image[64:]) == fields[6], "Payload CRC mismatch")
    require(fields[7:11] == (5, 2, 2, 0), "Expected Linux ARM kernel / compression none")
    decoder = lzma.LZMADecompressor(format=lzma.FORMAT_XZ)
    decoded = decoder.decompress(image[64 + 0x3868:], max_length=8 * 1024 * 1024)
    require(decoder.eof and decoded == raw, "Original zImage XZ does not match retained raw")
    sections = elf_sections(elf)
    for name in (".text", ".rodata", ".init.text", ".data"):
        section = sections[name]
        begin = section[3] - 0xc0008000
        require(elf[section[4]:section[4] + section[5]] == raw[begin:begin + section[5]],
                "Analysis ELF changed kernel bytes: " + name)
    # BSS in this analysis wrapper is synthetic; never use it as a runtime RAM map.
    symsec, strsec = sections[".symtab"], sections[".strtab"]
    names = elf[strsec[4]:strsec[4] + strsec[5]]
    symbols = {}
    for offset in range(symsec[4], symsec[4] + symsec[5], 16):
        name, address, _, info, _, _ = struct.unpack_from("<IIIBBH", elf, offset)
        if info & 15 == 2:
            text = names[name:names.index(b"\0", name)].decode()
            symbols[text] = address
    required = {"prepare_namespace": 0xc03f4f8c, "rd_load_image": 0xc03f528c,
                "default_rootfs": 0xc03f591c, "parse_tag_initrd2": 0xc03f6f4c,
                "brd_alloc": 0xc019a72c, "squashfs_xz_uncompress": 0xc00c5a7c}
    require(all(symbols.get(k) == v for k, v in required.items()), "Recovered symbols changed")
    absent = ["initrd_load", "handle_initrd", "populate_rootfs", "unpack_to_rootfs"]
    require(not any(k in symbols for k in absent), "Previously absent initrd functions now present")
    branches = []
    for section_name in (".text", ".init.text"):
        section = sections[section_name]
        for offset in range(0, section[5] - 3, 4):
            pc = section[3] + offset
            word = struct.unpack_from("<I", elf, section[4] + offset)[0]
            target = branch_target(word, pc)
            if target and target[1]:
                branches.append((pc, target[0]))
    rd_calls = [pc for pc, dest in branches if dest == required["rd_load_image"]]
    require(rd_calls and all(RANGES["rd_load_disk"][0] <= pc < RANGES["rd_load_disk"][1]
                             for pc in rd_calls), "Unexpected rd_load_image call path")
    prepare_calls = [{"pc": hex(pc), "target": hex(dest),
                      "symbol": next((k for k, v in symbols.items() if v == dest), None)}
                     for pc, dest in branches if RANGES["prepare_namespace"][0] <= pc < 0xc03f5114]
    # Positive instruction witnesses: bypass directly to root_wait; only floppy-major
    # (2) reaches rd_load_disk. Root_RAM0 has major 1, not major 2.
    words = {0xc03f5054: 0xe59f40d0, 0xc03f50b4: 0xe3530002,
             0xc03f50b8: 0x1a000008, 0xc03f57f0: 0x73717368,
             0xc03f5420: 0xe1c522d8, 0xc03f57f8: 0x000003ff,
             0xc03f598c: 0x00000501}
    for address, word in words.items():
        require(struct.unpack_from("<I", raw, address - 0xc0008000)[0] == word,
                "Instruction/literal witness changed: " + hex(address))
    rd_size = struct.unpack_from("<I", raw, 0xc042bd98 - 0xc0008000)[0]
    require(rd_size == 4096, "Unexpected stock RAM block device default")
    source_files = ["init/Makefile", "init/do_mounts.h", "init/do_mounts.c",
                    "init/do_mounts_initrd.c", "init/do_mounts_rd.c", "init/noinitramfs.c",
                    "arch/arm/mm/init.c", "drivers/block/brd.c", "fs/squashfs/xz_wrapper.c",
                    "COPYING"]
    sources = []
    for name in source_files:
        path = SOURCE / name
        data = path.read_bytes()
        sources.append({"path": str(path.relative_to(ROOT)), "bytes": len(data), "sha256": sha(data)})
    checks = [("init/Makefile", "obj-y                          += noinitramfs.o"),
              ("init/do_mounts.h", "static inline int initrd_load(void) { return 0; }"),
              ("init/do_mounts.c", "if (initrd_load())"),
              ("init/do_mounts_initrd.c", 'rd_load_image("/initrd.image")'),
              ("init/noinitramfs.c", "rootfs_initcall(default_rootfs);"),
              ("arch/arm/mm/init.c", "#ifdef CONFIG_BLK_DEV_INITRD")]
    for name, text in checks:
        require(text in (SOURCE / name).read_text(), "Source anchor changed: " + name)
    commands = []
    for name, (start, end) in RANGES.items():
        argv = ["llvm-objdump", "-d", "--start-address=" + hex(start),
                "--stop-address=" + hex(end), str(elf_path)]
        result = subprocess.run(argv, check=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        (output / (name + ".disassembly.txt")).write_bytes(result.stdout)
        commands.append({"argv": argv, "output": name + ".disassembly.txt"})
    version = subprocess.run(["llvm-objdump", "--version"], check=True,
                             stdout=subprocess.PIPE, text=True).stdout
    (output / "llvm-objdump-version.txt").write_text(version)
    report = {
        "unit": "ram-root-rescue01", "status": "HOLD_STOCK_KERNEL_INITRD_DISABLED",
        "started_utc": started, "finished_utc": dt.datetime.now(dt.timezone.utc).isoformat(),
        "argv": sys.argv, "python": platform.python_version(), "script_sha256": sha(Path(__file__).read_bytes()),
        "input_sha256": PINS, "source_files": sources,
        "original_uimage_bytes": len(image), "original_uimage_crc_verified": True,
        "original_zimage_xz_to_raw_equal": True, "analysis_elf_payload_equal": True,
        "analysis_bss": "synthetic; not runtime geometry", "raw_bytes": len(raw),
        "kernel_ikconfig_present": b"IKCFG_ST" in raw,
        "present_functions": {k: hex(v) for k, v in required.items()},
        "absent_functions": absent, "rd_load_image_direct_BL_callers": [hex(pc) for pc in rd_calls],
        "prepare_namespace_direct_calls": prepare_calls,
        "stock_rd_size_KiB": rd_size, "stock_ramdisk_bytes": rd_size * 1024,
        "squashfs_identification": "PRESENT: magic 0x73717368; ceil(bytes_used/1024)",
        "squashfs_xz": "PRESENT, but no external-initrd-to-ram0 population path",
        "config_inference": "CONFIG_BLK_DEV_INITRD disabled: noinitramfs default_rootfs plus absent initrd_load call path",
        "uboot_initrd": "Parent-reported native bootm help supports initrd; not sufficient for this kernel",
        "image_outputs": [], "build_attempted": False, "native_test": "NOT_RUN",
        "device_network_socket_private_access": False, "programming": "NONE",
        "commands": commands,
    }
    (output / "REPORT.json").write_text(json.dumps(report, indent=2) + "\n")
    manifest = [{"path": p.name, "bytes": p.stat().st_size, "sha256": sha(p.read_bytes())}
                for p in sorted(output.iterdir()) if p.is_file()]
    (output / "MANIFEST.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps({"status": report["status"], "evidence": str(output), "image_outputs": []}))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True, help="Fresh offline evidence directory")
    audit(parser.parse_args().output)
