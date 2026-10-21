#!/usr/bin/env python3
"""Reproducible source/host-only speaker route08 build. ARM is never executed."""
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
BASE = ROOT / "evidence/delegated/20261016-speaker/route08"
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
        baseline = json.loads((BASE / "BASELINE.json").read_text())
        for row in baseline["inputs"]:
            assert pin(ROOT / row["path"])["sha256"] == row["sha256"], row["path"]
        report["baseline_inputs"] = baseline
        original = (ROOT / baseline["inputs"][0]["path"]).read_text()
        current = (source / "console.c").read_text()
        def body(text, name):
            match = re.search(r"^static [^\n]*\b" + name + r"\([^\n]*\)\n\{.*?^\}", text, re.M | re.S)
            assert match, name
            return match.group()
        primitives = ["root_runtime", "dac_rate", "packet", "triangle", "make_block", "progress", "submit_period"]
        for name in primitives:
            assert body(original, name) == body(current, name), name
        report["unchanged_OUR_console7_primitives"] = primitives
        for mode, extra in [("optimized", ["-O2"]), ("sanitized", ["-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"])]:
            binary = out / ("host-test-" + mode)
            run(mode + "-compile", ["/usr/bin/cc", *flags, *extra, "-MD", "-MF", out / (mode + ".d"), source / "host_test.c", "-o", binary])
            result = json.loads(run(mode + "-tests", [binary]))
            assert result["host_cases"] >= 9200 and result["real_CLI_POSIX_MMIO"]
            assert result["five_second_wraps_total"] >= 18 and result["max_ISR_zero_end"] == 53248
            assert result["logical_ring_bytes"] == 49152 and result["backing_bytes"] == 65536
            assert result["tone_scratch_bytes"] == 512 and not result["native_executed"]
            long = result["examples"]["long_tone"]
            assert long["record"] == "stock-speaker-route-v8" and long["state"] == "TONE_COMPLETED_AWAIT_USER"
            assert long["frequency_hz"] == 4005 and long["duration_ms"] == 5000 and long["frames_generated"] == 40060
            assert long["digital_peak"] == 32767 and long["refill_periods"] >= 30
            assert long["gain_restored"] and long["close_ok"] and long["mux_restoration_verified"]
            assert long["HP_encoded_after"] == long["HP_encoded_before"]
            assert long["speaker_enable_attempts"] == 1 and long["speaker_enable_word"] == 1 and long["speaker_enable_bytes"] == 4
            for field in ["speaker_enable_intent", "speaker_enable_return_zero", "speaker_enabled_low", "speaker_low_after_RESUME", "speaker_low_after_PAUSE", "speaker_low_after_close", "OUT_restore_after_close", "mux_restore_after_OUT_restore", "UART_isolation_token"]:
                assert long[field], field
            assert long["OUT0_restore_writes"] == 1 and long["MMIO_write_count"] == 3 and long["effects"] == 6
            assert long["HP_source_during_valid"] and long["HP_source_during"] == 4
            assert not long["physical_isolation_verified_by_helper"] and long["HP_direct_stores"] == 0
            noop = result["examples"]["noop"]
            assert noop["state"] == "TONE_NOOP_ZERO_DURATION" and noop["effects"] == 0 and noop["frames_generated"] == 0
            for field in ["speaker_enable_attempts", "speaker_enable_word", "speaker_enable_bytes", "gain_SET_attempts", "MMIO_write_count", "bytes_submitted", "OUT0_restore_writes", "periods_completed"]:
                assert noop[field] == 0, field
            assert not noop["speaker_enable_intent"] and not noop["speaker_enabled_low"] and not noop["HP_source_during_valid"]
            cap = result["examples"]["capabilities"]
            assert cap["record"] == "speaker-capabilities-v8" and cap["duration_max_ms"] == 5000 and cap["positive_tone_requires_uart_isolated"] and not cap["ambient_one_shot"]
            status = result["examples"]["status"]
            assert status["record"] == "speaker-status-v8" and status["state"] == "STATUS"
            assert status["calculated_DAC_rate"] == 8012 and status["frequency_max_hz"] == 4005
            assert status["HP_gain_code"] == 5 and status["ambient_path_present"] is True
            first = result["examples"]["first224ms"]
            assert first["frequency_hz"] == 1000 and first["gain_test"] == first["gain_before"] == first["gain_after"] == 1
            assert first["volume_percent"] == 25 and first["digital_peak"] == 8192 and first["duration_ms"] == 224
            assert first["speaker_enabled_low"] and first["mux_restore_after_OUT_restore"]
            report[mode] = result
        run("host-cli-compile", ["/usr/bin/cc", *flags, "-O2", source / "console.c", "-o", out / "host-cli"])
        assert json.loads(run("host-cli-capabilities", [out / "host-cli", "--capabilities"]))["record"] == "speaker-capabilities-v8"
        arch = ["-march=armv5te", "-marm", "-mfloat-abi=soft"]
        run("arm-compile", [cc, *flags, "-Os", *arch, "-fstack-usage", "-MD", "-MF", out / "native.d", "-c", source / "console.c", "-o", out / "native.o"])
        run("arm-link", [cc, *arch, "-static", "-no-pie", "-Wl,--build-id=none,-z,noexecstack", "-Wl,-Map=" + str(out / "native.map"), out / "native.o", "-o", out / "qc-speaker-route08-armv5"])
        imports = run("application-imports", [nm, "-u", out / "native.o"])
        assert not re.search(r"\b(system|execve|fork|malloc|calloc|realloc|free|pwrite|mprotect|socket|connect)\b", imports)
        c = (source / "console.c").read_text()
        assert c.count("r->sys[0x74/4]=") == 2 and c.count("r->gpio[3]=2U;") == 1
        assert not re.search(r"\[0x9c/4\]\s*=(?!=)", c)
        run("arm-object-assembly", [objdump, "-dr", out / "native.o"])
        assert "#define SPEAKER_ENABLE 0x40045081UL" in c and "--uart-isolated" in c
        assert "static int ambient(" not in c and "latch_effect" not in c
        run("arm-elf", [readelf, "-h", "-l", "-A", out / "qc-speaker-route08-armv5"])
        elf = (out / "qc-speaker-route08-armv5").read_bytes()
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
        report["artifact"] = pin(out / "qc-speaker-route08-armv5")
        for row in baseline["inputs"]:
            assert pin(ROOT / row["path"])["sha256"] == row["sha256"], row["path"]
        report["state"] = "HOST_PASS_NATIVE_UNEXECUTED_PARENT_INDEPENDENT_REVIEW_REQUIRED"
    except Exception as exc:
        report["state"] = "FAILED_PRESERVED"; report["error"] = str(exc); raise
    finally:
        report["finished_utc"] = datetime.datetime.now(datetime.timezone.utc).isoformat()
        with (out / "BUILD.json").open("x") as f: json.dump(report, f, indent=2); f.write("\n")
        rows = [pin(p) for p in sorted(out.rglob("*")) if p.is_file()]
        with (out / "SHA256SUMS").open("x") as f: f.write("".join(row["sha256"] + "  " + row["path"] + "\n" for row in rows))
    print(json.dumps({"output": str(out), "state": report["state"], "host_cases_per_mode": report["optimized"]["host_cases"], "native_executed": False}))


if __name__ == "__main__": main()
