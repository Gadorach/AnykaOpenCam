#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Independent piecewise G.711 oracle, not microphone or resampling evidence."""
from bisect import bisect_right
from pathlib import Path
import sys


def decode_mu(code):
    inverted = 255 - code
    negative = inverted >= 128
    segment = (inverted // 16) % 8
    mantissa = inverted % 16
    magnitude = (132 + 8 * mantissa) * (2 ** segment) - 132
    return -magnitude if negative else magnitude


def decode_a(code):
    inverted = code ^ 0x55
    positive = inverted >= 128
    segment = (inverted // 16) % 8
    mantissa = inverted % 16
    magnitude = 8 + 16 * mantissa if segment == 0 else (264 + 16 * mantissa) * (2 ** (segment - 1))
    return magnitude if positive else -magnitude


def encode_mu(sample):
    # Quantizer interval inverse, independent of C exponent/segment search.
    lower = [max(0, (128 + 8 * mantissa) * (2 ** segment) - 132)
             for segment in range(8) for mantissa in range(16)]
    cell = bisect_right(lower, abs(sample)) - 1
    return cell ^ (0x7f if sample < 0 else 0xff)


def encode_a(sample):
    lower = [16 * m if s == 0 else (256 + 16 * m) * (2 ** (s - 1))
             for s in range(8) for m in range(16)]
    cell = bisect_right(lower, -sample - 1 if sample < 0 else sample) - 1
    return cell ^ (0x55 if sample < 0 else 0xd5)


def main():
    out = Path(sys.argv[1]);out.mkdir()
    for law, encoder, decoder in (("mu", encode_mu, decode_mu), ("a", encode_a, decode_a)):
        codes = bytes(encoder(sample) for sample in range(-32768, 32768))
        decoded = b"".join(decoder(code).to_bytes(2, "little", signed=True) for code in range(256))
        with (out / (law + "-encode-oracle.bin")).open("xb") as f:f.write(codes)
        with (out / (law + "-decode-oracle.s16le")).open("xb") as f:f.write(decoded)
    print("piecewise interval oracle: 65536 S16 values and 256 decode words per law; synthetic only")


if __name__ == "__main__":main()
