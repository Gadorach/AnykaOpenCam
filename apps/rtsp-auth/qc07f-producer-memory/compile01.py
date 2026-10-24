# SPDX-License-Identifier: GPL-2.0-only
"""Fixed production compiles/rooted REL/ELF metadata only; no candidate calls."""
from pathlib import Path
import datetime
import difflib
import hashlib
import json
import re
import shlex
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[3]
SRC = ROOT / "apps/rtsp-auth/qc07f-producer-memory"
DOC = ROOT / "docs/qualification-plans/qc07f-producer-memory-20261024"
OUT = DOC / "runs/build01"
BASE = ROOT / "apps/rtsp-auth/qc07f-span-memory"
HUB = ROOT / "apps/rtsp-auth/span-hub-producer-candidate"
SPAN = ROOT / "apps/rtsp-auth/rtp-span-candidate"
OLD_TOOLS = ROOT / "docs/qualification-plans/qc07f-span-memory-20261024/runs/build01/TOOLS.json"
ARM = ROOT / ("work/lightbulb-camera/implementation-20261008T205404Z/sources/"
              "toolchain-armv5-musl-202508/armv5-eabi--musl--stable-2025.08-1/"
              "bin/arm-buildroot-linux-musleabi-gcc")
ENV = {"PATH": "/usr/bin:/bin", "LC_ALL": "C", "PYTHONDONTWRITEBYTECODE": "1",
       "TMPDIR": str(OUT / "tmp")}
NAMES = ("memory_reserve.c", "memory_reserve.h", "memory_runtime.c",
         "memory_runtime.h", "memory_reader.c", "handoff.h")
FROZEN = [
    "apps/rtsp-auth/qc07f-span-memory",
    "docs/qualification-plans/qc07f-span-memory-20261024",
    "docs/qualification-plans/qc07f-span-memory-review-20261024",
    "apps/rtsp-auth/span-hub-producer-candidate",
    "docs/qualification-plans/span-hub-producer-20261024",
    "docs/qualification-plans/span-hub-producer-review-20261024",
    "apps/rtsp-auth/rtp-span-candidate", "apps/rtsp-auth/span-hub-v2",
    "apps/rtsp-auth/qc07f-span-runtime-candidate",
    "apps/rtsp-auth/qc07f-span-serve-candidate",
]


def pin(path):
    p = Path(path).absolute()
    return {"path": str(p), "resolved": str(p.resolve()), "bytes": p.stat().st_size,
            "sha256": hashlib.sha256(p.read_bytes()).hexdigest()}


def dump(path, value):
    with path.open("x") as f:
        f.write(json.dumps(value, indent=2) + "\n")


def frozen():
    return {n: [pin(p) for p in sorted((ROOT / n).rglob("*")) if p.is_file()]
            for n in FROZEN}


def deltas():
    records = []
    for name in NAMES:
        a = (BASE / name).read_text().splitlines(keepends=True)
        b = (SRC / name).read_text().splitlines(keepends=True)
        if name not in ("memory_reserve.c", "memory_reserve.h"):
            assert a == b, "STOP: unchanged local unit drift: " + name
        for label, old, new in (("BASE_TO_PRODUCER", a, b), ("PRODUCER_TO_BASE", b, a)):
            diff = list(difflib.unified_diff(old, new, fromfile=label + "/old/" + name,
                                           tofile=label + "/new/" + name))
            rebuilt, cursor = [], 0
            for line in diff[2:]:
                if line.startswith("@@"):
                    m = re.match(r"@@ -(\d+)(?:,\d+)? \+\d+(?:,\d+)? @@", line)
                    start = int(m.group(1)) - 1
                    rebuilt.extend(old[cursor:start]); cursor = start
                elif line.startswith(" "):
                    assert old[cursor] == line[1:]
                    rebuilt.append(line[1:]); cursor += 1
                elif line.startswith("-"):
                    assert old[cursor] == line[1:]; cursor += 1
                elif line.startswith("+"):
                    rebuilt.append(line[1:])
                else:
                    raise SystemExit("STOP: unrecognized diff record")
            rebuilt.extend(old[cursor:])
            assert rebuilt == new, "STOP: inverse reconstruction mismatch"
            target = OUT / (label + "-" + name + ".diff")
            with target.open("x") as f:
                f.writelines(diff)
            records.append({"diff": pin(target), "reconstruction_exact": True})
    a, b = (BASE / "memory_reserve.c").read_text(), (SRC / "memory_reserve.c").read_text()
    markers = ("_Static_assert(CAMERA_SPAN_HUB_", "int nh_memory_credit_span_hub(")
    assert a[:a.index(markers[0])] == b[:b.index(markers[0])]
    assert a[a.index(markers[1]):] == b[b.index(markers[1]):]
    dump(OUT / "SOURCE_DELTA.json", {"base": [pin(BASE / n) for n in NAMES],
         "candidate": [pin(SRC / n) for n in NAMES], "diffs": records,
         "identical_files": list(NAMES[2:]),
         "identical_reserve_regions": ["before first hub static assertion",
                                        "credit_span_hub through EOF"],
         "binary_ABI_claim": False})


def main():
    OUT.mkdir(parents=True, exist_ok=False)
    for n in ("logs", "objects", "tmp"):
        (OUT / n).mkdir()
    commands, tools = [], {}
    try:
        protected = frozen()
        assert all(protected.values()), "STOP: missing frozen directory"
        dump(OUT / "FROZEN_INPUTS.json", protected)
        expected_base = (
            "3de7e750b5f846c1a77804113862881947074f42ed5718627efe70942337316a",
            "9893a15d37fd9867d1595bd0b5485fd7f19157b8ee28a7b4b3046cc94bd66e0e",
            "0e0434b5a8a80493e2582f8efa890fb634dd21543da35808ffd706c3a0a00332",
            "eafa9773488bdd03d0e4bcf2dfd218810c30351e5dc029275be0e0bac53dfcd5",
            "c9bd8401476f3baf73b43051651e6c306f14a05c1d5a5317531302569d4261c0",
            "9b88ce3e24a3a6c1c16336315323b70b29560c473ac9a48d36043bd60c840113")
        for name, expected in zip(NAMES, expected_base):
            assert pin(BASE / name)["sha256"] == expected, "STOP: baseline drift"
        for path, expected in (
            (HUB / "span_hub.c", "72e06177ae86c8659972ba8a59c81208302d44e0cb78f29b6114f8a4d5631f93"),
            (HUB / "span_hub.h", "460320064f85d1f1f8ada1a9afd1e282c287583fdff34787ba63c7354adeed8e"),
            (SPAN / "rtp_h264_spans.c", "a608fe8ee8a292b1172ce62c62f19c2a61f9969f64d71385f1a742fdd326560e"),
            (SPAN / "rtp_h264_spans.h", "41126a8b9827894d6ecb71788cd1cc71bf3b5b036e44fddf998f4c5740f05f91")):
            assert pin(path)["sha256"] == expected, "STOP: accepted source drift"
        selected = sorted(p for p in SRC.iterdir() if p.is_file()) + [
            DOC / "CONTRACT.md", DOC / "AUTHOR_AUDIT.md", OLD_TOOLS,
            ROOT / "framework/include/camera/backend.h",
            ROOT / "docs/qualification-plans/qc07f-span-serve-20261024/PARENT_DISPOSITION.md",
        ]
        inputs = [pin(p) for p in selected]
        dump(OUT / "INPUTS.json", inputs)
        deltas()
        retained = {p["resolved"]: p for p in json.loads(OLD_TOOLS.read_text())}

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
                raise SystemExit("STOP: " + name + " failed; preserved, no retry")
            return result.stdout.decode().strip()

        remember(Path("/usr/bin/python3"))
        common = ["-std=c11", "-Wall", "-Wextra", "-Werror", "-Wpedantic",
                  "-Wno-misleading-indentation", "-fno-common", "-fstack-usage",
                  "-ffunction-sections", "-fdata-sections",
                  "-I" + str(ROOT / "framework/include")]
        memory_roots = re.findall(r"^(?:int|void) (nh_memory_\w+)\(",
                                 (SRC / "memory_reserve.h").read_text() +
                                 (SRC / "memory_runtime.h").read_text(), re.M)
        hub_roots = re.findall(r"^int (camera_span_hub_\w+)\(",
                              (HUB / "span_hub.h").read_text(), re.M)
        assert len(memory_roots) == len(set(memory_roots)) == 16
        assert len(hub_roots) == len(set(hub_roots)) == 17
        roots = memory_roots + hub_roots
        units = [("reserve", SRC / "memory_reserve.c"), ("runtime", SRC / "memory_runtime.c"),
                 ("reader", SRC / "memory_reader.c"), ("hub", HUB / "span_hub.c"),
                 ("spans", SPAN / "rtp_h264_spans.c")]
        for role, cc, flags in (("host", Path("/usr/bin/cc"), ["-O2"]),
                                ("arm", ARM, ["-Os", "-march=armv5te", "-marm", "-mfloat-abi=soft"])):
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
            rel = OUT / (role + "-PRODUCER-MEMORY-REAL-HUB-SPANS-REL.o")
            run(role + "-integration-REL", [found["ld"], "-r", "--gc-sections", "-t",
                "-Map=" + str(OUT / (role + "-rel.map"))] +
                [arg for root in roots for arg in ("-u", root)] + objects + ["-o", rel])
            elf = run(role + "-REL-ELF", ["/usr/bin/readelf", "-h", "-A", "-S", rel])
            symbols = run(role + "-REL-symbols", ["/usr/bin/nm", "-g", rel])
            run(role + "-REL-relocations", ["/usr/bin/readelf", "-rW", rel])
            assert "REL (Relocatable file)" in elf and "Entry point address:               0x0" in elf
            for symbol in roots + ["camera_rtp_h264_spans_packet_v1"]:
                assert re.search(r"\bT " + symbol + r"$", symbols, re.M), symbol
            assert not re.search(r"\bU (camera_|nh_memory_)", symbols)
            assert not re.search(r"\b[TW] (main|_start)$", symbols, re.M)
            if role == "arm":
                assert re.search(r"\bU __ehdr_start$", symbols, re.M)
        dependencies = {}
        for dep in sorted((OUT / "objects").glob("*.d")):
            for word in shlex.split(dep.read_text().replace("\\\n", " ").split(":", 1)[1]):
                p = (ROOT / word).absolute()
                dependencies[str(p)] = pin(p)
        dump(OUT / "DEPENDENCIES.json", list(dependencies.values()))
        dump(OUT / "TOOLS.json", list(tools.values()))
        assert inputs == [pin(p) for p in selected], "STOP: selected input drift"
        assert protected == frozen(), "STOP: frozen bytes/membership drift"
        assert all(pin(p["path"]) == p for p in tools.values()), "STOP: tool drift"
        dump(OUT / "REPORT.json", {"status": "COMPILE_ROOTED_REL_METADATA_PASS_NOT_RUN",
             "utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
             "C_compiles": 10, "real_rooted_REL_links": 2, "commands": commands,
             "memory_roots": memory_roots, "hub_roots": hub_roots,
             "frozen_files": sum(map(len, protected.values())),
             "frozen_membership_bytes_unchanged": True, "inputs_tools_unchanged": True,
             "dependencies": len(dependencies), "tools": len(tools),
             "artifacts": [pin(p) for p in sorted(OUT.rglob("*")) if p.is_file()],
             "candidate_library_helper_test_function_calls": 0,
             "executable_built": False, "compiler_closure_or_native_ABI_claim": False})
        print("COMPILE_ROOTED_REL_METADATA_PASS compiles=10 REL=2 commands=28 NOT_RUN")
    except BaseException as error:
        dump(OUT / "FAILED_PRESERVED.json", {"status": "FAILED_PRESERVED_NO_RETRY",
             "error": str(error), "commands": commands, "tools": list(tools.values()),
             "artifacts": [pin(p) for p in sorted(OUT.rglob("*")) if p.is_file()],
             "candidate_function_calls": 0})
        raise


if __name__ == "__main__":
    main()
