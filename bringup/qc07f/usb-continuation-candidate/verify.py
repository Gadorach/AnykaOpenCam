#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Independent final artifact/archive/range/rollback checks. No target access."""
import datetime
import gzip
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import struct
import subprocess
import sys

if not __debug__:
    raise SystemExit("Candidate safety checks require assertions; do not use Python -O")

ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent
FOUND = ROOT / "work/lightbulb-camera/foundation-20261011T041148Z"
ART = FOUND / "artifacts/native-usb76-source01"
BUILD = FOUND / "analysis/native-usb76-package01"
FIRST = FOUND / "analysis/native-usb76-build01"
OLD = FOUND / "analysis/native-security75-build01"


def digest(data):
    return hashlib.sha256(data).hexdigest()


def archive(data):
    """Separate parser, not the transformation module's parser."""
    offset = 0; result = {}
    while True:
        start = offset
        header = data[offset:offset + 110]
        assert len(header) == 110 and header[:6] == b"070701"
        fields = tuple(int(header[6 + 8 * i:14 + 8 * i], 16) for i in range(13))
        assert fields[11] > 0 and fields[12] == 0
        offset += 110
        name = data[offset:offset + fields[11]]
        assert len(name) == fields[11] and name[-1] == 0 and 0 not in name[:-1]
        name = name[:-1].decode("ascii")
        path = name.removeprefix("./")
        assert path and not path.startswith("/") and ".." not in PurePosixPath(path).parts
        assert str(PurePosixPath(path)) == path and path not in result
        offset = (offset + fields[11] + 3) // 4 * 4
        payload = data[offset:offset + fields[6]]
        assert len(payload) == fields[6]
        offset = (offset + fields[6] + 3) // 4 * 4
        assert offset <= len(data)
        result[path] = {"fields": fields, "payload": payload, "record": data[start:offset]}
        if path == "TRAILER!!!":
            assert fields[6] == 0 and not any(data[offset:])
            return result


def main():
    out = Path(sys.argv[1]); out.mkdir(parents=True, exist_ok=False)
    package = json.loads((BUILD / "package76-manifest.json").read_text())
    assert package["state"] == "RAM_CANDIDATE76_PACKAGED_NOT_BOOTED"
    checks = {}
    checks["all_packaged_output_pins_match"] = all(digest((ROOT / path).read_bytes()) == value for path, value in package["output_sha256"].items())
    image = (ROOT / package["image"]).read_bytes()
    words = struct.unpack(">7I4B32s", image[:64])
    # Independent IEEE CRC implementation provided by Python's binascii wrapper.
    import binascii
    checks["uImage_magic_size_load_entry_type"] = words[0] == 0x27051956 and words[3] == len(image) - 64 and words[4:6] == (0x82408000, 0x82408040) and words[7:11] == (5, 2, 2, 0)
    checks["header_IEEE_CRC"] = binascii.crc32(image[:4] + b"\0" * 4 + image[8:64]) & 0xffffffff == words[1]
    checks["payload_IEEE_CRC"] = binascii.crc32(image[64:]) & 0xffffffff == words[6]
    dtb = (ART / "inputs/attempt45.dtb").read_bytes()
    checks["payload_exact_zImage_plus_retained_DTB"] = image[64:] == (BUILD / "build/arch/arm/boot/zImage").read_bytes() + dtb
    checks["staging_below_CMA"] = 0x82408000 + len(image) < 0x82800000
    symbols = subprocess.run(["llvm-nm", "--defined-only", str(BUILD / "build/vmlinux")], check=True, capture_output=True, text=True, timeout=30).stdout
    found = {x.split()[-1]: int(x.split()[0], 16) for x in symbols.splitlines() if x.split()[-1] in ("_text", "_end")}
    start, end = (0x81800000 + found[name] - 0xc0000000 for name in ("_text", "_end"))
    checks["independent_decompressed_range"] = start == 0x81808000 and end < 0x82408000 and package["decompressed_range"] == [hex(start), hex(end)]
    cpio50 = (ART / "inputs/initramfs50.cpio").read_bytes()
    cpio76 = (ART / "inputs/initramfs76.cpio").read_bytes()
    old, new = archive(cpio50), archive(cpio76)
    replacements = {"hgics.ko", "bin/rtspd"}; added = {"bin/qc07f-dhcp-script-bridge"}
    checks["exact_archive_path_delta"] = set(new) == set(old) | added
    checks["all_unselected_archive_records_unchanged"] = all(new[name]["record"] == row["record"] for name, row in old.items() if name not in replacements)
    checks["replacement_metadata_preserved_except_size"] = all(all(a == b for i, (a, b) in enumerate(zip(old[name]["fields"], new[name]["fields"])) if i != 6) for name in replacements)
    checks["final_build_embeds_selected_new_archive"] = gzip.decompress((BUILD / "build/usr/initramfs_inc_data").read_bytes()) == cpio76
    checks["fresh_final_radio_equals_embedded_and_bootstrap"] = new["hgics.ko"]["payload"] == (ART / "station76-final/hgic_smac/hgics.ko").read_bytes() == (ART / "images/hgics-linux6-station76.ko").read_bytes()
    checks["current_RTSP_and_bridge_pins_embedded"] = digest(new["bin/rtspd"]["payload"]) == "ddc45d7bd18ed8ab525c309e5ed617db127147670d5f9da0eefeeef6e3f467dd" and digest(new["bin/qc07f-dhcp-script-bridge"]["payload"]) == "6c7766557c1d0fe9e83ef33a5339f3836c78a29f84e2a4a02ae64b0d2bb39ef2"
    archive_test = subprocess.run([sys.executable, "-B", str(HERE / "test_cpio.py")], capture_output=True, text=True, timeout=30)
    (out / "cpio-tests.stdout").write_text(archive_test.stdout); (out / "cpio-tests.stderr").write_text(archive_test.stderr)
    checks["17_archive_tests_reproduced"] = archive_test.returncode == 0 and "Ran 17 tests" in archive_test.stderr and "OK" in archive_test.stderr
    inventory = json.loads((OLD / "complete-source75-inventory.json").read_text())["files"]
    expected = dict(json.loads((FIRST / "prepare76-manifest.json").read_text())["only_changed_kernel_file"])
    preserved = True; changed = {}
    for name, info in inventory.items():
        old_file = FOUND / "artifacts/native-security75-source01/linux" / name
        current = ART / "linux" / name
        if "symlink" in info:
            preserved &= old_file.is_symlink() and os.readlink(old_file) == info["symlink"] and current.is_symlink() and os.readlink(current) == info["symlink"]
        else:
            preserved &= digest(old_file.read_bytes()) == info["sha256"]
            actual = digest(current.read_bytes())
            if actual != info["sha256"]:
                changed[name] = actual
    checks["91268_baseline_source_entries_preserved"] = bool(preserved)
    checks["only_admitted_USB_source_diff"] = changed == expected
    checks["baseline75_outputs_preserved"] = all(digest(Path(path).read_bytes()) == value for path, value in json.loads((OLD / "build75-manifest.json").read_text())["output_sha256"].items())
    checks["first76_outputs_preserved"] = all(digest((ROOT / path).read_bytes()) == value for path, value in json.loads((FIRST / "build76-manifest.json").read_text())["output_sha256"].items())
    crc = subprocess.run([str(BUILD / "image_crc"), str(ROOT / package["image"])], check=True, capture_output=True, text=True, timeout=30).stdout.strip()
    checks["host_C_BZIP2_matches_manifest"] = crc == package["native_expected_fullimage_BZIP2"]
    checks["no_UART_owner_socket_or_operation_HOLD"] = not os.path.lexists(ROOT / "work/qc-uart-live/uart.sock") and not (ROOT / "work/qc-uart-live/peripheral-gui-operation.lock").read_text()
    result = {"utc": datetime.datetime.now(datetime.timezone.utc).isoformat(), "state": "OFFLINE_CANDIDATE76_VERIFIED_NOT_BOOTED",
              "checks": checks, "all_pass": all(checks.values()), "manifest_sha256": digest((BUILD / "package76-manifest.json").read_bytes()),
              "input_recipe_pins": {p.name: digest(p.read_bytes()) for p in HERE.glob("*.py")},
              "target_operation": False, "native_MTU1500_pass": False, "new_GUI_admission": False}
    (out / "result.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps({"state": result["state"], "checks": len(checks), "all_pass": result["all_pass"]}))
    assert all(checks.values())


if __name__ == "__main__":
    main()
