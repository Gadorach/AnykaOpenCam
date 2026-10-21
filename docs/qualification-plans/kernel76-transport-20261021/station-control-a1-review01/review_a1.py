#!/usr/bin/env python3
"""Independent public-source audit and native HOST extraction, exclusive outputs.

No author runner/collector is executed or modified. No hardware/project imports.
"""
import datetime
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys

if sys.flags.optimize or sys.argv[1:]:
    raise SystemExit("no optimized Python or arguments")
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
APP = ROOT / "apps/qualification/kernel76-station"
SOURCE = APP / "source"
AUTHOR = HERE.parent / "station-control-a1-host01"
ORIGINAL = ROOT / "work/lightbulb-camera/foundation-20261011T041148Z/artifacts/native-usb76-source01/station76-final"
OUT = HERE / "run01"
OUT.mkdir(exist_ok=False)
inputs, commands, slices = {}, [], []


def utc():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


def digest(data):
    return hashlib.sha256(data).hexdigest()


def read(path):
    data = path.read_bytes()
    inputs[str(path.relative_to(ROOT))] = {"bytes": len(data), "sha256": digest(data)}
    return data


def write(path, data):
    with path.open("xb") as f:
        f.write(data.encode() if isinstance(data, str) else data)


def command(name, argv, failure=False):
    start = utc()
    p = subprocess.run([str(x) for x in argv], cwd=ROOT, capture_output=True, timeout=90,
                       env={"PATH": "/usr/bin:/bin", "LC_ALL": "C",
                            "ASAN_OPTIONS": "detect_leaks=1:abort_on_error=0",
                            "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"})
    write(OUT / (name + ".stdout"), p.stdout)
    write(OUT / (name + ".stderr"), p.stderr)
    commands.append({"name": name, "argv": [str(x) for x in argv], "cwd": str(ROOT),
                     "start": start, "end": utc(), "exit": p.returncode,
                     "stdout_sha256": digest(p.stdout), "stderr_sha256": digest(p.stderr)})
    if (not failure and p.returncode != 0) or (failure and p.returncode == 0):
        raise RuntimeError(name + ": unexpected runtime/compile exit " + str(p.returncode))
    return p


def extract(path, text, start, end):
    if text.count(start) != 1 or text.count(end) != 1:
        raise RuntimeError("nonunique extraction")
    first, last = text.index(start), text.index(end)
    assert first < last
    body = text[first:last]
    slices.append({"path": str(path.relative_to(ROOT)), "offset": first,
                   "bytes": last - first, "sha256": digest(body.encode())})
    return body


def main():
    read(Path(__file__))
    read(HERE / "supplement.c")
    seal = json.loads(read(AUTHOR / "candidate-seal.json"))
    for rel, pin in seal["files"].items():
        data = read(ROOT / rel)
        assert len(data) == pin["bytes"] and digest(data) == pin["sha256"], rel
    run = json.loads(read(AUTHOR / "run06/result.json"))
    assert run["all_pass"] and len(run["mutants"]) == 16
    for rel, pin in run["inputs"].items():
        data = read(ROOT / rel)
        assert len(data) == pin["bytes"] and digest(data) == pin["sha256"], rel
    for rel, pin in json.loads(read(AUTHOR / "run06/output-hashes.json")).items():
        data = read(AUTHOR / "run06" / rel)
        assert len(data) == pin["bytes"] and digest(data) == pin["sha256"], rel
    manifest = ROOT / "work/lightbulb-camera/foundation-20261011T041148Z/analysis/native-usb76-build01/prepare76-manifest.json"
    data = read(manifest)
    assert digest(data) == "686ad5da743ea3396962606f6349cf07f6d6c7454c84d572fbc5c4418bdd581d"
    pins = json.loads(data)["paired_radio_source75_pins"]
    baseline = json.loads(read(AUTHOR / "copy-baseline.json"))
    assert len(pins) == baseline["count"] == 67
    assert {"station75/" + x["path"]: x["original_sha256"] for x in baseline["files"]} == pins
    changed = []
    for x in baseline["files"]:
        a, b = read(ORIGINAL / x["path"]), read(SOURCE / x["path"])
        assert digest(a) == x["original_sha256"] == x["copied_sha256"]
        assert x["source_extent"] == {"offset": 0, "bytes": len(a)}
        assert not (ORIGINAL / x["path"]).is_symlink()
        if a != b:
            changed.append(x["path"])
    assert changed == ["utils/fwctrl.c", "utils/fwctrl.h"]
    assert {str(p.relative_to(SOURCE)) for p in SOURCE.rglob("*") if p.is_file()} == {x["path"] for x in baseline["files"]} | {"utils/control_receipt.h"}
    for name, pin in {"fwctrl.c": "67e95c81ad1d044475eb521f90f95ffc43702b50d2a6d4c8e4be6d6026750243", "fwctrl.h": "e35102bd358117fb02104e17894d053abc7a31a4940953d849c02b06e6fb27fc", "control_receipt.h": "3b80f917e989fd3587f61e94d9dea713bb26059d7243de5cf0c7bbaccc869073"}.items():
        assert digest(read(SOURCE / "utils" / name)) == pin
    inc = OUT / "include/linux"
    inc.mkdir(parents=True)
    for name in ("completion.h", "workqueue.h", "unaligned.h", "etherdevice.h", "build_bug.h"):
        write(inc / name, '#include "shim.h"\n')
    write(inc / "errno.h", '#include_next <linux/errno.h>\n')
    defs = read(SOURCE / "hgic_def.h").decode()
    constants = []
    for name in ("HGIC_WDEV_ID_STA", "HGIC_HDR_TX_MAGIC", "HGIC_HDR_RX_MAGIC", "HGIC_WLAN_8400", "HGIC_TX_COOKIE_MASK", "HGIC_CMD_TIMEOUT"):
        matches = re.findall(r"^#define " + name + r"\s+[^\r\n]+", defs, re.M)
        assert len(matches) == 1
        constants += matches

    def unit(tree, code=None, receipt=None, legacy=False):
        path = tree / "utils/fwctrl.c"
        text = read(path).decode() if code is None else code
        return ('#include "shim.h"\n' + ('#define A1_LEGACY 1\n' if legacy else '') +
                '#include "' + str(tree / "hgic.h") + '"\n' + '\n'.join(constants) +
                '\n#include "' + str(tree / "utils/fwctrl.h") + '"\n' +
                ('' if legacy else '#include "' + str(receipt or SOURCE / "utils/control_receipt.h") + '"\n') +
                extract(path, text, "#define STR_LEN", "void hgic_fwctrl_init(") +
                extract(path, text, "int hgic_fwctrl_get_fwinfo(", "int hgic_fwctrl_set_countryregion(") +
                '\n#include "' + str(APP / "tests/test_control.c") + '"\n')

    command("compiler-version", ["cc", "--version"])
    machine = command("compiler-target", ["cc", "-dumpmachine"])
    assert machine.stdout.strip() == b"x86_64-pc-linux-gnu"
    common = ["cc", "-std=gnu11", "-g", "-Wall", "-Wextra", "-Werror", "-Wno-unused-function", "-Wno-unused-parameter", "-Wno-sign-compare", "-I" + str(inc.parent), "-I" + str(APP / "tests"), "-I" + str(OUT)]
    candidate = unit(SOURCE)
    assert candidate.encode() == read(AUTHOR / "run06/candidate-extracted.c")
    write(OUT / "candidate.c", candidate)
    write(OUT / "original.c", unit(ORIGINAL, legacy=True))
    for name, src in (("candidate", OUT / "candidate.c"), ("supplement", HERE / "supplement.c")):
        for flavor, flags in (("optimized", ["-O2"]), ("sanitized", ["-O1", "-fsanitize=address,undefined", "-fno-omit-frame-pointer"])):
            label = name + "-" + flavor
            binary = OUT / (label + ".host")
            command(label + "-compile", common + flags + [src, "-o", binary])
            command(label + "-test", [binary])
    san = ["-O1", "-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
    binary = OUT / "original.host"
    command("original-compile", common + san + [OUT / "original.c", "-o", binary])
    command("original-identity-repro", [binary])
    failure = command("original-overread-repro", [binary, "short-payload"], failure=True)
    assert b"heap-buffer-overflow" in failure.stderr
    killed = []
    for mutant in run["mutants"]:
        name, where, old, new = (mutant[x] for x in ("name", "target", "old", "new"))
        path = SOURCE / "utils" / ("control_receipt.h" if where == "header" else "fwctrl.c")
        text = read(path).decode()
        assert text.count(old) == 1
        text = text.replace(old, new)
        header = None
        if where == "header":
            header = OUT / (name + ".h")
            write(header, text)
        src = OUT / (name + ".c")
        write(src, unit(SOURCE, code=text if where == "code" else None, receipt=header))
        binary = OUT / (name + ".host")
        command(name + "-compile", common + ["-Wno-type-limits"] + san + [src, "-o", binary])
        failure = command(name + "-test", [binary], failure=True)
        assert b"FAIL line" in failure.stderr or b"ERROR: AddressSanitizer" in failure.stderr or b"runtime error:" in failure.stderr
        killed.append({"name": name, "compiled": True, "runtime_exit": failure.returncode})
    # Revalidate all inspected inputs after runtime tests without importing or
    # invoking any source lane's scripts, old collector, target tool, or binary.
    for rel, pin in inputs.items():
        data = (ROOT / rel).read_bytes()
        assert len(data) == pin["bytes"] and digest(data) == pin["sha256"], rel
    return {"all_pass": True, "originals": 67, "unchanged_copies": 65,
            "changed": changed, "seal_and_run06_pins_verified": True,
            "runtime_mutants": killed, "original_identity_and_overread_reproduced": True}


result = {"start": utc(), "argv": sys.argv, "python": sys.version,
          "scope": "INDEPENDENT_PUBLIC_SOURCE_SYNTHETIC_HOST_ONLY_NO_BUILD_OR_TARGET_AUTHORITY"}
try:
    result.update(main())
except BaseException as exc:
    result.update({"all_pass": False, "failure": repr(exc)})
    raise
finally:
    result.update({"end": utc(), "inputs": inputs, "commands": commands, "extractions": slices})
    write(OUT / "result.json", json.dumps(result, indent=2) + "\n")
    outputs = {str(p.relative_to(OUT)): {"bytes": p.stat().st_size, "sha256": digest(p.read_bytes())}
               for p in sorted(OUT.rglob("*")) if p.is_file()}
    write(OUT / "output-hashes.json", json.dumps(outputs, indent=2) + "\n")
print("Independent A1 HOST reproduction and policy supplements PASS; attach remains NO-GO")
