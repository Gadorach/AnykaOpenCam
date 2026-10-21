#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Finite actual TLS/socket/library/process campaign, owned loopback PUBLIC DUMMY.
No private/camera/guest input. No secrets, cookie/CSRF or fixture key hashes logged.
"""
from pathlib import Path
import datetime, fcntl, hashlib, http.client, json, os, resource, signal, socket, ssl, struct, subprocess, sys, time, traceback
campaign_deadline=time.monotonic()+180
def budget():
 if time.monotonic()>=campaign_deadline:raise TimeoutError('finite 180s measured-cost campaign budget exhausted')
A=Path(__file__).resolve().parent;sys.path.insert(0,str(A.parent))
from build_support import resolve_run,fresh_run
build_run=resolve_run(sys.argv[1],kind='service');build=build_run/'build'
name=sys.argv[2]
R=fresh_run(name,'service');(R/'logs').mkdir();(R/'PUBLICDUMMY-fixtures').mkdir(mode=0o700)
F=R/'PUBLICDUMMY-fixtures';events=[];commands=[];services=[];checks=0
env={'PATH':'/usr/bin:/bin','LC_ALL':'C','TMPDIR':str(R),'TEMP':str(R),'TMP':str(R)}
password=b'PUBLIC-DUMMY-NOT-A-PRODUCT-PASSWORD';wrong=b'PUBLIC-DUMMY-WRONG-PASSWORD-ONLY'
salt=b'PUBLIC-DUMMY-001';dummy_salt=b'PUBLIC-DUMMY-002'
material=lambda s:struct.pack('<II',1,600000)+s+hashlib.pbkdf2_hmac('sha256',password,s,600000,32)
credentials=material(dummy_salt)+material(salt)
def check(ok,label,**metadata):
 global checks
 checks+=1;events.append({'check':label,'pass':bool(ok),**metadata})
 if not ok:raise AssertionError(label)
 budget()
def run(args,label):
 record={'argv':list(map(str,args)),'cwd':str(R),'started':datetime.datetime.now(datetime.timezone.utc).isoformat(),
         'log':str(R/'logs'/(label+'.txt'))};commands.append(record)
 with Path(record['log']).open('xb') as f:
  p=subprocess.run(record['argv'],cwd=R,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=20)
 record['exit']=p.returncode
 check(p.returncode==0,label)
def private(name,data):
 path=F/name
 fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
 try:os.write(fd,data)
 finally:os.close(fd)
 return path
class Service:
 def __init__(self,label,absolute=60,idle=30,corrupt=None,fault=None,response=35,binary_override=None,abi=2,runtime=180):
  self.label=label;self.started=time.monotonic();self.high=[];self.control=None
  sock=socket.socket();sock.bind(('127.0.0.1',0));self.port=sock.getsockname()[1];sock.close()
  authority=f'127.0.0.1:{self.port}';self.origin='https://'+authority
  text=lambda s,n:s.encode()+bytes(n-len(s.encode()))
  config=struct.pack('<IIII',0x41575331,abi,768,1)+text(self.origin,128)+text(authority,128)+text(self.origin,160)
  config+=b''.join(text(s,64) for s in ['public-fixture-device','public-fixture-sensor','public-fixture-profile','public-fixture-user'])
  epoch=int(time.monotonic_ns());config+=struct.pack('<QQQQQQ',epoch,epoch+1,1,absolute*10**9,idle*10**9,runtime*10**9)
  config+=(struct.pack('<Q',int(response*10**9))+bytes(24)) if abi==2 else bytes(32)
  assert len(config)==768
  paths=[F/'PUBLICDUMMY-server.crt',F/'PUBLICDUMMY-server.key',private(label+'-PUBLICDUMMY-credential.bin',credentials),private(label+'-config.bin',config)]
  if corrupt=='cert':paths[0]=private(label+'-invalid-cert',b'PUBLICDUMMY INVALID CERTIFICATE\n')
  if corrupt=='credential':paths[2]=private(label+'-invalid-credential',credentials[:4]+bytes(4)+credentials[8:])
  for path in paths:
   fd=os.open(path,os.O_RDONLY|os.O_NOFOLLOW);high=fcntl.fcntl(fd,fcntl.F_DUPFD_CLOEXEC,32);os.close(fd);self.high.append(high)
  self.control,peer=socket.socketpair(socket.AF_UNIX,socket.SOCK_SEQPACKET)
  self.control.settimeout(3);high=fcntl.fcntl(peer.fileno(),fcntl.F_DUPFD_CLOEXEC,32);peer.close();self.high.append(high)
  def setup():
   for source,target in zip(self.high,range(3,8)):os.dup2(source,target,inheritable=True)
   for fd in self.high:os.close(fd)
  self.log_path=R/'logs'/(label+'-service.txt');self.log=self.log_path.open('xb')
  binary=binary_override or build/('web-service-host' if fault is None else 'web-service-host-fault'+str(fault))
  self.proc=subprocess.Popen([str(binary),str(build/'credential-child-host')],
    cwd=R,env={},stdin=subprocess.DEVNULL,stdout=self.log,stderr=subprocess.STDOUT,close_fds=False,preexec_fn=setup)
  services.append(self)
  commands.append({'argv':[str(binary),str(build/'credential-child-host')],
    'private_inputs':'fixed FDs 3..7 only; no data/hash in manifest','host_pid':self.proc.pid,'cwd':str(R),'environment':{},'log':str(self.log_path)})
  for fd in self.high:os.close(fd)
  self.context=ssl.create_default_context(cafile=str(F/'PUBLICDUMMY-server.crt'))
  self.context.check_hostname=True
  deadline=time.monotonic()+6
  while time.monotonic()<deadline:
   if b'web_ready=1' in self.log_path.read_bytes():break
   if self.proc.poll() is not None:break
   time.sleep(0.01)
  if not corrupt and fault is None:check(b'web_ready=1' in self.log_path.read_bytes(),label+'-strict-ready')
 def metadata(self):
  # Current task forbids proc inspection. Scoped exit getrusage/heap counters
  # remain in the exact owned service log; no host/native RSS inference here.
  return {'proc_inspection':False,'owned_service_running':self.proc.poll() is None,
          'resource_evidence':'redacted exit getrusage/heap summary; no post-exec VmRSS claim'}
 def tls(self,tls12=False):
  # Stay below the production accept bucket except explicit socket-cap tests.
  time.sleep(0.075)
  context=self.context
  if tls12:
   context=ssl.create_default_context(cafile=str(F/'PUBLICDUMMY-server.crt'));context.maximum_version=ssl.TLSVersion.TLSv1_2
   context.set_ciphers('ECDHE-ECDSA-CHACHA20-POLY1305')
  raw=socket.create_connection(('127.0.0.1',self.port),timeout=4)
  stream=context.wrap_socket(raw,server_hostname='127.0.0.1');stream.settimeout(8)
  return stream
 def wire(self,method,path,body=b'',cookie=None,csrf=None,account=None,origin=True,extra=''):
  headers=f'{method} {path} HTTP/1.1\r\nHost: 127.0.0.1:{self.port}\r\nConnection: close\r\n'
  if cookie:headers+='Cookie: '+cookie+'\r\n'
  if csrf:headers+='X-CSRF-Token: '+csrf+'\r\n'
  if account:headers+='X-Account: '+account+'\r\nContent-Type: application/octet-stream\r\n'
  if method=='POST':
   headers+=f'Content-Length: {len(body)}\r\n'
   if origin:headers+='Origin: '+(self.origin if origin is True else origin)+'\r\n'
  return (headers+extra+'\r\n').encode()+body
 def request(self,method,path,body=b'',cookie=None,csrf=None,account=None,origin=True,extra='',tls12=False):
  stream=self.tls(tls12);start=time.monotonic()
  try:
   stream.sendall(self.wire(method,path,body,cookie,csrf,account,origin,extra))
   reply=http.client.HTTPResponse(stream);reply.begin();payload=reply.read(8193)
   return reply.status,dict(reply.getheaders()),json.loads(payload),time.monotonic()-start
  finally:stream.close()
 def challenge(self):
  status,headers,payload,_=self.request('GET','/api/session');check(status==200 and not payload['authenticated'],'anonymous-challenge')
  cookie=headers['Set-Cookie'].split(';',1)[0]
  check(all(s in headers['Set-Cookie'] for s in ['__Host-camera=','Secure','HttpOnly','SameSite=Strict','Path=/']),'cookie-attributes')
  return cookie,payload['csrf']
 def login(self,p=password,account='public-fixture-user'):
  cookie,csrf=self.challenge()
  status,headers,payload,elapsed=self.request('POST','/api/login',p,cookie,csrf,account)
  return status,headers,payload,elapsed,cookie,csrf
 def control_command(self,op,generation,mat=b''):
  self.control.send(struct.pack('<IIQ',op,len(mat),generation)+mat)
  response=self.control.recv(8);check(response==struct.pack('<II',op,0),'private-owner-command-ack',op=op)
 def wait_job(self,count=1):
  until=time.monotonic()+3
  while time.monotonic()<until and self.log_path.read_bytes().count(b'credential_job_admitted=1')<count:
   if self.proc.poll() is not None:break
   time.sleep(0.002)
  check(self.log_path.read_bytes().count(b'credential_job_admitted=1')>=count,'actual-submitted-credential-job-receipt')
 def wait_drained(self,count=1):
  until=time.monotonic()+4
  while time.monotonic()<until and self.log_path.read_bytes().count(b'credential_job_drained=1')<count:
   if self.proc.poll() is not None:break
   time.sleep(0.005)
  check(self.proc.poll() is None and self.log_path.read_bytes().count(b'credential_job_drained=1')>=count,
    'actual-wiped-job-drain-service-still-usable',count=count)
 def finish(self,expected=0):
  if self.proc.poll() is None:self.proc.send_signal(signal.SIGTERM)
  try:code=self.proc.wait(timeout=5)
  except subprocess.TimeoutExpired:self.proc.kill();code=self.proc.wait(timeout=3);raise AssertionError('owned service termination timeout')
  self.log.close();self.control.close()
  data=self.log_path.read_bytes()
  check(code==expected,self.label+'-owned-exit',exit=code,elapsed_seconds=time.monotonic()-self.started)
  if b'web_ready=1' in data:check(b'child_retirement=observed' in data,self.label+'-actual-child-wait-receipt')
  check(password not in data and wrong not in data and b'-----BEGIN' not in data,self.label+'-redacted-log')
  return data.decode()
manifest={'started':datetime.datetime.now(datetime.timezone.utc).isoformat(),'scope':'OWNED LOOPBACK PUBLIC DUMMY ONLY canonical-source baseline model',
 'target_operations':False,'all_product_flags':False,'commands':commands,'events':events}
try:
 check(os.getuid()!=0,'host-nonroot-identity',uid=os.getuid(),gid=os.getgid())
 run(['openssl','version','-a'],'openssl-version')
 run(['openssl','req','-new','-x509','-newkey','ec','-pkeyopt','ec_paramgen_curve:P-256','-sha256','-nodes',
  '-keyout',F/'PUBLICDUMMY-server.key','-out',F/'PUBLICDUMMY-server.crt','-days','1',
  '-subj','/CN=PUBLICDUMMY-HOST-ONLY','-addext','subjectAltName=IP:127.0.0.1'],'public-fixture-certificate-create')
 os.chmod(F/'PUBLICDUMMY-server.crt',0o600);os.chmod(F/'PUBLICDUMMY-server.key',0o600)
 for kind in ['cert','credential']:
  s=Service('missing-policy-'+kind,corrupt=kind);check(s.proc.wait(timeout=5)==20,kind+'-failclosed-no-listener');s.finish(20)
 if (build/'web-service-host-fault1').exists():
  for fault in [1,2,3,4]:
   s=Service('startup-fault'+str(fault),fault=fault)
   check(s.proc.wait(timeout=5)==20 and b'web_ready=1' not in s.log_path.read_bytes(),'service-entropy-clock-failclosed',fault=fault)
   s.finish(20)
 s=Service('functional');manifest['functional_owner_metadata']=s.metadata()
 raw=socket.create_connection(('127.0.0.1',s.port),timeout=2)
 try:
  raw.sendall(s.wire('GET','/api/status'))
  try:wire=raw.recv(512)
  except ConnectionResetError:wire=b''
  check(not wire.startswith(b'HTTP/') and b'authenticated_https' not in wire,'plaintext-HTTP-no-alternate-service')
 finally:raw.close()
 # Protected paths cannot expose anything through alternative names/protocols.
 for path in ['/api/status','/api/capabilities','/api/media/live','/api/media/snapshot','/snapshot','/media','/ws','/control','/api/settings','/../../etc/passwd']:
  status,_,_,_=s.request('GET',path);check(status==401,'protected-path-no-cookie',path=path)
 status,headers,payload,elapsed,old_cookie,old_csrf=s.login(wrong)
 check(status==401 and 'Set-Cookie' not in headers,'wrong-password-real600k',elapsed_seconds=elapsed)
 time.sleep(1.02)
 status,headers,payload,elapsed,_,_=s.login(password,account='unknown-fixture-user')
 check(status==401 and 'Set-Cookie' not in headers,'unknown-account-real600k-uniform',elapsed_seconds=elapsed)
 time.sleep(1.02)
 status,headers,payload,elapsed,old_cookie,old_csrf=s.login()
 check(status==200 and payload['authenticated'],'login-real600k-HTTPS',elapsed_seconds=elapsed)
 cookie=headers['Set-Cookie'].split(';',1)[0];csrf=payload['csrf'];generation=payload['session_generation']
 check(cookie!=old_cookie and csrf!=old_csrf,'login-token-rotation')
 status,_,_,_=s.request('POST','/api/login',password,old_cookie,old_csrf,'public-fixture-user')
 check(status==403,'login-challenge-replay-denied')
 for path in ['/api/status','/api/capabilities','/api/session']:
  status,_,payload,_=s.request('GET',path,cookie=cookie);check(status==200,'protected-api-success',path=path)
 status,_,payload,_=s.request('GET','/api/media/live',cookie=cookie)
 check(status==501 and payload['error']=='unsupported_unqualified','no-fake-video-after-auth')
 status,_,_,_=s.request('GET','/api/status',cookie=cookie,tls12=True);check(status==200,'TLS12-selected-ChaCha')
 status,_,_,_=s.request('POST','/api/logout',cookie=cookie,csrf='0'*64);check(status==403,'bad-CSRF-no-logout')
 status,_,_,_=s.request('POST','/api/logout',cookie=cookie,csrf=csrf,origin='https://wrong.invalid');check(status==400,'exact-origin-no-logout')
 status,_,_,_=s.request('GET','/api/status',cookie=cookie);check(status==200,'CSRF-origin-denial-preserved-session')
 # Actual malformed framing, bounded body/header and duplicate tests.
 for label,wire in [
  ('duplicate-cookie',s.wire('GET','/api/status',cookie=cookie,extra='Cookie: '+cookie+'\r\n')),
  ('duplicate-length',s.wire('POST','/api/logout',cookie=cookie,csrf=csrf,extra='Content-Length: 0\r\n')),
  ('transfer-encoding',s.wire('GET','/api/status',cookie=cookie,extra='Transfer-Encoding: chunked\r\n')),
  ('oversize-body',s.wire('POST','/api/logout',b'X'*4097,cookie,csrf)),
  ('oversize-header',s.wire('GET','/api/status',cookie=cookie,extra='X-Long: '+'X'*8192+'\r\n')),
  ('obs-fold',s.wire('GET','/api/status',cookie=cookie,extra=' X-Header: no\r\n')),
  ('LF-only',f'GET /api/status HTTP/1.1\nHost: 127.0.0.1:{s.port}\n\n'.encode()),
  ('pipeline',s.wire('GET','/api/status',cookie=cookie)*2),
  ('authorization-bypass',s.wire('GET','/api/status',extra='Authorization: Basic UFVCTElDRFVNTVk=\r\n')),
  ('upgrade-bypass',s.wire('GET','/ws',cookie=cookie,extra='Upgrade: websocket\r\n'))]:
  stream=s.tls()
  try:
   start=time.monotonic();stream.settimeout(7);kind='HTTP400'
   try:
    stream.sendall(wire);reply=http.client.HTTPResponse(stream);reply.begin();payload=reply.read(8193)
    rejected=reply.status==400
   except (http.client.RemoteDisconnected,ConnectionResetError,ssl.SSLError):
    # Oversize input may be rejected before complete HTTP delivery by the actual
    # selected TLS/receive stack. Preserve this distinct observation, never call
    # it HTTP400 or relax to successful admission. Original HTTP400 expectation
    # failure is retained in host07/08. Ordinary framing still requires HTTP400.
    kind='owned_TLS_connection_closed';rejected=label in ['oversize-body','oversize-header']
   check(rejected,'actual-framing-'+label,rejection_kind=kind,elapsed_seconds=time.monotonic()-start)
  finally:stream.close()
 status,_,_,_=s.request('GET','/api/status',cookie=cookie)
 check(status==200,'malformed-oversize-no-effects-session-preserved')
 # Two actually established TLS sockets; third refused, no per-peer producer.
 one=s.tls();two=s.tls()
 try:
  rejected=False
  try:three=s.tls();three.close()
  except (OSError,ssl.SSLError):rejected=True
  check(rejected,'actual-socket-cap-two')
 finally:one.close();two.close()
 time.sleep(0.05)
 stream=s.tls();stream.sendall(b'GET /api/status HTTP/1.1\r\nHost:')
 try:
  start=time.monotonic();stream.settimeout(7)
  try:wire=stream.recv(1024)
  except (OSError,ssl.SSLError):wire=b''
  check(not wire or not wire.startswith(b'HTTP/1.1 200'),'absolute-incomplete-header-deadline',elapsed_seconds=time.monotonic()-start)
 finally:stream.close()
 status,_,_,_=s.request('POST','/api/logout',cookie=cookie,csrf=csrf);check(status==200,'logout-success')
 status,_,_,_=s.request('GET','/api/status',cookie=cookie);check(status==401,'logout-cookie-replay-denied')
 time.sleep(1.02)
 status,headers,payload,_,_,_=s.login();check(status==200 and payload['session_generation']>generation,'session-generation-increases')
 cookie2=headers['Set-Cookie'].split(';',1)[0]
 s.control_command(2,2,material(salt))
 status,_,_,_=s.request('GET','/api/status',cookie=cookie2);check(status==401,'credential-change-old-cookie-revoked')
 time.sleep(1.02)
 status,headers,payload,_,_,_=s.login();check(status==200 and payload['credential_generation']==2,'new-credential-generation-login')
 cookie3=headers['Set-Cookie'].split(';',1)[0]
 s.control_command(1,3)
 status,_,_,_=s.request('GET','/api/status',cookie=cookie3);check(status==401,'account-revocation-old-cookie-denied')
 time.sleep(1.02)
 status,headers,_,_,_,_=s.login();check(status==401 and 'Set-Cookie' not in headers,'revoked-account-real600k-no-grant')
 manifest['functional_final_metadata']=s.metadata()
 manifest['functional_summary']=s.finish()
 s=Service('expiry',absolute=2,idle=1)
 status,headers,payload,_,_,_=s.login();check(status==200,'expiry-campaign-login')
 cookie=headers['Set-Cookie'].split(';',1)[0];time.sleep(1.1)
 status,_,_,_=s.request('GET','/api/status',cookie=cookie);check(status==401,'idle-expiry-denied')
 manifest['expiry_summary']=s.finish()
 # Reproduce OLD FAIL semantics from an explicitly labelled HOST-only canonical
 # source model, not a historical ELF or ignored work source/cache dependency.
 baseline=build/'web-service-host-baseline-semantics'
 s=Service('baseline-exhaustion',binary_override=baseline,abi=2)
 for cycle in range(17):
  status,headers,payload,_,_,_=s.login()
  if cycle==16:
   check(status==401,'old-lifetime16-exhaustion-reproduced-real600k');break
  check(status==200,'old-before-exhaustion-real600k',cycle=cycle)
  c=headers['Set-Cookie'].split(';',1)[0]
  status,_,_,_=s.request('POST','/api/logout',cookie=c,csrf=payload['csrf']);check(status==200,'old-logout')
  time.sleep(0.08)
 manifest['old_exhaustion_summary']=s.finish()
 s=Service('baseline-cancel',binary_override=baseline,abi=2);c,token=s.challenge();stream=s.tls()
 stream.sendall(s.wire('POST','/api/login',password,c,token,'public-fixture-user'));s.wait_job();stream.close()
 check(s.proc.wait(timeout=5)==20,'old-peer-abort-total-fatal-reproduced')
 manifest['old_cancel_summary']=s.finish(20)
 # 36 ACTUAL full600k sequential logins, alternate logout and owned idle expiry.
 # Same service/child, no re-provisioning/reset of policy/generation/throttle.
 s=Service('reuse36',absolute=60,idle=1,runtime=0);last_generation=0
 for cycle in range(36):
  status,headers,payload,elapsed,anon,oldcsrf=s.login()
  check(status==200 and payload['authenticated'] and payload['session_generation']>last_generation,
        'new-reuse-real600k-login',cycle=cycle,elapsed_seconds=elapsed)
  last_generation=payload['session_generation'];c=headers['Set-Cookie'].split(';',1)[0]
  check(s.request('GET','/api/status',cookie=c)[0]==200,'new-reuse-protected-status',cycle=cycle)
  if cycle%2:
   time.sleep(1.06)
  else:
   check(s.request('POST','/api/logout',cookie=c,csrf=payload['csrf'])[0]==200,'new-reuse-exact-logout',cycle=cycle)
  check(s.request('GET','/api/status',cookie=c)[0]==401,'new-reuse-old-cookie-expired-revoked',cycle=cycle)
  check(s.request('POST','/api/login',password,anon,oldcsrf,'public-fixture-user')[0]==403,
        'new-reuse-anonymous-CSRF-cookie-replay-denied',cycle=cycle)
 manifest['reuse36_final_metadata']=s.metadata();manifest['reuse36_summary']=s.finish()
 check('login_success=36' in manifest['reuse36_summary'],'36-grants-no-lifetime-exhaustion')
 s=Service('peer-local');status,headers,payload,_,_,_=s.login();check(status==200,'survivor-session-login')
 survivor=headers['Set-Cookie'].split(';',1)[0];admitted=1;drained=0
 for kind,account in [('disconnect-known','public-fixture-user'),('disconnect-unknown','unknown-fixture-user'),
                      ('extra-TLS-known','public-fixture-user')]:
  time.sleep(0.12);c,token=s.challenge();stream=s.tls()
  stream.sendall(s.wire('POST','/api/login',password,c,token,account));admitted+=1;s.wait_job(admitted)
  time.sleep(0.025)
  if kind.startswith('extra'):stream.sendall(b'EXTRA-UNTRUSTED-BYTES')
  else:stream.close()
  # Admission stays globally serialized while discarded CPU work really runs.
  retry,csrf_retry=s.challenge()
  check(s.request('POST','/api/login',password,retry,csrf_retry,'public-fixture-user')[0]==429,
        'discarded-real-KDF-slot-still-busy',kind=kind)
  check(s.request('GET','/api/status',cookie=survivor)[0]==200,'unrelated-session-survives',kind=kind)
  drained+=1;s.wait_drained(drained);stream.close()
  check(s.request('GET','/api/status',cookie=c)[0]==401,'abandoned-cookie-no-late-MATCH-grant',kind=kind)
  check(s.request('POST','/api/login',password,c,token,account)[0]==403,'abandoned-request-replay-denied',kind=kind)
 check(s.request('POST','/api/logout',cookie=survivor,csrf=payload['csrf'])[0]==200,'survivor-logout')
 manifest['peer_local_summary']=s.finish()
 check('login_success=1' in manifest['peer_local_summary'],'discarded-known-unknown-late-MATCH-count-zero')
 s=Service('response-expiry');c,token=s.challenge();stream=s.tls()
 stream.sendall(s.wire('POST','/api/login',password,c,token,'public-fixture-user',extra='X-Login-Wait-Ms: 25\r\n'));s.wait_job();s.wait_drained()
 check(s.proc.poll() is None,'HTTP-expiry-does-not-retire-child')
 check(s.request('GET','/api/status',cookie=c)[0]==401,'HTTP-expired-cookie-denied');stream.close()
 status,headers,payload,_,_,_=s.login();check(status==200,'HTTP-expiry-next-login-usable-without-restart')
 manifest['response_expiry_summary']=s.finish()
 check('login_success=1' in manifest['response_expiry_summary'],'HTTP-expiry-old-MATCH-never-grants')
 s=Service('revocation-pending');c,token=s.challenge();stream=s.tls()
 stream.sendall(s.wire('POST','/api/login',password,c,token,'public-fixture-user'));s.wait_job();time.sleep(0.025)
 s.control_command(1,2);s.wait_drained();stream.close()
 check(s.proc.poll() is None,'pending-revocation-service-alive')
 check(s.request('GET','/api/status',cookie=c)[0]==401,'revocation-stale-cookie-denied')
 s.control_command(2,3,material(salt));status,headers,payload,_,_,_=s.login()
 check(status==200 and payload['credential_generation']==3,'revoked-then-provisioned-owner-grants-after-ACK-no-restart')
 manifest['pending_revocation_summary']=s.finish()
 check('login_success=1' in manifest['pending_revocation_summary'],'pending-revocation-old-MATCH-count-zero')
 # Credential replacement while real KDF is outstanding also denies old MATCH.
 s=Service('replacement-pending');c,token=s.challenge();stream=s.tls()
 stream.sendall(s.wire('POST','/api/login',password,c,token,'public-fixture-user'));s.wait_job();time.sleep(0.025)
 s.control_command(2,2,material(salt));s.wait_drained();stream.close()
 status,headers,payload,_,_,_=s.login();check(status==200 and payload['credential_generation']==2,
       'replacement-new-generation-usable-same-instance')
 manifest['pending_replacement_summary']=s.finish()
 check('login_success=1' in manifest['pending_replacement_summary'],'pending-replacement-old-MATCH-count-zero')
 manifest['state']='actual-owned-loopback-HTTPS-full600k-strict-child-sockets-passed-NOT-native-or-product'
except BaseException:
 manifest['state']='failed';manifest['failure']=traceback.format_exc();print(manifest['failure'])
finally:
 for s in services:
  if s.proc.poll() is None:
   try:s.proc.terminate();s.proc.wait(timeout=5)
   except BaseException:s.proc.kill();s.proc.wait(timeout=3)
 for p in (R/'logs').iterdir():
  if p.is_file() and (password in p.read_bytes() or wrong in p.read_bytes()):manifest['log_scan_failure']=p.name;manifest['state']='failed'
 manifest['checks']=checks;manifest['ended']=datetime.datetime.now(datetime.timezone.utc).isoformat()
 manifest['python_ssl']=ssl.OPENSSL_VERSION;manifest['host_limits']={'uid':os.getuid(),'gid':os.getgid(),'memlock':resource.getrlimit(resource.RLIMIT_MEMLOCK)}
 manifest['build_inputs']=[{'path':str(p),'sha256':hashlib.sha256(p.read_bytes()).hexdigest()}
  for p in [build/'web-service-host',build/'credential-child-host',A/'host_https_test.py'] if p.is_file()]
 manifest['no_private_fixture_material_hashes']=True
 with (R/'manifest.json').open('x') as f:json.dump(manifest,f,indent=2)
 print(manifest['state'],checks)
if manifest['state']=='failed':raise SystemExit(1)
