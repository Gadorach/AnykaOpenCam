#!/usr/bin/env python3
"""Fixed read-only monitor inventory after the owned RAM76 boot completes."""
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
SHA = "bd8a2a08be1483e1ddcd1efa2c68c9e20549047dec79796137979179197f654f"
PROMPT = b"qc07f> "
BOOTARGS = "earlycon console=ttyAK0,115200n8 rdinit=/init loglevel=8 panic=0 cma=16M@0x82800000-0x83800000"
QUERIES = {
    "boot_id": "cat /proc/sys/kernel/random/boot_id",
    "pid1": "cat /proc/1/comm",
    "identity": "cat /proc/1/status",
    "release": "cat /proc/sys/kernel/osrelease",
    "cmdline": "cmdline", "modules": "cat /proc/modules",
    "mounts": "cat /proc/mounts", "memory": "mem", "iomem": "iomem",
    "ISP": "isp state", "watchdog": "cat /sys/class/watchdog/watchdog0/state",
    "nowayout": "cat /sys/class/watchdog/watchdog0/nowayout",
    "UART": "cat /sys/class/tty/ttyAK0/rx_state"}


def body(response, command):
    assert response.endswith(PROMPT) and response.count(PROMPT) == 1
    raw = response[:-len(PROMPT)].replace(b"\r", b"")
    lines = raw.split(b"\n")
    # TTY echo may be absent; it is never used as a completion witness. A fresh
    # monitor prompt after this command's TX is mandatory, and query outputs are
    # independently schema-checked before any result is accepted.
    while lines and lines[0] == b"": lines.pop(0)
    if lines and lines[0] == command.encode(): lines.pop(0)
    while lines and lines[-1] == b"": lines.pop()
    data = b"\n".join(lines).decode("ascii")
    assert not re.search(r"(?i)unknown command|open failed|read failed|not found|No such file|permission denied", data)
    return data


def parse(rows):
    assert rows["pid1"].strip() == "init"
    assert rows["release"].strip() == "6.18.55-qc07f-prototype-dirty"
    uid = re.findall(r"(?m)^Uid:[ \t]+(\d+)[ \t]+(\d+)[ \t]+(\d+)[ \t]+(\d+)[ \t]*$", rows["identity"])
    pid = re.findall(r"(?m)^Pid:[ \t]+(\d+)[ \t]*$", rows["identity"])
    cap = re.findall(r"(?m)^CapEff:[ \t]+([0-9a-f]{16})[ \t]*$", rows["identity"])
    assert uid == [("0", "0", "0", "0")] and pid == ["1"] and len(cap) == 1
    assert int(cap[0], 16) & (1 << 17)
    assert rows["cmdline"].strip() == BOOTARGS
    assert not rows["modules"].strip()
    mounts = {}
    for line in rows["mounts"].splitlines():
        parts = line.split()
        assert len(parts) == 6 and parts[1] not in mounts and parts[4:] == ["0", "0"]
        assert "rw" in parts[3].split(",") and "ro" not in parts[3].split(",")
        mounts[parts[1]] = (parts[0], parts[2])
    assert set(mounts) == {"/", "/dev", "/proc", "/sys"}
    assert mounts["/"][0] in {"rootfs", "ramfs", "tmpfs", "none"}
    assert mounts["/"][1] in {"rootfs", "ramfs", "tmpfs"}
    assert mounts["/dev"] == ("devtmpfs", "devtmpfs")
    assert mounts["/proc"] == ("proc", "proc") and mounts["/sys"] == ("sysfs", "sysfs")
    memory_pairs = re.findall(r"(?m)^(MemTotal|MemFree|CmaTotal|CmaFree):[ \t]+(\d+) kB$", rows["memory"])
    assert len(memory_pairs) == 4
    mem = dict(memory_pairs)
    assert set(mem) == {"MemTotal", "MemFree", "CmaTotal", "CmaFree"}
    mem = {key: int(value) for key, value in mem.items()}
    assert 24000 < mem["MemTotal"] <= 40960 and mem["CmaTotal"] == 16384
    assert mem["MemFree"] <= mem["MemTotal"] and mem["CmaFree"] <= mem["CmaTotal"]
    assert re.search(r"(?m)^81800000-83ffffff : System RAM$", rows["iomem"])
    isp_pairs = re.findall(r"(?:^|[\s;])([A-Za-z_][A-Za-z0-9_]*)=([^\s;]+)", rows["ISP"])
    isp = dict(isp_pairs)
    assert len(isp_pairs) == len(isp), "duplicate ISP fields"
    for key, expected in {"result": "-61", "readable": "0", "guard_errors": "0", "irq_mode": "0", "irq_frames": "0",
                          "queue_mode": "0", "queue_started": "0", "queue_held": "0", "wb_owner_active": "0"}.items():
        assert isp.get(key) == expected, key
    assert rows["watchdog"].strip() == "inactive" and rows["nowayout"].strip() == "0"
    uart = re.fullmatch(r"rx_error_events=(\d+) mem_busy_ticks=(\d+) last_status=[0-9a-f]{8} rxpos=\d+; cached diagnostic, lost bytes cannot be reconstructed", rows["UART"].strip())
    assert uart and uart.group(1) == uart.group(2) == "0"
    assert re.fullmatch(r"[0-9a-f]{8}(?:-[0-9a-f]{4}){3}-[0-9a-f]{12}", rows["boot_id"].strip())
    assert rows["boot_id"].strip() == rows["boot_id_after"].strip()
    return mem


def checked_state(instance, expected_tx):
    state = instance.check_stream()
    assert state["part"] == "foundation-20261021a" and state["tx_busy"] is False
    assert type(state["tx_bytes"]) is int and state["tx_bytes"] == expected_tx
    assert type(state["rx_bytes"]) is int and state["rx_bytes"] >= 0
    return state


def exchange(instance, wire, expected_tx):
    assert 0 < len(wire) <= 64 and wire.endswith(b"\r")
    assert wire == b"\r" or wire[:-1].decode("ascii") in QUERIES.values()
    state = checked_state(instance, expected_tx)
    if wire != b"\r":
        assert state["recent_rx"].endswith("qc07f> ")
    start = state["rx_bytes"]
    instance.send(wire, "Fixed read-only RAM76 monitor inventory/refresh")
    expected_tx += len(wire)
    answer = instance.wait(PROMPT, start, 6, reject_reset=True)
    after = checked_state(instance, expected_tx)
    assert after["rx_bytes"] >= start + len(answer)
    return answer, expected_tx


def main():
    assert sys.argv[1:] == ["--owned-RAM76-boot-completed"]
    os.umask(0o077)
    with (b.CONTROL / "peripheral-gui-operation.lock").open("r+") as held:
        fcntl.flock(held, fcntl.LOCK_EX | fcntl.LOCK_NB)
        assert not held.read(1)
        held.write(json.dumps({"state": "HOLD", "operation": "RAM76_readonly_postcheck21a"}) + "\n")
        held.flush(); os.fsync(held.fileno())
        instance = None
        try:
            receipt = json.loads((Path(__file__).parent / "RAM76_BOOT21A01.json").read_text())
            assert receipt["state"] == "RAM76_BOOT_AND_INERT_MONITOR_OBSERVED" and receipt["image_sha256"] == SHA
            checkpoint = json.loads(private.private_path(BASE / "boot-checkpoint21a01.private.json").read_text())
            assert checkpoint["image_sha256"] == SHA and checkpoint["monitor_observed"] is True and checkpoint["shell_started"] is False
            instance = private.PrivateBoot(BASE / "postcheck21a01", BASE / "uart-capture21a")
            state = instance.owner(margin=300)
            assert state["part"] == "foundation-20261021a" and not state["tx_busy"]
            assert state["part"] == checkpoint["part"] and instance.peer[0] == checkpoint["recorder_pid"]
            assert b.ticks(instance.peer[0]) == checkpoint["start_ticks"] and state["tx_bytes"] == checkpoint["tx_bytes"]
            assert type(checkpoint["tx_bytes"]) is int and checkpoint["tx_bytes"] >= 0
            expected_tx = checkpoint["tx_bytes"]
            assert state["rx_bytes"] >= checkpoint["rx_bytes"] and "qc07f> " in state["recent_rx"]
            since = instance.raw_since(checkpoint["rx_bytes"], state)
            assert b.BANNER not in since and b"Linux version" not in since
            instance.epoch_cursor = checkpoint["rx_bytes"]
            state = checked_state(instance, expected_tx)
            refresh = False
            if not state["recent_rx"].endswith("qc07f> "):
                # Exactly one empty monitor line after observed own boot, never
                # a repeated effect/command or an assumed stock-shell refresh.
                answer, expected_tx = exchange(instance, b"\r", expected_tx)
                assert answer.endswith(PROMPT) and answer.count(PROMPT) == 1
                refresh = True
            rows = {}
            for key, command in list(QUERIES.items()) + [("boot_id_after", QUERIES["boot_id"])]:
                wire = command.encode() + b"\r"
                answer, expected_tx = exchange(instance, wire, expected_tx)
                rows[key] = body(answer, command)
                instance.log("readonly_monitor_query", key=key, response=rows[key])
            mem = parse(rows)
            checked_state(instance, expected_tx)
            with (BASE / "runtime21a01.private.json").open("x") as f:
                os.chmod(f.name, 0o600); json.dump(rows, f, indent=2); f.write("\n")
            record = {"utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
                      "state": "RAM76_NATIVE_BOOT_READONLY_IDLE_POSTCHECK_PASS", "part": state["part"],
                      "image_sha256": SHA, "PID1_init_UIDs_zero_CAP_SYS_RAWIO": True,
                      "kernel_release": rows["release"].strip(), "boot_id_retained_private_and_stable": True,
                      "memory_KiB": mem, "ISP_idle_queues_guards_zero": True,
                      "watchdog_inactive_nowayout_zero": True, "modules_empty": True,
                      "no_flash_SD_mounts": True, "UART_errors_busy_zero": True,
                      "one_empty_monitor_refresh": refresh, "no_radio_media_peripheral_or_persistent_write": True,
                      "exact_cumulative_TX_accounted_no_other_sender_observed": True,
                      "USB_large_payload_MTU1500_or_product_qualified": False,
                      "recipe_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}
            checked_state(instance, expected_tx)
            with (Path(__file__).parent / "RAM76_POSTCHECK21A01.json").open("x") as f:
                json.dump(record, f, indent=2); f.write("\n")
            held.seek(0); held.truncate(); held.flush(); os.fsync(held.fileno())
            print("RAM76 actual boot + read-only idle postcheck PASS; no radio/media/controls; not USB/MTU qualification")
        except BaseException:
            if instance is not None:
                instance.log("HOLD_RAM76_postcheck_failed", automatic_retry=False)
            raise


if __name__ == "__main__":
    try:
        main()
    except Exception:
        with (BASE / "postcheck-error01.private.txt").open("x") as f:
            os.chmod(f.name, 0o600); traceback.print_exc(file=f)
        print("RAM76 postcheck HOLD; no repeated query or target repair", file=sys.stderr)
        raise SystemExit(1)
