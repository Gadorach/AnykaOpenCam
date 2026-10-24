# SPDX-License-Identifier: GPL-2.0-only
"""Fixed production compilation/REL/ELF metadata ONLY. No candidate calls."""
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
SRC = ROOT / "apps/rtsp-auth/span-hub-producer-candidate"
DOC = ROOT / "docs/qualification-plans/span-hub-producer-20261024"
OUT = DOC / "runs/build01"
BASE = ROOT / "apps/rtsp-auth/span-hub-v2"
SPAN = ROOT / "apps/rtsp-auth/rtp-span-candidate"
OLD_TOOLS = ROOT / "docs/qualification-plans/span-hub-20261024/runs/build01/TOOLS.json"
ARM = ROOT / ("work/lightbulb-camera/implementation-20261008T205404Z/sources/"
              "toolchain-armv5-musl-202508/armv5-eabi--musl--stable-2025.08-1/"
              "bin/arm-buildroot-linux-musleabi-gcc")
ENV = {"PATH": "/usr/bin:/bin", "LC_ALL": "C", "PYTHONDONTWRITEBYTECODE": "1",
       "TMPDIR": str(OUT / "tmp")}
FROZEN = [
    "apps/rtsp-auth/span-hub-candidate", "docs/qualification-plans/span-hub-20261024",
    "docs/qualification-plans/span-hub-review-20261024",
    "apps/rtsp-auth/span-hub-v2", "docs/qualification-plans/span-hub-v2-20261024",
    "docs/qualification-plans/span-hub-parent-20261024",
    "apps/rtsp-auth/rtp-span-candidate", "docs/qualification-plans/rtp-span-20261024",
    "apps/qualification/qc07f-encoder-output-spans",
    "docs/qualification-plans/qc07f-encoder-output-spans-20261024",
    "apps/rtsp-auth/qc07f-span-serve-candidate",
    "docs/qualification-plans/qc07f-span-serve-20261024",
]


def pin(path):
    p = Path(path).absolute()
    return {"path": str(p), "resolved": str(p.resolve()), "bytes": p.stat().st_size,
            "sha256": hashlib.sha256(p.read_bytes()).hexdigest()}


def dump(path, value):
    with path.open("x") as f:
        f.write(json.dumps(value, indent=2) + "\n")


def frozen():
    return {name: [pin(p) for p in sorted((ROOT / name).rglob("*")) if p.is_file()]
            for name in FROZEN}


def text_delta():
    records = []
    for name in ("span_hub.c", "span_hub.h"):
        a = (BASE / name).read_text().splitlines(keepends=True)
        b = (SRC / name).read_text().splitlines(keepends=True)
        for label, old, new in (("V2_TO_PRODUCER", a, b), ("PRODUCER_TO_V2", b, a)):
            diff = list(difflib.unified_diff(old, new, fromfile=label + "/old/" + name,
                                           tofile=label + "/new/" + name))
            # Independently reconstruct each side from the actual diff body.
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
                    raise SystemExit("STOP: unexpected diff record")
            rebuilt.extend(old[cursor:])
            assert rebuilt == new
            target = OUT / (label + "-" + name + ".diff")
            with target.open("x") as f:
                f.writelines(diff)
            records.append({"diff": pin(target), "inverse_reconstruction_exact": True})
    # Text inspection only: no C function is invoked.
    def body(text, name):
        start = text.index(name + "(")
        opening = text.index("{", start)
        depth = 1
        i = opening + 1
        while depth:
            depth += (text[i] == "{") - (text[i] == "}")
            i += 1
        return text[start:i]
    a, b = (BASE / "span_hub.c").read_text(), (SRC / "span_hub.c").read_text()
    unchanged = []
    for name in ("consumer_valid", "lose_backlog", "input_valid", "copy_input",
                 "camera_span_hub_packet_v1"):
        assert body(a, name) == body(b, name), name
        unchanged.append(name)
    dump(OUT / "SOURCE_DELTA.json", {"base": [pin(BASE / n) for n in ("span_hub.c", "span_hub.h")],
         "candidate": [pin(SRC / n) for n in ("span_hub.c", "span_hub.h")],
         "diffs": records, "byte_identical_C_function_text": unchanged,
         "consumer_identity_meaning_preserved_not_binary_ABI": True})


def main():
    OUT.mkdir(parents=True, exist_ok=False)
    for name in ("logs", "objects", "tmp"):
        (OUT / name).mkdir()
    protected = frozen()
    assert all(protected.values()), "STOP: missing frozen directory"
    dump(OUT / "FROZEN_INPUTS.json", protected)
    for path, expected in [
        (BASE / "span_hub.c", "bcf50041bb1fe1b20352d92d1ec9ba61d9e70156fa2cf4c96b06378d837322b1"),
        (BASE / "span_hub.h", "310160cf0b3bf753daa199193a5674e9e223e036af467e320cc5905e415b3454"),
        (SPAN / "rtp_h264_spans.c", "a608fe8ee8a292b1172ce62c62f19c2a61f9969f64d71385f1a742fdd326560e"),
        (SPAN / "rtp_h264_spans.h", "41126a8b9827894d6ecb71788cd1cc71bf3b5b036e44fddf998f4c5740f05f91"),
    ]:
        assert pin(path)["sha256"] == expected, "STOP: accepted source drift"
    selected = sorted(p for p in SRC.iterdir() if p.is_file()) + [
        DOC / "CONTRACT.md", DOC / "AUTHOR_AUDIT.md", OLD_TOOLS,
        ROOT / "framework/include/camera/backend.h", ROOT / "framework/include/camera/rtp_h264.h",
    ]
    inputs = [pin(p) for p in selected]
    dump(OUT / "INPUTS.json", inputs)
    text_delta()
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
            raise SystemExit("STOP: " + name + " failed; preserved, no retry")
        return result.stdout.decode().strip()

    remember(Path("/usr/bin/python3"))
    common = ["-std=c11", "-Wall", "-Wextra", "-Werror", "-Wpedantic", "-fno-common",
              "-fstack-usage", "-ffunction-sections", "-fdata-sections",
              "-I" + str(ROOT / "framework/include")]
    roots = re.findall(r"^int (camera_span_hub_\w+)\(", (SRC / "span_hub.h").read_text(), re.M)
    assert len(roots) == 17 and len(set(roots)) == 17
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
        run(role + "-integration-REL", [found["ld"], "-r", "--gc-sections", "-t",
            "-Map=" + str(OUT / (role + "-rel.map"))] +
            [arg for root in roots for arg in ("-u", root)] + [
            OUT / ("objects/" + role + "-hub.o"), OUT / ("objects/" + role + "-spans.o"), "-o", rel])
        elf = run(role + "-REL-ELF", ["/usr/bin/readelf", "-h", "-A", "-S", rel])
        symbols = run(role + "-REL-symbols", ["/usr/bin/nm", "-g", rel])
        run(role + "-REL-relocations", ["/usr/bin/readelf", "-rW", rel])
        assert "REL (Relocatable file)" in elf and "Entry point address:               0x0" in elf
        for symbol in roots + ["camera_rtp_h264_spans_packet_v1"]:
            assert re.search(r"\bT " + symbol + r"$", symbols, re.M), symbol
        assert not re.search(r"\bU camera_", symbols)
        assert not re.search(r"\b[TW] (main|_start)$", symbols, re.M)
    dependencies = {}
    for dep in sorted((OUT / "objects").glob("*.d")):
        for word in shlex.split(dep.read_text().replace("\\\n", " ").split(":", 1)[1]):
            p = (ROOT / word).absolute()
            dependencies[str(p)] = pin(p)
    dump(OUT / "DEPENDENCIES.json", list(dependencies.values()))
    dump(OUT / "TOOLS.json", list(tools.values()))
    assert inputs == [pin(p) for p in selected], "STOP: input drift"
    assert protected == frozen(), "STOP: frozen bytes/membership drift"
    assert all(pin(p["path"]) == p for p in tools.values()), "STOP: tool drift"
    dump(OUT / "REPORT.json", {"status": "COMPILE_ROOTED_REL_METADATA_PASS_NOT_RUN",
         "utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
         "C_compiles": 4, "real_rooted_REL_links": 2, "commands": commands,
         "roots": roots, "frozen_files": sum(map(len, protected.values())),
         "frozen_membership_bytes_unchanged": True, "selected_inputs_tools_unchanged": True,
         "dependencies": len(dependencies), "tools": len(tools),
         "artifacts": [pin(p) for p in sorted(OUT.rglob("*")) if p.is_file()],
         "candidate_library_helper_test_function_calls": 0, "executable_built": False,
         "compiler_closure_or_native_ABI_claim": False})
    print("COMPILE_ROOTED_REL_METADATA_PASS compiles=4 REL=2 commands=22 NOT_RUN")


if __name__ == "__main__":
    main()
