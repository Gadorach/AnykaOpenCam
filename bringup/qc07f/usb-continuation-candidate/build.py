#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Build isolated kernel76 and paired station76, never install or run target code."""
import datetime
import hashlib
import json
from pathlib import Path
import subprocess
import sys

if not __debug__:
    raise SystemExit("Candidate safety checks require assertions; do not use Python -O")

ROOT = Path(__file__).resolve().parents[3]
FOUND = ROOT / "work/lightbulb-camera/foundation-20261011T041148Z"
ART = FOUND / "artifacts/native-usb76-source01"
OUT = FOUND / "analysis/native-usb76-build01"
OLD = FOUND / "analysis/native-security75-build01"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    audit = json.loads((OUT / "prepare76-manifest.json").read_text())
    assert audit["source75_files_verified"] == 91268 and audit["config_delta"] == {}
    src = ART / "linux"; build = OUT / "build"
    log = OUT / "compile01"; log.mkdir(exist_ok=False); (log / "home").mkdir(); (log / "tmp").mkdir()
    env = dict(audit["environment"], HOME=str(log / "home"), TMPDIR=str(log / "tmp"))
    for rel, expected in audit["only_changed_kernel_file"].items():
        assert sha(src / rel) == expected
    assert sha(build / ".config") == audit["config_sha256"]
    for name in ("attempt45.dtb", "initramfs50.cpio"):
        assert sha(ART / "inputs" / name) == audit["retained_input_hashes"][name]
    old = json.loads((OLD / "build75-manifest.json").read_text())["output_sha256"]
    assert all(sha(Path(path)) == value for path, value in old.items())
    records = []

    def run(name, argv, timeout=1800):
        command = {"start": datetime.datetime.now(datetime.timezone.utc).isoformat(), "argv": list(map(str, argv)), "cwd": str(ROOT)}
        print("START", name, flush=True)
        with (log / (name + ".log")).open("xb") as f:
            result = subprocess.run(command["argv"], cwd=ROOT, env=env, stdout=f, stderr=subprocess.STDOUT, timeout=timeout)
        command.update(exit=result.returncode, end=datetime.datetime.now(datetime.timezone.utc).isoformat())
        records.append(command)
        with (log / "commands.jsonl").open("a") as f:
            f.write(json.dumps(command) + "\n")
        print("END", name, result.returncode, flush=True)
        assert result.returncode == 0, name

    base = ["make", "-C", src, "O=" + str(build), "ARCH=arm", "LLVM=1"]
    state = "BUILD_HOLD"
    try:
        run("kernel76", base + ["-j4", "zImage", "modules"])
        assert sha(build / ".config") == audit["config_sha256"]
        run("station76", base + ["M=" + str(ART / "station76/hgic_smac"), "CONFIG_HGICS=m", "CONFIG_HGIC_USB=y", "CONFIG_HGIC_2G=y", "-j4", "modules"], 300)
        images = ART / "images"; images.mkdir(exist_ok=False)
        with (images / "hgics-linux6-station76.ko").open("xb") as f:
            f.write((ART / "station76/hgic_smac/hgics.ko").read_bytes())
        run("musb76-assembly", ["llvm-objdump", "-dr", "--disassemble-symbols=musb_host_tx", build / "drivers/usb/musb/musb_host.o"], 30)
        run("station76-modinfo", ["llvm-readelf", "-h", "-A", "-p", ".modinfo", images / "hgics-linux6-station76.ko"], 30)
        run("vmlinux76-ELF", ["llvm-readelf", "-h", "-l", "-S", build / "vmlinux"], 30)
        assert (build / "include/config/kernel.release").read_bytes() == (OLD / "build/include/config/kernel.release").read_bytes()
        assert sha(build / "Module.symvers") == sha(OLD / "build/Module.symvers")
        assert all(sha(Path(path)) == value for path, value in old.items())
        state = "BUILT_LINKED_NOT_PACKAGED_NOT_BOOTED"
    finally:
        outputs = [build / ".config", build / "Module.symvers", build / "vmlinux", build / "arch/arm/boot/zImage", build / "arch/arm/boot/Image",
                   build / "drivers/usb/musb/musb_host.o", ART / "images/hgics-linux6-station76.ko"]
        manifest = {"utc": datetime.datetime.now(datetime.timezone.utc).isoformat(), "state": state,
                    "commands": records, "environment": env, "recipe_sha256": sha(Path(__file__)),
                    "output_sha256": {str(path.relative_to(ROOT)): sha(path) for path in outputs if path.is_file()},
                    "packaging_review_required": True, "target_operation": False, "kernel76_booted": False,
                    "MTU1500_qualified": False, "baseline75_unchanged": all(sha(Path(path)) == value for path, value in old.items())}
        with (OUT / "build76-manifest.json").open("x") as f:
            json.dump(manifest, f, indent=2); f.write("\n")
    print("kernel76 + station76 built; not packaged/booted; no MTU acceptance", flush=True)


if __name__ == "__main__":
    main()
