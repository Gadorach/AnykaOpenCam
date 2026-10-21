#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Deterministic PUBLICDUMMY logical UART/client tests; never device/IPC/TLS/private input."""
from dataclasses import replace
from pathlib import Path
import itertools,sys,unittest
sys.dont_write_bytecode=True
from recipe import *

PINS=('a'*64,'c9cfc8b954bd5c2370c955f0ce6d886224b27d3899d1972d80baf285fa5f0e2e','8bb3d596065efb35338e2f28bedd60718eac8254d4f569e4412e77567f9c119a')
SPEC=Spec('UI_NATIVE02','/run/launch-ui-native02.elf','/run/auth-ui-native02','/run/service-ui-native02.elf','/run/child-ui-native02.elf',PINS,'/PUBLICDUMMY/never-read')
IDENTITY=('PUBLICDUMMY-stream','PUBLICDUMMY-epoch',501,100,501,1000,'PUBLICDUMMY-adapter',7,8)
POST={'watchdog':'inactive','uart_rx_error_events':0,'uart_mem_busy_ticks':0,'isp_readable':0,'guard_errors':0,'irq':0,'irq_frames':0,'queue_started':0,'queue_held':0,'wb_owner_active':0,'remaining_owned_lineage':0,'listener_18443':0}
READY=['UI_NATIVE02_PARENT_PID=230','# UI_NATIVE02_BOOT_PID=232','bootstrap_class=ROOT_RAM_QUALIFICATION nonroot_UID=1000 nonroot_GID=1000',
 'bootstrap_launched=1 memlock_service_bytes=2097152 memlock_child_bytes=2097152 fd_contract=3_7 readonly_files=1',
 'service_ready=1 source=pinned_service strict_child_ready_reported=1']

class Clock:
 def __init__(self):self.now=100.
 def __call__(self):return self.now
class Client:
 def __init__(self):self.starts=0;self.events=[];self.code=None;self.cancelled=0;self.bad_preflight=False
 def preflight(self):
  if self.bad_preflight:raise Hold('PC_pidfd_capability_missing')
 def start(self,path):assert path==SPEC.pc_private_directory;self.starts+=1
 def lines(self):out=self.events;self.events=[];return out
 def poll(self):return self.code
 def cancel(self):self.cancelled+=1
class Backend:
 def __init__(self,clock):
  self.clock=clock;self.identity=IDENTITY;self.seq=0;self.base=1000;self.data=bytearray();self.sent=[];self.alive=True;self.ticks=77;self.auto=True;self.override={};self.post=POST.copy();self.chunk_size=4096
 def probe(self):
  a=Admission(self.identity,'PUBLICDUMMY-boot','b'*40,'c'*64,'PUBLICDUMMY-arm',400.,self.clock(),self.base+len(self.data),self.seq,1,0,'actual75_linux',PINS,(0o700,(0o600,)*4,(0o500,)*3),(0,(0,)*4,(0,)*3),(2097152,2097152),(2,768,180),0,'# ')
  return replace(a,**self.override)
 def add(self,lines):self.data.extend(('\r\n'.join(lines)+'\r\n').encode())
 def checked_command(self,command,a):
  assert a.tx_sequence==self.seq;start=self.base+len(self.data);self.seq+=1;self.sent.append(command);self.add([command])
  if self.auto:
   if 'for n in 1 2 3' in command:self.add(READY)
   elif 'kill -USR1' in command:self.add(['UI_NATIVE02_OWN_REVOKE_SIGNAL:0','owner_revoke_sent=1','owner_revoke_ack=1'])
   elif 'kill -TERM' in command:
    self.add(['UI_NATIVE02_OWN_STOP_SIGNAL:0','owned_lineage_no_remaining_children=1 adopted_exit_receipts=0',
      'service_wait_exit_observed=1 strict_child_retirement_reported=1 service_status=0 rss_peak_kb=568 bootstrap_result=0','UI_NATIVE02_BOOT_OWN_EXIT:0',
      'GUARD_FIRST_EXEC_MATCH:1','GUARD_PING:1','GUARD_WAIT_OBSERVED:0']);self.alive=False
   elif command.startswith('cat /sys/class/watchdog'):self.add(['UI_NATIVE02_LINEAGE_EXITED','UI_NATIVE02_LISTENER_ABSENT'])
  return TxReceipt(self.identity,start,self.seq)
 def uart_capture(self,start):
  body=bytes(self.data[max(0,start-self.base):]);return [Chunk(self.identity,start+i,body[i:i+self.chunk_size]) for i in range(0,len(body),self.chunk_size)]
 def prove_bootstrap(self,pid,parent,exe,checked_inspection):
  if not self.alive:return None
  return BootstrapProof(pid,parent,self.ticks,exe,'PUBLICDUMMY-boot',self.identity,self.clock())
 def poststate(self,offset):assert offset==self.base+len(self.data);return self.post

def make():
 clock=Clock();b=Backend(clock);c=Client();a=b.probe();expected=Expected(a.identity,a.boot_id,a.kernel_build_id,a.kernel_config_sha256,a.arm_id,a.arm_end)
 return Campaign(SPEC,expected,b,c,clock),b,c,clock
def client_stage():
 j,b,c,t=make();j.start();j.tick();assert j.stage=='CLIENT';return j,b,c,t
def pass_job(j,b,c):
 c.events=[CLIENT_ASSETS,CLIENT_REVOKE];j.tick();j.tick();c.events=[CLIENT_PASS];c.code=0;j.tick();j.tick();j.tick()

class ParserTests(unittest.TestCase):
 def test_guard_faults_refuse_and_periodic_progress_is_not_duplicate_first_ping(self):
  for line in ['GUARD_SOURCE_REFUSED:1','GUARD_EXE_REFUSED:2','GUARD_WAIT_OBSERVED:20']:
   with self.assertRaises(Hold):Scalars(SPEC,IDENTITY,0).feed(Chunk(IDENTITY,0,(line+'\n').encode()))
  p=Scalars(SPEC,IDENTITY,0)
  p.feed(Chunk(IDENTITY,0,b'GUARD_FIRST_EXEC_MATCH:1\nGUARD_PING:1\nGUARD_PING:2\nGUARD_PING:3\nGUARD_WAIT_OBSERVED:0\n'))
  self.assertEqual(p.values,{'guard_exec':True,'guard_ping':True,'guard_wait':True})
 def test_every_two_chunk_split_prompt_PID(self):
  data=b'# UI_NATIVE02_BOOT_PID=232\r\n'
  for n in range(len(data)+1):
   p=Scalars(SPEC,IDENTITY,100);p.feed(Chunk(IDENTITY,100,data[:n]));p.feed(Chunk(IDENTITY,100+n,data[n:]));self.assertEqual(p.values,{'boot':232})
 def test_every_chunk_size_combined_READY_PID(self):
  data=('\r\n'.join(READY)+'\r\n').encode()
  for size in range(1,len(data)+1):
   p=Scalars(SPEC,IDENTITY,100)
   for n in range(0,len(data),size):p.feed(Chunk(IDENTITY,100+n,data[n:n+size]))
   self.assertEqual(p.values['boot'],232);self.assertTrue(p.values['ready'])
 def test_echo_comments_scripts_old_aliases_not_success(self):
  p=Scalars(SPEC,IDENTITY,100)
  command=startup(SPEC);p.echo(command);p.echo('# UI_NATIVE02_BOOT_PID=666')
  data=(command+'\r\n# UI_NATIVE02_BOOT_PID=666\r\necho UI_NATIVE02_BOOT_OWN_EXIT:0\r\n# echo service_ready=1 source=pinned_service strict_child_ready_reported=1\r\nUI_NATIVE01_BOOT_PID=8\r\n').encode()
  p.feed(Chunk(IDENTITY,100,data));self.assertEqual(p.values,{})
 def test_prefix_wrapped_async_prompt(self):
  p=Scalars(SPEC,IDENTITY,0);p.feed(Chunk(IDENTITY,0,b'# \r\n# UI_NATIVE02_BOOT_PID=232\r\n# \r\n'));self.assertEqual(p.values['boot'],232)
 def test_stale_before_startoffset(self):
  data=b'UI_NATIVE02_BOOT_PID=7\r\n';p=Scalars(SPEC,IDENTITY,len(data));p.feed(Chunk(IDENTITY,0,data));self.assertEqual(p.values,{})
 def test_conflicts_duplicates_READY_retirement(self):
  for line in ['UI_NATIVE02_BOOT_PID=232','service_ready=1 source=pinned_service strict_child_ready_reported=1','owned_lineage_no_remaining_children=1 adopted_exit_receipts=0']:
   p=Scalars(SPEC,IDENTITY,0);data=(line+'\n'+line+'\n').encode()
   with self.assertRaises(Hold):p.feed(Chunk(IDENTITY,0,data))
 def test_conflictingPID(self):
  p=Scalars(SPEC,IDENTITY,0)
  with self.assertRaises(Hold):p.feed(Chunk(IDENTITY,0,b'UI_NATIVE02_BOOT_PID=232\nUI_NATIVE02_BOOT_PID=233\n'))
 def test_RXgap_and_epoch(self):
  for chunk in [Chunk(IDENTITY,1,b'x'),Chunk(('wrong',),0,b'x')]:
   with self.assertRaises(Hold):Scalars(SPEC,IDENTITY,0).feed(chunk)
 def test_line_bound(self):
  with self.assertRaises(Hold):Scalars(SPEC,IDENTITY,0).feed(Chunk(IDENTITY,0,b'x'*2049))

class EngineTests(unittest.TestCase):
 def test_retirement_requires_all_actual_guard_receipts(self):
  markers=['GUARD_FIRST_EXEC_MATCH:1','GUARD_PING:1','GUARD_WAIT_OBSERVED:0']
  for missing in markers:
   j,b,c,t=client_stage();c.events=[CLIENT_ASSETS,CLIENT_REVOKE];j.tick();j.tick()
   b.auto=False;c.events=[CLIENT_PASS];c.code=0;j.tick()
   b.add(['UI_NATIVE02_OWN_STOP_SIGNAL:0','owned_lineage_no_remaining_children=1 adopted_exit_receipts=0',
     'service_wait_exit_observed=1 strict_child_retirement_reported=1 service_status=0 rss_peak_kb=568 bootstrap_result=0',
     'UI_NATIVE02_BOOT_OWN_EXIT:0']+[x for x in markers if x!=missing])
   j.tick();self.assertEqual(j.stage,'RETIRE')
   t.now+=8;j.tick();self.assertEqual(j.stage,'HOLD')
 def test_complete_one_job_only(self):
  j,b,c,t=client_stage();pass_job(j,b,c);self.assertEqual(j.stage,'PASS');self.assertEqual(c.starts,1)
  self.assertEqual(sum('for n in 1 2 3' in x for x in b.sent),1)
  with self.assertRaises(Hold):j.start()
 def test_stale_READY_PID_ignored_afterfreshTX(self):
  j,b,c,t=make();b.auto=False;b.add(READY);j.start();j.tick();self.assertEqual(c.starts,0);self.assertEqual(j.stage,'WAIT_READY')
 def test_combined_ready_pid_rootwait_neverstartsclient(self):
  j,b,c,t=make();b.auto=False;j.start();b.add(READY+['UI_NATIVE02_BOOT_OWN_EXIT:0']);j.tick();self.assertEqual(j.stage,'HOLD');self.assertEqual(c.starts,0)
 def test_exactREADY_missing_any_field(self):
  for missing in range(5):
   j,b,c,t=make();b.auto=False;j.start();b.add([x for n,x in enumerate(READY) if n!=missing]);j.tick();self.assertEqual(c.starts,0)
 def test_ready_on_lastbyte_immediate(self):
  j,b,c,t=make();b.chunk_size=1;j.start();j.tick();self.assertEqual(c.starts,1);self.assertEqual(t(),100.)
 def test_no_manual_revoke_or_STOP_flag_shortcut(self):
  j,b,c,t=client_stage();n=len(b.sent)
  for kind in ['REVOKE','STOP']:
   with self.assertRaises(Hold):j.native_signal(kind)
  self.assertEqual(len(b.sent),n)
 def test_inadequate_runtime_neverclient(self):
  j,b,c,t=make();j.start();t.now=130.;j.tick();self.assertEqual(c.starts,0);self.assertEqual(j.stage,'HOLD')
 def test_lateREADY_before_runtime_exhaustion_is_still_timeout(self):
  j,b,c,t=make();j.start();t.now=111.;j.tick();self.assertEqual(c.starts,0);self.assertEqual(j.stage,'HOLD')
 def test_slow_ownership_proof_cannotstartclient_orlate_revoke(self):
  j,b,c,t=make();j.start();old=b.prove_bootstrap
  def slow(*args):t.now+=11;return old(*args)
  b.prove_bootstrap=slow;j.tick();self.assertEqual(c.starts,0);self.assertEqual(j.stage,'HOLD')
  j,b,c,t=client_stage();old=b.prove_bootstrap
  def slower(*args):t.now+=141;return old(*args)
  b.prove_bootstrap=slower;c.events=[CLIENT_REVOKE];j.tick();self.assertEqual(j.stage,'HOLD');self.assertFalse(any('kill -USR1' in x for x in b.sent))
 def test_parent_inspection_TX_is_scoped_and_echo_filtered(self):
  j,b,c,t=make();old=b.prove_bootstrap
  def inspected(pid,parent,exe,send):send(inspection_command(pid));return old(pid,parent,exe,send)
  b.prove_bootstrap=inspected;j.start();j.tick();self.assertEqual(j.stage,'CLIENT');pass_job(j,b,c);self.assertEqual(j.stage,'PASS')
 def test_all_admission_changes_failclosed(self):
  changes=[{'identity':('new',)+IDENTITY[1:]},{'boot_id':'new'},{'kernel_build_id':'d'*40},{'kernel_config_sha256':'d'*64},
   {'arm_id':'new'},{'arm_end':401.},{'sole_receivers':2},{'transfers':1},{'runtime':'old_linux'}, {'observed':0},
   {'private_modes':(0o755,(0o600,)*4,(0o500,)*3)},{'private_owners':(1000,(0,)*4,(0,)*3)},{'cfg_abi_runtime':(2,768,0)},{'prompt':'U-Boot>'},{'arm_end':float('nan')}]
  for change in changes:
   j,b,c,t=client_stage();n=len(b.sent);b.override=change;j.tick();self.assertEqual(j.stage,'HOLD');self.assertEqual(c.cancelled,1);self.assertEqual(len(b.sent),n)
 def test_unregistered_TX_denied(self):
  j,b,c,t=client_stage();b.seq+=1;j.tick();self.assertEqual(j.stage,'HOLD');self.assertEqual(c.cancelled,1)
 def test_boolean_admission_not_authority(self):
  j,b,c,t=make();b.probe=lambda:True;j.start();self.assertEqual(j.stage,'HOLD');self.assertEqual(b.sent,[])
 def test_oldarm_initial_idle_and_pidfd_preflight(self):
  for change in [{'arm_end':110.},{'idle_services':1}]:
   j,b,c,t=make();b.override=change;j.start();self.assertEqual(b.sent,[]);self.assertEqual(j.stage,'HOLD')
  j,b,c,t=make();c.bad_preflight=True;j.start();self.assertEqual(b.sent,[])
 def test_wrongexe_dead_reused_PID(self):
  for kind in ['dead','reused','wrongexe']:
   j,b,c,t=client_stage()
   if kind=='dead':b.alive=False
   if kind=='reused':b.ticks+=1
   if kind=='wrongexe':b.prove_bootstrap=lambda *a:BootstrapProof(232,230,77,'/run/wrong','PUBLICDUMMY-boot',IDENTITY,t())
   count=len(b.sent);c.events=[CLIENT_ASSETS,CLIENT_REVOKE];j.tick();self.assertEqual(j.stage,'HOLD');self.assertEqual(len(b.sent),count)
 def test_ACK_missing_timeout_no_secondlaunch(self):
  j,b,c,t=client_stage();b.auto=False;c.events=[CLIENT_ASSETS,CLIENT_REVOKE];j.tick();t.now+=2.;j.tick();self.assertEqual(j.stage,'HOLD');self.assertEqual(c.cancelled,1)
 def test_early_late_ACK_and_grant(self):
  j,b,c,t=client_stage();b.add(['owner_revoke_ack=1']);j.tick();self.assertEqual(j.stage,'HOLD')
  j,b,c,t=client_stage();b.auto=False;c.events=[CLIENT_REVOKE];j.tick();t.now+=3;j.tick();b.add(['owner_revoke_ack=1']);c.events=[CLIENT_PASS];j.tick();self.assertEqual(j.stage,'HOLD')
 def test_clienterror_exit_and_fakepass_noexit(self):
  for code,lines in [(1,[]),(0,[]),(0,['private_HTTPS_campaign=PASS']),(0,[CLIENT_ASSETS,CLIENT_PASS])]:
   j,b,c,t=client_stage();c.code=code;c.events=lines;j.tick();self.assertNotEqual(j.stage,'PASS')
  j,b,c,t=client_stage();c.events=[CLIENT_PASS];j.tick();t.now+=140;j.tick();self.assertEqual(j.stage,'HOLD')
 def test_retirement_not_killrequested(self):
  j,b,c,t=client_stage();c.events=[CLIENT_ASSETS,CLIENT_REVOKE];j.tick();j.tick();b.auto=False;c.events=[CLIENT_PASS];c.code=0;j.tick();self.assertEqual(j.stage,'RETIRE');t.now+=8;j.tick();self.assertEqual(j.stage,'HOLD')
 def test_rootwaitnonzero_and_poststate_bad(self):
  j,b,c,t=client_stage();c.events=[CLIENT_ASSETS,CLIENT_REVOKE];j.tick();j.tick();b.auto=False;c.events=[CLIENT_PASS];c.code=0;j.tick();b.add(['UI_NATIVE02_BOOT_OWN_EXIT:20']);j.tick();self.assertEqual(j.stage,'HOLD')
  j,b,c,t=client_stage();b.post['guard_errors']=1;pass_job(j,b,c);self.assertEqual(j.stage,'HOLD')
 def test_postmarkers_cannotprecede_postTX(self):
  j,b,c,t=client_stage();b.add(['UI_NATIVE02_LINEAGE_EXITED','UI_NATIVE02_LISTENER_ABSENT']);j.tick();self.assertEqual(j.stage,'HOLD')
 def test_disposal_cancels_only_ownedjob(self):
  j,b,c,t=client_stage();j.abort('disposed');self.assertEqual(j.stage,'HOLD');self.assertEqual(c.cancelled,1);self.assertTrue(any('kill -TERM 232' in x for x in b.sent));self.assertFalse(any('kill -TERM 230' in x for x in b.sent))
 def test_callback_error_isredacted(self):
  j,b,c,t=client_stage();b.probe=lambda:(_ for _ in ()).throw(RuntimeError('PUBLICDUMMY sensitive-value-must-not-log'));j.tick();self.assertEqual(j.stage,'HOLD');self.assertNotIn('sensitive',str(j.history))
  j,b,c,t=client_stage();b.probe=lambda:(_ for _ in ()).throw(Hold('SECRETALPHANUMERIC123'));j.tick();self.assertNotIn('SECRETALPHANUMERIC123',str(j.history))
 def test_fresh_paths_only(self):
  self.assertEqual(replace(SPEC,launcher='/tmp/launch-native02.elf').launcher,'/tmp/launch-native02.elf')
  for path in ['/run/native01.scope','/run/x;kill1','/run/subdir/x']:
   with self.assertRaises(Hold):replace(SPEC,private_scope=path)
  with self.assertRaises(Hold):replace(SPEC,pc_private_directory='/PUBLICDUMMY/native01-private')

if __name__=='__main__':unittest.main(verbosity=2)
