#!/usr/bin/env python3
"""A1 host-only sealed source extraction; no project imports or device access.

Never runs a vendor Makefile or cross compiler. Each output directory is new.
All inputs are public pinned C/headers or this lane's synthetic test sources.
"""
import sys

if sys.flags.optimize:
    raise SystemExit("refuse optimized Python")
import argparse
import datetime
import difflib
import hashlib
import json
from pathlib import Path
import re
import subprocess

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
SOURCE = HERE / "source"
ORIGINAL = ROOT / "work/lightbulb-camera/foundation-20261011T041148Z/artifacts/native-usb76-source01/station76-final"
EVIDENCE = ROOT / "docs/qualification-plans/kernel76-transport-20261021/station-control-a1-host01"
parser = argparse.ArgumentParser()
parser.add_argument("--run", required=True)
args = parser.parse_args()
if not re.fullmatch(r"run[0-9]{2}", args.run):
    raise SystemExit("runNN required")
OUT = EVIDENCE / args.run
OUT.mkdir(exist_ok=False)
inputs, commands, extractions = {}, [], []


def utc():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


def sha(data):
    return hashlib.sha256(data).hexdigest()


def read(path):
    data = path.read_bytes()
    inputs[str(path.relative_to(ROOT))] = {"bytes": len(data), "sha256": sha(data)}
    return data


def write(path, data):
    with path.open("xb") as f:
        f.write(data.encode() if isinstance(data, str) else data)


def command(label, argv, expected=0):
    start = utc()
    result = subprocess.run(argv, cwd=ROOT, capture_output=True, timeout=60,
                            env={"PATH": "/usr/bin:/bin", "LC_ALL": "C",
                                 "ASAN_OPTIONS": "detect_leaks=1:abort_on_error=0",
                                 "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"})
    write(OUT / (label + ".stdout"), result.stdout)
    write(OUT / (label + ".stderr"), result.stderr)
    commands.append({"label": label, "argv": [str(x) for x in argv], "cwd": str(ROOT),
                     "utc_start": start, "utc_end": utc(), "exit": result.returncode,
                     "stdout_sha256": sha(result.stdout), "stderr_sha256": sha(result.stderr)})
    if expected is not None and result.returncode != expected:
        raise RuntimeError(f"{label}: exit {result.returncode}, expected {expected}")
    return result


def extract(text, start, end, path):
    if text.count(start) != 1 or text.count(end) != 1:
        raise RuntimeError("nonunique extraction boundary")
    first, last = text.index(start), text.index(end)
    if first >= last:
        raise RuntimeError("extraction order")
    body = text[first:last]
    extractions.append({"source": str(path.relative_to(ROOT)), "start_byte": first,
                        "bytes": last - first, "start_line": text.count("\n", 0, first) + 1,
                        "sha256": sha(body.encode()), "start_marker": start, "end_marker": end})
    return body


def main():
    baseline = json.loads(read(EVIDENCE / "copy-baseline.json"))
    if baseline["count"] != 67:
        raise RuntimeError("not 67 pinned files")
    changed = []
    for item in baseline["files"]:
        rel = item["path"]
        original, copied = read(ORIGINAL / rel), read(SOURCE / rel)
        if sha(original) != item["original_sha256"] or item["copied_sha256"] != sha(original):
            raise RuntimeError("original/baseline changed")
        if original != copied:
            changed.append(rel)
            write(OUT / (rel.replace("/", "_") + ".diff"), "".join(difflib.unified_diff(
                original.decode().splitlines(keepends=True), copied.decode().splitlines(keepends=True),
                fromfile="sealed67/" + rel, tofile="candidate/" + rel)))
    if changed != ["utils/fwctrl.c", "utils/fwctrl.h"]:
        raise RuntimeError(f"unexpected changed source files: {changed}")
    admitted = {x["path"] for x in baseline["files"]} | {"utils/control_receipt.h"}
    if {str(p.relative_to(SOURCE)) for p in SOURCE.rglob("*") if p.is_file()} != admitted:
        raise RuntimeError("unexpected file in source tree")
    for path in (HERE / "tests/shim.h", HERE / "tests/test_control.c", Path(__file__),
                 HERE / "prepare_source.py", SOURCE / "utils/control_receipt.h"):
        read(path)
    inc = OUT / "include"
    (inc / "linux").mkdir(parents=True)
    for name in ("completion.h", "workqueue.h", "unaligned.h", "etherdevice.h", "build_bug.h"):
        write(inc / "linux" / name, '#include "shim.h"\n')
    write(inc / "linux/errno.h", "#include_next <linux/errno.h>\n")
    definitions = []
    constants = ["HGIC_WDEV_ID_STA", "HGIC_HDR_TX_MAGIC", "HGIC_HDR_RX_MAGIC",
                 "HGIC_WLAN_8400", "HGIC_TX_COOKIE_MASK", "HGIC_CMD_TIMEOUT"]
    defs = read(SOURCE / "hgic_def.h").decode()
    for name in constants:
        matches = re.findall(r"^#define " + name + r"\s+[^\r\n]+", defs, re.M)
        if len(matches) != 1:
            raise RuntimeError("nonunique ABI constant")
        definitions += matches
    def unit(source, header, legacy=False, code_override=None, receipt_override=None):
        path = source / "utils/fwctrl.c"
        # decode retains CRLF for byte-accurate extraction extents.
        text = read(path).decode() if code_override is None else code_override
        prefix = extract(text, "#define STR_LEN", "void hgic_fwctrl_init(", path)
        info = extract(text, "int hgic_fwctrl_get_fwinfo(", "int hgic_fwctrl_set_countryregion(", path)
        receipt = receipt_override or str(SOURCE / "utils/control_receipt.h")
        return ('#include "shim.h"\n' + ('#define A1_LEGACY 1\n' if legacy else '') +
                '#include "' + str(source / "hgic.h") + '"\n' +
                "\n".join(definitions) + '\n#include "' + str(header) + '"\n' +
                ('' if legacy else '#include "' + receipt + '"\n') + prefix + info +
                '\n#include "' + str(HERE / "tests/test_control.c") + '"\n')

    version = command("compiler-version", ["cc", "--version"])
    if b"clang" not in version.stdout.lower() and b"gcc" not in version.stdout.lower() and b"free software foundation" not in version.stdout.lower():
        raise RuntimeError("unknown host compiler")
    target = command("compiler-target", ["cc", "-dumpmachine"])
    if target.stdout.strip() != b"x86_64-pc-linux-gnu":
        raise RuntimeError("not the selected native x86_64 host compiler")
    common = ["cc", "-std=gnu11", "-g", "-Wall", "-Wextra", "-Werror",
              "-Wno-unused-function", "-Wno-unused-parameter", "-Wno-sign-compare",
              "-I" + str(inc), "-I" + str(HERE / "tests")]
    candidate = OUT / "candidate-extracted.c"
    write(candidate, unit(SOURCE, SOURCE / "utils/fwctrl.h"))
    for label, flags in (("optimized", ["-O2"]),
                         ("sanitized", ["-O1", "-fsanitize=address,undefined", "-fno-omit-frame-pointer"])):
        binary = OUT / (label + ".host")
        command(label + "-compile", common + flags + [str(candidate), "-o", str(binary)])
        command(label + "-test", [str(binary)])

    legacy = OUT / "original-extracted.c"
    write(legacy, unit(ORIGINAL, ORIGINAL / "utils/fwctrl.h", legacy=True))
    legacy_binary = OUT / "original-sanitized.host"
    command("original-compile", common + ["-O1", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                                          str(legacy), "-o", str(legacy_binary)])
    command("original-invalid-identity-repro", [str(legacy_binary)])
    failure = command("original-short-payload-repro", [str(legacy_binary), "short-payload"], expected=None)
    if failure.returncode == 0 or b"heap-buffer-overflow" not in failure.stderr:
        raise RuntimeError("original memory failure not reproduced by ASan")

    receipt_text = read(SOURCE / "utils/control_receipt.h").decode()
    code = read(SOURCE / "utils/fwctrl.c").decode()
    mutations = [
        ("missing-min-header", "header", "len >= sizeof(struct hgic_ctrl_hdr)", "len >= 0"),
        ("missing-opcode", "header", "actual_opcode != opcode", "false && actual_opcode != opcode"),
        ("missing-type", "header", "data[2] != type", "false"),
        ("missing-declared-length", "header", "get_unaligned((const u16 *)(data + 4)) == len", "true"),
        ("missing-ifidx", "header", "(data[3] & 0x0f) != ifidx", "false"),
        ("missing-cookie", "header", "get_unaligned((const u16 *)(data + 6)) != cookie", "false"),
        ("unsigned-status", "header", "value = (s16)", "value = (u16)"),
        ("missing-version", "header", "get_unaligned_le32(data) != 0x02010001", "false"),
        ("missing-SVN", "header", "get_unaligned_le32(data + 4) != 22118", "false"),
        ("missing-chip", "header", "get_unaligned_le16(data + 8) != HGIC_WLAN_8400", "false"),
        ("missing-CPU", "header", "get_unaligned_le16(data + 10) != 1", "false"),
        ("missing-retail-length", "header", "len != 24", "false"),
        ("missing-MAC", "header", "is_valid_ether_addr(mac)", "true"),
        ("missing-capacity", "header", "(size_t)value > capacity", "false"),
        ("cookie-reuse", "code", "cookie = ctrl->cookie <= HGIC_TX_COOKIE_MASK ? ctrl->cookie++ : 0xffff;",
         "cookie = ctrl->cookie++; ctrl->cookie &= HGIC_TX_COOKIE_MASK;"),
        ("replace-first-reply", "code", "if (resp->done)", "if (false)"),
    ]
    mutants = []
    for name, where, old, new in mutations:
        original = receipt_text if where == "header" else code
        if original.count(old) != 1:
            raise RuntimeError("mutation not unique: " + name)
        changed_text = original.replace(old, new)
        mut_header = OUT / (name + ".h")
        if where == "header":
            write(mut_header, changed_text)
        mut_source = OUT / (name + ".c")
        write(mut_source, unit(SOURCE, SOURCE / "utils/fwctrl.h",
                              code_override=changed_text if where == "code" else code,
                              receipt_override=str(mut_header) if where == "header" else None))
        binary = OUT / (name + ".host")
        command(name + "-compile", common + ["-Wno-type-limits", "-O1", "-fsanitize=address,undefined",
                                            "-fno-omit-frame-pointer", str(mut_source), "-o", str(binary)])
        killed = command(name + "-test", [str(binary)], expected=None)
        if killed.returncode == 0:
            raise RuntimeError("surviving mutant: " + name)
        mutants.append({"name": name, "target": where, "old": old, "new": new,
                        "killed": True, "runtime_exit": killed.returncode})
    # Verify the sealed originals again after all host programs. They are never
    # linked or executed; only controlled extracted functions ran with fake I/O.
    for item in baseline["files"]:
        if sha(read(ORIGINAL / item["path"])) != item["original_sha256"]:
            raise RuntimeError("original source changed during host run")
    return {"all_pass": True, "changed_original_files": changed,
            "new_source_files": ["utils/control_receipt.h"], "mutants": mutants,
            "original_failure_preserved": True,
            "limitations": "single-threaded host primitive model, not kernel async/USB/attach qualification"}


result = {"utc_start": utc(), "scope": "A1_PUBLIC_SOURCE_SYNTHETIC_HOST_ONLY_ATTACH_NO_GO",
          "argv": sys.argv, "python": sys.version, "runner_sha256": sha(Path(__file__).read_bytes())}
try:
    result.update(main())
except Exception as exc:
    result.update({"all_pass": False, "failure": str(exc)})
    raise
finally:
    result.update({"utc_end": utc(), "inputs": inputs, "commands": commands, "extractions": extractions})
    write(OUT / "result.json", json.dumps(result, indent=2) + "\n")
    outputs = {str(p.relative_to(OUT)): {"bytes": p.stat().st_size, "sha256": sha(p.read_bytes())}
               for p in sorted(OUT.rglob("*")) if p.is_file()}
    write(OUT / "output-hashes.json", json.dumps(outputs, indent=2) + "\n")
print("A1 host candidate PASS; full attach remains NO-GO")
