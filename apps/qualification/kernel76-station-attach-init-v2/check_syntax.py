#!/usr/bin/env python3
"""HOST syntax-only for the three corrected P2 C units; no tests/module/build."""
import datetime
import hashlib
import json
from pathlib import Path
import shlex
import subprocess
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
EVIDENCE = ROOT / "docs/qualification-plans/kernel76-transport-20261021/station-attach-init-p2-v2-source01"
KERNEL = Path("/usr/src/linux-cachyos-lts")
COMPILER = Path("/usr/bin/clang")


def pin(path):
    data = path.read_bytes()
    return {"bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}


def main():
    if sys.flags.optimize or len(sys.argv) != 2 or not sys.argv[1].startswith("syntax") or not sys.argv[1].isalnum():
        raise SystemExit("usage: python3 -B check_syntax.py syntaxNN (fresh name)")
    out = EVIDENCE / sys.argv[1]
    out.mkdir(exist_ok=False)
    source = HERE / "source"
    inputs = {str(p.relative_to(ROOT)): pin(p) for p in sorted(source.rglob("*")) if p.is_file()}
    args = [str(COMPILER), "-fsyntax-only", "-std=gnu11", "-nostdinc", "-D__KERNEL__", "-DMODULE",
            '-DKBUILD_MODNAME="hgics_p2_v2_syntax"', '-DKBUILD_BASENAME="hgics_p2_v2_syntax"',
            "-DCONFIG_HGIC_USB", "-DCONFIG_HGIC_2G", "-Werror=implicit-function-declaration",
            "-Werror=incompatible-pointer-types", "-Wno-address-of-packed-member",
            "-Wno-pointer-sign", "-Wno-gnu-variable-sized-type-not-at-end"]
    for rel in ("arch/x86/include", "arch/x86/include/generated", "include",
                "arch/x86/include/uapi", "arch/x86/include/generated/uapi", "include/uapi",
                "include/generated/uapi"):
        args += ["-I", str(KERNEL / rel)]
    for rel in ("include/linux/compiler-version.h", "include/linux/kconfig.h",
                "include/linux/compiler_types.h"):
        args += ["-include", str(KERNEL / rel)]
    args += ["-include", str(source / "linux6-compat.h")]
    version = subprocess.run([str(COMPILER), "--version"], capture_output=True, check=False)
    (out / "compiler.txt").write_bytes(version.stdout + version.stderr)
    units = ["hgic_smac/attach_init.c", "hgic_smac/attach_status.c", "utils/fwctrl.c"]
    commands = []
    dependencies = {}
    for index, rel in enumerate(units):
        depfile = out / (f"{index + 1:02d}-headers.d")
        argv = args + ["-MD", "-MF", str(depfile), "-MT", "syntax-only", str(source / rel)]
        start = datetime.datetime.now(datetime.timezone.utc).isoformat()
        proc = subprocess.run(argv, cwd=ROOT, capture_output=True, check=False, timeout=90)
        log = out / (f"{index + 1:02d}-" + rel.replace("/", "-") + ".txt")
        log.write_bytes(proc.stdout + proc.stderr)
        commands.append({"argv": argv, "start_utc": start,
                         "end_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
                         "returncode": proc.returncode, "log": log.name, "log_pin": pin(log)})
        if depfile.exists():
            text = depfile.read_text().replace("\\\n", " ")
            for name in shlex.split(text.split(":", 1)[1]):
                path = Path(name)
                if not path.is_absolute():
                    path = ROOT / path
                dependencies[str(path)] = pin(path)
    kernel_inputs = {str(KERNEL / rel): pin(KERNEL / rel) for rel in (
        "include/generated/utsrelease.h", "include/generated/autoconf.h", ".config")}
    for rel, expected in inputs.items():
        if pin(ROOT / rel) != expected:
            raise SystemExit("source changed during syntax check: " + rel)
    receipt = {"utc": datetime.datetime.now(datetime.timezone.utc).isoformat(), "argv": sys.argv,
               "cwd": str(ROOT), "python": sys.version, "runner": pin(Path(__file__)),
               "compiler": {"path": str(COMPILER), **pin(COMPILER)},
               "kernel_inputs": kernel_inputs, "header_dependencies": dependencies,
               "source": inputs, "commands": commands,
               "pass": all(row["returncode"] == 0 for row in commands),
               "scope": "HOST x86 syntax-only changed units; no unchanged suites, tests, object/module/ARM build, link or target IO"}
    with (out / "receipt.json").open("x") as stream:
        json.dump(receipt, stream, indent=2)
        stream.write("\n")
    print(json.dumps({"syntax": sys.argv[1], "pass": receipt["pass"],
                      "returncodes": [row["returncode"] for row in commands]}))


if __name__ == "__main__":
    main()
