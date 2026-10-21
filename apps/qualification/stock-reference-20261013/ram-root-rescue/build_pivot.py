#!/usr/bin/env python3
"""Build inert public-code tmpfs pivot tar; no target/ARM execution or credentials."""
import argparse
import datetime as dt
import hashlib
import io
import json
import lzma
from pathlib import Path
import re
import struct
import subprocess
import tarfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
IMPL = ROOT / "work/lightbulb-camera/implementation-20261008T205404Z"
BB_BUILD = IMPL / "build/wifi-userland3/busybox"
BB_SOURCE = IMPL / "sources/busybox-1.37.0/busybox-1.37.0"
STOCK = ROOT / "firmware/stock/lightbulb-camera/stock-filesystems-deep-20261008T055753Z"
LIB = STOCK / "rootfs/root/lib"
RELEASE = ROOT / "evidence/delegated/20261013-stock-reference/ssh-control02/release01"
KERNEL = ROOT / ("work/lightbulb-camera/deep-bringup-20261008T055753Z/"
                 "sources/ak3918ev200/vendor/linux-3.4.35")
EPOCH = 1792108800  # Fixed public build metadata: 2026-10-16T00:00:00Z.
PINS = {
    "static_busybox": "3f7e15eae5f1c03a0af78ce45a341aaa48ad0eb0a00850a0914512e830b7c3a1",
    "stock_busybox": "9ac796da06ec22d82dbce9aa8996a27e97d95cf8de4bfeb0441a76ad8bb10e5b",
    "dropbear": "e65041f6cc3b2ef8c1a22fca2a204d85860f3840617b27b0eb597f4dd7af3ef4",
}


def sha(data):
    return hashlib.sha256(data).hexdigest()


def require(ok, message):
    if not ok:
        raise ValueError(message)


def elf_info(path):
    data = path.read_bytes()
    require(data[:7] == b"\x7fELF\x01\x01\x01", "Expected ELF32 LE")
    header = struct.unpack_from("<16sHHIIIIIHHHHHH", data)
    require(header[2] == 40, "Not ARM")
    program = [struct.unpack_from("<8I", data, header[5] + i * header[9])
               for i in range(header[10])]
    interp = [data[p[1]:p[1] + p[4]].rstrip(b"\0").decode() for p in program if p[0] == 3]
    command = ["readelf", "-hAlWd", str(path)]
    text = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                          check=True, text=True).stdout
    return {"path": str(path.relative_to(ROOT)), "sha256": sha(data), "bytes": len(data),
            "interpreter": interp, "dynamic": any(p[0] == 2 for p in program),
            "needed": re.findall(r"\(NEEDED\).*?\[([^]]+)\]", text)}, text, command


def tar_bytes(entries):
    out = io.BytesIO()
    with tarfile.open(fileobj=out, mode="w", format=tarfile.USTAR_FORMAT) as archive:
        for name, row in sorted(entries.items()):
            require(name and not name.startswith("/") and ".." not in name.split("/"), "Bad tar path")
            info = tarfile.TarInfo(name)
            info.uid = info.gid = 0
            info.uname = info.gname = "root"
            info.mtime = EPOCH
            info.mode = row["mode"]
            kind = row["kind"]
            if kind == "dir":
                info.type = tarfile.DIRTYPE
            elif kind == "symlink":
                info.type = tarfile.SYMTYPE
                info.linkname = row["link"]
            elif kind == "char":
                info.type = tarfile.CHRTYPE
                info.devmajor, info.devminor = row["device"]
            else:
                info.size = len(row["data"])
            archive.addfile(info, io.BytesIO(row["data"]) if kind == "file" else None)
    return out.getvalue()


def inspect_tar(blob, entries):
    """Check in-memory metadata/contents only; NEVER extract nodes onto the host."""
    with tarfile.open(fileobj=io.BytesIO(blob), mode="r:") as archive:
        members = archive.getmembers()
        require(len(members) == len(entries), "Unexpected tar member count")
        require({p.name for p in members} == set(entries), "Tar allowlist changed")
        for p in members:
            row = entries[p.name]
            require((p.uid, p.gid, p.mtime, p.mode) == (0, 0, EPOCH, row["mode"]), "Tar metadata mismatch")
            if row["kind"] == "file":
                require(p.isfile() and archive.extractfile(p).read() == row["data"], "Tar file mismatch")
            elif row["kind"] == "dir":
                require(p.isdir(), "Tar directory mismatch")
            elif row["kind"] == "char":
                require(p.ischr() and (p.devmajor, p.devminor) == row["device"], "Tar node mismatch")
            else:
                require(p.issym() and p.linkname == row["link"], "Tar symlink mismatch")
    return True


def build(output):
    output = output.absolute()
    output.mkdir(parents=True, exist_ok=False)
    entries, inputs, elfs, commands = {}, [], [], []

    def directory(name, mode=0o755):
        entries[name] = {"kind": "dir", "mode": mode}

    def file(name, data, mode=0o644):
        entries[name] = {"kind": "file", "mode": mode, "data": data}

    def source(name, path, mode=0o644, pin=None):
        data = path.read_bytes()
        require(pin is None or sha(data) == pin, "Pinned nonsecret input changed: " + name)
        file(name, data, mode)
        inputs.append({"source": str(path.relative_to(ROOT)) if path.is_relative_to(ROOT) else str(path),
                       "output": name, "sha256": sha(data), "bytes": len(data)})

    def inspect(path):
        row, text, command = elf_info(path)
        index = len(elfs)
        elfs.append(row)
        (output / ("elf-%02d.txt" % index)).write_text(text)
        commands.append(command)
        return row

    for name in ["bin", "sbin", "dev", "proc", "sys", "state", "oldroot", "usr", "lib", "etc",
                 "rescue", "rescue/licenses", "rescue-tools", "root"]:
        directory(name, 0o700 if name in ["state", "oldroot", "root"] else 0o755)
    # Only metadata in the tar: no host mknod/device creation.
    entries["dev/console"] = {"kind": "char", "mode": 0o600, "device": (5, 1)}
    entries["dev/null"] = {"kind": "char", "mode": 0o666, "device": (1, 3)}
    entries["tmp"] = {"kind": "symlink", "mode": 0o777, "link": "state"}
    static = inspect(BB_BUILD / "busybox")
    require(not static["dynamic"] and not static["interpreter"] and not static["needed"], "BusyBox not static")
    config = (BB_BUILD / ".config").read_text()
    require("CONFIG_STATIC=y\n" in config and "CONFIG_ASH=y\n" in config, "Missing static ash config")
    applets = re.findall(r'^"([a-z0-9_]+)" "\\0"$', (BB_BUILD / "include/applet_tables.h").read_text(), re.M)
    require({"sh", "ash", "env", "sha256sum", "dd", "insmod", "udhcpc"} <= set(applets), "Missing static applet")
    require(not ({"mount", "umount", "pivot_root", "mdev", "tar"} & set(applets)), "Re-review changed applet set")
    source("bin/busybox", BB_BUILD / "busybox", 0o755, PINS["static_busybox"])
    source("rescue-tools/busybox", STOCK / "rootfs/root/bin/busybox", 0o755, PINS["stock_busybox"])
    stockbb = inspect(STOCK / "rootfs/root/bin/busybox")
    require(stockbb["interpreter"] == ["/lib/ld-uClibc.so.0"], "Unexpected stock loader")
    # Check contiguous stock applet-name table, not host execution or target --list.
    data = (STOCK / "rootfs/root/bin/busybox").read_bytes()
    require(b"mdev\0mesg\0mkdir\0" in data and b"mount\0mountpoint\0mv\0" in data and
            b"ping\0pivot_root\0pkill\0" in data and b"uevent\0umount\0uname\0" in data,
            "Stock applet table changed")
    for applet in applets:
        entries["bin/" + applet] = {"kind": "symlink", "mode": 0o777, "link": "busybox"}
    for applet in ["mount", "umount", "readlink", "stat", "tar"]:
        entries["bin/" + applet] = {"kind": "symlink", "mode": 0o777, "link": "../rescue-tools/busybox"}
    for applet in ["pivot_root", "mdev"]:
        entries["sbin/" + applet] = {"kind": "symlink", "mode": 0o777, "link": "../rescue-tools/busybox"}
    # Pre-copy ROOT-resident closure for stock BusyBox AND the stock WPA/libnl pair.
    # WPA/libnl executables stay out of this bundle, available only if parent moves
    # the separately mounted read-only USR. No factory config is copied.
    pending = set(stockbb["needed"] + ["ld-uClibc.so.0", "librt.so.0", "libpthread.so.0"])
    copied = set()
    while pending:
        name = sorted(pending)[0]
        pending.remove(name)
        if name in copied:
            continue
        require(re.fullmatch(r"[a-zA-Z0-9_.-]+", name), "Unsafe dependency name")
        path = LIB / name
        require(path.is_symlink(), "Expected original SONAME link")
        target = str(path.readlink())
        require("/" not in target, "Unexpected original library link")
        entries["lib/" + name] = {"kind": "symlink", "mode": 0o777, "link": target}
        source("lib/" + target, LIB / target, 0o755)
        row = inspect(LIB / target)
        pending.update(set(row["needed"]) - copied)
        copied.add(name)
    wpa = inspect(STOCK / "usr/root/bin/wpa_supplicant")
    nl = inspect(STOCK / "usr/root/lib/libnl.so.1.1.4")
    require(set(wpa["needed"]) - {"libnl.so.1"} <= copied and set(nl["needed"]) <= copied,
            "Stock WPA ROOT library closure incomplete")
    source("rescue/dropbear", RELEASE / "dropbear-armv5-static", 0o755, PINS["dropbear"])
    db = inspect(RELEASE / "dropbear-armv5-static")
    require(not db["dynamic"] and not db["interpreter"] and not db["needed"], "Dropbear not static")
    for path in sorted((RELEASE / "licenses").iterdir()):
        source("rescue/licenses/" + path.name, path)
    source("rescue/licenses/BusyBox-GPL2", BB_SOURCE / "LICENSE")
    source("rescue/licenses/LGPL-2.1-text", Path("/usr/share/licenses/spdx/LGPL-2.1-only.txt"))
    source("rescue/static-busybox.config", BB_BUILD / ".config")
    source("rescue/static-busybox-applet-tables.h", BB_BUILD / "include/applet_tables.h")
    for name in ["pivot-stage1.sh", "pivot-stage2.sh", "PIVOT-LICENSING.md"]:
        source("rescue/" + name, HERE / name)
    file("rescue/BUNDLE-ID", b"QC07F_RAM_ROOT_PIVOT01_PUBLIC_CODE_ONLY_NO_AUTOSTART\n")
    file("etc/passwd", b"root:x:0:0:RAM rescue:/root:/bin/sh\n")
    file("etc/group", b"root:x:0:\n")
    file("etc/mdev.conf", b"# Deliberately empty; no command hooks and no automatic mdev scan.\n")
    # No shadow, original accounts, DNS/WPA config, keys, daemon init or boot script.
    checksums = "".join(sha(row["data"]) + "  " + name + "\n" for name, row in sorted(entries.items())
                        if row["kind"] == "file")
    file("rescue/SHA256SUMS", checksums.encode())
    manifest = []
    for name, row in sorted(entries.items()):
        item = {"path": name, "kind": row["kind"], "mode": oct(row["mode"]),
                "uid": 0, "gid": 0, "mtime": EPOCH}
        if row["kind"] == "file":
            item.update(bytes=len(row["data"]), sha256=sha(row["data"]))
        elif row["kind"] == "symlink":
            item["link"] = row["link"]
        elif row["kind"] == "char":
            item["device"] = row["device"]
        manifest.append(item)
    plain = tar_bytes(entries)
    inspect_tar(plain, entries)
    packed = lzma.compress(plain, format=lzma.FORMAT_XZ, check=lzma.CHECK_CRC32,
                           filters=[{"id": lzma.FILTER_LZMA2, "dict_size": 65536, "preset": 6}])
    require(len(plain) <= 4 * 1024 * 1024 and len(packed) < 1024 * 1024, "Payload size budget exceeded")
    artifacts = []
    for name, payload in [("ram-root-pivot01.tar", plain), ("ram-root-pivot01.tar.xz", packed)]:
        (output / name).write_bytes(payload)
        artifacts.append({"path": name, "bytes": len(payload), "sha256": sha(payload)})
    (output / "CONTENTS.json").write_text(json.dumps(manifest, indent=2) + "\n")
    sources = [BB_SOURCE / n for n in ["LICENSE", "libbb/appletlib.c", "shell/ash.c", "util-linux/pivot_root.c"]]
    sources += [KERNEL / n for n in ["fs/namespace.c", "fs/fs_struct.c", "COPYING"]]
    sources += [IMPL / "build_wifi_userland.py", HERE / "build_pivot.py", HERE / "pivot-stage1.sh", HERE / "pivot-stage2.sh"]
    report = {"unit": "ram-root-pivot01", "utc": dt.datetime.now(dt.timezone.utc).isoformat(),
              "status": "HOST_BUILT_ONLY_PARENT_REVIEW_AND_NATIVE_PIVOT_REQUIRED", "artifacts": artifacts,
              "inputs": inputs, "elfs": elfs, "commands": commands, "static_busybox_applets": applets,
              "stock_root_library_closure": sorted(copied), "static_busybox_rebuilt": False,
              "source_hashes": [{"path": str(p.relative_to(ROOT)), "sha256": sha(p.read_bytes())} for p in sources],
              "tar_metadata_and_contents_verified": True, "host_nodes_created": False,
              "device_network_socket_private_access": False, "arm_executed": False,
              "originals_modified": False, "native": "NOT_RUN", "power_loss_rollback": "NOT_ESTABLISHED"}
    (output / "REPORT.json").write_text(json.dumps(report, indent=2) + "\n")
    version = subprocess.run(["readelf", "--version"], check=True, stdout=subprocess.PIPE, text=True).stdout
    (output / "readelf-version.txt").write_text(version)
    print(json.dumps({"output": str(output), "artifacts": artifacts, "status": report["status"]}))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    build(parser.parse_args().output)
