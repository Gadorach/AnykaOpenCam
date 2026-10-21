#!/usr/bin/env python3
"""Compare two or more offline flash reads. Never opens a hardware device."""

import argparse
from collections import Counter
from datetime import datetime, timezone
import hashlib
import json
import math
from pathlib import Path
import sys


CHUNK_SIZE = 1024 * 1024
BLOCK_SIZE = 4096
PERIODS = tuple(2 ** n for n in range(13))


def positive_int(value):
    try:
        number = int(value, 0)
    except ValueError as exc:
        raise argparse.ArgumentTypeError("use a decimal or 0x-prefixed integer") from exc
    if number <= 0:
        raise argparse.ArgumentTypeError("must be positive")
    return number


def entropy(counts, size):
    if not size:
        return 0.0
    return -sum((count / size) * math.log2(count / size)
                for count in counts.values() if count)


def regular_file(value):
    path = Path(value).expanduser().resolve(strict=True)
    if not path.is_file():
        raise ValueError(f"expected an offline regular file: {path}")
    return path


def analyse_file(path, expected_size=None):
    before = path.stat()
    digest = hashlib.sha256()
    histogram = Counter()
    block_hashes = Counter()
    candidates = None
    size = 0
    with path.open("rb") as stream:
        while True:
            chunk = stream.read(CHUNK_SIZE)
            if not chunk:
                break
            digest.update(chunk)
            histogram.update(chunk)
            if candidates is None:
                candidates = {period: chunk[:period] for period in PERIODS
                              if len(chunk) >= period}
            for period, pattern in list(candidates.items()):
                phase = size % period
                repeated = (pattern * ((phase + len(chunk) + period - 1) // period))
                if chunk != repeated[phase:phase + len(chunk)]:
                    del candidates[period]
            # CHUNK_SIZE is a multiple of BLOCK_SIZE; omit only a final partial block.
            for start in range(0, len(chunk) - BLOCK_SIZE + 1, BLOCK_SIZE):
                block = chunk[start:start + BLOCK_SIZE]
                block_hashes[hashlib.sha256(block).hexdigest()] += 1
            size += len(chunk)
    after = path.stat()
    periods = [period for period in (candidates or {})
               if size >= 2 * period and size % period == 0]
    repeated_period = min(periods) if periods else None
    full_blocks = sum(block_hashes.values())
    common_blocks = max(block_hashes.values(), default=0)
    uniform_byte = next(iter(histogram)) if len(histogram) == 1 else None
    notes = []
    if size == 0:
        notes.append("Empty file: not evidence of a successful flash read.")
    elif uniform_byte is not None:
        notes.append(f"Entire file is byte 0x{uniform_byte:02x}; matching hashes alone are insufficient.")
    elif repeated_period is not None:
        notes.append(f"Entire file repeats a {repeated_period}-byte pattern; inspect the acquisition.")
    if full_blocks >= 2 and common_blocks / full_blocks > 0.5:
        notes.append("Many 4 KiB blocks repeat. Erased/padded flash regions commonly do this; it is not proof of corruption.")
    changed = (before.st_size != after.st_size or before.st_mtime_ns != after.st_mtime_ns
               or size != before.st_size)
    if changed:
        notes.append("Input size or modification time changed while it was being read.")
    return {
        "path": str(path), "size_bytes": size, "sha256": digest.hexdigest(),
        "expected_size_bytes": expected_size,
        "expected_size_matches": None if expected_size is None else size == expected_size,
        "zero_bytes": histogram[0], "ff_bytes": histogram[255],
        "zero_fraction": histogram[0] / size if size else 0.0,
        "ff_fraction": histogram[255] / size if size else 0.0,
        "entropy_bits_per_byte": round(entropy(histogram, size), 6),
        "uniform_byte": uniform_byte,
        "smallest_detected_full_file_repeat_period": repeated_period,
        "repeat_periods_tested_bytes": list(PERIODS),
        "full_4k_blocks": full_blocks,
        "unique_4k_block_hashes": len(block_hashes),
        "most_common_4k_block_fraction": common_blocks / full_blocks if full_blocks else None,
        "changed_during_read": changed,
        "obviously_trivial_content": size == 0 or uniform_byte is not None or repeated_period is not None,
        "notes": notes,
    }


def first_difference(left, right):
    """Return the exact first differing byte, including a short file's EOF."""
    offset = 0
    with left.open("rb") as a, right.open("rb") as b:
        while True:
            ca, cb = a.read(CHUNK_SIZE), b.read(CHUNK_SIZE)
            if ca != cb:
                for index, (va, vb) in enumerate(zip(ca, cb)):
                    if va != vb:
                        return offset + index
                return offset + min(len(ca), len(cb))
            if not ca:
                return None
            offset += len(ca)


def make_report(paths, expected_size=None):
    states = [(p.stat().st_size, p.stat().st_mtime_ns) for p in paths]
    files = [analyse_file(p, expected_size) for p in paths]
    comparisons = []
    for path in paths[1:]:
        difference = first_difference(paths[0], path)
        comparisons.append({"reference": str(paths[0]), "other": str(path),
                            "byte_identical": difference is None,
                            "first_difference_offset": difference,
                            "first_difference_offset_hex": None if difference is None else hex(difference)})
    same_inputs = [(i + 1, j + 1) for i in range(len(paths))
                   for j in range(i + 1, len(paths)) if paths[i].samefile(paths[j])]
    changed = any((p.stat().st_size, p.stat().st_mtime_ns) != old
                  for p, old in zip(paths, states)) or any(f["changed_during_read"] for f in files)
    all_equal = all(c["byte_identical"] for c in comparisons)
    warnings = []
    if same_inputs:
        warnings.append("Some inputs refer to the same physical file; they do not represent separate read files.")
    if changed:
        warnings.append("An input changed during analysis; repeat using stable offline copies.")
    expected_ok = all(f["expected_size_matches"] is not False for f in files)
    trivial = any(f["obviously_trivial_content"] for f in files)
    acceptable = all_equal and expected_ok and not trivial and not same_inputs and not changed
    if not all_equal:
        verdict = "reads_differ"
    elif not expected_ok:
        verdict = "matching_reads_with_unexpected_size"
    elif not acceptable:
        verdict = "matching_reads_need_investigation"
    else:
        verdict = "matching_nontrivial_reads"
    return {
        "schema_version": 1, "tool": "validate_dumps.py",
        "created_utc": datetime.now(timezone.utc).isoformat(),
        "expected_size_bytes": expected_size, "inputs": files,
        "comparisons": comparisons, "all_byte_identical": all_equal,
        "same_physical_input_pairs_1_based": same_inputs,
        "warnings": warnings, "verdict": verdict, "exit_code": 0 if acceptable else 1,
        "limitations": [
            "Matching files do not prove they were acquired in independent hardware reads.",
            "A stable misconfiguration can produce matching but incorrect data.",
            "Entropy is descriptive; high entropy does not establish encryption or corruption.",
            "Expected flash capacity is checked only when explicitly supplied.",
        ],
    }


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("dumps", nargs="+", help="at least two independently acquired offline read files")
    parser.add_argument("--expected-size", type=positive_int,
                        help="optional known capacity in bytes, e.g. 8388608 or 0x800000")
    parser.add_argument("--json-out", type=Path, help="write a NEW JSON report; an existing path is refused")
    args = parser.parse_args(argv)
    if len(args.dumps) < 2:
        parser.error("supply at least two dump files")
    try:
        paths = [regular_file(p) for p in args.dumps]
        report = make_report(paths, args.expected_size)
        if args.json_out is not None:
            with args.json_out.expanduser().open("x", encoding="utf-8") as output:
                json.dump(report, output, indent=2, ensure_ascii=True)
                output.write("\n")
    except (OSError, ValueError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2
    for item in report["inputs"]:
        print(f"{item['path']}\n  bytes: {item['size_bytes']}\n  SHA256: {item['sha256']}")
        print(f"  00: {item['zero_fraction']:.2%}; FF: {item['ff_fraction']:.2%}; entropy: {item['entropy_bits_per_byte']:.4f} bits/byte")
        for note in item["notes"]:
            print(f"  note: {note}")
    for comparison in report["comparisons"]:
        detail = "byte-identical" if comparison["byte_identical"] else f"first difference at {comparison['first_difference_offset_hex']}"
        print(f"Compared with first read: {comparison['other']}: {detail}")
    for warning in report["warnings"]:
        print(f"Warning: {warning}")
    print(f"Result: {report['verdict']}")
    print("Entropy does not identify encryption or corruption. Independent reads and correct chip identification still matter.")
    return report["exit_code"]


if __name__ == "__main__":
    sys.exit(main())
