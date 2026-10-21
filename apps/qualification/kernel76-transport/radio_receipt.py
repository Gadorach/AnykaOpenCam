#!/usr/bin/env python3
"""Strict host parsing for a finite monitor76 trial; never sends target commands."""
import re
import struct

if not __debug__:
    raise SystemExit("Assertions required")

ACK = re.compile(r"RADIO BOOT RX cmd0x([0-9a-f]{8}) cookie0x([0-9a-f]{8}) len0x([0-9a-f]{8}) header ([0-9a-f]{32})")
BEGIN = "RADIO EXACT RETAIL RAM LOAD BEGIN20000000..20024e10 AESoff CRCraw0; no SPI/OTA/OTP"
DONE = "RADIO RAM LOAD/CRC/RUN ACKNOWLEDGED; application/ABI/association still require verification"


def ack(line, command, cookie):
    match = ACK.fullmatch(line)
    assert match and int(match[1], 16) == command and int(match[2], 16) == cookie
    assert int(match[3], 16) == 16
    header = bytes.fromhex(match[4])
    # Type/vif/length independently corroborated by historical successful256
    # ACK metadata. No device identifier or firmware payload is exported.
    assert header[:4] == bytes.fromhex("1a2b0700") and int.from_bytes(header[4:6], "little") == 16
    assert int.from_bytes(header[6:8], "little") == cookie and header[9] == 0 and header[15] == 0xfd
    if command != 0xffffffff:
        assert header[8] == command


def parse_loader(text):
    lines = text.replace("\r", "").strip().splitlines()
    # Caller supplies only the completed command body. No dropped ACK/log lines
    # or interleaved warning is silently ignored to obtain a positive result.
    index = 0
    def take():
        nonlocal index
        assert index < len(lines)
        value = lines[index]; index += 1; return value
    ack(take(), 0, 100)
    ack(take(), 1, 101)
    assert take() == BEGIN
    offset, cookie, chunks, total_wire = 0, 102, 0, 0
    while offset < 151056:
        payload = min(2048, 151056 - offset)
        wire = payload + (4 if payload % 512 == 0 else 0)
        ack(take(), 2, cookie)
        assert take() == f"RADIO DATA OUT BEGIN payload0x{payload:08x} wire0x{wire:08x}"
        assert take() == f"RADIO DATA OUT RESULT0x{wire:08x}"
        ack(take(), 0xffffffff, cookie)
        offset += payload; cookie += 1; chunks += 1; total_wire += wire
        if cookie & 7 == 0:
            assert take() == f"RADIO RAM BYTES ACCEPTED0x{offset:08x}"
    ack(take(), 4, 176)
    assert take() == DONE and index == len(lines)
    assert chunks == 74 and total_wire == 151348
    return {"functional_full_size_loader_complete": True, "payload_bytes": offset,
            "fragments": chunks, "data_wire_bytes": total_wire, "matched_ACKs": 151,
            "CRC_enabled_RUN_ACK": True, "explicit_ZLPs": 0,
            "MODE_NAK_toggle_or_MTU1500_proved": False}


def parse_application_state(text):
    lines = text.replace("\r", "").strip().splitlines()
    assert len(lines) == 5
    assert lines[0] == "RADIO BOOT STATE QUERY TX RESULT 0x00000014"
    assert lines[1] == "RADIO RX RESULT 0x00000010"
    match = re.fullmatch(r"RADIO RX BYTES ([0-9a-f]{32})", lines[2]); assert match
    reply = bytes.fromhex(match[1])
    assert reply[:4] == bytes.fromhex("1a2b0700") and int.from_bytes(reply[4:6], "little") == 16
    assert int.from_bytes(reply[6:8], "little") == 2 and reply[8] == 0 and reply[9] == 0xff and reply[15] == 0xfd
    assert lines[3] == "RADIO BOOT QUERY CHECK/CMD/COOKIE MATCH, RESULT 0x000000ff"
    assert lines[4] == "RADIO STATE APPLICATION"
    return True


def parse_firmware_info(text):
    lines = text.replace("\r", "").strip().splitlines()
    assert len(lines) == 4
    assert lines[0] == "RADIO READONLY FWINFO TX RESULT 0x0000000c"
    assert lines[1] == "RADIO RX RESULT 0x00000024"
    match = re.fullmatch(r"RADIO RX BYTES ([0-9a-f]{72})", lines[2]); assert match
    reply = bytes.fromhex(match[1])
    assert reply[:4] == bytes.fromhex("1a2b0301")
    assert int.from_bytes(reply[4:6], "little") == 36 and int.from_bytes(reply[6:8], "little") == 1
    assert reply[8] == 43 and struct.unpack_from("<h", reply, 10)[0] == 24
    assert struct.unpack_from("<IIHH", reply, 12) == (0x02010001, 22118, 0x8400, 1)
    assert lines[3] == "RADIO FWINFO COOKIE/CMD MATCH, RETURN/PAYLOAD LENGTH 0x00000018"
    # Remaining firmware-info bytes can identify the device: never return them.
    return {"retained_chip_version_SVN_CPU_identity_matches": True,
            "device_identifying_fields_kept_private": True}


def parse_parent_fds(text):
    result = {}
    lines = text.replace("\r", "").splitlines()
    assert len(lines) == 4
    for line in lines:
        # Bound A6: DESKTOP/LS_USERNAME/LS_TIMESTAMPS are disabled. `ls -l`
        # prints mode, nlink, size and name/link only: no total/uid/gid/date.
        match = re.fullmatch(r"lrwx------[ \t]+1[ \t]+64[ \t]+([0-3]) -> (/dev/(?:console|ttyAK0))", line)
        assert match and int(match[1]) not in result
        result[int(match[1])] = match[2]
    assert set(result) == {0, 1, 2, 3}
    return {"PID1_console_fds_only": True, "PID1_USB_watchdog_fds_absent": True}
