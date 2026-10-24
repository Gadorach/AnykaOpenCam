# SPDX-License-Identifier: GPL-2.0-only
"""Fixed 3-C-compile recipe, HOST link/ELF inspection only; never runs candidates."""
from pathlib import Path
import datetime
import hashlib
import json
import shlex
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[3]
SRC = ROOT / "apps/rtsp-auth/rtp-span-candidate"
DOC = ROOT / "docs/qualification-plans/rtp-span-20261024"
OUT = DOC / "runs/build01"
ARM = ROOT / ("work/lightbulb-camera/implementation-20261008T205404Z/sources/"
              "toolchain-armv5-musl-202508/armv5-eabi--musl--stable-2025.08-1/"
              "bin/arm-buildroot-linux-musleabi-gcc")
ENV = {"PATH": "/usr/bin:/bin", "LC_ALL": "C"}


def pin(path):
    p = Path(path).absolute()
    return {"path": str(p), "resolved": str(p.resolve()), "bytes": p.stat().st_size,
            "sha256": hashlib.sha256(p.read_bytes()).hexdigest()}


def dump(path, value):
    with path.open("x") as f:
        f.write(json.dumps(value, indent=2) + "\n")


def main():
    OUT.mkdir(parents=True, exist_ok=False)
    (OUT / "logs").mkdir()
    (OUT / "objects").mkdir()
    sources = sorted(p for p in SRC.iterdir() if p.is_file()) + [
        DOC / "CONTRACT.md", ROOT / "framework/src/rtp_h264.c",
        ROOT / "framework/include/camera/rtp_h264.h",
        ROOT / "framework/include/camera/backend.h",
        ROOT / "framework/src/packet_hub.c",
        ROOT / "apps/rtsp-auth/qc07f-owned-runtime/native/native_adapter.c",
        ROOT / "apps/rtsp-auth/qc07f-owned-runtime/server/rtsp_server_candidate.c",
        ROOT / "apps/rtsp-auth/qc07f-owned-runtime/server/candidate_auth.h",
    ]
    inputs = [pin(p) for p in sources]
    dump(OUT / "INPUTS.json", inputs)
    tools, commands = {}, []

    def run(name, argv):
        argv = [str(a) for a in argv]
        tool = Path(shutil.which(argv[0], path=ENV["PATH"]) or argv[0])
        tools[str(tool)] = pin(tool)
        r = subprocess.run(argv, cwd=ROOT, env=ENV, capture_output=True)
        log = OUT / "logs" / (name + ".txt")
        with log.open("xb") as f:
            f.write(r.stdout + r.stderr)
        item = {"name": name, "argv": argv, "cwd": str(ROOT), "environment": ENV,
                "returncode": r.returncode, "log": pin(log)}
        commands.append(item)
        dump(OUT / ("command-%02d.json" % len(commands)), item)
        if r.returncode:
            raise SystemExit("STOP: " + name + " failed; preserve, no retry")
        return r.stdout.decode().strip()

    common = ["-std=c11", "-Wall", "-Wextra", "-Werror", "-Wpedantic", "-fno-common",
              "-fstack-usage", "-I" + str(ROOT / "framework/include")]
    for role, cc, flags in [("host", Path("/usr/bin/cc"), ["-O2"]),
                            ("arm", ARM, ["-Os", "-march=armv5te", "-marm",
                                          "-mfloat-abi=soft"])]:
        run(role + "-version", [cc, "--version"])
        run(role + "-machine", [cc, "-dumpmachine"])
        for name in (("cc1", "as", "collect2", "ld") if role == "host" else ("cc1", "as")):
            value = run(role + "-tool-" + name, [cc, "-print-prog-name=" + name])
            tool = Path(shutil.which(value, path=ENV["PATH"]) or value)
            tools[str(tool)] = pin(tool)
        obj = OUT / "objects" / (role + "-spans.o")
        run(role + "-compile-spans", [cc] + common + flags + [
            "-MD", "-MF", obj.with_suffix(".d"), "-c", SRC / "rtp_h264_spans.c", "-o", obj])
        run(role + "-object-ELF", ["/usr/bin/readelf", "-h", "-A", obj])
        run(role + "-object-symbols", ["/usr/bin/nm", "-g", obj])
    obj = OUT / "objects/host-normal.o"
    run("host-compile-normal", ["/usr/bin/cc"] + common + ["-O2", "-MD", "-MF",
        obj.with_suffix(".d"), "-c", SRC / "host_normal.c", "-o", obj])
    exe = OUT / "HOST-NORMAL-NOT-NATIVE"
    run("host-link-normal", ["/usr/bin/cc", obj, OUT / "objects/host-spans.o",
        "-Wl,-t", "-Wl,-Map=" + str(OUT / "host-link.map"), "-o", exe])
    run("host-normal-ELF", ["/usr/bin/readelf", "-h", "-A", exe])
    run("host-normal-symbols", ["/usr/bin/nm", "-g", exe])
    # A small inventory of real consumed headers; no whole-tree/closure claim.
    dependencies = {}
    for dep in sorted((OUT / "objects").glob("*.d")):
        words = shlex.split(dep.read_text().replace("\\\n", " ").split(":", 1)[1])
        for word in words:
            p = (ROOT / word).absolute()
            dependencies[str(p)] = pin(p)
    for line in (OUT / "logs/host-link-normal.txt").read_text().splitlines():
        p = Path(line)
        if p.is_absolute() and p.is_file():
            tools[str(p)] = pin(p)
    dump(OUT / "DEPENDENCIES.json", list(dependencies.values()))
    dump(OUT / "TOOLS.json", list(tools.values()))
    if inputs != [pin(p) for p in sources]:
        raise SystemExit("STOP: selected input drift; no acceptance")
    artifacts = [pin(p) for p in sorted(OUT.rglob("*")) if p.is_file()]
    dump(OUT / "REPORT.json", {"status": "COMPILE_INSPECT_PASS_NOT_RUN",
        "utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "C_compiles": 3, "commands": commands, "artifacts": artifacts,
        "inputs_unchanged": True, "host_observation_executions": 0,
        "arm_execution": False, "compiler_closure_claim": False})
    print("COMPILE_INSPECT_PASS C_compiles=3 HOST_NOT_RUN ARM_NOT_RUN")


if __name__ == "__main__":
    main()
