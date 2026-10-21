#!/usr/bin/env python3
"""Package allowlisted PUBLIC binaries/notices and seal this one host-only unit."""
import datetime
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct

HERE = Path(__file__).resolve().parent
WORKSPACE = HERE.parents[3]
EVIDENCE = WORKSPACE / "evidence/delegated/20261013-stock-reference/ssh-control02"
DOCS = WORKSPACE / "docs/qualification-plans/stock-reference-20261013/ssh-control02"
SOURCE = WORKSPACE / "work/lightbulb-camera/ssh-control02-source01/dropbear-28216cd9af822732a1549b78621c2ea9a76a0fe5"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def save(path, value):
    with path.open("x") as stream:
        json.dump(value, stream, indent=2)
        stream.write("\n")


def alloc_sections(path):
    data = path.read_bytes()
    assert data[:6] == b"\x7fELF\x01\x01"
    table = struct.unpack_from("<I", data, 32)[0]
    entry_size, count, names = struct.unpack_from("<HHH", data, 46)
    entries = [struct.unpack_from("<10I", data, table + entry_size * i) for i in range(count)]
    text = entries[names]
    strings = data[text[4]:text[4] + text[5]]
    result = {}
    for name, kind, flags, address, offset, size, *_ in entries:
        if flags & 2:
            label = strings[name:strings.index(b"\0", name)].decode()
            result[label] = (kind, address, size, None if kind == 8 else data[offset:offset + size])
    return result


def main():
    build = EVIDENCE / "build03"
    result = json.loads((build / "BUILD.json").read_text())
    assert result["result"] == "BUILD_AND_STATIC_INSPECTION_PASS"
    assert result["ssh_ready"] is False and result["key_generation"] == "NONE"
    for profile in result["profiles"]:
        for key in ("binary", "stripped_binary"):
            if key in profile:
                entry = profile[key]
                p = Path(entry["path"])
                assert p.stat().st_size == entry["bytes"] and sha(p) == entry["sha256"]
    assert alloc_sections(build / "arm/dropbear") == alloc_sections(build / "arm/dropbear-armv5-static")
    source_files = json.loads((EVIDENCE / "acquisition01/SOURCE_FILES.json").read_text())["files"]
    for entry in source_files:
        assert sha(SOURCE / entry["file"]) == entry["sha256"]
    release = EVIDENCE / "release01"
    release.mkdir(exist_ok=False)
    copies = {
        "dropbear-armv5-static": build / "arm/dropbear-armv5-static",
        "dropbear-armv5-unstripped": build / "arm/dropbear",
        "dropbearkey-host": build / "host-keytool/dropbearkey",
        "licenses/Dropbear-LICENSE": SOURCE / "LICENSE",
        "licenses/LibTomCrypt-LICENSE": SOURCE / "libtomcrypt/LICENSE",
        "licenses/LibTomMath-LICENSE": SOURCE / "libtommath/LICENSE",
        "licenses/musl-1.2.5-COPYRIGHT": EVIDENCE / "licenses01/musl-1.2.5-COPYRIGHT",
        "licenses/musl-arm-memcpy.S": EVIDENCE / "licenses03/musl-arm-memcpy.S",
        "licenses/gcc14-COPYING.RUNTIME": EVIDENCE / "licenses03/gcc14-COPYING.RUNTIME",
        "licenses/gcc14-COPYING3": EVIDENCE / "licenses03/gcc14-COPYING3",
    }
    packaged = []
    for name, origin in copies.items():
        target = release / name
        target.parent.mkdir(parents=True, exist_ok=True)
        with target.open("xb") as stream:
            stream.write(origin.read_bytes())
        if name.startswith("dropbear"):
            target.chmod(0o755)
        packaged.append({"file": name, "origin": str(origin), "bytes": target.stat().st_size, "sha256": sha(target)})
    # No hostname, environment dump, keys, account contents, or target reads.
    host = {"glibc": os.confstr("CS_GNU_LIBC_VERSION"), "libraries": []}
    for name in ("/usr/lib/libc.so.6", "/lib64/ld-linux-x86-64.so.2"):
        path = Path(name).resolve()
        assert path.is_file()
        host["libraries"].append({"path": str(path), "bytes": path.stat().st_size, "sha256": sha(path)})
    save(release / "MANIFEST.json", {"utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
                                  "files": packaged, "host_runtime": host, "ssh_ready": False,
                                  "keys": "NONE", "listeners": "NONE", "native": "NOT_RUN",
                                  "alloc_sections_identical_after_strip": True})
    for folder, paths in [(EVIDENCE / "recipe-snapshot", sorted(HERE.glob("*.py")) +
                           sorted(HERE.glob("*.h")) + [HERE / "README.md"]),
                          (EVIDENCE / "docs-snapshot", sorted(DOCS.glob("*.md")))]:
        folder.mkdir(exist_ok=False)
        for path in paths:
            shutil.copyfile(path, folder / path.name)
    json_count = 0
    for path in EVIDENCE.rglob("*.json"):
        json.loads(path.read_text())
        json_count += 1
    for path in EVIDENCE.rglob("*.jsonl"):
        for line in path.read_text().splitlines():
            json.loads(line)
    for path in HERE.glob("*.py"):
        compile(path.read_text(), str(path), "exec")
    save(EVIDENCE / "FINAL_VALIDATION.json", {
        "utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "result": "SOURCE_BUILD_STATIC_AND_INERT_PARSER_PASS",
        "public_source_files_unchanged": len(source_files), "prior_reviewed_hashes_match": 23,
        "json_documents_parsed_before_this_receipt": json_count,
        "script_compile": "PASS", "alloc_sections_identical_after_strip": True,
        "key_generation": "NONE", "keytool_execution": "NONE", "listeners": "NONE",
        "target_operations": "NONE", "native_abi_rng_rss": "NOT_QUALIFIED", "ssh_ready": False})
    paths = sorted([p for p in EVIDENCE.rglob("*") if p.is_file()] +
                   list(HERE.glob("*.py")) + list(HERE.glob("*.h")) + [HERE / "README.md"] +
                   list(DOCS.glob("*.md")))
    with (EVIDENCE / "SHA256SUMS").open("x") as stream:
        for path in paths:
            stream.write(f"{sha(path)}  {path.relative_to(WORKSPACE)}\n")
    print(json.dumps({"packaged_files": len(packaged), "sealed_files": len(paths),
                      "release": str(release), "ssh_ready": False}, indent=2))


if __name__ == "__main__":
    main()
