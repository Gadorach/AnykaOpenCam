#!/usr/bin/env python3
"""Inspect an offline firmware file without extracting or executing its contents."""

import argparse
from collections import Counter
from datetime import datetime, timezone
import hashlib
import json
import math
from pathlib import Path
import re
import struct
import sys
import zlib


UIMAGE = struct.Struct(">7I4B32s")
FDT_HEADER = struct.Struct(">10I")
SQUASHFS = struct.Struct("<5I6H8Q")
MAX_FDT_TOKENS = 250000
MAX_RESERVATIONS = 4096
UIMAGE_COMP = {0: "none", 1: "gzip", 2: "bzip2", 3: "lzma", 4: "lzo", 5: "lz4", 6: "zstd"}
SQUASH_COMP = {1: "gzip", 2: "lzma", 3: "lzo", 4: "xz", 5: "lz4", 6: "zstd"}


def positive_int(value):
    try:
        result = int(value, 0)
    except ValueError as exc:
        raise argparse.ArgumentTypeError("use a decimal or 0x-prefixed integer") from exc
    if result <= 0:
        raise argparse.ArgumentTypeError("must be positive")
    return result


def offsets(data, magic, limit):
    positions = []
    start = 0
    while True:
        position = data.find(magic, start)
        if position < 0:
            return positions, False
        if len(positions) >= limit:
            return positions, True
        positions.append(position)
        start = position + 1


def inspect_uimage(data, offset):
    result = {"offset": offset, "offset_hex": hex(offset), "valid_checks": False, "errors": []}
    if offset + UIMAGE.size > len(data):
        result["errors"].append("truncated_header")
        return result
    magic, hcrc, stamp, size, load, entry, dcrc, os_id, arch, kind, comp, name = UIMAGE.unpack_from(data, offset)
    header = bytearray(data[offset:offset + UIMAGE.size])
    header[4:8] = b"\0" * 4
    computed_header_crc = zlib.crc32(header) & 0xffffffff
    payload_start, payload_end = offset + UIMAGE.size, offset + UIMAGE.size + size
    bounds_ok = payload_end <= len(data)
    computed_data_crc = zlib.crc32(memoryview(data)[payload_start:payload_end]) & 0xffffffff if bounds_ok else None
    result.update({
        "name": name.split(b"\0", 1)[0].decode("ascii", errors="replace"),
        "timestamp_unix": stamp, "payload_offset": payload_start, "payload_size": size,
        "declared_end_offset": payload_end, "load_address": hex(load), "entry_address": hex(entry),
        "os_code": os_id, "os_name": "Linux" if os_id == 5 else "other_or_unknown",
        "architecture_code": arch, "architecture_name": {2: "ARM", 3: "x86", 5: "MIPS", 22: "AArch64"}.get(arch, "other_or_unknown"),
        "image_type_code": kind, "image_type_name": {1: "standalone", 2: "kernel", 3: "ramdisk", 4: "multi", 5: "firmware", 6: "script", 8: "flat_dt"}.get(kind, "other_or_unknown"),
        "compression_code": comp, "compression_name": UIMAGE_COMP.get(comp, "unknown"),
        "header_crc_stored": f"{hcrc:08x}", "header_crc_computed": f"{computed_header_crc:08x}",
        "header_crc_valid": hcrc == computed_header_crc, "payload_bounds_valid": bounds_ok,
        "payload_crc_stored": f"{dcrc:08x}",
        "payload_crc_computed": None if computed_data_crc is None else f"{computed_data_crc:08x}",
        "payload_crc_valid": None if not bounds_ok else dcrc == computed_data_crc,
    })
    if hcrc != computed_header_crc:
        result["errors"].append("header_crc_mismatch")
    if not bounds_ok:
        result["errors"].append("payload_extends_beyond_input")
    elif dcrc != computed_data_crc:
        result["errors"].append("payload_crc_mismatch")
    result["valid_checks"] = not result["errors"]
    result["note"] = "Checks cover this legacy header and payload CRC only; they do not prove authenticity, board compatibility, or bootability."
    return result


def align4(value):
    return (value + 3) & ~3


def inspect_fdt(data, offset):
    result = {"offset": offset, "offset_hex": hex(offset), "valid_checks": False, "errors": []}
    if offset + 28 > len(data):
        result["errors"].append("truncated_base_header")
        return result
    _, total, off_struct, off_strings, off_resv, version, last = struct.unpack_from(">7I", data, offset)
    result.update({"total_size": total, "version": version, "last_compatible_version": last,
                   "structure_offset_relative": off_struct, "strings_offset_relative": off_strings,
                   "reservation_offset_relative": off_resv})
    # Version 17+ is the documented layout. Older layouts are retained as candidates.
    if version < 17 or last > 17 or last > version:
        result["errors"].append("unsupported_or_inconsistent_version_for_this_scanner")
        return result
    if offset + FDT_HEADER.size > len(data):
        result["errors"].append("truncated_v17_header")
        return result
    fields = FDT_HEADER.unpack_from(data, offset)
    cpu, strings_size, struct_size = fields[7:]
    result.update({"boot_cpu_id": cpu, "strings_size": strings_size, "structure_size": struct_size})
    errors = result["errors"]
    if total < 40 or offset + total > len(data):
        errors.append("total_size_out_of_bounds")
    if off_struct < 40 or off_struct % 4 or struct_size < 4 or off_struct + struct_size > total:
        errors.append("structure_block_out_of_bounds_or_unaligned")
    if off_strings < 40 or off_strings + strings_size > total:
        errors.append("strings_block_out_of_bounds")
    if off_resv < 40 or off_resv % 8 or off_resv + 16 > total:
        errors.append("reservation_block_out_of_bounds_or_unaligned")
    if max(off_struct, off_strings) < min(off_struct + struct_size, off_strings + strings_size):
        errors.append("structure_and_strings_overlap")
    if errors:
        return result
    blob = memoryview(data)[offset:offset + total]
    resv_pos = off_resv
    resv_limit = min([value for value in (off_struct, off_strings, total) if value > off_resv])
    reservation_count = 0
    terminated = False
    for _ in range(MAX_RESERVATIONS):
        if resv_pos + 16 > resv_limit:
            break
        address, size = struct.unpack_from(">QQ", blob, resv_pos)
        resv_pos += 16
        if address == size == 0:
            terminated = True
            break
        reservation_count += 1
    if not terminated:
        errors.append("reservation_terminator_missing_or_scan_limit_reached")
        return result
    for block_start, block_end in ((off_struct, off_struct + struct_size), (off_strings, off_strings + strings_size)):
        if max(off_resv, block_start) < min(resv_pos, block_end):
            errors.append("reservation_block_overlaps_other_block")
    if errors:
        return result
    result["memory_reservations"] = reservation_count
    strings = data[offset + off_strings:offset + off_strings + strings_size]
    position, end = off_struct, off_struct + struct_size
    stack, samples = [], {}
    nodes, properties, tokens = 0, 0, 0
    root_seen, root_closed, found_end = False, False, False
    while position + 4 <= end and tokens < MAX_FDT_TOKENS:
        token = struct.unpack_from(">I", blob, position)[0]
        position += 4
        tokens += 1
        if token == 1:  # FDT_BEGIN_NODE
            if root_closed:
                errors.append("second_root_node")
                break
            null = data.find(b"\0", offset + position, offset + end)
            if null < 0:
                errors.append("unterminated_node_name")
                break
            name = data[offset + position:null].decode("ascii", errors="replace")
            position = align4(null - offset + 1)
            if position > end:
                errors.append("node_name_padding_out_of_bounds")
                break
            stack.append(name)
            root_seen = True
            nodes += 1
        elif token == 2:  # FDT_END_NODE
            if not stack:
                errors.append("unbalanced_end_node")
                break
            stack.pop()
            root_closed = not stack
        elif token == 3:  # FDT_PROP
            if not stack or position + 8 > end:
                errors.append("property_outside_node_or_truncated")
                break
            length, nameoff = struct.unpack_from(">II", blob, position)
            position += 8
            value_end = position + length
            if align4(value_end) > end or nameoff >= len(strings):
                errors.append("property_value_or_name_offset_out_of_bounds")
                break
            null = strings.find(b"\0", nameoff)
            if null < 0:
                errors.append("unterminated_property_name")
                break
            name = strings[nameoff:null].decode("ascii", errors="replace")
            node_path = "/" + "/".join(stack[1:])
            if (node_path == "/" and name in ("model", "compatible")) or (node_path == "/chosen" and name == "bootargs"):
                value = bytes(blob[position:min(value_end, position + 4096)])
                samples[f"{node_path}:{name}"] = {
                    "strings": [item.decode("ascii", errors="replace") for item in value.rstrip(b"\0").split(b"\0")],
                    "truncated_for_report": length > 4096,
                }
            properties += 1
            position = align4(value_end)
        elif token == 4:  # FDT_NOP
            pass
        elif token == 9:  # FDT_END
            if stack or not root_seen or not root_closed:
                errors.append("end_token_before_balanced_root")
            else:
                found_end = True
            break
        else:
            errors.append(f"unknown_structure_token_0x{token:x}")
            break
    if not found_end and not errors:
        errors.append("structure_scan_limit_reached" if tokens >= MAX_FDT_TOKENS else "missing_fdt_end_token")
    result.update({"nodes": nodes, "properties": properties, "tokens_examined": tokens,
                   "sample_properties": samples, "valid_checks": not errors and found_end,
                   "note": "Structural validation only; peripheral values and board compatibility are not verified."})
    return result


def inspect_squashfs(data, offset, swapped=False):
    result = {"offset": offset, "offset_hex": hex(offset), "plausible_v4_superblock": False,
              "byte_order": "big_endian_candidate" if swapped else "little_endian", "errors": []}
    if offset + 32 > len(data):
        result["errors"].append("truncated_superblock_prefix")
        return result
    order = ">" if swapped else "<"
    major, minor = struct.unpack_from(order + "HH", data, offset + 28)
    result.update({"major": major, "minor": minor})
    if (major, minor) != (4, 0):
        result["errors"].append("not_supported_squashfs_v4_0_layout")
        return result
    if offset + 96 > len(data):
        result["errors"].append("truncated_v4_superblock")
        return result
    fields = struct.unpack_from(order + "5I6H8Q", data, offset)
    _, inodes, stamp, block, fragments, compression, block_log, flags, ids, _, _, root, used, id_table, xattr, inode_table, directory_table, fragment_table, lookup = fields
    result.update({"inodes": inodes, "creation_timestamp_unix": stamp, "block_size": block,
                   "fragments": fragments, "compression_code": compression,
                   "compression_name": SQUASH_COMP.get(compression, "unknown"),
                   "block_log": block_log, "flags": flags, "id_count": ids,
                   "root_inode_reference": hex(root), "bytes_used": used,
                   "declared_end_offset": offset + used,
                   "table_offsets_relative": {"id": id_table, "xattr": xattr, "inode": inode_table,
                                              "directory": directory_table, "fragment": fragment_table, "lookup": lookup}})
    errors = result["errors"]
    if used < 96 or offset + used > len(data):
        errors.append("filesystem_extent_out_of_bounds")
    if block < 4096 or block > 1048576 or block & (block - 1) or block_log != block.bit_length() - 1:
        errors.append("invalid_block_size_or_log")
    if not inodes:
        errors.append("zero_inodes")
    for name, position in result["table_offsets_relative"].items():
        if position == 0xffffffffffffffff and name not in ("inode", "directory"):
            continue
        if position < 96 or position > used:
            errors.append(f"{name}_table_offset_out_of_bounds")
    if swapped:
        errors.append("byte_swapped_signature_candidate_not_standard_v4_little_endian")
    result["plausible_v4_superblock"] = not errors
    result["note"] = "Only superblock fields are checked. No filesystem metadata or file contents were decompressed or validated."
    return result


def entropy(data):
    if not data:
        return 0.0
    size = len(data)
    return -sum((count / size) * math.log2(count / size) for count in Counter(data).values())


def make_report(data, path, window_size=65536, candidate_limit=256, string_limit=200):
    u_offsets, u_limited = offsets(data, b"\x27\x05\x19\x56", candidate_limit)
    f_offsets, f_limited = offsets(data, b"\xd0\x0d\xfe\xed", candidate_limit)
    s_offsets, s_limited = offsets(data, b"hsqs", candidate_limit)
    swapped_offsets, swapped_limited = offsets(data, b"sqsh", candidate_limit)
    strings = []
    pattern = rb"(?:Linux version |U-Boot(?: SPL)? |Anyka Linux Kernel Version|AnyCloud|anycloud_|AK3918|ak3918|KM01|km01)[\x20-\x7e]{0,200}"
    strings_limited = False
    for match in re.finditer(pattern, data):
        if len(strings) >= string_limit:
            strings_limited = True
            break
        strings.append({"offset": match.start(), "offset_hex": hex(match.start()),
                        "text": match.group().decode("ascii")})
    return {
        "schema_version": 1, "tool": "inspect_firmware.py",
        "created_utc": datetime.now(timezone.utc).isoformat(), "input": str(path),
        "size_bytes": len(data), "sha256": hashlib.sha256(data).hexdigest(),
        "uimage_candidates": [inspect_uimage(data, n) for n in u_offsets],
        "fdt_candidates": [inspect_fdt(data, n) for n in f_offsets],
        "squashfs_candidates": sorted([inspect_squashfs(data, n) for n in s_offsets]
                                      + [inspect_squashfs(data, n, True) for n in swapped_offsets], key=lambda x: x["offset"]),
        "interesting_strings": strings,
        "scan_limits_reached": {"uimage": u_limited, "fdt": f_limited,
                                "squashfs": s_limited, "swapped_squashfs": swapped_limited,
                                "strings": strings_limited},
        "entropy_bits_per_byte": round(entropy(data), 6), "entropy_window_size": window_size,
        "entropy_windows": [{"offset": start, "size": len(data[start:start + window_size]),
                             "bits_per_byte": round(entropy(data[start:start + window_size]), 6)}
                            for start in range(0, len(data), window_size)],
        "limitations": [
            "Signatures can occur inside other data and do not define partition boundaries.",
            "High entropy can represent compression, encryption, random data, or other content; it is not a corruption verdict.",
            "FDT full structure checks require the v17 header layout and compatibility with version 17; older candidates are reported but not validated.",
            "CRC success is not a signature/authenticity check and does not establish that an image can boot this board.",
            "No decompression, extraction, binary execution, hardware access, or firmware modification is performed.",
        ],
    }


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("image", type=Path, help="offline firmware dump (regular file)")
    parser.add_argument("--json-out", type=Path, help="write a NEW JSON report; an existing path is refused")
    parser.add_argument("--window-size", type=positive_int, default=65536, help="entropy window bytes (default 65536)")
    parser.add_argument("--max-candidates", type=positive_int, default=256, help="maximum hits per signature (default 256)")
    parser.add_argument("--max-string-hits", type=positive_int, default=200)
    parser.add_argument("--max-bytes", type=positive_int, default=256 * 1024 * 1024,
                        help="memory-use guard: maximum input size (default 256 MiB)")
    args = parser.parse_args(argv)
    try:
        path = args.image.expanduser().resolve(strict=True)
        if not path.is_file():
            raise ValueError("input must be an offline regular file")
        before = path.stat()
        if before.st_size > args.max_bytes:
            raise ValueError(f"input exceeds --max-bytes ({args.max_bytes}); increase explicitly if appropriate")
        with path.open("rb") as source:
            data = source.read(args.max_bytes + 1)
        after = path.stat()
        if len(data) > args.max_bytes or len(data) != before.st_size or before.st_mtime_ns != after.st_mtime_ns:
            raise ValueError("input changed during reading or exceeded the size limit")
        report = make_report(data, path, args.window_size, args.max_candidates, args.max_string_hits)
        if args.json_out is not None:
            with args.json_out.expanduser().open("x", encoding="utf-8") as output:
                json.dump(report, output, indent=2, ensure_ascii=True)
                output.write("\n")
    except (OSError, ValueError, struct.error) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2
    print(f"Input: {path}\nBytes: {len(data)}\nSHA256: {report['sha256']}")
    for key, label, valid_key in (("uimage_candidates", "uImage", "valid_checks"),
                                  ("fdt_candidates", "FDT", "valid_checks"),
                                  ("squashfs_candidates", "SquashFS", "plausible_v4_superblock")):
        print(f"{label}: {len(report[key])} candidate(s)")
        for candidate in report[key]:
            state = "listed checks passed" if candidate[valid_key] else ", ".join(candidate["errors"])
            print(f"  {candidate['offset_hex']}: {state}")
            if "name" in candidate:
                print(f"    name={candidate['name']!r}; payload={candidate['payload_size']} bytes")
            if candidate.get("sample_properties"):
                print("    " + json.dumps(candidate["sample_properties"], ensure_ascii=True))
    for item in report["interesting_strings"]:
        print(f"String {item['offset_hex']}: {item['text']!r}")
    print(f"Entropy: {report['entropy_bits_per_byte']:.4f} bits/byte; descriptive only.")
    if any(report["scan_limits_reached"].values()):
        print("Some hit lists reached their configured limit; inspect scan_limits_reached in JSON.")
    print("No extracted files or firmware changes were made. Use --json-out for detailed metadata and per-window entropy.")
    # A completed scan succeeds even if no signatures validate; candidates are evidence, not a universal format requirement.
    return 0


if __name__ == "__main__":
    sys.exit(main())
