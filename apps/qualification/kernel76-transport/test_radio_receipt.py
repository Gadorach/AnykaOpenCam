#!/usr/bin/env python3
import struct
import unittest
import radio_receipt as r


def ack(command, cookie):
    header = bytearray.fromhex("1a2b07001000000000000000000000fd")
    header[6:8] = cookie.to_bytes(2, "little")
    header[8] = command if command != 0xffffffff else 0xaa
    return f"RADIO BOOT RX cmd0x{command:08x} cookie0x{cookie:08x} len0x00000010 header {header.hex()}"


def loader():
    lines = [ack(0, 100), ack(1, 101), r.BEGIN]
    offset, cookie = 0, 102
    while offset < 151056:
        size = min(2048, 151056 - offset); wire = size + (4 if size % 512 == 0 else 0)
        lines += [ack(2, cookie), f"RADIO DATA OUT BEGIN payload0x{size:08x} wire0x{wire:08x}",
                  f"RADIO DATA OUT RESULT0x{wire:08x}", ack(0xffffffff, cookie)]
        offset += size; cookie += 1
        if cookie & 7 == 0: lines.append(f"RADIO RAM BYTES ACCEPTED0x{offset:08x}")
    return "\n".join(lines + [ack(4, 176), r.DONE])


class Receipts(unittest.TestCase):
    def test_complete_loader(self):
        result = r.parse_loader(loader())
        self.assertEqual(result["matched_ACKs"], 151)
        self.assertFalse(result["MODE_NAK_toggle_or_MTU1500_proved"])

    def test_loader_mutants(self):
        text = loader()
        mutants = [text.replace("RESULT0x00000804", "RESULT0xffffff92", 1),
                   text.replace("wire0x00000804", "wire0x00000800", 1),
                   text.replace(ack(2, 102) + "\n", "", 1),
                   text.replace(ack(2, 102), ack(2, 103), 1),
                   text.replace("1a2b07001000", "1a2b07002000", 1),
                   text.replace("1a2b0700", "1a2b0800", 1),
                   text.replace("len0x00000010", "len0x00000020", 1),
                   text.replace(r.DONE, "RADIO RAM LOAD FAILED offset0x00000000 result0xffffff92"),
                   text + "\nUNEXPECTED", text.replace(ack(4, 176) + "\n", ""),
                   text.replace("RADIO RAM BYTES ACCEPTED0x00001000", "BAD"),
                   text.replace("RADIO DATA OUT RESULT0x00000610", "RADIO DATA OUT RESULT0x0000060f")]
        for value in mutants:
            with self.subTest(value=value[:40]):
                self.assertNotEqual(value, text)
                with self.assertRaises(AssertionError): r.parse_loader(value)

    def test_firmware_info_never_returns_device_bytes(self):
        reply = bytearray.fromhex("1a2b0301240001002b001800") + bytearray(24)
        struct.pack_into("<IIHH", reply, 12, 0x02010001, 22118, 0x8400, 1)
        reply[24:] = b"PUBLIC_DUMMY"  # not acquired device data
        text = "\n".join(["RADIO READONLY FWINFO TX RESULT 0x0000000c", "RADIO RX RESULT 0x00000024",
                          "RADIO RX BYTES " + reply.hex(), "RADIO FWINFO COOKIE/CMD MATCH, RETURN/PAYLOAD LENGTH 0x00000018"])
        self.assertEqual(set(r.parse_firmware_info(text)), {"retained_chip_version_SVN_CPU_identity_matches", "device_identifying_fields_kept_private"})
        changed = bytearray(reply); changed[12] ^= 1
        with self.assertRaises(AssertionError): r.parse_firmware_info(text.replace(reply.hex(), changed.hex()))

    def test_state(self):
        header = bytearray.fromhex("1a2b07001000020000ff0000000000fd")
        text = "\n".join(["RADIO BOOT STATE QUERY TX RESULT 0x00000014", "RADIO RX RESULT 0x00000010",
                          "RADIO RX BYTES " + header.hex(), "RADIO BOOT QUERY CHECK/CMD/COOKIE MATCH, RESULT 0x000000ff", "RADIO STATE APPLICATION"])
        self.assertTrue(r.parse_application_state(text))
        with self.assertRaises(AssertionError): r.parse_application_state(text.replace("00ff00", "000000"))

    def test_fd_inventory(self):
        text = "\n".join(f"lrwx------    1        64 {i} -> /dev/console" for i in range(4))
        self.assertTrue(r.parse_parent_fds(text)["PID1_USB_watchdog_fds_absent"])
        for value in ("", text + "\nlrwx------ 1 64 4 -> /dev/watchdog0", text.replace("3 ->", "2 ->"),
                      "total 0\n" + text, text + "\ntotal 0", "\n" + text, text + "\n\n",
                      "\n".join(f"ls: unexpected {i} -> /dev/console" for i in range(4)),
                      text.replace("1        64", "1 0 0 64 Jan 1")):
            with self.assertRaises(AssertionError): r.parse_parent_fds(value)


if __name__ == "__main__": unittest.main()
