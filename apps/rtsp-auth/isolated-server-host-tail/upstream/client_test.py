#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Owned localhost PUBLICDUMMY client; real independent hashlib vs Mbed TLS.
No native endpoint, device/status/private files, media acquisition or NVR.
No Authorization/password/HA1/response/session/nonce/URI/payload hashes logged.
"""
import hashlib
import json
from pathlib import Path
import re
import select
import socket
import subprocess
import sys
import threading
import time

CHECKS = 0
MAX_NAL = 1048576
EXPECTED = bytes([0x65]) + bytes((i * 31 ^ (i >> 8)) & 255 for i in range(1, MAX_NAL))
EXPECTED_P = bytes([0x41]) + EXPECTED[1:]


class Failure(Exception):
    pass


def check(condition):
    global CHECKS
    CHECKS += 1
    if not condition:
        raise Failure('scalar_check=' + str(CHECKS))


def bzip_crc(data):
    # Raw public synthetic NAL integrity only, not cryptography/credential hash.
    table = []
    for i in range(256):
        c = i << 24
        for _ in range(8):
            c = ((c << 1) ^ (0x04c11db7 if c & 0x80000000 else 0)) & 0xffffffff
        table.append(c)
    c = 0xffffffff
    for value in data:
        c = ((c << 8) ^ table[((c >> 24) ^ value) & 255]) & 0xffffffff
    return c ^ 0xffffffff


RAW_CRC = bzip_crc(EXPECTED)
RAW_CRC_P = bzip_crc(EXPECTED_P)


def rfc_crypto():
    # Public RFC2617/7616 vectors, never logged or separately payload-hashed.
    for alg, realm, password, nonce, cnonce, expected in (
        ('MD5', 'testrealm@host.com', 'Circle Of Life', 'dcd98b7102dd2f0e8b11d0f600bfb0c093', '0a4f113b', '6629fae49393a05397450978507c4ef1'),
        ('SHA-256', 'http-auth@example.org', 'Circle of Life', '7ypf/xlj9XXwfDPEoM4URrv/xwf94BcCAzFZH4GiTo0v', 'f2/wE4q74E6zIJEtWaHKaf5wv/H5QzzpXusqGemxURZJ', '753927fa0e85d155564e2e272a28d1802ca10daf4496794697cf8db5856cb6c1')):
        h = hashlib.md5 if alg == 'MD5' else hashlib.sha256
        h1 = h(('Mufasa:' + realm + ':' + password).encode()).hexdigest()
        h2 = h(b'GET:/dir/index.html').hexdigest()
        value = h((h1 + ':' + nonce + ':00000001:' + cnonce + ':auth:' + h2).encode()).hexdigest()
        check(value == expected)


class Server:
    def __init__(self, binary, logs, name, alg='SHA-256', ttl=12000, rate=512, refused=False, chunk=0, begin_mode=0, release_mode=0):
        self.clients = []
        # Only our own loopback port reservation, never socket-status discovery.
        with socket.socket() as reserve:
            reserve.bind(('127.0.0.1', 0))
            self.port = reserve.getsockname()[1]
        self.name = name
        self.log_path = logs / (name + '-server.txt')
        self.log = self.log_path.open('xb')
        self.process = subprocess.Popen([str(binary), alg, str(self.port), str(ttl), str(rate), str(chunk), str(begin_mode), str(release_mode)],
            stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=self.log, bufsize=0)
        self.lines = []
        self.refused = refused
        self.begin_mode = begin_mode
        self.release_mode = release_mode
        if refused:
            self.process.wait(timeout=5)
            out = self.process.stdout.read().decode('ascii')
            self.lines.extend(out.splitlines())
            check(self.process.returncode != 0 and 'READY' not in out)
        else:
            check(bool(select.select([self.process.stdout], [], [], 5)[0]))
            line = self.process.stdout.readline().decode('ascii').strip()
            check(line == 'HOST_ONLY READY port=' + str(self.port))
            self.lines.append(line)

    def client(self, alg='SHA-256', password='PUBLICDUMMY-not-product'):
        client = Client(self.port, alg, password)
        self.clients.append(client)
        return client

    def command(self, cmd):
        self.process.stdin.write(cmd.encode('ascii'))
        self.process.stdin.flush()

    def finish(self):
        try:
            if self.process.poll() is None:
                try:
                    self.command('Q')
                except BrokenPipeError:
                    pass  # owned child can fail-close between poll and pipe write
                self.process.wait(timeout=5)
            rest = self.process.stdout.read().decode('ascii')
            self.lines.extend(rest.splitlines())
            self.process.stdin.close()
            self.process.stdout.close()
            for client in self.clients:
                client.close()
            self.log.close()
            text = self.log_path.read_text() + '\n' + '\n'.join(self.lines)
            # Receipt remains scalar; stdout includes only endpoint and SOURCE counters.
            with (self.log_path.parent / (self.name + '-source.txt')).open('x') as f:
                f.write('\n'.join(self.lines) + '\n')
            source = next(line for line in self.lines if line.startswith('HOST_ONLY SOURCE '))
            self.source = {key: int(value) for key, value in re.findall(r'([A-Za-z_]+)=(-?[0-9]+)', source)}
            summary = next((line for line in text.splitlines() if line.startswith('HOST_ONLY SUMMARY ')), '')
            self.summary = {key: int(value) for key, value in re.findall(r'([A-Za-z_]+)=([0-9]+)', summary)}
            check(self.source['guard_errors'] == 0)
            check(self.source['retained'] == (1 if self.begin_mode in (1, 2, 3, 4) or self.release_mode in (2, 3) else 0))
            check(self.source['next'] == self.source['releases'])
            check(self.source['release_wipes'] == self.source['releases'] and self.source['end_key_mismatch'] == 0)
            check(self.source['demands'] == (1 if self.release_mode in (2, 3) else 0))
            check(self.source['frame_open'] == (1 if self.begin_mode == 4 or self.release_mode in (2, 3) else 0))
            check(self.source['guard_bound'] == 0)
            check(self.source['raw_NAL_CRC'] == RAW_CRC)
            # No secret-sensitive output path; inspect known dangerous payload prefixes.
            for forbidden in ('Authorization:', 'WWW-Authenticate:', 'username=', 'response=', 'nonce=', 'Session:', 'rtsp://', 'received command', 'PUBLICDUMMY-not-product'):
                check(forbidden not in text)
            check(self.process.returncode == (2 if self.refused or self.begin_mode or self.release_mode in (2, 3, 4, 5) else 0))
        finally:
            for client in self.clients:
                client.close()
            if self.process.poll() is None:
                self.process.terminate()  # exact child handle only, no PID discovery
                self.process.wait(timeout=5)


class Client:
    def __init__(self, port, alg, password):
        self.socket = socket.create_connection(('127.0.0.1', port), timeout=3)
        self.socket.settimeout(3)
        self.alg, self.password = alg, password
        self.live = 'rtsp://127.0.0.1:' + str(port) + '/live'
        self.track = self.live + '/track0'
        self.seq = self.nc = 0
        self.challenge = None
        self.session = None
        self.buffer = b''
        self.nal = bytearray()
        self.frames = 0
        self.idr_frames = self.p_frames = 0
        self.media_bytes = 0

    def close(self):
        self.socket.close()

    def wire(self, method, target=None, authenticated=True, nc=None, extra='', body=b'', duplicate=False):
        target = target or self.live
        self.seq += 1
        if method == 'SETUP' and not extra:
            extra = 'Transport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n'
        if method in ('PLAY', 'PAUSE', 'GET_PARAMETER', 'TEARDOWN') and self.session:
            extra += 'Session: ' + self.session + '\r\n'
        auth = ''
        if authenticated:
            check(self.challenge is not None)
            self.nc = self.nc + 1 if nc is None else nc
            nc_value = format(self.nc, '08x')
            cnonce = 'PUBLICDUMMY-client-' + str(self.seq)  # fresh each request
            h = hashlib.md5 if self.alg == 'MD5' else hashlib.sha256
            realm, nonce = self.challenge['realm'], self.challenge['nonce']
            ha1 = h(('PUBLICDUMMY:' + realm + ':' + self.password).encode()).hexdigest()
            ha2 = h((method + ':' + target).encode()).hexdigest()
            response = h((ha1 + ':' + nonce + ':' + nc_value + ':' + cnonce + ':auth:' + ha2).encode()).hexdigest()
            value = 'Digest username="PUBLICDUMMY", realm="' + realm + '", uri="' + target + '", algorithm=' + self.alg + ', qop=auth, nonce="' + nonce + '", cnonce="' + cnonce + '", nc=' + nc_value + ', response="' + response + '"'
            if duplicate:
                value += ', QOP=auth'
            auth = 'Authorization: ' + value + '\r\n'
        if body:
            extra += 'Content-Length: ' + str(len(body)) + '\r\n'
        return (method + ' ' + target + ' RTSP/1.0\r\nCSeq: ' + str(self.seq) + '\r\n' + auth + extra + '\r\n').encode() + body

    def item(self):
        while True:
            if self.buffer.startswith(b'$') and len(self.buffer) >= 4:
                n = int.from_bytes(self.buffer[2:4], 'big') + 4
                check(16 <= n <= 1404)
                if len(self.buffer) >= n:
                    packet, self.buffer = self.buffer[4:n], self.buffer[n:]
                    self.media_bytes += n
                    payload = packet[12:]
                    if payload[0] & 31 == 28:
                        if payload[1] & 0x80:
                            check(not self.nal)
                            self.nal.append((payload[0] & 0xe0) | (payload[1] & 31))
                        self.nal.extend(payload[2:])
                        check(len(self.nal) <= MAX_NAL)
                        if payload[1] & 0x40:
                            check(self.nal[0] in (0x65, 0x41))
                            is_idr = self.nal[0] == 0x65
                            check(bytes(self.nal) == (EXPECTED if is_idr else EXPECTED_P))
                            check(bzip_crc(self.nal) == (RAW_CRC if is_idr else RAW_CRC_P))
                            if is_idr:
                                self.idr_frames += 1
                            else:
                                self.p_frames += 1
                            self.frames += 1
                            self.nal.clear()
                    else:
                        check(payload[0] & 31 in (7, 8))
                    return 'rtp', None
            if self.buffer and not self.buffer.startswith(b'$') and b'\r\n\r\n' in self.buffer:
                pos = self.buffer.index(b'\r\n\r\n') + 4
                head = self.buffer[:pos].decode('ascii')
                rows = head.split('\r\n')
                check(rows[0].startswith('RTSP/1.0 '))
                headers = {}
                for row in rows[1:]:
                    if ':' in row:
                        key, value = row.split(':', 1)
                        headers[key.lower()] = value.strip()
                n = int(headers.get('content-length', '0'))
                check(0 <= n <= 2048)
                if len(self.buffer) >= pos + n:
                    body, self.buffer = self.buffer[pos:pos+n], self.buffer[pos+n:]
                    status = int(rows[0].split(' ')[1])
                    if status == 401:
                        value = headers.get('www-authenticate', '')
                        check(value.startswith('Digest '))
                        challenge = {}
                        for key, quoted, token in re.findall(r'([a-z]+)=(?:"([^"]*)"|([^, ]+))', value):
                            challenge[key] = quoted or token
                        check(challenge['algorithm'] == self.alg and challenge['qop'] == 'auth' and len(challenge['nonce']) == 64)
                        self.challenge, self.nc = challenge, 0
                    if 'session' in headers:
                        self.session = headers['session'].split(';')[0]
                        check(len(self.session) == 64)
                    return 'reply', (status, headers, body)
            try:
                data = self.socket.recv(65536)
            except ConnectionResetError:
                return 'closed', None  # fail-closed server may reset unread inbound bytes
            if not data:
                return 'closed', None
            self.buffer += data
            check(len(self.buffer) <= 2097152)

    def reply(self, expected):
        deadline = time.monotonic() + 4
        while time.monotonic() < deadline:
            kind, value = self.item()
            if kind == 'reply':
                check(value[0] == expected)
                return value
            check(kind != 'closed')
        raise Failure('reply_deadline')

    def send(self, method, **kw):
        self.socket.sendall(self.wire(method, **kw))

    def login(self):
        self.send('DESCRIBE', authenticated=False)
        response = self.reply(401)
        check(not response[2] and 'application/sdp' not in response[1].get('content-type', ''))
        self.send('DESCRIBE')
        response = self.reply(200)
        check(response[2].startswith(b'v=0') and b'NOT-NATIVE' in response[2])

    def play(self):
        self.send('SETUP', target=self.track)
        self.reply(200)
        self.send('PLAY')
        self.reply(200)

    def frame(self):
        target = self.frames + 1
        deadline = time.monotonic() + 4
        while self.frames < target and time.monotonic() < deadline:
            kind, _ = self.item()
            check(kind != 'closed')
        check(self.frames >= target)

    def closed(self):
        deadline = time.monotonic() + 4
        while time.monotonic() < deadline:
            kind, _ = self.item()
            if kind == 'closed':
                return
        raise Failure('close_deadline')


def campaign(binary, logs):
    rfc_crypto()
    completed = []
    for mode in ('BAD-DOMAIN', 'BAD-RNG'):
        s = Server(binary, logs, mode.lower(), mode, refused=True)
        s.finish()
        check(s.source['starts'] == s.source['next'] == 0)
        completed.append(mode)
    for alg in ('SHA-256', 'MD5'):
        s = Server(binary, logs, alg.lower() + '-unauthorized', alg)
        try:
            c = s.client(alg)
            for method, target in (('OPTIONS', '*'), ('DESCRIBE', c.live), ('SETUP', c.track), ('PLAY', c.live)):
                c.send(method, target=target, authenticated=False)
                c.reply(401)
            c.password = 'PUBLICDUMMY-wrong-password'
            c.send('DESCRIBE')
            c.reply(401)
            c.password = 'PUBLICDUMMY-not-product'
            c.send('DESCRIBE', duplicate=True)
            c.reply(401)
            c.close()
        finally:
            s.finish()
        check(s.source['starts'] == s.source['next'] == 0)
        check(s.summary['sdp'] == s.summary['setup'] == s.summary['play'] == s.summary['rtp_calls'] == 0)
        completed.append(alg + '-unauthorized')
        s = Server(binary, logs, alg.lower() + '-normal', alg)
        try:
            c = s.client(alg)
            c.login()
            c.send('OPTIONS')
            c.reply(200)
            # Fragmented and pipelined real TCP control, without crypto mocks.
            wire = c.wire('SETUP', target=c.track)
            c.socket.sendall(wire[:17]);time.sleep(0.005);c.socket.sendall(wire[17:])
            c.reply(200)
            first = c.wire('GET_PARAMETER')
            second = c.wire('PLAY')
            c.socket.sendall(first + second)
            c.reply(200);c.reply(200);c.frame()
            # RR then authenticated empty-body GET_PARAMETER while active media.
            c.socket.sendall(b'$\x01\x00\x08\x80\xc9\x00\x01\x00\x00\x00\x00' + c.wire('GET_PARAMETER'))
            c.reply(200);c.frame()
            c.send('PAUSE');c.reply(200)
            c.send('PLAY');c.reply(200);c.frame()
            # Authenticated nonempty body remains deliberately unsupported.
            c.send('GET_PARAMETER', body=b'PUBLICDUMMY')
            c.closed()
        finally:
            s.finish()
        check(s.source['starts'] >= 1 and s.source['idles'] == s.source['starts'])
        check(s.summary['lease_releases'] >= 2 and s.summary['rtp_bytes'] >= 2 * MAX_NAL)
        completed.append(alg + '-normal')
        s = Server(binary, logs, alg.lower() + '-replay', alg)
        try:
            c = s.client(alg);c.login();c.play();c.frame()
            c.send('GET_PARAMETER', nc=c.nc)  # fresh cnonce, same global nc
            c.closed()
        finally:
            s.finish()
        check(s.source['idles'] == s.source['starts'])
        completed.append(alg + '-replay')
        s = Server(binary, logs, alg.lower() + '-teardown', alg)
        try:
            c = s.client(alg);c.login();c.play();c.frame();c.send('TEARDOWN');c.reply(200);c.closed()
        finally:
            s.finish()
        check(s.source['idles'] == s.source['starts'])
        completed.append(alg + '-teardown')
    # Per-client owner cancel preserves another independent authorized peer.
    s = Server(binary, logs, 'peer-cancel')
    ended = threading.Event()
    thread_errors = []
    try:
        a = s.client();a.login();a.play();a.frame()
        def drain_a():
            try:
                while a.item()[0] != 'closed':
                    pass
            except BaseException as exc:
                thread_errors.append(type(exc).__name__)
            finally:
                ended.set()
        thread = threading.Thread(target=drain_a)
        thread.start()
        b = s.client();b.login();b.play();b.frame()
        check(not ended.is_set())
        s.command('1');check(ended.wait(3));thread.join(timeout=3);check(not thread_errors);b.frame()
    finally:
        s.finish()
    check(s.source['idles'] == s.source['starts'])
    check(s.source['cancel_hits'] == 1)
    completed.append('peer-cancel')
    s = Server(binary, logs, 'real-partial-cancel', chunk=17)
    try:
        c = s.client();c.login();s.command('P');c.play();c.closed()
    finally:
        s.finish()
    check(s.summary['partial_discard'] == 1 and s.summary['lease_releases'] == 1)
    check(s.source['next'] == s.source['releases'] == 1 and s.source['starts'] == s.source['idles'] == 1 and s.source['cancel_hits'] == 1)
    completed.append('real-partial-cancel')
    # Rotation destroys old session/nonce authority; old credentials never resume.
    s = Server(binary, logs, 'rotate')
    try:
        c = s.client();c.login();c.play();c.frame();s.command('G');c.closed()
        d = s.client();d.send('DESCRIBE', authenticated=False);d.reply(401);d.send('DESCRIBE');d.reply(401)
        d.password = 'PUBLICDUMMY-rotated-not-product';d.send('DESCRIBE');d.reply(200);d.play();d.frame()
        s.command('R');d.closed()
    finally:
        s.finish()
    check(s.source['idles'] == s.source['starts'])
    completed.append('rotate-revoke')
    # Peer churn cannot reset global/account attempt quotas.
    s = Server(binary, logs, 'rate-churn', rate=8)
    try:
        closed = 0
        for _ in range(12):
            c = s.client();c.send('DESCRIBE', authenticated=False)
            kind, value = c.item()
            if kind == 'closed':
                closed += 1
            else:
                check(kind == 'reply' and value[0] == 401)
            c.close()
        check(closed > 0)
    finally:
        s.finish()
    check(s.source['starts'] == s.source['next'] == 0 and s.summary['opened'] < 12)
    completed.append('rate-churn')
    # Stalled partial cursor expires at auth TTL, not a widened whole-AU budget.
    s = Server(binary, logs, 'stall-expire', ttl=700)
    try:
        c = s.client();c.socket.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 4096)
        c.login();c.play();time.sleep(0.85);c.closed()
    finally:
        s.finish()
    check(s.summary['lease_releases'] >= 1 and s.source['idles'] == s.source['starts'])
    completed.append('stall-expire')
    # Blocking readiness cannot extend auth or send deadlines after it returns.
    s = Server(binary, logs, 'block-expire', ttl=700)
    try:
        c = s.client();c.login();c.play();time.sleep(0.35);s.command('B');c.closed()
    finally:
        s.finish()
    check(s.source['idles'] == s.source['starts'])
    completed.append('block-expire')
    # All V1 cases stay above unchanged. New real owned-loopback BEGIN boundaries.
    controls = []
    for alg in ('SHA-256', 'MD5'):
        for mode, label in ((1, 'expire-after-open'), (2, 'revoke-at-boundary'),
                            (3, 'cancel-at-boundary'), (4, 'end-reject-retain'),
                            (5, 'expire-before-start-effect')):
            name = alg.lower() + '-begin-' + label
            s = Server(binary, logs, name, alg, ttl=250, begin_mode=mode)
            try:
                c = s.client(alg);c.login();c.play();c.closed()
                check(c.media_bytes == 0 and c.frames == 0)
            finally:
                s.finish()
            check(s.source['attaches'] == s.source['detaches'] == s.source['begins'] == 1)
            check(s.source['next'] == s.source['releases'] == s.source['crc_reads'] == 0)
            check(s.summary['next_calls'] == s.summary['hub_publish_calls'] == s.summary['rtp_calls'] == s.summary['rtp_bytes'] == 0)
            if mode == 5:
                # Bound START hook denies before even the logical-start statistic.
                check(s.source['starts'] == s.source['ends'] == s.source['frame_retained'] == 0)
                check(s.source['source_gate_denied'] == 1 and s.source['idles'] == 1)
                check(s.source['needs_stop'] == s.source['stop_pending'] == s.source['active_present'] == 0)
            else:
                check(s.source['starts'] == s.source['ends'] == s.source['frame_retained'] == 1)
                check(s.source['idles'] == 0 and s.source['needs_stop'] == 1 and s.source['stop_pending'] == 0)
                check(s.source['source_state'] == 3)  # actual DRAINING; flag0 is NOT a STOP acknowledgement
                check(s.summary['boundary_denied'] == s.summary['frame_ends'] == 1)
                check(s.source['active_present'] == (1 if mode == 4 else 0))
            controls.append({'profile': alg, 'boundary': label,
                'attaches': s.source['attaches'], 'begins': s.source['begins'], 'logical_starts': s.source['starts'],
                'source_next': s.source['next'], 'frame_CRC_reads': s.source['crc_reads'],
                'frame_ends': s.source['ends'], 'detaches': s.source['detaches'], 'idle': s.source['idles'],
                'frame_open': s.source['frame_open'], 'retained': s.source['retained'],
                'needs_stop': s.source['needs_stop'], 'stop_pending': s.source['stop_pending'],
                'active_present': s.source['active_present'], 'hub_publish': s.summary['hub_publish_calls'],
                'RTP_calls': s.summary['rtp_calls'], 'source_gate_denied': s.source['source_gate_denied'],
                'result': 'HOST FAIL-CLOSED PASS', 'native': False})
            completed.append(name)
    with (logs / 'BEGIN-CONTROL-TABLE.json').open('x') as f:
        json.dump({'evidence_state': 'HOST ONLY', 'rows': controls, 'STOP_uncertainty_bypassed': False}, f, indent=2)
        f.write('\n')
    release_controls = []
    for alg in ('SHA-256', 'MD5'):
        s = Server(binary, logs, alg.lower() + '-destructive-IDR-P', alg, release_mode=1)
        try:
            c = s.client(alg);c.login();c.play()
            while c.frames < 4:
                c.frame()
            check(c.idr_frames == 1 and c.p_frames >= 3)
            c.send('TEARDOWN');c.reply(200);c.closed()
        finally:
            s.finish()
        check(s.source['forced_nexts'] == s.source['produced_idr'] == s.source['end_idr'] == 1)
        check(s.source['produced_p'] >= 3 and s.source['end_p'] == s.source['produced_p'])
        check(s.source['idles'] == s.source['starts'] == 1 and s.source['release_wipes'] == s.source['next'])
        release_controls.append({'profile': alg, 'boundary': 'destructive-IDR-P', 'next': s.source['next'],
            'release_wipes': s.source['release_wipes'], 'IDR': s.source['produced_idr'], 'P': s.source['produced_p'],
            'END_IDR': s.source['end_idr'], 'END_P': s.source['end_p'], 'forced_nexts': s.source['forced_nexts'],
            'end_key_mismatch': s.source['end_key_mismatch'], 'native': False})
        completed.append(alg + '-destructive-IDR-P')
        for mode, label in ((2, 'clock-after-next'), (3, 'clock-after-release'), (4, 'invalid-keyframe'), (5, 'oversize-packet')):
            s = Server(binary, logs, alg.lower() + '-' + label, alg, release_mode=mode)
            try:
                c = s.client(alg);c.login();c.play();c.closed()
                check(c.media_bytes == 0)
            finally:
                s.finish()
            check(s.source['next'] == s.source['releases'] == s.source['release_wipes'] == 1)
            check(s.summary['rtp_calls'] == s.summary['rtp_bytes'] == 0)
            if mode in (2, 3):
                check(s.source['ends'] == s.source['idles'] == s.source['detaches'] == 0)
                check(s.source['frame_retained'] == 1 and s.source['demands'] == s.source['frame_open'] == 1)
                check(s.summary['hub_publish_calls'] == (1 if mode == 3 else 0))
            else:
                check(s.summary['hub_publish_calls'] == 0 and s.source['ends'] == s.source['detaches'] == s.source['idles'] == 1)
            release_controls.append({'profile': alg, 'boundary': label, 'next': s.source['next'], 'release_wipes': s.source['release_wipes'],
                'END': s.source['ends'], 'idle': s.source['idles'], 'detach': s.source['detaches'],
                'frame_open': s.source['frame_open'], 'demands': s.source['demands'], 'frame_retained': s.source['frame_retained'],
                'hub_publish': s.summary['hub_publish_calls'], 'RTP_calls': s.summary['rtp_calls'], 'native': False})
            completed.append(alg + '-' + label)
    with (logs / 'RELEASE-CONTROL-TABLE.json').open('x') as f:
        json.dump({'evidence_state': 'HOST ONLY', 'rows': release_controls, 'release_is_DMA_stop_receipt': False}, f, indent=2)
        f.write('\n')
    return completed


def main():
    binary, directory = Path(sys.argv[1]).resolve(), Path(sys.argv[2]).resolve()
    directory.mkdir()  # fresh per-build campaign, refuse overwrite
    try:
        cases = campaign(binary, directory)
        result = {'evidence_state': 'HOST ONLY', 'checks': CHECKS, 'cases': cases, 'result': 'PASS',
            'crypto_mocked': False, 'native_executed': False, 'NVR_qualified': False, 'G3': False, 'G8': False,
            'synthetic_raw_NAL_bytes': MAX_NAL, 'synthetic_raw_NAL_CRC_BZIP2': RAW_CRC}
        with (directory / 'RESULT.json').open('x') as f:
            json.dump(result, f, indent=2);f.write('\n')
        print('HOST_ONLY PASS checks=' + str(CHECKS) + ' cases=' + str(len(cases)) + ' crypto_mocked=0 native=0')
    except BaseException as exc:
        # No traceback/argument/data/header/credential dumps; only scalar failure.
        with (directory / 'FAIL.json').open('x') as f:
            json.dump({'result': 'FAIL', 'checks': CHECKS, 'error_type': type(exc).__name__}, f)
        print('HOST_ONLY FAIL checks=' + str(CHECKS) + ' error_type=' + type(exc).__name__)
        raise SystemExit(1)


if __name__ == '__main__':
    main()
