#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Offline strict-reassembly fixtures; no sockets or camera access."""
import io
import struct
import unittest
from rtsp_capture import Capture, crc_bzip2

def packet(seq, payload, marker=False, timestamp=9000):
    return struct.pack('!BBHII', 0x80, 96 | (128 if marker else 0), seq, timestamp, 123) + payload

class Tests(unittest.TestCase):
    def capture(self):
        return Capture(io.BytesIO(), io.BytesIO(), io.StringIO())

    def test_oracle_and_single(self):
        self.assertEqual(crc_bzip2(b'123456789'), 0xfc891918)
        c = self.capture()
        c.packet(packet(65535, b'\x67sps'))
        c.packet(packet(0, b'\x68pps'))
        c.packet(packet(1, b'\x65slice', True))
        self.assertEqual(c.units[0]['nal_bytes'], 6)
        self.assertEqual(c.media.getvalue(), b'\0\0\0\1\x67sps\0\0\0\1\x68pps\0\0\0\1\x65slice')

    def test_fragment_completion_and_no_partial_output(self):
        c = self.capture()
        c.packet(packet(1, b'\x7c\x85first'))
        c.packet(packet(2, b'\x7c\x05middle'))
        self.assertEqual(c.media.getvalue(), b'')
        c.packet(packet(3, b'\x7c\x45last', True))
        self.assertEqual(c.media.getvalue(), b'\0\0\0\1\x65firstmiddlelast')
        c.packet(packet(4, b'\x5c\x81incomplete', timestamp=10000))
        self.assertEqual(len(c.units), 1)
        self.assertEqual(c.media.getvalue(), b'\0\0\0\1\x65firstmiddlelast')

    def test_invalid_never_delivered(self):
        cases = [
            [packet(1, b'\x7c\x45orphan', True)],
            [packet(1, b'\x7c\xc5both', True)],
            [packet(1, b'\x7c\x85start'), packet(3, b'\x7c\x45gap', True)],
            [packet(1, b'\x7c\x85start'), packet(2, b'\x7c\x45changed', True, 9100)],
            [packet(1, b'\x7c\x85start'), packet(2, b'\x7c\x41type', True)],
            [packet(1, b'\x7c\x85start'), packet(2, b'\x7c\x05earlymarker', True)],
            [packet(1, b'\x65one'), packet(2, b'\x61two', True)],
            [packet(1, b'\x67sps', True)],
        ]
        for packets in cases:
            c = self.capture()
            with self.subTest(packets=packets), self.assertRaises(ValueError):
                for p in packets:
                    c.packet(p)
            self.assertEqual(c.media.getvalue(), b'')
            self.assertEqual(c.units, [])

if __name__ == '__main__':
    unittest.main()
