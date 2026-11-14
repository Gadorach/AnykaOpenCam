#!/usr/bin/env python3
"""Read-only, pinned-stock-dump carving and offline extraction; never installation.

Consolidates the 2026-10-08 session basic_analysis.py/basic_details.py and
extract_romfs.py/extract_littlefs.py methods. See README.md for provenance/limits.
"""
import argparse
import hashlib
import json
import lzma
import os
from pathlib import Path, PurePosixPath
import shutil
import stat
import struct
import subprocess
import sys
import zlib

from profiles import EXPECTED_SIZE, PROFILES
from romfs import unpack_romfs

LIMIT = 32 * 1024 * 1024
BASE = Path(__file__).resolve().parent.parent


def digest(data):
    return hashlib.sha256(data).hexdigest()


def private_mkdir(path):
    # pathlib's parents=True uses the default mode for intermediate directories.
    # Create each missing component explicitly instead; do not chmod existing dirs.
    if not path.exists():
        private_mkdir(path.parent)
        path.mkdir(mode=0o700)


class Output:
    def __init__(self, root):
        self.root = root
        self.artifacts = []

    def save(self, name, data, source, offset, length, operation):
        path = self.root / name
        private_mkdir(path.parent)
        with path.open("xb") as stream:
            stream.write(data)
        path.chmod(0o444)
        self.artifacts.append({
            "path": name, "bytes": len(data), "sha256": digest(data),
            "source": source, "source_offset": offset, "source_length": length,
            "operation": operation,
        })
        return path

    def report(self, name, obj):
        return self.save(name, (json.dumps(obj, indent=2) + "\n").encode(),
                         "generated metadata", None, None, "audit report")

    def directory(self, name):
        private_mkdir(self.root / name)


def fresh_directory(path):
    # Do not resolve away a symlink before checking it. No existing destination,
    # even an empty directory, is accepted. The new tree is owner-only.
    path = Path(os.path.abspath(path))
    current = Path(path.anchor)
    for component in path.parts[1:]:
        current /= component
        if current.is_symlink():
            raise ValueError("Output path contains a symlink")
    if path.exists():
        raise ValueError("Output destination already exists; choose a fresh path")
    private_mkdir(path.parent)
    path.mkdir(mode=0o700)
    return path


def read_original(path, profile):
    fd = os.open(path, os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK)
    with os.fdopen(fd, "rb") as stream:
        info = os.fstat(stream.fileno())
        if not stat.S_ISREG(info.st_mode) or info.st_size != EXPECTED_SIZE:
            raise ValueError("Input must be a regular file of exactly 8388608 bytes")
        data = stream.read(EXPECTED_SIZE + 1)
    if len(data) != EXPECTED_SIZE or digest(data) != profile["sha256"]:
        raise ValueError("Original dump size/SHA-256 does not match selected profile")
    return data


def carve_uimage(data, name, start, end, out, xz_offset=None):
    header = bytearray(data[start:start + 64])
    magic, hcrc, _, size, _, _, dcrc = struct.unpack_from(">7I", header)
    header[4:8] = b"\0" * 4
    if magic != 0x27051956 or zlib.crc32(header) & 0xffffffff != hcrc:
        raise ValueError("uImage header CRC/magic failure")
    stop = start + 64 + size
    if stop > end or header[31] != 0:
        raise ValueError("uImage extent/compression disagrees with profile")
    payload = data[start + 64:stop]
    if zlib.crc32(payload) & 0xffffffff != dcrc:
        raise ValueError("uImage payload CRC failure")
    prefix = f"containers/{start:08x}-{name}"
    out.save(prefix + ".uimage", data[start:stop], "original", start, stop-start,
             "CRC-valid uImage including original header")
    suffix = ".zimage" if xz_offset is not None else "-payload.bin"
    out.save(prefix + suffix, payload, "original", start+64, size,
             "uncompressed uImage payload; not necessarily an uncompressed kernel")
    if xz_offset is not None:
        if struct.unpack_from("<I", payload, 36)[0] != 0x016f2818:
            raise ValueError("ARM zImage magic failure")
        codec = lzma.LZMADecompressor(format=lzma.FORMAT_XZ, memlimit=128*1024*1024)
        decoded = codec.decompress(payload[xz_offset:], max_length=LIMIT+1)
        if len(decoded) > LIMIT or not codec.eof:
            raise ValueError("Embedded kernel XZ is truncated or exceeds 32 MiB")
        consumed = len(payload) - xz_offset - len(codec.unused_data)
        offset = start + 64 + xz_offset
        out.save(f"containers/{offset:08x}-kernel.xz", payload[xz_offset:xz_offset+consumed],
                 "original", offset, consumed, "exact embedded XZ stream, excluding zImage trailer")
        out.save("decoded/linux-kernel.bin", decoded, "original", offset, consumed,
                 "bounded XZ decoding inside ARM zImage; not an ELF or boot recipe")


def run_unsquashfs(argv):
    # Only the host filesystem parser runs. Never invoke a vendor file or shell.
    result = subprocess.run(["unsquashfs", *argv], stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, check=False,
                            env={**os.environ, "LC_ALL": "C"})
    if result.returncode:
        raise ValueError("Host unsquashfs failed (vendor content/error text not printed)")
    return result.stdout


def unpack_squashfs(image, label, source_name, out):
    # Adapted from the session extract_squashfs.py path audit. Instead of bulk
    # extraction, read each audited regular file via -cat; never create links,
    # device nodes, original permissions or an executable vendor program.
    listing = run_unsquashfs(["-lln", "-UTC", str(image)]).decode("utf-8", "strict")
    entries, links, names = [], set(), set()
    import re
    pattern = re.compile(r"^(\S{10})\s+\d+/\d+\s+(.+?)\s+\d{4}-\d\d-\d\d\s+\d\d:\d\d\s+(squashfs-root(?:/.*)?)$")
    for line in listing.splitlines():
        match = pattern.fullmatch(line)
        if not match:
            raise ValueError("Unsupported unsquashfs listing format; no tree extraction")
        mode, size, text = match.groups()
        target = None
        if mode[0] == "l":
            text, sep, target = text.partition(" -> ")
            if not sep:
                raise ValueError("Unrecognized symlink listing")
        path = PurePosixPath(text)
        if path.is_absolute() or ".." in path.parts or path.parts[0] != "squashfs-root":
            raise ValueError("Unsafe SquashFS path")
        relative = str(path.relative_to("squashfs-root"))
        if relative in names or "\x00" in relative:
            raise ValueError("Duplicate/unsafe SquashFS name")
        names.add(relative)
        entry = {"path": relative, "original_mode": mode, "type": mode[0]}
        if mode[0] in "-dl":
            entry["bytes"] = int(size)
        if mode[0] == "l":
            links.add(path)
            entry.update(target=target, action="recorded only; symlink not created")
        elif mode[0] not in "-d":
            entry["action"] = "special node recorded only; not created"
        entries.append(entry)
    if not entries:
        raise ValueError("Empty SquashFS listing")
    for name in names:
        if any(p in links for p in (PurePosixPath("squashfs-root") / name).parents):
            raise ValueError("SquashFS member descends through a symlink")
    out.report(f"audits/{label}-squashfs.json", entries)
    for entry in entries:
        relative = entry["path"]
        if entry["type"] == "d":
            out.directory(f"filesystems/{label}/{relative}")
        elif entry["type"] == "-":
            if entry["bytes"] > LIMIT:
                raise ValueError("SquashFS regular file exceeds extraction bound")
            # -no-wildcards makes image member names literal, not selectors.
            decoded = run_unsquashfs(["-no-wildcards", "-cat", str(image), relative])
            if len(decoded) != entry["bytes"]:
                raise ValueError("SquashFS regular-file length disagrees with listing")
            out.save(f"filesystems/{label}/{relative}", decoded, source_name, None, None,
                     "host unsquashfs -cat of audited regular file; " + relative)


def unpack_littlefs(data, out):
    # Same immutable-copy / denied callbacks as the original session extractor.
    from littlefs import LittleFS, UserContext

    class ReadOnlyContext(UserContext):
        def __init__(self, payload):
            self.buffer = bytes(payload)
            self.writes = 0

        def read(self, cfg, block, off, size):
            start = block * cfg.block_size + off
            if start < 0 or size < 0 or start+size > len(self.buffer):
                raise ValueError("LittleFS read outside CONFIG")
            return bytearray(self.buffer[start:start+size])

        def prog(self, *args):
            self.writes += 1
            return -5

        def erase(self, *args):
            self.writes += 1
            return -5

    context = ReadOnlyContext(data)
    fs = LittleFS(context=context, mount=False, block_size=4096, block_count=128,
                  read_size=16, prog_size=16, cache_size=256, lookahead_size=16)
    fs.mount()  # No auto-format on failure.
    entries = []

    def scan(directory, depth=0):
        if depth > 32 or len(entries) > 10000:
            raise ValueError("LittleFS traversal limit")
        for item in fs.scandir("/" + directory):
            if item.name in ("", ".", "..") or "/" in item.name:
                raise ValueError("Unsafe LittleFS name")
            name = str(PurePosixPath(directory) / item.name)
            if item.type not in (1, 2) or item.size > len(data):
                raise ValueError("Unsupported LittleFS entry")
            entries.append({"path": name, "type_code": item.type, "bytes": item.size})
            if item.type == 2:
                scan(name, depth+1)

    try:
        scan("")
        out.report("audits/littlefs.json", entries)
        for entry in entries:
            if entry["type_code"] == 2:
                out.directory("filesystems/config/" + entry["path"])
            else:
                with fs.open("/"+entry["path"], "rb") as stream:
                    decoded = stream.read(entry["bytes"]+1)
                if len(decoded) != entry["bytes"]:
                    raise ValueError("LittleFS length mismatch")
                out.save("filesystems/config/"+entry["path"], decoded,
                         "partitions/0077e000-CONFIG.bin", None, None, "read-only LittleFS regular file")
    finally:
        fs.unmount()
    if context.writes or context.buffer != data:
        raise ValueError("LittleFS attempted a write")
    out.report("audits/littlefs-readonly.json", {"write_callbacks_attempted": context.writes,
                                                "immutable_buffer_unchanged": True})


def main(default_profile=None):
    parser = argparse.ArgumentParser(description=__doc__)
    if default_profile is None:
        parser.add_argument("profile", choices=sorted(PROFILES))
    parser.add_argument("--input", type=Path, help="original dump; default firmware/original/PROFILE.bin")
    parser.add_argument("--output", type=Path, help="NEW directory; default firmware/extracted/PROFILE")
    parser.add_argument("--carve-only", action="store_true", help="skip filesystem trees; still decode kernel XZ")
    parser.add_argument("--littlefs", action="store_true", help="also unpack KM01 CONFIG; requires littlefs-python")
    args = parser.parse_args()
    selected = default_profile or args.profile
    profile = PROFILES[selected]
    if args.littlefs and ("littlefs" not in profile or args.carve_only):
        parser.error("--littlefs requires KM01 and cannot be combined with --carve-only")
    data = read_original(args.input or BASE / "original" / (selected+".bin"), profile)
    if not args.carve_only and profile.get("squashfs") and not shutil.which("unsquashfs"):
        parser.error("host unsquashfs is required for Linux file extraction; use --carve-only to omit it")
    if args.littlefs:
        import littlefs  # Fail before creating an output directory if unavailable.
    out = Output(fresh_directory(args.output or BASE / "extracted" / selected))
    for start, size in profile.get("environments", []):
        env = data[start:start+size]
        if struct.unpack_from("<I", env)[0] != zlib.crc32(env[4:]) & 0xffffffff:
            raise ValueError("Saved environment CRC failure")
    for name, start, end in profile["partitions"]:
        out.save(f"partitions/{start:08x}-{name}.bin", data[start:end], "original", start, end-start,
                 "full candidate partition; " + profile["basis"])
    for name, start, end in profile["uimages"]:
        carve_uimage(data, name, start, end, out, profile.get("xz_in_payload"))
    for start in profile.get("dtbs", []):
        magic, size = struct.unpack_from(">2I", data, start)
        if magic != 0xd00dfeed or size < 40 or start+size > len(data):
            raise ValueError("Invalid DTB extent")
        out.save(f"containers/{start:08x}-board.dtb", data[start:start+size], "original", start, size,
                 "DTB declared extent (no dtc execution)")
    if "romfs" in profile:
        start, end = profile["romfs"]
        unpack_romfs(data[start:end], start, out, unpack=not args.carve_only)
    for label, start, end in profile.get("squashfs", []):
        if data[start:start+4] != b"hsqs" or struct.unpack_from("<H", data, start+28)[0] != 4:
            raise ValueError("Expected SquashFS v4 header")
        size = struct.unpack_from("<Q", data, start+40)[0]
        if not 96 <= size <= end-start:
            raise ValueError("SquashFS declared extent outside partition")
        name = f"containers/{start:08x}-{label}.squashfs"
        image = out.save(name, data[start:start+size], "original", start, size,
                         "SquashFS bytes_used extent, excluding partition tail")
        if not args.carve_only:
            unpack_squashfs(image, label, name, out)
    if args.littlefs:
        start, end = profile["littlefs"]
        unpack_littlefs(data[start:end], out)
    out.report("manifest.json", {
        "profile": selected, "identity": profile["identity"],
        "original_bytes": len(data), "original_sha256": digest(data),
        "map_basis": profile["basis"], "carve_only": args.carve_only,
        "littlefs_unpacked": args.littlefs,
        "limitations": "No JFFS2 unpacking; no symlink/device-node creation; no target execution or installation",
        "artifacts": list(out.artifacts),
    })
    print(f"Completed {selected}: {len(out.artifacts)} artifacts in {out.root}")


def cli(default_profile=None):
    try:
        main(default_profile)
    except (ValueError, OSError, ImportError, lzma.LZMAError) as exc:
        # Keep partial outputs for inspection; never silently repair or retry.
        print(f"Extraction stopped: {exc}", file=sys.stderr)
        raise SystemExit(1)


if __name__ == "__main__":
    cli()
