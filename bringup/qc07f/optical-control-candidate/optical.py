#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Fixed local ioctl transport; no network/shell/GPIO export/retry.

Use a persistent Optical() in the parent bridge: closing its fd releases IR.
The one-shot CLI deliberately closes immediately, including for ir-on.
Boot/generation/recovery/media admission belongs to the parent, NOT this API.
"""
import argparse
import fcntl
import json
import os
import struct
import time

DEVICE = "/dev/qc07f-optical"
LAYOUT = struct.Struct("=16I2Q")
STATUS = 0x80505160
COMMANDS = {
    "setup": 0x5161, "pulse-a": 0x5162, "pulse-b": 0x5163,
    "ir-on": 0x5164, "ir-off": 0x5165, "close": 0x5166,
}
FIELDS = (
    "abi", "setup", "fault", "owned_mask", "pair_latch", "pair_pad",
    "ir_latch", "ir_pad", "direction_mask", "mux_ok", "release_verified",
    "ir_active", "ir_remaining_ms", "cooldown_remaining_ms", "pulse_us",
    "ir_limit_ms", "sequence", "monotonic_ns",
)


class Optical:
    def __init__(self):
        self.fd = os.open(DEVICE, os.O_RDWR | os.O_CLOEXEC | os.O_NOFOLLOW)

    def status(self):
        data = bytearray(LAYOUT.size)
        fcntl.ioctl(self.fd, STATUS, data, True)
        state = dict(zip(FIELDS, LAYOUT.unpack(data)))
        if state["abi"] != 1:
            raise RuntimeError("unsupported optical ABI")
        return state

    def action(self, action):
        if action != "status" and action not in COMMANDS:
            raise ValueError("unknown fixed optical action")
        receipt = {"action": action, "host_monotonic_ns": time.monotonic_ns(),
                   "ioctl_completed": False, "errno": None, "state": None,
                   "snapshot_errno": None, "optical_position": "unknown"}
        if action == "status":
            try:
                receipt["state"] = self.status()
                receipt["ioctl_completed"] = True
            except OSError as error:
                receipt["snapshot_errno"] = error.errno
            return receipt
        try:
            fcntl.ioctl(self.fd, COMMANDS[action], 0)
            receipt["ioctl_completed"] = True
        except OSError as error:
            receipt["errno"] = error.errno
        # Never replay an effect if this separate status request fails.
        try:
            receipt["state"] = self.status()
        except OSError as error:
            receipt["snapshot_errno"] = error.errno
        return receipt

    def close(self):
        if self.fd is not None:
            fd, self.fd = self.fd, None
            os.close(fd)

    def __enter__(self):
        return self

    def __exit__(self, *_):
        self.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("status", *COMMANDS))
    args = parser.parse_args()
    try:
        with Optical() as endpoint:
            receipt = endpoint.action(args.action)
        receipt["transport_closed"] = True
        # ir-on's first snapshot is before close, not a surviving light lease.
        with Optical() as endpoint:
            receipt["after_close_state"] = endpoint.status()
        print(json.dumps(receipt, sort_keys=True))
        return 0 if receipt["ioctl_completed"] and receipt["snapshot_errno"] is None else 1
    except (OSError, RuntimeError) as error:
        print(json.dumps({"action": args.action, "transport_error": str(error),
                          "completion": "unknown", "retry": False}))
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
