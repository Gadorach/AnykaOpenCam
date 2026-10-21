#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Offline bounded C CLI build: fresh own runNN, read-only inputs, no target."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

SOURCE = Path(__file__).resolve().parent
ROOT = SOURCE.parents[2]
LANE = ROOT / "evidence/delegated/20261016-optical-cli"
TOOLCHAIN = ROOT / "work/lightbulb-camera/implementation-20261008T205404Z/sources/toolchain-armv5-musl-202508/armv5-eabi--musl--stable-2025.08-1"
OWN = ("optical_cli.c", "test_cli.c", "test_cli.py", "build_cli.py", "CLI_TRANSPORT.md")
SEALED = ("qc07f_optical.c", "optical_uapi.h", "optical_policy.h", "optical.py",
          "host_policy_test.c", "test_transport.py", "build_offline.py", "Makefile")
PINS = {
    "bin/toolchain-wrapper": "f4db963a226e01244ce6f7b5d1ec6d57474dd811119ff8834429f022835ce9c1",
    "bin/arm-buildroot-linux-musleabi-gcc.br_real": "e24ea223e7b521cce1f5889d63c25417e930cdbb35653b072f7b48d9a4b41729",
    "arm-buildroot-linux-musleabi/sysroot/lib/libc.a": "0982970abc52a36d247326a8e0b5dd487410222ff9728528cc99a4e8bd62e7b5",
    "lib/gcc/arm-buildroot-linux-musleabi/14.3.0/libgcc.a": "f707f3a5a94033cbbde76f76640d56dbc5dc615ab421be4efc2641c14344f19a",
}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def now():
    return datetime.now(timezone.utc).isoformat()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("run", help="unused runNN name; no overwrites")
    args = parser.parse_args()
    if not re.fullmatch(r"run[0-9]{2,}", args.run):
        parser.error("fresh runNN name required")
    LANE.mkdir(parents=True, exist_ok=True)
    if LANE.resolve() != LANE:
        parser.error("lane symlinks forbidden")
    out = LANE / args.run
    out.mkdir(exist_ok=False)
    for folder in ("input", "logs", "tmp"):
        (out / folder).mkdir()
    report = {"started_utc": now(), "invocation": [sys.executable, *sys.argv],
              "scope": "OFFLINE ONLY; no hardware/native execution or staging",
              "result": "FAILED", "commands": [], "inputs_sha256": {},
               "toolchain_pins": PINS, "native_executed": False}
    report["receipt_contract"] = {
        "lease_modes": ["none", "fork", "self"],
        "gui_ir_on_argv": ["--ir-on-hold", "LOWERCASE32HEX_NONCE"],
        "gui_ir_on_action": "ir-on", "gui_ir_on_lease_mode": "self",
        "gui_ir_on_keeper_pid": 0, "historical_fork_gui_admitted": False,
        "self_launch": "direct background job of owned persistent interactive ash",
        "native_ash_reaping_qualified": False,
    }
    env = {"PATH": os.environ.get("PATH", "/usr/bin:/bin"), "LC_ALL": "C",
           "TMPDIR": str(out / "tmp"), "PYTHONDONTWRITEBYTECODE": "1",
           "ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1",
           "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"}
    before = {name: sha(SOURCE / name) for name in (*OWN, *SEALED)}

    def run(label, command, timeout=60):
        argv = [str(arg) for arg in command]
        sandbox = ["bwrap", "--ro-bind", "/", "/", "--dev", "/dev",
                   "--bind", str(out), str(out), "--unshare-net", "--die-with-parent", "--", *argv]
        log = out / "logs" / (label + ".txt")
        entry = {"argv": sandbox, "cwd": str(out), "timeout_seconds": timeout,
                 "started_utc": now(), "log": str(log.relative_to(out))}
        report["commands"].append(entry)
        with log.open("xb") as stream:
            result = subprocess.run(sandbox, cwd=out, env=env, stdout=stream,
                                    stderr=subprocess.STDOUT, timeout=timeout)
        entry.update(returncode=result.returncode, finished_utc=now(), sha256=sha(log))
        if result.returncode:
            raise RuntimeError(f"{label} failed; preserved {log}")
        return log.read_text()

    try:
        for name, digest in before.items():
            shutil.copyfile(SOURCE / name, out / "input" / name)
            if sha(out / "input" / name) != digest:
                raise RuntimeError("copy/hash disagreement")
            report["inputs_sha256"][name] = digest
        for rel, digest in PINS.items():
            if sha(TOOLCHAIN / rel) != digest:
                raise RuntimeError("toolchain pin mismatch: " + rel)
        report["license_metadata"] = {
            "summary_sha256": sha(TOOLCHAIN / "summary.csv"),
            "selected_lines": [line for line in (TOOLCHAIN / "summary.csv").read_text().splitlines()
                               if line.startswith(('"gcc-final",', '"musl",', '"binutils",'))],
            "own_sources": "GPL-2.0-or-later; existing UAPI SPDX retained byte-exact",
        }
        armcc = TOOLCHAIN / "bin/arm-buildroot-linux-musleabi-gcc"
        readelf = TOOLCHAIN / "bin/arm-buildroot-linux-musleabi-readelf"
        for label, cc in (("host", Path(shutil.which("clang"))), ("arm", armcc)):
            report.setdefault("tools", {})[label] = {
                "version": run(label + "-version", [cc, "--version"]),
                "target": run(label + "-target", [cc, "-dumpmachine"]).strip(),
                "sha256": sha(cc.resolve()),
            }
        for tool in ("python3", "bwrap"):
            run(tool + "-version", [tool, "--version"])
        common = ["-std=c11", "-Wall", "-Wextra", "-Werror", "-Wconversion", "-Wshadow",
                  "-Wformat=2", "-Wstrict-prototypes", "-fno-common"]
        for label, flags in (("optimized", ["-O2"]),
                             ("sanitized", ["-O1", "-g", "-fsanitize=address,undefined",
                                            "-fno-omit-frame-pointer"])):
            host = out / ("host-" + label + "-mock")
            run(label + "-build", ["clang", *common, *flags, out / "input/test_cli.c", "-o", host])
            tests = run(label + "-tests", ["python3", "-B", out / "input/test_cli.py", host], timeout=30)
            match = re.search(r"^CLI_JSON_PASS receipts=(\d+) host_cases=(\d+) actual_uapi_fields=18$",
                              tests, re.M)
            if not match:
                raise RuntimeError("missing validated mock summary")
            report.setdefault("host_tests", {})[label] = {
                "receipts": int(match[1]), "cases": int(match[2]), "actual_uapi_fields": 18,
            }
        binary = out / "qc07f-optical-cli"
        obj = out / "optical_cli.o"
        armflags = ["-Os", "-march=armv5te", "-marm", "-mfloat-abi=soft"]
        run("arm-object-build", [armcc, *common, *armflags, "-c", out / "input/optical_cli.c", "-o", obj])
        run("arm-object-readelf", [readelf, "-h", "-A", obj])
        run("arm-static-build", [armcc, *armflags, "-static", "-Wl,--build-id=none",
                                 "-Wl,-Map=" + str(out / "arm-static.map"),
                                 obj, "-o", binary])
        elf = run("arm-readelf", [readelf, "-h", "-A", "-l", "-d", binary])
        if "Machine:                           ARM" not in elf or "INTERP" in elf or "NEEDED" in elf:
            raise RuntimeError("unexpected ELF machine/dynamic loader")
        report["binary"] = {"path": str(binary.relative_to(ROOT)), "sha256": sha(binary),
                            "bytes": binary.stat().st_size, "executed": False}
        report["object_sha256"] = sha(obj)
        report["runtime_attributes_limit"] = (
            "Pinned runtime merges v5TEJ/Thumb-2 metadata; own ARM-mode v5TE object is separately inspected. "
            "Attributes alone are not native ISA/ABI acceptance.")
        report["result"] = "BUILD_AND_CLI_MODEL_PASS_NOT_NATIVE"
    except BaseException as error:
        report["failure"] = {"type": type(error).__name__, "message": str(error)}
        raise
    finally:
        report["source_unchanged"] = before == {name: sha(SOURCE / name) for name in before}
        if not report["source_unchanged"]:
            report["result"] = "FAILED_INPUT_CHANGED"
        report["finished_utc"] = now()
        with (out / "REPORT.json").open("x") as stream:
            json.dump(report, stream, indent=2)
            stream.write("\n")
    if report["result"] != "BUILD_AND_CLI_MODEL_PASS_NOT_NATIVE":
        raise RuntimeError(report["result"])
    print(report["result"], report["binary"])


if __name__ == "__main__":
    main()
