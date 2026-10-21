#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""External-module ONLY, sealed tree read-only via bubblewrap. No target access.

Creates a new output directory, refuses overwrites. Copies prepared metadata,
never invokes modules_prepare/config/full-kernel targets. Host clang, lld and
copied modpost/fixdep are trusted local build tools, not retail firmware.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
from datetime import datetime, timezone


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    source = Path(__file__).resolve().parent
    repo = source.parents[2]
    base = repo / "work/lightbulb-camera/foundation-20261011T041148Z"
    kernel = base / "artifacts/native-security75-source01/linux"
    sealed = base / "analysis/native-security75-build01/build"
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    out = args.output.resolve()
    # No output nested in either sealed tree, even through symlinks.
    if out == kernel or kernel in out.parents or out == sealed or sealed in out.parents:
        parser.error("output may not be inside a sealed input")
    out.mkdir(parents=True, exist_ok=False)
    prepared = out / "kernel-metadata"
    prepared.mkdir()
    for name in (".config", "Module.symvers"):
        shutil.copy2(sealed / name, prepared / name)
    for name in ("include", "scripts", "arch/arm/include"):
        shutil.copytree(sealed / name, prepared / name, symlinks=False)
    module = out / "module"
    module.mkdir()
    temp = out / "tmp"
    temp.mkdir()
    inputs = {}
    for path in sorted(source.iterdir()):
        if path.is_file():
            inputs[str(path.relative_to(repo))] = sha(path)
    for root in (kernel / "arch/arm/mach-ak3918ev200", prepared):
        for path in sorted(root.rglob("*")):
            if path.is_file():
                inputs[str(path)] = sha(path)
    for path in (kernel / "Makefile", kernel / "include/linux/gpio/consumer.h",
                 kernel / "include/linux/gpio/driver.h", kernel / "include/linux/hrtimer.h",
                 kernel / "drivers/gpio/gpiolib.c"):
        inputs[str(path)] = sha(path)
    make = ["make", "-C", str(kernel), f"O={prepared}", f"M={source}", f"MO={module}",
            "ARCH=arm", "LLVM=1", "LLVM_IAS=1", "W=1", "modules"]
    host = out / "host-policy-test"
    commands = [
        ["clang", "--version"], ["ld.lld", "--version"], ["make", "--version"],
        ["python3", "--version"], ["bwrap", "--version"],
        ["clang", "-std=c11", "-Wall", "-Wextra", "-Werror", "-O2",
         "-fsanitize=address,undefined", str(source / "host_policy_test.c"), "-o", str(host)],
        [str(host)],
        ["python3", "-B", str(source / "test_transport.py")],
        make,
        ["llvm-readelf", "-h", "-p", ".modinfo", str(module / "qc07f_optical.ko")],
    ]
    report = {"started_utc": datetime.now(timezone.utc).isoformat(),
              "scope": "offline external module and host model only; native unexecuted",
              "inputs_sha256": inputs, "commands": [], "hardware_proof": "OPEN"}
    env = os.environ.copy()
    env["TMPDIR"] = str(temp)
    env["PYTHONDONTWRITEBYTECODE"] = "1"
    try:
        for index, cmd in enumerate(commands):
            sandbox = ["bwrap", "--ro-bind", "/", "/", "--dev", "/dev",
                       "--bind", str(out), str(out),
                       "--unshare-net", "--die-with-parent", "--", *cmd]
            result = subprocess.run(sandbox, env=env, stdout=subprocess.PIPE,
                                    stderr=subprocess.STDOUT, text=True)
            (out / f"command{index:02d}.log").write_text(result.stdout)
            report["commands"].append({"argv": sandbox, "returncode": result.returncode,
                                       "log": f"command{index:02d}.log"})
            print(result.stdout, end="", flush=True)
            if result.returncode:
                raise RuntimeError(f"command {index} failed")
        report["module_sha256"] = sha(module / "qc07f_optical.ko")
        report["result"] = "BUILD_AND_MODEL_PASS_NOT_NATIVE"
    finally:
        report["finished_utc"] = datetime.now(timezone.utc).isoformat()
        (out / "build-report.json").write_text(json.dumps(report, indent=2) + "\n")


if __name__ == "__main__":
    main()
