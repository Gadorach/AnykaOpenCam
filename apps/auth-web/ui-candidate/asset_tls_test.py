#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Real TLS asset/route checks using the exact canonical PUBLICDUMMY fixture helpers."""
from pathlib import Path
import datetime,hashlib,json,os,ssl,sys,traceback
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];SERVICE=ROOT/'apps/auth-web/service'
sys.path.insert(0,str(HERE));from build import owned_output
sys.path.insert(0,str(SERVICE.parent));from build_support import digest
if len(sys.argv)!=3:raise ValueError('candidate build and fresh test output required')
sys.argv[1]=str(owned_output(sys.argv[1]));sys.argv[2]=str(owned_output(sys.argv[2]))
pins=json.loads((HERE/'snapshot/manifest.json').read_text())
script=HERE/'snapshot/canonical-service/host_https_test.py'
if digest(script)!=pins['canonical_inputs']['host_https_test.py']['sha256']:raise RuntimeError('fixture helpers drift')
source=script.read_text();marker='try:\n check(os.getuid()!=0,\'host-nonroot-identity\''
prefix=source[:source.index(marker)]
g={'__name__':'__ui_asset_fixture__','__file__':str(SERVICE/'host_https_test.py')}
exec(compile(prefix,str(script),'exec'),g)
check=g['check'];run=g['run'];Service=g['Service'];F=g['F'];R=g['R'];services=g['services']
manifest=g['manifest'];manifest['scope']='ACTUAL TLS PUBLICDUMMY fixed UI only; canonical fixture helpers reused'
manifest['fixture_helper_sha256']=digest(script);manifest['asset_snapshot_sha256']=digest(HERE/'snapshot/manifest.json')
wire_records=[]
def request_bytes(s,path,cookie=None,host=None):
 stream=s.tls();start=g['time'].monotonic()
 try:
  wire=s.wire('GET',path,cookie=cookie)
  if host is not None:wire=wire.replace(('Host: 127.0.0.1:'+str(s.port)).encode(),('Host: '+host).encode())
  stream.sendall(wire);out=bytearray()
  while True:
   part=stream.recv(8193-len(out))
   if not part:break
   out.extend(part)
   if len(out)>8192:raise AssertionError('full HTTP response exceeded8192')
  head,body=bytes(out).split(b'\r\n\r\n',1)
  lines=head.decode('ascii').split('\r\n');status=int(lines[0].split(' ')[1]);headers={}
  for line in lines[1:]:
   key,value=line.split(':',1);key=key.lower()
   if key in headers:raise AssertionError('duplicate response header')
   headers[key]=value.strip()
  return status,headers,body,len(out),g['time'].monotonic()-start
 finally:stream.close()
def asset(s,row,path,cookie=None):
 status,headers,body,wire_bytes,elapsed=request_bytes(s,path,cookie)
 expected=(HERE/'snapshot/assets'/row['name']).read_bytes()
 check(status==200,'asset-status',route=path)
 check(body==expected and hashlib.sha256(body).hexdigest()==row['sha256'],'asset-source-v3-byte-identical',route=path)
 check(headers.get('content-type')==row['mime'],'asset-mime',route=path)
 check(headers.get('content-security-policy')==pins['csp'],'asset-exact-csp',route=path)
 check(headers.get('cache-control')=='no-store' and headers.get('x-content-type-options')=='nosniff' and
       headers.get('referrer-policy')=='no-referrer' and headers.get('connection')=='close','asset-security-headers',route=path)
 check(int(headers.get('content-length','-1'))==len(body) and wire_bytes==row['full_wire_bytes'] and wire_bytes<=8192,
       'asset-full-response-bound',route=path,wire_bytes=wire_bytes)
 check('set-cookie' not in headers and 'content-encoding' not in headers,'asset-no-token-or-gzip',route=path)
 wire_records.append({'route':path,'body_bytes':len(body),'wire_bytes':wire_bytes,'sha256':row['sha256'],'elapsed_seconds':elapsed})
try:
 check(os.getuid()==1000 and os.getgid()==1000,'HOST-PUBLICDUMMY-existing-UID1000',uid=os.getuid(),gid=os.getgid())
 run(['openssl','version','-a'],'openssl-version')
 run(['openssl','req','-new','-x509','-newkey','ec','-pkeyopt','ec_paramgen_curve:P-256','-sha256','-nodes',
  '-keyout',F/'PUBLICDUMMY-server.key','-out',F/'PUBLICDUMMY-server.crt','-days','1',
  '-subj','/CN=PUBLICDUMMY-HOST-ONLY','-addext','subjectAltName=IP:127.0.0.1'],'public-fixture-certificate-create')
 os.chmod(F/'PUBLICDUMMY-server.crt',0o600);os.chmod(F/'PUBLICDUMMY-server.key',0o600)
 s=Service('UI-fixed-assets')
 for row in pins['assets']:
  paths=['/','/index.html'] if row['name']=='index.html' else ['/'+row['name']]
  for path in paths:asset(s,row,path)
 for path in ['/app.js','/ui_assets.h','/snapshot/assets/index.html','/vendor/mongoose/mongoose.c',
              '/directory/','/../index.html','//app.mjs','/index','/API/session','/api/media/v1/live/PUBLICDUMMY-resource']:
  status,_,_,_,_=request_bytes(s,path);check(status==401,'unknown-static-no-public-file-route',route=path)
 for path in ['/app.mjs?PUBLICDUMMY=1','/%2e%2e/index.html','/app.mjs%00','http://PUBLICDUMMY@127.0.0.1/index.html']:
  status,_,_,_,_=request_bytes(s,path);check(status==400,'invalid-query-escape-userinfo-denied',route=path)
 status,_,_,_,_=request_bytes(s,'/',host='wrong.invalid');check(status==400,'asset-exact-host-required')
 status,headers,payload,elapsed,old_cookie,old_csrf=s.login()
 check(status==200 and payload['authenticated'],'UI-real600k-login',elapsed_seconds=elapsed)
 cookie=headers['Set-Cookie'].split(';',1)[0];csrf=payload['csrf']
 for path in ['/api/status','/api/capabilities']:
  status,_,payload,_,_=request_bytes(s,path,cookie);data=json.loads(payload)
  check(status==200,'API-preserved-after-assets',route=path)
  if path.endswith('capabilities'):check(data['browser_video'] is False and data['control'] is False,'media-and-controls-remain-disabled')
 asset(s,pins['assets'][0],'/',cookie)
 for path in ['/app.js','/ui_assets.h','/../index.html','/directory/','/api/media/v1/live/PUBLICDUMMY-resource']:
  status,_,payload,_,_=request_bytes(s,path,cookie);check(status==501 and json.loads(payload)['error']=='unsupported_unqualified',
    'authenticated-unknown-no-new-effect',route=path)
 check(s.request('POST','/api/login',g['password'],old_cookie,old_csrf,'public-fixture-user')[0]==403,'asset-campaign-challenge-replay-denied')
 check(s.request('POST','/api/logout',cookie=cookie,csrf=csrf)[0]==200,'asset-campaign-logout')
 check(s.request('GET','/api/status',cookie=cookie)[0]==401,'asset-campaign-old-cookie-denied')
 asset(s,pins['assets'][-1],'/player.css')
 manifest['ui_summary']=s.finish();manifest['wire_measurements']=wire_records
 manifest['state']='real-TLS-v3-assets-API501-PASS-HOST-only'
except BaseException:
 manifest['state']='failed';manifest['failure']=traceback.format_exc();print(manifest['failure'])
finally:
 for s in services:
  if s.proc.poll() is None:
   try:s.proc.terminate();s.proc.wait(timeout=5)
   except BaseException:s.proc.kill();s.proc.wait(timeout=3)
 for p in (R/'logs').iterdir():
  if p.is_file() and (g['password'] in p.read_bytes() or g['wrong'] in p.read_bytes()):manifest['state']='failed';manifest['log_scan_failure']=p.name
 manifest['checks']=g['checks'];manifest['ended']=datetime.datetime.now(datetime.timezone.utc).isoformat()
 manifest['python_ssl']=ssl.OPENSSL_VERSION
 manifest['host_limits']={'uid':os.getuid(),'gid':os.getgid(),'memlock':g['resource'].getrlimit(g['resource'].RLIMIT_MEMLOCK)}
 manifest['build_inputs']=[{'path':str(p),'sha256':digest(p)} for p in [g['build']/'web-service-host',g['build']/'credential-child-host']]
 manifest['no_private_fixture_material_hashes']=True
 with (R/'manifest.json').open('x') as f:json.dump(manifest,f,indent=2)
 print(manifest['state'],g['checks'])
if manifest['state']=='failed':raise SystemExit(1)
