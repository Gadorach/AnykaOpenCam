#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Parent-only glue to the EXISTING JSON receiver. Import is entirely inactive.

No serial open, receiver creation, arm/reset/observe/staging/private file access.
All tests inject IO/clock/ownership; agents must never configure with HostIO.
"""
from pathlib import Path
from dataclasses import dataclass
import copy,datetime,hashlib,json,math,os,re,socket,stat,struct,time
from recipe import (Admission,BootstrapProof,Chunk,Expected,Hold,Spec,TxReceipt,Scalars,
                    startup,inspection_command,signal_command,post_command,health_command)

ROOT=Path(__file__).resolve().parents[4]
TOOLS=ROOT/'work/lightbulb-camera/implementation-20261008T205404Z'
PUBLIC_SOURCE_INPUTS={
 'uart_capture':{'path':str(TOOLS/'uart_capture.py'),'sha256':'479430335e6b3f740b637848b9df4edf82398ff66199ac9575919178a7a5991a'},
 'probe':{'path':str(TOOLS/'probe.py'),'sha256':'191743d275e176b5217d378f829469a8d2d1f3ca1d963af2558b7ed6e187325c'},
 # Linux checked-command semantics are probe.py's64/.025 SEND, not U-Boot script.
 'checked_command':{'path':str(TOOLS/'probe.py'),'sha256':'191743d275e176b5217d378f829469a8d2d1f3ca1d963af2558b7ed6e187325c'}}
AUDITED_NOT_IMPORTED={'path':str(TOOLS/'checked_command.py'),'sha256':'64b41f33c7eccdc4998e2bd291e8eaec69d4059213d2e1cf4dbc5c3d0d683830'}
STATUS_BODY_LIMIT=6*32768+4096 # control RPC only; NOT camera HTTP/heap/MEMLOCK limits
SMALL_BODY_LIMIT=4096

def validate_reply(value,action):
    if not isinstance(value,dict):deny()
    if action=='status':
        required={'utc','rx_bytes','tx_bytes','recent_rx','part','adapter','abort_armed','read_only_inspection_active'}
        if not required.issubset(value) or set(value)-required-{'session'}:deny()
        recent=value['recent_rx']
        if not isinstance(recent,str) or len(recent)>4*32768 or len(json.dumps(recent,ensure_ascii=True).encode())-2>6*32768:deny()
        for key,limit in [('utc',64),('part',48),('adapter',1024),('session',2048)]:
            if key in value and (not isinstance(value[key],str) or len(value[key])>limit):deny()
        for key in ['rx_bytes','tx_bytes']:
            if type(value[key]) is not int or not 0<=value[key]<2**64:deny()
        for key in ['abort_armed','read_only_inspection_active']:
            if type(value[key]) is not bool:deny()
        metadata={k:v for k,v in value.items() if k!='recent_rx'}
        if len(json.dumps(metadata,ensure_ascii=True,allow_nan=False).encode())>4096:deny()
    elif action=='send':
        if set(value)!= {'transmitted_bytes','utc'} or type(value['transmitted_bytes']) is not int or not 0<=value['transmitted_bytes']<=64 or not isinstance(value['utc'],str) or len(value['utc'])>64:deny()
    else:deny()
    return value

def decode_reply(body,action):
    def pairs(items):
        out={}
        for key,value in items:
            if key in out:deny()
            out[key]=value
        return out
    value=json.loads(body,object_pairs_hook=pairs,parse_constant=lambda _:deny())
    return validate_reply(value,action)

def deny():raise Hold('current_admission_missing') # fixed redacted reason, no response/body text
def proc_stat(text):
    # comm may contain spaces/parentheses. Field22 is index19 after closing comm.
    m=re.fullmatch(r'([1-9][0-9]*) \((.*)\) ([A-Za-z]) (.*)',text.rstrip('\r\n'))
    if not m:deny()
    fields=[m[3]]+m[4].split()
    if len(fields)<20 or not fields[1].isdigit() or not fields[19].isdigit():deny()
    return int(m[1]),int(fields[1]),int(fields[19])

@dataclass(frozen=True)
class Meta:
    socket_dev:int
    socket_ino:int
    receiver_start_ticks:int
    raw_dev:int
    raw_ino:int
    raw_size:int

class HostIO:
    """Only parent invokes these existing-host observations; no action at construction."""
    def source(self,path):return Path(path).read_bytes() # PUBLIC code only
    def meta(self,socket_path,raw_path,pid):
        s=os.lstat(socket_path);r=os.lstat(raw_path)
        if not stat.S_ISSOCK(s.st_mode) or not stat.S_ISREG(r.st_mode):deny()
        p,_,ticks=proc_stat(Path(f'/proc/{pid}/stat').read_text())
        if p!=pid:deny()
        return Meta(s.st_dev,s.st_ino,ticks,r.st_dev,r.st_ino,r.st_size)
    def rpc(self,path,request,expected_peer,timeout):
        # Same audited newline JSON / EOF-response protocol, bounded instead of130s.
        with socket.socket(socket.AF_UNIX,socket.SOCK_STREAM) as s:
            end=time.monotonic()+timeout
            def budget():
                left=end-time.monotonic()
                if left<=0:deny()
                s.settimeout(left)
            budget();s.connect(path)
            peer=None
            try:peer=struct.unpack('3i',s.getsockopt(socket.SOL_SOCKET,socket.SO_PEERCRED,12))
            except OSError:pass
            good=peer is not None and peer[:2]==expected_peer
            # Even a bad peer/getter ALWAYS receives valid harmless JSON, never empty.
            packet=json.dumps(request if good else {'action':'status'}).encode()+b'\n'
            action=request['action'] if good else 'status'
            cap=STATUS_BODY_LIMIT if action=='status' else SMALL_BODY_LIMIT
            budget();s.sendall(packet);body=bytearray()
            while True:
                budget()
                part=s.recv(min(4096,cap+1-len(body)))
                if not part:break
                body.extend(part)
                if len(body)>cap:deny()
            if not good:deny()
            result=decode_reply(body,action)
            return peer,result
    def raw(self,path,identity,offset,count):
        fd=os.open(path,os.O_RDONLY|os.O_NOFOLLOW|os.O_CLOEXEC)
        try:
            s=os.fstat(fd)
            if not stat.S_ISREG(s.st_mode) or (s.st_dev,s.st_ino)!=identity or s.st_size<offset+count:deny()
            data=os.pread(fd,count,offset)
            if len(data)!=count:deny()
            return data
        finally:os.close(fd)
    def sleep(self,seconds):time.sleep(seconds)

class NativeEntry:
    def __init__(self,spec,expected,proofs,callbacks,io,clock,wall):
        self.spec=spec;self.expected=expected;self.proofs=copy.deepcopy(proofs);self.callbacks=callbacks
        self.io=io;self.clock=clock;self.wall=wall;self.sequence=0;self.tx_floor=None
        self.scan_cursor=None;self.rolling=b'';self.last_status=None;self.commands=[];self.proof_cache=None;self.post_start=None
        self.audit=None
        self.health=None;self.health_source=None;self.effects=[];self.startup_sent=False
        self._validate_proofs() # Before ANY syscall/IPC/FS through IO.
        for record in PUBLIC_SOURCE_INPUTS.values():
            if hashlib.sha256(io.source(record['path'])).hexdigest()!=record['sha256']:deny()
        self.initial=self.status(initial=True)
        self.tx_bytes=self.initial['tx_bytes'];self.baseline_rx=self.initial['rx_bytes']
        if self.baseline_rx!=proofs['receiver']['rx_bytes'] or self.tx_bytes!=proofs['receiver']['tx_bytes']:deny()
    def _validate_proofs(self):
        if not isinstance(self.spec,Spec) or not isinstance(self.expected,Expected) or not isinstance(self.proofs,dict):deny()
        if set(self.proofs)!= {'receiver','recovery','boot','staging'}:deny()
        if not isinstance(self.callbacks,dict) or set(self.callbacks)!= {'ownership','deadline'} or any(not callable(v) for v in self.callbacks.values()):deny()
        now=self.clock();r=self.proofs['receiver'];a=self.proofs['recovery'];b=self.proofs['boot'];g=self.proofs['staging']
        for p in (r,a,b,g):
            # Actual evidence creation/observation is immutable, not restamped on RPC.
            if not isinstance(p.get('source_id'),str) or not 0<len(p['source_id'])<=256 or not math.isfinite(p['observed']) or p['observed']>now:deny()
            if p['instance']!=r['instance'] or tuple(p['identity'])!=self.expected.identity or p['part']!=r['part']:deny()
        if not re.fullmatch('[a-z0-9-]{1,48}',r['part']) or r['adapter']!=self.expected.identity[6]:deny()
        if not isinstance(r['instance'],str) or r['instance']!=self.expected.identity[1]:deny()
        if not Path(r['socket_path']).is_absolute() or not Path(r['raw_root']).is_absolute():deny()
        if isinstance(self.io,HostIO) and (Path(r['raw_root'])!=TOOLS/'raw' or Path(r['socket_path'])!=TOOLS.parents[1]/'qc-uart-live/uart.sock'):deny()
        self.raw_path=str(Path(r['raw_root'])/('uart-rx-'+r['part']+'.bin'))
        if r['raw_path']!=self.raw_path or r['rx_bytes']<0 or r['tx_bytes']<0:deny()
        if a['action']!='arm_abort' or a['armed'] is not True or a['arm_id']!=self.expected.arm_id:deny()
        end=datetime.datetime.fromisoformat(a['ends_by']).timestamp()
        # Parent sealed mapping from actual arm ACK wall deadline to this PC monotonic.
        if abs((self.expected.arm_end-now)-(end-self.wall()))>1 or self.expected.arm_end-now<180:deny()
        if (b['boot_id'],b['kernel_build_id'],b['kernel_config_sha256'])!=(self.expected.boot_id,self.expected.kernel_build_id,self.expected.kernel_config_sha256):deny()
        if b['runtime']!='actual75_linux' or b['prompt']!='# ' or b['rx_end']!=r['rx_bytes'] or b['idle_services']!=0:deny()
        if b['health']!= {'uart_rx_error_events':0,'uart_mem_busy_ticks':0,'guard_errors':0,'irq':0,'queue_started':0,'wb_owner_active':0}:deny()
        if tuple(g['paths'])!=(self.spec.launcher,self.spec.private_scope,self.spec.service,self.spec.child) or tuple(g['pins'])!=self.spec.pins:deny()
        if tuple(g['cfg'])!=(2,768,180) or tuple(g['memlock'])!=(2097152,2097152):deny()
        if g['modes']!=[448,[384]*4,[320]*3] or g['owners']!=[0,[0]*4,[0]*3]:deny()
        if not isinstance(g.get('file_ids'),list) or len(g['file_ids'])!=8 or any(not isinstance(x,list) or len(x)!=3 or any(type(v) is not int or v<=0 for v in x) for x in g['file_ids']):deny()
    def timeout(self,maximum=.25):
        left=min(self.callbacks['deadline'](),self.expected.arm_end-5)-self.clock()
        if not math.isfinite(left) or left<=0:deny()
        return min(maximum,left)
    def meta(self):
        r=self.proofs['receiver'];i=self.expected.identity
        m=self.io.meta(r['socket_path'],self.raw_path,i[2])
        if (m.socket_dev,m.socket_ino,m.receiver_start_ticks,m.raw_dev,m.raw_ino)!=(i[7],i[8],i[3],r['raw_dev'],r['raw_ino']):deny()
        deadline=self.clock()+self.timeout()
        receipt=self.callbacks['ownership'](i,r['part'],r['adapter'],deadline)
        if self.clock()>deadline:deny()
        # Explicit fresh parent-bound fuser/no-transfer evidence; never derive missing0.
        if not isinstance(receipt,dict) or not receipt.get('source_id') or tuple(receipt['identity'])!=i or receipt['part']!=r['part'] or receipt['adapter']!=r['adapter'] or not 0<=self.clock()-receipt['observed']<=1:deny()
        if receipt['receive_pids']!=[i[2]] or receipt['transfers']!=[]:deny()
        # Original root fact observations may take seconds. Parent binds their last
        # ACTUAL combined read-only validation to this nonchanging source lifetime;
        # local peer/RX/TX checks maintain it, rather than inventing a fresh timestamp.
        facts=receipt.get('fact_lifetime');b=self.proofs['boot'];g=self.proofs['staging']
        if not isinstance(facts,dict) or not facts.get('source_id') or facts['instance']!=r['instance'] or not math.isfinite(facts['last_actual_observed']) or facts['last_actual_observed']>self.clock():deny()
        if facts['boot_source_id']!=b['source_id'] or facts['staging_source_id']!=g['source_id'] or (facts['boot_id'],facts['kernel_build_id'],facts['kernel_config_sha256'])!=(b['boot_id'],b['kernel_build_id'],b['kernel_config_sha256']):deny()
        if facts['paths']!=g['paths'] or facts['pins']!=g['pins'] or facts['file_ids']!=g['file_ids'] or facts['cfg']!=g['cfg'] or facts['modes']!=g['modes'] or facts['owners']!=g['owners']:deny()
        if hasattr(self,'fact_lifetime') and facts!=self.fact_lifetime:deny() # no restamping old facts
        self.fact_lifetime=copy.deepcopy(facts)
        # Passive local checks are NOT a fresh target health claim. Tracked effect
        # barriers below obtain actual health without recursive ownership->TX->meta.
        self.owner_receipt=receipt
        return m
    def rpc(self,request):
        self.meta();r=self.proofs['receiver'];i=self.expected.identity;start=self.clock();timeout=self.timeout()
        peer,response=self.io.rpc(r['socket_path'],request,(i[4],i[5]),timeout)
        if peer[:2]!=(i[4],i[5]) or self.clock()-start>timeout:deny()
        self.meta();return validate_reply(response,request['action'])
    def status(self,initial=False,allow_tx=None):
        s=self.rpc({'action':'status'});r=self.proofs['receiver']
        required={'utc','rx_bytes','tx_bytes','recent_rx','part','adapter','abort_armed','read_only_inspection_active'}
        if not required.issubset(s) or s['part']!=r['part'] or s['adapter']!=r['adapter'] or s['abort_armed'] is not True or s['read_only_inspection_active'] is not False:deny()
        if type(s['rx_bytes']) is not int or type(s['tx_bytes']) is not int or s['rx_bytes']<0 or s['tx_bytes']<0:deny()
        try:utc=datetime.datetime.fromisoformat(s['utc']).timestamp()
        except (TypeError,ValueError):deny()
        if abs(self.wall()-utc)>1:deny()
        m=self.meta()
        if m.raw_size<s['rx_bytes'] or self.last_status and s['rx_bytes']<self.last_status['rx_bytes']:deny()
        if not initial and s['tx_bytes']!=(self.tx_bytes if allow_tx is None else allow_tx):deny()
        if not initial and self.tx_floor is None and s['rx_bytes']!=self.baseline_rx:deny() # cannot read earlier historical RAW
        self.last_status=s
        if self.tx_floor is not None:self.scan(s['rx_bytes'])
        return s
    def scan(self,end):
        if end-self.scan_cursor>4096:deny() # Unknown loss/burst cannot be papered over.
        if end>self.scan_cursor:
            r=self.proofs['receiver'];data=self.io.raw(self.raw_path,(r['raw_dev'],r['raw_ino']),self.scan_cursor,end-self.scan_cursor)
            self.audit.feed(Chunk(self.expected.identity,self.scan_cursor,data))
            self.scan_cursor=end;self.rolling=(self.rolling+data)[-8192:]
        # Standalone auth job cannot reboot/change module/RX epoch or create UART busy/error.
        bad=(b'U-Boot ',b'Starting kernel',b'Linux version ',b'Kernel panic',b'Rebooting',b'module_epoch=',b'rx_epoch=')
        if any(x in self.rolling for x in bad):deny()
        if re.search(rb'(?:rx_error_events|mem_busy_ticks|guard_errors)=([1-9][0-9]*)',self.rolling):deny()
    def probe(self):
        s=self.status();r=self.proofs['receiver'];b=self.proofs['boot'];g=self.proofs['staging']
        # Values originate from fresh sealed parent receipts maintained by known-TX/raw
        # continuity, not fabricated receiver fields. No phase/no-capture status default.
        owner=self.owner_receipt
        return Admission(self.expected.identity,b['boot_id'],b['kernel_build_id'],b['kernel_config_sha256'],self.expected.arm_id,self.expected.arm_end,
          self.clock(),s['rx_bytes'],self.sequence,len(owner['receive_pids']),len(owner['transfers']),b['runtime'],tuple(g['pins']),
          (g['modes'][0],tuple(g['modes'][1]),tuple(g['modes'][2])),
          (g['owners'][0],tuple(g['owners'][1]),tuple(g['owners'][2])),tuple(g['memlock']),tuple(g['cfg']),
          b['idle_services'] if self.tx_floor is None else -1,b['prompt'])
    def health_lines(self,data,start):
        # Only complete, actual driver scalar lines, never echo/comment/typed flag aliases.
        results=[];offset=start
        pattern=rb'rx_error_events=([0-9]+) mem_busy_ticks=([0-9]+) last_status=[0-9a-fA-F]{8} rxpos=[0-9]+; cached diagnostic, lost bytes cannot be reconstructed'
        for line in data.splitlines(keepends=True):
            end=offset+len(line)
            if line.endswith(b'\n'):
                scalar=line.rstrip(b'\r\n').removeprefix(b'# ')
                match=re.fullmatch(pattern,scalar)
                if match:results.append((int(match[1]),int(match[2]),offset,end))
            offset=end
        if len(results)>1:deny()
        return results
    def set_health(self,data,start,observed):
        rows=self.health_lines(data,start)
        if len(rows)!=1 or rows[0][:2]!=(0,0):deny()
        self.health={'identity':self.expected.identity,'boot_id':self.expected.boot_id,'start':rows[0][2],'end':rows[0][3],'observed':observed}
        self.health_source=(self.expected.identity,self.expected.boot_id,rows[0][2],rows[0][3])
        self.last_health_observed=observed
    def healthy(self):
        return self.health is not None and self.health['identity']==self.expected.identity and self.health['boot_id']==self.expected.boot_id and 0<=self.clock()-self.health['observed']<=1 and self.clock()<self.callbacks['deadline']()
    def prepare_startup(self,checked_tx):
        self.status();receipt=self.proofs['boot'].get('uart_health_receipt')
        if isinstance(receipt,dict) and tuple(receipt.get('identity',()))==self.expected.identity and receipt.get('boot_id')==self.expected.boot_id and receipt.get('part')==self.proofs['receiver']['part'] and type(receipt.get('start')) is int and type(receipt.get('end')) is int and 0<=receipt['start']<receipt['end']<=self.baseline_rx and 0<=self.clock()-receipt['observed']<=1:
            data=receipt['line'].encode('ascii')
            if receipt['end']-receipt['start']!=len(data):deny()
            self.set_health(data,receipt['start'],receipt['observed'])
        if not self.healthy():
            self.pending_health=health_command();start=self.last_status['rx_bytes']
            try:checked_tx(self.pending_health)
            finally:self.pending_health=None
            self.await_health(start)
        if not self.healthy():deny()
    def await_health(self,start):
        deadline=self.clock()+self.timeout(maximum=1);r=self.proofs['receiver']
        while self.clock()<deadline:
            s=self.status();count=s['rx_bytes']-start
            if count>2048:deny()
            data=self.io.raw(self.raw_path,(r['raw_dev'],r['raw_ino']),start,count) if count else b''
            rows=self.health_lines(data,start)
            if rows:self.set_health(data,start,self.clock());return
            self.io.sleep(min(.01,self.timeout(maximum=.01)))
        deny()
    def cancel_uncommitted(self,predicted):
        # Linux current TTY kill-char semantics must have been actually sealed by parent.
        line=self.proofs['boot'].get('line_cancel')
        if not isinstance(line,dict) or not line.get('source_id') or line.get('boot_id')!=self.expected.boot_id or line.get('icanon') is not True or line.get('kill_hex')!='15':
            self.effects.append({'event':'HOLD_uncommitted_line_cancel_unconfirmed'});return
        try:
            self.status(allow_tx=predicted)
            reply=self.rpc({'action':'send','hex':'15','reason':'cancel own uncommitted Linux line; no retry '+self.spec.prefix})
            if reply['transmitted_bytes']!=1:deny()
            self.status(allow_tx=predicted+1);self.tx_bytes=predicted+1
            self.effects.append({'event':'own_uncommitted_line_cancelled_no_effect_claim'})
        except Exception:self.effects.append({'event':'HOLD_uncommitted_line_cancel_unconfirmed'})
    def checked_command(self,command,admission):
        if command==startup(self.spec) and self.startup_sent:deny()
        valid=[startup(self.spec)]
        # After startup, permitted command namespaces are derived only from actual RX PID
        # pair handed to proof/post methods, never user-configured PID selectors.
        if self.proof_cache:
            p=self.proof_cache.pid;parent=self.proof_cache.parent_pid
            valid += [inspection_command(p),signal_command(self.spec,p,'REVOKE'),signal_command(self.spec,p,'STOP'),post_command(self.spec,p,parent)]
        if command not in valid and command not in (getattr(self,'pending_inspection',None),getattr(self,'pending_health',None)):deny()
        effect=command==startup(self.spec) or 'kill -USR1' in command or 'kill -TERM' in command
        if effect and not self.healthy():deny()
        s=self.status()
        if admission.identity!=self.expected.identity or admission.tx_sequence!=self.sequence or admission.rx_end!=s['rx_bytes']:deny()
        if self.tx_floor is None:
            self.tx_floor=s['rx_bytes'];self.scan_cursor=self.tx_floor
            self.audit=Scalars(self.spec,self.expected.identity,self.tx_floor)
        self.audit.echo(command)
        packet=(command+'\r').encode('ascii')
        if any(ord(c)<32 or ord(c)>126 for c in command) or len(packet)>2048:deny()
        predicted=self.tx_bytes
        for offset in range(0,len(packet),64):
            if effect and not self.healthy():
                if offset:self.cancel_uncommitted(predicted)
                deny()
            self.status(allow_tx=predicted);chunk=packet[offset:offset+64]
            if effect and not self.healthy():
                if offset:self.cancel_uncommitted(predicted)
                deny()
            if effect and b'\r' in chunk:self.effects.append({'event':'target_CR_requested_effect_unconfirmed'})
            reply=self.rpc({'action':'send','hex':chunk.hex(),'reason':'parent-reviewed paced Linux command '+self.spec.prefix})
            if type(reply.get('transmitted_bytes')) is not int or reply['transmitted_bytes']!=len(chunk):deny()
            predicted+=len(chunk);self.status(allow_tx=predicted)
            if offset+64<len(packet):
                if self.timeout(maximum=.025)<.025:deny()
                self.io.sleep(.025)
        self.tx_bytes=predicted;self.sequence+=1;self.commands.append(command)
        if command==startup(self.spec):self.startup_sent=True
        if effect:self.health=None # one actual barrier cannot authorize multiple effects
        if command.startswith('cat /sys/class/watchdog'):self.post_start=s['rx_bytes']
        return TxReceipt(self.expected.identity,s['rx_bytes'],self.sequence)
    def audit_summary(self):
        return {'evidence_observations':{kind:{'original_observed':p['observed'],'age':self.clock()-p['observed'],'source_id':p['source_id']} for kind,p in self.proofs.items()},
          'last_actual_fact_validation':self.fact_lifetime['last_actual_observed'],
          'last_current_local_owner_validation':self.owner_receipt['observed'],
          'last_health_actual_source':self.health_source,'last_health_actual_observed':getattr(self,'last_health_observed',None),'effects':copy.deepcopy(self.effects),
          'creation_timestamps_restamped':False,'target_effect_pass_inferred_from_send':False}
    def uart_capture(self,offset):
        s=self.status();r=self.proofs['receiver']
        if self.tx_floor is None or offset<self.tx_floor or offset>s['rx_bytes'] or s['rx_bytes']-offset>4096:deny()
        count=s['rx_bytes']-offset
        data=self.io.raw(self.raw_path,(r['raw_dev'],r['raw_ino']),offset,count) if count else b''
        return [Chunk(self.expected.identity,offset,data)] if data else []
    def prove_bootstrap(self,pid,parent,exe,checked_inspection):
        self.status()
        if self.tx_floor is None or not 1<pid<4194304 or not 1<parent<4194304 or exe!=self.spec.launcher:deny()
        if (pid,parent)!=(self.audit.values.get('boot'),self.audit.values.get('parent')):deny()
        if self.proof_cache and self.healthy() and (self.proof_cache.pid,self.proof_cache.parent_pid,self.proof_cache.exe)==(pid,parent,exe) and self.clock()-self.proof_cache.observed<=1:
            return self.proof_cache # peer/instance/recovery/known TX checked first, no broad target scan
        start=self.last_status['rx_bytes'];self.pending_inspection=inspection_command(pid)
        try:checked_inspection(self.pending_inspection)
        finally:self.pending_inspection=None
        deadline=self.clock()+self.timeout(maximum=1)
        r=self.proofs['receiver']
        while self.clock()<deadline:
            s=self.status();count=s['rx_bytes']-start
            if count>2048:deny()
            data=self.io.raw(self.raw_path,(r['raw_dev'],r['raw_ino']),start,count) if count else b''
            # A partial stat/link line is not process identity, even if its prefix matches.
            complete=data[:data.rfind(b'\n')+1] if b'\n' in data else b''
            lines=[x.removeprefix(b'# ').decode('ascii','strict') for x in complete.splitlines()]
            stats=[];links=[]
            for line in lines:
                if re.fullmatch(r'[1-9][0-9]* \(.*\) [A-Za-z] .*',line):stats.append(proc_stat(line))
                if re.fullmatch(r'l[rwx-]{9}\s+[0-9]+\s+.*\s+/proc/'+str(pid)+r'/exe -> '+re.escape(exe),line):links.append(line)
            if len(stats)>1 or len(links)>1:deny()
            if len(stats)==len(links)==1:
                actual,ppid,ticks=stats[0]
                if actual!=pid or ppid!=parent or ticks<=0 or self.proof_cache and self.proof_cache.start_ticks!=ticks:deny()
                rows=self.health_lines(data,start)
                if not rows:
                    self.io.sleep(min(.01,self.timeout(maximum=.01)));continue
                self.set_health(data,start,self.clock())
                self.proof_cache=BootstrapProof(pid,parent,ticks,exe,self.expected.boot_id,self.expected.identity,self.clock());return self.proof_cache
            self.io.sleep(min(.01,self.timeout(maximum=.01)))
        deny()
    def poststate(self,end):
        s=self.status();r=self.proofs['receiver']
        if self.post_start is None or self.tx_floor is None or not self.post_start<=end<=s['rx_bytes'] or end-self.post_start>4096:deny()
        data=self.io.raw(self.raw_path,(r['raw_dev'],r['raw_ino']),self.post_start,end-self.post_start)
        # Linux's interactive shell emits an unterminated '# ' after the
        # complete post-command lines. It is a prompt, not a counter receipt.
        tail=data[data.rfind(b'\n')+1:]
        if tail not in (b'',b'# '):deny()
        if tail:data=data[:-len(tail)]
        text=data.decode('ascii','strict')
        lines=[line.removeprefix('# ') for line in text.splitlines()]
        # Do not match the echoed command's literals. Consume scalar counter lines only.
        scalars='\n'.join(x for x in lines if not any(token in x for token in ('echo ','cat /','grep ','case ','test ')))
        values={}
        mapping={'rx_error_events':'uart_rx_error_events','mem_busy_ticks':'uart_mem_busy_ticks','readable':'isp_readable',
          'guard_errors':'guard_errors','irq':'irq','irq_frames':'irq_frames','queue_started':'queue_started','queue_held':'queue_held','wb_owner_active':'wb_owner_active'}
        for raw,key in mapping.items():
            matches=re.findall(r'(?<![A-Za-z0-9_])'+raw+r'=([0-9]+)(?![0-9])',scalars)
            if len(matches)!=1:deny()
            values[key]=int(matches[0])
        if lines.count('inactive')!=1 or lines.count(self.spec.prefix+'_LINEAGE_EXITED')!=1 or lines.count(self.spec.prefix+'_LISTENER_ABSENT')!=1:deny()
        if self.spec.prefix+'_LISTENER_BUSY' in lines:deny()
        values.update(watchdog='inactive',remaining_owned_lineage=0,listener_18443=0)
        return values

_entry=None
def configure(spec,expected,publicproofs,callbacks=None,io=None,clock=time.monotonic,wall=time.time):
    global _entry
    if _entry is not None:deny()
    try:_entry=NativeEntry(spec,expected,publicproofs,callbacks,io or HostIO(),clock,wall)
    except Exception:deny() # Missing/unsealed facts are HOLD, never body/exception disclosure.
    return _entry
def probe():
    if _entry is None:deny()
    return _entry.probe()
def prepare_startup(checked_tx):
    if _entry is None:deny()
    return _entry.prepare_startup(checked_tx)
def audit_summary():
    if _entry is None:deny()
    return _entry.audit_summary()
def checked_command(command,admission):
    if _entry is None:deny()
    return _entry.checked_command(command,admission)
def uart_capture(offset):
    if _entry is None:deny()
    return _entry.uart_capture(offset)
def prove_bootstrap(pid,parent,exe,checked_inspection):
    if _entry is None:deny()
    return _entry.prove_bootstrap(pid,parent,exe,checked_inspection)
def poststate(end):
    if _entry is None:deny()
    return _entry.poststate(end)
