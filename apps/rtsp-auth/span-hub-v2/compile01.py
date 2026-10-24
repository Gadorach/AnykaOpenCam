# SPDX-License-Identifier: GPL-2.0-only
"""Fixed four production compiles/two REL links; no executable or program call."""
from pathlib import Path
import datetime
import hashlib
import json
import shlex
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[3]
SRC = ROOT / "apps/rtsp-auth/span-hub-v2"
SPAN = ROOT / "apps/rtsp-auth/rtp-span-candidate"
DOC = ROOT / "docs/qualification-plans/span-hub-v2-20261024"
OUT = DOC / "runs/build01"
OLD_TOOLS = ROOT / "docs/qualification-plans/span-hub-20261024/runs/build01/TOOLS.json"
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
    for name in ("logs", "objects", "tmp"):
        (OUT / name).mkdir()
    selected = sorted(p for p in SRC.iterdir() if p.is_file()) + [
        DOC / "CONTRACT.md", DOC / "AUTHOR_AUDIT.md", DOC / "SOURCE_DELTA.json",
        DOC / "V1_TO_V2.diff", DOC / "V2_TO_V1.diff", DOC / "FROZEN_INPUTS.json", OLD_TOOLS,
        ROOT / "apps/rtsp-auth/span-hub-candidate/span_hub.c",
        ROOT / "apps/rtsp-auth/span-hub-candidate/span_hub.h",
        ROOT / "docs/qualification-plans/span-hub-review-20261024/REVIEW.md",
        ROOT / "docs/qualification-plans/rtp-span-20261024/PARENT_DISPOSITION.md",
        SPAN / "rtp_h264_spans.c", SPAN / "rtp_h264_spans.h",
        ROOT / "framework/include/camera/backend.h",
        ROOT / "framework/include/camera/rtp_h264.h",
    ]
    inputs = [pin(p) for p in selected]
    dump(OUT / "INPUTS.json", inputs)
    for name, expected in [
        ("rtp_h264_spans.c", "a608fe8ee8a292b1172ce62c62f19c2a61f9969f64d71385f1a742fdd326560e"),
        ("rtp_h264_spans.h", "41126a8b9827894d6ecb71788cd1cc71bf3b5b036e44fddf998f4c5740f05f91"),
    ]:
        if pin(SPAN / name)["sha256"] != expected:
            raise SystemExit("STOP: accepted packetizer drift; no build")
    retained = {p["resolved"]: p for p in json.loads(OLD_TOOLS.read_text())}
    tools, commands = {}, []

    def tool_path(name):
        return Path(shutil.which(str(name), path=ENV["PATH"]) or name).absolute()

    def remember(path):
        record = pin(path)
        old = retained.get(record["resolved"])
        if not old or any(record[k] != old[k] for k in ("bytes", "sha256")):
            raise SystemExit("STOP: retained tool mismatch; no retry")
        tools[str(path)] = record

    def run(name, argv):
        argv = [str(a) for a in argv]
        remember(tool_path(argv[0]))
        start = datetime.datetime.now(datetime.timezone.utc).isoformat()
        result = subprocess.run(argv, cwd=ROOT, env=ENV, capture_output=True, timeout=120)
        log = OUT / "logs" / (name + ".txt")
        with log.open("xb") as f:
            f.write(result.stdout + result.stderr)
        item = {"name": name, "argv": argv, "cwd": str(ROOT), "environment": ENV,
                "started_utc": start,
                "finished_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
                "returncode": result.returncode, "log": pin(log)}
        commands.append(item)
        dump(OUT / ("command-%02d.json" % len(commands)), item)
        if result.returncode:
            raise SystemExit("STOP: " + name + " failed; retained, no retry")
        return result.stdout.decode().strip()

    remember(Path("/usr/bin/python3"))
    common = ["-std=c11", "-Wall", "-Wextra", "-Werror", "-Wpedantic", "-fno-common",
              "-fstack-usage", "-I" + str(ROOT / "framework/include")]
    for role, cc, flags in [("host", Path("/usr/bin/cc"), ["-O2"]),
                            ("arm", ARM, ["-Os", "-march=armv5te", "-marm", "-mfloat-abi=soft"])]:
        real = Path(str(cc) + ".br_real")
        if real.is_file():
            remember(real)
        run(role + "-version", [cc, "--version"])
        run(role + "-machine", [cc, "-dumpmachine"])
        found = {}
        for name in ("cc1", "as", "ld"):
            found[name] = tool_path(run(role + "-tool-" + name, [cc, "-print-prog-name=" + name]))
            remember(found[name])
        for unit, source in [("hub", SRC / "span_hub.c"), ("spans", SPAN / "rtp_h264_spans.c")]:
            obj = OUT / "objects" / (role + "-" + unit + ".o")
            run(role + "-compile-" + unit, [cc] + common + flags + [
                "-MD", "-MF", obj.with_suffix(".d"), "-c", source, "-o", obj])
        rel = OUT / (role + "-HUB-REAL-SPANS-REL.o")
        run(role + "-integration-REL", [found["ld"], "-r", "-t", "-Map=" + str(OUT / (role + "-rel.map")),
            OUT / ("objects/" + role + "-hub.o"), OUT / ("objects/" + role + "-spans.o"), "-o", rel])
        run(role + "-REL-ELF", ["/usr/bin/readelf", "-h", "-A", rel])
        run(role + "-REL-symbols", ["/usr/bin/nm", "-g", rel])
    dependencies = {}
    for dep in sorted((OUT / "objects").glob("*.d")):
        for word in shlex.split(dep.read_text().replace("\\\n", " ").split(":", 1)[1]):
            p = (ROOT / word).absolute()
            dependencies[str(p)] = pin(p)
    dump(OUT / "DEPENDENCIES.json", list(dependencies.values()))
    dump(OUT / "TOOLS.json", list(tools.values()))
    if inputs != [pin(p) for p in selected] or any(pin(p["path"]) != p for p in tools.values()):
        raise SystemExit("STOP: input/tool drift; no acceptance")
    dump(OUT / "REPORT.json", {"status": "COMPILE_REL_INSPECT_PASS_NOT_RUN",
        "utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "C_compiles": 4, "real_integration_REL_links": 2, "commands": commands,
        "artifacts": [pin(p) for p in sorted(OUT.rglob("*")) if p.is_file()],
        "inputs_tools_unchanged": True, "production_or_test_executions": 0,
        "executable_built": False, "compiler_closure_or_native_ABI_claim": False})
    print("COMPILE_REL_INSPECT_PASS C_compiles=4 REL_links=2 commands=20 NOT_RUN")


if __name__ == "__main__":
    main()
