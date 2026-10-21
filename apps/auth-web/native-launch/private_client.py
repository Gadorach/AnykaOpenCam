#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Parent's bounded private HTTPS campaign; NO tokens/passwords in stdout/logs.
Agent runs this only against owned PUBLICDUMMY host loopback fixtures.
"""
from pathlib import Path
import argparse,http.client,json,os,ssl,stat,struct,sys,time
p=argparse.ArgumentParser();p.add_argument('--private-directory',type=Path,required=True)
p.add_argument('--public-host-fixture',action='store_true');p.add_argument('--after-owner-revoke',action='store_true')
p.add_argument('--static-ui-assets',action='store_true')
args=p.parse_args();budget=time.monotonic()+140;phase=0
root=args.private_directory.resolve();s=root.lstat()
if not stat.S_ISDIR(s.st_mode) or s.st_uid!=os.getuid() or stat.S_IMODE(s.st_mode)!=0o700:raise SystemExit('private client directory policy denied')
def read_private(name,maximum):
 fd=os.open(root/name,os.O_RDONLY|os.O_NOFOLLOW|os.O_CLOEXEC)
 try:
  s=os.fstat(fd)
  if not stat.S_ISREG(s.st_mode) or s.st_uid!=os.getuid() or s.st_nlink!=1 or stat.S_IMODE(s.st_mode)!=0o600 or not 0<s.st_size<=maximum:raise ValueError('policy')
  value=os.read(fd,maximum+1)
  if len(value)!=s.st_size:raise ValueError('length')
  return value
 finally:os.close(fd)
try:
 config=read_private('config.bin',768)
 if len(config)!=768 or struct.unpack_from('<IIII',config)!=(0x41575331,2,768,1):raise ValueError('ABI')
 authority=config[144:272].split(b'\0',1)[0].decode('ascii');origin=config[272:432].split(b'\0',1)[0].decode('ascii')
 account=config[624:688].split(b'\0',1)[0].decode('ascii');address,port=authority.rsplit(':',1)
 if args.public_host_fixture and address!='127.0.0.1':raise ValueError('public fixtures restricted host')
 if not args.public_host_fixture and (address!='192.168.1.73' or port!='18443'):raise ValueError('native address policy')
 password=bytearray(read_private('operator.password',128))
 if not 12<=len(password)<=128:raise ValueError('length')
 # Scoped client trust is the parent's exact OOB-provisioned public certificate,
 # not global CA installation, warning bypass or public-trust assertion.
 cert=read_private('cert.pem',8192)
 context=ssl.create_default_context(cadata=cert.decode('ascii'));context.check_hostname=True
 def request(method,path,body=None,cookie=None,csrf=None,extra=None):
  global phase
  phase+=1
  if time.monotonic()>=budget:raise TimeoutError('finite campaign')
  connection=http.client.HTTPSConnection(address,int(port),context=context,timeout=37)
  headers={'Host':authority,'Connection':'close'}
  if cookie:headers['Cookie']=cookie
  if csrf:headers['X-CSRF-Token']=csrf
  if method=='POST':headers['Origin']=origin
  if extra:headers.update(extra)
  try:
   connection.request(method,path,body=body if body is not None else b'',headers=headers)
   response=connection.getresponse();data=response.read(4097)
   if len(data)>4096:raise ValueError('oversize response')
   return response.status,dict(response.getheaders()),json.loads(data)
  finally:connection.close()
  
 def static_ui_assets():
  # Public source assets only; never hash a response from an auth/media route.
  # The fixed source snapshot is not a URI/path supplied by the camera/client.
  import hashlib
  snapshot=Path(__file__).resolve().parents[1]/'ui-candidate/serialized/snapshot'
  pins=json.loads((snapshot/'manifest.json').read_text())
  names=('index.html','bootstrap.js','player.css','contract.mjs','session.mjs',
         'wire.mjs','sample.mjs','player.mjs','core.mjs','frame-proof.mjs','app.mjs')
  rows={row['name']:row for row in pins['assets']}
  if set(rows)!=set(names):raise ValueError('public asset allowlist')
  for name in names:
   if time.monotonic()>=budget:raise TimeoutError('finite campaign')
   expected=(snapshot/'assets'/name).read_bytes()
   if hashlib.sha256(expected).hexdigest()!=rows[name]['sha256']:raise ValueError('public source pin')
   connection=http.client.HTTPSConnection(address,int(port),context=context,timeout=37)
   try:
    connection.request('GET','/'+name,headers={'Host':authority,'Connection':'close'})
    response=connection.getresponse();data=response.read(8193)
    if (response.status!=200 or len(data)>7168 or data!=expected or
        response.getheader('Content-Type')!=rows[name]['mime'] or
        response.getheader('Content-Security-Policy')!=pins['csp'] or
        response.getheader('Cache-Control')!='no-store' or
        response.getheader('X-Content-Type-Options')!='nosniff'):
     raise ValueError('public asset contract')
   finally:connection.close()
  print('native_UI_static_assets=PASS count=11 source_bytes_exact=1 CSP=1 media_unqualified=1',flush=True)
 def require(ok):
  if not ok:raise ValueError('denied')
 if args.static_ui_assets:static_ui_assets()
 require(request('GET','/api/status')[0]==401)
 require(request('GET','/api/media/snapshot',extra={'Range':'bytes=0-63'})[0]==401)
 status,headers,data=request('GET','/api/session');require(status==200 and not data['authenticated'])
 anon=headers['Set-Cookie'].split(';',1)[0];old_csrf=data['csrf']
 status,headers,data=request('POST','/api/login',password,anon,old_csrf,
  {'X-Account':account,'Content-Type':'application/octet-stream'})
 require(status==200 and data['authenticated'])
 cookie=headers['Set-Cookie'].split(';',1)[0];csrf=data['csrf']
 require(cookie!=anon and csrf!=old_csrf)
 require(request('GET','/api/status',cookie=cookie)[0]==200)
 require(request('GET','/api/capabilities',cookie=cookie)[0]==200)
 require(request('GET','/api/media/live',cookie=cookie)[0]==501)
 require(request('POST','/api/logout',b'',cookie,'0'*64)[0]==403)
 require(request('POST','/api/logout',b'',cookie,csrf)[0]==200)
 require(request('GET','/api/status',cookie=cookie)[0]==401)
 require(request('POST','/api/login',password,anon,old_csrf,
  {'X-Account':account,'Content-Type':'application/octet-stream'})[0]==403)
 # One additional real600k abandoned known-user attempt; retain exclusive CPU
 # bound and request deadline. Wait40s covers the original CPU deadline, not a
 # weakened work factor. Native first campaign stays finite and small.
 # Preserve reviewed >=1s admission spacing; do not mistake a429 throttle
 # receipt for an admitted KDF/HTTP-wait-expiry observation on faster hosts.
 time.sleep(1.05)
 status,headers,data=request('GET','/api/session');require(status==200)
 anon=headers['Set-Cookie'].split(';',1)[0]
 try:
  request('POST','/api/login',password,anon,data['csrf'],
   {'X-Account':account,'Content-Type':'application/octet-stream','X-Login-Wait-Ms':'25'})
  raise ValueError('expired login unexpectedly responded')
 except http.client.RemoteDisconnected:pass
 # No need to expose a cookie or inspect target processes to observe usable API.
 deadline=time.monotonic()+(2 if args.public_host_fixture else 40)
 while time.monotonic()<deadline:
  require(request('GET','/api/status')[0]==401)
  time.sleep(1)
 # Actual next login proves discard/drain recovery, not just socket liveness.
 status,headers,data=request('GET','/api/session');require(status==200)
 status,headers,data=request('POST','/api/login',password,headers['Set-Cookie'].split(';',1)[0],data['csrf'],
  {'X-Account':account,'Content-Type':'application/octet-stream'})
 require(status==200 and data['authenticated'])
 if args.after_owner_revoke:
  # Parent uses its OWN bootstrap pidfd SIGUSR1 after this nonsecret marker.
  # This client has no PID/exe/command selector and cannot reach the owner FD7.
  print('private_campaign_owner_revoke_window=1',flush=True);time.sleep(3)
  require(request('GET','/api/status',cookie=headers['Set-Cookie'].split(';',1)[0])[0]==401)
 else:
  require(request('POST','/api/logout',b'',headers['Set-Cookie'].split(';',1)[0],data['csrf'])[0]==200)
 password[:]=b'\0'*len(password)
 print('private_HTTPS_campaign=PASS protected_status=1 full600k=1 logout_replay=1 discard_drain=1 media_unqualified=1')
except BaseException as error:
 # Never echo exception/request/header/body/config/secret representations.
 if 'password' in locals():password[:]=b'\0'*len(password)
 print('private_HTTPS_campaign=FAIL details_redacted=1 phase='+str(phase)+' error_class='+type(error).__name__);raise SystemExit(1)
