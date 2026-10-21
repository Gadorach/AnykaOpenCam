#!/usr/bin/env python3
"""Validate this lane's own static evidence; never opens project/private inputs."""
import sys
if sys.flags.optimize or sys.argv[1:]:
    raise SystemExit("Refuse optimized Python or arguments")
import ast
import datetime
import hashlib
import json
from pathlib import Path
import subprocess

here = Path(__file__).resolve().parent
destination = here / "validation01.json"
if destination.exists():
    raise SystemExit("Preserve existing validation output")


def hashes():
    return {str(p.relative_to(here)): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in sorted(here.rglob("*")) if p.is_file()}


before = hashes()
script = here / "inspect_station.py"
ast.parse(script.read_text())
binding = json.loads((here / "run02/binding.json").read_text())
if not (binding["all_source_checks_pass"] and binding["check_count"] == 98
        and binding["station_source_count"] == 67
        and binding["script_sha256"] == before["inspect_station.py"]):
    raise SystemExit("Evidence/source mismatch")
commands = []
for argv, expected in (([sys.executable, "-B", str(script)], "FileExistsError"),
                       ([sys.executable, "-O", "-B", str(script)], "Refuse optimized Python")):
    result = subprocess.run(argv, capture_output=True, text=True, timeout=30)
    if result.returncode == 0 or expected not in result.stderr:
        raise SystemExit("Expected non-overwriting refusal missing")
    commands.append({"argv": argv, "exit": result.returncode,
                     "stdout": result.stdout, "stderr": result.stderr})
if before != hashes():
    raise SystemExit("Refusal mutated retained evidence")
record = {"utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
          "scope": "OWN_SOURCE_EVIDENCE_VALIDATION_ONLY",
          "python": sys.version, "syntax_parse_pass": True,
          "98_static_checks_and_67_source_pins_recorded": True,
          "exclusive_rerun_and_optimized_mode_refuse_without_changes": True,
          "commands": commands, "evidence_sha256": before,
          "report_sha256": hashlib.sha256((here.parent / "STATION76_ENTRY_REVIEW.md").read_bytes()).hexdigest(),
          "hardware_private_network_build_Git_ARM_execution": False}
with destination.open("x") as out:
    json.dump(record, out, indent=2)
    out.write("\n")
print("Own source evidence validation PASS; no target access; execution remains NO-GO")
