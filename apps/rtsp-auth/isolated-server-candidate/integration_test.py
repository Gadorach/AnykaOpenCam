#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Additional actual owned-child/socket PUBLICDUMMY campaigns; scalar only."""
from pathlib import Path
import json,sys,time
import client_test as C
def ipc(s):
 import re
 text=s.log_path.read_text()
 line=next(x for x in text.splitlines() if x.startswith('HOST_ONLY IPC '))
 return {k:int(v) for k,v in re.findall(r'([A-Za-z_]+)=([0-9]+)',line)}
def main():
 binary,delay,strict,C.GENERATOR,C.CHILD,delay_child,directory=map(lambda s:Path(s).resolve(),sys.argv[1:])
 directory.mkdir();rows=[]
 def finish(s,label):
  s.finish();stats=ipc(s) if not s.refused else {}
  if stats:C.check(stats['retired_results']==stats['submitted'])
  rows.append({'case':label,'source':s.source,'verifier':s.verifier,'IPC':stats,'native':False})
 #Native source class cannot borrow HOST image/UID/READY; pre-input refusal.
 s=C.Server(strict,directory,'strict-native-nonroot',refused=True)
 s.finish();C.check(s.verifier['owned_exits']==0)
 material=next(x for x in s.lines if x.startswith('HOST_ONLY MATERIAL '))
 C.check('parent_read_calls=0' in material and 'parent_max_read_bytes=0' in material)
 rows.append({'case':'native-before-material-refusal','source':s.source,'verifier':s.verifier,'native':False})
 for alg in ('SHA-256','MD5'):
  s=C.Server(binary,directory,alg.lower()+'-lineage-negatives',alg)
  try:
   c=s.client(alg);c.send('DESCRIBE',authenticated=False);c.reply(401)
   wire=c.wire('DESCRIBE',target=c.track)
   first,rest=wire.split(b'\r\n',1)
   c.socket.sendall(b'DESCRIBE '+c.live.encode()+b' RTSP/1.0\r\n'+rest);c.reply(401)
   wire=c.wire('DESCRIBE').replace(b', qop=auth',b'')
   c.socket.sendall(wire);c.reply(401)
   original=c.alg;c.alg='MD5' if original=='SHA-256' else 'SHA-256'
   wire=c.wire('DESCRIBE');c.alg=original;c.socket.sendall(wire);c.reply(401)
   c.send('DESCRIBE');c.reply(200);c.close()
  finally:finish(s,alg+'-URI-qop-explicit-algorithm')
  C.check(s.source['next']==0 and s.summary['sdp']==1)
 #Child death: no fake BYE/wait0; provider backing stays alive until OWN wait.
 s=C.Server(binary,directory,'owned-child-hardfault',hardfault=True)
 try:
  c=s.client();c.login();c.play();c.frame();s.command('K');c.closed()
 finally:finish(s,'child-hardfault-owned-signaled-retirement')
 C.check(s.source['starts']==s.source['idles'] and s.verifier['signaled']==1)
 #Owner revocation while a real TCP packet is partially written.
 s=C.Server(binary,directory,'partial-RTP-revoke',chunk=17)
 try:
  c=s.client();c.login();s.command('V');c.play();c.closed()
 finally:finish(s,'queued-partial-RTP-revoke')
 C.check(s.summary['partial_discard']==s.summary['lease_releases']==1)
 C.check(s.source['next']==s.source['releases']==s.source['release_wipes']==1)
 #Actual provider retire failure retains exact demand/backing: no STOP retry.
 s=C.Server(binary,directory,'source-retire-failure',release_mode=6)
 try:
  c=s.client();c.login();c.play();c.frame();c.closed()
 finally:finish(s,'source-retire-uncertainty-retains-backing')
 C.check(s.source['retained']==s.source['demands']==1 and s.source['idles']==s.source['detaches']==0)
 #Actual-clock namespace regression observation, no kernel clock/time change.
 s=C.Server(binary,directory,'clock-regression',clockfault=True)
 try:
  c=s.client();c.login();s.command('T');c.closed()
 finally:finish(s,'clock-regression-fences-admission')
 C.check(s.source['next']==0)
 original_child=C.CHILD;C.CHILD=delay_child
 for label,command,close in [('discard-busy','JD',False),('drop-verification','',True),('revoke-pending','R',False)]:
  s=C.Server(delay,directory,label)
  try:
   c=s.client();c.login()
   if command=='R':
    c.send('DESCRIBE');time.sleep(0.025);s.command('R');c.closed()
   else:
    if command:s.command(command)
    c.send('DESCRIBE')
    if close:c.close();time.sleep(0.3)
    else:c.closed()
  finally:finish(s,label+'-actual-result-RETIRE-ACK')
  stats=rows[-1]['IPC'];C.check(stats['discarded']==1 and stats['retired_results']==stats['submitted'])
  if command=='JD':C.check(stats['busy']==1)
  C.check(s.source['next']==0)
 C.CHILD=original_child
 #Four actual child-owned peers; fifth cannot allocate/reset a crypto owner.
 s=C.Server(binary,directory,'child-four-peer-bound')
 try:
  peers=[s.client() for _ in range(4)]
  for c in peers:c.login()
  fifth=s.client();fifth.closed()
  for c in peers:c.close()
 finally:finish(s,'four-peer-bound')
 C.check(s.summary['opened']==4)
 with (directory/'CONTROL-TABLE.json').open('x') as f:json.dump({'evidence_state':'HOST PUBLICDUMMY ONLY','checks':C.CHECKS,'rows':rows,'native':False},f,indent=2);f.write('\n')
 print('HOST_ONLY PASS integration_checks='+str(C.CHECKS)+' cases='+str(len(rows))+' native=0 private=0')
if __name__=='__main__':
 try:main()
 except BaseException as e:print('HOST_ONLY FAIL integration_checks='+str(C.CHECKS)+' error_type='+type(e).__name__);sys.exit(1)
