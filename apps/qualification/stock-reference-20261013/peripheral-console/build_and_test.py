#!/usr/bin/env python3
"""Reproducible source/host-only audio7 build. ARM is never executed."""
import datetime
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[4]
SRC = Path(__file__).resolve().parent
BASE = ROOT / "evidence/delegated/20261013-stock-reference/peripheral-console07"
TC = ROOT / ("work/lightbulb-camera/implementation-20261008T205404Z/sources/"
             "toolchain-armv5-musl-202508/armv5-eabi--musl--stable-2025.08-1")


def pin(p):
    p = Path(p)
    return {"path": str(p), "bytes": p.stat().st_size,
            "sha256": hashlib.sha256(p.read_bytes()).hexdigest()}


def main():
    if len(sys.argv) != 2 or not re.fullmatch(r"build[0-9]{2}", sys.argv[1]):
        raise SystemExit("usage: build_and_test.py buildNN (new output only)")
    out = BASE / sys.argv[1]
    out.mkdir(exist_ok=False)
    (out / "source").mkdir()
    (out / "tmp").mkdir()
    for p in SRC.iterdir():
        if p.is_file(): shutil.copyfile(p, out / "source" / p.name)
    env = {"PATH": "/usr/bin:/bin", "LC_ALL": "C", "TMPDIR": str(out / "tmp"),
           "SOURCE_DATE_EPOCH": "0", "ASAN_OPTIONS": "detect_leaks=1:abort_on_error=1",
           "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"}
    report = {"started_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
              "state": "RUNNING", "commands": [], "environment": env,
              "native_executed": False, "hardware_operations": False}

    def run(label, argv):
        start = datetime.datetime.now(datetime.timezone.utc).isoformat()
        p = subprocess.run([str(x) for x in argv], cwd=out, env=env,
                           capture_output=True, timeout=90)
        for suffix, data in [("stdout.txt", p.stdout), ("stderr.txt", p.stderr)]:
            with (out / (label + "." + suffix)).open("xb") as f: f.write(data)
        report["commands"].append({"label": label, "argv": [str(x) for x in argv],
                                   "started_utc": start, "returncode": p.returncode,
                                   "ended_utc": datetime.datetime.now(datetime.timezone.utc).isoformat()})
        if p.returncode: raise RuntimeError(f"{label} failed: {p.returncode}; evidence preserved")
        return p.stdout.decode()

    try:
        cc = TC / "bin/arm-buildroot-linux-musleabi-gcc"
        nm = TC / "bin/arm-buildroot-linux-musleabi-nm"
        readelf = TC / "bin/arm-buildroot-linux-musleabi-readelf"
        objdump = TC / "bin/arm-buildroot-linux-musleabi-objdump"
        for label, tool in [("arm", cc), ("host", Path("/usr/bin/cc")), ("readelf", readelf), ("objdump", objdump)]:
            run(label + "-version", [tool, "--version"])
        run("arm-config", [cc, "-v"])
        assert "musl" in (TC / "summary.csv").read_text() and "1.2.5" in (TC / "summary.csv").read_text()
        report["toolchain_inputs"] = [pin(p) for p in [cc.resolve(), TC / "bin/arm-buildroot-linux-musleabi-gcc.br_real",
                                                       nm.resolve(), readelf.resolve(), objdump.resolve(), TC / "summary.csv", TC / "buildroot.config", TC / "README.txt"]]
        flags = ["-std=c11", "-Wall", "-Wextra", "-Werror", "-fno-common"]
        source = out / "source"
        for mode, extra in [("optimized", ["-O2"]), ("sanitized", ["-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"])]:
            binary = out / ("host-test-" + mode)
            run(mode + "-compile", ["/usr/bin/cc", *flags, *extra, "-MD", "-MF", out / (mode + ".d"), source / "host_test.c", "-o", binary])
            result = json.loads(run(mode + "-tests", [binary]))
            assert result["host_cases"] >= 120 and result["real_CLI_POSIX_MMIO"]
            assert result["five_second_wraps_total"] >= 18 and result["max_ISR_zero_end"] == 53248
            assert result["logical_ring_bytes"] == 49152 and result["backing_bytes"] == 65536
            assert result["tone_scratch_bytes"] == 512 and not result["native_executed"]
            long = result["examples"]["long_tone"]
            assert long["record"] == "stock-tone-control-v7" and long["state"] == "TONE_COMPLETED_AWAIT_USER"
            assert long["frequency_hz"] == 4005 and long["duration_ms"] == 5000 and long["frames_generated"] == 40060
            assert long["digital_peak"] == 32767 and long["refill_periods"] >= 30
            assert long["gain_restored"] and long["close_ok"] and long["mux_restoration_verified"]
            assert long["HP_encoded_after"] == long["HP_encoded_before"]
            noop = result["examples"]["noop"]
            assert noop["state"] == "TONE_NOOP_ZERO_DURATION" and noop["effects"] == 0 and noop["frames_generated"] == 0
            cap = result["examples"]["capabilities"]
            assert cap["record"] == "peripheral-capabilities-v7" and cap["duration_max_ms"] == 5000
            status = result["examples"]["status"]
            assert status["record"] == "peripheral-status-v7" and status["state"] == "STATUS"
            assert status["calculated_DAC_rate"] == 8012 and status["frequency_max_hz"] == 4005
            assert status["HP_gain_code"] == 5 and status["ambient_path_present"] is True
            ambient = result["examples"]["ambient"]
            assert ambient == {"record": "stock-ambient-control-v1", "state": "READ_COMPLETED", "value": 1536,
                               "units": "stock_adc_scaled_uncalibrated", "higher_is_darker": True, "close_ok": True}
            report[mode] = result
        arch = ["-march=armv5te", "-marm", "-mfloat-abi=soft"]
        run("arm-compile", [cc, *flags, "-Os", *arch, "-fstack-usage", "-MD", "-MF", out / "native.d", "-c", source / "console.c", "-o", out / "native.o"])
        run("arm-link", [cc, *arch, "-static", "-no-pie", "-Wl,--build-id=none,-z,noexecstack", "-Wl,-Map=" + str(out / "native.map"), out / "native.o", "-o", out / "qc-peripheral-audio7-armv5"])
        imports = run("application-imports", [nm, "-u", out / "native.o"])
        assert not re.search(r"\b(system|execve|fork|malloc|calloc|realloc|free|pwrite|mprotect|socket|connect)\b", imports)
        c = (source / "console.c").read_text()
        assert c.count("r->sys[0x74/4]=") == 2 and c.count("r->gpio[3]=2U;") == 1
        assert not re.search(r"\[0x9c/4\]\s*=(?!=)", c)
        run("arm-object-assembly", [objdump, "-dr", out / "native.o"])
        run("arm-elf", [readelf, "-h", "-l", "-A", out / "qc-peripheral-audio7-armv5"])
        elf = (out / "qc-peripheral-audio7-armv5").read_bytes()
        assert elf[:6] == b"\x7fELF\x01\x01" and int.from_bytes(elf[18:20], "little") == 40
        off = int.from_bytes(elf[28:32], "little"); size = int.from_bytes(elf[42:44], "little"); num = int.from_bytes(elf[44:46], "little")
        assert all(int.from_bytes(elf[off+i*size:off+i*size+4], "little") not in (2, 3) for i in range(num))
        deps = set()
        for dep in out.glob("*.d"):
            for name in dep.read_text().replace("\\\n", " ").split(":", 1)[1].split():
                if Path(name).is_file(): deps.add(Path(name))
        report["source_header_dependencies"] = [pin(p) for p in sorted(deps)]
        for name in ["libc.a", "libgcc.a", "crt1.o", "crti.o", "crtn.o"]:
            path = run("link-input-" + name.replace(".", "-"), [cc, "-print-file-name=" + name]).strip()
            report.setdefault("link_inputs", []).append(pin(path))
        report["artifact"] = pin(out / "qc-peripheral-audio7-armv5")
        report["state"] = "HOST_PASS_NATIVE_UNEXECUTED_PARENT_RECOVERY_REVIEW_REQUIRED"
    except Exception as exc:
        report["state"] = "FAILED_PRESERVED"; report["error"] = str(exc); raise
    finally:
        report["finished_utc"] = datetime.datetime.now(datetime.timezone.utc).isoformat()
        with (out / "BUILD.json").open("x") as f: json.dump(report, f, indent=2); f.write("\n")
        rows = [pin(p) for p in sorted(out.rglob("*")) if p.is_file()]
        with (out / "SHA256SUMS").open("x") as f: f.write("".join(row["sha256"] + "  " + row["path"] + "\n" for row in rows))
    print(json.dumps({"output": str(out), "state": report["state"], "host_cases_per_mode": report["optimized"]["host_cases"], "native_executed": False}))


if __name__ == "__main__": main()
