#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Owned nonroot PUBLICDUMMY test of launcher/actual reviewed HTTPS process.
NOT root/group-drop/native2MiB proof. No private operator secrets or camera.
"""
from pathlib import Path
import datetime,hashlib,json,os,signal,socket,stat,struct,subprocess,sys,time,traceback
A=Path(__file__).resolve().parent;sys.path.insert(0,str(A.parent))
from build_support import resolve_run,fresh_run,artifacts_from_service
build_root=resolve_run(sys.argv[1],kind='native-launch');build=build_root/'build';name=sys.argv[2]
static_ui=len(sys.argv)==4 and sys.argv[3]=='--static-ui-assets'
if len(sys.argv) not in (3,4) or (len(sys.argv)==4 and not static_ui):raise ValueError('unknown host campaign option')
R=fresh_run(name,'native-launch');(R/'logs').mkdir();events=[];commands=[];children=[]
env={'PATH':'/usr/bin:/bin','LC_ALL':'C','TMPDIR':str(R),'TEMP':str(R),'TMP':str(R)}
deadline=time.monotonic()+40
def check(ok,label,**data):
 events.append({'label':label,'pass':bool(ok),**data})
 if not ok:raise AssertionError(label)
 if time.monotonic()>deadline:raise TimeoutError('finite40s owned host campaign')
def run(args,label,expected=0,timeout=15):
 args=list(map(str,args));record={'argv':args,'log':str(R/'logs'/(label+'.txt')),'timeout_seconds':timeout,'environment':env};commands.append(record)
 with Path(record['log']).open('xb') as f:p=subprocess.run(args,env=env,cwd=R,stdout=f,stderr=subprocess.STDOUT,timeout=timeout)
 record['exit']=p.returncode;check(p.returncode==expected,label);return Path(record['log'])
build_manifest=json.loads((build_root/'manifest.json').read_text())
service_root=Path(build_manifest['service_build']['path']);exact_images=artifacts_from_service(service_root)
if exact_images!=build_manifest['exact_image_inputs']:raise SystemExit('launcher/test exact-image closure mismatch')
original=service_root/'build';service=R/'service-source0500';child=R/'child-source0500'
def provision(label):
 scope=R/label
 s=socket.socket();s.bind(('127.0.0.1',0));port=s.getsockname()[1];s.close()
 run(['python3',A/'provision_pc.py','--public-host-fixture','--material-tool',build/'material-gen-PC-PUBLICDUMMY',
      '--directory',scope,'--address','127.0.0.1','--port',port,'--boot-generation',time.monotonic_ns(),
      '--child-generation',time.monotonic_ns()+1,'--runtime','120'],'provision-'+label,timeout=10)
 return scope
manifest={'started':datetime.datetime.now(datetime.timezone.utc).isoformat(),'scope':'HOST PUBLICDUMMY only; native/root proof absent','events':events,'commands':commands}
try:
 check(os.getuid()==1000 and os.getgid()==1000,'host-fixture-identity-not-native-proof')
 for source,destination in [(original/'web-service-host',service),(original/'credential-child-host',child)]:
  fd=os.open(destination,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o500)
  try:os.write(fd,source.read_bytes())
  finally:os.close(fd)
 check(stat.S_IMODE(service.stat().st_mode)==0o500,'staged0500-fixture')
 scope=provision('fixed2m-failclosed')
 log=run([build/'auth-web-launch-host2m',scope,service,child],'fixed2m-failclosed',expected=20)
 check(b'service_ready=1' not in log.read_bytes(),'host-glibc-over-2MiB-refuses-no-auto-raise')
 scope=provision('bad-config');config=scope/'config.bin'
 value=bytearray(config.read_bytes());struct.pack_into('<I',value,4,1);config.write_bytes(value)
 log=run([build/'auth-web-launch-host4m',scope,service,child],'bad-private-config',expected=20)
 check(b'no_service_exec=1' in log.read_bytes(),'ABI-type-size-policy-fails-before-exec')
 scope=provision('image-pin-denied');altered=R/'wrong-public-image'
 value=bytearray(service.read_bytes());value[-1]^=1;fd=os.open(altered,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o500)
 try:os.write(fd,value)
 finally:os.close(fd)
 log=run([build/'auth-web-launch-host4m',scope,altered,child],'image-pin-denied',expected=20)
 check(b'no_service_exec=1' in log.read_bytes(),'Mbed-SHA256-opened-image-pin-not-forged-readiness')
 scope=provision('actual-https')
 logpath=R/'logs/actual-bootstrap.txt';log=logpath.open('xb')
 owner=subprocess.Popen([str(build/'auth-web-launch-host4m'),str(scope),str(service),str(child)],
  cwd=R,env={},stdin=subprocess.DEVNULL,stdout=log,stderr=subprocess.STDOUT)
 children.append(owner);ownerfd=os.pidfd_open(owner.pid,0)
 commands.append({'argv':[str(build/'auth-web-launch-host4m'),str(scope),str(service),str(child)],'log':str(logpath),'environment':{},'own_pid':owner.pid})
 until=time.monotonic()+6
 while time.monotonic()<until and b'service_ready=1' not in logpath.read_bytes():
  if owner.poll() is not None:break
  time.sleep(0.01)
 check(owner.poll() is None and b'service_ready=1' in logpath.read_bytes(),'actual-reviewed-Mongoose-Mbed-service-strict-child-ready')
 check(stat.S_IMODE(scope.stat().st_mode)==0o700,'private-scope0700-retained')
 for name in ['cert.pem','key.pem','credentials.bin','config.bin','operator.password']:
  check(stat.S_IMODE((scope/name).stat().st_mode)==0o600,'private-file0600-'+name)
 public=Path(str(scope)+'.public-code')
 check(stat.S_IMODE(public.stat().st_mode)==0o555,'separate-public-code-tree0555')
 for name in ['service','child']:check(stat.S_IMODE((public/name).stat().st_mode)==0o555,'public-code0555-'+name)
 check(stat.S_IMODE(service.stat().st_mode)==0o500,'original-staged0500-unchanged')
 client_log=R/'logs/private-client.txt';out=client_log.open('xb')
 client=subprocess.Popen(['python3',str(A/'private_client.py'),'--public-host-fixture','--private-directory',str(scope),'--after-owner-revoke',*(['--static-ui-assets'] if static_ui else [])],
  env=env,cwd=R,stdout=out,stderr=subprocess.STDOUT)
 children.append(client);until=time.monotonic()+15
 while time.monotonic()<until and b'private_campaign_owner_revoke_window=1' not in client_log.read_bytes():
  if client.poll() is not None:break
  time.sleep(0.01)
 check(b'private_campaign_owner_revoke_window=1' in client_log.read_bytes(),'private-client-revoke-window-no-tokens-output')
 signal.pidfd_send_signal(ownerfd,signal.SIGUSR1)
 check(client.wait(timeout=8)==0,'real-HTTPS-login-status-session-logout-Range401-discard-drain-owner-revoke')
 if static_ui:check(b'native_UI_static_assets=PASS count=11' in client_log.read_bytes(),'actual-TLS-static-UI-assets-public-source-byte-pins-CSP')
 out.close()
 check(b'owner_revoke_ack=1' in logpath.read_bytes(),'FD7-root-creator-semantics-owner-command-ACK')
 signal.pidfd_send_signal(ownerfd,signal.SIGTERM)
 check(owner.wait(timeout=6)==0,'normal-owned-pidfd-SIGTERM-service-and-child-retired')
 os.close(ownerfd);log.close()
 check(b'owned_lineage_no_remaining_children=1' in logpath.read_bytes(),'actual-subtree-retirement-not-PDEATHSIG-request-alone')
 check(b'service_wait_exit_observed=1 strict_child_retirement_reported=1 service_status=0' in logpath.read_bytes(),'actual-service-wait-plus-pinned-child-receipt')
 for path in (R/'logs').iterdir():
  text=path.read_bytes()
  check(b'PUBLIC-DUMMY-NOT-A-PRODUCT-PASSWORD' not in text and b'__Host-camera=' not in text and
    b'-----BEGIN' not in text and b'login_success=' not in text,'no-password-cookie-key-match-values-in-log-'+path.name)
 manifest['state']='HOST PUBLICDUMMY launcher actual HTTPS passed; root UID/groups/tmpfs/native2MiB not proved'
except BaseException:
 manifest['state']='failed';manifest['failure']=traceback.format_exc();print(manifest['failure'])
finally:
 for c in children:
  if c.poll() is None:
   try:c.terminate();c.wait(timeout=5)
   except BaseException:c.kill();c.wait(timeout=3)
 manifest['ended']=datetime.datetime.now(datetime.timezone.utc).isoformat()
 manifest['files']=[{'path':str(p.relative_to(R)),'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()}
  for p in sorted((R/'logs').rglob('*')) if p.is_file()]
 manifest['secret_input_files_never_hashed']=True
 with (R/'manifest.json').open('x') as f:json.dump(manifest,f,indent=2)
 print(manifest['state'])
if manifest['state']=='failed':raise SystemExit(1)
