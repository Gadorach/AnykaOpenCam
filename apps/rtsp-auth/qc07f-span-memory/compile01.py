# SPDX-License-Identifier: GPL-2.0-only
"""Fixed offline production compiles/REL inspection; NEVER call candidate code."""
from pathlib import Path
import datetime
import hashlib
import json
import shlex
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[3]
SRC = ROOT / "apps/rtsp-auth/qc07f-span-memory"
OLD = ROOT / "apps/rtsp-auth/qc07f-owned-runtime/native"
HUB = ROOT / "apps/rtsp-auth/span-hub-v2"
SPAN = ROOT / "apps/rtsp-auth/rtp-span-candidate"
DOC = ROOT / "docs/qualification-plans/qc07f-span-memory-20261024"
OUT = DOC / "runs/build01"
OLD_TOOLS = ROOT / "docs/qualification-plans/span-hub-v2-20261024/runs/build01/TOOLS.json"
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
        DOC / "ORIGINAL_TO_CANDIDATE.diff", DOC / "CANDIDATE_TO_ORIGINAL.diff",
        DOC / "FROZEN_INPUTS.json", OLD_TOOLS,
        ROOT / "docs/qualification-plans/span-hub-parent-20261024/V2_DISPOSITION.md",
        ROOT / "apps/rtsp-auth/qc07f-owned-runtime/owned_runtime.c",
        OLD / "provider-return.inc", OLD / "native_adapter.c",
    ] + [OLD / name for name in ("memory_reserve.c", "memory_reserve.h", "memory_runtime.c",
                                  "memory_runtime.h", "memory_reader.c", "handoff.h")
         ] + [HUB / "span_hub.c", HUB / "span_hub.h",
              SPAN / "rtp_h264_spans.c", SPAN / "rtp_h264_spans.h"]
    inputs = [pin(p) for p in selected]
    dump(OUT / "INPUTS.json", inputs)
    for p, expected in [
        (HUB / "span_hub.c", "bcf50041bb1fe1b20352d92d1ec9ba61d9e70156fa2cf4c96b06378d837322b1"),
        (HUB / "span_hub.h", "310160cf0b3bf753daa199193a5674e9e223e036af467e320cc5905e415b3454"),
        (SPAN / "rtp_h264_spans.c", "a608fe8ee8a292b1172ce62c62f19c2a61f9969f64d71385f1a742fdd326560e"),
        (SPAN / "rtp_h264_spans.h", "41126a8b9827894d6ecb71788cd1cc71bf3b5b036e44fddf998f4c5740f05f91"),
    ]:
        if pin(p)["sha256"] != expected:
            raise SystemExit("STOP: accepted input drift; no compilation")
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
    common = ["-std=c11", "-Wall", "-Wextra", "-Werror", "-Wpedantic",
              "-Wno-misleading-indentation", "-fno-common", "-fstack-usage",
              "-I" + str(ROOT / "framework/include")]
    units = [("reserve", SRC / "memory_reserve.c"), ("runtime", SRC / "memory_runtime.c"),
             ("reader", SRC / "memory_reader.c"), ("hub", HUB / "span_hub.c"),
             ("spans", SPAN / "rtp_h264_spans.c")]
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
        objects = []
        for unit, source in units:
            obj = OUT / "objects" / (role + "-" + unit + ".o")
            run(role + "-compile-" + unit, [cc] + common + flags + [
                "-MD", "-MF", obj.with_suffix(".d"), "-c", source, "-o", obj])
            objects.append(obj)
        rel = OUT / (role + "-MEMORY-REAL-HUB-SPANS-REL.o")
        run(role + "-integration-REL", [found["ld"], "-r", "-t",
            "-Map=" + str(OUT / (role + "-rel.map"))] + objects + ["-o", rel])
        elf = run(role + "-REL-ELF", ["/usr/bin/readelf", "-h", "-A", rel])
        symbols = run(role + "-REL-symbols", ["/usr/bin/nm", "-g", rel])
        if "REL (Relocatable file)" not in elf:
            raise SystemExit("STOP: unexpected ELF type; no execution")
        for name in ("nh_memory_prefault_span_hub", "nh_memory_credit_span_hub",
                     "nh_memory_span_hub_forecast", "nh_memory_read_fixed",
                     "camera_span_hub_init_v1", "camera_rtp_h264_spans_packet_v1"):
            if not any(line.split()[-2:] == ["T", name] for line in symbols.splitlines()):
                raise SystemExit("STOP: missing actual integration symbol")
        if role == "arm" and not any(line.split() == ["U", "__ehdr_start"] for line in symbols.splitlines()):
            raise SystemExit("STOP: ARM static layout anchor missing")
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
        "C_compiles": 10, "real_integration_REL_links": 2, "commands": commands,
        "artifacts": [pin(p) for p in sorted(OUT.rglob("*")) if p.is_file()],
        "inputs_tools_unchanged": True, "production_or_test_executions": 0,
        "executable_built": False, "compiler_closure_or_native_ABI_claim": False})
    print("COMPILE_REL_INSPECT_PASS C_compiles=10 REL_links=2 commands=26 NOT_RUN")


if __name__ == "__main__":
    main()
