#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Offline HOST qualification and optional read-only pinned ARMv5 static link.

No installs, network, devices, private provisioning, vendor writes or ARM execution.
Every run gets a new local/validation/rtsp-auth directory; existing paths refused.
"""
import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[2]
APP = ROOT / "apps/rtsp-auth"
VENDOR = ROOT / "apps/auth-web/service/vendor/mbedtls"
VENDOR_PIN = "bab965e248c4b691cc84e8bccc7a6bd40101305b8ea789408896b64bfe2281f5"
ARM_PINS = {
    "bin/toolchain-wrapper": "f4db963a226e01244ce6f7b5d1ec6d57474dd811119ff8834429f022835ce9c1",
    "bin/arm-buildroot-linux-musleabi-gcc.br_real": "e24ea223e7b521cce1f5889d63c25417e930cdbb35653b072f7b48d9a4b41729",
    "arm-buildroot-linux-musleabi/sysroot/lib/libc.a": "0982970abc52a36d247326a8e0b5dd487410222ff9728528cc99a4e8bd62e7b5",
    "lib/gcc/arm-buildroot-linux-musleabi/14.3.0/libgcc.a": "f707f3a5a94033cbbde76f76640d56dbc5dc615ab421be4efc2641c14344f19a",
}
SOURCES = [ROOT / "framework/src/rtsp_digest.c", ROOT / "framework/tests/rtsp_digest_test.c"] + [
    VENDOR / "library" / name for name in ("sha256.c", "md5.c", "constant_time.c", "platform_util.c")
]
OWN = [APP / "build.py", APP / "README.md", APP / "rtsp_crypto_config.h",
       ROOT / "framework/include/camera/rtsp_digest.h"] + SOURCES[:2]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def now():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


def vendor_pin():
    files = sorted(p for p in VENDOR.rglob("*") if p.is_file())
    h = hashlib.sha256()
    for p in files:
        if p.is_symlink():
            raise RuntimeError("vendor symlink refused")
        h.update(p.relative_to(VENDOR).as_posix().encode() + b"\0" + bytes.fromhex(sha(p)))
    if len(files) != 277 or h.hexdigest() != VENDOR_PIN:
        raise RuntimeError("vendored Mbed TLS 3.6.7 pin mismatch")
    return {"version": "3.6.7", "files": len(files), "tree_sha256": h.hexdigest(),
            "source": str(VENDOR), "licenses": {str(p): sha(p) for p in files if "LICENSE" in p.name}}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("run", help="new simple directory name")
    parser.add_argument("--toolchain", type=Path, help="existing pinned Bootlin ARMv5 musl 14.3.0 root")
    args = parser.parse_args()
    if not args.run or not all(c.isalnum() or c in "-_" for c in args.run):
        parser.error("run must be a fresh simple name")
    out = ROOT / "local/validation/rtsp-auth" / args.run
    out.parent.mkdir(parents=True, exist_ok=True)
    if out.parent.resolve() != out.parent:
        raise RuntimeError("output parent symlink refused")
    out.mkdir()  # exist_ok=False: never overwrite prior evidence
    logs, objects, tmp = out / "logs", out / "objects", out / "tmp"
    for p in (logs, objects, tmp):
        p.mkdir()
    env = {"PATH": "/usr/bin:/bin", "LC_ALL": "C", "SOURCE_DATE_EPOCH": "1791763200", "TMPDIR": str(tmp)}
    report = {"tasks": ["SEC-05", "STREAM-07"], "started": now(), "status": "RUNNING",
              "evidence_state": "HOST ONLY", "native_executed": False, "integrated": False,
              "SEC_05_finished": False, "G3": False, "G8": False, "product_gates": False,
              "ports": "T34/Fulhan deferred", "environment": env, "commands": []}

    def run(argv, label, timeout=120):
        argv = list(map(str, argv))
        log = logs / (label + ".txt")
        record = {"argv": argv, "cwd": str(ROOT), "started": now(), "log": str(log), "timeout_seconds": timeout}
        report["commands"].append(record)
        with log.open("xb") as f:
            result = subprocess.run(argv, cwd=ROOT, env=env, stdout=f, stderr=subprocess.STDOUT, timeout=timeout)
        record.update(exit=result.returncode, ended=now())
        if result.returncode:
            raise RuntimeError("command failed; inspect " + str(log))
        return log.read_text()

    def compiler(cc, label):
        report.setdefault("compilers", {})[label] = {
            "version": run([cc, "--version"], label + "-version"),
            "target": run([cc, "-dumpmachine"], label + "-target").strip(),
        }
        for item in ("cc1", "as", "ld", "libgcc.a", "crt1.o", "Scrt1.o", "crti.o", "crtbeginS.o", "crtbeginT.o", "crtend.o", "crtn.o", "libc.a", "libc.so"):
            flag = "-print-prog-name=" if item in ("cc1", "as", "ld") else "-print-file-name="
            value = run([cc, flag + item], label + "-closure-" + item.replace(".", "_")).strip()
            p = Path(value) if "/" in value else Path(shutil.which(value, path=env["PATH"]) or value)
            if p.is_file():
                report.setdefault("compiler_runtime_closure", {})[str(p.resolve())] = sha(p)

    common = ["-std=c99", "-Wall", "-Wextra", "-Werror", "-fno-common", "-fno-strict-aliasing",
              "-fstack-usage", "-DMBEDTLS_CONFIG_FILE=\"rtsp_crypto_config.h\"", "-I" + str(APP),
              "-I" + str(ROOT / "framework/include"), "-I" + str(VENDOR / "include"),
              "-I" + str(VENDOR / "library")]

    def build(cc, label, flags, execute):
        directory = objects / label
        directory.mkdir()
        objs = []
        for i, source in enumerate(SOURCES):
            obj = directory / (str(i) + "-" + source.stem + ".o")
            run([cc, *common, *flags, "-MD", "-MF", obj.with_suffix(".d"), "-c", source, "-o", obj], label + "-compile-" + str(i))
            objs.append(obj)
        binary = out / (label + "-test")
        run([cc, *flags, *objs, "-Wl,-t", "-Wl,-Map=" + str(directory / "link.map"), "-o", binary], label + "-link")
        report.setdefault("binaries", {})[label] = {"path": str(binary), "sha256": sha(binary), "bytes": binary.stat().st_size, "executed": execute}
        if execute:
            report.setdefault("tests", {})[label] = run([binary], label + "-test", timeout=30).strip()
        else:
            readelf = str(args.toolchain / "bin/arm-buildroot-linux-musleabi-readelf")
            run([readelf, "-h", "-A", "-l", binary], label + "-elf-metadata")
            for i, obj in enumerate(objs):
                run([readelf, "-A", obj], label + "-object-attributes-" + str(i))
            report["arm_metadata_caveat"] = "New objects ARMv5TE; pinned runtime merged ELF advertises v5TEJ/Thumb-2. Link only, no native ISA/ABI execution qualification."

    try:
        report["dependency"] = vendor_pin()
        before = {str(p): sha(p) for p in OWN + SOURCES[2:]}
        report["source_sha256"] = before
        cc = "/usr/bin/cc"
        compiler(cc, "host")
        build(cc, "host", ["-O2"], True)
        # Non-PIE gives ASan deterministic shadow placement in this host harness.
        build(cc, "sanitized", ["-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"], True)
        # Header remains C++-compatible; actual API linkage is exercised.
        cpp = out / "header.cpp"
        cpp.write_text('#include <camera/rtsp_digest.h>\nint main(){camera_rtsp_digest s{};camera_rtsp_digest_destroy(&s);return s.initialized;}\n')
        compiler("/usr/bin/c++", "cxx")
        cxx_objects = [p for p in sorted((objects / "host").glob("*.o")) if not p.name.startswith("1-")]
        run(["/usr/bin/c++", "-std=c++11", "-Wall", "-Wextra", "-Werror", "-I" + str(ROOT / "framework/include"), cpp,
             *cxx_objects, "-Wl,-t", "-o", out / "cxx-header-test"], "cxx-link")
        run([out / "cxx-header-test"], "cxx-test")
        report["cxx_header_link_run"] = "PASS"
        report["cxx_fixture_sha256"] = sha(cpp)
        report["binaries"]["cxx"] = {"path": str(out / "cxx-header-test"), "sha256": sha(out / "cxx-header-test"), "executed": True}
        if args.toolchain:
            args.toolchain = args.toolchain.expanduser().resolve()
            for relative, expected in ARM_PINS.items():
                if sha(args.toolchain / relative) != expected:
                    raise RuntimeError("pinned ARM toolchain mismatch: " + relative)
            report["arm_toolchain_pins"] = ARM_PINS
            armcc = str(args.toolchain / "bin/arm-buildroot-linux-musleabi-gcc")
            compiler(armcc, "arm")
            build(armcc, "arm", ["-Os", "-march=armv5te", "-marm", "-mfloat-abi=soft", "-static"], False)
            report["arm_link"] = "PASS, NOT EXECUTED"
        else:
            report["arm_link"] = "BLOCKED: explicit installed pinned toolchain path required"
        closure = {}
        for d in objects.rglob("*.d"):
            for name in shlex.split(d.read_text().replace("\\\n", "").split(":", 1)[1]):
                p = Path(name)
                if p.is_file():
                    closure[str(p.resolve())] = sha(p)
        report["source_header_closure_sha256"] = closure
        report["stack_usage"] = {str(p): p.read_text() for p in objects.rglob("*.su")}
        # Link trace identifies resolved runtime objects/archives (not secret data).
        traced = {}
        for log in logs.glob("*-link.txt"):
            for line in log.read_text().splitlines():
                p = Path(line)
                if p.is_absolute() and p.is_file():
                    traced[str(p.resolve())] = sha(p)
        report["link_closure_sha256"] = traced
        if before != {str(p): sha(p) for p in OWN + SOURCES[2:]} or report["dependency"] != vendor_pin():
            raise RuntimeError("source changed during qualification; results not adoptable")
        report["status"] = "HOST PASS; ARM LINK PASS" if args.toolchain else "HOST PASS; ARM BLOCKED"
    except BaseException as e:
        report["status"] = "FAIL"
        report["failure"] = type(e).__name__ + ": " + str(e)
        raise
    finally:
        report["ended"] = now()
        with (out / "REPORT.json").open("x") as f:
            json.dump(report, f, indent=2)
            f.write("\n")
        print("evidence=" + str(out / "REPORT.json"))


if __name__ == "__main__":
    main()
