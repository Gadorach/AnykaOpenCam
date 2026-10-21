#!/usr/bin/env python3
"""Build/inspect public SSH candidates only. Never generate keys or listen.

Run only after the pinned entry/source review. Reuse one source tree, fresh output.
"""
import argparse
import datetime
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys

HERE = Path(__file__).resolve().parent
WORKSPACE = HERE.parents[3]
COMMIT = "28216cd9af822732a1549b78621c2ea9a76a0fe5"
TC = WORKSPACE / "work/lightbulb-camera/implementation-20261008T205404Z/sources/toolchain-armv5-musl-202508/armv5-eabi--musl--stable-2025.08-1"
CONFIG = ["--enable-bundled-libtom", "--disable-zlib", "--disable-syslog",
          "--disable-pam", "--disable-plugin-deprecated", "--disable-lastlog",
          "--disable-utmp", "--disable-utmpx", "--disable-wtmp", "--disable-wtmpx"]


def stamp():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


def metadata(path):
    return {"path": str(path), "bytes": path.stat().st_size,
            "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}


def save(path, value):
    with path.open("x") as output:
        json.dump(value, output, indent=2)
        output.write("\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    source, output = args.source.resolve(), args.output.resolve()
    if source.name != f"dropbear-{COMMIT}":
        raise ValueError("Wrong source directory")
    output.mkdir(parents=True, exist_ok=False)
    home = output / "empty-home"
    home.mkdir(mode=0o700)
    temporary = output / "compiler-tmp"
    temporary.mkdir(mode=0o700)
    env = {"PATH": f"{TC / 'bin'}:/usr/bin:/bin", "HOME": str(home),
           "TMPDIR": str(temporary), "LC_ALL": "C", "TZ": "UTC", "CONFIG_SITE": "/dev/null"}
    save(output / "ENVIRONMENT.json", env)
    commands = output / "COMMANDS.jsonl"
    sequence = 0

    def run(argv, cwd, name, overrides=None, accept=(0,), stdin=None):
        nonlocal sequence
        sequence += 1
        log = output / f"{sequence:03d}-{name}.txt"
        effective = dict(env, **(overrides or {}))
        start = stamp()
        with log.open("x") as stream:
            result = subprocess.run([str(a) for a in argv], cwd=cwd, env=effective,
                                    input=stdin, text=True, stdout=stream, stderr=subprocess.STDOUT,
                                    timeout=240)
        with commands.open("a") as record:
            record.write(json.dumps({"argv": [str(a) for a in argv], "cwd": str(cwd),
                                     "env": effective, "stdin": stdin, "start_utc": start,
                                     "finish_utc": stamp(), "exit_code": result.returncode,
                                     "log": str(log)}) + "\n")
        if result.returncode not in accept:
            raise RuntimeError(f"HOLD: {name} exit{result.returncode}; preserved {log}")
        return log.read_text()

    gcc = TC / "bin/arm-buildroot-linux-musleabi-gcc"
    inputs = [Path(sys.executable), HERE / "build.py", HERE / "localoptions.h",
              gcc.resolve(), TC / "bin/arm-buildroot-linux-musleabi-gcc.br_real",
              TC / "README.txt"]
    for command in ["cc", "make", "ar", "ranlib", "readelf", "objdump", "nm", "size"]:
        resolved = shutil.which(command, path=env["PATH"])
        if not resolved:
            raise RuntimeError(f"HOLD: missing host build tool {command}")
        inputs.append(Path(resolved).resolve())
    run([gcc, "--version"], output, "arm-gcc-version")
    run([gcc, "-v"], output, "arm-gcc-config")
    run(["cc", "--version"], output, "host-gcc-version")
    run(["make", "--version"], output, "make-version")
    for library in ["libc.a", "libgcc.a", "crt1.o", "crti.o", "crtn.o"]:
        path = Path(run([gcc, f"-print-file-name={library}"], output, f"locate-{library}").strip())
        if not path.is_file():
            raise RuntimeError(f"HOLD: missing {library}")
        inputs.append(path.resolve())
    save(output / "TOOL_INPUTS.json", [metadata(p) for p in sorted(set(inputs))])
    profiles = []
    for kind, program in [("arm", "dropbear"), ("host-keytool", "dropbearkey"),
                          ("host-options", "dropbear")]:
        directory = output / kind
        directory.mkdir()
        shutil.copyfile(HERE / "localoptions.h", directory / "localoptions.h")
        arm = kind == "arm"
        cc = str(gcc) if arm else "/usr/bin/cc"
        flags = "-Os -fstack-usage -MD"
        if arm:
            flags += " -march=armv5te -marm -mfloat-abi=soft"
        overrides = {"CC": cc, "CFLAGS": flags, "LTM_CFLAGS": flags,
                     "LDFLAGS": "-Wl,-Map=link.map,--cref,-t" + (" -static" if arm else "")}
        configure = [source / "configure", *CONFIG]
        if arm:
            configure += ["--enable-static", "--build=x86_64-pc-linux-gnu",
                          "--host=arm-buildroot-linux-musleabi"]
        run(configure, directory, f"{kind}-configure", overrides)
        run(["make", "-j2", f"PROGRAMS={program}"], directory, f"{kind}-make", overrides)
        binary = directory / program
        prefix = str(TC / "bin/arm-buildroot-linux-musleabi-") if arm else ""
        elf = run([prefix + "readelf", "-hAlWd", binary], directory, f"{kind}-readelf")
        run([prefix + "nm", "-an", binary], directory, f"{kind}-nm")
        undefined = run([prefix + "nm", "-u", binary], directory, f"{kind}-undefined")
        size = run([prefix + "size", binary], directory, f"{kind}-size")
        run([prefix + "objdump", "-dr", binary], directory, f"{kind}-disassembly")
        feature_args = [cc, "-dM", "-E", "-x", "c", "-DLOCALOPTIONS_H_EXISTS", "-I.",
                        f"-I{source / 'src'}", "-include", "options.h"]
        if program == "dropbear":
            feature_args += ["-DDROPBEAR_SERVER"]
        feature_args += ["-"]
        features = run(feature_args, directory, f"{kind}-features", stdin="")
        if arm:
            assert "INTERP" not in elf and "(NEEDED)" not in elf
            assert "Machine:                           ARM" in elf
            assert not undefined.strip(), "ARM unresolved symbols"
            assert "#define HAVE_GETRANDOM 1" in features
        # Every contract macro must survive the actual preprocessing unchanged.
        for line in (HERE / "localoptions.h").read_text().splitlines():
            if line.startswith("#define "):
                assert line in features, (kind, line)
        assert "#define DISABLE_WTMP 1" in features
        assert "#define DISABLE_WTMPX 1" in features
        assert "#define DROPBEAR_PLUGIN 0" in features
        assert "#define DISABLE_ZLIB 1" in features
        for name in ["Makefile", "config.h", "config.log", "config.status", "default_options_guard.h",
                     "libtomcrypt/libtomcrypt.a", "libtommath/libtommath.a", "link.map"]:
            assert (directory / name).is_file(), name
        dependencies = set()
        for depfile in directory.rglob("*.d"):
            text = depfile.read_text().replace("\\\n", " ")
            # Build/source paths in this workspace contain no whitespace.
            for token in text.partition(":")[2].split():
                p = Path(token)
                if not p.is_absolute():
                    candidates = [directory / token, directory / "libtomcrypt" / token,
                                  directory / "libtommath" / token, depfile.parent / token]
                    p = next((candidate for candidate in candidates if candidate.is_file()), candidates[0])
                if p.is_file():
                    dependencies.add(p.resolve())
        save(directory / "HEADER_SOURCE_CLOSURE.json", [metadata(p) for p in sorted(dependencies)])
        stack = []
        for stackfile in directory.rglob("*.su"):
            for line in stackfile.read_text().splitlines():
                parts = line.split("\t")
                if len(parts) == 3 and parts[1].isdigit():
                    stack.append({"location": parts[0], "bytes": int(parts[1]), "kind": parts[2]})
        stack.sort(key=lambda entry: entry["bytes"], reverse=True)
        save(directory / "STACK_USAGE.json", stack)
        profile = {"kind": kind, "program": program, "binary": metadata(binary),
                   "size_output": size, "max_recorded_frame": stack[0] if stack else None,
                   "runtime": "NOT_RUN" if kind != "host-options" else "INERT_QUERIES_ONLY"}
        profiles.append(profile)
        if kind == "arm":
            stripped = directory / "dropbear-armv5-static"
            run([prefix + "strip", "-o", stripped, binary], directory, "arm-strip-copy")
            profile["stripped_binary"] = metadata(stripped)
        if kind == "host-options":
            help_text = run([binary, "-h"], directory, "host-help")
            assert "2026.94" in help_text and "-D" in help_text
            assert "-R" not in help_text and "-B" not in help_text
            run([binary, "-D", "/NONEXISTENT_RAM/keys", "-P", "/NONEXISTENT_RAM/pid",
                 "-r", "/NONEXISTENT_RAM/hostkey", "-p", "127.0.0.1:22222", "-s",
                 "-I", "60", "-M", "300", "-T", "3", "-h"], directory, "host-ram-options-help")
            # Upstream parses legacy -R even when DELAY_HOSTKEY is compiled out.
            # Never use it in the admitted launch; document rather than patch it.
            run([binary, "-R", "-h"], directory, "host-legacy-R-parsed-not-admitted")
            for flag in ["-B", "-t", "-A", "-i", "-E"]:
                text = run([binary, flag, "-h"], directory, "host-reject-" + flag[1:], accept=(1,))
                assert "Invalid option" in text
            for category in ["kex", "sig", "cipher", "mac", "compress"]:
                run([binary, "-Q", category], directory, "host-algos-" + category)
    save(output / "BUILD.json", {"utc": stamp(), "result": "BUILD_AND_STATIC_INSPECTION_PASS",
                                "profiles": profiles, "key_generation": "NONE", "listeners": "NONE",
                                "target_operations": "NONE", "native_qualification": "NOT_RUN",
                                "ssh_ready": False})
    print(json.dumps({"result": "BUILD_AND_STATIC_INSPECTION_PASS", "output": str(output),
                      "artifacts": [p["binary"] for p in profiles], "ssh_ready": False}, indent=2))


if __name__ == "__main__":
    main()
