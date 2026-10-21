#!/usr/bin/env python3
"""Build ARM only; execute solely the in-memory host IO implementation.

Fresh evidence/work directories are mandatory. No device/proc/sys/private input,
network, serial or target operation is part of this recipe.
"""
import datetime as dt
import difflib
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[4]
SRC = Path(__file__).resolve().parent
IMPL = ROOT / "work/lightbulb-camera/implementation-20261008T205404Z"
TC = IMPL / "sources/toolchain-armv5-musl-202508/armv5-eabi--musl--stable-2025.08-1/bin"
UPSTREAM = IMPL / "sources/busybox-1.37.0/busybox-1.37.0"


def sha(path):
    h = hashlib.sha256()
    with path.open("rb") as f:
        for data in iter(lambda: f.read(65536), b""):
            h.update(data)
    return h.hexdigest()


def main():
    if len(sys.argv) != 2 or not re.fullmatch(r"build[0-9]{2}", sys.argv[1]):
        raise SystemExit("usage: build_and_mock.py buildNN (FRESH outputs only)")
    os.umask(0o077)
    out = ROOT / "evidence/delegated/20261015-resume/root-programmer01" / sys.argv[1]
    work = ROOT / "work/qualification/root-programmer-20261015" / sys.argv[1]
    out.mkdir(parents=True, exist_ok=False)
    work.mkdir(parents=True, exist_ok=False)
    (work / "tmp").mkdir()
    snap = out / "source"
    snap.mkdir()
    files = ["root_programmer.c", "mock_io.h", "mock_io.c", "build_and_mock.py", "LICENSE"]
    for name in files:
        (snap / name).write_bytes((SRC / name).read_bytes())
    (snap / "upstream-flashcp.c").write_bytes((UPSTREAM / "miscutils/flashcp.c").read_bytes())
    (out / "upstream-adaptation.diff").write_text("".join(difflib.unified_diff(
        (snap / "upstream-flashcp.c").read_text().splitlines(True),
        (snap / "root_programmer.c").read_text().splitlines(True),
        fromfile="BusyBox-1.37.0/miscutils/flashcp.c", tofile="root_programmer.c")))
    assert (SRC / "LICENSE").read_bytes() == (UPSTREAM / "LICENSE").read_bytes()
    env = {"PATH": "/usr/bin:/bin", "LC_ALL": "C", "TMPDIR": str(work / "tmp"),
           "SOURCE_DATE_EPOCH": "0", "PYTHONDONTWRITEBYTECODE": "1"}
    commands, tests = [], []
    report = {"started_utc": dt.datetime.now(dt.timezone.utc).isoformat(), "argv": sys.argv,
              "python": sys.version, "cwd": str(Path.cwd()), "work": str(work),
              "status": "RUNNING", "commands": commands, "tests": tests,
              "source_sha256": {name: sha(SRC / name) for name in files},
              "upstream_flashcp": {"path": str(UPSTREAM / "miscutils/flashcp.c"),
                                   "sha256": sha(UPSTREAM / "miscutils/flashcp.c")},
              "native_executed": False, "hardware_access": False,
              "private_inputs_accessed": False}

    def run(argv, label, timeout=45, extra=None):
        argv = [str(x) for x in argv]
        p = subprocess.run(argv, cwd=work, env=dict(env, **(extra or {})),
                           capture_output=True, text=True, timeout=timeout)
        (out / (label + ".stdout")).write_text(p.stdout)
        (out / (label + ".stderr")).write_text(p.stderr)
        commands.append({"argv": argv, "cwd": str(work), "env": dict(env, **(extra or {})),
                         "returncode": p.returncode, "label": label})
        if p.returncode:
            raise RuntimeError(f"{label}: exit{p.returncode}: {p.stderr[:2000]}")
        return p.stdout

    try:
        cc = TC / "arm-buildroot-linux-musleabi-gcc"
        readelf = TC / "arm-buildroot-linux-musleabi-readelf"
        nm = TC / "arm-buildroot-linux-musleabi-nm"
        common = ["-std=c11", "-Os", "-Wall", "-Wextra", "-Werror", "-Wformat=2",
                  "-ffunction-sections", "-fdata-sections", "-fno-common", "-fstack-usage", "-fno-ident"]
        report["tool_sha256"] = {str(p): sha(p) for p in [cc, cc.with_name(cc.name + ".br_real"), readelf, Path("/usr/bin/cc")]}
        run([cc, "--version"], "arm-compiler-version")
        run([cc, "-v"], "arm-compiler-config")
        run(["/usr/bin/cc", "--version"], "host-compiler-version")
        run([readelf, "--version"], "binutils-version")
        run([cc, *common, "-march=armv5te", "-marm", "-mfloat-abi=soft", "-MD",
             "-MF", work / "native.d", "-c", snap / "root_programmer.c", "-o", work / "native.o"], "arm-compile")
        binary = work / "qc07f-root-programmer-armv5"
        run([cc, "-static", "-no-pie", "-march=armv5te", "-marm", "-mfloat-abi=soft",
             "-Wl,--gc-sections,--build-id=none,-z,noexecstack", "-Wl,-Map=" + str(work / "native.map"),
             work / "native.o", "-o", binary], "arm-link")
        run([readelf, "-h", "-l", "-A", binary], "arm-elf")
        run([TC / "arm-buildroot-linux-musleabi-size", "-A", binary], "arm-size")
        imports = run([nm, "-u", work / "native.o"], "application-imports")
        symbols = run([nm, "-S", binary], "arm-symbols")
        assert not re.search(r"\b(?:r_open|r_ioctl|mock_|malloc|calloc|realloc|mmap)\b", imports)
        assert not re.search(r"\b(?:r_open|r_ioctl|root_programmer_mock)\b", symbols)
        for name in ("buf", "buf2"):
            assert re.search(r"\b00001000 [bB] " + name + r"$", symbols, re.M), name
        stack_text = (work / "native.su").read_text()
        frames = [int(line.split("\t")[1]) for line in stack_text.splitlines()]
        report["stack_max_frame_bytes"] = max(frames)
        report["stack_all_application_frames_sum_bytes"] = sum(frames)
        report["static_copy_buffers_bytes"] = 8192
        assert max(frames) <= 2048 and sum(frames) <= 8192
        elf = binary.read_bytes()
        assert elf[:6] == b"\x7fELF\x01\x01" and int.from_bytes(elf[18:20], "little") == 40
        off, size, num = int.from_bytes(elf[28:32], "little"), int.from_bytes(elf[42:44], "little"), int.from_bytes(elf[44:46], "little")
        types = [int.from_bytes(elf[off + i * size:off + i * size + 4], "little") for i in range(num)]
        assert 2 not in types and 3 not in types
        report["arm_binary"] = {"path": str(binary), "bytes": len(elf), "sha256": sha(binary),
                                "static_armv5_little_endian": True, "executed": False}
        # Compiler dependencies are public headers only; record exact ABI inputs.
        deps = (work / "native.d").read_text().replace("\\\n", " ").split(":", 1)[1].split()
        report["compiler_input_sha256"] = {str(Path(p).resolve()): sha(Path(p)) for p in deps}

        base = ["--image", "/mock/input", "--expected-erasesize", "65536", "--board-nor-root-only"]
        program = base + ["--program-root-only", "--backup", "/mock/backup"]
        failures = [
            "uid", "euid", "signals", "core-limit", "name-open", "name-wrong", "cmdline-wrong", "cmdline-duplicate", "cmdline-missing",
            "proc-size", "proc-erase", "proc-name", "proc-extra", "root-stat", "root-statfs", "root-wrong-fs", "root-other-ram",
            "root-mtd", "root-unknown", "root-alias", "root-mounted", "bind-mounted", "bind-escaped", "mount-stat", "mount-escape", "mount-stacked",
            "metadata-read", "metadata-bound", "metadata-nul", "metadata-fs", "image-symlink", "image-stat", "image-small", "image-large",
            "image-owner", "image-mode", "image-fifo", "image-hardlink", "image-change", "image-truncate", "image-grow", "image-busy",
            "device-open", "device-stat", "device-regular", "device-owner", "device-major", "device-minor", "info-fail", "nand", "unknown-type",
            "info-size", "info-erase", "info-writesize", "info-oob", "info-readonly", "owner-busy", "root-short",
            "backup-parent-open", "backup-parent-mode", "backup-parent-owner", "backup-parent-fs", "backup-exists", "backup-open",
            "backup-mode", "backup-fs", "backup-write", "backup-short", "backup-verify", "backup-sync", "backup-dir-sync", "interrupt-backup", "mount-late",
        ]
        late = ["writable-open", "writable-minor", "writable-inode", "writable-geometry", "mount-preerase", "image-change-preerase", "backup-changed", "flash-changed"]
        destructive_failures = ["erase-fail", "interrupt-erase", "write-fail", "write-short", "readback-io", "readback-mismatch",
                                "image-change-written", "mtd-sync-einval", "mtd-sync-eio", "close-fail", "receipt-fail"]

        for flavor, flags in [("optimized", []), ("sanitized", ["-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-no-pie"])]:
            host = work / ("host-mock-" + flavor)
            run(["/usr/bin/cc", *common, *flags, "-DROOT_PROGRAMMER_MOCK", snap / "root_programmer.c", snap / "mock_io.c",
                 "-Wl,--gc-sections", "-o", host], "host-build-" + flavor)

            def test(case, args, expected, *, zero_rw=False, no_erase=False):
                label = f"test-{flavor}-{len(tests):03d}-{case}"
                data = json.loads(run([host, case, *args], label, extra={"ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1", "UBSAN_OPTIONS": "halt_on_error=1"}))
                assert data["bad"] == 0 and data["returncode"] == expected, (label, data)
                assert data["opened"] == data["closed"], (label, data)
                assert data["max_io"] <= 4096 and data["max_flash_end"] <= 1048576, (label, data)
                if expected:
                    assert "/mock" not in data["errors"] and "0x200000,0x300000" in data["errors"]
                else:
                    assert not data["errors"]
                if zero_rw:
                    assert data["rw_open"] == data["erase_calls"] == data["write_calls"] == 0, (label, data)
                if no_erase:
                    assert data["erase_calls"] == data["write_calls"] == 0, (label, data)
                if not expected:
                    receipt = json.loads(data["out"])
                    assert receipt["native_qualified"] is False and receipt["bytes"] == 1048576
                    if "--program-root-only" in args:
                        assert data["flash_equals_image"] and data["backup_equals_original"]
                        assert data["written"] == data["readback"] == receipt["bytes_verified"] == 1048576
                        assert data["write_calls"] == 256 and data["mtd_sync"] == 1
                        assert data["backup_reads"] == 2097152 and receipt["backup_verified"]
                        assert data["erase_calls"] == 1048576 // receipt["erasesize"]
                    else:
                        assert receipt["result"] == "CHECK_ONLY" and not receipt["backup_verified"]
                        assert data["creates"] == data["backup_written"] == data["rw_open"] == 0 and data["flash_unchanged"]
                else:
                    assert not data["out"], (label, data)
                    if data["creates"]:
                        assert data["backup_size"] >= 0  # no unlink exists in either implementation
                    if case in ("mtd-sync-einval", "mtd-sync-eio"):
                        assert data["readback"] == 1048576 and data["flash_equals_image"] and data["backup_equals_original"]
                    if case == "erase-fail": assert data["erase_calls"] == 3 and data["blocks_erased"] == 2 and not data["write_calls"]
                    if case == "interrupt-erase": assert data["erase_calls"] == 2 and not data["write_calls"]
                    if case in ("write-fail", "write-short"): assert data["write_calls"] == 3
                tests.append({"flavor": flavor, "case": case, "passed": True, "result": data})

            test("default-check", base, 0, zero_rw=True)
            test("explicit-check", base + ["--check"], 0, zero_rw=True)
            for case in ("success", "tmpfs", "ramfs"):
                test(case, program, 0)
            for case, erase in (("erase4k", "4096"), ("erase1m", "1048576")):
                args = list(program); args[3] = erase
                test(case, args, 0)
            for case in failures:
                test(case, program, 1, zero_rw=True)
            for case in late:
                test(case, program, 1, no_erase=True)
            for case in destructive_failures:
                test(case, program, 1)
            invalid = [[], base[:-1], base + ["--check", "--program-root-only", "--backup", "/mock/backup"],
                       base + ["--backup", "/mock/backup"], program + ["--device", "/dev/mtd0"],
                       program + ["--unlock"], program + ["--image", "/mock/input"], program + ["--expected-erasesize", "65536"],
                       program[:-2], ["--image", "relative", *program[2:]], ["--image", "/mock/\ninput", *program[2:]]]
            for value in ("0", "3", "65535", "2097152", "-1", "+65536", "0x10000", "", "999999999999999999999999999999999"):
                args = list(program); args[3] = value; invalid.append(args)
            for index, args in enumerate(invalid):
                test("arguments-" + str(index), args, 1, zero_rw=True)
        report["status"] = "PASS_SOURCE_BUILD_HOST_MOCK_ONLY_NATIVE_PROGRAMMING_UNQUALIFIED"
        report["test_counts"] = {flavor: sum(t["flavor"] == flavor for t in tests) for flavor in ("optimized", "sanitized")}
    finally:
        report["finished_utc"] = dt.datetime.now(dt.timezone.utc).isoformat()
        report["source_unchanged"] = {name: sha(SRC / name) == report["source_sha256"][name] for name in files}
        report["work_outputs_sha256"] = {p.name: sha(p) for p in work.iterdir() if p.is_file()}
        report["evidence_outputs_sha256"] = {p.name: sha(p) for p in out.iterdir() if p.is_file()}
        (out / "REPORT.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"status": report["status"], "test_counts": report.get("test_counts"),
                      "arm_binary": report.get("arm_binary"), "report": str(out / "REPORT.json")}, indent=2))


if __name__ == "__main__":
    main()
