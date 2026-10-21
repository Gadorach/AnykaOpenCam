#!/usr/bin/env python3
"""Additional static closure/ABI/RNG evidence; never execute a built program."""
import argparse
import datetime
import hashlib
import json
from pathlib import Path
import re
import subprocess

HERE = Path(__file__).resolve().parent
WORKSPACE = HERE.parents[3]
TC = WORKSPACE / "work/lightbulb-camera/implementation-20261008T205404Z/sources/toolchain-armv5-musl-202508/armv5-eabi--musl--stable-2025.08-1"
PREFIX = str(TC / "bin/arm-buildroot-linux-musleabi-")


def digest(path):
    return {"path": str(path), "bytes": path.stat().st_size,
            "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, required=True)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--acquisition", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    build, source, output = args.build.resolve(), args.source.resolve(), args.output.resolve()
    json.loads((build / "BUILD.json").read_text())
    output.mkdir(parents=True, exist_ok=False)
    env = json.loads((build / "ENVIRONMENT.json").read_text())
    records = []

    def run(argv, name, cwd=build, stdin=None):
        start = datetime.datetime.now(datetime.timezone.utc).isoformat()
        result = subprocess.run([str(a) for a in argv], cwd=cwd, env=env, input=stdin,
                                text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=60)
        with (output / name).open("x") as stream:
            stream.write(result.stdout)
        records.append({"argv": [str(a) for a in argv], "cwd": str(cwd), "env": env,
                        "stdin": stdin, "start_utc": start, "exit": result.returncode, "log": name})
        assert result.returncode == 0, name
        return result.stdout

    tools = []
    for name in ["cc1", "collect2", "as", "ld"]:
        path = Path(run([PREFIX + "gcc", "-print-prog-name=" + name], "locate-" + name + ".txt").strip()).resolve()
        assert path.is_file(), name
        tools.append(digest(path))
        if name in ("as", "ld"):
            run([path, "--version"], name + "-version.txt")
    closure = []
    for kind in ("arm", "host-keytool", "host-options"):
        paths = set()
        for line in (build / kind / "link.map").read_text().splitlines():
            if line.startswith("LOAD "):
                path = Path(line[5:])
                if not path.is_absolute():
                    path = build / kind / path
                if path.is_file():
                    paths.add(path.resolve())
        closure.append({"kind": kind, "load_inputs": [digest(p) for p in sorted(paths)]})
    for name in ["main", "main_noinetd.constprop.0", "readhostkey", "addrandom", "seedrandom",
                 "load_all_hostkeys", "getrandom", "writev", "__restore", "__restore_rt",
                 "__libc_sigaction", "__select_time64", "__clock_gettime", "fork", "socket",
                 "setresgid", "setuid", "__setxid", "getusershell", "__getusershell"]:
        run([PREFIX + "objdump", "-d", "--disassemble=" + name, build / "arm/dropbear"],
            "arm-" + name + ".txt")
    for unit in ["svr-main", "common-runopts", "dbrandom", "svr-runopts", "svr-kex"]:
        run([PREFIX + "objdump", "-dr", build / "arm/obj" / (unit + ".o")], "object-" + unit + ".txt")
    gcc = PREFIX + "gcc"
    for unit in ["svr-main", "common-runopts", "dbrandom", "svr-kex", "svr-runopts"]:
        run([gcc, "-E", "-P", "-DDROPBEAR_SERVER", "-DLOCALOPTIONS_H_EXISTS", "-I.",
             "-I" + str(source / "src"), "-I" + str(source / "libtomcrypt/src/headers"),
             source / "src" / (unit + ".c")], "preprocessed-" + unit + ".c", cwd=build / "arm")
    libc = Path(subprocess.check_output([gcc, "-print-file-name=libc.a"], env=env, text=True).strip()).resolve()
    attributes = run([PREFIX + "readelf", "-A", libc], "libc-attributes.txt")
    thumb2 = []
    for part in re.split(r"(?=^File: )", attributes, flags=re.M):
        if "Tag_THUMB_ISA_use: Thumb-2" in part:
            thumb2.append(part.splitlines()[0])
    match = json.loads((args.acquisition / "SOURCE_FILES.json").read_text())
    for item in match["files"]:
        path = source / item["file"]
        assert path.stat().st_size == item["bytes"] and digest(path)["sha256"] == item["sha256"], path
    memory = run([PREFIX + "readelf", "-lW", build / "arm/dropbear"], "arm-load-segments.txt")
    segments = []
    for line in memory.splitlines():
        fields = line.split()
        if fields and fields[0] == "LOAD":
            segments.append({"vaddr": int(fields[2], 16), "file_bytes": int(fields[4], 16),
                             "memory_bytes": int(fields[5], 16)})
    result = {"utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
              "tools": tools, "link_closure": closure, "source_files_unchanged": len(match["files"]),
              "load_segments": segments, "load_segment_memory_sum": sum(s["memory_bytes"] for s in segments),
              "libc_thumb2_attribute_members": thumb2,
              "scope": "static inspection, no built program executed; NOT native ABI/RNG/RSS PASS"}
    with (output / "INSPECTION.json").open("x") as stream:
        json.dump(result, stream, indent=2)
        stream.write("\n")
    with (output / "COMMANDS.json").open("x") as stream:
        json.dump(records, stream, indent=2)
        stream.write("\n")
    print(json.dumps({"source_files_unchanged": result["source_files_unchanged"],
                      "load_segment_memory_sum": result["load_segment_memory_sum"],
                      "libc_thumb2_attribute_members": thumb2}, indent=2))


if __name__ == "__main__":
    main()
