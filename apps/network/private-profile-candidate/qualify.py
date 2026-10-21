#!/usr/bin/env python3
"""PUBLICDUMMY only. Never hash/read inventory any fixture/crypto payload files."""
import datetime
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
VENDOR = ROOT / "apps/auth-web/service/vendor/mbedtls"
PIN = ROOT / "apps/auth-web/dependency-pins.json"
ARM = ROOT / "work/lightbulb-camera/implementation-20261008T205404Z/sources/toolchain-armv5-musl-202508/armv5-eabi--musl--stable-2025.08-1/bin"
LIBS = ["aes", "gcm", "cipher", "cipher_wrap", "block_cipher", "md", "sha256", "hkdf", "platform", "platform_util", "constant_time"]


def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def stamp():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


def main():
    pins = json.loads(PIN.read_text())
    files = sorted(p for p in VENDOR.rglob("*") if p.is_file())
    tree = hashlib.sha256()
    for p in files:
        if p.is_symlink():
            raise RuntimeError("vendor symlink refused")
        tree.update(p.relative_to(VENDOR).as_posix().encode() + b"\0" + bytes.fromhex(sha(p)))
    expected = pins["trees"]["vendor/mbedtls"]
    if tree.hexdigest() != expected["sha256"] or len(files) != expected["files"]:
        raise RuntimeError("canonical Mbed TLS pin mismatch; no fetch/install")
    base = ROOT / "local/validation/private-profile"
    base.mkdir(parents=True, exist_ok=True)
    out = Path(tempfile.mkdtemp(prefix="publicdummy-", dir=base))
    (out / "compiler-scratch").mkdir()
    (out / "inputs").mkdir()
    for name in ["private_profile.c", "private_profile.h", "private_profile_test.c", "pp_mbedtls_config.h", "qualify.py"]:
        with (out / "inputs" / name).open("xb") as f:
            f.write((HERE / name).read_bytes())
    gcc, cpp = shutil.which("gcc"), shutil.which("g++")
    arm, readelf = ARM / "arm-buildroot-linux-musleabi-gcc", ARM / "arm-buildroot-linux-musleabi-readelf"
    report = {"started_utc": stamp(), "class": "HOST_PUBLICDUMMY_ONLY", "native": "HOLD-before-key-read",
              "arm_executed": False, "G6_accepted": False, "G8_accepted": False,
              "vendor_pin": {"version": "3.6.7", "files": len(files), "sha256": tree.hexdigest()},
              "sources": {str(p.relative_to(ROOT)): sha(p) for p in
                          [HERE / "private_profile.c", HERE / "private_profile.h", HERE / "private_profile_test.c", HERE / "pp_mbedtls_config.h", Path(__file__), PIN]},
              "vendor_sources": {str(p.relative_to(ROOT)): sha(p) for p in files},
              "commands": [], "tools": {}, "passed": False,
              "private_artifact_exclusion": "ALL fixture directories/key/nonce/tag/ciphertext/checkpoint/readback files excluded from content reads/hash inventory; source/header/compiler/ELF/object/log hashes only"}

    def run(argv, extra=None):
        env = {"PATH": "/usr/bin:/bin", "LC_ALL": "C", "LANG": "C", "TMPDIR": str(out / "compiler-scratch")}
        env.update(extra or {})
        started = stamp()
        p = subprocess.run([str(v) for v in argv], cwd=out, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=180)
        log = out / f"{len(report['commands']) + 1:03d}.log"
        with log.open("xb") as f:
            f.write(p.stdout)
        report["commands"].append({"argv": [str(v) for v in argv], "cwd": str(out), "started_utc": started,
                                   "ended_utc": stamp(), "exit": p.returncode, "specified_environment": env,
                                   "log": str(log), "log_sha256": sha(log)})
        if p.returncode:
            raise RuntimeError("retained failure log=" + str(log))
        return p.stdout.decode(errors="replace").strip()

    common = ["-std=c11", "-Wall", "-Wextra", "-Werror", "-I" + str(HERE), "-I" + str(VENDOR / "include"),
              '-DMBEDTLS_CONFIG_FILE="pp_mbedtls_config.h"']
    vendor_sources = [VENDOR / "library" / (name + ".c") for name in LIBS]
    own = HERE / "private_profile.c"
    test = HERE / "private_profile_test.c"
    try:
        for tool in [gcc, cpp] + ([arm, readelf] if arm.is_file() else []):
            if not tool:
                raise RuntimeError("existing compiler missing")
            report["tools"][str(tool)] = {"sha256": sha(tool)}
            run([tool, "--version"])
        runtime = {}
        for tool in [gcc] + ([arm] if arm.is_file() else []):
            names = ["libc.a", "libgcc.a", "include/stdint.h"]
            if str(tool) == str(gcc):
                names += ["libc.so.6", "libasan.so", "libubsan.so", "libstdc++.so"]
            for name in names:
                path = run([tool, "-print-file-name=" + name])
                if Path(path).is_file():
                    runtime[path] = {"sha256": sha(path), "bytes": Path(path).stat().st_size}
        report["selected_runtime_closure"] = runtime
        real_arm = arm.with_name(arm.name + ".br_real")
        if real_arm.is_file():
            report["tools"][str(real_arm)] = {"sha256": sha(real_arm)}
        for compiler, label, flags in [(gcc, "host", ["-O2"]),
                                       (gcc, "asan", ["-O1", "-g", "-fno-omit-frame-pointer", "-fsanitize=address,undefined", "-fno-sanitize-recover=all"])] + (
                [(arm, "arm", ["-Os", "-march=armv5te", "-marm", "-mfloat-abi=soft"])] if arm.is_file() else []):
            build = out / label
            build.mkdir()
            objects = []
            for source in [*vendor_sources, own]:
                obj = build / (source.stem + ".o")
                # Own fresh config/objects per mode, including crypto. Never cached TLS objects.
                run([compiler, *common, *flags, "-DPP_PUBLIC_DUMMY_BUILD", "-fstack-usage", "-MD", "-MF", build / (source.stem + ".d"),
                     "-c", source, "-o", obj])
                objects.append(obj)
            binary = build / ("test-ARM-NOTRUN" if label == "arm" else "test-HOST")
            link = ["-static"] if label == "arm" else (["-no-pie"] if label == "asan" else [])
            run([compiler, *common, *flags, "-MD", "-MF", build / "test.d", "-DPP_PUBLIC_DUMMY_TEST", "-DPP_PUBLIC_DUMMY_BUILD", test, *objects,
                 "-Wl,--wrap=pread", *link, "-o", binary])
            if label == "arm":
                run([readelf, "-h", "-A", "-l", "-d", binary])
            else:
                env = {"ASAN_OPTIONS": "detect_leaks=1:abort_on_error=1:disable_coredump=1", "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"} if label == "asan" else None
                run([binary, out / (label + "-SOURCE_PRIVATE-fixtures")], env)
            if label == "host":
                smoke = build / "smoke.cpp"
                with smoke.open("x") as f:
                    f.write('#include "private_profile.h"\nint main(){pp_session s{};pp_options o{};o.mode=PP_NATIVE;return pp_session_init(&s,&o)==PP_HOLD?0:1;}\n')
                run([cpp, "-std=c++11", "-Wall", "-Wextra", "-Werror", "-I" + str(HERE), "-I" + str(VENDOR / "include"),
                     '-DMBEDTLS_CONFIG_FILE="pp_mbedtls_config.h"', "-MD", "-MF", build / "cpp.d", smoke, *objects, "-o", build / "cpp-HOST"])
                run([build / "cpp-HOST"])
                # Non-publicdummy compile is always HOLD; separate source closure.
                run([gcc, *common, "-O2", "-Wno-unused-function", "-c", own, "-o", build / "native-HOLD.o"])
        closure = {}
        for mode in ["host", "asan", "arm"]:
            if not (out / mode).exists():
                continue
            for dep in (out / mode).glob("*.d"):
                for name in dep.read_text().replace("\\\n", " ").split(":", 1)[1].split():
                    path = Path(name)
                    if path.is_file():
                        closure[str(path)] = {"sha256": sha(path), "bytes": path.stat().st_size}
        report["actual_header_source_closure"] = closure
        report["passed"] = True
    finally:
        report["ended_utc"] = stamp()
        # Deliberately NO rglob(out)! Only declared build trees + root scalar logs.
        artifacts = list(out.glob("*.log"))
        for mode in ["inputs", "host", "asan", "arm"]:
            if (out / mode).exists():
                artifacts += [p for p in (out / mode).iterdir() if p.is_file()]
        report["public_build_artifacts"] = {str(p.relative_to(out)): {"sha256": sha(p), "bytes": p.stat().st_size} for p in sorted(artifacts)}
        with (out / "REPORT.json").open("x") as f:
            json.dump(report, f, indent=2)
            f.write("\n")
        print(out / "REPORT.json")


if __name__ == "__main__":
    main()
