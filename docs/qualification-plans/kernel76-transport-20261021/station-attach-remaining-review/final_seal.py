#!/usr/bin/env python3
"""Static custody closeout only. Does not import or execute the audit/inputs."""
import datetime
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[4]
OUT = Path(__file__).resolve().parent
DOC = OUT.parent
REPORT = DOC / "STATION76_ATTACH_REMAINING_SOURCE_PLAN.md"


def pin(path):
    data = path.read_bytes()
    return {"path": str(path.relative_to(ROOT)), "offset": 0, "bytes": len(data),
            "sha256": hashlib.sha256(data).hexdigest()}


def main():
    if len(sys.argv) != 1 or sys.flags.optimize:
        raise SystemExit("No arguments/optimized mode")
    dest = OUT / "final-seal.json"
    if dest.exists():
        raise SystemExit("Preserve existing exclusive seal")
    receipt = json.loads((OUT / "reader-pins.json").read_text())
    for row in receipt["source_pins"] + receipt["input_pins"] + [receipt["excerpts_pin"]]:
        if pin(ROOT / row["path"]) != row:
            raise SystemExit(f"Original public input changed: {row['path']}")
    review = DOC / "STATION76_EVENT_A2B_REVIEW.md"
    review_seal = DOC / "station-event-a2b-review01/review-seal.json"
    inventory = json.loads(review_seal.read_text())["files"]
    row = pin(review)
    expected = inventory[row["path"]]
    if any(row[k] != expected[k] for k in ("bytes", "sha256")):
        raise SystemExit("New A2b review report differs from its seal")
    # Compare each explicit source/config/symvers hash in the authored report to
    # its already-pinned input; this is document provenance, not a safety check.
    text = REPORT.read_text()
    for line in text.splitlines():
        if line.startswith("| S/`") or line.startswith("| B/`"):
            name = line.split("`")[1]
            if line.startswith("| S/"):
                path = Path("apps/qualification/kernel76-station-events/source") / name
            else:
                path = Path("work/lightbulb-camera/foundation-20261011T041148Z/analysis/native-usb76-package01/build") / name
            if pin(ROOT / path)["sha256"] not in line:
                raise SystemExit(f"Report pin typo: {path}")
    files = [REPORT, review, review_seal] + sorted(p for p in OUT.iterdir() if p.is_file())
    result = {
        "utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "argv": sys.argv, "cwd": str(Path.cwd()), "python": sys.version,
        "scope": "SOURCE_PLAN_STATIC_CUSTODY_ONLY_NO_IMPLEMENTATION_HOST_TEST_BUILD_HARDWARE",
        "all_204_original_source_pins_and_original_public_inputs_rechecked": True,
        "A2b_new_independent_review_report_matches_review_seal": True,
        "A2b_dependency": "EXACT_PIN_NARROW_GO_READ_NOT_BEHAVIORALLY_REPRODUCED_HERE",
        "next_recommendation": "P1_ATTACH_ONLY_RX_TYPE_EXCLUSION_REQUIRES_PARENT_SELECTION",
        "preserved_tool_failures": [
            "Initial static read used nonexistent utils/bootdl.c; actual file utils/fwdl.c subsequently read.",
            "Two report update patches refused mismatching context atomically; no input changed. Correct exact-context patch then applied only to the owned new report."
        ],
        "files": [pin(p) for p in files],
        "original_HOLD_not_accessed_or_modified": True,
        "no_target_network_private_input_binary_build_test_or_Git_operation": True,
        "no_HARDWARE_queue": True,
    }
    with dest.open("x") as f:
        json.dump(result, f, indent=2)
        f.write("\n")
    print("Static custody sealed; 204 original source pins unchanged; A2b GO report pinned; no tests executed")


if __name__ == "__main__":
    main()
