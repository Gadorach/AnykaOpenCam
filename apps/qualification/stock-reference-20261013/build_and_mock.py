#!/usr/bin/env python3
"""Build fixed ARM payload, execute only the explicit host mock, retain closure.

All writes are confined to the user-authorized query-helper evidence directory.
An existing output directory is refused. No target/stock binary is executed.
"""
import datetime
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
SRC = Path(__file__).resolve().parent
BASE = ROOT / "evidence/delegated/20261013-stock-reference/query-helper"
TC = ROOT / ("work/lightbulb-camera/implementation-20261008T205404Z/sources/"
             "toolchain-armv5-musl-202508/armv5-eabi--musl--stable-2025.08-1")
PREFIX = TC / "bin/arm-buildroot-linux-musleabi-"


def digest(path):
    h = hashlib.sha256()
    with open(path, "rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def main():
    if len(sys.argv) != 2 or not re.fullmatch(r"build[0-9]{2}", sys.argv[1]):
        raise SystemExit("usage: build_and_mock.py buildNN (fresh output only)")
    BASE.mkdir(parents=True, exist_ok=True)
    out = BASE / sys.argv[1]
    out.mkdir(exist_ok=False)
    (out / "tmp").mkdir()
    env = {"PATH": "/usr/bin:/bin", "LC_ALL": "C", "TMPDIR": str(out / "tmp"),
           "SOURCE_DATE_EPOCH": "0", "PYTHONDONTWRITEBYTECODE": "1"}
    commands, tests = [], []
    report = {"started_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
              "status": "running", "commands": commands, "tests": tests,
              "native_executed": False, "device_operations": False}

    def run(args, label, check=True, runenv=None, executable=None):
        argv = [str(a) for a in args]
        p = subprocess.run(argv, cwd=out, env=runenv or env, executable=executable,
                           capture_output=True, text=True, timeout=30)
        (out / (label + ".stdout.txt")).write_text(p.stdout)
        (out / (label + ".stderr.txt")).write_text(p.stderr)
        commands.append({"argv": argv, "executable": executable, "cwd": str(out),
                         "label": label, "returncode": p.returncode})
        if check and p.returncode:
            raise RuntimeError(f"{label} failed: {p.stderr}")
        return p

    try:
        cc = str(PREFIX) + "gcc"
        readelf = str(PREFIX) + "readelf"
        nm = str(PREFIX) + "nm"
        size = str(PREFIX) + "size"
        common = ["-std=c11", "-Os", "-Wall", "-Wextra", "-Werror",
                  "-ffunction-sections", "-fdata-sections", "-fno-common",
                  "-fstack-usage", "-fno-ident"]
        run([cc, "--version"], "arm-compiler-version")
        run([cc, "-v"], "arm-compiler-config")
        run([readelf, "--version"], "binutils-version")
        run(["/usr/bin/cc", "--version"], "host-compiler-version")
        run([cc, *common, "-march=armv5te", "-marm", "-mfloat-abi=soft", "-MD",
             "-MF", out / "native.d", "-c", SRC / "query.c", "-o", out / "native.o"],
            "arm-compile")
        run([cc, "-static", "-no-pie", "-march=armv5te", "-marm", "-mfloat-abi=soft",
             "-Wl,--gc-sections,--build-id=none,-z,noexecstack",
             "-Wl,-Map=" + str(out / "native.map"), out / "native.o",
             "-o", out / "stock-get-armv5"], "arm-link")
        run([readelf, "-h", "-l", "-A", out / "stock-get-armv5"], "arm-elf")
        run([size, "-A", out / "stock-get-armv5"], "arm-size")
        run([nm, "-u", out / "native.o"], "application-imports")
        symbols = run([nm, out / "stock-get-armv5"], "arm-symbols").stdout
        assert not re.search(r"\b(?:q_|mock_)", symbols)
        elf = (out / "stock-get-armv5").read_bytes()
        assert elf[:6] == b"\x7fELF\x01\x01" and int.from_bytes(elf[18:20], "little") == 40
        phoff = int.from_bytes(elf[28:32], "little")
        phsize = int.from_bytes(elf[42:44], "little")
        phnum = int.from_bytes(elf[44:46], "little")
        ph_types = [int.from_bytes(elf[phoff+i*phsize:phoff+i*phsize+4], "little")
                    for i in range(phnum)]
        assert 3 not in ph_types and 2 not in ph_types  # no INTERP or DYNAMIC
        assert not re.search(r"\b(?:malloc|calloc|realloc|free)\b",
                             (out / "application-imports.stdout.txt").read_text())
        stack = (out / "native.su").read_text()
        stack_sizes = [int(line.split("\t")[1]) for line in stack.splitlines()]
        assert sum(stack_sizes) <= 4096, stack
        report["application_stack_frames_sum_bytes"] = sum(stack_sizes)
        run(["/usr/bin/cc", *common, "-DSTOCK_QUERY_MOCK", SRC / "query.c",
             SRC / "mock_syscalls.c", "-o", out / "stock-get-host-mock"], "host-mock-build")
        host = str(out / "stock-get-host-mock")
        modes = {"motor0-query": ("/dev/motor0", 0x40046d43, 36),
                 "motor1-query": ("/dev/motor1", 0x40046d43, 36),
                 "sensor-fps": ("/dev/isp_char", 0x8004497b, 4),
                 "isp-fps": ("/dev/isp_char", 0x8004494f, 24)}

        def test(case, mode="sensor-fps", expected=1, args=None, absolute=True,
                 opens=1, ioctls=1, closes=1):
            argv = args if args is not None else [host, "--mock-stock-get", mode]
            if not absolute:
                argv = ["./stock-get-host-mock", *argv[1:]]
            testenv = dict(env, MOCK_CASE=case)
            label = f"mock-{len(tests):02d}-{case}-{mode}"
            p = run(argv, label, check=False, runenv=testenv, executable=host)
            assert p.returncode == expected, (label, p.returncode, p.stderr)
            assert p.stderr.count("MOCK open ") == opens, (label, p.stderr)
            assert p.stderr.count("MOCK ioctl ") == ioctls, (label, p.stderr)
            assert p.stderr.count("MOCK close") == closes, (label, p.stderr)
            if expected == 0:
                data = json.loads(p.stdout)
                path, req, n = modes[mode]
                assert (data["path"], int(data["request"], 16), data["bytes"]) == (path, req, n)
                raw = bytes.fromhex(data["raw_hex"])
                want = bytes(n) if case == "zero" else bytes(range(1, n+1))
                if mode.startswith("motor"):
                    want = b"\0"*4 + want[4:]
                assert raw == want and len(raw) == n
                assert data["words_le_hex"] == [f"0x{int.from_bytes(raw[i:i+4], 'little'):08x}"
                                                for i in range(0, n, 4)]
                assert data["physical_fps_measured"] is False and data["close_ok"] is True
            else:
                assert p.stdout == "", (label, "failure emitted success data")
            tests.append({"label": label, "case": case, "mode": mode,
                          "exit": p.returncode, "opens": opens, "ioctls": ioctls,
                          "closes": closes, "passed": True})

        for mode in modes:
            test("success", mode, 0)
            test("shortcopy", mode)
            test("overrun", mode)
        test("zero", "sensor-fps", 0)
        test("zero", "isp-fps", 0)
        for case in ["nonroot", "euid", "wrongkernel", "unamefail", "sigfail", "maskfail",
                     "clockstart", "clockinvalid", "lstatfail", "regular", "symlink"]:
            test(case, opens=0, ioctls=0, closes=0)
        test("openfail", ioctls=0, closes=0)
        for case in ["fstatfail", "race"]:
            test(case, ioctls=0)
        for case in ["enodev", "negative", "positive", "closefail", "clockend", "backwards", "deadline"]:
            test(case)
        test("busy", "motor0-query")
        for case, code in [("alarm", 124), ("interrupt", 130), ("terminate", 143)]:
            test(case, expected=code, closes=0)
        for args in [[host], [host, "--native-stock-get", "sensor-fps"],
                     [host, "--mock-stock-get", "/dev/motor0"],
                     [host, "--mock-stock-get", "sensor-fps", "0x4004497a"],
                     [host, "--mock-stock-get", "motor-reset"]]:
            test("invalid-entry", args=args, opens=0, ioctls=0, closes=0)
        test("relative-entry", absolute=False, opens=0, ioctls=0, closes=0)

        # Header and linker closure: read only compiler-reported inputs, never firmware.
        dep = (out / "native.d").read_text().replace("\\\n", " ").split(":", 1)[1]
        inputs = {Path(p).resolve() for p in dep.split()}
        inputs.update(p.resolve() for p in SRC.iterdir() if p.is_file())
        inputs.update([Path(cc).resolve(), Path(readelf).resolve(), Path(nm).resolve(),
                       TC / "summary.csv", TC / "buildroot.config"])
        for name in ["cc1", "as", "ld"]:
            p = run([cc, "-print-prog-name=" + name], "tool-" + name).stdout.strip()
            inputs.add(Path(p).resolve())
        for line in (out / "native.map").read_text().splitlines():
            if line.startswith("LOAD "):
                p = Path(line[5:].strip()).resolve()
                if p.is_file(): inputs.add(p)
        report["input_closure"] = [{"path": str(p), "bytes": p.stat().st_size,
                                     "sha256": digest(p)} for p in sorted(inputs)]
        report["build_environment"] = env
        report["toolchain_archive_retained_sha256"] = "8cdb4ad70c6b5a66427fa3315fe3ddde1c19c90232674dec59045e04d2a36cf1"
        report["toolchain"] = {"gcc": "14.3.0", "musl": "1.2.5", "linux_headers": "5.4.296",
                                "kernel_target": "3.4.35", "native_runtime_tested": False}
        report["status"] = "host-mocks-pass-arm-static-built-not-native-qualified"
        report["native_sha256"] = digest(out / "stock-get-armv5")
        report["native_bytes"] = (out / "stock-get-armv5").stat().st_size
    except Exception as exc:
        report["status"] = "failed"
        report["failure"] = repr(exc)
        raise
    finally:
        report["finished_utc"] = datetime.datetime.now(datetime.timezone.utc).isoformat()
        (out / "build-and-mock-report.json").write_text(json.dumps(report, indent=2) + "\n")
        artifacts = []
        for p in sorted(out.iterdir()):
            if p.is_file(): artifacts.append(f"{digest(p)}  {p.name}\n")
        (out / "SHA256SUMS").write_text("".join(artifacts))
    print(json.dumps({"status": report["status"], "tests": len(tests),
                      "payload": str(out / "stock-get-armv5"),
                      "sha256": report["native_sha256"], "bytes": report["native_bytes"]}))


if __name__ == "__main__":
    main()
