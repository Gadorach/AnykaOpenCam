#!/usr/bin/env python3
"""Read only named public text; inventory/excerpt audit, never a behavior test."""
import datetime
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[4]
OUT = Path(__file__).resolve().parent
DOC = Path("docs/qualification-plans/kernel76-transport-20261021")
KERNEL = Path("work/lightbulb-camera/foundation-20261011T041148Z/artifacts/native-usb76-source01/linux")
BUILD = Path("work/lightbulb-camera/foundation-20261011T041148Z/analysis/native-usb76-package01/build")
TREES = [Path("apps/qualification") / name / "source" for name in (
    "kernel76-station", "kernel76-station-ingress", "kernel76-station-events")]
SEALS = [DOC / name / "candidate-seal.json" for name in (
    "station-control-a1-host01", "station-ingress-a2a-host01", "station-event-a2b-host01")]
REPORTS = [DOC / name for name in (
    "STATION76_ENTRY_REVIEW.md", "STATION76_CONTROL_A1_REVIEW.md",
    "STATION76_INGRESS_A2A_REVIEW.md", "STATION76_EVENT_ENTRY_REVIEW.md",
    "STATION76_EVENT_A2B_CANDIDATE.md", "STATION76_A2B_PARENT_REPRODUCTION01.json",
    "WATCHDOG_RECOVERY_REVIEW.md")]
EXCERPTS = {
    TREES[2] / "hgic_smac/rx.c": [(164, 205), (207, 325)],
    TREES[2] / "hgic_smac/event.c": [(25, 72)],
    TREES[2] / "hgic_smac/core.c": [(95, 130), (159, 200), (281, 319),
        (563, 642), (703, 744), (802, 898), (900, 1127), (1129, 1289)],
    TREES[2] / "utils/if_usb.c": [(17, 155), (184, 363), (365, 505)],
    TREES[2] / "utils/fwctrl.c": [(52, 130), (170, 388), (414, 438)],
    TREES[2] / "utils/fwctrl.h": [(10, 38)],
    TREES[2] / "utils/fwdl.c": [(25, 50), (110, 138), (261, 289)],
    TREES[2] / "utils/ota.c": [(40, 86)],
    TREES[2] / "hgic_smac/procfs.c": [(9, 89), (194, 249)],
    TREES[2] / "hgic_smac/11bgn.c": [(111, 190), (193, 244)],
    TREES[2] / "hgic_smac/Makefile": [(1, 26)],
    TREES[2] / "hgic.h": [(112, 159), (195, 234), (270, 285)],
    TREES[2] / "hgic_def.h": [(16, 68), (169, 190)],
    Path("bringup/qc07f/initramfs/wifi-monitor.h"): [(1, 37)],
    Path("bringup/qc07f/mainline/arch/arm/mach-ak3918ev200/watchdog.c"): [(39, 129)],
    KERNEL / "arch/arm/mach-ak3918ev200/watchdog.c": [(39, 129)],
    KERNEL / "drivers/watchdog/watchdog_dev.c": [(76, 141), (368, 389), (697, 735), (797, 826), (864, 919), (932, 989)],
    KERNEL / "include/uapi/linux/watchdog.h": [(16, 54)],
    KERNEL / "drivers/usb/core/urb.c": [(232, 259), (678, 721)],
    KERNEL / "net/mac80211/main.c": [(1595, 1620)],
    KERNEL / "net/mac80211/iface.c": [(457, 473), (1419, 1429), (1496, 1509)],
    KERNEL / "fs/proc/inode.c": [(255, 273)],
    BUILD / ".config": [(334, 341), (500, 507), (674, 674), (737, 758), (1174, 1185)],
}


def pin(path):
    data = (ROOT / path).read_bytes()
    return {"path": str(path), "offset": 0, "bytes": len(data),
            "sha256": hashlib.sha256(data).hexdigest()}


def put(name, obj):
    with (OUT / name).open("x", encoding="utf-8") as f:
        json.dump(obj, f, indent=2, ensure_ascii=True)
        f.write("\n")


def main():
    if sys.flags.optimize:
        raise SystemExit("No optimized audit invocation")
    if len(sys.argv) != 1:
        raise SystemExit("No arguments or execution mode supported")
    if any((OUT / name).exists() for name in ("reader-pins.json", "public-excerpts.json")):
        raise SystemExit("Exclusive outputs already exist; preserve them")
    sources = []
    for tree, sealpath in zip(TREES, SEALS):
        seal = json.loads((ROOT / sealpath).read_text())
        if "source_extents" in seal:
            expected = {str(tree / row["path"]): row for row in seal["source_extents"]}
        else:
            expected = {p: row for p, row in seal["files"].items()
                        if p.startswith(str(tree) + "/")}
        actual = {str(p.relative_to(ROOT)) for p in (ROOT / tree).rglob("*") if p.is_file()}
        if actual != set(expected) or len(actual) != 68:
            raise SystemExit(f"Source path set differs: {tree}")
        for path in sorted(actual):
            got = pin(Path(path))
            row = expected[path]
            if any(got[k] != row[k] for k in ("bytes", "sha256")):
                raise SystemExit(f"Source seal mismatch: {path}")
            sources.append(got)
    inputs = {str(p): pin(p) for p in sorted(set(SEALS + REPORTS + list(EXCERPTS) + [BUILD / "Module.symvers", Path(__file__).relative_to(ROOT)]))}
    excerpts = []
    for path, ranges in EXCERPTS.items():
        lines = (ROOT / path).read_bytes().splitlines(keepends=True)
        for start, end in ranges:
            if not 1 <= start <= end <= len(lines):
                raise SystemExit(f"Invalid range: {path}:{start}-{end}")
            data = b"".join(lines[start-1:end])
            excerpts.append({"path": str(path), "first_line": start, "last_line": end,
                "offset": sum(map(len, lines[:start-1])), "bytes": len(data),
                "sha256": hashlib.sha256(data).hexdigest(),
                "text": data.decode("utf-8", errors="backslashreplace")})
    # Re-read only public source/docs; no code from those inputs is executed.
    for row in sources + list(inputs.values()):
        if pin(Path(row["path"])) != row:
            raise SystemExit(f"Input changed during audit: {row['path']}")
    put("public-excerpts.json", excerpts)
    put("reader-pins.json", {
        "utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "argv": sys.argv, "cwd": str(Path.cwd()), "python": sys.version,
        "scope": "STATIC_PUBLIC_TEXT_AUDIT_ONLY_NO_HOST_TEST_BUILD_OR_HARDWARE",
        "source_seals_match": True, "source_files_each": 68,
        "source_pins": sources, "input_pins": list(inputs.values()),
        "excerpts_pin": pin((OUT / "public-excerpts.json").relative_to(ROOT)),
        "missing_path_observation": "A read of utils/bootdl.c returned File not found; actual selected definitions are in utils/fwdl.c. No execution or input edit.",
    })
    print("Static public-text audit recorded: 3 x 68 sealed sources; no behavioral test executed")


if __name__ == "__main__":
    main()
