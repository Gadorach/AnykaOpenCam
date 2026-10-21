#!/usr/bin/env python3
"""Offline ROOT-only proposal builder. Never opens a device or installs anything.

Extraction, modification, repack and verification share ONE fakeroot session.
No credential content/digest or factory-account digest is logged anywhere.
"""
import argparse
import datetime as dt
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import stat
import struct
import subprocess
import sys
import zlib

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
PINS = json.loads((HERE / "public-pins.json").read_text())
RELEASE = ROOT / "evidence/delegated/20261013-stock-reference/ssh-control02/release01"
DUMMY = {"hostkey": b"NOTFLASHABLE PUBLIC DUMMY HOSTKEY\n",
         "authorized_keys": b"NOTFLASHABLE PUBLIC DUMMY AUTHORIZATION\n",
         "wpa.conf": b"NOTFLASHABLE PUBLIC DUMMY WLAN PROFILE\n"}


def utc():
    return dt.datetime.now(dt.timezone.utc).isoformat()


def require(condition, message):
    if not condition:
        raise ValueError(message)


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def private(path, directory=False):
    path = Path(path).absolute()
    require(".." not in path.parts, "Noncanonical private path is refused")
    s = path.lstat()
    require((stat.S_ISDIR(s.st_mode) if directory else stat.S_ISREG(s.st_mode)) and
            s.st_uid == os.getuid() and stat.S_IMODE(s.st_mode) == (0o700 if directory else 0o600),
            "Unsafe private input/output owner, mode or file type")
    require(directory or s.st_nlink == 1, "Private input must not have hardlink aliases")
    for ancestor in path.parents:
        info = ancestor.lstat()
        require(stat.S_ISDIR(info.st_mode) and info.st_uid in (0, os.getuid()) and not info.st_mode & 0o022,
                "Unsafe private path ancestor")
    return path.resolve(strict=True)


def credentials(path, dummy):
    path = private(path, True)
    require({p.name for p in path.iterdir()} == set(DUMMY), "Exactly three credential inputs required; NEVER a client private key")
    for name in DUMMY:
        private(path / name)
    if dummy:
        require(all((path / name).read_bytes() == value for name, value in DUMMY.items()),
                "Dummy mode accepts only the literal public NOTFLASHABLE fixtures")
        return
    # Parent-only execution of this branch. Do not log contents, parse errors or hashes.
    auth = (path / "authorized_keys").read_text()
    require(re.fullmatch(r"restrict ssh-ed25519 [A-Za-z0-9+/]+={0,2}(?: [^\r\n]+)?\n?", auth) is not None,
            "Authorization must be one restricted Ed25519 public key")
    text = (path / "wpa.conf").read_text()
    lines = [line.strip() for line in text.splitlines() if line.strip() and not line.lstrip().startswith("#")]
    require(lines[:3] == ["ctrl_interface=/tmp/bench-ssh/wpa", "update_config=0", "network={"] and lines[-1:] == ["}"],
            "WLAN profile must use the fixed RAM control path and one network")
    values = {}
    for line in lines[3:-1]:
        key, sep, value = line.partition("=")
        require(sep and key not in values, "Invalid/duplicate WLAN field")
        values[key] = value
    expected = {"ssid": '"VertexHotspot"', "key_mgmt": "WPA-PSK", "proto": "RSN", "pairwise": "CCMP", "group": "CCMP"}
    require(set(values) == set(expected) | {"psk"} and all(values.get(k) == v for k, v in expected.items()),
            "Only the reviewed VertexHotspot WPA2/CCMP network is admitted")
    require(re.fullmatch(r'(?:[0-9A-Fa-f]{64}|"(?:[^"\\\r\n]|\\.){8,63}")', values["psk"]) is not None,
            "Invalid private WPA credential encoding")
    require(32 <= (path / "hostkey").stat().st_size <= 4096, "Parent hostkey input has unexpected size")


def audit_listing(text):
    """Audit names/links BEFORE extraction; never follow a target filesystem link."""
    entries = {}
    for line in text.splitlines():
        if not line or not line[0] in "dlcbps-":
            continue
        require(" squashfs-root" in line, "Unrecognized SquashFS listing")
        name = line.split(" squashfs-root", 1)[1]
        target = None
        if line[0] == "l":
            name, separator, target = name.partition(" -> ")
            require(separator and target, "Invalid symlink listing")
        if name.startswith("/"):
            name = name[1:]
        require(not name or re.fullmatch(r"[A-Za-z0-9_.+/-]+", name), "Unsupported/ambiguous archive path")
        parts = PurePosixPath(name).parts
        require(not any(p in (".", "..") for p in parts) and not name.startswith("/"), "Archive path traversal")
        require(name not in entries, "Duplicate archive path")
        if target:
            resolved = [] if target.startswith("/") else list(parts[:-1])
            for part in target.split("/"):
                if part == "..":
                    require(bool(resolved), "Symlink escapes the target root")
                    resolved.pop()
                elif part not in ("", "."):
                    resolved.append(part)
        entries[name] = line[0]
    require(entries.get("") == "d", "No filesystem root listing")
    for name in entries:
        for parent in PurePosixPath(name).parents:
            require(entries.get(str(parent) if str(parent) != "." else "", "d") == "d", "Archive member below a symlink/non-directory")
    return len(entries)


def metadata(tree):
    result = {}
    def visit(path):
        s = path.lstat()
        key = path.relative_to(tree).as_posix()
        row = (stat.S_IFMT(s.st_mode), stat.S_IMODE(s.st_mode), s.st_uid, s.st_gid, int(s.st_mtime), s.st_rdev)
        result[key] = (row, os.readlink(path) if stat.S_ISLNK(s.st_mode) else None, s.st_ino)
        if stat.S_ISDIR(s.st_mode):
            for child in sorted(path.iterdir()):
                visit(child)
    visit(tree)
    return result


def hardlinks(meta, names):
    groups = {}
    for name in names:
        row, target, inode = meta[name]
        if row[0] != stat.S_IFDIR:
            groups.setdefault(inode, set()).add(name)
    return {frozenset(group) for group in groups.values() if len(group) > 1}


def compare(original, candidate, allowed_new, changed_inittab):
    before, after = metadata(original), metadata(candidate)
    require(set(after) == set(before) | set(allowed_new), "Candidate added/removed an unapproved path")
    for name, (row, target, inode) in before.items():
        require(after[name][:2] == (row, target), "Original metadata/link changed")
        if row[0] == stat.S_IFREG:
            left, right = (original / name).read_bytes(), (candidate / name).read_bytes()
            require(right == (changed_inittab if name == "etc/inittab" else left), "Original file content changed")
    require(hardlinks(before, before) == hardlinks(after, before), "Original hardlink topology changed")
    for name, (mode, data) in allowed_new.items():
        row, target, inode = after[name]
        require(row[0] == (stat.S_IFDIR if data is None else stat.S_IFREG) and target is None,
                "Overlay file type changed")
        require(row[1:5] == (mode, 0, 0, PINS["timestamp"]), "Overlay ownership/mode/time changed")
        if data is not None:
            require((candidate / name).read_bytes() == data, "Overlay/input bytes differ")
    return {"original_paths": len(before), "new_paths": len(allowed_new),
            "all_unchanged_metadata_equal": True, "all_unchanged_content_equal": True,
            "original_hardlinks_equal": True, "private_input_bytes_equal": True}


def validate_live_regions(acquired, live):
    require(len(acquired) == len(live) == PINS["flash_bytes"], "Live baseline size changed")
    # ENV/JFFS may contain existing runtime configuration; preserve, never reset.
    for start, end in ((0, 0x60000), (0x70000, 0x200000), (0x300000, 0x780000)):
        require(acquired[start:end] == live[start:end], "Live immutable boot/MAC/kernel/USR differs; review required")


def validate_live_root(acquired_tree, live_tree):
    """Admit only the already authorized passwd-only ROOT delta; never log it."""
    before, after = metadata(acquired_tree), metadata(live_tree)
    require(set(before) == set(after), "Live ROOT path set differs; review required")
    changed = []
    for name, (row, target, inode) in before.items():
        require(after[name][:2] == (row, target), "Live ROOT metadata/link differs; review required")
        if row[0] == stat.S_IFREG and (acquired_tree/name).read_bytes() != (live_tree/name).read_bytes():
            changed.append(name)
    require(hardlinks(before, before) == hardlinks(after, after), "Live ROOT hardlinks differ")
    require(changed in ([], ["etc/passwd"]), "Live ROOT has unreviewed content changes")
    return {"live_original_paths": len(before), "only_prior_passwd_delta": changed == ["etc/passwd"],
            "all_other_content_and_metadata_equal": True}


def superblock(path):
    raw = Path(path).read_bytes()[:96]
    fields = struct.unpack("<5I6H8Q", raw)
    require(fields[0] == 0x73717368 and fields[9:11] == (4, 0), "Not ordinary SquashFS4.0")
    return {"inodes": fields[1], "mkfs_time": fields[2], "block_size": fields[3],
            "compression": fields[5], "flags": fields[7], "id_count": fields[8], "bytes_used": fields[12]}


def xz_geometry(path):
    """Aggregate ONLY validated XZ headers; never export filesystem payloads."""
    image = Path(path).read_bytes()
    offset, streams, dictionaries, filters, checks = 0, 0, set(), set(), set()
    def vli(data, pos):
        value, shift = 0, 0
        while True:
            byte = data[pos]
            pos += 1
            value |= (byte & 127) << shift
            if byte < 128:
                return value, pos
            shift += 7
            require(shift <= 63, "Invalid XZ header integer")
    while True:
        offset = image.find(b"\xfd7zXZ\x00", offset)
        if offset < 0:
            break
        start = offset
        offset += 6
        header = image[start:start + 12]
        if len(header) != 12 or zlib.crc32(header[6:8]) != int.from_bytes(header[8:12], "little"):
            continue
        block_start = start + 12
        length = (image[block_start] + 1) * 4
        block = image[block_start:block_start + length]
        require(len(block) == length and zlib.crc32(block[:-4]) == int.from_bytes(block[-4:], "little"), "Invalid XZ block header")
        flags, pos = block[1], 2
        if flags & 64:
            _, pos = vli(block, pos)
        if flags & 128:
            _, pos = vli(block, pos)
        for _ in range((flags & 3) + 1):
            kind, pos = vli(block, pos)
            size, pos = vli(block, pos)
            properties = block[pos:pos + size]
            pos += size
            filters.add(kind)
            if kind == 0x21:
                require(size == 1 and properties[0] <= 40, "Invalid LZMA2 dictionary header")
                prop = properties[0]
                dictionaries.add(0xffffffff if prop == 40 else (2 | (prop & 1)) << ((prop >> 1) + 11))
        checks.add(header[7])
        streams += 1
    require(streams > 0, "No validated XZ streams")
    return {"validated_stream_headers": streams, "lzma2_dictionary_bytes": sorted(dictionaries),
            "filter_ids": sorted(filters), "stream_check_ids": sorted(checks)}


def run(command, output, report, label):
    started = utc()
    proc = subprocess.run(command, capture_output=True, check=False)
    # All tool diagnostics stay in the private output; they can contain paths.
    (output / (label + ".stdout")).write_bytes(proc.stdout)
    (output / (label + ".stderr")).write_bytes(proc.stderr)
    report["commands"].append({"utc": started, "argv": command, "returncode": proc.returncode})
    require(proc.returncode == 0, "Host tool failed; private attempt retained")
    return proc.stdout


def inner(plan_path):
    os.umask(0o077)
    plan = json.loads(Path(plan_path).read_text())
    output, inputs = Path(plan["output"]), Path(plan["inputs"])
    report = {"started_utc": utc(), "scope": "OFFLINE_ROOT_PROPOSAL_NOT_INSTALLATION_AUTHORITY",
              "dummy_only": plan["dummy"], "commands": [], "original_flash_sha256": PINS["original_flash_sha256"]}
    try:
        root_image = output / "original-root.bin"
        flash = Path(plan["flash"]).read_bytes()
        if plan.get("live_flash"):
            live = Path(plan["live_flash"]).read_bytes()
            require(hashlib.sha256(live).hexdigest() == plan["live_flash_private_sha256"], "Live baseline changed")
            validate_live_regions(flash, live)
            acquired_root = output / "acquired-root.bin"
            acquired_root.write_bytes(flash[PINS["root_offset"]:PINS["root_offset"]+PINS["root_bytes"]])
            listing = run(["/usr/bin/unsquashfs", "-lln", "-full", "-UTC", str(acquired_root)], output, report, "acquired-list")
            audit_listing(listing.decode())
            run(["/usr/bin/unsquashfs", "-strict-errors", "-no-progress", "-processors", "1", "-d", str(output/"acquired-tree"), str(acquired_root)], output, report, "acquired-extract")
            flash = live
        root_image.write_bytes(flash[PINS["root_offset"]:PINS["root_offset"] + PINS["root_bytes"]])
        original_super = superblock(root_image)
        require(original_super["bytes_used"] == PINS["original_bytes_used"] and original_super["block_size"] == 131072,
                "Original ROOT geometry changed")
        listing = run(["/usr/bin/unsquashfs", "-lln", "-full", "-UTC", str(root_image)], output, report, "original-list")
        report["audited_archive_paths"] = audit_listing(listing.decode())
        original, tree = output / "original-tree", output / "candidate-tree"
        for label, destination in (("original-extract", original), ("candidate-extract", tree)):
            run(["/usr/bin/unsquashfs", "-strict-errors", "-no-progress", "-processors", "1", "-d", str(destination), str(root_image)], output, report, label)
        if plan.get("live_flash"):
            report["reviewed_live_root_baseline"] = validate_live_root(output/"acquired-tree", original)
            report["outside_root_reference"] = "fresh_reviewed_live_backup_including_existing_ENV_JFFS"
        else:
            report["outside_root_reference"] = "immutable_original_acquisition"
        public_runtime = dict(PINS["runtime"])
        for path, sha in public_runtime.items():
            if path.startswith("/usr/"):
                content = run(["/usr/bin/unsquashfs", "-cat", "-o", "3145728", plan["flash"], path[5:]], output, report,
                              "check-public-" + Path(path).name)
            elif path == "/bench-ssh/dropbear":
                content = Path(plan["dropbear"]).read_bytes()
            else:
                content = (original / path.lstrip("/")).read_bytes()
            require(hashlib.sha256(content).hexdigest() == sha, "Public runtime source pin mismatch")
        public_runtime["/bin/busybox"] = digest(original / "bin/busybox")
        report["public_runtime_pins_match_original"] = True
        saved = metadata(tree)
        inittab = tree / "etc/inittab"
        require(stat.S_ISREG(inittab.lstat().st_mode) and inittab.lstat().st_nlink == 1, "Unexpected inittab kind/hardlink")
        original_tab = inittab.read_bytes()
        old = b"::sysinit:/etc/init.d/rcS"
        require(original_tab.count(old) == 1, "Expected unique stock sysinit missing")
        changed_tab = original_tab.replace(old, b"::sysinit:/bench-ssh/bootstrap") + b"\n::respawn:/bench-ssh/serve\n"
        inittab.write_bytes(changed_tab)
        new = {"bench-ssh": (0o700, None), "bench-ssh/private": (0o700, None), "bench-ssh/licenses": (0o700, None)}
        for name in ("bootstrap", "local-only-dhcp", "serve"):
            new["bench-ssh/" + name] = (0o700, (HERE / "overlay" / name).read_bytes())
        new["bench-ssh/dropbear"] = (0o700, Path(plan["dropbear"]).read_bytes())
        new["bench-ssh/runtime.sha256"] = (0o600, "".join(f"{sha}  {path}\n" for path, sha in public_runtime.items()).encode())
        for path in sorted((RELEASE / "licenses").iterdir()):
            require(path.is_file() and not path.is_symlink(), "Unexpected license artifact")
            new["bench-ssh/licenses/" + path.name] = (0o600, path.read_bytes())
        for name in DUMMY:
            new["bench-ssh/private/" + name] = (0o600, (inputs / name).read_bytes())
        if plan["dummy"]:
            new["bench-ssh/NOTFLASHABLE"] = (0o600, b"PUBLIC DUMMY INPUTS: NEVER FLASH OR BOOT THIS IMAGE\n")
        for name, (mode, data) in new.items():
            path = tree / name
            require(not path.exists() and not path.is_symlink(), "Overlay path collision")
            if data is None:
                path.mkdir(mode=mode)
            else:
                path.write_bytes(data)
            os.chmod(path, mode)
            os.chown(path, 0, 0)
            os.utime(path, (PINS["timestamp"], PINS["timestamp"]))
        # Restore all original timestamps, including directories affected by additions.
        for name, (row, target, inode) in saved.items():
            os.utime(tree / name, (row[4], row[4]), follow_symlinks=False)
        for name, (mode, data) in new.items():
            os.utime(tree / name, (PINS["timestamp"], PINS["timestamp"]))
        report["prepack_equality"] = compare(original, tree, new, changed_tab)
        squash = output / "root.squashfs"
        flags = ["-noappend", "-comp", "xz", "-b", "131072", "-Xdict-size", "131072", "-no-xattrs",
                 "-exports", "-mkfs-time", str(PINS["timestamp"]), "-processors", "1", "-single-reader", "-no-progress", "-exit-on-error"]
        run(["/usr/bin/mksquashfs", str(tree), str(squash), *flags], output, report, "pack")
        os.chmod(squash, 0o600)
        report["original_superblock"], report["candidate_superblock"] = original_super, superblock(squash)
        report["original_xz_headers"], report["candidate_xz_headers"] = xz_geometry(root_image), xz_geometry(squash)
        geometry = report["candidate_xz_headers"]
        require(geometry["filter_ids"] == [0x21] and max(geometry["lzma2_dictionary_bytes"]) <= 131072 and
                set(geometry["stream_check_ids"]) <= set(report["original_xz_headers"]["stream_check_ids"]),
                "HOLD: new XZ decoder/filter/dictionary requirement")
        require(report["candidate_superblock"]["compression"] == 4 and report["candidate_superblock"]["block_size"] == 131072,
                "HOLD: non-original SquashFS codec/block geometry")
        report["padded_squashfs_bytes"] = squash.stat().st_size
        report["fits_1MiB"] = squash.stat().st_size <= PINS["root_bytes"]
        require(report["fits_1MiB"], "HOLD: ROOT exceeds 1MiB; no USR/config fallback permitted")
        run(["/usr/bin/unsquashfs", "-s", str(squash)], output, report, "candidate-superblock")
        listing = run(["/usr/bin/unsquashfs", "-lln", "-full", "-UTC", str(squash)], output, report, "candidate-list")
        audit_listing(listing.decode())
        verified = output / "verified-tree"
        run(["/usr/bin/unsquashfs", "-strict-errors", "-no-progress", "-processors", "1", "-d", str(verified), str(squash)], output, report, "verify-extract")
        report["roundtrip_equality"] = compare(original, verified, new, changed_tab)
        padded = squash.read_bytes() + b"\xff" * (PINS["root_bytes"] - squash.stat().st_size)
        (output / "ROOT-NOTFLASHABLE.bin" if plan["dummy"] else output / "ROOT-PARENT-REVIEW.bin").write_bytes(padded)
        start, end = PINS["root_offset"], PINS["root_offset"] + PINS["root_bytes"]
        composed = flash[:start] + padded + flash[end:]
        require(composed[:start] == flash[:start] and composed[end:] == flash[end:], "Non-ROOT bytes changed")
        (output / "flash-NOTFLASHABLE.bin" if plan["dummy"] else output / "flash-PARENT-REVIEW.bin").write_bytes(composed)
        report["outside_root_byte_equal"] = True
        report["root_partition_bytes"] = len(padded)
        report["partition_headroom_bytes"] = PINS["root_bytes"] - squash.stat().st_size
        # Whole composed image digest ONLY in a private ledger, never the public report.
        (output / "PRIVATE-LEDGER.json").write_text(json.dumps({"whole_image_sha256": hashlib.sha256(composed).hexdigest()}) + "\n")
        require(digest(plan["flash"]) == PINS["original_flash_sha256"], "Immutable input changed during build")
        if plan.get("live_flash"):
            require(digest(plan["live_flash"]) == plan["live_flash_private_sha256"], "Live baseline changed during build")
        report["result"] = "DUMMY_FIT_STRUCTURE_PASS_NOTFLASHABLE" if plan["dummy"] else "PRIVATE_PROPOSAL_PARENT_REVIEW_REQUIRED"
    except Exception as error:
        report["result"] = "HOLD_PRIVATE_ATTEMPT_RETAINED"
        report["failure_class"] = type(error).__name__
        # Our own ValueErrors contain only fixed diagnostic labels, no credentials.
        if isinstance(error, ValueError):
            report["failure_label"] = str(error)
        raise
    finally:
        report["finished_utc"] = utc()
        (output / "PRIVATE-REPORT.json").write_text(json.dumps(report, indent=2) + "\n")


def main():
    os.umask(0o077)
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--original-flash", type=Path)
    parser.add_argument("--private-inputs", type=Path)
    parser.add_argument("--private-output", type=Path)
    parser.add_argument("--reviewed-live-flash", type=Path,
                        help="Parent-only protected fresh backup; admit only prior passwd delta in ROOT and preserve ENV/JFFS")
    parser.add_argument("--dummy-only", action="store_true")
    parser.add_argument("--inner", type=Path, help=argparse.SUPPRESS)
    args = parser.parse_args()
    if args.inner:
        require(bool(os.environ.get("FAKEROOTKEY")), "Internal stage requires one fakeroot session")
        inner(args.inner)
        return
    require(all((args.original_flash, args.private_inputs, args.private_output)), "Explicit input/output paths required")
    require(args.dummy_only or args.reviewed_live_flash is not None,
            "Fresh reviewed live backup is required for real composition")
    flash = args.original_flash.absolute()
    require(stat.S_ISREG(flash.lstat().st_mode) and flash.stat().st_size == PINS["flash_bytes"], "Expected regular immutable 8MiB read")
    require(digest(flash) == PINS["original_flash_sha256"], "Original acquisition digest mismatch")
    inputs = private(args.private_inputs, True)
    credentials(inputs, args.dummy_only)
    live_flash = None
    if args.reviewed_live_flash:
        require(not args.dummy_only, "Live baseline is parent-only, not a dummy fixture")
        live_flash = private(args.reviewed_live_flash)
        require(not live_flash.is_relative_to(ROOT) and live_flash.stat().st_size == PINS["flash_bytes"],
                "Live backup must be private, outside Git and exactly8MiB")
        validate_live_regions(flash.read_bytes(), live_flash.read_bytes())
    output = args.private_output.absolute()
    require(not output.exists() and not output.is_symlink(), "Refusing output overwrite")
    output = private(output.parent, True) / output.name
    if not args.dummy_only:
        require(not inputs.is_relative_to(ROOT) and not output.is_relative_to(ROOT), "Real private inputs/output must stay outside Git")
    binary = RELEASE / "dropbear-armv5-static"
    require(binary.stat().st_size == PINS["dropbear_bytes"] and digest(binary) == PINS["dropbear_sha256"], "Reviewed Dropbear mismatch")
    output.mkdir(mode=0o700)
    (output / "tmp").mkdir(mode=0o700)
    plan = {"flash": str(flash), "inputs": str(inputs), "output": str(output), "dummy": args.dummy_only, "dropbear": str(binary)}
    if live_flash:
        plan["live_flash"] = str(live_flash)
        plan["live_flash_private_sha256"] = digest(live_flash)
    (output / "PRIVATE-PLAN.json").write_text(json.dumps(plan) + "\n")
    env = dict(os.environ, TMPDIR=str(output / "tmp"), LC_ALL="C", TZ="UTC", PYTHONDONTWRITEBYTECODE="1")
    for key in ("SOURCE_DATE_EPOCH", "SQFS_CMDLINE"):
        env.pop(key, None)
    # No shell, sudo, target binary execution, device path or flash-writing tool.
    process = subprocess.run(["/usr/bin/fakeroot", "--", sys.executable, "-B", str(Path(__file__).resolve()), "--inner", str(output / "PRIVATE-PLAN.json")],
                             env=env, capture_output=True)
    (output / "fakeroot.stdout").write_bytes(process.stdout)
    (output / "fakeroot.stderr").write_bytes(process.stderr)
    # Payload metadata was verified INSIDE fakeroot. After that session ends,
    # protect retained host tree roots too; never use them as repack inputs later.
    for name in ("original-tree", "candidate-tree", "verified-tree", "acquired-tree"):
        tree = output / name
        if tree.exists() and not tree.is_symlink() and tree.is_dir():
            os.chmod(tree, 0o700)
    require(process.returncode == 0, "HOLD: inspect the preserved private attempt; no image admitted")
    print("OFFLINE candidate only; private report retained; NOT installed/booted/authorized for writing")


if __name__ == "__main__":
    try:
        main()
    except Exception:
        # Tracebacks could contain private path/parse details. Keep failures generic.
        print("HOLD: private build/input validation failed; preserve the attempt for parent review", file=sys.stderr)
        raise SystemExit(1)
