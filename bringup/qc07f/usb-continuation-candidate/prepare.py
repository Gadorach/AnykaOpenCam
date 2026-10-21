#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Fresh isolated kernel76 source/config. Never edits75 or accesses hardware."""
import datetime
import difflib
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

if not __debug__:
    raise SystemExit("Candidate safety checks require assertions; do not use Python -O")

ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent
FOUND = ROOT / "work/lightbulb-camera/foundation-20261011T041148Z"
OLD = FOUND / "artifacts/native-security75-source01"
OLD_BUILD = FOUND / "analysis/native-security75-build01"
ART = FOUND / "artifacts/native-usb76-source01"
OUT = FOUND / "analysis/native-usb76-build01"
REL = "drivers/usb/musb/musb_host.c"
PIN = "725bd2f3c81d54edccb66fead01d4c0c222e2231"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def verify(src, files):
    for name, info in files.items():
        rel = Path(name)
        assert not rel.is_absolute() and ".." not in rel.parts
        path = src / rel
        if "symlink" in info:
            assert path.is_symlink() and os.readlink(path) == info["symlink"], name
        else:
            assert path.is_file() and not path.is_symlink() and sha(path) == info["sha256"], name


def copy(src, dst):
    dst.parent.mkdir(parents=True, exist_ok=True)
    with dst.open("xb") as f:
        f.write(src.read_bytes())


def main():
    os.umask(0o077)
    ART.mkdir(exist_ok=False); OUT.mkdir(exist_ok=False)
    inputs = ART / "inputs"; inputs.mkdir()
    log = OUT / "prepare01"; log.mkdir(); (log / "home").mkdir(); (log / "tmp").mkdir()
    env = {"PATH": "/usr/bin:/bin", "HOME": str(log / "home"), "TMPDIR": str(log / "tmp"),
           "LANG": "C", "LC_ALL": "C", "PYTHONDONTWRITEBYTECODE": "1",
           "KBUILD_BUILD_TIMESTAMP": "2026-10-08 00:00:00 UTC", "KBUILD_BUILD_USER": "qc07f", "KBUILD_BUILD_HOST": "offline-bringup"}
    records = []

    def run(name, argv, cwd=ROOT, timeout=300):
        entry = {"start": datetime.datetime.now(datetime.timezone.utc).isoformat(), "argv": list(map(str, argv)), "cwd": str(cwd)}
        result = subprocess.run(entry["argv"], cwd=cwd, env=env, capture_output=True, timeout=timeout)
        (log / (name + ".stdout")).write_bytes(result.stdout)
        (log / (name + ".stderr")).write_bytes(result.stderr)
        entry.update(exit=result.returncode, end=datetime.datetime.now(datetime.timezone.utc).isoformat())
        records.append(entry)
        with (log / "commands.jsonl").open("a") as f:
            f.write(json.dumps(entry) + "\n")
        assert result.returncode == 0, name
        return result.stdout

    inventory = json.loads((OLD_BUILD / "complete-source75-inventory.json").read_text())["files"]
    assert len(inventory) == 91268
    verify(OLD / "linux", inventory)
    assert run("head75", ["git", "--no-optional-locks", "-C", OLD / "linux", "rev-parse", "HEAD"]).decode().strip() == PIN
    delta = run("source75-delta", ["git", "--no-optional-locks", "-C", OLD / "linux", "diff", "--no-ext-diff", "--binary", "HEAD", "--"])
    (inputs / "source75-retained.patch").write_bytes(delta)
    other = run("source75-untracked", ["git", "--no-optional-locks", "-C", OLD / "linux", "ls-files", "--others", "--exclude-standard", "-z"]).decode().split("\0")
    other = [x for x in other if x]
    assert all(x in inventory for x in other)
    src = ART / "linux"
    run("clone76", ["git", "-c", "gc.auto=0", "clone", "--no-hardlinks", "--no-checkout", OLD / "linux", src])
    run("checkout76", ["git", "-C", src, "checkout", "--detach", PIN])
    run("apply75-check", ["git", "-C", src, "apply", "--check", inputs / "source75-retained.patch"])
    run("apply75", ["git", "-C", src, "apply", inputs / "source75-retained.patch"])
    for name in other:
        target = src / name
        target.parent.mkdir(parents=True, exist_ok=True)
        if "symlink" in inventory[name]:
            target.symlink_to(inventory[name]["symlink"])
        else:
            copy(OLD / "linux" / name, target)
    verify(src, inventory)
    # Only one kernel source file changes, from a pinned exact input.
    old = (src / REL).read_text()
    helper = (HERE / "pio_kick.c.inc").read_text()
    marker = "static inline void musb_h_tx_dma_start(struct musb_hw_ep *ep)\n"
    assert old.count(marker) == 1
    new = old.replace(marker, helper + "\n" + marker, 1)
    nak = "\t\t\tmusb_writew(epio, MUSB_TXCSR,\n\t\t\t\t\tMUSB_TXCSR_H_WZC_BITS\n\t\t\t\t\t| MUSB_TXCSR_TXPKTRDY);"
    continuation = "\tmusb_writew(epio, MUSB_TXCSR,\n\t\t\tMUSB_TXCSR_H_WZC_BITS | MUSB_TXCSR_TXPKTRDY);"
    assert new.count(nak) == 1 and new.count(continuation) == 1
    new = new.replace(nak, "\t\t\tmusb_h_tx_pio_kick(musb, epio);", 1)
    new = new.replace(continuation, "\tmusb_h_tx_pio_kick(musb, epio);", 1)
    (src / REL).write_text(new)
    patch = "".join(difflib.unified_diff(old.splitlines(True), new.splitlines(True), fromfile="a/" + REL, tofile="b/" + REL))
    (inputs / "usb76-only.patch").write_text(patch)
    changed = {name: sha(src / name) for name, info in inventory.items()
               if "sha256" in info and sha(src / name) != info["sha256"]}
    assert set(changed) == {REL}
    # Original rollback build outputs must match their retained manifest.
    retained = json.loads((OLD_BUILD / "build75-manifest.json").read_text())["output_sha256"]
    assert all(sha(Path(path)) == value for path, value in retained.items())
    for name in ("attempt45.dtb", "initramfs50.cpio"):
        copy(OLD / "inputs" / name, inputs / name)
    sealed_radio = json.loads((OLD_BUILD / "prepare75-manifest.json").read_text())["radio_source75_exact74"]
    radio = {"station75/" + name: value for name, value in sealed_radio.items()}
    assert len(radio) > 30
    for name, value in radio.items():
        assert sha(OLD / name) == value
        copy(OLD / name, ART / "station76" / Path(name).relative_to("station75"))
    build = OUT / "build"; build.mkdir()
    copy(OLD_BUILD / "build/.config", build / ".config")
    config_pin = sha(build / ".config")
    assert config_pin == "22acc5e770af2d6d4804ca8f64cd7e9ffe71f69f2e5c8a0fa455a23dc9217bf8"
    config = (build / ".config").read_text()
    assert "CONFIG_MUSB_PIO_ONLY=y" in config and "# CONFIG_PM is not set" in config
    for tool in ("clang", "ld.lld", "make", "git", "llvm-objdump", "dtc"):
        run("tool-" + tool, [tool, "--version"])
    base = ["make", "-C", src, "O=" + str(build), "ARCH=arm", "LLVM=1"]
    run("olddefconfig76", base + ["olddefconfig"])
    assert sha(build / ".config") == config_pin
    run("prepare76", base + ["-j4", "prepare"])
    verify(OLD / "linux", inventory)
    manifest = {"utc": datetime.datetime.now(datetime.timezone.utc).isoformat(), "source_pin": PIN,
                "source75_files_verified": len(inventory), "only_changed_kernel_file": changed,
                "config_sha256": config_pin, "config_delta": {}, "environment": env, "commands": records,
                "paired_radio_source75_pins": radio,
                "retained_input_hashes": {name: sha(inputs / name) for name in ("attempt45.dtb", "initramfs50.cpio", "usb76-only.patch", "source75-retained.patch")},
                "recipe_sha256": sha(Path(__file__)), "helper_sha256": sha(HERE / "pio_kick.c.inc"),
                "target_operation": False, "kernel76_booted": False, "rollback75_unchanged": True}
    (OUT / "prepare76-manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps({"state": "SOURCE76_PREPARED_NOT_BUILT", "source_files_verified": len(inventory),
                      "changed": list(changed), "manifest": str(OUT / "prepare76-manifest.json")}))


if __name__ == "__main__":
    main()
