#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Strict TCP/RFC6184 evidence capture; incomplete AUs never enter decoder input."""
import argparse
import datetime
import hashlib
import ipaddress
import json
import re
from pathlib import Path
import socket
import struct
import sys
import time
from urllib.parse import urlsplit

def crc_bzip2(data):
    crc = 0xffffffff
    for byte in data:
        crc ^= byte << 24
        for _ in range(8):
            crc = ((crc << 1) ^ (0x04c11db7 if crc & 0x80000000 else 0)) & 0xffffffff
    return crc ^ 0xffffffff

class Capture:
    """Only the bounded fixed RTP headers emitted by our lab scheduler."""
    def __init__(self, wire, media, metadata):
        self.wire, self.media, self.metadata = wire, media, metadata
        self.buffer = bytearray()
        self.units = []
        self.sequence = self.ssrc = self.timestamp = None
        self.fu = None
        self.nals = []

    def exact(self, peer, count):
        while len(self.buffer) < count:
            data = peer.recv(65536)
            if not data:
                raise EOFError('TCP EOF')
            self.wire.write(data)
            self.buffer.extend(data)
        result = bytes(self.buffer[:count])
        del self.buffer[:count]
        return result

    def reply(self, peer, seq):
        header = bytearray()
        while not header.endswith(b'\r\n\r\n'):
            header.extend(self.exact(peer, 1))
            if len(header) > 8192:
                raise ValueError('Oversized RTSP reply')
        fields = header.decode('ascii').split('\r\n')
        if fields[0] != 'RTSP/1.0 200 OK':
            raise ValueError('RTSP request rejected')
        values = {}
        critical = {'cseq', 'content-length', 'session', 'transport', 'authorization'}
        lines = fields[1:-2]
        if len(lines) > 24:
            raise ValueError('Too many RTSP reply fields')
        for line in lines:
            if any(ord(c) < 32 or ord(c) > 126 for c in line) or ':' not in line:
                raise ValueError('Malformed RTSP reply field')
            name, value = line.split(':', 1)
            if len(name) > 64 or not re.fullmatch(r"[!#$%&'*+.^_`|~0-9A-Za-z-]+", name) or len(value) > 1024:
                raise ValueError('Malformed RTSP reply field')
            name, value = name.lower(), value.strip(' ')
            if name in {'transfer-encoding', 'trailer'} or name in critical and (name in values or not value):
                raise ValueError('Ambiguous RTSP reply field')
            values[name] = value
        def decimal(value, maximum):
            if not re.fullmatch(r'[0-9]{1,10}', value) or int(value) > maximum:
                raise ValueError('Invalid RTSP reply number')
            return int(value)
        if 'cseq' not in values or decimal(values['cseq'], 0xffffffff) != seq:
            raise ValueError('RTSP CSeq mismatch')
        count = decimal(values.get('content-length', '0'), 8192)
        session = values.get('session', '')
        if session:
            if len(session) > 128:
                raise ValueError('Oversized RTSP Session')
            match = re.fullmatch(r'([A-Za-z0-9$_.+\-]{1,64})(?:;timeout=([0-9]{1,10}))?', session, re.IGNORECASE)
            if not match or match[2] is not None and not decimal(match[2], 0xffffffff):
                raise ValueError('Invalid RTSP Session')
            session = match[1]  # Only the bounded opaque token; never a credential.
        if len(values.get('transport', '')) > 512:
            raise ValueError('Oversized RTSP Transport')
        self.exact(peer, count)
        # Prototype compatibility change: preserve the SETUP Session for PLAY.
        # No Authorization parsing or authentication is implemented here.
        return session

    def packet(self, data):
        if len(data) < 13 or data[0] != 0x80 or data[1] & 127 != 96:
            raise ValueError('Unsupported RTP header')
        seq, ts, ssrc = struct.unpack_from('!HII', data, 2)
        if self.sequence is not None and seq != (self.sequence + 1) & 65535:
            raise ValueError('RTP sequence gap')
        if self.ssrc is not None and self.ssrc != ssrc:
            raise ValueError('SSRC changed')
        self.sequence, self.ssrc = seq, ssrc
        if self.timestamp is not None and self.timestamp != ts:
            raise ValueError('Timestamp changed inside an AU')
        self.timestamp = ts
        payload = data[12:]
        kind = payload[0] & 31
        if payload[0] & 0x80:
            raise ValueError('Forbidden NAL bit')
        if kind == 28:
            if len(payload) < 3 or payload[1] & 0x20:
                raise ValueError('Malformed FU-A')
            begin, end = payload[1] & 0x80, payload[1] & 0x40
            identity = (payload[0] & 0xe0) | (payload[1] & 31)
            if not 1 <= identity & 31 <= 23 or begin and end:
                raise ValueError('Invalid FU-A NAL type/flags')
            if begin:
                if self.fu is not None:
                    raise ValueError('Nested FU-A')
                self.fu = bytearray([identity])
            if self.fu is None or self.fu[0] != identity:
                raise ValueError('FU-A without matching start')
            self.fu.extend(payload[2:])
            if len(self.fu) > 1048576:
                raise ValueError('Oversized FU-A')
            if end:
                self.nals.append(bytes(self.fu))
                self.fu = None
        elif 1 <= kind <= 23:
            if self.fu is not None:
                raise ValueError('NAL interrupts FU-A')
            self.nals.append(payload)
        else:
            raise ValueError('Unsupported packetization')
        if len(self.nals) > 3:
            raise ValueError('Too many NALs in lab AU')
        if data[1] & 0x80:
            vcl = [n for n in self.nals if n[0] & 31 in (1, 5)]
            if self.fu is not None or len(vcl) != 1 or self.nals[-1] is not vcl[0]:
                raise ValueError('Marker without one complete final VCL NAL')
            nal = vcl[0]
            row = {'index': len(self.units), 'timestamp': ts, 'nal_bytes': len(nal),
                   'keyframe': (nal[0] & 31) == 5, 'crc_bzip2': f'{crc_bzip2(nal):08x}',
                   'sha256': hashlib.sha256(nal).hexdigest(), 'complete': True}
            for n in self.nals:
                self.media.write(b'\0\0\0\1' + n)
            self.media.flush()
            self.metadata.write(json.dumps(row) + '\n')
            self.metadata.flush()
            self.units.append(row)
            self.timestamp, self.nals = None, []

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--endpoint', required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--frames', type=int, default=120)
    parser.add_argument('--seconds', type=int, default=180)
    args = parser.parse_args()
    url = urlsplit(args.endpoint)
    if url.scheme != 'rtsp' or url.username or url.password or url.path != '/live':
        raise ValueError('Only credential-free explicit lab /live endpoints')
    ipaddress.IPv4Address(url.hostname)
    if not 1 <= args.frames <= 2000 or not 1 <= args.seconds <= 1800:
        raise ValueError('Capture bounds')
    out = args.output.resolve()
    out.mkdir(mode=0o700)
    report = {'start_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
              'argv': sys.argv, 'endpoint': args.endpoint, 'python': sys.version,
              'input_script_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
              'incomplete_au_sent_to_decoder': False, 'requested_frames': args.frames}
    start = time.monotonic()
    with (out / 'received.wire').open('xb') as wire, (out / 'complete.h264').open('xb') as media, \
            (out / 'units.jsonl').open('x') as metadata, socket.socket() as peer:
        capture = Capture(wire, media, metadata)
        try:
            peer.settimeout(10)
            peer.connect((url.hostname, url.port or 554))
            session = ''
            for seq, method in enumerate(('OPTIONS', 'SETUP', 'PLAY'), 1):
                extra = 'Transport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n' if method == 'SETUP' else ''
                if method == 'PLAY':
                    if not session:
                        raise ValueError('Missing SETUP Session')
                    extra = f'Session: {session}\r\n'
                peer.sendall(f'{method} {args.endpoint} RTSP/1.0\r\nCSeq: {seq}\r\n{extra}\r\n'.encode())
                returned_session = capture.reply(peer, seq)
                if method == 'SETUP':
                    session = returned_session
            while len(capture.units) < args.frames:
                if time.monotonic() - start > args.seconds:
                    raise TimeoutError('Whole capture deadline')
                header = capture.exact(peer, 4)
                if header[:2] != b'$\0':
                    raise ValueError('Unsupported interleaved channel/control data')
                size = int.from_bytes(header[2:], 'big')
                if not 13 <= size <= 1400:
                    raise ValueError('Invalid interleaved length')
                capture.packet(capture.exact(peer, size))
            report['all_complete'] = True
        except (EOFError, OSError, ValueError, TimeoutError) as error:
            report.update(all_complete=False, error=str(error), error_type=type(error).__name__)
        finally:
            report.update(frames=len(capture.units), elapsed_seconds=time.monotonic() - start,
                          partial_fu_bytes=len(capture.fu) if capture.fu is not None else 0,
                          partial_au_nals=len(capture.nals), buffered_wire_bytes=len(capture.buffer),
                          end_utc=datetime.datetime.now(datetime.timezone.utc).isoformat())
    report['outputs'] = [{'path': p.name, 'bytes': p.stat().st_size,
                          'sha256': hashlib.sha256(p.read_bytes()).hexdigest()}
                         for p in out.iterdir()]
    (out / 'manifest.json').open('x').write(json.dumps(report, indent=2))
    print(json.dumps(report, indent=2))
    return 0 if report['all_complete'] else 1

if __name__ == '__main__':
    raise SystemExit(main())
