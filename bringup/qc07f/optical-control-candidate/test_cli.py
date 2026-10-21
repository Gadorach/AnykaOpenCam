#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Parse actual included-C mock receipts; execute HOST test binary only."""
import json
from pathlib import Path
import re
import subprocess
import sys

FIELDS = (
    "abi", "setup", "fault", "owned_mask", "pair_latch", "pair_pad",
    "ir_latch", "ir_pad", "direction_mask", "mux_ok", "release_verified",
    "ir_active", "ir_remaining_ms", "cooldown_remaining_ms", "pulse_us",
    "ir_limit_ms", "sequence", "monotonic_ns",
)
KEYS = {
    "action", "nonce", "result", "ioctl_completed", "command_errno",
    "snapshot_errno", "transport_errno", "keeper_errno", "verification_errno",
    "close_errno", "keeper_pid", "lease_mode", "state",
}
NONCE = "0123456789abcdef0123456789abcdef"


def main():
    binary = Path(sys.argv[1]).resolve()
    # Refuse ARM and everything except the expected host ELF test executable.
    header = binary.read_bytes()[:20]
    if header[:6] != b"\x7fELF\x02\x01" or header[18:20] != b"\x3e\x00":
        raise RuntimeError("only x86-64 HOST mock ELF may execute")
    result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=20)
    print(result.stdout, end="")
    if result.returncode or result.stderr:
        raise RuntimeError(f"mock failed: code={result.returncode}, stderr={result.stderr}")
    receipts = []
    for line in result.stdout.splitlines():
        if not line.startswith("RECEIPT "):
            continue
        _, name, code, raw = line.split(" ", 3)
        r = json.loads(raw)
        assert len(raw) < 2048 and set(r) == KEYS
        assert type(r["ioctl_completed"]) is bool
        assert (r["result"] == "OK") == (code == "0")
        assert r["nonce"] in (None, NONCE)
        assert r["action"] in (None, "status", "setup", "pulse-a", "pulse-b", "ir-on", "ir-off")
        assert r["lease_mode"] in ("none", "fork", "self")
        if r["action"] == "ir-on":
            assert r["lease_mode"] == ("self" if name.startswith("self-") else "fork")
        else:
            assert r["lease_mode"] == "none"
        if r["lease_mode"] != "fork":
            assert r["keeper_pid"] == 0
        for key in KEYS - {"action", "nonce", "result", "ioctl_completed", "state", "lease_mode"}:
            assert type(r[key]) is int and r[key] >= 0
        state = r["state"]
        if state is not None:
            assert set(state) == set(FIELDS)
            for i, key in enumerate(FIELDS):
                assert type(state[key]) is int and 0 <= state[key] < 2 ** (32 if i < 16 else 64)
            # No uint64 rounding/truncation and no omitted/relabelled UAPI fields.
            if name == "max-fields":
                assert all(state[k] == 2**32 - 1 for k in FIELDS[:16])
                assert state["sequence"] == state["monotonic_ns"] == 2**64 - 1
            else:
                assert state["sequence"] == 18364758544493064720
                assert state["monotonic_ns"] == 121456789123
                if name == "distinct-fields":
                    assert [state[k] for k in FIELDS[:16]] == [1, *range(11, 176, 11)]
                else:
                    assert state["cooldown_remaining_ms"] == 1876
                    assert state["pulse_us"] == 10000
        if r["result"] == "OK":
            assert r["ioctl_completed"] and state is not None
            assert not any(r[k] for k in KEYS if k.endswith("_errno"))
        if name == "ir-on":
            assert r["keeper_pid"] == 931 and state["ir_active"] == 1
            assert 0 < state["ir_remaining_ms"] <= 4900
        elif name in ("self-on", "self-short-write", "self-holder-exit"):
            assert r["lease_mode"] == "self" and r["keeper_pid"] == 0
            assert r["result"] == "OK" and state["ir_active"] == 1
            assert state["ir_latch"] == state["ir_pad"] == 1
            assert 0 < state["ir_remaining_ms"] <= 4900
            assert state["ir_limit_ms"] == 4900
        elif name == "self-fstat-close-error":
            assert r["transport_errno"] and r["close_errno"] and state is None
        elif name == "fork-failure":
            assert r["result"] == "FAILED" and r["keeper_errno"] != 0
            assert r["ioctl_completed"] and r["keeper_pid"] == 0 and state is None
        elif name == "snapshot-failure":
            assert r["ioctl_completed"] and r["snapshot_errno"] != 0 and state is None
        elif name in ("command-eintr", "command-errno", "unknown-command-result", "unknown-on-result"):
            assert not r["ioctl_completed"] and r["command_errno"] != 0
            assert state is not None
        elif name == "bad-nonce":
            assert r["nonce"] is None and r["transport_errno"] != 0
        elif name in ("invalid-on-snapshot", "self-invalid-on-snapshot"):
            assert r["ioctl_completed"] and r["verification_errno"] != 0
        elif name == "status-unknown-abi":
            assert r["ioctl_completed"] and r["command_errno"] == 0 and r["snapshot_errno"] != 0
        receipts.append(name)
    assert set(("status", "pulse-a", "pulse-b", "ir-on", "ir-off", "--setup")) <= set(receipts)
    assert receipts.count("invalid-on-snapshot") == 14
    assert receipts.count("self-invalid-on-snapshot") == 14
    assert receipts.count("self-holder-exit") == 6
    assert {"self-on", "self-short-write", "self-fstat-close-error"} <= set(receipts)
    match = re.search(r"^CLI_MOCK_PASS cases=(\d+) no_real_devices_or_forks$", result.stdout, re.M)
    assert match and int(match[1]) == len(receipts) + 17
    print(f"CLI_JSON_PASS receipts={len(receipts)} host_cases={match[1]} actual_uapi_fields=18")


if __name__ == "__main__":
    main()
