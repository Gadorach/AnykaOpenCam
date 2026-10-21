#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""V4 status/body and real tracked-health models. All IO is injected PUBLICDUMMY."""
import copy,datetime,json,sys,unittest
from pathlib import Path
from unittest.mock import patch
sys.dont_write_bytecode=True
import native_entry as n
from recipe import *
from test_recipe import SPEC
import test_native_entry as fixtures
from test_native_entry import entry,bundle,FakeIO,HEALTH

class Caps(unittest.TestCase):
    def status(self,recent):
        return {'utc':'2026-10-12T00:00:00+00:00','rx_bytes':0,'tx_bytes':0,'recent_rx':recent,'part':'h-dummy','adapter':'PUBLICDUMMY-A25','abort_armed':True,'read_only_inspection_active':False}
    def decode_fragments(self,raw):
        s=fixtures.SocketProtocolTests.Sock();s.responses=[raw[i:i+4096] for i in range(0,len(raw),4096)]+[b'']
        with patch.object(n.socket,'socket',return_value=s):return n.HostIO().rpc('/PUBLICDUMMY/socket',{'action':'status'},(501,1000),.25)
    def test_32KiB_ASCII_recent_actual_JSON(self):
        raw=json.dumps(self.status('x'*32768)).encode();self.assertGreater(len(raw),16384);_,reply=self.decode_fragments(raw);self.assertEqual(len(reply['recent_rx']),32768)
    def test_CTL_worst_case_6x_actual_JSON(self):
        raw=json.dumps(self.status(bytes(32768).decode('utf-8','backslashreplace'))).encode();self.assertGreater(len(raw),6*32768);_,reply=self.decode_fragments(raw);self.assertEqual(reply['recent_rx'],'\0'*32768)
    def test_nonUTF_backslashreplace_actual_JSON(self):
        value=(b'\xff'*32768).decode('utf-8','backslashreplace');raw=json.dumps(self.status(value)).encode();_,reply=self.decode_fragments(raw);self.assertEqual(reply['recent_rx'],value)
    def test_status_EOF_total_cap_and_perfield_cap(self):
        with self.assertRaises(Hold):self.decode_fragments(b'x'*(n.STATUS_BODY_LIMIT+1))
        with self.assertRaises(Hold):self.decode_fragments(json.dumps(self.status('\0'*32769)).encode())
        with self.assertRaises(Hold):self.decode_fragments(json.dumps(self.status('x'*(4*32768+1))).encode())
    def test_unknown_formats_and_nonstring_field_denied(self):
        values=[[],{'extra':1},self.status(None),{**self.status(''),'adapter':1},{**self.status(''),'tx_bytes':True},{**self.status(''),'mystery':0}]
        for value in values:
            with self.assertRaises(Hold):self.decode_fragments(json.dumps(value).encode())
    def test_duplicate_keys_nan_and_small_reply_cap(self):
        for raw in [b'{"utc":1,"utc":2}',b'{"x":NaN}']:
            with self.assertRaises(Hold):n.decode_reply(raw,'status')
        s=fixtures.SocketProtocolTests.Sock();s.responses=[b'x'*4096,b'x',b'']
        with patch.object(n.socket,'socket',return_value=s):
            with self.assertRaises(Hold):n.HostIO().rpc('/PUBLICDUMMY/socket',{'action':'send','hex':'0d','reason':'PUBLICDUMMY'},(501,1000),.25)
    def test_native_HTTP_or_memlock_limits_not_changed(self):
        self.assertEqual(n.STATUS_BODY_LIMIT,6*32768+4096);self.assertEqual(n.SMALL_BODY_LIMIT,4096)
        self.assertIn('2097152',(Path(__file__).parent/'recipe.py').read_text())

class Health(unittest.TestCase):
    def prepare(self,x):x.prepare_startup(lambda cmd:x.checked_command(cmd,x.probe()))
    def test_passive_status_and_ticks_need_no_fresh_target_health(self):
        x,t,io,e,p,c=entry();t.now+=20;x.probe();self.assertIsNone(x.health);self.assertFalse(any(req['action']=='send' for req,_ in io.requests))
    def test_actual_tracked_read_before_startup_no_effect_without_barrier(self):
        x,t,io,e,p,c=entry()
        with self.assertRaises(Hold):x.checked_command(startup(SPEC),x.probe())
        self.prepare(x);self.assertEqual(x.commands,[health_command()]);self.assertTrue(x.healthy());self.assertEqual(x.health_source[0],e.identity)
        x.checked_command(startup(SPEC),x.probe());self.assertIsNone(x.health)
    def test_missing_or_typed_health_line_no_startup(self):
        for replacement in [b'',b'health_ok=1\r\n',b'echo '+HEALTH,HEALTH.replace(b'mem_busy_ticks=0',b'mem_busy_ticks=1')]:
            x,t,io,e,p,c=entry();old=io.rpc
            def altered(*args,old=old,replacement=replacement):
                result=old(*args)
                if args[1]['action']=='send' and bytes.fromhex(args[1]['hex']).endswith(b'\r'):io.rx=bytearray(bytes(io.rx).replace(HEALTH,replacement))
                return result
            io.rpc=altered
            with self.assertRaises(Hold):self.prepare(x)
            self.assertFalse(any('for n in 1 2 3' in cmd for cmd in x.commands))
    def test_partial_health_not_a_barrier(self):
        x,t,io,e,p,c=entry();old=io.rpc
        def partial(*a):
            r=old(*a)
            if a[1]['action']=='send' and bytes.fromhex(a[1]['hex']).endswith(b'\r'):io.rx=io.rx[:-2]
            return r
        io.rpc=partial
        with self.assertRaises(Hold):self.prepare(x)
    def test_stale_health_before_effect_denied(self):
        x,t,io,e,p,c=entry();self.prepare(x);t.now+=1.01
        count=sum(req['action']=='send' for req,_ in io.requests)
        with self.assertRaises(Hold):x.checked_command(startup(SPEC),x.probe())
        self.assertEqual(sum(req['action']=='send' for req,_ in io.requests),count)
    def test_health_expires_during_pacing_CtrlU_no_CR_no_retry(self):
        x,t,io,e,p,c=entry();self.prepare(x);io.sleep=lambda seconds:setattr(t,'now',t()+seconds+.1)
        with self.assertRaises(Hold):x.checked_command(startup(SPEC),x.probe())
        sends=[bytes.fromhex(req['hex']) for req,_ in io.requests if req['action']=='send'];self.assertEqual(sends[-1],b'\x15')
        self.assertFalse(x.startup_sent);self.assertFalse(any('for n in 1 2 3' in cmd for cmd in x.commands));self.assertEqual(io.packet,b'')
        self.assertEqual(x.effects[-1]['event'],'own_uncommitted_line_cancelled_no_effect_claim')
    def test_no_CtrlU_if_linux_semantics_unsealed(self):
        x,t,io,e,p,c=entry();self.prepare(x);x.proofs['boot'].pop('line_cancel');io.sleep=lambda seconds:setattr(t,'now',t()+seconds+.1)
        with self.assertRaises(Hold):x.checked_command(startup(SPEC),x.probe())
        self.assertFalse(any(req.get('hex')=='15' for req,_ in io.requests));self.assertEqual(x.effects[-1]['event'],'HOLD_uncommitted_line_cancel_unconfirmed')
    def test_foreign_TX_and_new_error_during_chunk_never_commits(self):
        for mode in ['foreign','error','reboot']:
            x,t,io,e,p,c=entry();self.prepare(x);old=io.rpc;first=[True]
            def changed(*a):
                r=old(*a)
                if a[1]['action']=='send' and first[0]:
                    first[0]=False
                    if mode=='foreign':io.tx+=1
                    if mode=='error':io.rx.extend(b'rx_error_events=1\r\n')
                    if mode=='reboot':io.rx.extend(b'U-Boot new\r\n')
                return r
            io.rpc=changed
            with self.assertRaises(Hold):x.checked_command(startup(SPEC),x.probe())
            self.assertFalse(x.startup_sent)
    def test_immutable_creation_times_and_current_revalidation_distinct(self):
        t,io,e,p,c=bundle()
        for name in p:p[name]['observed']=20.
        x=n.NativeEntry(SPEC,e,p,c,io,t,lambda:io.wallbase+t());a=x.probe();self.assertEqual(x.proofs['recovery']['observed'],20.);self.assertEqual(a.observed,100.)
        self.prepare(x);self.assertTrue(x.healthy());self.assertEqual(x.proofs['staging']['observed'],20.)
        summary=x.audit_summary();self.assertEqual(summary['evidence_observations']['recovery']['original_observed'],20.);self.assertFalse(summary['creation_timestamps_restamped']);self.assertEqual(summary['last_health_actual_observed'],t())
    def test_old_arm_creation_same_owner_valid_but_changed_end_owner_denied(self):
        t,io,e,p,c=bundle();p['recovery']['observed']=0.;x=n.NativeEntry(SPEC,e,p,c,io,t,lambda:io.wallbase+t());x.probe()
        io.ticks+=1
        with self.assertRaises(Hold):x.probe()
        t,io,e,p,c=bundle();p['recovery']['ends_by']='2000-01-01T00:00:00+00:00'
        with self.assertRaises(Hold):n.NativeEntry(SPEC,e,p,c,io,t,lambda:io.wallbase+t())
    def test_no_restamping_lifetime_cache_as_fresh_fact(self):
        x,t,io,e,p,c=entry();old=x.callbacks['ownership'];x.callbacks['ownership']=lambda *a:{**old(*a),'fact_lifetime':{**old(*a)['fact_lifetime'],'last_actual_observed':t()+.01}}
        with self.assertRaises(Hold):x.probe()
    def test_fresh_parent_health_receipt_used_without_historical_RAW_read(self):
        t,io,e,p,c=bundle();start=len(io.rx);io.rx.extend(HEALTH);p['receiver']['rx_bytes']=len(io.rx);p['boot']['rx_end']=len(io.rx)
        p['boot']['uart_health_receipt']={'identity':list(e.identity),'boot_id':e.boot_id,'part':p['receiver']['part'],'start':start,'end':len(io.rx),'observed':t(),'line':HEALTH.decode()}
        x=n.NativeEntry(SPEC,e,p,c,io,t,lambda:io.wallbase+t());self.prepare(x);self.assertEqual(io.reads,[]);self.assertEqual(x.commands,[]);self.assertEqual(x.health_source[-2:],(start,len(io.rx)))
    def test_bad_initial_health_source_not_typed_flag_permission(self):
        t,io,e,p,c=bundle();p['boot']['uart_health_receipt']={'identity':list(e.identity),'boot_id':e.boot_id,'part':p['receiver']['part'],'start':0,'end':1,'observed':t(),'line':'health_ok=1\n'}
        x=n.NativeEntry(SPEC,e,p,c,io,t,lambda:io.wallbase+t())
        with self.assertRaises(Hold):self.prepare(x)
        self.assertFalse(any(req['action']=='send' for req,_ in io.requests))
    def test_signal_requires_combined_fresh_PID_and_health_once(self):
        x,t,io,e,p,c=entry();self.prepare(x);x.checked_command(startup(SPEC),x.probe())
        send=lambda cmd:x.checked_command(cmd,x.probe());x.prove_bootstrap(232,230,SPEC.launcher,send)
        self.assertEqual(x.commands[-1],inspection_command(232));self.assertTrue(x.healthy())
        x.checked_command(signal_command(SPEC,232,'REVOKE'),x.probe());self.assertIsNone(x.health)
        before=len(x.commands)
        with self.assertRaises(Hold):x.checked_command(signal_command(SPEC,232,'STOP'),x.probe())
        self.assertEqual(len(x.commands),before)
        x.prove_bootstrap(232,230,SPEC.launcher,send);x.checked_command(signal_command(SPEC,232,'STOP'),x.probe())
    def test_combined_inspection_missing_health_or_PIDreuse_no_kill(self):
        for mode in ['nohealth','reuse']:
            x,t,io,e,p,c=entry();self.prepare(x);x.checked_command(startup(SPEC),x.probe());send=lambda cmd:x.checked_command(cmd,x.probe())
            if mode=='reuse':x.prove_bootstrap(232,230,SPEC.launcher,send);t.now+=1.01
            old=io.rpc
            def bad(*args):
                result=old(*args)
                if args[1]['action']=='send' and bytes.fromhex(args[1]['hex']).endswith(b'\r'):
                    if mode=='nohealth':io.rx=bytearray(bytes(io.rx).rsplit(HEALTH,1)[0])
                    else:io.rx=bytearray(bytes(io.rx).replace(b' 77 0 ',b' 78 0 '))
                return result
            io.rpc=bad
            with self.assertRaises(Hold):x.prove_bootstrap(232,230,SPEC.launcher,send)
            self.assertFalse(any('kill -' in cmd for cmd in x.commands))
    def test_health_proof_latency_cannot_extend_phase(self):
        x,t,io,e,p,c=entry();x.callbacks['deadline']=lambda:100.01;io.sleep=lambda seconds:setattr(t,'now',t()+.02)
        # Large startup is never reached; only the read-only barrier is attempted.
        old=io.rpc
        def missing(*args):
            result=old(*args)
            if args[1]['action']=='send':io.rx=bytearray(bytes(io.rx).replace(HEALTH,b''))
            return result
        io.rpc=missing
        with self.assertRaises(Hold):self.prepare(x)
        self.assertFalse(x.startup_sent)

if __name__=='__main__':unittest.main(verbosity=2)
