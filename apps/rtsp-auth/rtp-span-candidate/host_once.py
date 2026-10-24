# SPDX-License-Identifier: GPL-2.0-only
"""ONE fixed normal HOST observation after author audit; never retry or run ARM."""
from pathlib import Path
import datetime
import hashlib
import json
import subprocess

ROOT = Path(__file__).resolve().parents[3]
DOC = ROOT / "docs/qualification-plans/rtp-span-20261024"
BUILD = DOC / "runs/build01"
OUT = DOC / "runs/host01"


def dump(name, data):
    with (OUT / name).open("x") as f:
        f.write(json.dumps(data, indent=2) + "\n")


def verify(pins):
    for p in pins:
        path = Path(p["path"])
        if (path.stat().st_size != p["bytes"] or
                hashlib.sha256(path.read_bytes()).hexdigest() != p["sha256"]):
            raise SystemExit("STOP: public source/build drift; no call/retry")


def main():
    OUT.mkdir(parents=True, exist_ok=False)  # retained even if verification fails
    dump("ATTEMPT.json", {"utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
                          "normal_cases": 1, "retries": 0, "native": False})
    report = json.loads((BUILD / "REPORT.json").read_text())
    if report["status"] != "COMPILE_INSPECT_PASS_NOT_RUN":
        raise SystemExit("STOP: build incomplete; no call")
    audit = json.loads((DOC / "PRECALL_AUDIT.json").read_text())
    if audit["disposition"] != "AUTHOR_AUDITED_ONE_NORMAL_HOST_ONLY":
        raise SystemExit("STOP: no author audit; no call")
    pins = (json.loads((BUILD / "INPUTS.json").read_text()) +
            json.loads((BUILD / "DEPENDENCIES.json").read_text()) +
            json.loads((BUILD / "TOOLS.json").read_text()) + report["artifacts"] +
            audit["pins"])
    verify(pins)
    argv = [str(BUILD / "HOST-NORMAL-NOT-NATIVE")]
    env = {"LC_ALL": "C"}
    dump("INVOCATION.json", {"argv": argv, "cwd": str(ROOT), "environment": env})
    try:
        r = subprocess.run(argv, cwd=ROOT, env=env, capture_output=True, timeout=10)
        stdout, stderr, code = r.stdout, r.stderr, r.returncode
    except subprocess.TimeoutExpired as e:
        stdout, stderr, code = e.stdout or b"", e.stderr or b"", None
    with (OUT / "stdout.txt").open("xb") as f:
        f.write(stdout)
    with (OUT / "stderr.txt").open("xb") as f:
        f.write(stderr)
    expected = (b"HOST_NORMAL_PASS packets=126 nal_bytes=174080 payload_bytes=174079 "
                b"wire_bytes=176347 boundary_crossings=2 final_offset=174080 "
                b"final_sequence=80 NOT_NATIVE\n")
    passed = code == 0 and stdout == expected and not stderr
    dump("OBSERVATION.json", {"utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "returncode": code, "passed": passed, "normal_cases": 1,
        "candidate_packet_calls_expected_on_pass": 126, "native_acceptance": False,
        "payload_logged_or_hashed": False})
    if not passed:
        raise SystemExit("STOP: sole normal HOST observation failed; preserved, no retry")
    verify(pins)
    print(stdout.decode(), end="")


if __name__ == "__main__":
    main()
