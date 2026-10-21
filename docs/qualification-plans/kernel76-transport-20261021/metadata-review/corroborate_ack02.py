#!/usr/bin/env python3
"""Preserve a historical truncation; export only complete public ACK metadata."""
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


def main():
    source = json.loads(LOAD.read_text())
    text = source["text"]
    assert bytes.fromhex(source["rx_hex"]).decode("ascii") == text
    rows = re.findall(r"RADIO BOOT RX cmd0x([0-9a-f]{8}) cookie0x([0-9a-f]{8}) "
                      r"len0x([0-9a-f]{8}) header ([0-9a-f]*)", text)
    assert len(rows) == 1185
    complete = [row for row in rows if len(row[3]) == 32]
    incomplete = [row for row in rows if len(row[3]) != 32]
    assert len(complete) == 1184 and len(incomplete) == 1 and len(incomplete[0][3]) == 28
    assert all(int(n, 16) == 16 and h.startswith("1a2b07001000") for cmd, cookie, n, h in rows)
    correlation = all(int.from_bytes(bytes.fromhex(h)[6:8], "little") == int(cookie, 16)
                      and bytes.fromhex(h)[9] == 0 and bytes.fromhex(h)[15] == 0xfd
                      and (int(cmd, 16) == 0xffffffff or bytes.fromhex(h)[8] == int(cmd, 16))
                      for cmd, cookie, n, h in complete)
    assert correlation
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
        "historical_256_logged_ACK_count": len(rows),
        "complete_ACK_headers_used_for_corroboration": len(complete),
        "truncated_logged_headers_not_accepted": len(incomplete),
        "public_text_and_hex_identical_no_repair": True,
        "complete_ACKs_actual_length16_type7_vif0_flags0_declared_length16": True,
        "complete_ACKs_cookie_command_status0_checkFD_match": correlation,
        "historical_APPLICATION_FF_same_header_shape": True,
        "historical_full_transcript_strict_acceptance_claimed": False,
        "raw_transcript_or_header_bytes_exported": False,
        "private_capture_access": False,
        "target_operation": False,
        "native76_trial_claimed": False,
    }
    with (OUT / "ack-corroboration02.json").open("x") as f:
        json.dump(record, f, indent=2); f.write("\n")
    print("1184 complete historical public ACK headers corroborate shape; one truncation retained, not accepted.")


if __name__ == "__main__":
    main()
