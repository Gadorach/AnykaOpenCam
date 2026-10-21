#!/usr/bin/env python3
"""One postfailure cached retirement read; never clears the original trial HOLD."""
from pathlib import Path
import datetime
import fcntl
import hashlib
import json
import os
import re
import sys
import time
import traceback
import metadata21a as meta
import trial21a as trial

if not __debug__:
    raise SystemExit("Assertions required")
BASE, EVIDENCE, b = meta.BASE, meta.EVIDENCE, meta.b
OLD_HOLD = {"state": "HOLD", "operation": "one_radio_native_load_RUN21a"}
TRANSITION = re.compile(rb"USB disconnect|new [^\r\n]*USB device|reset [^\r\n]*USB device|New USB device", re.IGNORECASE)
NEW_KERNEL = re.compile(rb"Linux version |Booting Linux|Starting kernel|U-Boot 2013")
SOURCE_PINS = {
    "trial21a.py": "6d0ce652d3355baef2e5505e9d2a34813aa1f780289b17268ccb160c4748da9c",
    "metadata21a.py": "f0a0e48969184d8df441d525777bc4ae7110177eaf81016c8ab746c8338b2376",
    "radio_receipt.py": "4b7d9c1887572a6fc7c7138ab3881c8739a00cbf08b0d8a775ded0465af5125c",
}


def read_private(name):
    return json.loads(meta.private.private_path(BASE / name).read_text())


def wire_lengths(post):
    command = meta.script("1" * 32).encode()
    assert len(command) == 527
    return ([len(c.encode()) + 1 for c in list(post.QUERIES.values()) + [post.QUERIES["boot_id"]]]
            + [6] + [len(command[i:i + 64]) for i in range(0, len(command), 64)] + [1, 5, 18])


def reconcile_counter(post, idle, checkpoint, failed, post_tx, pre, events):
    assert type(failed["optional_initial_monitor_refresh"]) is bool
    assert failed["checkpoint_tx_bytes"] == post_tx == checkpoint["tx_bytes"] + 320 + int(idle["one_empty_monitor_refresh"])
    expected = checkpoint["tx_bytes"] + 1197 + int(idle["one_empty_monitor_refresh"]) + int(failed["optional_initial_monitor_refresh"])
    assert type(failed["tx_bytes"]) is int and failed["tx_bytes"] == expected
    assert pre["event"] == "parent_before_trial_admission" and pre["tx_bytes"] == post_tx and pre["target_TX"] is False
    events = [row for row in events if row["utc"] >= pre["utc"]]
    complete = [row["bytes"] for row in events if row["kind"] == "tx_complete"]
    intents = [row["bytes"] for row in events if row["kind"] == "tx_protected_intent"]
    lengths = ([1] if failed["optional_initial_monitor_refresh"] else []) + wire_lengths(post)
    assert complete == intents == lengths
    assert not any(row["kind"].startswith("tx") and row["kind"] not in ("tx_complete", "tx_protected_intent") for row in events)
    assert failed["completed_sends_count"] == len(lengths)
    assert failed["approved_recipe_send_lengths"] == wire_lengths(post)
    assert failed["original_preflight_rx_bytes"] == pre["rx_bytes"]
    return expected


def handoff():
    for name, pin in SOURCE_PINS.items():
        assert hashlib.sha256(Path(__file__).with_name(name).read_bytes()).hexdigest() == pin
    post, boot, idle, checkpoint, post_tx = meta.admitted_handoff()
    failure = json.loads((EVIDENCE / "RADIO_NATIVE76_21A01_HOLD.json").read_text())
    assert failure["state"] == "RADIO76_STRICT_TRIAL_HOLD_RUN_RECEIPT_INTERLEAVED"
    assert failure["HOLD_retained"] is True and failure["application_state_info_queries_sent"] is False
    assert failure["post_load_descriptor_FD_idle_retirement_performed"] is False
    failed = read_private("radio-failed-trial21a01.private-checkpoint.json")
    assert failed["state"] == "OFFLINE_FAILED_TRIAL_TX_DERIVATION_VERIFIED"
    assert failed["part"] == meta.PART and failed["image_sha256"] == meta.SHA
    assert failed["recorder_pid"] == checkpoint["recorder_pid"] and failed["start_ticks"] == checkpoint["start_ticks"]
    pre = [json.loads(line) for line in meta.private.private_path(BASE / "radio-parent-preflight21a01/events.jsonl").read_text().splitlines()]
    assert len(pre) == 1 and pre[0]["event"] == "parent_before_trial_admission"
    pre = pre[0]
    events = [json.loads(line) for line in meta.private.private_path(BASE / "uart-capture21a/uart-events-foundation-20261021a.jsonl").read_text().splitlines()]
    expected = reconcile_counter(post, idle, checkpoint, failed, post_tx, pre, events)
    snapshot = meta.private.private_path(BASE / "radio-failure21a01.saved-private.bin").read_bytes()
    assert failed["saved_snapshot_end_rx_bytes"] == pre["rx_bytes"] + len(snapshot)
    assert b"RADIO RAM LOAD/CRC/RUN ACKNOWLEDGED; application/ABI/association still require verification" in snapshot
    assert b"qc07f> " in snapshot
    assert b"RADIO BOOT STATE QUERY TX RESULT" not in snapshot and b"RADIO READONLY FWINFO TX RESULT" not in snapshot
    runtime = read_private("runtime21a01.private.json")
    return post, checkpoint, failed, expected, runtime, snapshot


class Guard:
    """Bound existing readers at call boundaries; no new transport/recovery path."""
    def __init__(self, session, held, hold_bytes, original_peer, start_ticks, original_rx, settled_rx, deadline):
        self.session, self.held, self.hold_bytes = session, held, hold_bytes
        self.original_peer, self.start_ticks = original_peer, start_ticks
        self.original_rx, self.settled_rx, self.deadline = original_rx, settled_rx, deadline

    def __getattr__(self, name):
        return getattr(self.session, name)

    def check_stream(self):
        assert time.monotonic() < self.deadline
        self.held.seek(0); assert self.held.read() == self.hold_bytes
        assert self.session.peer == self.original_peer and b.ticks(self.original_peer[0]) == self.start_ticks
        state = self.session.check_stream()
        assert state["abort_armed"] is True and state["tx_busy"] is False
        assert state["rx_bytes"] >= self.settled_rx
        whole = self.session.raw_since(self.original_rx, state)
        assert not trial.HCD_DIAGNOSTIC.search(whole) and not NEW_KERNEL.search(whole)
        assert not TRANSITION.search(self.session.raw_since(self.settled_rx, state))
        return state

    def send(self, wire, reason):
        self.check_stream()
        return self.session.send(wire, reason)

    def wait(self, marker, start, timeout, **kwargs):
        self.check_stream()
        remaining = self.deadline - time.monotonic()
        assert remaining > 0
        value = self.session.wait(marker, start, min(timeout, remaining), **kwargs)
        self.check_stream()
        return value


def main():
    assert sys.argv[1:] == ["--one-readonly-retirement-preserve-HOLD21a"]
    os.umask(0o077)
    with (b.CONTROL / "peripheral-gui-operation.lock").open("r+") as held:
        fcntl.flock(held, fcntl.LOCK_EX | fcntl.LOCK_NB)
        hold_bytes = held.read(); assert json.loads(hold_bytes) == OLD_HOLD
        post, checkpoint, failed, expected_tx, runtime, snapshot = handoff()
        session = meta.private.PrivateBoot(BASE / "radio-retirement21a01", BASE / "uart-capture21a")
        try:
            current = session.owner(margin=300)
            # Original launch is the same unprivileged parent identity; pinned
            # recorder/launcher have no credential-changing or exec path.
            peer = (checkpoint["recorder_pid"], os.getuid(), os.getgid())
            assert session.peer == peer and b.ticks(peer[0]) == checkpoint["start_ticks"]
            assert current["part"] == meta.PART and current["tx_bytes"] == expected_tx and current["tx_busy"] is False and current["abort_armed"] is True
            assert current["rx_bytes"] >= failed["saved_snapshot_end_rx_bytes"]
            assert session.raw_since(failed["original_preflight_rx_bytes"], dict(current, rx_bytes=failed["saved_snapshot_end_rx_bytes"])) == snapshot
            session.epoch_cursor = checkpoint["rx_bytes"]
            deadline = time.monotonic() + 150
            guard = Guard(session, held, hold_bytes, peer, checkpoint["start_ticks"], checkpoint["rx_bytes"], current["rx_bytes"], deadline)
            current = meta.state(guard, expected_tx)
            time.sleep(2)
            current = meta.state(guard, expected_tx)
            refreshed = not current["recent_rx"].endswith("qc07f> ")
            if refreshed:
                assert "qc07f> " in current["recent_rx"]
                answer, expected_tx = meta.fixed(guard, b"\r", expected_tx, meta.MONITOR, 5)
                assert answer.replace(b"\r", b"") == b"\nqc07f> "
            metadata, expected_tx = trial.metadata_roundtrip(guard, expected_tx)
            rows, memory, expected_tx = trial.idle_inventory(guard, expected_tx, post)
            assert rows["boot_id"].strip() == runtime["boot_id"].strip()
            refreshed_owner = session.owner(margin=150)
            assert session.peer == peer and b.ticks(peer[0]) == checkpoint["start_ticks"]
            assert refreshed_owner["part"] == meta.PART and refreshed_owner["abort_armed"] is True
            final = meta.state(guard, expected_tx)
            assert final["recent_rx"].endswith("qc07f> ")
            assert expected_tx == failed["tx_bytes"] + 859 + int(refreshed)
            assert time.monotonic() < deadline
            record = {"utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
                      "state": "READONLY_POSTFAILURE_RETIREMENT_OBSERVED_ORIGINAL_TRIAL_FAILED_HOLD",
                      "image_sha256": meta.SHA, "part": meta.PART,
                      "original_trial_failed_HOLD_preserved_byte_exact": True,
                      "same_original_boot_private_UUID": True, "same_original_owner_source_exact_TX": True,
                      "no_new_HCD_diagnostic_or_followup_topology_transition": True,
                      "owned_shell_returned_with_cached_metadata": metadata, "idle_memory_KiB": memory,
                      "ISP_idle_logical_watchdog_inactive_UART_clean": True,
                      "one_empty_monitor_refresh": refreshed,
                      "no_radio_USB_query_reset_module_network_media_or_persistent_write": True,
                      "all_URB_DMA_electrical_watchdog_or_original_RUN_ACK_proved": False,
                      "recipe_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}
            private_record = {"part": meta.PART, "image_sha256": meta.SHA, "peer": peer,
                              "start_ticks": checkpoint["start_ticks"], "tx_bytes": expected_tx,
                              "rx_bytes": final["rx_bytes"], "rows": rows, "metadata": metadata,
                              "original_HOLD": hold_bytes, "original_trial_still_failed": True}
            with (BASE / "radio-retirement21a01.private-checkpoint.json").open("x") as stream:
                json.dump(private_record, stream, indent=2); stream.write("\n")
            meta.state(guard, expected_tx)
            with (EVIDENCE / "RADIO_READONLY_RETIREMENT21A01.json").open("x") as stream:
                json.dump(record, stream, indent=2); stream.write("\n")
            held.seek(0); assert held.read() == hold_bytes
            session.log("readonly_retirement_complete_original_HOLD_preserved", tx_bytes=expected_tx, rx_bytes=final["rx_bytes"])
            print("Read-only postfailure retirement observed; ORIGINAL TRIAL FAILED/HOLD unchanged")
        except BaseException:
            session.log("readonly_retirement_failed_original_HOLD_retained", no_retry=True)
            raise


if __name__ == "__main__":
    try:
        main()
    except Exception:
        with (BASE / "radio-retirement-error21a01.private.txt").open("x") as stream:
            traceback.print_exc(file=stream)
        print("Retirement-only follow-up HOLD; no retry/reset/repair", file=sys.stderr)
        raise SystemExit(1)
