# SPDX-License-Identifier: GPL-2.0-only
"""ONE normal HOST invocation after author audit. Exclusive attempt; no retry."""
from pathlib import Path
import datetime
import hashlib
import json
import re
import subprocess

ROOT = Path(__file__).resolve().parents[3]
DOC = ROOT / "docs/qualification-plans/span-hub-20261024"
BUILD = DOC / "runs/build01"
OUT = DOC / "runs/host01"


def dump(name, value):
    with (OUT / name).open("x") as f:
        f.write(json.dumps(value, indent=2) + "\n")


def verify(pins):
    for item in pins:
        p = Path(item["path"])
        if (p.stat().st_size != item["bytes"] or
                hashlib.sha256(p.read_bytes()).hexdigest() != item["sha256"]):
            raise SystemExit("STOP: public input/build drift; no call/retry")


def main():
    OUT.mkdir(parents=True, exist_ok=False)
    dump("ATTEMPT.json", {"utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "normal_cases": 1, "maximum_process_invocations": 1, "retries": 0, "native": False})
    report = json.loads((BUILD / "REPORT.json").read_text())
    audit = json.loads((DOC / "PRECALL_AUDIT.json").read_text())
    if (report["status"] != "COMPILE_REL_INSPECT_PASS_NOT_RUN" or
            audit["disposition"] != "AUTHOR_AUDITED_ONE_NORMAL_HOST_ONLY"):
        raise SystemExit("STOP: missing complete build/author audit; no call")
    pins = (json.loads((BUILD / "INPUTS.json").read_text()) +
            json.loads((BUILD / "DEPENDENCIES.json").read_text()) +
            json.loads((BUILD / "TOOLS.json").read_text()) + report["artifacts"] + audit["pins"])
    verify(pins)
    argv = [str(BUILD / "HOST-NORMAL-NOT-NATIVE")]
    env = {"LC_ALL": "C"}
    dump("INVOCATION.json", {"argv": argv, "cwd": str(ROOT), "environment": env,
        "timeout_seconds": 10})
    try:
        result = subprocess.run(argv, cwd=ROOT, env=env, capture_output=True, timeout=10)
        stdout, stderr, code = result.stdout, result.stderr, result.returncode
    except subprocess.TimeoutExpired as error:
        stdout, stderr, code = error.stdout or b"", error.stderr or b"", None
    with (OUT / "stdout.txt").open("xb") as f:
        f.write(stdout)
    with (OUT / "stderr.txt").open("xb") as f:
        f.write(stderr)
    match = re.fullmatch(
        rb"HOST_NORMAL_PASS allocations=8 total_requested=1048576 max_request=131072 "
        rb"hub_bytes=(\d+) view_bytes=(\d+) cursor_bytes=(\d+) clients=4 packets=504 "
        rb"nal_bytes=174080 checked_body_bytes=696316 wire_bytes=705388 "
        rb"drained=1 disposed=1 NOT_NATIVE\n", stdout)
    sizes = [int(x) for x in match.groups()] if match else []
    passed = (code == 0 and not stderr and len(sizes) == 3 and
              0 < sizes[0] < 4096 and 0 < sizes[1] < 4096 and 1404 < sizes[2] < 4096)
    dump("OBSERVATION.json", {"utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "returncode": code, "passed": passed, "normal_cases": 1,
        "packet_calls_on_pass": 504, "hub_publish_calls_on_pass": 1,
        "HOST_structure_sizes_on_matching_stdout": sizes, "native_acceptance": False,
        "payload_logged_or_hashed": False})
    if not passed:
        raise SystemExit("STOP: sole HOST observation failed; preserve outputs, no retry")
    verify(pins)
    print(stdout.decode(), end="")


if __name__ == "__main__":
    main()
