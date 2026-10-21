#!/usr/bin/env python3
"""One finite full native-load/RUN trial with same-owner pre/post retirement."""
from pathlib import Path
import datetime
import fcntl
import hashlib
import importlib.util
import json
import os
import re
import sys
import time
import traceback
import uuid
import metadata21a as meta
import radio_receipt as receipt

if not __debug__: raise SystemExit("Assertions required")
b, private, ROOT, BASE, EVIDENCE, SHA = meta.b, meta.private, meta.ROOT, meta.BASE, meta.EVIDENCE, meta.SHA
HCD_DIAGNOSTIC = re.compile(
    rb"could not flush|rx\d+, packet/\d+ ready\?|broken !rx_reinit|bogus ep0 stage|"
    rb"no URB for end 0|RX\d+ dma busy, csr|Rx interrupt with no errors or packet!|"
    rb"trying to suspend as [^\r\n]* while active|(?:usb|musb)[^\r\n]*(?:error|warning|timeout)|"
    rb"bogus (?:host|peripheral) RESUME \([^\r\n]*\)|unhandled DISCONNECT transition \([^\r\n]*\)|"
    rb"musb[^\r\n]*: Babble(?:\r?\n|$)|"
    rb"WARNING:|BUG:|Oops:|Kernel panic", re.IGNORECASE)


def load_post():
    return meta.admitted_handoff()[0]


def clean_diagnostics(instance, interval_start, expected_tx):
    current = meta.state(instance, expected_tx)
    assert not HCD_DIAGNOSTIC.search(instance.raw_since(interval_start, current))
    return current


def monitor(instance, command, expected_tx, timeout=6, diagnostic_start=None):
    allowed = {"radio native-load", "radio state", "radio info"}
    post = load_post()
    assert command in allowed or command in post.QUERIES.values()
    current = meta.state(instance, expected_tx)
    assert current["recent_rx"].endswith("qc07f> ")
    if command in allowed:
        assert diagnostic_start is not None
        clean_diagnostics(instance, diagnostic_start, expected_tx)
    answer, expected_tx = meta.fixed(instance, command.encode() + b"\r", expected_tx, meta.MONITOR, timeout)
    if command in allowed:
        clean_diagnostics(instance, diagnostic_start, expected_tx)
    return post.body(answer, command), expected_tx


def metadata_roundtrip(instance, expected_tx):
    current = meta.state(instance, expected_tx)
    assert current["recent_rx"].endswith("qc07f> ")
    answer, expected_tx = meta.fixed(instance, b"shell\r", expected_tx, b"# ", 6)
    assert re.search(rb"(?:^|\n)# $", answer.replace(b"\r", b""))
    nonce = uuid.uuid4().hex; command = meta.script(nonce)
    start = meta.state(instance, expected_tx)["rx_bytes"]
    for i in range(0, len(command), 64):
        meta.state(instance, expected_tx)
        chunk = command[i:i + 64].encode(); instance.send(chunk, "Fixed read-only parentFD/USB metadata")
        expected_tx += len(chunk); meta.state(instance, expected_tx)
        if i + 64 < len(command): time.sleep(.025)
    deadline = time.monotonic() + 3
    while True:
        current = meta.state(instance, expected_tx)
        echo = re.sub(rb"\r+\n", b"", instance.raw_since(start, current))
        if echo == command.encode(): break
        assert command.encode().startswith(echo) and time.monotonic() < deadline
        time.sleep(.01)
    answer, expected_tx = meta.fixed(instance, b"\r", expected_tx, b"# ", 10)
    text = answer.decode("ascii").replace("\r", "")
    assert re.search(r"(?:^|\n)# $", text)
    assert text.splitlines().count(f"K76MD_BEGIN_{nonce}") == 1
    assert len(re.findall(r"(?m)^K76MD_DONE_" + nonce + r":\d+$", text)) == 1
    # A complete owned read-only shell transaction may return to the monitor
    # before its metadata is accepted. This is child retirement, not hardware
    # repair or permission to clear a failed metadata HOLD.
    answer, expected_tx = meta.fixed(instance, b"exit\r", expected_tx, meta.MONITOR, 6)
    assert b"RAM-only shell returned to monitor" in answer and answer.endswith(meta.MONITOR)
    values = meta.parse(text, nonce)
    return values, expected_tx


def idle_inventory(instance, expected_tx, post):
    rows = {}
    for key, command in list(post.QUERIES.items()) + [("boot_id_after", post.QUERIES["boot_id"])]:
        rows[key], expected_tx = monitor(instance, command, expected_tx)
    memory = post.parse(rows)
    return rows, memory, expected_tx


def main():
    assert sys.argv[1:] == ["--one-whole-native-load-and-RUN21a"]
    os.umask(0o077)
    with (b.CONTROL / "peripheral-gui-operation.lock").open("r+") as held:
        fcntl.flock(held, fcntl.LOCK_EX | fcntl.LOCK_NB); assert not held.read(1)
        held.write(json.dumps({"state": "HOLD", "operation": "one_radio_native_load_RUN21a"}) + "\n")
        held.flush(); os.fsync(held.fileno()); instance = None
        try:
            post, boot, idle, checkpoint, expected_tx = meta.admitted_handoff()
            original_runtime = json.loads(private.private_path(BASE / "runtime21a01.private.json").read_text())
            image = ROOT / "work/lightbulb-camera/foundation-20261011T041148Z/artifacts/native-usb76-source01/images/QC6U76.UIM"
            assert hashlib.sha256(image.read_bytes()).hexdigest() == SHA
            binding = json.loads((ROOT / "docs/qualification-plans/kernel76-transport-20261021/source-review/entry-bind.json").read_text())
            assert binding["image_sha256"] == SHA and binding["embedded_cpio_equals_final_selected_cpio"]
            assert binding["embedded_init_equals_attempt50_cpio_and_retained_init"]
            assert all(binding["private_firmware_admission_booleans_only"].values())
            for name, pin in binding["source_pins"].items():
                assert pin["matches_build50_input"] and hashlib.sha256((ROOT / name).read_bytes()).hexdigest() == pin["sha256"]
            instance = private.PrivateBoot(BASE / "radio-native-load21a01", BASE / "uart-capture21a")
            current = instance.owner(margin=1800)
            assert current["part"] == meta.PART and current["tx_bytes"] == expected_tx and current["abort_armed"] is True
            assert instance.peer[0] == checkpoint["recorder_pid"] and b.ticks(instance.peer[0]) == checkpoint["start_ticks"]
            original_peer = instance.peer
            original_start = checkpoint["start_ticks"]
            instance.epoch_cursor = checkpoint["rx_bytes"]
            current = meta.state(instance, expected_tx)
            interval_start = current["rx_bytes"]
            if not current["recent_rx"].endswith("qc07f> "):
                assert "qc07f> " in current["recent_rx"]
                answer, expected_tx = meta.fixed(instance, b"\r", expected_tx, meta.MONITOR, 5)
                assert answer.endswith(meta.MONITOR)
            before_rows, before_memory, expected_tx = idle_inventory(instance, expected_tx, post)
            assert before_rows["boot_id"].strip() == original_runtime["boot_id"].strip()
            before_metadata, expected_tx = metadata_roundtrip(instance, expected_tx)
            refreshed = instance.owner(margin=1800)
            assert instance.peer == original_peer and b.ticks(original_peer[0]) == original_start
            assert refreshed["part"] == meta.PART and refreshed["abort_armed"] is True and refreshed["tx_busy"] is False
            meta.state(instance, expected_tx)
            clean_diagnostics(instance, interval_start, expected_tx)
            print("Fresh owned idle/descriptor/FD admission passed; one native-load with automatic RAM RUN", flush=True)
            raw, expected_tx = monitor(instance, "radio native-load", expected_tx, timeout=600, diagnostic_start=interval_start)
            parsed = receipt.parse_loader(raw)
            instance.log("native_load_complete_transcript", transcript=raw, summary=parsed)
            # Passive settling is not proof of readiness; following actual
            # descriptor/FD/identity/inventory checks remain mandatory.
            time.sleep(2)
            current = meta.state(instance, expected_tx)
            if not current["recent_rx"].endswith("qc07f> "):
                answer, expected_tx = meta.fixed(instance, b"\r", expected_tx, meta.MONITOR, 5)
                assert answer.endswith(meta.MONITOR)
            state_text, expected_tx = monitor(instance, "radio state", expected_tx, diagnostic_start=interval_start)
            assert receipt.parse_application_state(state_text)
            info_text, expected_tx = monitor(instance, "radio info", expected_tx, diagnostic_start=interval_start)
            identity = receipt.parse_firmware_info(info_text)
            instance.log("application_identity_raw_private", state=state_text, info=info_text)
            after_metadata, expected_tx = metadata_roundtrip(instance, expected_tx)
            after_rows, after_memory, expected_tx = idle_inventory(instance, expected_tx, post)
            assert before_rows["boot_id"].strip() == after_rows["boot_id"].strip()
            final = meta.state(instance, expected_tx)
            clean_diagnostics(instance, interval_start, expected_tx)
            record = {"utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
                      "state": "RADIO76_FUNCTIONAL_NATIVE_FULLSIZE_LOAD_IDENTITY_RETIREMENT_PASS",
                      "image_sha256": SHA, "part": meta.PART, "loader": parsed, "identity": identity,
                      "before_metadata": before_metadata, "after_metadata": after_metadata,
                      "memory_before_KiB": before_memory, "memory_after_KiB": after_memory,
                      "watchdog_inactive_ISP_idle_no_modules_or_persistent_mounts_UART_clean": True,
                      "same_boot_UUID_private": True, "exact_TX_no_other_sender_observed": True,
                      "no_new_HCD_error_FIFO_warning_observed_in_owned_interval": True,
                      "verification_queries_reset_endpoints_ONLY_AFTER_native_load": True,
                      "no_256_fallback_GPIO42_reset_module_association_or_MTU_change": True,
                      "direct_MODE_NAK_toggle_or_MTU1500_qualification": False,
                      "recipe_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}
            with (BASE / "radio-trial21a01.private.json").open("x") as f:
                os.chmod(f.name, 0o600); json.dump({"before": before_rows, "after": after_rows,
                    "tx_bytes": expected_tx, "rx_bytes": final["rx_bytes"]}, f, indent=2); f.write("\n")
            meta.state(instance, expected_tx)
            with (EVIDENCE / "RADIO_NATIVE76_21A01.json").open("x") as f:
                json.dump(record, f, indent=2); f.write("\n")
            held.seek(0); held.truncate(); held.flush(); os.fsync(held.fileno())
            print("One native full-size radio load/application/idle retirement PASS; no MTU/network claim", flush=True)
        except BaseException:
            if instance: instance.log("HOLD_radio_trial21a_failed", automatic_retry=False, no_fallback_or_RUN_repeat=True)
            raise


if __name__ == "__main__":
    try: main()
    except Exception:
        with (BASE / "radio-trial-error21a.private.txt").open("x") as f:
            os.chmod(f.name, 0o600); traceback.print_exc(file=f)
        print("Radio trial HOLD; no repeat/RUN/fallback/target repair", file=sys.stderr); raise SystemExit(1)
