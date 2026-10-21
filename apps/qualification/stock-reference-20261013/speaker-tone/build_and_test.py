#!/usr/bin/env python3
"""Build fixed ARM candidate; execute only POSIX-mocked host tests."""
import datetime
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[4]
SRC = Path(__file__).resolve().parent
BASE = ROOT / "evidence/delegated/20261013-stock-reference/speaker-tone06"
TC = ROOT / ("work/lightbulb-camera/implementation-20261008T205404Z/sources/"
             "toolchain-armv5-musl-202508/armv5-eabi--musl--stable-2025.08-1")


def pin(path):
    p = Path(path)
    return {"path": str(p), "sha256": hashlib.sha256(p.read_bytes()).hexdigest(),
            "bytes": p.stat().st_size}


def main():
    if len(sys.argv) != 2 or not re.fullmatch(r"build[0-9]{2}", sys.argv[1]):
        raise SystemExit("usage: build_and_test.py buildNN (must not exist)")
    out = BASE / sys.argv[1]
    out.mkdir(exist_ok=False)
    (out / "source").mkdir()
    (out / "tmp").mkdir()
    for p in sorted(SRC.iterdir()):
        if p.is_file():
            shutil.copyfile(p, out / "source" / p.name)
    env = {"PATH": "/usr/bin:/bin", "LC_ALL": "C", "TMPDIR": str(out / "tmp"),
           "SOURCE_DATE_EPOCH": "0", "ASAN_OPTIONS": "detect_leaks=1:abort_on_error=1",
           "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"}
    report = {"started_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
              "state": "RUNNING", "commands": [], "native_executed": False,
              "native_admission": "CONDITIONAL_PARENT_REVIEW_REQUIRED", "environment": env}

    def run(label, argv, expected=0):
        started = datetime.datetime.now(datetime.timezone.utc).isoformat()
        p = subprocess.run([str(a) for a in argv], cwd=out, env=env,
                           capture_output=True, timeout=60)
        for suffix, data in [("stdout.txt", p.stdout), ("stderr.txt", p.stderr)]:
            with (out / (label + "." + suffix)).open("xb") as f:
                f.write(data)
        report["commands"].append({"label": label, "argv": [str(a) for a in argv],
                                   "started_utc": started, "returncode": p.returncode,
                                   "ended_utc": datetime.datetime.now(datetime.timezone.utc).isoformat()})
        if p.returncode != expected:
            raise RuntimeError(f"{label}: return {p.returncode}, expected {expected}")
        return p.stdout.decode()

    try:
        cc = TC / "bin/arm-buildroot-linux-musleabi-gcc"
        nm = TC / "bin/arm-buildroot-linux-musleabi-nm"
        readelf = TC / "bin/arm-buildroot-linux-musleabi-readelf"
        run("arm-version", [cc, "--version"])
        run("arm-config", [cc, "-v"])
        run("host-version", ["/usr/bin/cc", "--version"])
        run("binutils-version", [readelf, "--version"])
        summary = (TC / "summary.csv").read_text()
        assert "musl" in summary and "1.2.5" in summary
        report["toolchain_summary"] = pin(TC / "summary.csv")
        report["musl_summary_rows"] = [r for r in summary.splitlines() if "musl" in r]
        report["toolchain_inputs"] = [pin(p) for p in [cc.resolve(),
            TC / "bin/arm-buildroot-linux-musleabi-gcc.br_real", nm.resolve(),
            readelf.resolve(), TC / "buildroot.config", TC / "README.txt"]]
        flags = ["-std=c11", "-Wall", "-Wextra", "-Werror", "-fno-common"]
        source = out / "source"
        prior = ROOT / "evidence/delegated/20261013-stock-reference/speaker-tone05/build02/source/tone.c"
        current = (source / "tone.c").read_text()
        unchanged = ["mark_cancel", "make_block", "stamp", "budget", "within", "pins", "baseline",
                     "store_out1", "store_mux_enter", "store_mux_exit", "block_controls", "controls_pending",
                     "enter_mux", "restore_quiescent", "dac_rate", "packet", "retain", "same_node",
                     "node", "text", "metadata", "release_views", "writable_views"]
        def function_text(text, name):
            match = re.search(r"^static [^;\n]*\b" + name + r"\([^;{}]*\)\s*\{", text, re.M)
            assert match, name
            start = match.end() - 1
            depth = 0
            for end in range(start, len(text)):
                depth += (text[end] == "{") - (text[end] == "}")
                if not depth:
                    return text[match.start():end+1]
            raise AssertionError(name)
        for name in unchanged:
            assert function_text(current, name) == function_text(prior.read_text(), name), name
        report["unchanged_native_core"] = {"reference": pin(prior), "byte_identical_functions": unchanged,
                                           "new_behavior": "gain ioctl lifecycle and read-only HP witnesses"}
        for mode, extra in [("optimized", ["-O2"]),
                            ("sanitized", ["-O1", "-g", "-fsanitize=address,undefined",
                                           "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"])]:
            binary = out / ("host-test-" + mode)
            run(mode + "-compile", ["/usr/bin/cc", *flags, *extra,
                                    "-MD", "-MF", out / (mode + ".d"),
                                    source / "host_test.c", "-o", binary])
            result = json.loads(run(mode + "-tests", [binary]))
            assert 190 <= result["host_cases"] <= 250 and result["actual_syscall_sequence"]
            assert result["digital_peak"] == 8192 and result["per_channel_sample_sum"] == 0
            assert result["max_helper_MMIO_writes"] == 3 and result["mux_enter_before_PCM"]
            assert result["baseline_maps_readonly"] and result["SYS78_writes"] is False
            assert result["HP_direct_stores"] == 0 and result["max_GAIN_SET_attempts"] == 2
            assert result["normal_return_restoration_checked"]
            examples = result["mock_result_examples"]
            for name, row in examples.items():
                assert row["record"] == "stock-speaker-tone-v6" and row["digital_peak"] == 8192
                assert row["physical_heard"] == "AWAIT_USER" and row["HP_direct_stores"] == 0
                assert row["bank_lease_verified_by_helper"] is False
                assert row["UART_disconnection_admission"] == "USER_UART_SIGNALS_DISCONNECTED"
                assert row["physical_disconnection_verified_by_helper"] is False
                held = name in ("active_failure", "restore_mismatch_retained")
                out_restore = name in ("restored", "set3_failed_restored")
                assert row["MMIO_write_count"] == (3 if out_restore else 1 if held else 2)
                assert row["MMIO_writes"] is True and row["mux_enter_writes"] == 1
                assert row["OUT0_restore_writes"] == (1 if out_restore else 0)
                assert row["mux_exit_writes"] == (0 if held else 1)
                assert row["OUT0_restore_after_close"] is out_restore
                assert row["SYS74_during"] == 0x02000184
                assert row["DIR0_during"] == 0x008000e6
                assert row["OUT0_during"] == (0 if name in ("restored", "active_failure", "restore_mismatch_retained") else 2)
                if name in ("restored", "already_restored"):
                    assert row["close_ok"] and row["latch_restoration_verified"] and row["OUT0_after"] == 2
                    assert row["SYS74_before"] == row["SYS74_after"] == 0x02000186
                    assert row["DIR0_before"] == row["DIR0_after"] == 0x008000e6
                    assert row["mux_exit_after_close"] and row["mux_restoration_verified"]
                    assert row["DMA_stop_evidence"] == "SOURCE_CONDITIONAL_PAUSE_IRQ_COMPLETION"
                    assert [row["gain_before"], row["gain_test"], row["gain_during"], row["gain_after"]] == [1,3,3,1]
                    assert [row["HP_encoded_before"], row["HP_encoded_during"], row["HP_encoded_after"]] == [8,2,8]
                    assert row["gain_restored"] and row["gain_SET_attempts"] == 2
                    assert ((row["HP9c_after_close"] >> 18) & 31) == 8
                elif held:
                    assert not row["close_ok"] and row["state"] == "RETAINED_RESET_REQUIRED"
                    assert not row["mux_restoration_verified"] and row["SYS74_after"] == 0x02000184
                    assert not row["gain_restored"]
                    if name == "restore_mismatch_retained":
                        assert row["gain_restore_attempts"] == 1 and row["gain_restore_return_zero"]
                        assert row["gain_after"] == 1 and row["HP_encoded_after"] == 2
                    else:
                        assert row["gain_restore_attempts"] == 0
                elif name == "set3_failed_restored":
                    assert row["state"] == "PREACTIVE_FAILED_CLOSED" and row["close_ok"]
                    assert row["gain_change_intent"] and row["gain_set3_attempts"] == 1
                    assert not row["gain_set3_return_zero"] and row["gain_restore_attempts"] == 1
                    assert row["gain_restored"] and row["gain_after"] == 1 and row["HP_encoded_after"] == 8
                else:
                    assert name == "unopened_rollback" and not row["close_ok"]
                    assert row["unopened_mux_rollback"] and row["mux_restoration_verified"]
                    assert row["SYS74_after"] == row["SYS74_before"] and row["DMA_stop_evidence"] == "NONE"
            report[mode] = result
        arch = ["-march=armv5te", "-marm", "-mfloat-abi=soft"]
        run("arm-compile", [cc, *flags, "-Os", *arch, "-fstack-usage", "-MD",
                             "-MF", out / "native.d", "-c", source / "tone.c",
                             "-o", out / "native.o"])
        run("arm-link", [cc, *arch, "-static", "-no-pie", "-Wl,--build-id=none,-z,noexecstack",
                          "-Wl,-Map=" + str(out / "native.map"), out / "native.o",
                          "-o", out / "stock-speaker-tone-armv5"])
        imports = run("application-imports", [nm, "-u", out / "native.o"])
        assert not re.search(r"\b(system|execve|fork|malloc|calloc|realloc|free|pwrite|mprotect)\b", imports)
        for symbol in ["open", "ioctl", "write", "mmap", "sigaction", "kill"]:
            assert re.search(r"\b" + symbol + r"\b", imports), symbol
        # Three fixed sites with separate host seams, no generic MMIO writer.
        tone_source = (source / "tone.c").read_text()
        assert tone_source.count("gpio[0x0c/4]=2U;") == 1
        assert tone_source.count("r->sys[0x74/4]=r->mux_before&~2U;") == 1
        assert tone_source.count("r->sys[0x74/4]=r->mux_before;") == 1
        assert len(re.findall(r"(?:r->)?sys\s*\[[^]]+\]\s*=(?!=)", tone_source)) == 2
        assert "const volatile uint32_t *view=r->sys;" in tone_source and "return view[0x9c/4];" in tone_source
        assert not re.search(r"\[0x9c/4\]\s*=(?!=)", tone_source)
        assert "PROT_READ|PROT_WRITE,MAP_SHARED,r->mem,0x20170000" in tone_source
        assert "PROT_READ|PROT_WRITE,MAP_SHARED,r->mem,0x08000000" in tone_source
        assert "PROT_READ,MAP_SHARED,r.mem,0x08000000" in tone_source
        assert "PROT_READ,MAP_SHARED,r.mem,0x20170000" in tone_source
        run("arm-object-assembly", [TC / "bin/arm-buildroot-linux-musleabi-objdump", "-dr", out / "native.o"])
        run("arm-elf", [readelf, "-h", "-l", "-A", out / "stock-speaker-tone-armv5"])
        elf = (out / "stock-speaker-tone-armv5").read_bytes()
        assert elf[:6] == b"\x7fELF\x01\x01" and int.from_bytes(elf[18:20], "little") == 40
        phoff = int.from_bytes(elf[28:32], "little")
        phsize = int.from_bytes(elf[42:44], "little")
        phnum = int.from_bytes(elf[44:46], "little")
        types = [int.from_bytes(elf[phoff+i*phsize:phoff+i*phsize+4], "little") for i in range(phnum)]
        assert 2 not in types and 3 not in types
        deps = set()
        for dep in out.glob("*.d"):
            for name in dep.read_text().replace("\\\n", " ").split(":", 1)[1].split():
                p = Path(name)
                if p.is_file(): deps.add(p)
        report["header_and_source_dependencies"] = [pin(p) for p in sorted(deps)]
        for name in ["libc.a", "libgcc.a", "crt1.o", "crti.o", "crtn.o"]:
            p = run("link-input-" + name.replace(".", "-"), [cc, "-print-file-name=" + name]).strip()
            report.setdefault("link_inputs", []).append(pin(p))
        report["native_artifact"] = pin(out / "stock-speaker-tone-armv5")
        report["state"] = "HOST_PASS_NATIVE_NOT_EXECUTED_CONDITIONAL_CANDIDATE"
    except Exception as exc:
        report["state"] = "FAILED_PRESERVED"
        report["error"] = str(exc)
        raise
    finally:
        report["finished_utc"] = datetime.datetime.now(datetime.timezone.utc).isoformat()
        with (out / "BUILD.json").open("x") as f:
            json.dump(report, f, indent=2); f.write("\n")
        rows = [pin(p) for p in sorted(out.rglob("*")) if p.is_file()]
        with (out / "SHA256SUMS").open("x") as f:
            f.write("".join(r["sha256"] + "  " + r["path"] + "\n" for r in rows))
    print(json.dumps({"output": str(out), "state": report["state"],
                      "host_cases_per_mode": report["optimized"]["host_cases"], "native_executed": False}))


if __name__ == "__main__":
    main()
