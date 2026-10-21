#!/usr/bin/env python3
"""Inspect completed offline reproduction only; no builds/target/private inputs."""
from pathlib import Path
import datetime
import hashlib
import json
import sys

if not __debug__:
    raise SystemExit("Assertions required")
ROOT = Path(__file__).resolve().parents[5]
BASE = ROOT / "evidence/delegated/20261012-native-handoff"
OLD, NEW = BASE / "run34", BASE / "runParentV6RP04A"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    old = json.loads((OLD / "manifest.json").read_text())
    new = json.loads((NEW / "manifest.json").read_text())
    assert old["state"] == new["state"] == "BUILD-candidate-HOST-fixture-passed-NOT-native-qualified"
    assert old["native_execution"] is new["native_execution"] is False
    assert old["import"] == new["import"]
    assert old["strictMemoryReserve_footprint"] == new["strictMemoryReserve_footprint"]
    assert set(old["source_inputs"]) == set(new["source_inputs"])
    for path, pin in new["source_inputs"].items():
        assert sha(Path(path)) == pin["sha256"] == old["source_inputs"][path]["sha256"]
        if "copy" in pin:
            assert sha(Path(pin["copy"])) == pin["copy_sha256"] == old["source_inputs"][path]["copy_sha256"]
    def closure(manifest, run):
        return {path.replace(str(run), "<RUN>"): pin
                for path, pin in manifest["consumed_header_source_closure"].items()}
    assert closure(old, OLD) == closure(new, NEW)
    assert old["tool_inputs"] == new["tool_inputs"]
    for path, pin in new["tool_inputs"].items():
        assert sha(Path(path)) == pin
    elf = NEW / "build/native-handoff-arm.stripped"
    assert elf.read_bytes() == (OLD / "build/native-handoff-arm.stripped").read_bytes()
    assert elf.stat().st_size == 91236
    assert sha(elf) == "64434b3c3c7dc9bae28c6ff66bddbc01a4f08a9e6972d0082b0f418036ff2319"
    expected = {
        "HOST-regression.txt": "cases=42 NOT_CAMERA_TARGET",
        "HOST-sanitized.txt": "cases=42 NOT_CAMERA_TARGET",
        "HOST-memory-reserve.txt": "35 public synthetic",
        "HOST-memory-reserve-sanitized.txt": "35 public synthetic",
        "HOST-imported-adapter.txt": "67 synthetic syscall/explicit-exact139 cases PASS",
        "HOST-imported-adapter-sanitized.txt": "67 synthetic syscall/explicit-exact139 cases PASS",
        "HOST-TCP-mocks.txt": "24 mocked cases PASS",
        "HOST-TCP-mocks-sanitized.txt": "24 mocked cases PASS",
        "PUBLICDUMMY-receiver-tests.txt": "16 parser/refuse-overwrite/owned-localhost13-12 cases PASS",
    }
    for name, marker in expected.items():
        assert marker in (NEW / "logs" / name).read_text()
    assert all(row["exit"] == 0 for row in new["commands"])
    result = {
        "utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "state": "PARENT_UNCHANGED_V6_OFFLINE_REPRODUCTION_PASS_NATIVE_HOLD",
        "argv": [sys.executable, "-B", str(Path(__file__).relative_to(ROOT))],
        "python": sys.version,
        "collector_sha256": sha(Path(__file__)),
        "build_argv": ["python3", "-B", "apps/web-media/native-producer/build.py", "runParentV6RP04A",
                       "--toolchain", "work/lightbulb-camera/implementation-20261008T205404Z/sources/toolchain-armv5-musl-202508/armv5-eabi--musl--stable-2025.08-1"],
        "build_started": new["started"], "build_ended": new["ended"],
        "manifest_sha256": {"run34": sha(OLD / "manifest.json"), "parent": sha(NEW / "manifest.json")},
        "matching_original_source_pin_entries": len(new["source_inputs"]),
        "normalized_consumed_closure_entries_match": len(closure(new, NEW)),
        "tool_inputs_entries_match": len(new["tool_inputs"]),
        "controlled_import_and_ELF_memory_footprint_unchanged": True,
        "ARM_ELF_byte_identical_to_run34": True, "ARM_ELF_bytes": elf.stat().st_size,
        "ARM_ELF_sha256": sha(elf),
        "host_cases_each_optimized_and_sanitized": {"handoff": 42, "memory": 35, "adapter": 67, "TCP": 24},
        "owned_loopback_receiver_cases": 16,
        "output_log_sha256": {name: sha(NEW / "logs" / name) for name in expected},
        "native_execution": False, "target_IO": False, "placement_policy_changed": False,
        "kernel76_adoption_or_native_memory_admission": False,
        "memory_footprint": new["strictMemoryReserve_footprint"],
    }
    with Path(__file__).with_name("result.json").open("x") as stream:
        json.dump(result, stream, indent=2)
        stream.write("\n")
    print("Unchanged v6 offline reproduction/byte-identical ARM/host suites PASS; native HOLD")


if __name__ == "__main__":
    main()
