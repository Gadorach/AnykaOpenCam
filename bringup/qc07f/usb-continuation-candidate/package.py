#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Deliberate paired initramfs76, fresh final build, and RAM-only image package."""
import datetime
import gzip
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import sys
import zlib
import cpio

if not __debug__:
    raise SystemExit("Candidate safety checks require assertions; do not use Python -O")

ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent
FOUND = ROOT / "work/lightbulb-camera/foundation-20261011T041148Z"
ART = FOUND / "artifacts/native-usb76-source01"
FIRST = FOUND / "analysis/native-usb76-build01"
OUT = FOUND / "analysis/native-usb76-package01"
OLD = FOUND / "analysis/native-security75-build01"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def elf(path):
    data = path.read_bytes()
    assert data[:7] == b"\x7fELF\x01\x01\x01" and struct.unpack_from("<H", data, 18)[0] == 40
    offset = struct.unpack_from("<I", data, 32)[0]
    size, count = struct.unpack_from("<HH", data, 46)
    assert size == 40 and offset + count * size <= len(data)
    sections = [struct.unpack_from("<10I", data, offset + i * size) for i in range(count)]
    symbols = {}
    for sec in sections:
        if sec[1] != 2:
            continue
        strings = sections[sec[6]]
        text = data[strings[4]:strings[4] + strings[5]]
        assert sec[9] == 16
        for i in range(0, sec[5], 16):
            name, value, length, info, other, index = struct.unpack_from("<IIIBBH", data, sec[4] + i)
            if name:
                key = text[name:text.index(b"\0", name)].decode()
                symbols[key] = (value, length, index)

    def read(address, length):
        match = [s for s in sections if s[1] != 8 and s[3] <= address and address + length <= s[3] + s[5]]
        assert len(match) == 1
        s = match[0]; start = s[4] + address - s[3]
        return data[start:start + length]
    return symbols, read


def config(text):
    return dict(re.findall(r"(?m)^(CONFIG_\w+)=(.*)$", text))


def main():
    OUT.mkdir(exist_ok=False)
    build = OUT / "build"; build.mkdir()
    log = OUT / "commands"; log.mkdir(); (OUT / "home").mkdir(); (OUT / "tmp").mkdir()
    prep = json.loads((FIRST / "prepare76-manifest.json").read_text())
    first = json.loads((FIRST / "build76-manifest.json").read_text())
    assert first["state"] == "BUILT_LINKED_NOT_PACKAGED_NOT_BOOTED"
    assert all(sha(ROOT / path) == value for path, value in first["output_sha256"].items())
    original = json.loads((OLD / "build75-manifest.json").read_text())["output_sha256"]
    assert all(sha(Path(path)) == value for path, value in original.items())
    src = ART / "linux"
    for name, value in prep["only_changed_kernel_file"].items():
        assert sha(src / name) == value
    reviews = [ROOT / "docs/qualification-plans/kernel76-integration-20261020" / name
               for name in ("PACKAGING_REVIEW.md", "USB_PARENT_REVIEW.md")]
    assert all(path.is_file() for path in reviews)
    radio = ART / "images/hgics-linux6-station76.ko"
    rtsp = ROOT / "evidence/delegated/20261016-optical-media-parent/run01/linked-arm/rtspd"
    bridge = ROOT / "work/lightbulb-camera/implementation-20261008T205404Z/artifacts/qc07f-dhcp-script-bridge-armv5"
    assert sha(rtsp) == "ddc45d7bd18ed8ab525c309e5ed617db127147670d5f9da0eefeeef6e3f467dd"
    assert sha(bridge) == "6c7766557c1d0fe9e83ef33a5339f3836c78a29f84e2a4a02ae64b0d2bb39ef2"
    old_cpio = ART / "inputs/initramfs50.cpio"
    assert sha(old_cpio) == "6a3cf9d1825bf10791d32913509f0a625b592a2e4dcac9e65883231327f28672"
    old_config = (FIRST / "build/.config").read_text()
    selected = Path(json.loads(config(old_config)["CONFIG_INITRAMFS_SOURCE"]))
    assert sha(selected) == sha(old_cpio)
    embedded = gzip.decompress((FIRST / "build/usr/initramfs_inc_data").read_bytes())
    assert embedded == old_cpio.read_bytes()
    replacements = {"hgics.ko": radio.read_bytes(), "bin/rtspd": rtsp.read_bytes()}
    additions = {"bin/qc07f-dhcp-script-bridge": bridge.read_bytes()}
    packed = cpio.update(embedded, replacements, additions)
    initramfs = ART / "inputs/initramfs76.cpio"
    with initramfs.open("xb") as f:
        f.write(packed)
    old_rows = {x["path"]: x for x in cpio.parse(embedded)}
    new_rows = {x["path"]: x for x in cpio.parse(packed)}
    entries = {name: {"bytes": len(new_rows[name]["payload"]), "sha256": hashlib.sha256(new_rows[name]["payload"]).hexdigest(),
                      "mode": oct(new_rows[name]["fields"][1]), "uid": new_rows[name]["fields"][2], "gid": new_rows[name]["fields"][3]}
               for name in replacements.keys() | additions.keys()}
    assert new_rows["init"]["raw"] == old_rows["init"]["raw"]
    new_config = re.sub(r'(?m)^CONFIG_INITRAMFS_SOURCE=.*$', 'CONFIG_INITRAMFS_SOURCE="' + str(initramfs) + '"', old_config)
    (build / ".config").write_text(new_config)
    expected_delta = {"CONFIG_INITRAMFS_SOURCE": {"old": config(old_config)["CONFIG_INITRAMFS_SOURCE"], "new": config(new_config)["CONFIG_INITRAMFS_SOURCE"]}}
    env = dict(prep["environment"], HOME=str(OUT / "home"), TMPDIR=str(OUT / "tmp"))
    records = []

    def run(name, argv, timeout=1800):
        item = {"start": datetime.datetime.now(datetime.timezone.utc).isoformat(), "argv": list(map(str, argv)), "cwd": str(ROOT)}
        print("START", name, flush=True)
        with (log / (name + ".log")).open("xb") as f:
            result = subprocess.run(item["argv"], cwd=ROOT, env=env, stdout=f, stderr=subprocess.STDOUT, timeout=timeout)
        item.update(exit=result.returncode, end=datetime.datetime.now(datetime.timezone.utc).isoformat())
        records.append(item)
        with (log / "commands.jsonl").open("a") as f:
            f.write(json.dumps(item) + "\n")
        assert result.returncode == 0, name
        print("END", name, result.returncode, flush=True)
        return (log / (name + ".log")).read_text()

    base = ["make", "-C", src, "O=" + str(build), "ARCH=arm", "LLVM=1"]
    run("olddefconfig-final76", base + ["olddefconfig"])
    actual = config((build / ".config").read_text()); before = config(old_config)
    delta = {key: {"old": before.get(key), "new": actual.get(key)} for key in before.keys() | actual.keys() if before.get(key) != actual.get(key)}
    assert delta == expected_delta
    run("kernel-final76", base + ["-j4", "zImage", "modules"])
    assert gzip.decompress((build / "usr/initramfs_inc_data").read_bytes()) == packed
    # A second radio build against the FINAL output must exactly reproduce the
    # embedded module. No merely matching vermagic/symvers substitution.
    station = ART / "station76-final"
    station.mkdir(exist_ok=False)
    for name, value in prep["paired_radio_source75_pins"].items():
        source = ART / "station76" / Path(name).relative_to("station75")
        assert sha(source) == value
        dest = station / Path(name).relative_to("station75")
        dest.parent.mkdir(parents=True, exist_ok=True)
        with dest.open("xb") as f:
            f.write(source.read_bytes())
    run("station-final76", base + ["M=" + str(station / "hgic_smac"), "CONFIG_HGICS=m", "CONFIG_HGIC_USB=y", "CONFIG_HGIC_2G=y", "-j4", "modules"], 300)
    final_radio = station / "hgic_smac/hgics.ko"
    assert final_radio.read_bytes() == radio.read_bytes(), "final-output module differs from embedded candidate; HOLD"
    assert (build / "Module.symvers").read_bytes() == (FIRST / "build/Module.symvers").read_bytes()
    assert (build / "include/config/kernel.release").read_bytes() == (FIRST / "build/include/config/kernel.release").read_bytes()
    symbols, read = elf(build / "vmlinux")
    text_phys = 0x81800000 + symbols["_text"][0] - 0xc0000000
    end_phys = 0x81800000 + symbols["_end"][0] - 0xc0000000
    assert text_phys == 0x81808000 and end_phys < 0x82408000
    old_symbols, old_read = elf(OLD / "build/vmlinux")
    assert symbols["crc_ram01_static_t"][1] == old_symbols["crc_ram01_static_t"][1] == 4096
    assert read(symbols["crc_ram01_static_t"][0], 4096) == old_read(old_symbols["crc_ram01_static_t"][0], 4096)
    syscall_matrix = json.loads((OLD / "prepare75-manifest.json").read_text())["syscall_prerequisites_source_table"]
    table, ni = symbols["sys_call_table"][0], symbols["sys_ni_syscall"][0]
    for item in syscall_matrix.values():
        address = struct.unpack("<I", read(table + item["nr"] * 4, 4))[0]
        assert address == symbols[item["entry"]][0] and address != ni
    assert struct.unpack("<I", read(table + 386 * 4, 4))[0] == ni
    for name, path in (("station", final_radio), ("rtsp", rtsp), ("DHCP-bridge", bridge)):
        report = run(name + "-ELF", ["llvm-readelf", "-h", "-l", "-d", path], 30)
        assert "INTERP" not in report and "NEEDED" not in report
    run("final76-musb-assembly", ["llvm-objdump", "-dr", "--disassemble-symbols=musb_host_tx", build / "drivers/usb/musb/musb_host.o"], 30)
    dtb = ART / "inputs/attempt45.dtb"
    assert sha(dtb) == "d837c58c90b56ebb6ce9346b7c1d69689d5a1122dcef55b0f72341e6d003e0b5"
    payload = (build / "arch/arm/boot/zImage").read_bytes() + dtb.read_bytes()
    header = struct.pack(">7I4B32s", 0x27051956, 0, 0, len(payload), 0x82408000, 0x82408040,
                         zlib.crc32(payload) & 0xffffffff, 5, 2, 2, 0, b"QC07F USB76 paired RAM".ljust(32, b"\0"))
    header = header[:4] + struct.pack(">I", zlib.crc32(header) & 0xffffffff) + header[8:]
    image = header + payload
    assert 0x82408000 + len(image) < 0x82800000
    image_path = ART / "images/QC6U76.UIM"
    unverified_path = OUT / "QC6U76.UNVERIFIED"
    with unverified_path.open("xb") as f:
        f.write(image)
    sys.path.insert(0, str(ROOT / "scripts"))
    from qc07f_boot import crc_bzip2
    bzip = f"{crc_bzip2(image):08x}"
    # Independently execute only an OWN host CRC checker with the selected kernel
    # CRC implementation; no target executable or firmware input is run on host.
    checker = OUT / "image_crc.c"
    checker.write_text('#include <stdio.h>\n#include <stdlib.h>\n#include "crc_ram01.h"\nint main(int n,char**v){if(n!=2)return 2;FILE*f=fopen(v[1],"rb");if(!f)return 3;fseek(f,0,SEEK_END);long z=ftell(f);rewind(f);if(z<=0)return 4;unsigned char*p=malloc((size_t)z);if(!p)return 5;if(fread(p,1,(size_t)z,f)!=(size_t)z)return 6;fclose(f);printf("%08x\\n",crc_ram01_static4(p,(size_t)z));free(p);return 0;}\n')
    run("CRC-host-compile", ["clang", "-std=c11", "-Wall", "-Wextra", "-Werror", "-O2", "-I" + str(src / "arch/arm/mach-ak3918ev200"), checker, "-o", OUT / "image_crc"], 30)
    assert run("CRC-host-run", [OUT / "image_crc", unverified_path], 30).strip() == bzip
    assert all(sha(ROOT / path) == value for path, value in first["output_sha256"].items())
    assert all(sha(Path(path)) == value for path, value in original.items())
    # Do not create the named candidate image until all substantive validation
    # passed. Consumers still require the successful final manifest and verifier.
    with image_path.open("xb") as f:
        f.write(image)
    outputs = [image_path, initramfs, final_radio, build / ".config", build / "Module.symvers", build / "vmlinux", build / "arch/arm/boot/zImage", build / "drivers/usb/musb/musb_host.o"]
    result = {"utc": datetime.datetime.now(datetime.timezone.utc).isoformat(), "state": "RAM_CANDIDATE76_PACKAGED_NOT_BOOTED",
              "image": str(image_path.relative_to(ROOT)), "bytes": len(image), "image_sha256": sha(image_path),
              "native_expected_fullimage_BZIP2": bzip, "independent_host_CRC_matches": True,
              "staging_range": [hex(0x82408000), hex(0x82408000 + len(image))], "decompressed_range": [hex(text_phys), hex(end_phys)],
              "source_pin": prep["source_pin"], "only_kernel_source_change": prep["only_changed_kernel_file"], "config_delta": delta,
              "entry_replacements": sorted(replacements), "entry_additions": sorted(additions), "entry_details": entries,
              "all_other_cpio_records_byte_exact": True, "retained_init_monitor_unchanged": True,
              "actual_final_build_embedded_cpio_verified": True, "final_output_rebuilt_radio_byte_identical_to_embedded": True,
              "baseline75_and_first76_build_outputs_unchanged": True, "syscall_prerequisites_linked": len(syscall_matrix), "BPF_admin_syscall_stays_ENOSYS": True,
              "new_autostart_or_credentials_added": False, "GUI_controls_admitted": False,
              "MTU1500_qualified": False, "kernel76_booted": False, "target_operation": False, "product_gate_pass": False,
              "requires_volatile_CMA_bootargs_for_future_media": "cma=16M@0x82800000-0x83800000",
              "output_sha256": {str(path.relative_to(ROOT)): sha(path) for path in outputs},
              "review_pins": {str(path.relative_to(ROOT)): sha(path) for path in reviews},
              "recipe_sha256": sha(Path(__file__)), "cpio_transform_sha256": sha(HERE / "cpio.py"), "commands": records, "environment": env}
    with (OUT / "package76-manifest.json").open("x") as f:
        json.dump(result, f, indent=2); f.write("\n")
    print(json.dumps({key: result[key] for key in ("state", "image", "bytes", "image_sha256", "staging_range", "decompressed_range")}))


if __name__ == "__main__":
    main()
