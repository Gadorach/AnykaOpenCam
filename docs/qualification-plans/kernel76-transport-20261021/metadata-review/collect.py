#!/usr/bin/env python3
"""Offline source/artifact binding and the two admitted host-only unit scripts.

Never opens a control socket, target device, network, or private capture. Never
imports a build recipe or executes an ARM artifact. Outputs are exclusive.
"""
import ast
import datetime
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[4]
OUT = Path(__file__).resolve().parent
OLD = ROOT / "work/lightbulb-camera/implementation-20261008T205404Z"
NEW = ROOT / "work/lightbulb-camera/foundation-20261011T041148Z"


def sha(data):
    return hashlib.sha256(data).hexdigest()


def archive(data):
    records = {}
    pos = 0
    while True:
        assert data[pos:pos + 6] == b"070701"
        h = [int(data[pos + 6 + i * 8:pos + 14 + i * 8], 16) for i in range(13)]
        end = pos + 110 + h[11]
        assert end <= len(data) and data[end - 1] == 0
        name = data[pos + 110:end - 1].decode("ascii")
        start = (end + 3) & ~3
        assert start + h[6] <= len(data) and name not in records
        records[name] = (h[1], data[start:start + h[6]])
        pos = (start + h[6] + 3) & ~3
        if name == "TRAILER!!!":
            assert not any(data[pos:])
            return records


def main():
    started = datetime.datetime.now(datetime.timezone.utc).isoformat()
    app = ROOT / "apps/qualification/kernel76-transport"
    source_paths = [app / name for name in (
        "metadata21a.py", "radio_receipt.py", "test_metadata21a.py", "test_radio_receipt.py")]
    source_paths += [ROOT / p for p in (
        "scripts/qc07f_boot.py", "scripts/qc07f_private_root_transfer.py",
        "bringup/qc07f/initramfs/init.c", "bringup/qc07f/initramfs/wifi-monitor.h",
        "bringup/qc07f/initramfs/radio-ram-loader.h",
        "evidence/qualification/qc07f/execution-20261021/ram76-resume21a/postcheck.py")]
    bb_source = OLD / "sources/busybox-1.37.0/busybox-1.37.0"
    source_paths += [bb_source / p for p in ("coreutils/ls.c", "coreutils/test.c", "shell/ash.c")]
    linux = NEW / "artifacts/native-usb76-source01/linux"
    source_paths += [linux / p for p in (
        "drivers/usb/core/sysfs.c", "drivers/usb/core/endpoint.c",
        "drivers/usb/core/message.c", "fs/proc/fd.c", "drivers/base/dd.c")]
    station = NEW / "artifacts/native-usb76-source01/station76-final"
    source_paths += [station / p for p in ("hgic_def.h", "utils/fwdl.h", "utils/fwdl.c")]
    pins = {str(p.relative_to(ROOT)): sha(p.read_bytes()) for p in source_paths}

    manifest_path = NEW / "analysis/native-usb76-package01/package76-manifest.json"
    manifest = json.loads(manifest_path.read_text())
    cpio_path = ROOT / next(p for p in manifest["output_sha256"] if p.endswith("initramfs76.cpio"))
    cpio = cpio_path.read_bytes()
    assert sha(cpio) == manifest["output_sha256"][str(cpio_path.relative_to(ROOT))]
    embedded_path = NEW / "analysis/native-usb76-package01/build/usr/initramfs_inc_data"
    assert gzip.decompress(embedded_path.read_bytes()) == cpio
    records = archive(cpio)
    bb = OLD / "artifacts/busybox-wifi-armv5-a6"
    build = json.loads((OLD / "analysis/wifi-static-userland-build-a6.json").read_text())
    bb_pin = next(p for p in build["outputs"] if p["path"].endswith("/busybox-wifi-armv5-a6"))
    assert records["bin/busybox"][1] == bb.read_bytes()
    assert sha(bb.read_bytes()) == bb_pin["sha256"]
    assert len(bb.read_bytes()) == bb_pin["bytes"]
    config = build["config_busybox"].splitlines()
    keys = ("DESKTOP", "ASH", "ASH_TEST", "LS", "CAT", "TEST", "READLINK", "FEATURE_EDITING",
            "FEATURE_LS_USERNAME", "FEATURE_LS_TIMESTAMPS", "FEATURE_LS_COLOR", "FEATURE_SH_STANDALONE")
    selected = {k: next(line for line in config if line.startswith("CONFIG_" + k + "=")
                       or line == "# CONFIG_" + k + " is not set") for k in keys}
    assert selected["FEATURE_EDITING"] == "# CONFIG_FEATURE_EDITING is not set"
    assert selected["READLINK"] == "# CONFIG_READLINK is not set"
    assert selected["DESKTOP"] == "# CONFIG_DESKTOP is not set"
    assert not any(name in records for name in ("etc/profile", ".profile", "root/.profile"))
    assert records["init"][1] == (OLD / "artifacts/init-monitor-attempt50").read_bytes()

    post = ast.parse((source_paths[9]).read_text())
    queries = next(ast.literal_eval(n.value) for n in post.body if isinstance(n, ast.Assign)
                   and any(isinstance(t, ast.Name) and t.id == "QUERIES" for t in n.targets))
    post_tx = sum(len(s.encode()) + 1 for s in list(queries.values()) + [queries["boot_id"]])
    tests = []
    for name in ("test_metadata21a.py", "test_radio_receipt.py"):
        argv = [sys.executable, "-B", str(app / name), "-v"]
        begin = datetime.datetime.now(datetime.timezone.utc).isoformat()
        result = subprocess.run(argv, cwd=ROOT, env=dict(os.environ, PYTHONDONTWRITEBYTECODE="1"),
                                capture_output=True, text=True, timeout=60)
        log = OUT / (name + ".log")
        with log.open("x") as f:
            f.write("argv: " + json.dumps(argv) + "\n")
            f.write("cwd: " + str(ROOT) + "\n")
            f.write("start_utc: " + begin + "\n")
            f.write("stdout:\n" + result.stdout + "\nstderr:\n" + result.stderr)
            f.write("\nexit: " + str(result.returncode) + "\n")
        tests.append({"argv": argv, "start_utc": begin, "exit": result.returncode,
                      "log": log.name, "log_sha256": sha(log.read_bytes())})
    assert pins == {str(p.relative_to(ROOT)): sha(p.read_bytes()) for p in source_paths}
    report = {
        "start_utc": started,
        "end_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "scope": "offline source/artifact binding and two host-only test scripts; no native operation",
        "python": sys.version,
        "collector_sha256": sha(Path(__file__).read_bytes()),
        "source_sha256": pins,
        "busybox_a6": {"bytes": bb_pin["bytes"], "sha256": bb_pin["sha256"],
                       "matches_selected_cpio_and_retained_build_receipt": True,
                       "selected_cpio_matches_final_build_embedded_archive": True,
                       "selected_config_facts": selected, "profile_entries_absent": True},
        "selected_cpio_sha256": sha(cpio),
        "monitor_matches_retained_attempt50": True,
        "fixed_postcheck_query_count_including_repeat_boot_id": len(queries) + 1,
        "fixed_postcheck_command_TX_bytes_excluding_optional_one_CR_refresh": post_tx,
        "expected_native_protocol": {"chunks": 74, "ACKs": 2 + 2 * 74 + 1,
                                     "payload_bytes": 73 * 2048 + 1552,
                                     "wire_bytes": 73 * 2052 + 1552},
        "tests": tests,
        "target_operation": False,
        "private_capture_access": False,
        "native_metadata_or_loader_pass_claimed": False,
    }
    with (OUT / "result.json").open("x") as f:
        json.dump(report, f, indent=2); f.write("\n")
    assert all(t["exit"] == 0 for t in tests)
    print("Offline binding and two host-only test scripts PASS; no native operation.")


if __name__ == "__main__":
    main()
