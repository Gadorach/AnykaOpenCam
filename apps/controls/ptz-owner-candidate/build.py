#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Isolated HOST/cross-link evidence; writes only the authorized evidence lane."""
import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[3]
APP = Path(__file__).resolve().parent
LANE = ROOT / "evidence/delegated/20261012-ptz-owner"
ARM_PINS = {
    "bin/toolchain-wrapper": "f4db963a226e01244ce6f7b5d1ec6d57474dd811119ff8834429f022835ce9c1",
    "bin/arm-buildroot-linux-musleabi-gcc.br_real": "e24ea223e7b521cce1f5889d63c25417e930cdbb35653b072f7b48d9a4b41729",
    "arm-buildroot-linux-musleabi/sysroot/lib/libc.a": "0982970abc52a36d247326a8e0b5dd487410222ff9728528cc99a4e8bd62e7b5",
    "lib/gcc/arm-buildroot-linux-musleabi/14.3.0/libgcc.a": "f707f3a5a94033cbbde76f76640d56dbc5dc615ab421be4efc2641c14344f19a",
}
OWN_NAMES = ["ptz_auth.h", "ptz_auth.c", "ptz_owner.h", "ptz_owner.c", "ptz_owner_test.c", "header_link.cpp", "size_probe.c", "README.md", "build.py"]
SHARED = ["framework/include/camera/backend.h", "framework/include/camera/access_policy.h", "framework/src/backend.c", "framework/src/access_policy.c"]
ANNOTATIONS = ["AGENTS.md", "docs/plan/camera-foundation/00-INDEX.md", "docs/plan/camera-foundation/11-PTZ-AND-PERIPHERALS.md",
 "docs/plan/camera-foundation/PARALLEL_WORK_20261012.md", "docs/qualification-plans/peripherals-audio-nvr/MATRIX.md",
 "docs/qualification-plans/peripherals-audio-nvr/CAMPAIGNS.md", "bringup/qc07f/PTZ_DIAGNOSTIC_PLAN.md",
 "bringup/qc07f/mainline/arch/arm/mach-ak3918ev200/ptz.c"]


def now():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("run")
    parser.add_argument("--toolchain", type=Path, required=True)
    args = parser.parse_args()
    args.toolchain = args.toolchain.expanduser().resolve()
    if not args.run or not all(c.isalnum() or c in "-_" for c in args.run):
        parser.error("run must be a fresh simple name")
    LANE.mkdir(parents=True, exist_ok=True)
    if LANE.resolve() != LANE:
        raise RuntimeError("Evidence symlink refused")
    out = LANE / args.run
    out.mkdir()
    for name in ("snapshot", "logs", "tmp", "objects"):
        (out / name).mkdir()
    env = {"PATH": "/usr/bin:/bin", "LC_ALL": "C", "SOURCE_DATE_EPOCH": "1791763200", "TMPDIR": str(out / "tmp"),
           "ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1", "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"}
    report = {"started_utc": now(), "command": ["python3", str(Path(__file__).relative_to(ROOT)), *os.sys.argv[1:]],
              "cwd": str(ROOT), "python": os.sys.version, "state": "RUNNING", "tasks": ["CTRL-02", "CTRL-04", "CTRL-03/10 prerequisites"],
              "evidence_class": "SOURCE + PUBLICDUMMY HOST; ARM LINK ONLY", "environment": env,
              "candidate_version": "v2-malformed-client-authority",
              "native_executed": False, "physical_movement_tested": False, "integrated": False,
              "CTRL_complete": False, "G4": False, "G8": False, "commands": [], "inputs": {}, "binaries": {}}
    sources = [APP / n for n in OWN_NAMES] + [ROOT / p for p in SHARED]

    def run(argv, label, timeout=120, expected_exit=0):
        argv = list(map(str, argv))
        log = out / "logs" / (label + ".txt")
        entry = {"argv": argv, "cwd": str(out), "started_utc": now(), "log": str(log.relative_to(ROOT)), "timeout_seconds": timeout,
                 "expected_exit_code": expected_exit}
        report["commands"].append(entry)
        with log.open("xb") as f:
            result = subprocess.run(argv, cwd=out, env=env, stdout=f, stderr=subprocess.STDOUT, timeout=timeout)
        entry.update(exit_code=result.returncode, finished_utc=now(), sha256=sha(log))
        if result.returncode != expected_exit:
            raise RuntimeError("Failed command preserved: " + str(log))
        return log.read_text()

    def compiler(cc, label):
        report.setdefault("compilers", {})[label] = {"version": run([cc, "--version"], label + "-version"),
            "target": run([cc, "-dumpmachine"], label + "-target").strip(), "executable_sha256": sha(Path(cc).resolve())}
        for item in ("cc1", "cc1plus", "as", "ld", "libgcc.a", "libstdc++.so", "libasan.so", "libubsan.so", "crt1.o", "Scrt1.o", "crti.o", "crtbeginS.o", "crtbeginT.o", "crtend.o", "crtn.o", "libc.a", "libc.so"):
            flag = "-print-prog-name=" if item in ("cc1", "cc1plus", "as", "ld") else "-print-file-name="
            value = run([cc, flag + item], label + "-closure-" + item.replace(".", "_")).strip()
            p = Path(value) if "/" in value else Path(shutil.which(value, path=env["PATH"]) or value)
            if p.is_file():
                report.setdefault("compiler_runtime_closure", {})[str(p.resolve())] = sha(p)

    try:
        preservation = LANE / "v1-snapshot/PRESERVATION.json"
        v1 = LANE / "v1-snapshot/apps/controls/ptz-owner-candidate/ptz_owner.c"
        preserved = json.loads(preservation.read_text())
        expected_v1 = preserved["files"]["apps/controls/ptz-owner-candidate/ptz_owner.c"]["sha256"]
        if sha(v1) != expected_v1:
            raise RuntimeError("Preserved V1 owner source mismatch")
        sources.append(v1)
        report["v1_preservation_manifest_sha256"] = sha(preservation)
        before = {str(p.relative_to(ROOT)): sha(p) for p in sources}
        for p in sources + [ROOT / p for p in ANNOTATIONS]:
            relative = p.relative_to(ROOT)
            if p.is_symlink():
                raise RuntimeError("Source symlink refused: " + str(relative))
            dest = out / "snapshot" / relative
            dest.parent.mkdir(parents=True, exist_ok=True)
            with dest.open("xb") as f:
                f.write(p.read_bytes())
            report["inputs"][str(relative)] = {"sha256": sha(dest), "bytes": dest.stat().st_size,
                "copy": str(dest.relative_to(ROOT)), "class": "read-only source" if p in sources else "source/evidence annotation"}
        candidate = out / "snapshot" / APP.relative_to(ROOT)
        shared = out / "snapshot/framework"
        units = [candidate / "ptz_auth.c", candidate / "ptz_owner.c", shared / "src/backend.c", shared / "src/access_policy.c", candidate / "ptz_owner_test.c"]
        common = ["-std=c11", "-Wall", "-Wextra", "-Werror", "-Wconversion", "-Wshadow", "-fno-common", "-fstack-usage",
                  "-I" + str(candidate), "-I" + str(shared / "include")]
        for p in sources:
            if p.suffix in (".h", ".c", ".cpp") and "SPDX-License-Identifier: GPL-2.0-only" not in p.read_text().splitlines()[0]:
                raise RuntimeError("Source license notice missing: " + str(p))
        report["license_check"] = "PASS: owned and shared per-file GPL-2.0-only notices; exact original diagnostic notice retained"
        for relative, expected in ARM_PINS.items():
            if sha(args.toolchain / relative) != expected:
                raise RuntimeError("Pinned ARM toolchain mismatch: " + relative)
        report["arm_toolchain_pins"] = ARM_PINS
        compiler("/usr/bin/cc", "host")
        compiler("/usr/bin/c++", "cxx")

        def build(cc, label, flags, execute):
            folder = out / "objects" / label
            folder.mkdir()
            objs = []
            for i, source in enumerate(units):
                obj = folder / (str(i) + "-" + source.stem + ".o")
                run([cc, *common, *flags, "-MD", "-MF", obj.with_suffix(".d"), "-c", source, "-o", obj], label + "-compile-" + str(i))
                objs.append(obj)
            binary = out / (label + "-test")
            run([cc, *flags, *objs, "-Wl,-t", "-Wl,-Map=" + str(folder / "link.map"), "-o", binary], label + "-link")
            report["binaries"][label] = {"path": str(binary.relative_to(ROOT)), "sha256": sha(binary), "bytes": binary.stat().st_size, "executed": execute}
            report.setdefault("costs", {})[label] = run(["/usr/bin/size", *objs], label + "-size")
            if execute:
                report.setdefault("tests", {})[label] = run([binary], label + "-test", 30)
            else:
                readelf = args.toolchain / "bin/arm-buildroot-linux-musleabi-readelf"
                run([readelf, "-h", "-A", "-l", binary], label + "-elf")
                run([readelf, "-A", objs[0], objs[1]], label + "-core-attributes")
                report["ARM"] = "STATIC LINK PASS, NOT EXECUTED; merged pinned runtime attributes do not prove native ISA/ABI"
            return objs

        host = build("/usr/bin/cc", "optimized", ["-O2"], True)
        # The NEW boundary fixture must reject the frozen OLD implementation.
        # Compile/link only copied V1 source; this is an expected HOST failure,
        # not execution of firmware or mutation of the preserved run06 evidence.
        v1_object = out / "objects/v1-owner-regression.o"
        run(["/usr/bin/cc", *common, "-O2", "-MD", "-MF", v1_object.with_suffix(".d"), "-c",
             out / "snapshot" / v1.relative_to(ROOT), "-o", v1_object], "v1-regression-compile")
        v1_binary = out / "v1-client-boundary-regression"
        run(["/usr/bin/cc", "-O2", host[0], v1_object, *host[2:], "-Wl,-t", "-o", v1_binary], "v1-regression-link")
        old_failure = run([v1_binary, "--client-boundary-only"], "v1-regression-test", 30, expected_exit=1)
        if "MOVE mutation=0 submits_before=1 after=2 fault=1" not in old_failure:
            raise RuntimeError("V1 failed outside the intended malformed-client authority counterexample")
        report["v1_client_boundary_regression"] = {"result": "EXPECTED FAIL: null MOVE neutralizes active A and latches fault",
            "log": old_failure, "source_sha256": expected_v1, "executed": "HOST ONLY", "binary_sha256": sha(v1_binary)}
        build("/usr/bin/cc", "sanitized", ["-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"], True)
        cpp = out / "cxx-header-test"
        run(["/usr/bin/c++", "-std=c++11", "-O2", "-Wall", "-Wextra", "-Werror", "-MD", "-MF", out / "objects/header.d",
             "-I" + str(candidate), "-I" + str(shared / "include"), candidate / "header_link.cpp", *host[:-1], "-Wl,-t", "-o", cpp], "cxx-link")
        run([cpp], "cxx-test", 30)
        report["CXX"] = "C++ header -> actual C API LINK/RUN PASS"
        report["binaries"]["cxx"] = {"path": str(cpp.relative_to(ROOT)), "sha256": sha(cpp), "bytes": cpp.stat().st_size, "executed": True}
        armcc = args.toolchain / "bin/arm-buildroot-linux-musleabi-gcc"
        compiler(str(armcc), "arm")
        build(str(armcc), "arm", ["-Os", "-march=armv5te", "-marm", "-mfloat-abi=soft", "-static"], False)
        probe = out / "objects/arm-size-probe.o"
        run([armcc, *common, "-Os", "-march=armv5te", "-marm", "-mfloat-abi=soft", "-c", candidate / "size_probe.c", "-o", probe], "arm-size-probe-compile")
        report["arm_storage_sizes"] = run([args.toolchain / "bin/arm-buildroot-linux-musleabi-nm", "-S", "--size-sort", probe], "arm-size-probe-symbols")
        if before != {str(p.relative_to(ROOT)): sha(p) for p in sources}:
            raise RuntimeError("Compiled source input changed during run")
        report["state"] = "HOST FIXTURE PASS; C++ LINK/RUN PASS; ARM STATIC LINK PASS NOT EXECUTED; PARENT REVIEW PENDING"
    except BaseException as exc:
        report["state"] = "FAIL"
        report["failure"] = type(exc).__name__ + ": " + str(exc)
        raise
    finally:
        headers = {}
        for d in (out / "objects").rglob("*.d"):
            for name in shlex.split(d.read_text().replace("\\\n", "").split(":", 1)[1]):
                p = Path(name)
                if p.is_file():
                    headers[str(p.resolve())] = sha(p)
        report["source_header_closure"] = headers
        report["stack_usage"] = {str(p.relative_to(ROOT)): p.read_text() for p in (out / "objects").rglob("*.su")}
        link = {}
        for p in (out / "logs").glob("*-link.txt"):
            for line in p.read_text().splitlines():
                source = Path(line)
                if source.is_absolute() and source.is_file():
                    link[str(source.resolve())] = sha(source)
        report["link_runtime_closure"] = link
        report["finished_utc"] = now()
        with (out / "REPORT.json").open("x") as f:
            json.dump(report, f, indent=2)
            f.write("\n")
        print("evidence=" + str((out / "REPORT.json").relative_to(ROOT)))


if __name__ == "__main__":
    main()
