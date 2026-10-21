#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Own-lane input-only HOST/cross-link evidence; no device, PTZ or global edits."""
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
LANE = ROOT / "evidence/delegated/20261012-reset-input"
OWN = ["reset_input.h", "reset_input.c", "reset_input_test.c", "header_link.cpp", "size_probe.c", "README.md", "build.py"]
ANNOTATIONS = ["AGENTS.md", "docs/plan/camera-foundation/00-INDEX.md", "docs/plan/camera-foundation/11-PTZ-AND-PERIPHERALS.md",
               "docs/QC07F_RETAIL_ILLUMINATION_ROUTING.md"]
ARM_PINS = {
    "bin/toolchain-wrapper": "f4db963a226e01244ce6f7b5d1ec6d57474dd811119ff8834429f022835ce9c1",
    "bin/arm-buildroot-linux-musleabi-gcc.br_real": "e24ea223e7b521cce1f5889d63c25417e930cdbb35653b072f7b48d9a4b41729",
    "arm-buildroot-linux-musleabi/sysroot/lib/libc.a": "0982970abc52a36d247326a8e0b5dd487410222ff9728528cc99a4e8bd62e7b5",
    "lib/gcc/arm-buildroot-linux-musleabi/14.3.0/libgcc.a": "f707f3a5a94033cbbde76f76640d56dbc5dc615ab421be4efc2641c14344f19a",
}


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
    for name in ("snapshot", "objects", "logs", "tmp"):
        (out / name).mkdir()
    env = {"PATH": "/usr/bin:/bin", "LC_ALL": "C", "SOURCE_DATE_EPOCH": "1791763200", "TMPDIR": str(out / "tmp"),
           "ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1", "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"}
    report = {"started_utc": now(), "command": ["python3", str(Path(__file__).relative_to(ROOT)), *os.sys.argv[1:]],
              "python": os.sys.version, "cwd": str(ROOT), "environment": env, "state": "RUNNING", "tasks": ["CTRL-08", "CTRL-10 input contribution"],
              "evidence_class": "SOURCE + PUBLICDUMMY HOST; ARM LINK ONLY", "commands": [], "inputs": {}, "binaries": {},
              "native_button_qualified": False, "reset_executed": False, "footage_actions": False, "native_executed": False,
              "integrated": False, "CTRL_complete": False, "G4": False, "G6": False, "G8": False}

    def run(argv, label, timeout=120):
        argv = list(map(str, argv));log = out / "logs" / (label + ".txt")
        entry = {"argv": argv, "cwd": str(out), "started_utc": now(), "log": str(log.relative_to(ROOT)), "timeout_seconds": timeout}
        report["commands"].append(entry)
        with log.open("xb") as f:
            result = subprocess.run(argv, cwd=out, env=env, stdout=f, stderr=subprocess.STDOUT, timeout=timeout)
        entry.update(exit_code=result.returncode, finished_utc=now(), sha256=sha(log))
        if result.returncode:
            raise RuntimeError("Command failed, original retained: " + str(log))
        return log.read_text()

    def compiler(cc, label):
        report.setdefault("compilers", {})[label] = {"version": run([cc, "--version"], label + "-version"),
            "target": run([cc, "-dumpmachine"], label + "-target").strip(), "sha256": sha(Path(cc).resolve())}
        for item in ("cc1", "cc1plus", "as", "ld", "libgcc.a", "libstdc++.so", "libasan.so", "libubsan.so", "crt1.o", "Scrt1.o", "crti.o", "crtbeginS.o", "crtbeginT.o", "crtend.o", "crtn.o", "libc.a", "libc.so"):
            flag = "-print-prog-name=" if item in ("cc1", "cc1plus", "as", "ld") else "-print-file-name="
            value = run([cc, flag + item], label + "-closure-" + item.replace(".", "_")).strip()
            p = Path(value) if "/" in value else Path(shutil.which(value, path=env["PATH"]) or value)
            if p.is_file():
                report.setdefault("compiler_runtime_closure", {})[str(p.resolve())] = sha(p)

    try:
        sources = [APP / n for n in OWN]
        before = {str(p.relative_to(ROOT)): sha(p) for p in sources}
        for p in sources + [ROOT / p for p in ANNOTATIONS]:
            if p.is_symlink():
                raise RuntimeError("Source symlink refused")
            relative = p.relative_to(ROOT);dest = out / "snapshot" / relative
            dest.parent.mkdir(parents=True, exist_ok=True)
            with dest.open("xb") as f:
                f.write(p.read_bytes())
            report["inputs"][str(relative)] = {"sha256": sha(dest), "bytes": dest.stat().st_size,
                "copy": str(dest.relative_to(ROOT)), "class": "owned source" if p in sources else "bounded source/evidence annotation"}
        for p in sources:
            if p.suffix in (".c", ".h", ".cpp") and "SPDX-License-Identifier: GPL-2.0-only" not in p.read_text().splitlines()[0]:
                raise RuntimeError("Owned license notice absent")
        report["license_check"] = "PASS: per-file GPL-2.0-only owned code, no stock implementation reused or workspace-wide license grant"
        for relative, expected in ARM_PINS.items():
            if sha(args.toolchain / relative) != expected:
                raise RuntimeError("Pinned ARM toolchain mismatch: " + relative)
        report["arm_toolchain_pins"] = ARM_PINS
        summary = args.toolchain / "summary.csv"
        report["runtime_license_metadata"] = {"path": str(summary), "sha256": sha(summary),
            "selected_package_lines": [line for line in summary.read_text().splitlines() if line.startswith(('"gcc-final",', '"musl",', '"binutils",'))],
            "note": "Installed Buildroot license metadata, not a new grant; exact linked archives/header closure below"}
        app = out / "snapshot" / APP.relative_to(ROOT)
        common = ["-std=c11", "-Wall", "-Wextra", "-Werror", "-Wconversion", "-Wshadow", "-fno-common", "-fstack-usage", "-I" + str(app)]
        compiler("/usr/bin/cc", "host");compiler("/usr/bin/c++", "cxx")

        def build(cc, label, flags, execute):
            folder = out / "objects" / label;folder.mkdir();objs = []
            for i, source in enumerate((app / "reset_input.c", app / "reset_input_test.c")):
                obj = folder / (str(i) + "-" + source.stem + ".o")
                run([cc, *common, *flags, "-MD", "-MF", obj.with_suffix(".d"), "-c", source, "-o", obj], label + "-compile-" + str(i));objs.append(obj)
            binary = out / (label + "-test")
            run([cc, *flags, *objs, "-Wl,-t", "-Wl,-Map=" + str(folder / "link.map"), "-o", binary], label + "-link")
            report["binaries"][label] = {"path": str(binary.relative_to(ROOT)), "bytes": binary.stat().st_size, "sha256": sha(binary), "executed": execute}
            report.setdefault("costs", {})[label] = run(["/usr/bin/size", *objs], label + "-size")
            if execute:
                report.setdefault("tests", {})[label] = run([binary], label + "-test", 30)
            else:
                elf = args.toolchain / "bin/arm-buildroot-linux-musleabi-readelf"
                run([elf, "-h", "-A", "-l", binary], "arm-elf");run([elf, "-A", objs[0]], "arm-core-attributes")
                report["ARM"] = "STATIC LINK PASS, NOT EXECUTED; merged pinned-runtime ISA attributes are not native proof"
            return objs

        objects = build("/usr/bin/cc", "optimized", ["-O2"], True)
        imports = run(["/usr/bin/nm", "-u", objects[0]], "core-undefined-symbols")
        allowed = {"memcpy", "memset", "memcmp", "__stack_chk_fail"}
        imported = {line.split()[-1] for line in imports.splitlines() if line.strip()}
        if not imported <= allowed:
            raise RuntimeError("Unexpected core action/runtime dependency: " + repr(imported))
        report["core_action_dependency_check"] = {"result": "PASS", "imports": sorted(imported),
            "only_callback": "trusted input-only observation read", "reset_filesystem_GPIO_network_executors": "ABSENT"}
        build("/usr/bin/cc", "sanitized", ["-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"], True)
        cpp = out / "cxx-header-test"
        run(["/usr/bin/c++", "-std=c++11", "-O2", "-Wall", "-Wextra", "-Werror", "-MD", "-MF", out / "objects/header.d",
             "-I" + str(app), app / "header_link.cpp", objects[0], "-Wl,-t", "-o", cpp], "cxx-link")
        run([cpp], "cxx-test", 30);report["CXX"] = "C++ header -> actual C API LINK/RUN PASS"
        report["binaries"]["cxx"] = {"path": str(cpp.relative_to(ROOT)), "bytes": cpp.stat().st_size, "sha256": sha(cpp), "executed": True}
        armcc = args.toolchain / "bin/arm-buildroot-linux-musleabi-gcc";compiler(str(armcc), "arm")
        build(str(armcc), "arm", ["-Os", "-march=armv5te", "-marm", "-mfloat-abi=soft", "-static"], False)
        probe = out / "objects/arm-size-probe.o"
        run([armcc, *common, "-Os", "-march=armv5te", "-marm", "-mfloat-abi=soft", "-c", app / "size_probe.c", "-o", probe], "arm-size-probe")
        report["arm_storage_sizes"] = run([args.toolchain / "bin/arm-buildroot-linux-musleabi-nm", "-S", "--size-sort", probe], "arm-size-probe-symbols")
        if before != {str(p.relative_to(ROOT)): sha(p) for p in sources}:
            raise RuntimeError("Owned source changed during qualification")
        report["state"] = "SOURCE/HOST PASS; C++ LINK/RUN PASS; ARM LINK ONLY; PARENT REVIEW PENDING"
    except BaseException as exc:
        report["state"] = "FAIL";report["failure"] = type(exc).__name__ + ": " + str(exc);raise
    finally:
        headers = {}
        for d in (out / "objects").rglob("*.d"):
            for name in shlex.split(d.read_text().replace("\\\n", "").split(":", 1)[1]):
                p = Path(name)
                if p.is_file():headers[str(p.resolve())] = sha(p)
        report["source_header_closure"] = headers
        report["stack_usage"] = {str(p.relative_to(ROOT)): p.read_text() for p in (out / "objects").rglob("*.su")}
        link = {}
        for log in (out / "logs").glob("*-link.txt"):
            for line in log.read_text().splitlines():
                p = Path(line)
                if p.is_absolute() and p.is_file():link[str(p.resolve())] = sha(p)
        report["link_runtime_closure"] = link;report["finished_utc"] = now()
        with (out / "REPORT.json").open("x") as f:json.dump(report, f, indent=2);f.write("\n")
        print("evidence=" + str((out / "REPORT.json").relative_to(ROOT)))


if __name__ == "__main__":
    main()
