#!/usr/bin/env python3
"""Source-only placement review: no imports/execution of project code or binaries.

Writes one fresh JSON beside this script; reads only explicitly selected public
source, effective config and JSON/Markdown evidence. No procfs, target or network.
"""
import ast
import datetime
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[5]
HERE = Path(__file__).resolve().parent
K = "work/lightbulb-camera/foundation-20261011T041148Z/artifacts/native-usb76-source01/linux/"
P = "apps/web-media/native-producer/"
V = "evidence/delegated/20261012-native-handoff/"
BASE = "docs/qualification-plans/execution-20261021/memory-admission01/"
CFG = "work/lightbulb-camera/foundation-20261011T041148Z/analysis/native-usb76-package01/build/.config"


def seal(name):
    data = (ROOT / name).read_bytes()
    return {"path": name, "bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}


def compare(name, expected):
    item = seal(name)
    item.update(expected_sha256=expected, matches=item["sha256"] == expected)
    return item


def main():
    out = HERE / "source-audit.json"
    if out.exists():
        raise SystemExit("refuse to overwrite evidence")
    report = json.loads((ROOT / V / "REPORT-v6.json").read_text())
    old = json.loads((ROOT / BASE / "audit.json").read_text())
    tree = ast.parse((ROOT / P / "build.py").read_text())
    pins = next(ast.literal_eval(n.value) for n in tree.body
                if isinstance(n, ast.Assign) and any(isinstance(t, ast.Name)
                and t.id == "PINS" for t in n.targets))
    comparisons = []
    for name, expected in report["source_hashes"].items():
        comparisons.append(compare(P + name, expected))
        comparisons.append(compare(V + "run34/inputs/" + name, expected))
    for name, expected in pins.items():
        comparisons.append(compare(name, expected))
        if "/mach-ak3918ev200/" in name:
            candidate = K + "arch/arm/mach-ak3918ev200/" + Path(name).name
        elif "/sources/linux-lts/" in name:
            candidate = K + name.split("/sources/linux-lts/", 1)[1]
        else:
            continue
        comparisons.append(compare(candidate, expected))
    for item in old["current_framework_vs_run34"]:
        comparisons.append(compare(item["path"], item["sha256"]))
    comparisons.append(compare(V + "run34/manifest.json", report["manifest_sha256"]))
    package_name = "work/lightbulb-camera/foundation-20261011T041148Z/analysis/native-usb76-package01/package76-manifest.json"
    package = json.loads((ROOT / package_name).read_text())
    comparisons.append(compare(CFG, package["output_sha256"][CFG]))

    # Whole-source hashes plus bounded verbatim excerpts, never a binary dump.
    ranges = {
        K + "arch/arm/mach-ak3918ev200/isp.c": [(48, 58), (143, 151), (215, 243), (282, 295), (473, 495), (1491, 1524), (1557, 1571), (1580, 1614)],
        K + "arch/arm/mach-ak3918ev200/qc07f_hwenc.c": [(155, 169), (240, 265), (278, 303), (321, 359), (364, 374), (429, 457), (478, 504)],
        K + "arch/arm/mach-ak3918ev200/stage-telemetry.h": [(108, 128), (130, 155)],
        K + "arch/arm/mach-ak3918ev200/stage-telemetry-uapi.h": [(25, 47)],
        K + "arch/arm/mach-ak3918ev200/capture-retirement.c": [(42, 79)],
        K + "arch/arm/mach-ak3918ev200/capture-retirement-uapi.h": [(1, 35)],
        K + "arch/arm/mach-ak3918ev200/isp-stream.h": [(1, 80)],
        K + "arch/arm/mm/dma-mapping.c": [(391, 424), (533, 599), (1811, 1823)],
        K + "kernel/dma/mapping.c": [(628, 668)],
        K + "kernel/dma/direct.c": [(205, 221)],
        K + "kernel/dma/contiguous.c": [(304, 339)],
        K + "include/linux/dma-map-ops.h": [(99, 107)],
        K + "include/linux/dma-direct.h": [(75, 115)],
        K + "mm/cma.c": [(782, 859), (990, 1018)],
        P + "memory_reserve.c": [(114, 143)],
        P + "memory_runtime.c": [(24, 50)],
        P + "native_adapter.c": [(36, 91)],
        P + "main.c": [(18, 37)],
        P + "build.py": [(14, 25), (164, 175)],
    }
    evidence = []
    for name, spans in ranges.items():
        lines = (ROOT / name).read_text().splitlines()
        item = seal(name)
        item["excerpts"] = [{"first": a, "last": min(b, len(lines)),
                             "text": "\n".join(f"{i}: {lines[i-1]}" for i in range(a, min(b, len(lines)) + 1))}
                            for a, b in spans]
        evidence.append(item)
    fixed_kib = (0x4000 + 0x200000 + 0x400000) // 1024
    cases = []
    # DATED i scalars ONLY, conditional algebra, NOT newly parsed native evidence.
    for mode, cpu in (("raw-reuse-k75", 2844), ("separate", 3872)):
        for fixed in (0, fixed_kib):
            occupied, dma, n, available = 7168, 8256, 5476, 14756
            migration = min(occupied - fixed, dma)
            need_n, need_a = cpu + 512 + 1024 + migration, cpu + dma + 512 + 1024
            cases.append({"mode": mode, "H": 512, "R": 1024, "conditional_fixed_kib": fixed,
                          "unknown_occupied_kib": occupied - fixed, "future_kib": migration,
                          "require_N_kib": need_n, "N_margin_kib": n - need_n,
                          "require_A_kib": need_a, "A_margin_kib": available - need_a,
                          "placement_proved": False, "native_admission": False})
    result = {
        "utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "command": "python3 -B " + BASE + "placement-review/audit_source.py",
        "python": sys.version,
        "scope": "public source/config/manifests only; source strings and integer arithmetic",
        "builds_existing_tests_ARM_execution_target_network_private_reads": False,
        "script": seal(str(Path(__file__).resolve().relative_to(ROOT))),
        "reference_seals": [seal(BASE + "REPORT.md"), seal(BASE + "audit.json"),
                            seal(BASE + "audit.py"), seal(V + "REPORT-v6.json"),
                            seal(V + "RAW-REUSE-CONTRACT-v6.md"), seal(V + "PAUSED-v6-STATUS.md"),
                            seal(package_name), seal(CFG)],
        "identity_comparisons": comparisons,
        "all_comparisons_match": all(x["matches"] for x in comparisons),
        "comparison_count": len(comparisons),
        "source_evidence": evidence,
        "conditional_dated_i_arithmetic_only": cases,
        "fixed_request_kib": fixed_kib,
        "target_placement_receipt_present": False,
        "parent_v6_rerun_or_76_adoption_complete": False,
    }
    with out.open("x") as stream:
        json.dump(result, stream, indent=2)
        stream.write("\n")
    print(json.dumps({"output": str(out.relative_to(ROOT)),
                      "comparisons": len(comparisons),
                      "all_match": result["all_comparisons_match"],
                      "fixed_request_kib_not_placement_proof": fixed_kib}))
    if not result["all_comparisons_match"]:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
