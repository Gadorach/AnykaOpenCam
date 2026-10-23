# SPDX-License-Identifier: GPL-2.0-only
"""Fixed compile/inspection only. Never runs the HOST harness or target code."""
from pathlib import Path
import datetime
import hashlib
import json
import shlex
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[3]
SRC = ROOT / "apps/rtsp-auth/rtsp-os-random"
DOC = ROOT / "docs/qualification-plans/rtsp-os-random-20261023"
OUT = DOC / "runs/build01"
ARM = ROOT / ("work/lightbulb-camera/implementation-20261008T205404Z/sources/"
              "toolchain-armv5-musl-202508/armv5-eabi--musl--stable-2025.08-1/"
              "bin/arm-buildroot-linux-musleabi-gcc")
TLS = ROOT / "apps/auth-web/service/vendor/mbedtls"
ENV = {"PATH": "/usr/bin:/bin", "LC_ALL": "C"}


def pin(path):
    path = Path(path).absolute()
    return {"path": str(path), "resolved": str(path.resolve()),
            "bytes": path.stat().st_size,
            "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}


def write(path, data):
    with path.open("x") as stream:
        stream.write(data)


def dump(path, data):
    write(path, json.dumps(data, indent=2) + "\n")


def main():
    OUT.mkdir(parents=True, exist_ok=False)
    (OUT / "objects").mkdir()
    (OUT / "logs").mkdir()
    commands, tools = [], {}
    selected = sorted(SRC.glob("*"))
    selected = [p for p in selected if p.is_file()] + [
        DOC / "CONTRACT.md",
        ROOT / "apps/rtsp-auth/rtsp_crypto_config.h",
        ROOT / "apps/rtsp-auth/qc07f-owned-runtime/server/candidate_auth.c",
        ROOT / "apps/rtsp-auth/qc07f-owned-runtime/server/candidate_auth.h",
        ROOT / "docs/qualification-plans/rtsp-owned-runtime-20261023/PARENT_DISPOSITION.md",
        ROOT / "docs/qualification-plans/qc07f-bootstrap-20261023/CONTRACT.md",
        ROOT / "docs/plan/camera-foundation/09-SECURITY-AND-CONFIGURATION.md",
        TLS / "library/platform_util.c", TLS / "LICENSE",
        Path("/usr/share/man/man2/getrandom.2.gz"),
    ]
    inputs = [pin(p) for p in selected]
    dump(OUT / "INPUTS.json", inputs)

    def run(name, argv):
        argv = [str(a) for a in argv]
        tool = Path(shutil.which(argv[0], path=ENV["PATH"]) or argv[0])
        tools[str(tool)] = pin(tool)
        result = subprocess.run(argv, cwd=ROOT, env=ENV, capture_output=True)
        log = OUT / "logs" / (name + ".txt")
        write(log, result.stdout.decode(errors="replace") +
              result.stderr.decode(errors="replace"))
        commands.append({"name": name, "argv": argv, "cwd": str(ROOT),
                         "environment": ENV, "returncode": result.returncode,
                         "log": pin(log)})
        dump(OUT / ("command-%02d.json" % len(commands)), commands[-1])
        if result.returncode:
            raise SystemExit("STOP: " + name + " failed; preserved, no retry")
        return result.stdout.decode().strip()

    flags = ["-std=gnu11", "-Wall", "-Wextra", "-Werror", "-fno-common",
             "-fno-strict-aliasing", "-ffunction-sections", "-fdata-sections",
             "-fstack-protector-strong", "-D_FORTIFY_SOURCE=2", "-fstack-usage",
             '-DMBEDTLS_CONFIG_FILE="rtsp_crypto_config.h"',
             "-I" + str(ROOT / "framework/include"),
             "-I" + str(ROOT / "apps/rtsp-auth"),
             "-I" + str(ROOT / "apps/rtsp-auth/isolated-verifier"),
             "-I" + str(TLS / "include"), "-I" + str(TLS / "library")]
    for role, cc, arch in [("host", Path("/usr/bin/cc"), ["-O2"]),
                           ("arm", ARM, ["-Os", "-march=armv5te", "-marm",
                                        "-mfloat-abi=soft"])]:
        run(role + "-version", [cc, "--version"])
        run(role + "-machine", [cc, "-dumpmachine"])
        for prog in ("cc1", "as", "ld", "collect2"):
            value = run(role + "-tool-" + prog, [cc, "-print-prog-name=" + prog])
            path = Path(shutil.which(value, path=ENV["PATH"]) or value)
            tools[str(path)] = pin(path)
        objects = []
        for unit, source in [("random", SRC / "os_random.c"),
                             ("zeroize", TLS / "library/platform_util.c")]:
            obj = OUT / "objects" / (role + "-" + unit + ".o")
            run(role + "-cc-" + unit,
                [cc] + flags + arch + ["-MD", "-MF", obj.with_suffix(".d"),
                                      "-c", source, "-o", obj])
            objects.append(obj)
        rel = OUT / (role + "-random-NOT-EXECUTABLE.o")
        run(role + "-REL", [cc] + arch + ["-nostdlib", "-r"] + objects + ["-o", rel])
        run(role + "-ELF", ["/usr/bin/readelf", "-h", "-A", rel])
        run(role + "-symbols", ["/usr/bin/nm", "-g", rel])
    harness = OUT / "objects/host-one-call.o"
    run("host-cc-one-call", ["/usr/bin/cc"] + flags + ["-O2",
        "-DRTSP_OS_RANDOM_HOST_ONE_CALL", "-MD", "-MF", harness.with_suffix(".d"),
        "-c", SRC / "host_one_call.c", "-o", harness])
    elf = OUT / "HOST-OS-RANDOM-ONE-CALL-NOT-NATIVE"
    run("host-link-one-call", ["/usr/bin/cc", "-Wl,--gc-sections",
        "-Wl,-z,relro,-z,now", "-Wl,-Map=" + str(OUT / "host-link.map"),
        "-Wl,-t", harness, OUT / "objects/host-random.o",
        OUT / "objects/host-zeroize.o", "-o", elf])
    run("host-one-call-ELF", ["/usr/bin/readelf", "-h", "-A", elf])
    run("host-one-call-symbols", ["/usr/bin/nm", "-g", elf])
    # Actual linked inputs named by the host linker trace; no program execution.
    for line in (OUT / "logs/host-link-one-call.txt").read_text().splitlines():
        p = Path(line)
        if p.is_absolute() and p.is_file():
            tools[str(p)] = pin(p)
    dependencies = []
    for dep in sorted((OUT / "objects").glob("*.d")):
        words = shlex.split(dep.read_text().replace("\\\n", " ").split(":", 1)[1])
        dependencies.append({"unit": dep.name,
                             "files": [pin(ROOT / word) for word in words]})
    dump(OUT / "DEPENDENCIES.json", dependencies)
    if inputs != [pin(p) for p in selected]:
        raise SystemExit("STOP: selected input drift; no acceptance")
    dump(OUT / "TOOLS.json", list(tools.values()))
    write(OUT / "COMMANDS.txt", "\n".join(shlex.join(c["argv"]) for c in commands) + "\n")
    artifacts = [pin(p) for p in sorted(OUT.rglob("*")) if p.is_file()]
    dump(OUT / "REPORT.json", {"status": "COMPILE_INSPECT_PASS_NOT_RUN",
        "utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "C_compilations": 5, "commands": commands, "artifacts": artifacts,
        "inputs_unchanged": True, "functional_calls": 0,
        "native_executed": False})
    print("COMPILE_INSPECT_PASS: 5 C compiles; HOST harness NOT RUN; ARM NOT RUN")


if __name__ == "__main__":
    main()
