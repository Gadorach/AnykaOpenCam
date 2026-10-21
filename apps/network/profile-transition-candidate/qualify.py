#!/usr/bin/env python3
"""Offline actual-source integration. NEVER hash any PRIVATE fixture payloads."""
import datetime
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
PP = ROOT / "apps/network/private-profile-candidate"
VENDOR = ROOT / "apps/auth-web/service/vendor/mbedtls"
ARM = ROOT / "work/lightbulb-camera/implementation-20261008T205404Z/sources/toolchain-armv5-musl-202508/armv5-eabi--musl--stable-2025.08-1/bin"
LIBS = ["aes", "gcm", "cipher", "cipher_wrap", "block_cipher", "md", "sha256", "hkdf", "platform", "platform_util", "constant_time"]


def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def stamp():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


def main():
    pins = json.loads((ROOT / "apps/auth-web/dependency-pins.json").read_text())
    vendor = sorted(p for p in VENDOR.rglob("*") if p.is_file())
    h = hashlib.sha256()
    for p in vendor:
        if p.is_symlink():
            raise RuntimeError("vendor symlink")
        h.update(p.relative_to(VENDOR).as_posix().encode() + b"\0" + bytes.fromhex(sha(p)))
    if h.hexdigest() != pins["trees"]["vendor/mbedtls"]["sha256"]:
        raise RuntimeError("pinned vendor mismatch; no install/fetch")
    # Adopted sources are immutable inputs; detect unintended edits, not rewrite them.
    adopted = {}
    for manifest in ["evidence/delegated/20261012-private-profile/v2/MANIFEST.json",
                     "evidence/delegated/20261012-network-transaction-store/v2/MANIFEST.json"]:
        m = json.loads((ROOT / manifest).read_text())
        for name, expected in m["source_sha256"].items():
            if name.endswith((".c", ".h")):
                if sha(ROOT / name) != expected:
                    raise RuntimeError("reviewed source changed: " + name)
                adopted[name] = expected
    base = ROOT / "local/validation/profile-transition"
    base.mkdir(parents=True, exist_ok=True)
    out = Path(tempfile.mkdtemp(prefix="publicdummy-", dir=base))
    (out / "compiler-scratch").mkdir();(out / "inputs").mkdir()
    own = [HERE / "profile_transition.c", HERE / "profile_transition.h", HERE / "profile_transition_test.c", Path(__file__)]
    for p in own:
        with (out / "inputs" / p.name).open("xb") as f:
            f.write(p.read_bytes())
    core = [ROOT / "framework/src" / (n + ".c") for n in ["access_policy", "network_transition", "network_transaction_store", "settings_store"]]
    sources = [*[VENDOR / "library" / (n + ".c") for n in LIBS], PP / "private_profile.c", *core, HERE / "profile_transition.c"]
    gcc, cpp = shutil.which("gcc"), shutil.which("g++")
    arm, readelf = ARM / "arm-buildroot-linux-musleabi-gcc", ARM / "arm-buildroot-linux-musleabi-readelf"
    report = {"started_utc": stamp(), "class": "HOST_PUBLICDUMMY_SOURCE_SYNTHETIC_ONLY", "arm_executed": False,
              "hardware_operated": False, "SEC_accepted": False, "G6_accepted": False, "G8_accepted": False,
              "memory_constraints": "systemRAM40MiB[0x81800000,0x84000000),explicitCMA16MiB/defaultcompiled0; native2MiBsecurity unchanged",
              "source_sha256": {str(p.relative_to(ROOT)): sha(p) for p in [*own, *sources, PP / "private_profile.h", PP / "pp_mbedtls_config.h", ROOT / "framework/include/camera/access_policy.h", ROOT / "framework/include/camera/network_transaction_store.h"]},
              "adopted_source_pins": adopted, "vendor_tree_sha256": h.hexdigest(),
              "private_exclusion": "All fixture directories (including metadata/key/profile/nonce/tag/ciphertext/checkpoints/cookies) excluded from content/hash inventory; source/header/compiler/library/ELF/object/scalar logs only",
              "commands": [], "tools": {}, "passed": False}

    def run(argv, extra=None):
        env = {"PATH": "/usr/bin:/bin", "LANG": "C", "LC_ALL": "C", "TMPDIR": str(out / "compiler-scratch")}
        env.update(extra or {});started = stamp()
        p = subprocess.run([str(v) for v in argv], cwd=out, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=180)
        log = out / f"{len(report['commands']) + 1:03d}.log"
        with log.open("xb") as f:
            f.write(p.stdout)
        report["commands"].append({"argv": [str(v) for v in argv], "cwd": str(out), "started_utc": started,
                                   "ended_utc": stamp(), "exit": p.returncode, "specified_environment": env,
                                   "log": str(log), "log_sha256": sha(log)})
        if p.returncode:
            raise RuntimeError("failure preserved=" + str(log))
        return p.stdout.decode(errors="replace").strip()

    common = ["-std=c11", "-Wall", "-Wextra", "-Werror", "-I" + str(HERE), "-I" + str(PP),
              "-I" + str(ROOT / "framework/include"), "-I" + str(VENDOR / "include"), '-DMBEDTLS_CONFIG_FILE="pp_mbedtls_config.h"',
              "-DPP_PUBLIC_DUMMY_BUILD", "-DPT_PUBLIC_DUMMY_BUILD"]
    try:
        for tool in [gcc, cpp] + ([arm, readelf] if arm.is_file() else []):
            if not tool:
                raise RuntimeError("existing tool missing")
            report["tools"][str(tool)] = sha(tool);run([tool, "--version"])
        runtime = {}
        for compiler in [gcc] + ([arm] if arm.is_file() else []):
            for name in ["libc.a", "libgcc.a", "include/stdint.h"] + (["libc.so.6", "libasan.so", "libubsan.so", "libstdc++.so"] if str(compiler) == str(gcc) else []):
                path = run([compiler, "-print-file-name=" + name])
                if Path(path).is_file():
                    runtime[path] = sha(path)
        report["runtime_closure"] = runtime
        for compiler, mode, flags in [(gcc, "host", ["-O2"]), (gcc, "asan", ["-O1", "-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer"])] + (
                [(arm, "arm", ["-Os", "-march=armv5te", "-marm", "-mfloat-abi=soft"])] if arm.is_file() else []):
            build = out / mode;build.mkdir();objects = []
            for p in sources:
                obj = build / (p.stem + ".o")
                run([compiler, *common, *flags, "-MD", "-MF", build / (p.stem + ".d"), "-fstack-usage", "-c", p, "-o", obj]);objects.append(obj)
            binary = build / ("test-ARM-NOTRUN" if mode == "arm" else "integration-HOST")
            run([compiler, *common, *flags, "-DPT_PUBLIC_DUMMY_TEST", HERE / "profile_transition_test.c", *objects,
                 "-Wl,--wrap=syscall", "-Wl,--wrap=camera_network_transaction_query",
                 *( ["-static"] if mode == "arm" else ["-no-pie"] if mode == "asan" else []), "-o", binary])
            if mode == "arm":
                run([readelf, "-h", "-A", "-l", "-d", binary])
            else:
                summary = {"cases": 0, "checks": 0, "private_dirs": 0, "retained_HOLD_processes": 0, "scenarios": 30}
                for scenario in range(30):
                    # Process-isolated HOLD retains adapter/provider/lock through
                    # exit; never manually free/reinitialize to run another case.
                    text = run([binary, out / (mode + "-SOURCE_PRIVATE-fixtures-" + str(scenario)), str(scenario)],
                               {"ASAN_OPTIONS": "detect_leaks=1:abort_on_error=1:disable_coredump=1", "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"} if mode == "asan" else None)
                    values = dict(item.split("=", 1) for item in text.split() if "=" in item)
                    for key in ["cases", "checks", "private_dirs"]:
                        summary[key] += int(values[key])
                    summary["retained_HOLD_processes"] += int(values["retained_HOLD"])
                report[mode + "_integration_summary"] = summary
            if mode == "host":
                with (build / "smoke.cpp").open("x") as f:
                    f.write('#include "profile_transition.h"\nint main(){profile_transition s{};return pt_close(&s)==PT_INVALID?0:1;}\n')
                run([cpp, "-std=c++11", "-Wall", "-Wextra", "-Werror", "-I" + str(HERE), "-I" + str(PP), "-I" + str(ROOT / "framework/include"), "-I" + str(VENDOR / "include"),
                     '-DMBEDTLS_CONFIG_FILE="pp_mbedtls_config.h"', build / "smoke.cpp", *objects, "-o", build / "cpp-HOST"]);run([build / "cpp-HOST"])
        headers = {}
        for mode in ["host", "asan", "arm"]:
            if (out / mode).exists():
                for d in (out / mode).glob("*.d"):
                    for name in d.read_text().replace("\\\n", " ").split(":", 1)[1].split():
                        if Path(name).is_file():
                            headers[name] = sha(name)
        report["actual_source_header_closure"] = headers;report["passed"] = True
    finally:
        report["ended_utc"] = stamp();artifacts = list(out.glob("*.log"))
        for mode in ["inputs", "host", "asan", "arm"]:
            if (out / mode).exists():
                artifacts += [p for p in (out / mode).iterdir() if p.is_file()]
        report["public_build_artifacts"] = {str(p.relative_to(out)): {"sha256": sha(p), "bytes": p.stat().st_size} for p in sorted(artifacts)}
        with (out / "REPORT.json").open("x") as f:
            json.dump(report, f, indent=2);f.write("\n")
        print(out / "REPORT.json")


if __name__ == "__main__":
    main()
