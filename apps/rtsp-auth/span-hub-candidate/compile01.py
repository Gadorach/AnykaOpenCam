# SPDX-License-Identifier: GPL-2.0-only
"""Fixed offline five-C-compile/two-REL recipe. Never executes any candidate."""
from pathlib import Path
import datetime
import hashlib
import json
import shlex
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[3]
SRC = ROOT / "apps/rtsp-auth/span-hub-candidate"
SPAN = ROOT / "apps/rtsp-auth/rtp-span-candidate"
DOC = ROOT / "docs/qualification-plans/span-hub-20261024"
OUT = DOC / "runs/build01"
ARM = ROOT / ("work/lightbulb-camera/implementation-20261008T205404Z/sources/"
              "toolchain-armv5-musl-202508/armv5-eabi--musl--stable-2025.08-1/"
              "bin/arm-buildroot-linux-musleabi-gcc")
ENV = {"PATH": "/usr/bin:/bin", "LC_ALL": "C", "PYTHONDONTWRITEBYTECODE": "1",
       "TMPDIR": str(OUT / "tmp")}


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
    (OUT / "tmp").mkdir()
    selected = sorted(p for p in SRC.iterdir() if p.is_file()) + [
        DOC / "CONTRACT.md", SPAN / "rtp_h264_spans.c", SPAN / "rtp_h264_spans.h",
        ROOT / "framework/include/camera/backend.h",
        ROOT / "framework/include/camera/rtp_h264.h",
        ROOT / "framework/include/camera/packet_hub.h", ROOT / "framework/src/packet_hub.c",
        ROOT / "docs/qualification-plans/rtp-span-20261024/CONTRACT.md",
        ROOT / "docs/qualification-plans/rtp-span-20261024/PARENT_DISPOSITION.md",
        ROOT / "docs/qualification-plans/rtp-span-review-20261024/REVIEW.md",
        ROOT / "docs/plan/camera-foundation/05-CAPTURE-AND-MEMORY.md",
        ROOT / "docs/plan/camera-foundation/08-STREAMING.md",
        ROOT / "docs/STREAMING_MEMORY_AND_BACKPRESSURE.md",
    ]
    inputs = [pin(p) for p in selected]
    dump(OUT / "INPUTS.json", inputs)
    for name, expected in [
        ("rtp_h264_spans.c", "a608fe8ee8a292b1172ce62c62f19c2a61f9969f64d71385f1a742fdd326560e"),
        ("rtp_h264_spans.h", "41126a8b9827894d6ecb71788cd1cc71bf3b5b036e44fddf998f4c5740f05f91"),
    ]:
        if pin(SPAN / name)["sha256"] != expected:
            raise SystemExit("STOP: accepted packetizer pin mismatch; no build")
    tools, commands = {"/usr/bin/python3": pin("/usr/bin/python3")}, []

    def tool_path(name):
        return Path(shutil.which(str(name), path=ENV["PATH"]) or name).absolute()

    def run(name, argv):
        argv = [str(a) for a in argv]
        tool = tool_path(argv[0])
        tools[str(tool)] = pin(tool)
        result = subprocess.run(argv, cwd=ROOT, env=ENV, capture_output=True)
        log = OUT / "logs" / (name + ".txt")
        with log.open("xb") as f:
            f.write(result.stdout + result.stderr)
        item = {"name": name, "argv": argv, "cwd": str(ROOT), "environment": ENV,
                "returncode": result.returncode, "log": pin(log)}
        commands.append(item)
        dump(OUT / ("command-%02d.json" % len(commands)), item)
        if result.returncode:
            raise SystemExit("STOP: " + name + " failed; outputs retained, no retry")
        return result.stdout.decode().strip()

    common = ["-std=c11", "-Wall", "-Wextra", "-Werror", "-Wpedantic", "-fno-common",
              "-fstack-usage", "-I" + str(ROOT / "framework/include")]
    for role, cc, flags in [("host", Path("/usr/bin/cc"), ["-O2"]),
                            ("arm", ARM, ["-Os", "-march=armv5te", "-marm", "-mfloat-abi=soft"])]:
        run(role + "-version", [cc, "--version"])
        run(role + "-machine", [cc, "-dumpmachine"])
        found = {}
        for name in (("cc1", "as", "ld", "collect2") if role == "host" else ("cc1", "as", "ld")):
            value = run(role + "-tool-" + name, [cc, "-print-prog-name=" + name])
            found[name] = tool_path(value)
            tools[str(found[name])] = pin(found[name])
        real = Path(str(cc) + ".br_real")
        if real.is_file():
            tools[str(real)] = pin(real)
        for unit, source in [("hub", SRC / "span_hub.c"), ("spans", SPAN / "rtp_h264_spans.c")]:
            obj = OUT / "objects" / (role + "-" + unit + ".o")
            run(role + "-compile-" + unit, [cc] + common + flags + [
                "-MD", "-MF", obj.with_suffix(".d"), "-c", source, "-o", obj])
            run(role + "-" + unit + "-ELF", ["/usr/bin/readelf", "-h", "-A", obj])
            run(role + "-" + unit + "-symbols", ["/usr/bin/nm", "-g", obj])
        rel = OUT / (role + "-HUB-REAL-SPANS-REL.o")
        run(role + "-integration-REL", [found["ld"], "-r", "-t", "-Map=" + str(OUT / (role + "-rel.map")),
            OUT / ("objects/" + role + "-hub.o"), OUT / ("objects/" + role + "-spans.o"), "-o", rel])
        run(role + "-REL-ELF", ["/usr/bin/readelf", "-h", "-A", rel])
        run(role + "-REL-symbols", ["/usr/bin/nm", "-g", rel])
    obj = OUT / "objects/host-normal.o"
    run("host-compile-normal", ["/usr/bin/cc"] + common + ["-O2", "-MD", "-MF",
        obj.with_suffix(".d"), "-c", SRC / "host_normal.c", "-o", obj])
    exe = OUT / "HOST-NORMAL-NOT-NATIVE"
    run("host-link-normal", ["/usr/bin/cc", obj, OUT / "host-HUB-REAL-SPANS-REL.o",
        "-Wl,-t", "-Wl,-Map=" + str(OUT / "host-link.map"), "-o", exe])
    run("host-normal-ELF", ["/usr/bin/readelf", "-h", "-A", exe])
    run("host-normal-symbols", ["/usr/bin/nm", "-g", exe])
    dependencies = {}
    for dep in sorted((OUT / "objects").glob("*.d")):
        for word in shlex.split(dep.read_text().replace("\\\n", " ").split(":", 1)[1]):
            p = (ROOT / word).absolute()
            dependencies[str(p)] = pin(p)
    for line in (OUT / "logs/host-link-normal.txt").read_text().splitlines():
        p = Path(line)
        if p.is_absolute() and p.is_file():
            tools[str(p)] = pin(p)
    dump(OUT / "DEPENDENCIES.json", list(dependencies.values()))
    dump(OUT / "TOOLS.json", list(tools.values()))
    if inputs != [pin(p) for p in selected]:
        raise SystemExit("STOP: input drift; no acceptance")
    dump(OUT / "REPORT.json", {"status": "COMPILE_REL_INSPECT_PASS_NOT_RUN",
        "utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "C_compiles": 5, "real_integration_REL_links": 2, "commands": commands,
        "artifacts": [pin(p) for p in sorted(OUT.rglob("*")) if p.is_file()],
        "inputs_unchanged": True, "candidate_executions": 0, "ARM_executable": False,
        "compiler_closure_or_native_ABI_claim": False})
    print("COMPILE_REL_INSPECT_PASS C_compiles=5 REL_links=2 HOST_NOT_RUN ARM_NOT_RUN")


if __name__ == "__main__":
    main()
