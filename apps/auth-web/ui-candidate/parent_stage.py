#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Parent-only finite transfer of three pinned public UI qualification ELFs.

No private directory, credentials, footage or arbitrary file serving. This is
not a product route; target execution/ownership/recovery stays with the parent.
"""
from pathlib import Path
import argparse, datetime, hashlib, http.server, json, signal, time

ROOT = Path(__file__).resolve().parents[3]
p = argparse.ArgumentParser()
p.add_argument('--launcher-build', required=True, type=Path)
p.add_argument('--output', required=True, type=Path)
args = p.parse_args()
build = args.launcher_build.resolve()
output = args.output.resolve()
if not build.is_relative_to(ROOT / 'local/build/auth-web/native-launch'):
    raise ValueError('canonical launcher build required')
if not output.is_relative_to(ROOT / 'evidence/qualification/qc07f'):
    raise ValueError('fresh scoped public evidence output required')
output.parent.mkdir(parents=True, exist_ok=True)
output.mkdir(mode=0o700)
manifest = json.loads((build / 'manifest.json').read_text())
if manifest['state'] != 'host/native-bootstrap-helper-linked-ARM-not-executed-root/native-proof-absent':
    raise ValueError('successful exact launcher build required')
launcher = build / 'build/auth-web-launch-arm.stripped'
pin = next(row for row in manifest['files'] if row['path'] == 'build/auth-web-launch-arm.stripped')
entries = {'ui-launcher': (launcher, pin['sha256'], pin['bytes'])}
for name, image in [('ui-service', 'web-service-arm.stripped'), ('ui-child', 'credential-child-arm.stripped')]:
    row = manifest['exact_image_inputs'][image]
    entries[name] = (Path(row['path']), row['sha256'], row['bytes'])
files = {}
for name, (path, digest, size) in entries.items():
    data = path.read_bytes()
    if len(data) != size or hashlib.sha256(data).hexdigest() != digest:
        raise ValueError('public image pin mismatch')
    files[name] = data
start = time.monotonic()
receipt = {'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
           'public_only': True, 'camera_peer': '192.168.1.73', 'deadline_seconds': 120,
           'max_requests': 3, 'events': [],
           'images': {n: {'sha256': v[1], 'bytes': v[2]} for n, v in entries.items()}}

class Handler(http.server.BaseHTTPRequestHandler):
    def setup(self):
        super().setup()
        self.connection.settimeout(20)

    def log_message(self, *_):
        pass

    def do_GET(self):
        name = self.path.removeprefix('/')
        if self.client_address[0] != '192.168.1.73' or name not in files:
            self.send_error(404)
            return
        data = files.pop(name)  # exactly one transfer per pinned image; no retry
        self.send_response(200)
        self.send_header('Content-Type', 'application/octet-stream')
        self.send_header('Content-Length', str(len(data)))
        self.send_header('Connection', 'close')
        self.end_headers()
        self.wfile.write(data)
        receipt['events'].append({'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
                                  'image': name, 'bytes_written': len(data)})

server = http.server.HTTPServer(('192.168.1.15', 19561), Handler)
server.timeout = 1
def expired(_signum, _frame):
    raise TimeoutError('finite public stage deadline')
old_handler = signal.signal(signal.SIGALRM, expired)
signal.setitimer(signal.ITIMER_REAL, 120)
print('PUBLIC_PINNED_UI_STAGE_READY max3 deadline120s camera_peer_only=1', flush=True)
try:
    for _ in range(3):
        remaining = 120 - (time.monotonic() - start)
        if remaining <= 0:
            break
        server.timeout = remaining
        server.handle_request()
finally:
    signal.setitimer(signal.ITIMER_REAL, 0)
    signal.signal(signal.SIGALRM, old_handler)
    server.server_close()
    receipt['remaining_images'] = sorted(files)
    receipt['elapsed_seconds'] = time.monotonic() - start
    with (output / 'public-stage.json').open('x') as f:
        json.dump(receipt, f, indent=2)
