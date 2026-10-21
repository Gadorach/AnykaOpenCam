#!/usr/bin/env python3
"""Offline RP-04 hash/arithmetic audit. No builds, processes, procfs or devices."""
import ast
import datetime
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[4]
OUT = Path(__file__).resolve().parent / "audit.json"
K = ROOT / "work/lightbulb-camera/foundation-20261011T041148Z/artifacts/native-usb76-source01/linux"
V = ROOT / "evidence/delegated/20261012-native-handoff"
P = ROOT / "apps/web-media/native-producer"


def seal(path):
    data = path.read_bytes()
    return {"path": str(path.relative_to(ROOT)), "bytes": len(data),
            "sha256": hashlib.sha256(data).hexdigest()}


def compare(path, expected):
    item = seal(path)
    item.update(expected_sha256=expected, matches=item["sha256"] == expected)
    return item


def main():
    if OUT.exists():
        raise SystemExit("refuse to overwrite existing evidence")
    report = json.loads((V / "REPORT-v6.json").read_text())
    manifest = json.loads((V / "run34/manifest.json").read_text())
    tree = ast.parse((P / "build.py").read_text())
    pins = next(ast.literal_eval(n.value) for n in tree.body
                if isinstance(n, ast.Assign) and any(isinstance(t, ast.Name)
                   and t.id == "PINS" for t in n.targets))
    current = [compare(P / name, expected)
               for name, expected in report["source_hashes"].items()]
    audit_pins = [compare(ROOT / name, expected) for name, expected in pins.items()]
    candidate = []
    for name, expected in pins.items():
        if "/mach-ak3918ev200/" in name:
            path = K / "arch/arm/mach-ak3918ev200" / Path(name).name
        elif "/sources/linux-lts/" in name:
            path = K / name.split("/sources/linux-lts/", 1)[1]
        else:
            continue
        candidate.append(compare(path, expected))
    framework = []
    for name, entry in manifest["source_inputs"].items():
        marker = "/framework/"
        if marker in name:
            framework.append(compare(ROOT / "framework" / name.split(marker, 1)[1], entry["sha256"]))
    run_inputs = [compare(V / "run34/inputs" / name, expected)
                  for name, expected in report["source_hashes"].items()]
    package_path = ROOT / "work/lightbulb-camera/foundation-20261011T041148Z/analysis/native-usb76-package01/package76-manifest.json"
    package = json.loads(package_path.read_text())
    artifacts = [compare(V / "run34/manifest.json", report["manifest_sha256"]),
                 compare(ROOT / report["ELF"]["path"], report["ELF"]["sha256"]),
                 compare(ROOT / package["image"], package["image_sha256"])]
    for path, expected in package["output_sha256"].items():
        if path.endswith("/.config"):
            artifacts.append(compare(ROOT / path, expected))
    MiB = 1048576
    frame, ref, table, stat = 2 * MiB, 4 * MiB, 16384, 12288
    codec = 4 * 2 * MiB + 65536
    isp = frame + ref + table
    dma = {}
    for name, size in {
        "gated_isp": isp,
        "codec": codec,
        "gated_isp_and_codec": isp + codec,
        "four_slot_capture": 4 * frame + ref + table + 4 * stat,
        "four_slot_and_codec": 4 * frame + ref + table + 4 * stat + codec,
        "four_slot_and_imported_codec": 4 * frame + ref + table + 4 * stat + codec - frame,
        "two_slot_four_stats_and_imported_codec": 2 * frame + ref + table + 4 * stat + codec - frame,
        "two_slot_four_stats_import_plus_sink": 3 * frame + ref + table + 4 * stat + codec - frame,
    }.items():
        dma[name] = {"bytes": size, "KiB": size / 1024, "MiB": size / MiB,
                     "16MiB_minus_requested_bytes": 16 * MiB - size,
                     "not_admission": True}
    checks = current + audit_pins + candidate + framework + run_inputs + artifacts
    result = {
        "utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "command": "python3 -B docs/qualification-plans/execution-20261021/memory-admission01/audit.py",
        "python": sys.version,
        "scope": "public repository source/manifests/artifact hashes and integer arithmetic ONLY",
        "native_operations": False, "new_build_or_test_execution": False,
        "source_script": seal(Path(__file__).resolve()),
        "reference_seals": [seal(V / "REPORT-v6.json"), seal(package_path),
                            seal(V / "RAW-REUSE-CONTRACT-v6.md"),
                            seal(ROOT / "docs/qualification-plans/qc07f-performance/COST-AND-MEMORY.md")],
        "v6_current_vs_report": current, "v6_run34_inputs_vs_report": run_inputs,
        "current_build_audit_pins": audit_pins,
        "candidate76_vs_v6_kernel_audit_pins": candidate,
        "current_framework_vs_run34": framework,
        "artifact_identity": artifacts,
        "all_comparisons_match": all(c["matches"] for c in checks),
        "comparison_count": len(checks),
        "dma_requested_arithmetic": dma,
        "v6_CPU_KiB": {"RX": 88, "RW_including_raw_and_sampler": 1724,
                       "nonshared_staging": 4, "each_large_musl_mapping": 1028,
                       "raw_reuse": 2844, "separate": 3872,
                       "raw_reuse_payload_bytes": 2776576,
                       "separate_payload_bytes": 3825152},
        "dated_i_arithmetic_only": report["i_initial_arithmetic"],
        "not_proved": ["current target state", "placement", "CMA contiguity",
                       "native v6", "raw overlay on ASIC", "combined TLS fit",
                       "two-slot masks/starvation/import/retirement", "G2/G8"],
    }
    with OUT.open("x") as output:
        json.dump(result, output, indent=2)
        output.write("\n")
    print(json.dumps({"output": str(OUT.relative_to(ROOT)),
                      "comparison_count": len(checks),
                      "all_comparisons_match": result["all_comparisons_match"],
                      "candidate_kernel_pin_count": len(candidate),
                      "framework_count": len(framework)}))


if __name__ == "__main__":
    main()
