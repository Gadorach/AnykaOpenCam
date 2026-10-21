#!/usr/bin/env python3
"""Parent-only cold21a identity/neutral readback. No GO or GPIO store."""
from pathlib import Path
import datetime
import fcntl
import hashlib
import json
import os
import re
import sys
import traceback

if not __debug__:
    raise SystemExit("Assertions required")
ROOT = Path(__file__).resolve().parents[5]
sys.path.insert(0, str(ROOT / "scripts"))
import qc07f_private_root_transfer as private
b = private.boot
BASE = Path("/home/gadorach/.local/state/anyka-bench-ssh/execute-20261021T0407Z")
REGS = {"s74": 0x08000074, "s78": 0x08000078, "s7c": 0x0800007c,
        "dir": 0x20170000, "out": 0x2017000c, "in": 0x20170018,
        "t3": 0x080000c8, "t4": 0x080000d0}


class Cold(private.PrivateBoot):
    def command(self, command, **kwargs):
        if command in {f"md.l {value:#x} 1" for value in REGS.values()}:
            return b.Boot.command(self, command, **kwargs)
        return super().command(command, **kwargs)


def main():
    assert sys.argv[1:] == ["--user-ON-fresh21a"]
    os.umask(0o077)
    with (b.CONTROL / "peripheral-gui-operation.lock").open("r+") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        assert not lock.read(1)
        instance = Cold(BASE / "cold21a01", BASE / "uart-capture21a", vendor_simple_parser=True)
        try:
            instance.caught(5)
            state = instance.owner(margin=7200)
            assert state["part"] == "foundation-20261021a"
            lock.write(json.dumps({"state": "HOLD", "operation": "RAM76_cold21a_readonly_gate"}) + "\n")
            lock.flush(); os.fsync(lock.fileno())
            instance.pin_bootloader()
            values = {}
            for name, address in REGS.items():
                response = instance.command(f"md.l {address:#x} 1")
                rows = re.findall(rb"(?m)^0?" + f"{address:x}".encode() + rb":\s+([0-9a-fA-F]{8})", response)
                assert len(rows) == 1
                values[name] = int(rows[0], 16)
            assert values["s74"] == 0x02000006 and values["s78"] == values["s7c"] == 0
            assert values["dir"] == values["out"] == 0 and values["in"] & 0x8000c0 == 0
            assert values["t3"] == values["t4"] == 0
            instance.log("cold21a_original_UBoot_neutral_readback_pass", registers=values, GPIO_stores=False)
            record = {"utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
                      "state": "FRESH21A_ORIGINAL_UBOOT_COLD_READONLY_GATE_PASS",
                      "part": state["part"], "original_bootloader_pin_matches": True,
                      "neutral_mux_direction_latch_pair_IR_and_timers_match": True,
                      "no_GO_RAM_loader_or_Linux_execution": True, "no_manual_GPIO_store": True,
                      "no_flash_ENV_card_write": True, "recipe_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}
            with (Path(__file__).parent / "COLD21A01.json").open("x") as f:
                json.dump(record, f, indent=2); f.write("\n")
            lock.seek(0); lock.truncate(); lock.flush(); os.fsync(lock.fileno())
            print("Fresh21a original U-Boot and neutral readback verified; no GO/Linux/GPIO store")
        except BaseException:
            instance.log("HOLD_cold21a_failed", automatic_retry=False)
            raise


if __name__ == "__main__":
    try:
        main()
    except Exception:
        with (BASE / "cold21a-error01.private.txt").open("x") as f:
            os.chmod(f.name, 0o600); traceback.print_exc(file=f)
        print("Cold21a HOLD; no automatic retry", file=sys.stderr)
        raise SystemExit(1)
