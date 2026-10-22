#!/usr/bin/env python3
"""Changed-unit HOST syntax only. No Kbuild/object/link/test/module execution."""
import datetime
import hashlib
import json
from pathlib import Path
import shlex
import subprocess
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
E = ROOT / "docs/qualification-plans/kernel76-transport-20261021/station-owner-lifetime-p4a-source01"
K = Path("/usr/src/linux-cachyos-lts")
CC = Path("/usr/bin/clang")


def pin(p):
    b = p.read_bytes()
    return {"bytes": len(b), "sha256": hashlib.sha256(b).hexdigest()}


def main():
    if sys.flags.optimize or len(sys.argv) != 2 or not sys.argv[1].startswith("syntax") or not sys.argv[1].isalnum():
        raise SystemExit("usage: python3 -B check_syntax.py syntaxNN (fresh name)")
    out = E / sys.argv[1]
    out.mkdir(exist_ok=False)
    source = HERE / "source"
    inputs = {str(p.relative_to(ROOT)): pin(p) for p in sorted(source.rglob("*")) if p.is_file()}
    args = [str(CC), "-fsyntax-only", "-std=gnu11", "-nostdinc", "-D__KERNEL__", "-DMODULE",
            '-DKBUILD_MODNAME="hgics_p4a_syntax"', '-DKBUILD_BASENAME="hgics_p4a_syntax"',
            "-DCONFIG_HGIC_USB", "-DCONFIG_HGIC_2G", "-Werror=implicit-function-declaration",
            "-Werror=incompatible-pointer-types", "-Wno-address-of-packed-member",
            "-Wno-pointer-sign", "-Wno-gnu-variable-sized-type-not-at-end"]
    for rel in ("arch/x86/include", "arch/x86/include/generated", "include", "arch/x86/include/uapi",
                "arch/x86/include/generated/uapi", "include/uapi", "include/generated/uapi"):
        args += ["-I", str(K / rel)]
    for rel in ("include/linux/compiler-version.h", "include/linux/kconfig.h", "include/linux/compiler_types.h"):
        args += ["-include", str(K / rel)]
    args += ["-include", str(source / "linux6-compat.h")]
    version = subprocess.run([str(CC), "--version"], capture_output=True, check=False)
    (out / "compiler.txt").write_bytes(version.stdout + version.stderr)
    units = ["hgic_smac/attach_init.c", "hgic_smac/attach_status.c", "utils/fwctrl.c", "utils/owner_usb.c"]
    commands, dependencies = [], {}
    for index, rel in enumerate(units):
        dep = out / f"{index + 1:02d}-headers.d"
        argv = args + ["-MD", "-MF", str(dep), "-MT", "syntax-only", str(source / rel)]
        start = datetime.datetime.now(datetime.timezone.utc).isoformat()
        proc = subprocess.run(argv, cwd=ROOT, capture_output=True, check=False, timeout=90)
        log = out / (f"{index + 1:02d}-" + rel.replace("/", "-") + ".txt")
        log.write_bytes(proc.stdout + proc.stderr)
        commands.append({"argv": argv, "start_utc": start,
                         "end_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
                         "returncode": proc.returncode, "log": log.name, "log_pin": pin(log)})
        if dep.exists():
            text = dep.read_text().replace("\\\n", " ")
            for name in shlex.split(text.split(":", 1)[1]):
                path = Path(name)
                if not path.is_absolute():
                    path = ROOT / path
                dependencies[str(path)] = pin(path)
    for rel, expected in inputs.items():
        if pin(ROOT / rel) != expected:
            raise SystemExit("source changed during syntax check: " + rel)
    receipt = {"utc": datetime.datetime.now(datetime.timezone.utc).isoformat(), "argv": sys.argv,
               "cwd": str(ROOT), "python": sys.version, "runner": pin(Path(__file__)),
               "compiler": {"path": str(CC), **pin(CC)}, "source": inputs,
               "host_kernel_inputs": {str(K / rel): pin(K / rel) for rel in (
                   "include/generated/utsrelease.h", "include/generated/autoconf.h", ".config")},
               "header_dependencies": dependencies, "commands": commands,
               "pass": all(c["returncode"] == 0 for c in commands),
               "scope": "HOST x86 changed-unit syntax only; no object/module/ARM build, link, tests or native IO"}
    with (out / "receipt.json").open("x") as f:
        json.dump(receipt, f, indent=2)
        f.write("\n")
    print(json.dumps({"syntax": sys.argv[1], "pass": receipt["pass"],
                      "returncodes": [c["returncode"] for c in commands]}))


if __name__ == "__main__":
    main()
