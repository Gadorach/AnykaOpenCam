#!/usr/bin/env python3
"""Read public historical protocol evidence; export counts/shape booleans only."""
import datetime
import hashlib
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[4]
OUT = Path(__file__).resolve().parent
LOAD = ROOT / ("work/lightbulb-camera/implementation-20261008T205404Z/analysis/"
               "i-qualified-retained256-radio-ram-restore02.json")
STATE = ROOT / ("evidence/qualification/qc07f/native-ui-20261012/native02/"
                "original-retained256-radio-restore-ACK-state02.json")
ACK = re.compile(r"RADIO BOOT RX cmd0x([0-9a-f]{8}) cookie0x([0-9a-f]{8}) "
                 r"len0x([0-9a-f]{8}) header ([0-9a-f]{32})")


def strings(value):
    if isinstance(value, str):
        yield value
    elif isinstance(value, dict):
        for v in value.values():
            yield from strings(v)
    elif isinstance(value, list):
        for v in value:
            yield from strings(v)


def main():
    # These are existing public historical receipt files, not private captures.
    candidates = [ACK.findall(s) for s in strings(json.loads(LOAD.read_text()))]
    replies = max(candidates, key=len)
    assert len(replies) == 1185  # ENTER, GET_SC, 591 command/data pairs, RUN.
    normal_shape = all(int(n, 16) == 16 and bytes.fromhex(h)[:6] == bytes.fromhex("1a2b07001000")
                       for cmd, cookie, n, h in replies)
    correlation = all(int.from_bytes(bytes.fromhex(h)[6:8], "little") == int(cookie, 16)
                      and bytes.fromhex(h)[9] == 0 and bytes.fromhex(h)[15] == 0xfd
                      and (int(cmd, 16) == 0xffffffff or bytes.fromhex(h)[8] == int(cmd, 16))
                      for cmd, cookie, n, h in replies)
    assert normal_shape and correlation
    public_state = json.loads(STATE.read_text())["actual_state"]
    match = re.search(r"RADIO RX BYTES ([0-9a-f]{32})", public_state)
    assert match
    state = bytes.fromhex(match[1])
    assert state[:6] == bytes.fromhex("1a2b07001000") and state[9] == 0xff and state[15] == 0xfd
    record = {
        "utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "scope": "independent offline public historical protocol metadata; not current runtime",
        "collector_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        "historical_load_source": str(LOAD.relative_to(ROOT)),
        "historical_application_state_source": str(STATE.relative_to(ROOT)),
        "historical_256_ACK_count": len(replies),
        "all_ACKs_actual_length16_type7_vif0_flags0_declared_length16": normal_shape,
        "all_ACKs_cookie_command_status0_checkFD_match": correlation,
        "historical_APPLICATION_FF_same_header_shape": True,
        "raw_transcript_or_header_bytes_exported": False,
        "private_capture_access": False,
        "target_operation": False,
        "native76_trial_claimed": False,
    }
    with (OUT / "ack-corroboration.json").open("x") as f:
        json.dump(record, f, indent=2); f.write("\n")
    print("Historical public ACK shape independently corroborated; counts/booleans only.")


if __name__ == "__main__":
    main()
