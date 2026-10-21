#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Finite loopback-only WEB-01 fixture; explicit files, no directory/raw serving."""
from pathlib import Path
from http.server import HTTPServer,BaseHTTPRequestHandler
from urllib.parse import urlsplit
import argparse,json,time
p=argparse.ArgumentParser();p.add_argument('--fixture',type=Path,required=True);p.add_argument('--port',type=int,default=19620)
p.add_argument('--seconds',type=int,default=90);a=p.parse_args()
if not 1<=a.seconds<=180:raise ValueError('finite fixture budget')
root=a.fixture.resolve();source=Path(__file__).resolve().parent
manifest=json.loads((root/'manifest.json').read_text());allowed={}
for name in ['init.mp4','manifest.json',*[s['file'] for s in manifest['samples']]]:
 if Path(name).name!=name:raise ValueError('fixture name')
 allowed['/'+name]=(root/name,'application/json' if name.endswith('.json') else 'video/mp4')
for name,mime in [('player.html','text/html'),('player.js','text/javascript'),('style.css','text/css')]:allowed['/'+name]=(source/name,mime)
allowed['/']=allowed['/player.html']
class Handler(BaseHTTPRequestHandler):
 def log_message(self,*args):pass
 def do_GET(self):
  item=allowed.get(urlsplit(self.path).path)
  if not item:self.send_error(404);return
  path,mime=item;data=path.read_bytes()
  self.send_response(200);self.send_header('Content-Type',mime);self.send_header('Content-Length',str(len(data)))
  self.send_header('Cache-Control','no-store');self.send_header('X-Content-Type-Options','nosniff')
  self.send_header('Content-Security-Policy',"default-src 'self'; script-src 'self'; style-src 'self'; media-src blob: 'self'; connect-src 'self'")
  self.end_headers();self.wfile.write(data)
server=HTTPServer(('127.0.0.1',a.port),Handler);server.timeout=.25
print('WEB01 finite retained hardware fixture loopback only; not a camera bridge/live/authentication endpoint',flush=True)
deadline=time.monotonic()+a.seconds
try:
 while time.monotonic()<deadline:server.handle_request()
finally:server.server_close()
