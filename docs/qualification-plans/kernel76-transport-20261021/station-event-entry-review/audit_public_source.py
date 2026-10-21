#!/usr/bin/env python3
"""Read-only public-source pins/excerpts; NOT a behavioral test or source patch."""
import datetime
import hashlib
import json
from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[4]
OUT = Path(__file__).resolve().parent
DOC = ROOT / "docs/qualification-plans/kernel76-transport-20261021"
A1 = "apps/qualification/kernel76-station/source/"
A2 = "apps/qualification/kernel76-station-ingress/source/"
REPORT = DOC / "STATION76_EVENT_ENTRY_REVIEW.md"


def utc():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


def pin(blob):
    return {"bytes": len(blob), "sha256": hashlib.sha256(blob).hexdigest()}


def load(relative):
    return (ROOT / relative).read_bytes()


def exclusive_json(path, value):
    with path.open("x", encoding="utf-8") as stream:
        json.dump(value, stream, indent=2)
        stream.write("\n")


def audit():
    started = utc()
    metadata = [
        "docs/qualification-plans/kernel76-transport-20261021/"
        + name
        for name in (
            "STATION76_CONTROL_A1_REVIEW.md",
            "STATION76_INGRESS_A2A_CANDIDATE.md",
            "STATION76_ENTRY_REVIEW.md",
            "station-control-a1-host01/candidate-seal.json",
            "station-ingress-a2a-host01/candidate-seal.json",
        )
    ]
    inputs = {name: pin(load(name)) for name in metadata}
    a1seal = json.loads(load(metadata[3]))["files"]
    a2seal = json.loads(load(metadata[4]))["files"]
    a1paths = {name[len(A1):] for name in a1seal if name.startswith(A1)}
    a2paths = {name[len(A2):] for name in a2seal if name.startswith(A2)}
    assert a1paths == a2paths and len(a1paths) == 68
    changed = []
    for rel in sorted(a1paths):
        for base, seal in ((A1, a1seal), (A2, a2seal)):
            name = base + rel
            inputs[name] = pin(load(name))
            assert inputs[name] == seal[name], name
        if inputs[A1 + rel] != inputs[A2 + rel]:
            changed.append(rel)
    assert changed == [
        "hgic_def.h", "hgic_smac/core.c", "hgic_smac/hgics.h",
        "hgic_smac/rx.c", "utils/fwctrl.c", "utils/fwctrl.h", "utils/if_usb.c",
    ]
    # These event-path inputs are unchanged, not new A2a safety conclusions.
    for rel in ("LICENSE", "hgic.h", "hgic_smac/event.c", "hgic_smac/event.h",
                "hgic_smac/procfs.c", "hgic_smac/Makefile"):
        assert inputs[A1 + rel] == inputs[A2 + rel]

    ranges = {
        "hgic.h": [(110, 159), (195, 234), (415, 449), (475, 498)],
        "hgic_def.h": [(16, 21)],
        "hgic_smac/event.c": [(1, 70)],
        "hgic_smac/event.h": [(1, 9)],
        "hgic_smac/Makefile": [(1, 26)],
        "hgic_smac/rx.c": [(164, 205), (207, 258), (285, 292), (305, 324)],
        "utils/fwctrl.c": [(170, 195), (254, 284), (334, 371)],
        "utils/fwctrl.h": [(20, 50)],
        "utils/control_receipt.h": [(1, 25)],
        "utils/if_usb.c": [(224, 263)],
        "hgic_smac/core.c": [(993, 1048), (1107, 1121), (1162, 1165),
                              (1193, 1211), (1323, 1352), (1367, 1369)],
        "hgic_smac/hgics.h": [(101, 128)],
        "hgic_smac/procfs.c": [(57, 89), (194, 208)],
        "tools/test_app/hgics.c": [(1, 14), (124, 168), (171, 220)],
        "tools/test_app/hgics_relay.c": [(1, 14), (171, 181), (237, 248),
                                         (317, 419)],
    }
    excerpts = []
    for rel, spans in ranges.items():
        name = A2 + rel
        blob = load(name)
        lines = blob.splitlines(keepends=True)
        for start, end in spans:
            assert 1 <= start <= end <= len(lines), (rel, start, end)
            offset = sum(map(len, lines[:start - 1]))
            excerpt = b"".join(lines[start - 1:end])
            excerpts.append({
                "source": name, "source_pin": inputs[name],
                "start_line": start, "end_line": end,
                "start_byte": offset, "extent_bytes": len(excerpt),
                "raw_excerpt_sha256": hashlib.sha256(excerpt).hexdigest(),
                "text": excerpt.decode("utf-8"),
            })
    # Narrow public-symbol inventory, not a search of devices, private inputs,
    # firmware, objects, binaries, parent evidence, or any external directory.
    needles = ("rx_event", "HDR_EVTID", "HGIC_EVENT_", "evt_list", "evt_sema",
               "fwevnt", "hgics_event(")
    references = []
    for rel in sorted(a2paths):
        if Path(rel).suffix not in (".c", ".h"):
            continue
        for number, line in enumerate(load(A2 + rel).decode("utf-8", "replace").splitlines(), 1):
            if any(needle in line for needle in needles):
                references.append({"source": A2 + rel, "line": number, "text": line})
    for name, expected in inputs.items():
        assert pin(load(name)) == expected, name
    script = Path(__file__).resolve()
    exclusive_json(OUT / "public-excerpts.json", {
        "utc": utc(), "kind": "PUBLIC_SOURCE_EXCERPTS_NOT_BEHAVIORAL_TESTS",
        "license": "SOURCE_LICENSE_GPL-2.0.txt; original notices retained in excerpts",
        "excerpts": excerpts,
    })
    exclusive_json(OUT / "event-references.json", {
        "utc": utc(), "scope": A2, "needles": needles, "references": references,
    })
    with (OUT / "SOURCE_LICENSE_GPL-2.0.txt").open("xb") as stream:
        stream.write(load(A2 + "LICENSE"))
    exclusive_json(OUT / "reader-pins.json", {
        "started_utc": started, "finished_utc": utc(),
        "argv": sys.argv, "cwd": str(Path.cwd()), "python": sys.version,
        "reader": str(script.relative_to(ROOT)), "reader_pin": pin(script.read_bytes()),
        "kind": "STATIC_PUBLIC_SOURCE_PINS_EXCERPTS_ONLY_NO_BEHAVIOR_EXECUTED",
        "A1_source_files": 68, "A2a_candidate_source_files": 68,
        "unchanged": 61, "changed_paths_provenance_only_not_change_review": changed,
        "A2a_status": "CANDIDATE_INDEPENDENT_REVIEW_PENDING_NOT_ACCEPTED_HERE",
        "all_pins_match_existing_seals": True,
        "all_inputs_rechecked_before_output": True, "inputs": inputs,
    })
    print("Public-source audit: 68+68 source pins match; event/header/procfs unchanged; no behavioral test.")


def seal():
    pins = json.loads((OUT / "reader-pins.json").read_text())
    for name, expected in pins["inputs"].items():
        assert pin(load(name)) == expected, name
    assert pin(Path(__file__).read_bytes()) == pins["reader_pin"]
    owned = [REPORT] + sorted(p for p in OUT.iterdir() if p.is_file())
    exclusive_json(OUT / "review-seal.json", {
        "utc": utc(), "argv": sys.argv, "cwd": str(Path.cwd()),
        "python": sys.version, "all_reader_inputs_rechecked_unchanged": True,
        "kind": "EVENT_ENTRY_DESIGN_ONLY_CONDITIONAL_SOURCE_GO_NOT_IMPLEMENTATION_OR_BUILD",
        "files": {str(path.relative_to(ROOT)): pin(path.read_bytes()) for path in owned},
        "self_excluded": True,
    })
    print("Design report sealed; all 136 source pins and five public review/seal inputs unchanged.")


if __name__ == "__main__":
    if len(sys.argv) == 1:
        audit()
    elif sys.argv[1:] == ["--seal"]:
        seal()
    else:
        raise SystemExit("usage: audit_public_source.py [--seal]")
