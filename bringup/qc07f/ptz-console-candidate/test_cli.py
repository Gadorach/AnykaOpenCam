#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Validate actual C body receipts plus mocked syscall traces; no hardware."""
import json
import subprocess
import sys

NONCE = "0123456789abcdef0123456789abcdef"
KEYS = {"schema", "action", "nonce", "result", "stage", "requested_steps",
        "write_attempted", "write_completed", "errno", "diagnostic_errno",
        "hold", "before", "after"}
STATE = {"axis", "issued_steps", "phase", "result", "coil_readback_ok",
         "coils_off_verified", "max_pulse_steps", "period_ms",
         "physical_position_known", "health"}
cases = 0
receipts = 0


def run(scenario, action="motor0-plus", nonce=NONCE, extra=(), result="OK",
        writes=1, completed=None, stdout=True):
    global cases, receipts
    command = [sys.argv[1], scenario, action, nonce, *extra]
    proc = subprocess.run(command, capture_output=True, timeout=3)
    cases += 1
    try:
        metrics = json.loads(proc.stderr)
        assert metrics["violations"] == metrics["live_fds"] == 0, metrics
        assert metrics["pulse_calls"] == writes, metrics
        assert max(metrics["state_reads"]) <= 2, metrics
        assert metrics["stdout_calls"] <= 64, metrics
        if not stdout:
            assert proc.returncode == 4
            return None, metrics
        assert len(proc.stdout) <= 4096 and proc.stdout.endswith(b"\n")
        data = json.loads(proc.stdout)
        receipts += 1
        assert set(data) == KEYS, data
        assert data["schema"] == "qc07f.ptz.sysfs.v1"
        assert data["result"] == result, data
        assert proc.returncode == {"OK": 0, "FAILED": 2, "UNKNOWN": 3}[result]
        assert data["write_attempted"] == bool(writes)
        if completed is not None:
            assert data["write_completed"] == completed, data
        assert isinstance(data["errno"], int) and data["errno"] >= 0
        assert isinstance(data["diagnostic_errno"], int) and data["diagnostic_errno"] >= 0
        if result != "OK":
            assert data["hold"] is True
        for which in ("before", "after"):
            assert len(data[which]) == 2
            for axis, state in enumerate(data[which]):
                if state is None:
                    continue
                assert set(state) == STATE, state
                assert state["axis"] == axis
                assert 0 <= state["issued_steps"] <= 0xFFFFFFFF
                assert 0 <= state["phase"] <= 3
                assert state["max_pulse_steps"] == 4 and state["period_ms"] == 20
                assert state["physical_position_known"] == 0
        if result == "OK" and writes:
            axis = int(action[5])
            assert metrics["written_axis"] == axis
            assert metrics["payload_negative"] == action.endswith("minus")
            assert data["requested_steps"] == (-4 if action.endswith("minus") else 4)
            assert data["after"][axis]["issued_steps"] == data["before"][axis]["issued_steps"] + 4
            assert data["after"][axis]["phase"] == data["before"][axis]["phase"]
            assert data["before"][1-axis] == data["after"][1-axis]
            assert data["after"][axis]["health"] == "READY" and not data["hold"]
        return data, metrics
    except Exception:
        print("FAILED", command, proc.returncode, proc.stdout, proc.stderr, file=sys.stderr)
        raise


for action in ("motor0-minus", "motor0-plus", "motor1-minus", "motor1-plus"):
    for scenario in ("cold", "ready", "phase3", "wrap_edge", "read_chunks"):
        run(scenario, action, completed=True)
for scenario in ("cold", "ready", "bad_pre_result", "bad_pre_off", "bad_pre_readback",
                 "bad_other_pre", "intmin", "overflow", "uintmax", "untested_phase"):
    data, metrics = run(scenario, "status", writes=0, completed=False)
    assert metrics["pulse_opens"] == 0 and data["after"] == [None, None]
    assert data["hold"] == (scenario in {"bad_pre_result", "bad_pre_off", "bad_pre_readback",
                                        "bad_other_pre", "intmin", "untested_phase"})
    if scenario == "cold":
        assert all(s["health"] == "UNTESTED" for s in data["before"])
for scenario in ("nonroot", "node_missing", "bad_owner", "bad_mode", "bad_fs", "stat_error",
                 "statfs_error", "driver_wrong", "driver_prefix", "driver_error", "driver_truncated",
                 "bad_compatible", "compatible_extra", "compatible_nonul", "bad_axis_property",
                 "axis_little_endian", "axis_short", "bad_board", "property_symlink",
                 "state_open", "read_eintr", "empty_state", "state_big", "missing_newline",
                 "state_nul", "state_trailing", "state_overflow", "signed_overflow", "leading_zero",
                 "state_reordered", "bad_axis", "bad_phase", "bad_bound", "bad_period", "bad_known",
                 "bad_flag", "bad_pre_off", "bad_pre_readback", "bad_pre_result", "bad_other_pre",
                 "intmin", "untested_phase", "overflow", "uintmax", "read_budget", "pulse_open",
                 "pulse_type", "state_close"):
    run(scenario, writes=0, result="FAILED", completed=False)
for action in ("motor0-plus", "motor1-minus"):
    for scenario in ("write_eintr", "write_errno", "write_short", "write_zero"):
        data, metrics = run(scenario, action, result="UNKNOWN", completed=False)
        assert metrics["state_reads"] == [2, 2]
        assert data["errno"] != 0 and data["hold"]
    for scenario in ("pulse_close", "device_close", "after_open", "after_read", "after_axis0_read", "after_count3",
                     "after_count8", "after_phase", "after_result", "after_positive_result", "after_off",
                     "after_readback", "foreign_count", "foreign_phase", "foreign_result", "foreign_off",
                     "foreign_readback", "after_bad_period", "after_malformed"):
        run(scenario, action, result="UNKNOWN", completed=True)
for action in ("motor0-plus", "motor1-minus", "status"):
    writes = int(action != "status")
    for scenario in ("stdout_eintr", "stdout_chunks"):
        run(scenario, action, writes=writes, completed=bool(writes))
    for scenario in ("stdout_forever_eintr", "stdout_error", "stdout_zero"):
        run(scenario, action, writes=writes, stdout=False)
for action in ("motor0-plus; reboot", "motor2-plus", "motor0-plus32", "4", "-4", "--help", ""):
    data, metrics = run("cold", action, result="FAILED", writes=0)
    assert metrics["opens"] == 0 and data["action"] == "invalid"
for nonce in ("", "a"*31, "a"*33, "A"*32, "g"*32, '"'*32, "a\n"*16):
    data, metrics = run("cold", nonce=nonce, result="FAILED", writes=0)
    assert metrics["opens"] == 0 and data["nonce"] == ""
run("cold", extra=("32",), result="FAILED", writes=0)
print(f"PTZ_JSON_PASS receipts={receipts} host_cases={cases} schema=qc07f.ptz.sysfs.v1")
