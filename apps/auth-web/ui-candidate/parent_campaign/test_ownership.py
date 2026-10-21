#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Exact-owned PC descriptor tests with logical mocks; no process/IPC/target calls."""
from pathlib import Path
import hashlib,json,signal,sys,unittest
sys.dont_write_bytecode=True
from recipe import *
from owned_client import OwnedClient
from test_recipe import SPEC

ROOT=Path(__file__).resolve().parents[4]
E=ROOT/'evidence/delegated/20261012-native-ui/parent-campaign/inputs'

class Pipe:
 def fileno(self):return 8
 def close(self):pass
class Proc:
 def __init__(self):self.pid=42;self.stdout=Pipe();self.code=None;self.waits=[]
 def poll(self):return self.code
 def wait(self,timeout):self.waits.append(timeout);self.code=0;return 0
class Mocks:
 def __init__(self):self.proc=Proc();self.opened=[];self.signals=[];self.closed=[];self.argv=None;self.bytes=[];self.fail_open=False
 def popen(self,argv,**kwargs):self.argv=argv;self.kwargs=kwargs;return self.proc
 def open(self,pid,flags):
  self.opened.append(pid)
  if self.fail_open:raise OSError('PUBLICDUMMY')
  return 9
 def send(self,fd,sig):self.signals.append((fd,sig))
 def close(self,fd):self.closed.append(fd)
 def read(self,fd,size):
  if not self.bytes:raise BlockingIOError()
  return self.bytes.pop(0)
 def client(self):
  source=E/'canonical-private-client.py'
  # A temporary public fixture filename is not necessary: source hash check is tested
  # independently; actual canonical source path remains read-only and never executed.
  actual=ROOT/'apps/auth-web/native-launch/private_client.py';pin=hashlib.sha256(actual.read_bytes()).hexdigest()
  return OwnedClient(actual,pin,self.popen,self.open,self.send,self.close,self.read,lambda *a:None)

class Ownership(unittest.TestCase):
 def test_native_tested_guard_transform_BYTE_equivalence(self):
  observed=json.loads((E/'native04-observed.json').read_text())['text_public_scalars'].split('\r\n')[0]
  review=ROOT/'evidence/delegated/20261012-native-ui/guard-review'
  pin=json.loads((review/'final-source.json').read_text())['source']['candidate.py']['sha256']
  source=ROOT/'apps/auth-web/ui-candidate/guard-review/candidate.py'
  self.assertEqual(hashlib.sha256(source.read_bytes()).hexdigest(),pin)
  scope={'__name__':'__reviewed_guard_test__'}
  exec(compile(source.read_text(),str(source),'exec'),scope)
  observed=scope['candidate'](observed)
  for old,new in [('/run/auth-launch-signal04.elf',SPEC.launcher),('/run/auth-web-signal04.elf',SPEC.service),
   ('/run/auth-child-signal04.elf',SPEC.child),('/run/auth-web-signal04',SPEC.private_scope),
   ('AUTHSIGNAL_BOOT04_PID','UI_NATIVE02_BOOT_PID'),('AUTHSIGNAL_BOOT04_OWN_EXIT','UI_NATIVE02_BOOT_OWN_EXIT'),
   ('AUTHSIGNAL_PARENT04_PID','UI_NATIVE02_PARENT_PID'),('AUTHSIGNAL04_NONZERO_NO_RETRY_QUALIFIED_RECOVERY','UI_NATIVE02_NONZERO_NO_RETRY_QUALIFIED_RECOVERY')]:observed=observed.replace(old,new)
  self.assertEqual(startup(SPEC),observed)
 def test_no_unavailable_BusyBox_dependencies(self):
  text=startup(SPEC)+signal_command(SPEC,232,'REVOKE')+signal_command(SPEC,232,'STOP')+post_command(SPEC,232,230)
  for command in ['awk ','uname ','rmdir ','stat ','ps ','rm -rf','stty ','wget ','base64 ']:self.assertNotIn(command,text)
  self.assertIn('sleep 4;',text);self.assertIn('then printf V >&3; exec 3>&-',text)
 def test_preflight_explicit_pidfd_only(self):
  m=Mocks();c=m.client();c.preflight();self.assertEqual(m.signals,[]);self.assertEqual(len(m.opened),1)
 def test_exactCLI_no_secret_input_read_by_recipe(self):
  m=Mocks();c=m.client();c.start('/PUBLICDUMMY/never-read')
  self.assertEqual(m.argv[-4:],['--private-directory','/PUBLICDUMMY/never-read','--static-ui-assets','--after-owner-revoke'])
  self.assertNotIn('shell',m.kwargs);self.assertEqual(m.kwargs['env'],{'PATH':'/usr/bin:/bin','LC_ALL':'C'})
 def test_PIDreuse_cancel_uses_original_descriptor(self):
  m=Mocks();c=m.client();c.start('/PUBLICDUMMY/never-read');m.proc.pid=999999;c.cancel()
  self.assertEqual(m.opened,[42]);self.assertEqual(m.signals,[(9,signal.SIGTERM)]);self.assertEqual(m.closed,[9])
 def test_already_dead_pidfd_no_PID_fallback(self):
  m=Mocks();c=m.client();c.start('/PUBLICDUMMY/never-read');m.send=lambda *a:(_ for _ in ()).throw(ProcessLookupError());c.send=m.send;c.cancel()
  self.assertEqual(m.opened,[42])
 def test_missing_pidfd_never_PID_signal(self):
  m=Mocks();c=m.client();m.fail_open=True
  with self.assertRaises(Hold):c.start('/PUBLICDUMMY/never-read')
  with self.assertRaises(Hold):c.cancel()
  self.assertEqual(m.signals,[])
 def test_all_client_line_split_sizes(self):
  data=(CLIENT_ASSETS+'\n'+CLIENT_REVOKE+'\n'+CLIENT_PASS+'\n').encode()
  for size in range(1,len(data)+1):
   m=Mocks();c=m.client();c.start('/PUBLICDUMMY/never-read');m.bytes=[data[i:i+size] for i in range(0,len(data),size)]+[b''];m.proc.code=0
   self.assertEqual(c.lines(),[CLIENT_ASSETS,CLIENT_REVOKE,CLIENT_PASS]);self.assertEqual(c.poll(),0)
 def test_poll_drain_race_retains_late_PASS(self):
  m=Mocks();c=m.client();c.start('/PUBLICDUMMY/never-read');self.assertEqual(c.lines(),[])
  m.proc.code=0;m.bytes=[(CLIENT_PASS+'\n').encode(),b''];self.assertIsNone(c.poll());self.assertEqual(c.lines(),[CLIENT_PASS]);self.assertEqual(c.poll(),0)
 def test_unknown_client_output_never_logged(self):
  for data in [b'PRIVATE COOKIE MUST NOT LOG\n',b'x'*513,b'private_HTTPS_campaign=PASS\n']:
   m=Mocks();c=m.client();c.start('/PUBLICDUMMY/never-read');m.bytes=[data]
   with self.assertRaises(Hold) as caught:c.lines()
   self.assertNotIn('COOKIE',str(caught.exception))
 def test_dispose_exited_close_without_signal(self):
  m=Mocks();c=m.client();c.start('/PUBLICDUMMY/never-read');m.proc.code=0;c.dispose();self.assertEqual(m.signals,[]);self.assertEqual(m.closed,[9])

if __name__=='__main__':unittest.main(verbosity=2)
