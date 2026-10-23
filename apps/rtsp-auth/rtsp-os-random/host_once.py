# SPDX-License-Identifier: GPL-2.0-only
"""One fixed HOST call, not part of compile01; fresh attempt marker, no retry."""
from pathlib import Path
import datetime
import hashlib
import json
import subprocess

ROOT = Path(__file__).resolve().parents[3]
DOC = ROOT / "docs/qualification-plans/rtsp-os-random-20261023"
BUILD = DOC / "runs/build01"
OUT = DOC / "runs/host01"


def write(name, text):
    with (OUT / name).open("x") as stream:
        stream.write(text)


def verify(records):
    for item in records:
        path = Path(item["path"])
        if (path.stat().st_size != item["bytes"] or
                hashlib.sha256(path.read_bytes()).hexdigest() != item["sha256"]):
            raise SystemExit("STOP: pinned public build/input drift; no call")


def main():
    # Created before verification/execution. Failure never removes this marker.
    OUT.mkdir(parents=True, exist_ok=False)
    write("ATTEMPT.json", json.dumps({
        "utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "selected_callback_calls": 1, "retries": 0, "native": False}) + "\n")
    report = json.loads((BUILD / "REPORT.json").read_text())
    if report["status"] != "COMPILE_INSPECT_PASS_NOT_RUN":
        raise SystemExit("STOP: no successful compile report; no call")
    dependencies = json.loads((BUILD / "DEPENDENCIES.json").read_text())
    pins = (json.loads((BUILD / "INPUTS.json").read_text()) +
            json.loads((BUILD / "TOOLS.json").read_text()) +
            report["artifacts"] +
            [p for unit in dependencies for p in unit["files"]])
    verify(pins)
    exe = BUILD / "HOST-OS-RANDOM-ONE-CALL-NOT-NATIVE"
    argv = [str(exe)]
    env = {"LC_ALL": "C"}  # no inherited loader/config/environment selector
    write("INVOCATION.json", json.dumps({"argv": argv, "cwd": str(ROOT),
                                         "environment": env}, indent=2) + "\n")
    # Sole invocation; no loop, syscall interception, fixture or alternate path.
    result = subprocess.run(argv, cwd=ROOT, env=env, capture_output=True, timeout=10)
    write("stdout.txt", result.stdout.decode())
    write("stderr.txt", result.stderr.decode())
    expected = b"HOST_OS_RANDOM_ONE_CALL_PASS NOT_NATIVE\n"
    passed = result.returncode == 0 and result.stdout == expected and not result.stderr
    write("OBSERVATION.json", json.dumps({
        "utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "returncode": result.returncode, "scalar_pass": passed,
        "callback_calls": 1, "native_qualification": False,
        "random_output_logged_or_hashed": False}, indent=2) + "\n")
    if not passed:
        raise SystemExit("STOP: sole HOST call refused/failed; preserved, no retry")
    verify(pins)
    print("HOST_OS_RANDOM_ONE_CALL_PASS NOT_NATIVE")


if __name__ == "__main__":
    main()
