#!/usr/bin/env python3
"""One parent-owned candidate76 RAM upload/boot. No radio, media or GPIO action."""
from pathlib import Path
import datetime
import fcntl
import hashlib
import json
import os
import sys
import traceback

if not __debug__:
    raise SystemExit("Assertions required")
ROOT = Path(__file__).resolve().parents[5]
sys.path.insert(0, str(ROOT / "scripts"))
import qc07f_private_root_transfer as private
b = private.boot
BASE = Path("/home/gadorach/.local/state/anyka-bench-ssh/execute-20261021T0407Z")
IMAGE = ROOT / "work/lightbulb-camera/foundation-20261011T041148Z/artifacts/native-usb76-source01/images/QC6U76.UIM"
PACKAGE = ROOT / "work/lightbulb-camera/foundation-20261011T041148Z/analysis/native-usb76-package01/package76-manifest.json"
FINAL = ROOT / "evidence/qualification/qc07f/kernel76-20261020/final-verify01/result.json"
SHA = "bd8a2a08be1483e1ddcd1efa2c68c9e20549047dec79796137979179197f654f"
SIZE = 3603555
CRC = "58eb4a22"
BOOTARGS = "earlycon console=ttyAK0,115200n8 rdinit=/init loglevel=8 panic=0 cma=16M@0x82800000-0x83800000"
EXTRA = {"mw.l 0x80000e4 0x552aea54 1", "mw.l 0x80000e8 0xaa000003 1",
         "setenv bootargs '" + BOOTARGS + "'", "printenv bootargs",
         f"crc32 0x82408000 {SIZE:#x}", "bootm 0x82408000"}


class RAM76(private.PrivateBoot):
    def command(self, command, **kwargs):
        if command in EXTRA:
            return b.Boot.command(self, command, **kwargs)
        return super().command(command, **kwargs)


def inputs():
    data, info = b.payload(IMAGE, SHA, True, boot=True)
    assert len(data) == SIZE and info["crc32_bzip2"] == CRC
    p = json.loads(PACKAGE.read_text()); v = json.loads(FINAL.read_text())
    assert p["state"] == "RAM_CANDIDATE76_PACKAGED_NOT_BOOTED"
    assert p["image_sha256"] == SHA and p["bytes"] == SIZE and p["native_expected_fullimage_BZIP2"] == CRC
    assert p["staging_range"] == ["0x82408000", "0x82777c63"]
    assert p["decompressed_range"] == ["0x81808000", "0x82059bc0"]
    assert v["all_pass"] is True and len(v["checks"]) == 20 and all(v["checks"].values())
    assert hashlib.sha256(PACKAGE.read_bytes()).hexdigest() == v["manifest_sha256"]
    assert all(hashlib.sha256((ROOT / path).read_bytes()).hexdigest() == digest for path, digest in p["output_sha256"].items())
    assert hashlib.sha256((b.IMPL / "uart_capture.py").read_bytes()).hexdigest() == b.RECORDER_SHA
    assert hashlib.sha256(b.LOADER.read_bytes()).hexdigest() == b.LOADER_SHA
    return data, info


def main():
    assert sys.argv[1:] in (["--dry-run"], ["--fresh21a-boot-only"])
    data, info = inputs()
    if sys.argv[1:] == ["--dry-run"]:
        print("RAM76 image/header/CRC/package/source gates PASS; no UART/device access")
        return
    cold = json.loads((Path(__file__).parent / "COLD21A01.json").read_text())
    assert cold["part"] == "foundation-20261021a" and cold["no_GO_RAM_loader_or_Linux_execution"] is True
    assert cold["original_bootloader_pin_matches"] and cold["neutral_mux_direction_latch_pair_IR_and_timers_match"]
    os.umask(0o077)
    with (b.CONTROL / "peripheral-gui-operation.lock").open("r+") as held:
        fcntl.flock(held, fcntl.LOCK_EX | fcntl.LOCK_NB)
        assert not held.read(1)
        instance = RAM76(BASE / "ram76-upload-boot01", BASE / "uart-capture21a", vendor_simple_parser=True)
        try:
            instance.caught(5)
            state = instance.owner(margin=5400)
            assert state["part"] == "foundation-20261021a"
            held.write(json.dumps({"state": "HOLD", "operation": "one_fixed_RAM76_cold21a_boot", "image_sha256": SHA, "flash_write": False}) + "\n")
            held.flush(); os.fsync(held.fileno())
            instance.pin_bootloader()
            print("Original cold U-Boot pin verified; starting one fixed RAM76 transfer", flush=True)
            instance.upload(data, info)
            print("Full RAM76 upload committed; checking independent RAM CRC and boot", flush=True)
            # Identical reviewed startup watchdog load/control as RAM75. Only
            # pinned /init magic-closes it; actual inactive state checked later.
            instance.command("mw.l 0x80000e4 0x552aea54 1")
            instance.command("mw.l 0x80000e8 0xaa000003 1")
            response = instance.command(f"crc32 0x82408000 {SIZE:#x}")
            assert b.crc_verified(response, 0x82408000, SIZE, CRC)
            start = instance.status()["rx_bytes"]
            instance.boot_image(0x82408000, BOOTARGS)
            response = instance.wait(b"qc07f> ", start, 45, reject_reset=True)
            assert b"Linux version 6.18.55-qc07f-prototype-dirty" in response
            assert b"USB qualification startup watchdog magic-close completed" in response
            instance.log("RAM76_actual_monitor_observed", image_sha256=SHA, no_radio_media_controls=True)
            checkpoint = instance.check_stream()
            with (BASE / "boot-checkpoint21a01.private.json").open("x") as f:
                os.chmod(f.name, 0o600)
                json.dump({"part": checkpoint["part"], "image_sha256": SHA,
                           "recorder_pid": instance.peer[0], "start_ticks": b.ticks(instance.peer[0]),
                           "rx_bytes": checkpoint["rx_bytes"], "tx_bytes": checkpoint["tx_bytes"],
                           "monitor_observed": True, "shell_started": False}, f, indent=2)
                f.write("\n")
            record = {"utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
                      "state": "RAM76_BOOT_AND_INERT_MONITOR_OBSERVED", "part": state["part"],
                      "image_sha256": SHA, "full_RAM_image_CRC_verified": True,
                      "startup_watchdog_magic_close_text_observed": True,
                      "manual_GPIO_stores": False, "radio_firmware_load_or_association": False,
                      "media_or_controls_started": False, "flash_ENV_SD_OTP_write": False,
                      "runtime_idle_watchdog_UART_inventory_pending": True,
                      "private_postboot_RX_TX_identity_checkpoint_retained": True,
                      "MTU1500_or_USB_payload_qualification": False,
                      "recipe_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}
            with (Path(__file__).parent / "RAM76_BOOT21A01.json").open("x") as f:
                json.dump(record, f, indent=2); f.write("\n")
            held.seek(0); held.truncate(); held.flush(); os.fsync(held.fileno())
            print("Actual candidate76 monitor observed; no radio/media/peripheral test started", flush=True)
        except BaseException:
            instance.log("HOLD_RAM76_boot_failed", automatic_retry=False)
            raise


if __name__ == "__main__":
    try:
        main()
    except Exception:
        with (BASE / "ram76-boot-error01.private.txt").open("x") as f:
            os.chmod(f.name, 0o600); traceback.print_exc(file=f)
        print("RAM76 HOLD; preserve capture/owner; no automatic boot retry", file=sys.stderr)
        raise SystemExit(1)
