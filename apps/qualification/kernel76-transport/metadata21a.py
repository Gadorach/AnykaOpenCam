#!/usr/bin/env python3
"""Fixed read-only PID1/USB metadata roundtrip via owned RAM76 shell; no RF IO."""
from pathlib import Path
import datetime
import fcntl
import importlib.util
import hashlib
import json
import os
import re
import sys
import time
import traceback
import uuid

if not __debug__: raise SystemExit("Assertions required")
ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "scripts"))
import qc07f_private_root_transfer as private
import radio_receipt
b = private.boot
BASE = Path("/home/gadorach/.local/state/anyka-bench-ssh/execute-20261021T0407Z")
EVIDENCE = ROOT / "evidence/qualification/qc07f/execution-20261021/ram76-resume21a"
SHA = "bd8a2a08be1483e1ddcd1efa2c68c9e20549047dec79796137979179197f654f"
PART = "foundation-20261021a"
MONITOR = b"qc07f> "
ATTRS = ("idVendor", "idProduct", "bConfigurationValue", "speed")
EPATTRS = ("bEndpointAddress", "bmAttributes", "wMaxPacketSize")


def admitted_handoff():
    boot = json.loads((EVIDENCE / "RAM76_BOOT21A01.json").read_text())
    post = json.loads((EVIDENCE / "RAM76_POSTCHECK21A01.json").read_text())
    assert boot["state"] == "RAM76_BOOT_AND_INERT_MONITOR_OBSERVED" and boot["part"] == PART
    assert post["state"] == "RAM76_NATIVE_BOOT_READONLY_IDLE_POSTCHECK_PASS" and post["part"] == PART
    assert boot["image_sha256"] == post["image_sha256"] == SHA
    assert boot["full_RAM_image_CRC_verified"] is True
    assert type(post["one_empty_monitor_refresh"]) is bool
    checkpoint = json.loads(private.private_path(BASE / "boot-checkpoint21a01.private.json").read_text())
    assert checkpoint["image_sha256"] == SHA and checkpoint["part"] == PART
    assert checkpoint["monitor_observed"] is True and checkpoint["shell_started"] is False
    for key in ("tx_bytes", "rx_bytes", "recorder_pid", "start_ticks"):
        assert type(checkpoint[key]) is int and checkpoint[key] >= 0
    assert checkpoint["recorder_pid"] > 1 and checkpoint["start_ticks"] > 0
    path = EVIDENCE / "postcheck.py"
    derivation = json.loads((EVIDENCE / "SOURCE_DERIVATION21A01.json").read_text())
    rows = [x for x in derivation["inputs"] if x["file"] == "postcheck.py"]
    assert len(rows) == 1 and rows[0]["exact_reversible_epoch_path_only_substitutions"] is True
    assert rows[0]["original_sha256"] == "035c106995b566b815a670748c1c84c99e49695a5e154c8d67314bf4304fa5e9"
    actual = hashlib.sha256(path.read_bytes()).hexdigest()
    assert actual == rows[0]["new_sha256"] == post["recipe_sha256"]
    spec = importlib.util.spec_from_file_location("post21a_readonly", path)
    module = importlib.util.module_from_spec(spec); spec.loader.exec_module(module)
    wires = list(module.QUERIES.values()) + [module.QUERIES["boot_id"]]
    assert len(wires) == 14 and sum(len(x.encode()) + 1 for x in wires) == 320
    expected_tx = checkpoint["tx_bytes"] + 320 + int(post["one_empty_monitor_refresh"])
    return module, boot, post, checkpoint, expected_tx


def script(nonce):
    assert re.fullmatch(r"[0-9a-f]{32}", nonce)
    cmd = (f"echo K76MD_BEGIN_{nonce}; echo FDS; /bin/busybox ls -l /proc/1/fd; r=$?; echo FD_RC:$r; "
           "for f in idVendor idProduct bConfigurationValue speed; do echo ATTR:$f; "
           "/bin/busybox cat /sys/bus/usb/devices/1-1/$f || r=1; done; "
           "for e in ep_01 ep_81; do for f in bEndpointAddress bmAttributes wMaxPacketSize; "
           "do echo EP:$e:$f; /bin/busybox cat /sys/bus/usb/devices/1-1:1.0/$e/$f || r=1; done; done; "
           "if test -L /sys/bus/usb/devices/1-1:1.0/driver; then r=1; fi; "
           f"echo K76MD_DONE_{nonce}:$r")
    assert len(cmd.encode()) <= 950 and not any(c in cmd for c in "\r\n\0")
    return cmd


def parse(text, nonce):
    lines = text.replace("\r", "").splitlines()
    begin, done = f"K76MD_BEGIN_{nonce}", f"K76MD_DONE_{nonce}:0"
    assert lines.count(begin) == 1 and lines.count(done) == 1
    start, end = lines.index(begin), lines.index(done)
    assert start < end and lines[start + 1] == "FDS"
    assert all(line == "" for line in lines[:start])
    assert lines[end + 1:] == ["# "]
    data = lines[start + 2:end]
    assert data.count("FD_RC:0") == 1
    split = data.index("FD_RC:0")
    result = radio_receipt.parse_parent_fds("\n".join(data[:split]))
    rest = data[split + 1:]
    expected = [("ATTR:" + key, value) for key, value in zip(ATTRS, ("a012", "8400", "1", "480"))]
    for ep, address in (("ep_01", "01"), ("ep_81", "81")):
        expected.extend((f"EP:{ep}:{key}", value) for key, value in zip(EPATTRS, (address, "02", "0200")))
    assert len(rest) == len(expected) * 2
    for i, (key, value) in enumerate(expected): assert rest[2 * i:2 * i + 2] == [key, value]
    return dict(result, owned_a012_8400_config1_speed480=True, bulk_OUT01_IN81_maxpacket512=True,
                no_bound_USB_interface_driver=True)


def state(instance, expected_tx):
    s = instance.check_stream()
    assert s["part"] == PART and s["tx_busy"] is False and type(s["tx_bytes"]) is int and s["tx_bytes"] == expected_tx
    return s


def fixed(instance, wire, expected_tx, marker, timeout):
    before = state(instance, expected_tx)
    instance.send(wire, "Fixed owned metadata shell/monitor roundtrip")
    expected_tx += len(wire)
    response = instance.wait(marker, before["rx_bytes"], timeout, reject_reset=True)
    after = state(instance, expected_tx)
    assert after["rx_bytes"] >= before["rx_bytes"] + len(response)
    return response, expected_tx


def main():
    assert sys.argv[1:] == ["--before-native-load"]
    phase = "before"
    os.umask(0o077)
    with (b.CONTROL / "peripheral-gui-operation.lock").open("r+") as held:
        fcntl.flock(held, fcntl.LOCK_EX | fcntl.LOCK_NB); assert not held.read(1)
        held.write(json.dumps({"state": "HOLD", "operation": "radio_metadata21a_" + phase}) + "\n")
        held.flush(); os.fsync(held.fileno()); instance = None
        try:
            module, boot, post, checkpoint, expected_tx = admitted_handoff()
            instance = private.PrivateBoot(BASE / ("radio-metadata-" + phase + "21a01"), BASE / "uart-capture21a")
            current = instance.owner(margin=1200)
            assert current["part"] == PART and current["tx_bytes"] == expected_tx and current["abort_armed"] is True
            assert instance.peer[0] == checkpoint["recorder_pid"] and b.ticks(instance.peer[0]) == checkpoint["start_ticks"]
            instance.epoch_cursor = checkpoint["rx_bytes"]
            current = state(instance, expected_tx)
            if not current["recent_rx"].endswith("qc07f> "):
                assert "qc07f> " in current["recent_rx"]
                answer, expected_tx = fixed(instance, b"\r", expected_tx, MONITOR, 5)
                assert answer.endswith(MONITOR)
            response, expected_tx = fixed(instance, b"shell\r", expected_tx, b"# ", 6)
            assert re.search(rb"(?:^|\n)# $", response.replace(b"\r", b""))
            nonce = uuid.uuid4().hex; command = script(nonce)
            start = state(instance, expected_tx)["rx_bytes"]
            for i in range(0, len(command), 64):
                state(instance, expected_tx)
                chunk = command[i:i + 64].encode(); instance.send(chunk, "Fixed read-only shell metadata, echo before CR")
                expected_tx += len(chunk); state(instance, expected_tx)
                if i + 64 < len(command): time.sleep(.025)
            deadline = time.monotonic() + 3
            while True:
                current = state(instance, expected_tx)
                echo = re.sub(rb"\r+\n", b"", instance.raw_since(start, current))
                if echo == command.encode(): break
                assert command.encode().startswith(echo) and time.monotonic() < deadline
                time.sleep(.01)
            response, expected_tx = fixed(instance, b"\r", expected_tx, b"# ", 10)
            text = response.decode("ascii")
            assert re.search(r"(?:^|\n)# $", text.replace("\r", ""))
            result = parse(text, nonce)
            answer, expected_tx = fixed(instance, b"exit\r", expected_tx, MONITOR, 6)
            assert b"RAM-only shell returned to monitor" in answer and answer.endswith(MONITOR)
            current = state(instance, expected_tx)
            private_checkpoint = {"part": PART, "image_sha256": SHA, "recorder_pid": instance.peer[0],
                                  "start_ticks": b.ticks(instance.peer[0]), "tx_bytes": expected_tx,
                                  "rx_bytes": current["rx_bytes"], "phase": phase}
            with (BASE / ("radio-metadata-" + phase + "21a01.private-checkpoint.json")).open("x") as f:
                os.chmod(f.name, 0o600); json.dump(private_checkpoint, f, indent=2); f.write("\n")
            record = {"utc": datetime.datetime.now(datetime.timezone.utc).isoformat(), "state": "READONLY_RADIO_METADATA_PASS",
                      "phase": phase, "result": result, "image_sha256": SHA,
                      "no_USB_vendor_transaction_firmware_or_radio_command": True,
                      "owned_shell_returned_and_reaped_by_monitor": True}
            state(instance, expected_tx)
            with (EVIDENCE / ("RADIO_META_" + phase.upper() + "21A01.json")).open("x") as f:
                json.dump(record, f, indent=2); f.write("\n")
            held.seek(0); held.truncate(); held.flush(); os.fsync(held.fileno())
            print("Owned read-only radio metadata PASS; no firmware/RF command")
        except BaseException:
            if instance: instance.log("HOLD_radio_metadata21a", phase=phase, automatic_retry=False)
            raise


if __name__ == "__main__":
    try: main()
    except Exception:
        with (BASE / "radio-metadata-error21a.private.txt").open("x") as f:
            os.chmod(f.name, 0o600); traceback.print_exc(file=f)
        print("Radio metadata HOLD; no repeat/repair", file=sys.stderr); raise SystemExit(1)
