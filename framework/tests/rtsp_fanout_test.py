#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Loopback recorded-media test: three decoders plus one stalled consumer."""
import argparse
import datetime
import hashlib
import json
from pathlib import Path
import re
import socket
import subprocess
import threading
import io
from rtsp_capture import Capture

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--server', type=Path, required=True)
    parser.add_argument('--input', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--port', type=int, default=19576)
    parser.add_argument('--send-deadline-ms', type=int, default=100)
    parser.add_argument('--tcp-coalesce', action='store_true')
    args = parser.parse_args()
    if not 1024 <= args.port <= 65535:
        raise ValueError('Port out of range')
    if not 10 <= args.send_deadline_ms <= 2000:
        raise ValueError('Send deadline out of range')
    out = args.output.resolve()
    out.mkdir(mode=0o700)
    server = args.server.resolve()
    media = args.input.resolve()
    server_argv = [str(server), str(media), str(args.port), str(args.send_deadline_ms), str(int(args.tcp_coalesce))]
    clients = []
    slow = None
    report = {'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
              'host_only_recorded_media_fixture': True, 'actual_camera_test': False,
              'server_argv': server_argv, 'server_sha256': digest(server),
              'media': str(media), 'media_sha256': digest(media), 'clients': []}
    with (out / 'server.log').open('xb') as log:
        proc = subprocess.Popen(server_argv, stderr=subprocess.PIPE)
        drainer = None
        try:
            first = proc.stderr.readline()
            log.write(first)
            assert b'SHARED RTSP LAB' in first
            def drain():
                while chunk := proc.stderr.read1(8192):
                    log.write(chunk)
            drainer = threading.Thread(target=drain, daemon=True)
            drainer.start()
            # Reading only RTSP replies, never RTP, deliberately stalls video.
            slow = socket.socket()
            slow.settimeout(3)
            slow.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 1024)
            slow.connect(('127.0.0.1', args.port))
            capture = Capture(io.BytesIO(), io.BytesIO(), io.StringIO())
            session = ''
            for seq, method in enumerate(['OPTIONS', 'SETUP', 'PLAY'], 1):
                extra = 'Transport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n' if method == 'SETUP' else ''
                if method == 'PLAY':
                    assert session
                    extra = f'Session: {session}\r\n'
                request = f'{method} rtsp://127.0.0.1:{args.port}/live RTSP/1.0\r\nCSeq: {seq}\r\n{extra}\r\n'
                slow.sendall(request.encode())
                returned = capture.reply(slow, seq)
                if method == 'SETUP':
                    session = returned
            for i in range(3):
                target = out / f'client{i}.framemd5'
                argv = ['ffmpeg', '-hide_banner', '-loglevel', 'warning', '-err_detect', 'explode',
                        '-rtsp_transport', 'tcp', '-analyzeduration', '1000000', '-probesize', '4096',
                        '-i', f'rtsp://127.0.0.1:{args.port}/live', '-frames:v', '24',
                        '-fps_mode', 'passthrough', '-f', 'framemd5', '-n', str(target)]
                stream = (out / f'client{i}.log').open('xb')
                decoder = subprocess.Popen(argv, stdout=stream, stderr=subprocess.STDOUT)
                clients.append((decoder, stream))
                report['clients'].append({'argv': argv, 'output': str(target)})
            for i, (decoder, stream) in enumerate(clients):
                assert decoder.wait(timeout=20) == 0
                stream.close()
                target = out / f'client{i}.framemd5'
                text = target.read_text()
                rows = [r for r in text.splitlines() if r and not r.startswith('#')]
                assert '#dimensions 0: 1280x720' in text and len(rows) == 24
                report['clients'][i].update(decoded_frames=24, sha256=digest(target))
        finally:
            if slow is not None:
                slow.close()
            for decoder, stream in clients:
                if decoder.poll() is None:
                    decoder.terminate()
                    decoder.wait(timeout=3)
                if not stream.closed:
                    stream.close()
            proc.terminate()
            try:
                proc.wait(timeout=3)
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.wait(timeout=3)
            if drainer is not None:
                drainer.join(timeout=3)
                assert not drainer.is_alive()
            else:
                log.write(proc.stderr.read())
    text = (out / 'server.log').read_text()
    assert proc.returncode == 0
    assert 'disconnected on bounded send failure' in text
    broadcasts = [int(v) for v in re.findall(r'published consumers=(\d+)', text)]
    assert sum(v >= 3 for v in broadcasts) >= 10
    assert 'AddressSanitizer' not in text and 'runtime error:' not in text
    report.update(all_passed=True, total_decoded_frames=72,
                  producer_calls=len(re.findall(r'FIXTURE PRODUCER frame=', text)),
                  broadcasts_with_three_or_more_consumers=sum(v >= 3 for v in broadcasts),
                  stalled_consumer_disconnected=True, authentication_implemented=False)
    with (out / 'manifest.json').open('x') as file:
        json.dump(report, file, indent=2)
    print('PASS loopback shared producer: three24-frame decoders survive stalled peer;', out)

if __name__ == '__main__':
    main()
