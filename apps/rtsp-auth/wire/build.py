#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Read-only pinned crypto/request/policy linkage; fresh HOST wire qualification."""
from pathlib import Path
import argparse
import datetime
import hashlib
import importlib.util
import json
import shlex
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[3]
APP = Path(__file__).resolve().parent
CONFIG = APP.parent
VENDOR = ROOT / "apps/auth-web/service/vendor/mbedtls"
spec = importlib.util.spec_from_file_location("digest_build_read_only", CONFIG / "build.py")
BASE = importlib.util.module_from_spec(spec)
spec.loader.exec_module(BASE)  # helpers only, no main/install/write on import; python -B
SOURCES = [ROOT / name for name in (
    "framework/src/rtsp_digest.c", "framework/tests/rtsp_digest_wire_test.c",
    "framework/src/rtsp_digest_wire.c", "framework/src/rtsp_request.c",
    "framework/src/rtsp_input.c", "framework/src/access_policy.c")] + [
    VENDOR / "library" / name for name in ("sha256.c", "md5.c", "constant_time.c", "platform_util.c")]
OWN = [APP / "build.py", APP / "README.md", ROOT / "framework/include/camera/rtsp_digest_wire.h"]
INPUTS = OWN + SOURCES + [CONFIG / "build.py", CONFIG / "rtsp_crypto_config.h", ROOT / "framework/include/camera/rtsp_digest.h",
    ROOT / "framework/include/camera/rtsp_request.h", ROOT / "framework/include/camera/rtsp_input.h",
    ROOT / "framework/include/camera/access_policy.h", ROOT / "framework/include/camera/backend.h",
    ROOT / "framework/src/rtsp_server.c", ROOT / "framework/include/camera/rtsp_server.h"]  # proposal-only pins, never linked/enabled


def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()


def now():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("run")
    p.add_argument("--toolchain", type=Path)
    a = p.parse_args()
    if not a.run or not all(c.isalnum() or c in "-_" for c in a.run):
        p.error("fresh simple run name required")
    out = ROOT / "local/validation/rtsp-wire" / a.run
    out.parent.mkdir(parents=True, exist_ok=True)
    if out.parent.resolve() != out.parent:
        raise RuntimeError("output symlink refused")
    out.mkdir()  # refuse ALL existing runs
    for name in ("logs", "objects", "inputs", "tmp"):
        (out / name).mkdir()
    env = {"PATH": "/usr/bin:/bin", "LC_ALL": "C", "SOURCE_DATE_EPOCH": "1791763200", "TMPDIR": str(out / "tmp")}
    report = {"tasks": ["SEC-05", "STREAM-04", "STREAM-07"], "started": now(), "status": "RUNNING",
        "evidence_state": "HOST ONLY", "native_executed": False, "server_enabled": False,
        "SEC_05_complete": False, "G3": False, "G8": False, "product_gates": False, "ports": "T34/Fulhan deferred",
        "environment": env, "commands": [], "snapshots": {}}

    def run(argv, label, timeout=120):
        argv = list(map(str, argv))
        log = out / "logs" / (label + ".txt")
        record = {"argv": argv, "cwd": str(ROOT), "started": now(), "log": str(log), "timeout_seconds": timeout}
        report["commands"].append(record)
        with log.open("xb") as f:
            result = subprocess.run(argv, cwd=ROOT, env=env, stdout=f, stderr=subprocess.STDOUT, timeout=timeout)
        record.update(exit=result.returncode, ended=now())
        if result.returncode:
            raise RuntimeError("command failed; inspect " + str(log))
        return log.read_text()

    def compiler(cc, label):
        report.setdefault("compilers", {})[label] = {"version": run([cc, "--version"], label + "-version"),
            "target": run([cc, "-dumpmachine"], label + "-target").strip()}
        for item in ("cc1", "as", "ld", "libgcc.a", "crt1.o", "Scrt1.o", "crti.o", "crtbeginS.o", "crtbeginT.o", "crtend.o", "crtn.o", "libc.a", "libc.so"):
            flag = "-print-prog-name=" if item in ("cc1", "as", "ld") else "-print-file-name="
            value = run([cc, flag + item], label + "-runtime-" + item.replace(".", "_")).strip()
            path = Path(value) if "/" in value else Path(shutil.which(value, path=env["PATH"]) or value)
            if path.is_file():
                report.setdefault("compiler_runtime_closure_sha256", {})[str(path.resolve())] = sha(path)

    common = ["-std=c99", "-Wall", "-Wextra", "-Werror", "-fno-common", "-fno-strict-aliasing", "-fstack-usage",
        '-DMBEDTLS_CONFIG_FILE="rtsp_crypto_config.h"', "-I" + str(CONFIG), "-I" + str(ROOT / "framework/include"),
        "-I" + str(VENDOR / "include"), "-I" + str(VENDOR / "library")]

    def build(cc, label, flags, execute):
        directory = out / "objects" / label
        directory.mkdir()
        objs = []
        for i, source in enumerate(SOURCES):
            obj = directory / (str(i) + "-" + source.stem + ".o")
            # Existing read-only RTSP input uses compact legacy statement style.
            # This exception is ONLY for that frozen dependency, not new code.
            legacy = ["-Wno-misleading-indentation", "-Wno-error=nonnull"] if source.name == "rtsp_input.c" else []
            run([cc, *common, *flags, *legacy, "-MD", "-MF", obj.with_suffix(".d"), "-c", source, "-o", obj], label + "-compile-" + str(i))
            objs.append(obj)
        binary = out / (label + "-test")
        run([cc, *flags, *objs, "-Wl,-t", "-Wl,-Map=" + str(directory / "link.map"), "-o", binary], label + "-link")
        report.setdefault("binaries", {})[label] = {"path": str(binary), "sha256": sha(binary), "bytes": binary.stat().st_size, "executed": execute}
        if execute:
            report.setdefault("tests", {})[label] = run([binary], label + "-test", 30).strip()
        else:
            readelf = a.toolchain / "bin/arm-buildroot-linux-musleabi-readelf"
            run([readelf, "-h", "-A", "-l", binary], label + "-metadata")
            run([readelf, "-A", directory / "2-rtsp_digest_wire.o"], label + "-wire-object-attributes")

    try:
        report["dependency"] = BASE.vendor_pin()
        before = {str(src): sha(src) for src in INPUTS}
        report["source_sha256"] = before
        for src in INPUTS:
            dst = out / "inputs" / src.relative_to(ROOT)
            dst.parent.mkdir(parents=True, exist_ok=True)
            with dst.open("xb") as f:
                f.write(src.read_bytes())
            dst.chmod(0o444)  # new immutable source-version copy, never original
            report["snapshots"][str(dst)] = sha(dst)
        compiler("/usr/bin/cc", "host")
        build("/usr/bin/cc", "host", ["-O2"], True)
        build("/usr/bin/cc", "sanitized", ["-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"], True)
        cpp = out / "header.cpp"
        cpp.write_text('#include <camera/rtsp_digest_wire.h>\nint main(){camera_digest_wire_limit b{};camera_digest_wire_adapter a{};camera_digest_wire_adapter_clear(&a);return camera_digest_wire_limit_init(&b,0,1000,1,1);}\n')
        compiler("/usr/bin/c++", "cxx")
        objs = [o for o in sorted((out / "objects/host").glob("*.o")) if not o.name.startswith("1-")]
        run(["/usr/bin/c++", "-std=c++11", "-Wall", "-Wextra", "-Werror", "-I" + str(ROOT / "framework/include"), cpp, *objs, "-Wl,-t", "-o", out / "cxx-test"], "cxx-link")
        run([out / "cxx-test"], "cxx-test")
        report["cxx_header_link_run"] = "PASS"
        report["cxx_fixture_sha256"] = sha(cpp)
        report["binaries"]["cxx"] = {"path": str(out / "cxx-test"), "sha256": sha(out / "cxx-test"), "bytes": (out / "cxx-test").stat().st_size, "executed": True}
        if a.toolchain:
            a.toolchain = a.toolchain.expanduser().resolve()
            for relative, expected in BASE.ARM_PINS.items():
                if sha(a.toolchain / relative) != expected:
                    raise RuntimeError("pinned ARM mismatch: " + relative)
            report["arm_toolchain_pins"] = BASE.ARM_PINS
            cc = a.toolchain / "bin/arm-buildroot-linux-musleabi-gcc"
            compiler(cc, "arm")
            build(cc, "arm", ["-Os", "-march=armv5te", "-marm", "-mfloat-abi=soft", "-static"], False)
            report["arm_link"] = "PASS, NOT EXECUTED"
            report["arm_caveat"] = "Read metadata: pinned runtime merged Thumb-2 attribute is NOT native ISA/ABI proof."
        else:
            report["arm_link"] = "BLOCKED: explicit existing pinned toolchain path required"
        closure = {}
        for dep in (out / "objects").rglob("*.d"):
            for name in shlex.split(dep.read_text().replace("\\\n", "").split(":", 1)[1]):
                path = Path(name)
                if path.is_file():
                    closure[str(path.resolve())] = sha(path)
        report["source_header_closure_sha256"] = closure
        report["stack_usage"] = {str(path): path.read_text() for path in (out / "objects").rglob("*.su")}
        links = {}
        for log in (out / "logs").glob("*-link.txt"):
            for line in log.read_text().splitlines():
                path = Path(line)
                if path.is_absolute() and path.is_file():
                    links[str(path.resolve())] = sha(path)
        report["link_closure_sha256"] = links
        if before != {str(src): sha(src) for src in INPUTS} or report["dependency"] != BASE.vendor_pin():
            raise RuntimeError("source/dependency changed during qualification")
        report["status"] = "HOST PASS; ARM LINK PASS" if a.toolchain else "HOST PASS; ARM BLOCKED"
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
