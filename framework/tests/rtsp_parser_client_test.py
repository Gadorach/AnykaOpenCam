#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Existing strict capture classes, owned loopback only, wire kept in memory.

No requests/headers/URIs/wire dumps are printed or persisted. All media synthetic.
"""
import importlib.util
import io
from pathlib import Path
import socket
import sys
import time

if len(sys.argv)!=3:raise SystemExit(2)
port=int(sys.argv[1]);baseline=Path(sys.argv[2])
if not 1024<=port<=65535:raise SystemExit(2)
def module(path,name):
    spec=importlib.util.spec_from_file_location(name,path)
    out=importlib.util.module_from_spec(spec);spec.loader.exec_module(out);return out
old=module(baseline,'frozen_capture')
new=module(Path(__file__).with_name('rtsp_capture.py'),'copied_capture')
endpoint=f'rtsp://127.0.0.1:{port}/live'
def connect_owned():
    # Only the exact loopback port of the child launched by this fixture runner.
    for _ in range(100):
        peer=socket.socket();peer.settimeout(2)
        try:peer.connect(('127.0.0.1',port));return peer
        except ConnectionRefusedError:peer.close();time.sleep(0.01)
    raise RuntimeError('owned mock startup failed')
def client(api,propagate):
    capture=api.Capture(io.BytesIO(),io.BytesIO(),io.StringIO());session=''
    with connect_owned() as peer:
        for seq,method in enumerate(('OPTIONS','SETUP','PLAY'),1):
            extra='Transport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n' if method=='SETUP' else ''
            if method=='PLAY' and propagate:
                assert session
                extra=f'Session: {session}\r\n'
            peer.sendall(f'{method} {endpoint} RTSP/1.0\r\nCSeq: {seq}\r\n{extra}\r\n'.encode())
            returned=capture.reply(peer,seq)
            if method=='SETUP':session=returned
        while len(capture.units)<3:
            header=capture.exact(peer,4);assert header[:2]==b'$\0'
            size=int.from_bytes(header[2:],'big');assert 13<=size<=1400
            capture.packet(capture.exact(peer,size))
        assert all(x['nal_bytes']==6000 and x['complete'] for x in capture.units)
        assert capture.units[0]['keyframe']
        for row in capture.units:
            public=bytes([0x65 if row['keyframe'] else 0x41])+bytes((i*31)&255 for i in range(1,6000))
            assert row['crc_bzip2']==f'{api.crc_bzip2(public):08x}'
try:client(old,False)
except EOFError:pass  # Positive rejection of unchanged missing-Session behavior.
else:raise AssertionError('missing Session was accepted')
# The actual owned server policy, not just a fake-clock parser fixture: empty
# and trickled incomplete first headers expire despite successful socket reads.
for trickle in (False,True):
    with connect_owned() as peer:
        peer.settimeout(3);started=time.monotonic()
        if trickle:
            peer.sendall(b'OPT');time.sleep(0.6);peer.sendall(b'I')
            time.sleep(0.6);peer.sendall(b'O')
        assert peer.recv(1)==b''
        elapsed=time.monotonic()-started
        assert 1.5<=elapsed<=3
client(new,True);client(new,True)
print('Capture compatibility: PASS old missing-Session rejected; real owned first-header empty/trickle deadlines passed; copied Session-aware reconnect twice, 3 complete synthetic AUs each')
