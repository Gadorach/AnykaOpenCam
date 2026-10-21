#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Injected publicdummy IO/IPC only. HostIO is NEVER invoked in these tests."""
from pathlib import Path
import copy,datetime,hashlib,json,struct,sys,unittest
from unittest.mock import patch
sys.dont_write_bytecode=True
import native_entry as n
from recipe import *
from test_recipe import SPEC,IDENTITY,READY,Clock,Client,POST

PUB=Path(__file__).resolve().parents[4]/'evidence/delegated/20261012-native-ui/parent-campaign/native-entry-v3/public-tool-sources'
HEALTH=b'rx_error_events=0 mem_busy_ticks=0 last_status=40080005 rxpos=77; cached diagnostic, lost bytes cannot be reconstructed\r\n'

class FakeIO:
    def __init__(self,t):
        self.t=t;self.wallbase=1700000000.;self.rx=bytearray(b'PRIVATE-HISTORY-NEVER-READ\n');self.tx=0;self.requests=[];self.reads=[];self.peer=(501,1000,1000);self.parts='h-dummy';self.adapter=IDENTITY[6];self.armed=True;self.inspecting=False;self.ticks=100;self.sockino=8;self.rawino=9;self.delay=0.;self.packet=bytearray();self.effect=None
    def source(self,path):return (PUB/Path(path).name).read_bytes()
    def meta(self,*args):return n.Meta(7,self.sockino,self.ticks,7,self.rawino,len(self.rx))
    def status(self):return {'utc':datetime.datetime.fromtimestamp(self.wallbase+self.t(),datetime.timezone.utc).isoformat(),
      'rx_bytes':len(self.rx),'tx_bytes':self.tx,'recent_rx':'# ','part':self.parts,'adapter':self.adapter,'abort_armed':self.armed,'read_only_inspection_active':self.inspecting}
    def rpc(self,path,request,peer,timeout):
        self.requests.append((copy.deepcopy(request),timeout));self.t.now+=self.delay
        if request['action']=='status':result=self.status()
        elif request['action']=='send':
            data=bytes.fromhex(request['hex']);self.tx+=len(data)
            if data==b'\x15':self.packet.clear();return self.peer,{'transmitted_bytes':1,'utc':self.status()['utc']}
            self.packet.extend(data)
            if data.endswith(b'\r'):
                cmd=bytes(self.packet[:-1]).decode();self.packet.clear();self.rx.extend((cmd+'\r\n').encode())
                if 'for n in 1 2 3' in cmd:self.rx.extend(('\r\n'.join(READY)+'\r\n').encode())
                if cmd.startswith('cat /proc/232/stat'):
                    fields=['S','230']+['0']*17+['77']+['0']*8
                    self.rx.extend(('232 (auth (launch) worker) '+' '.join(fields)+'\r\n'+
                      'lrwxrwxrwx 1 root root 0 Oct 12 00:00 /proc/232/exe -> '+SPEC.launcher+'\r\n').encode())
                if cmd.endswith('cat /sys/class/tty/ttyAK0/rx_state'):self.rx.extend(HEALTH)
                if self.effect:self.effect(cmd)
            result={'transmitted_bytes':len(data),'utc':self.status()['utc']}
        else:raise AssertionError('no observe/arm/private/stop/new protocol')
        return self.peer,result
    def raw(self,path,identity,offset,count):
        self.reads.append((offset,count));return bytes(self.rx[offset:offset+count])
    def sleep(self,seconds):self.t.now+=seconds

def bundle():
    t=Clock();io=FakeIO(t);ident=list(IDENTITY);ident[0]='PUBLICDUMMY-stream';ident[1]='PUBLICDUMMY-instance';ident=tuple(ident)
    # Original logical source owner ticks100/socketdev7/ino8; raw inode9 is separate.
    e=Expected(ident,'PUBLICDUMMY-boot','b'*40,'c'*64,'PUBLICDUMMY-arm',400.)
    common={'source_id':'PUBLICDUMMY-measured-receipt','observed':t(),'instance':'PUBLICDUMMY-instance','identity':list(ident),'part':'h-dummy'}
    receiver={**common,'adapter':io.adapter,'socket_path':'/PUBLICDUMMY/control/uart.sock','raw_root':'/PUBLICDUMMY/raw',
      'raw_path':'/PUBLICDUMMY/raw/uart-rx-h-dummy.bin','raw_dev':7,'raw_ino':9,'rx_bytes':len(io.rx),'tx_bytes':0}
    recovery={**common,'action':'arm_abort','armed':True,'arm_id':e.arm_id,'ends_by':datetime.datetime.fromtimestamp(io.wallbase+400,datetime.timezone.utc).isoformat()}
    boot={**common,'runtime':'actual75_linux','prompt':'# ','rx_end':len(io.rx),'idle_services':0,'boot_id':e.boot_id,
      'kernel_build_id':e.kernel_build_id,'kernel_config_sha256':e.kernel_config_sha256,
      'health':{'uart_rx_error_events':0,'uart_mem_busy_ticks':0,'guard_errors':0,'irq':0,'queue_started':0,'wb_owner_active':0}}
    staged={**common,'paths':[SPEC.launcher,SPEC.private_scope,SPEC.service,SPEC.child],'pins':list(SPEC.pins),
      'cfg':[2,768,180],'memlock':[2097152]*2,'modes':[448,[384]*4,[320]*3],'owners':[0,[0]*4,[0]*3],
      'file_ids':[[7,i,1] for i in range(20,28)]}
    boot['line_cancel']={'source_id':'PUBLICDUMMY-audited-Linux-stty-receipt','boot_id':e.boot_id,'icanon':True,'kill_hex':'15'}
    proofs={'receiver':receiver,'recovery':recovery,'boot':boot,'staging':staged}
    lifetime={'source_id':'PUBLICDUMMY-last-actual-combined-readonly-validation','instance':receiver['instance'],'last_actual_observed':t(),
      'boot_source_id':boot['source_id'],'staging_source_id':staged['source_id'],'boot_id':e.boot_id,'kernel_build_id':e.kernel_build_id,'kernel_config_sha256':e.kernel_config_sha256,
      'paths':staged['paths'],'pins':staged['pins'],'file_ids':staged['file_ids'],'cfg':staged['cfg'],'modes':staged['modes'],'owners':staged['owners']}
    def owner(i,part,adapter,deadline):return {'source_id':'PUBLICDUMMY-fresh-owner-proof','identity':i,'part':part,'adapter':adapter,'observed':t(),'receive_pids':[501],'transfers':[],'fact_lifetime':copy.deepcopy(lifetime)}
    callbacks={'ownership':owner,'deadline':lambda:280.}
    return t,io,e,proofs,callbacks
def entry():
    t,io,e,p,c=bundle();return n.NativeEntry(SPEC,e,p,c,io,t,lambda:io.wallbase+t()),t,io,e,p,c
def launch(x):
    x.prepare_startup(lambda cmd:x.checked_command(cmd,x.probe()))
    a=x.probe();r=x.checked_command(startup(SPEC),a);return r

class EntryTests(unittest.TestCase):
    def test_import_unconfigured_no_host_actions(self):
        with patch.object(n.HostIO,'rpc',side_effect=AssertionError('host forbidden')):
            with self.assertRaises(Hold):n.probe()
    def test_missing_unsealed_proofs_or_callbacks_before_IO(self):
        for field in ['receiver','recovery','boot','staging']:
            t,io,e,p,c=bundle();p.pop(field)
            with self.assertRaises(Hold):n.NativeEntry(SPEC,e,p,c,io,t,lambda:io.wallbase+t())
            self.assertEqual(io.requests,[]);self.assertEqual(io.reads,[])
        t,io,e,p,c=bundle()
        with self.assertRaises(Hold):n.NativeEntry(SPEC,e,p,{},io,t,lambda:io.wallbase+t())
        self.assertEqual(io.requests,[])
    def test_public_configure_missing_field_HOLD_no_IO(self):
        t,io,e,p,c=bundle();p['boot'].pop('kernel_build_id')
        with self.assertRaises(Hold):n.configure(SPEC,e,p,c,io,t,lambda:io.wallbase+t())
        self.assertEqual(io.requests,[]);self.assertIsNone(n._entry)
    def test_full_recipe_with_observed_fake_receiver_callbacks(self):
        x,t,io,e,p,c=entry();client=Client();job=Campaign(SPEC,e,x,client,t)
        def effect(cmd):
            if 'kill -USR1' in cmd:io.rx.extend(b'UI_NATIVE02_OWN_REVOKE_SIGNAL:0\r\nowner_revoke_sent=1\r\nowner_revoke_ack=1\r\n')
            elif 'kill -TERM' in cmd:io.rx.extend(b'UI_NATIVE02_OWN_STOP_SIGNAL:0\r\nowned_lineage_no_remaining_children=1 adopted_exit_receipts=0\r\nservice_wait_exit_observed=1 strict_child_retirement_reported=1 service_status=0 rss_peak_kb=568 bootstrap_result=0\r\nUI_NATIVE02_BOOT_OWN_EXIT:0\r\nGUARD_FIRST_EXEC_MATCH:1\r\nGUARD_PING:1\r\nGUARD_WAIT_OBSERVED:0\r\n')
            elif cmd.startswith('cat /sys/class/watchdog'):io.rx.extend(b'inactive\r\nrx_error_events=0 mem_busy_ticks=0; cached diagnostic\r\nreadable=0 guard_errors=0\r\nirq=0 irq_frames=0\r\nqueue_started=0 queue_held=0\r\nwb_owner_active=0\r\nUI_NATIVE02_LINEAGE_EXITED\r\nUI_NATIVE02_LISTENER_ABSENT\r\n')
        io.effect=effect;job.start();job.tick();self.assertEqual(job.stage,'CLIENT');self.assertEqual(client.starts,1)
        client.events=[CLIENT_ASSETS,CLIENT_REVOKE];job.tick();job.tick();client.events=[CLIENT_PASS];client.code=0;job.tick();job.tick();job.tick();self.assertEqual(job.stage,'PASS')
    def test_actual_status_fields_no_invented_fields(self):
        x,t,io,e,p,c=entry();a=x.probe();self.assertEqual(a.identity,e.identity);self.assertEqual(a.native_memlock,(2097152,2097152));self.assertEqual(io.reads,[])
        self.assertEqual(set(io.status()),{'utc','rx_bytes','tx_bytes','recent_rx','part','adapter','abort_armed','read_only_inspection_active'})
    def test_initial_proofs_frozen_not_mutable_aliases(self):
        x,t,io,e,p,c=entry();p['boot']['runtime']='invented';p['recovery']['ends_by']='2100-01-01T00:00:00+00:00'
        a=x.probe();self.assertEqual(a.runtime,'actual75_linux');self.assertEqual(a.arm_end,400.)
    def test_async_stdout_does_not_require_recent_rx_ends_prompt(self):
        x,t,io,e,p,c=entry();old=io.status;io.status=lambda:{**old(),'recent_rx':'service_ready=1 source=pinned_service strict_child_ready_reported=1\r\n'}
        launch(x);self.assertEqual(x.probe().prompt,'# ')
    def test_no_health_or_staging_defaults(self):
        for field in ['health','runtime','prompt','kernel_build_id']:
            t,io,e,p,c=bundle();p['boot'].pop(field)
            with self.assertRaises((Hold,KeyError)):n.NativeEntry(SPEC,e,p,c,io,t,lambda:io.wallbase+t())
            self.assertEqual(io.requests,[])
        t,io,e,p,c=bundle();p['staging']['cfg']=[2,768,0]
        with self.assertRaises(Hold):n.NativeEntry(SPEC,e,p,c,io,t,lambda:io.wallbase+t())
        self.assertEqual(io.requests,[])
    def test_arm_end_binds_actual_ACK_not_status_default(self):
        for key,value in [('armed',False),('arm_id','other'),('ends_by','2000-01-01T00:00:00+00:00'),('instance','old')]:
            t,io,e,p,c=bundle();p['recovery'][key]=value
            with self.assertRaises(Hold):n.NativeEntry(SPEC,e,p,c,io,t,lambda:io.wallbase+t())
            self.assertEqual(io.requests,[])
    def test_bounded_RPC_no130_timeout(self):
        x,t,io,e,p,c=entry();x.probe();self.assertTrue(all(0<timeout<=.25 for _,timeout in io.requests));io.delay=.3
        with self.assertRaises(Hold):x.probe()
    def test_phase_budget_no_extension(self):
        x,t,io,e,p,c=entry();x.callbacks['deadline']=lambda:t()-.01
        with self.assertRaises(Hold):x.probe()
    def test_parent_callback_cannot_silently_overrun_budget(self):
        x,t,io,e,p,c=entry();old=x.callbacks['ownership']
        def slow(*a):t.now+=.3;return old(*a)
        x.callbacks['ownership']=slow
        with self.assertRaises(Hold):x.probe()
    def test_pacing_bytecount_sequence_and_no_observe(self):
        x,t,io,e,p,c=entry();x.prepare_startup(lambda cmd:x.checked_command(cmd,x.probe()));io.requests.clear();start=t();r=x.checked_command(startup(SPEC),x.probe());data=(startup(SPEC)+'\r').encode()
        sent=[req for req,_ in io.requests if req['action']=='send']
        self.assertEqual(b''.join(bytes.fromhex(req['hex']) for req in sent),data);self.assertTrue(all(len(bytes.fromhex(req['hex']))<=64 for req in sent))
        self.assertAlmostEqual(t()-start,(len(sent)-1)*.025);self.assertEqual(x.probe().tx_sequence,2);self.assertEqual(io.tx,len(data)+len(health_command())+1);self.assertGreater(r.start_offset,p['receiver']['rx_bytes'])
        with self.assertRaises(Hold):x.checked_command(startup(SPEC),x.probe())
    def test_raw_reads_only_fresh_TX_range(self):
        x,t,io,e,p,c=entry();r=launch(x);chunks=x.uart_capture(r.start_offset)
        self.assertEqual(chunks[0].offset,r.start_offset);self.assertTrue(all(offset>=x.tx_floor for offset,_ in io.reads));self.assertNotIn(b'PRIVATE-HISTORY',chunks[0].data)
        with self.assertRaises(Hold):x.uart_capture(0)
    def test_unknown_TX_bytes_before_effect(self):
        x,t,io,e,p,c=entry();io.tx+=1
        with self.assertRaises(Hold):x.checked_command(startup(SPEC),x.probe())
        self.assertFalse(any(req['action']=='send' for req,_ in io.requests))
    def test_foreign_TX_between_paced_chunks_holds_no_retry(self):
        x,t,io,e,p,c=entry();old=io.rpc;sent=[False]
        def race(*a):
            result=old(*a)
            if a[1]['action']=='send' and not sent[0]:io.tx+=1;sent[0]=True
            return result
        io.rpc=race
        with self.assertRaises(Hold):launch(x)
        self.assertEqual(sum(req['action']=='send' for req,_ in io.requests),1)
    def test_missing_status_field_not_fabricated(self):
        for key in ['abort_armed','tx_bytes','rx_bytes','part','read_only_inspection_active','adapter']:
            x,t,io,e,p,c=entry();old=io.status;io.status=lambda old=old,key=key:{k:v for k,v in old().items() if k!=key}
            with self.assertRaises(Hold):x.probe()
    def test_peer_start_socket_raw_part_adapter_change(self):
        for attr,value in [('peer',(999,1000,1000)),('ticks',101),('sockino',99),('rawino',99),('parts','new'),('adapter','OTHER'),('armed',False),('inspecting',True)]:
            x,t,io,e,p,c=entry();setattr(io,attr,value)
            with self.assertRaises(Hold):x.probe()
    def test_ownership_requires_fresh_fuser_no_transfer_receipt(self):
        for field,value in [('receive_pids',[501,502]),('transfers',['unknown']),('observed',0),('identity',('other',))]:
            x,t,io,e,p,c=entry();old=x.callbacks['ownership'];x.callbacks['ownership']=lambda *a,old=old,field=field,value=value:{**old(*a),field:value}
            with self.assertRaises(Hold):x.probe()
    def test_silent_status_not_UART_loss_zero_proof(self):
        x,t,io,e,p,c=entry();x.probe();self.assertIsNone(x.health)
        with self.assertRaises(Hold):x.checked_command(startup(SPEC),x.probe())
    def test_new_boot_error_busy_module_epoch_denies(self):
        for line in [b'U-Boot banner\n',b'Linux version new\n',b'rx_error_events=1\n',b'mem_busy_ticks=1\n',b'module_epoch=2\n']:
            x,t,io,e,p,c=entry();launch(x);io.rx.extend(line)
            with self.assertRaises(Hold):x.probe()
    def test_raw_loss_rx_rollover_and_prejob_gap(self):
        x,t,io,e,p,c=entry();io.rx.extend(b'unclassified before TX')
        with self.assertRaises(Hold):x.probe()
        x,t,io,e,p,c=entry();launch(x);io.rx=io.rx[:1]
        with self.assertRaises(Hold):x.probe()
    def test_actual_proc_stat_parentheses_and_fixed_exe(self):
        x,t,io,e,p,c=entry();launch(x)
        proof=x.prove_bootstrap(232,230,SPEC.launcher,lambda cmd:x.checked_command(cmd,x.probe()))
        self.assertEqual((proof.pid,proof.parent_pid,proof.start_ticks),(232,230,77))
        sends=sum(req['action']=='send' for req,_ in io.requests);again=x.prove_bootstrap(232,230,SPEC.launcher,lambda cmd:(_ for _ in ()).throw(AssertionError('fresh cache should not inspect')))
        self.assertEqual(again,proof);self.assertEqual(sum(req['action']=='send' for req,_ in io.requests),sends)
    def test_expired_proof_cache_rechecks_only_fixed_owned_PID(self):
        x,t,io,e,p,c=entry();launch(x);send=lambda cmd:x.checked_command(cmd,x.probe())
        x.prove_bootstrap(232,230,SPEC.launcher,send);t.now+=1.01;before=sum(req['action']=='send' for req,_ in io.requests)
        x.prove_bootstrap(232,230,SPEC.launcher,send);self.assertGreater(sum(req['action']=='send' for req,_ in io.requests),before)
        self.assertTrue(all('cat /proc/232/stat' in cmd for cmd in x.commands if cmd.startswith('cat /proc')))
    def test_partial_exe_line_or_wrong_PPID_never_process_proof(self):
        for mode in ['partial','wrong_parent']:
            x,t,io,e,p,c=entry();launch(x)
            def modify(cmd):
                if cmd.startswith('cat /proc/232/stat'):
                    if mode=='partial':io.rx=io.rx[:-2] # remove CRLF: not a complete identity line
                    else:io.rx=bytearray(bytes(io.rx).replace(b' worker) S 230 ',b' worker) S 999 '))
            io.effect=modify
            with self.assertRaises(Hold):x.prove_bootstrap(232,230,SPEC.launcher,lambda cmd:x.checked_command(cmd,x.probe()))
    def test_no_unknown_PID_selector(self):
        x,t,io,e,p,c=entry();launch(x)
        for pid,parent in [(1,230),(999,230),(232,999)]:
            with self.assertRaises(Hold):x.prove_bootstrap(pid,parent,SPEC.launcher,lambda *a:None)
    def test_bad_stat_and_root_fields_not_body_alias(self):
        for text in ['bogus','232 (a)b) S 230','1 () S 0']:
            with self.assertRaises(Hold):n.proc_stat(text)
    def test_poststate_actual_complete_counter_lines_only(self):
        x,t,io,e,p,c=entry();launch(x);x.prove_bootstrap(232,230,SPEC.launcher,lambda cmd:x.checked_command(cmd,x.probe()))
        x.checked_command(post_command(SPEC,232,230),x.probe())
        io.rx.extend(b'inactive\r\nrx_error_events=0 mem_busy_ticks=0 last_status=40080005 rxpos=77; cached diagnostic\r\nresult=-61 readable=0 guard_errors=0\r\nirq=0 irq_frames=0\r\nqueue_started=0 queue_held=0\r\nwb_owner_active=0\r\nUI_NATIVE02_LINEAGE_EXITED\r\nUI_NATIVE02_LISTENER_ABSENT\r\n')
        end=x.probe().rx_end;post=x.poststate(end);self.assertEqual(post['uart_rx_error_events'],0);self.assertEqual(post['watchdog'],'inactive')
        io.rx.extend(b'rx_error_events=0\r\n')
        with self.assertRaises(Hold):x.poststate(x.probe().rx_end)

    def test_poststate_real_shell_prompt_not_partial_counter_receipt(self):
        for tail in (b'# ',b'rx_error_events=0',b'# rx_error_events=0',b'#'):
            x,t,io,e,p,c=entry();launch(x)
            x.prove_bootstrap(232,230,SPEC.launcher,lambda cmd:x.checked_command(cmd,x.probe()))
            x.checked_command(post_command(SPEC,232,230),x.probe())
            io.rx.extend(b'inactive\r\nrx_error_events=0 mem_busy_ticks=0 last_status=40080005 rxpos=77; cached diagnostic\r\nresult=-61 readable=0 guard_errors=0\r\nirq=0 irq_frames=0\r\nqueue_started=0 queue_held=0\r\nwb_owner_active=0\r\nUI_NATIVE02_LINEAGE_EXITED\r\nUI_NATIVE02_LISTENER_ABSENT\r\n'+tail)
            end=x.probe().rx_end
            if tail==b'# ':self.assertEqual(x.poststate(end),POST)
            else:
                with self.assertRaises(Hold):x.poststate(end)

class SocketProtocolTests(unittest.TestCase):
    class Sock:
        def __init__(self,peer=(501,1000,1000),getter_error=False):
            self.peer=peer;self.getter_error=getter_error;self.sent=[];self.timeouts=[]
            self.responses=[json.dumps({'utc':'2026-10-12T00:00:00+00:00','rx_bytes':0,'tx_bytes':0,'recent_rx':'','part':'h-dummy','adapter':'PUBLICDUMMY-A25','abort_armed':True,'read_only_inspection_active':False}).encode(),b'']
        def __enter__(self):return self
        def __exit__(self,*a):pass
        def settimeout(self,t):self.timeouts.append(t)
        def connect(self,path):pass
        def getsockopt(self,*a):
            if self.getter_error:raise OSError('PUBLICDUMMY')
            return struct.pack('3i',*self.peer)
        def sendall(self,data):self.sent.append(data)
        def recv(self,count):return self.responses.pop(0)
    def test_every_connection_has_valid_JSON_no_empty_probe(self):
        s=self.Sock()
        with patch.object(n.socket,'socket',return_value=s):
            peer,response=n.HostIO().rpc('/PUBLICDUMMY/socket',{'action':'status'},(501,1000),.25)
        self.assertEqual(json.loads(s.sent[0]),{'action':'status'});self.assertTrue(s.sent[0].endswith(b'\n'));self.assertEqual(peer,(501,1000,1000));self.assertTrue(all(0<x<=.25 for x in s.timeouts))
    def test_bad_peer_or_getter_always_sends_harmless_JSON_not_command(self):
        for s in [self.Sock(peer=(999,1000,1000)),self.Sock(getter_error=True)]:
            with patch.object(n.socket,'socket',return_value=s):
                with self.assertRaises(Hold):n.HostIO().rpc('/PUBLICDUMMY/socket',{'action':'send','hex':'0d','reason':'PUBLICDUMMY'},(501,1000),.25)
            self.assertEqual(json.loads(s.sent[0]),{'action':'status'})
    def test_slow_fragment_stream_cannot_extend_whole_RPC_deadline(self):
        s=self.Sock();times=iter([100.,100.,100.,100.,100.26])
        with patch.object(n.socket,'socket',return_value=s),patch.object(n.time,'monotonic',side_effect=lambda:next(times)):
            with self.assertRaises(Hold):n.HostIO().rpc('/PUBLICDUMMY/socket',{'action':'status'},(501,1000),.25)

class CLIBindingTests(unittest.TestCase):
    def test_unsafe_CLI_script_callback_never_imported(self):
        from parent_only import load_ownership
        record={'path':n.PUBLIC_SOURCE_INPUTS['probe']['path'],'sha256':n.PUBLIC_SOURCE_INPUTS['probe']['sha256'],'function':'ownership'}
        with self.assertRaises(Hold):load_ownership(record,[str(n.TOOLS)])

if __name__=='__main__':unittest.main(verbosity=2)
