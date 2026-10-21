#!/usr/bin/env python3
"""Acquire ONE pinned public archive; audit all members before extracting.

No target access, key inputs, service start, or build execution. Refuses reuse.
"""
import argparse
import datetime
import hashlib
import json
from pathlib import Path, PurePosixPath
import tarfile
import urllib.request

COMMIT = "28216cd9af822732a1549b78621c2ea9a76a0fe5"
URL = f"https://codeload.github.com/mkj/dropbear/tar.gz/{COMMIT}"
WORKSPACE = Path(__file__).resolve().parents[4]


def utc():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


def save(path, data):
    with path.open("x") as output:
        json.dump(data, output, indent=2)
        output.write("\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--evidence", type=Path, required=True)
    parser.add_argument("--work", type=Path, required=True)
    args = parser.parse_args()
    evidence, work = args.evidence.resolve(), args.work.resolve()
    evidence.mkdir(parents=True, exist_ok=False)
    if work.exists():
        raise FileExistsError(work)
    archive = evidence / f"dropbear-{COMMIT}.tar.gz"
    start = utc()
    digest, count = hashlib.sha256(), 0
    with urllib.request.urlopen(URL, timeout=60) as response, archive.open("xb") as output:
        while chunk := response.read(65536):
            count += len(chunk)
            if count > 16 * 1024 * 1024:
                raise ValueError("Public source archive exceeded16MiB bound; HOLD")
            output.write(chunk)
            digest.update(chunk)
    save(evidence / "ACQUISITION.json", {
        "url": URL, "commit": COMMIT, "start_utc": start, "finish_utc": utc(),
        "archive_bytes": count, "measured_sha256": digest.hexdigest(),
        "trust": "public HTTPS pinned-commit codeload, NOT official release authentication",
    })
    root_name = f"dropbear-{COMMIT}"
    members, seen, total = [], set(), 0
    with tarfile.open(archive, "r:gz") as source:
        entries = source.getmembers()
        if len(entries) > 10000:
            raise ValueError("Archive member count exceeded bound")
        for entry in entries:
            path = PurePosixPath(entry.name)
            if (path.is_absolute() or ".." in path.parts or "\\" in entry.name
                    or not path.parts or path.parts[0] != root_name or entry.name in seen):
                raise ValueError(f"Unsafe or duplicate archive path: {entry.name!r}")
            seen.add(entry.name)
            if not (entry.isfile() or entry.isdir() or entry.issym() or entry.islnk()):
                raise ValueError(f"Special archive member rejected: {entry.name!r}")
            if entry.mode & 0o6000:
                raise ValueError(f"Set-ID archive mode rejected: {entry.name!r}")
            if entry.issym() or entry.islnk():
                target = PurePosixPath(entry.linkname)
                if target.is_absolute() or ".." in target.parts or "\\" in entry.linkname:
                    raise ValueError(f"Unsafe archive link: {entry.name!r}")
                if entry.islnk() and (not target.parts or target.parts[0] != root_name):
                    raise ValueError(f"Hardlink leaves archive root: {entry.name!r}")
            total += entry.size
            if total > 128 * 1024 * 1024:
                raise ValueError("Uncompressed source exceeds128MiB bound")
            members.append({"path": entry.name, "type": entry.type.decode("ascii"),
                            "bytes": entry.size, "mode": oct(entry.mode), "link": entry.linkname})
        save(evidence / "TAR_INSPECTION.json", {
            "utc": utc(), "all_members_checked_before_extraction": True,
            "members": members, "total_regular_bytes": total,
            "links": [m for m in members if m["link"]],
        })
        # Python's data filter adds resolved-link containment checks at extraction.
        work.mkdir(parents=True, exist_ok=False)
        source.extractall(work, filter="data")
    source_root = work / root_name
    old = WORKSPACE / "evidence/delegated/20261013-stock-reference/ssh-control01"
    checked = []
    for manifest in sorted(old.glob("PUBLIC_SOURCE_*.json")):
        for entry in json.loads(manifest.read_text())["files"]:
            if "sha256" not in entry:
                continue
            data = (source_root / entry["file"]).read_bytes()
            actual = hashlib.sha256(data).hexdigest()
            assert len(data) == entry["bytes"] and actual == entry["sha256"], entry["file"]
            checked.append({"file": entry["file"], "sha256": actual, "bytes": len(data)})
    assert len(checked) == 23
    save(evidence / "PRIOR_SOURCE_MATCH.json", {"utc": utc(), "matched": checked, "result": "23/23 PASS"})
    files = []
    for path in sorted(source_root.rglob("*")):
        if path.is_file() and not path.is_symlink():
            files.append({"file": str(path.relative_to(source_root)), "bytes": path.stat().st_size,
                          "sha256": hashlib.sha256(path.read_bytes()).hexdigest()})
    save(evidence / "SOURCE_FILES.json", {"source_root": str(source_root), "utc": utc(), "files": files})
    print(json.dumps({"source_root": str(source_root), "archive_bytes": count,
                      "source_files": len(files), "prior_match": "23/23 PASS"}, indent=2))


if __name__ == "__main__":
    main()
