#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Offline/injected parent-only recipe. No serial/socket/guardian/device defaults."""
from dataclasses import dataclass
import math,re

class Hold(RuntimeError):
    """Fail closed; no retry or qualification claim."""

SAFE_CODES=frozenset('''marker_prefix approved_run_tmp_paths_only target_dot_alias private_scope_image_collision fresh_names_not_old_scope distinct_images image_pins reviewed_UI_child_pins_required opaque_PC_path fresh_PC_names_required
signal_selector inspection_PID_selector RX_epoch_owner_changed RX_gap RX_line_bound PID_range negative_native_receipt native_refused
typed_live_admission_required current_admission_missing staging_config_or_pin exact_runtime_seal_missing explicit_live_source_receipt_mismatch preflight_not_idle_or_arm_insufficient exact_peer_receive_identity owner_runtime_arm_or_TX_changed finite_service_or_arm_deadline TX_receipt_binding no_restart_or_retry no_source_owned_PID fixed_owned_inspection_only bootstrap_not_current_owned PID_reused phase_timeout_before_signal phase_timeout ACK_without_owned_revoke retirement_without_owned_STOP poststate_before_post_TX nonzero_native_status service_ended_before_client_STOP runtime_left_insufficient_or_already_retired proof_or_runtime_window_expired client_protocol_or_error duplicate_client_receipt late_revoke_window client_exit_without_real_PASS_assets actual_poststate_missing
canonical_client_source_pin no_second_PC_client PC_pidfd_open_failed_no_PID_fallback client_truncated_line client_line_bound client_non_scalar_output client_redacted_failure_or_unknown client_receipt_bound PC_pidfd_missing_no_PID_fallback PC_pidfd_capability_missing
explicit_public_entry_root parent_entry_source_pin existing_entry_callback_missing existing_source_bindings_missing existing_source_binding_pin explicit_parent_inputs_required public_plan_fields canonical_PC_client_path_only fresh_receipt_output_scope
disposed PC_disposal_unconfirmed callback_error callback_error_Hold callback_error_RuntimeError callback_error_ValueError callback_error_TypeError callback_error_OSError callback_error_TimeoutError callback_error_KeyboardInterrupt'''.split())|frozenset('multiple_or_conflicting_'+key for key in ['boot','parent','root_exit','revoke_status','stop_status','class','locked','ready','revoke_sent','revoke_ack','lineage','retired','lineage_absent','listener_absent'])

@dataclass(frozen=True)
class Spec:
    prefix: str
    launcher: str
    private_scope: str
    service: str
    child: str
    pins: tuple  # launcher/service/child public SHA256, supplied from reviewed build
    pc_private_directory: str  # opaque path; this recipe never reads/hashes its files
    def __post_init__(self):
        if not re.fullmatch(r'[A-Z][A-Z0-9_]{1,31}',self.prefix):raise Hold('marker_prefix')
        for path in [self.launcher,self.private_scope,self.service,self.child]:
            if not re.fullmatch(r'/(?:run|tmp)/[A-Za-z0-9_.-]{1,96}',path):raise Hold('approved_run_tmp_paths_only')
            if path.rsplit('/',1)[1] in ('.','..'):raise Hold('target_dot_alias')
            if 'native01' in path.lower() or 'signal04' in path.lower():raise Hold('fresh_names_not_old_scope')
        if len(set([self.launcher,self.service,self.child]))!=3:raise Hold('distinct_images')
        if self.private_scope in (self.launcher,self.service,self.child):raise Hold('private_scope_image_collision')
        if len(self.pins)!=3 or any(not re.fullmatch('[a-f0-9]{64}',x) for x in self.pins):raise Hold('image_pins')
        if self.pins[1:]!=('c9cfc8b954bd5c2370c955f0ce6d886224b27d3899d1972d80baf285fa5f0e2e','8bb3d596065efb35338e2f28bedd60718eac8254d4f569e4412e77567f9c119a'):raise Hold('reviewed_UI_child_pins_required')
        if not self.pc_private_directory.startswith('/') or any(c in self.pc_private_directory for c in '\r\n\0'):raise Hold('opaque_PC_path')
        if 'native01' in self.pc_private_directory.lower() or 'signal04' in self.pc_private_directory.lower():raise Hold('fresh_PC_names_required')

@dataclass(frozen=True)
class Admission:
    # Exact parent-measured source/epoch identity, not a boolean admission alias.
    identity: tuple  # stream_id,protocol_epoch,owner_pid,start_ticks,peer_pid,peer_uid,adapter_id,socket_dev,ino
    boot_id: str
    kernel_build_id: str
    kernel_config_sha256: str
    arm_id: str
    arm_end: float
    observed: float
    rx_end: int
    tx_sequence: int
    sole_receivers: int
    transfers: int
    runtime: str
    source_pins: tuple
    private_modes: tuple  # TARGET scope0700, four0600 FD inputs, three root0500 binaries
    private_owners: tuple  # native scope/files/binaries are root-owned; no permission fallback
    native_memlock: tuple
    cfg_abi_runtime: tuple
    idle_services: int
    prompt: str

@dataclass(frozen=True)
class TxReceipt:
    identity: tuple
    start_offset: int
    sequence: int

@dataclass(frozen=True)
class Chunk:
    identity: tuple
    offset: int
    data: bytes

@dataclass(frozen=True)
class BootstrapProof:
    pid: int
    parent_pid: int
    start_ticks: int
    exe: str
    boot_id: str
    identity: tuple
    observed: float

@dataclass(frozen=True)
class Expected:
    # Parent's explicit measured admission/source receipt, independently re-probed live.
    identity: tuple
    boot_id: str
    kernel_build_id: str
    kernel_config_sha256: str
    arm_id: str
    arm_end: float

def redacted_reason(error):
    value=str(error)
    if isinstance(error,Hold) and value in SAFE_CODES:return value
    code='callback_error_'+type(error).__name__
    return code if code in SAFE_CODES else 'callback_error'

def startup(spec):
    # Guard-only native-tested ordering; same5s/4s/45, no scheduling guarantee.
    p=spec.prefix
    return (f'(exec 3>/dev/watchdog0; echo . >&3; (exec 3>&-; exec {spec.launcher} {spec.private_scope} {spec.service} {spec.child}) & auth04_pid=$!; '
      f'echo {p}_BOOT_PID=$auth04_pid; guard_exec_seen=0; guard_fault=0; guard_stat=$(cat /proc/$auth04_pid/stat); '
      'guard_tail=${guard_stat##*) }; set -- $guard_tail; guard_ppid=${2}; guard_birth=${20}; '
      'case "$guard_ppid:$guard_birth" in *[!0-9:]*|:|:*|*:) guard_fault=1;; esac; '
      'for n in '+' '.join(map(str,range(1,46)))+
      '; do sleep 4; guard_stat=$(cat /proc/$auth04_pid/stat); guard_tail=${guard_stat##*) }; set -- $guard_tail; '
      'case "$guard_stat" in "") echo GUARD_OWN_PID_GONE:$n; break;; esac; '
      'if test "$guard_fault" != 0 || test "${2}" != "$guard_ppid" || test "${20}" != "$guard_birth"; '
      'then guard_fault=1; echo GUARD_SOURCE_REFUSED:$n; break; fi; '
      f'case "$(ls -l /proc/$auth04_pid/exe)" in *" -> {spec.launcher}") '
      'if test "$guard_exec_seen" = 0; then echo GUARD_FIRST_EXEC_MATCH:$n; fi; '
      'guard_exec_seen=1; echo . >&3; echo GUARD_PING:$n;; '
      '*) guard_fault=1; echo GUARD_EXE_REFUSED:$n; break;; esac; done; '
      f'wait $auth04_pid; auth04_rc=$?; echo {p}_BOOT_OWN_EXIT:$auth04_rc; '
      'echo GUARD_WAIT_OBSERVED:$auth04_rc; '
      'if test $auth04_rc = 0 && test "$guard_exec_seen" = 1 && test "$guard_fault" = 0; then printf V >&3; exec 3>&-; '
      f'else echo {p}_NONZERO_NO_RETRY_QUALIFIED_RECOVERY; sleep 6; fi) & echo {p}_PARENT_PID=$!')

def signal_command(spec,pid,kind):
    if kind not in ('REVOKE','STOP') or not isinstance(pid,int) or not 1<pid<4194304:raise Hold('signal_selector')
    name='USR1' if kind=='REVOKE' else 'TERM'
    refusal='REVOKE_REFUSED_WRONG_EXE' if kind=='REVOKE' else 'STOP_EXE_NOT_PRESENT'
    return f'case "$(ls -l /proc/{pid}/exe)" in *" -> {spec.launcher}") kill -{name} {pid}; echo {spec.prefix}_OWN_{kind}_SIGNAL:$?;; *) echo {spec.prefix}_{refusal};; esac'

def inspection_command(pid):
    if not isinstance(pid,int) or not 1<pid<4194304:raise Hold('inspection_PID_selector')
    # No awk/stat/ps/proc argv dependency. Parent parses start_ticks/PPID in memory.
    return f'cat /proc/{pid}/stat; ls -l /proc/{pid}/exe; cat /sys/class/tty/ttyAK0/rx_state'

def health_command():return 'cat /sys/class/tty/ttyAK0/rx_state'

def post_command(spec,pid,parent):
    return (f'cat /sys/class/watchdog/watchdog0/state; cat /sys/class/tty/ttyAK0/rx_state; cat /sys/bus/platform/devices/20000000.isp/state; '
      'grep -E "^(MemAvailable|MemFree|CmaFree):" /proc/meminfo; '
      f'if test ! -e /proc/{pid}/exe && test ! -e /proc/{parent}/exe; then echo {spec.prefix}_LINEAGE_EXITED; fi; '
      'if grep -E "^[[:space:]]*[0-9]+: [0-9A-F]+:480B [0-9A-F]+:[0-9A-F]+ 0A " /proc/net/tcp; '
      f'then echo {spec.prefix}_LISTENER_BUSY; else echo {spec.prefix}_LISTENER_ABSENT; fi')

class Scalars:
    """Anchored complete scalar lines only; no substring/echo/script success detector."""
    def __init__(self,spec,identity,start):
        self.spec=spec;self.identity=identity;self.next=start;self.pending=bytearray();self.values={};self.echoes=set()
        p=re.escape(spec.prefix)
        self.rules=[('boot',rf'{p}_BOOT_PID=([1-9][0-9]{{0,6}})'),('parent',rf'{p}_PARENT_PID=([1-9][0-9]{{0,6}})'),
           ('root_exit',rf'{p}_BOOT_OWN_EXIT:([0-9]{{1,3}})'),('revoke_status',rf'{p}_OWN_REVOKE_SIGNAL:([0-9]{{1,3}})'),
           ('guard_exec',r'GUARD_FIRST_EXEC_MATCH:1'),('guard_ping',r'GUARD_PING:1'),('guard_wait',r'GUARD_WAIT_OBSERVED:0'),
          ('stop_status',rf'{p}_OWN_STOP_SIGNAL:([0-9]{{1,3}})'),
          ('class',r'bootstrap_class=ROOT_RAM_QUALIFICATION nonroot_UID=1000 nonroot_GID=1000'),
          ('locked',r'bootstrap_launched=1 memlock_service_bytes=2097152 memlock_child_bytes=2097152 fd_contract=3_7 readonly_files=1'),
          ('ready',r'service_ready=1 source=pinned_service strict_child_ready_reported=1'),
          ('revoke_sent',r'owner_revoke_sent=1'),('revoke_ack',r'owner_revoke_ack=1'),
          ('lineage',r'owned_lineage_no_remaining_children=1 adopted_exit_receipts=0'),
          ('retired',r'service_wait_exit_observed=1 strict_child_retirement_reported=1 service_status=0 rss_peak_kb=[0-9]+ bootstrap_result=0'),
          ('lineage_absent',rf'{p}_LINEAGE_EXITED'),('listener_absent',rf'{p}_LISTENER_ABSENT')]
    def echo(self,command):
        for line in command.splitlines():
            if line.startswith('# '):line=line[2:]
            self.echoes.add(line.encode())
    def feed(self,chunk):
        if chunk.identity!=self.identity:raise Hold('RX_epoch_owner_changed')
        data=chunk.data
        if chunk.offset+len(data)<=self.next:return # historical byte ranges are never evidence
        if chunk.offset>self.next:raise Hold('RX_gap')
        data=data[self.next-chunk.offset:];self.next+=len(data)
        for byte in data:
            if byte!=10:
                self.pending.append(byte)
                if len(self.pending)>2048:raise Hold('RX_line_bound')
                continue
            line=bytes(self.pending);self.pending.clear()
            if line.endswith(b'\r'):line=line[:-1]
            if line.startswith(b'# '):line=line[2:]
            if line in self.echoes:continue
            try:text=line.decode('ascii')
            except UnicodeDecodeError:continue
            for key,pattern in self.rules:
                m=re.fullmatch(pattern,text)
                if not m:continue
                value=int(m[1]) if m.lastindex else True
                if key in ('boot','parent') and not 1<value<4194304:raise Hold('PID_range')
                if key in self.values:raise Hold('multiple_or_conflicting_'+key)
                self.values[key]=value
            # Explicit errors are not silently treated as an absent marker.
            if re.fullmatch(r'owner_revoke_ack=0|owner_revoke_sent=0|service_ready=0.*',text) or text==self.spec.prefix+'_LISTENER_BUSY':raise Hold('negative_native_receipt')
            if re.fullmatch(r'GUARD_(?:SOURCE|EXE)_REFUSED:[0-9]+|GUARD_WAIT_OBSERVED:[1-9][0-9]*',text):raise Hold('negative_native_receipt')
            for suffix in ['REVOKE_REFUSED_WRONG_EXE','STOP_EXE_NOT_PRESENT','NONZERO_NO_RETRY_QUALIFIED_RECOVERY']:
                if text==self.spec.prefix+'_'+suffix:raise Hold('native_refused')

CLIENT_PASS='private_HTTPS_campaign=PASS protected_status=1 full600k=1 logout_replay=1 discard_drain=1 media_unqualified=1'
CLIENT_ASSETS='native_UI_static_assets=PASS count=11 source_bytes_exact=1 CSP=1 media_unqualified=1'
CLIENT_REVOKE='private_campaign_owner_revoke_window=1'

class Campaign:
    """One finite parent job; backend methods MUST use existing validated ownership/admission."""
    def __init__(self,spec,expected,backend,client,clock):
        self.spec=spec;self.expected=expected;self.backend=backend;self.client=client;self.clock=clock
        self.stage='NEW';self.history=[];self.launched=False;self.client_started=False;self.proof=None;self.client_flags=set();self.failed=None
        self.stop_authorized=False
    def record(self,name):self.history.append({'event':name,'time':self.clock()}) # scalar-only, no raw logs/material hashes
    def fresh(self,initial=False):
        a=self.backend.probe();now=self.clock()
        if not isinstance(a,Admission):raise Hold('typed_live_admission_required')
        if not all(math.isfinite(x) for x in (now,a.observed,a.arm_end)) or not 0<=now-a.observed<=1 or a.sole_receivers!=1 or a.transfers!=0 or a.runtime!='actual75_linux' or a.prompt!='# ':raise Hold('current_admission_missing')
        if a.source_pins!=self.spec.pins or a.private_modes!=(0o700,(0o600,)*4,(0o500,)*3) or a.private_owners!=(0,(0,)*4,(0,)*3) or a.native_memlock!=(2097152,2097152) or a.cfg_abi_runtime!=(2,768,180):raise Hold('staging_config_or_pin')
        if not a.boot_id or not re.fullmatch('(?:[a-f0-9]{2}){8,32}',a.kernel_build_id) or not re.fullmatch('[a-f0-9]{64}',a.kernel_config_sha256):raise Hold('exact_runtime_seal_missing')
        if initial:
            if not isinstance(self.expected,Expected) or (a.identity,a.boot_id,a.kernel_build_id,a.kernel_config_sha256,a.arm_id,a.arm_end)!=(self.expected.identity,self.expected.boot_id,self.expected.kernel_build_id,self.expected.kernel_config_sha256,self.expected.arm_id,self.expected.arm_end):raise Hold('explicit_live_source_receipt_mismatch')
            if a.idle_services!=0 or len(a.identity)!=9 or a.arm_end-now<180:raise Hold('preflight_not_idle_or_arm_insufficient')
            stream,epoch,owner,ticks,peer,uid,adapter,dev,ino=a.identity
            if not stream or not epoch or not adapter or type(owner) is not int or type(peer) is not int or owner!=peer or owner<=1 or type(ticks) is not int or ticks<=0 or type(uid) is not int or uid<0 or dev<=0 or ino<=0:raise Hold('exact_peer_receive_identity')
            self.admission=a;self.sequence=a.tx_sequence;self.end=min(now+180,a.arm_end-5)
        elif (a.identity,a.boot_id,a.kernel_build_id,a.kernel_config_sha256,a.arm_id,a.arm_end)!=(self.admission.identity,self.admission.boot_id,self.admission.kernel_build_id,self.admission.kernel_config_sha256,self.admission.arm_id,self.admission.arm_end) or a.tx_sequence!=self.sequence:
            raise Hold('owner_runtime_arm_or_TX_changed')
        if now>=self.end:raise Hold('finite_service_or_arm_deadline')
        return a
    def tx(self,command):
        a=self.fresh();r=self.backend.checked_command(command,a)
        if not isinstance(r,TxReceipt) or r.identity!=a.identity or r.start_offset!=a.rx_end or r.sequence!=a.tx_sequence+1:raise Hold('TX_receipt_binding')
        self.sequence=r.sequence
        if not hasattr(self,'rx'):self.rx=Scalars(self.spec,r.identity,r.start_offset)
        self.rx.echo(command);return r
    def start(self):
        if self.stage!='NEW':raise Hold('no_restart_or_retry')
        try:
            self.client.preflight();self.fresh(initial=True);self.started=self.clock()
            self.phase_end=min(self.end,self.clock()+10) # health/pre-start work is within existing entry budget
            prepare=getattr(self.backend,'prepare_startup',None)
            if callable(prepare):prepare(self.tx)
            self.tx(startup(self.spec));self.launched=True
            self.stage='WAIT_READY';self.phase_end=min(self.end,self.clock()+10);self.record('one_bootstrap_TX')
        except Exception as e:self.abort(redacted_reason(e))
    def owned(self):
        self.fresh();v=self.rx.values
        if 'boot' not in v or 'parent' not in v:raise Hold('no_source_owned_PID')
        def checked_inspection(command):
            if command!=inspection_command(v['boot']):raise Hold('fixed_owned_inspection_only')
            return self.tx(command)
        # Existing parent inspector may use a fresh retained receipt, or this tracked
        # fixed read-only TX callback. Hidden/unregistered TX changes fail admission.
        proof=self.backend.prove_bootstrap(v['boot'],v['parent'],self.spec.launcher,checked_inspection)
        if not isinstance(proof,BootstrapProof) or (proof.pid,proof.parent_pid,proof.exe,proof.boot_id,proof.identity)!=(v['boot'],v['parent'],self.spec.launcher,self.admission.boot_id,self.admission.identity) or proof.start_ticks<=0 or not 0<=self.clock()-proof.observed<=1:raise Hold('bootstrap_not_current_owned')
        if self.proof and proof.start_ticks!=self.proof.start_ticks:raise Hold('PID_reused')
        self.proof=proof;return proof
    def native_signal(self,kind):
        if kind=='REVOKE' and (self.stage!='CLIENT' or CLIENT_REVOKE not in self.client_flags):raise Hold('late_revoke_window')
        if kind=='STOP' and self.stage!='HOLD' and not self.stop_authorized:raise Hold('retirement_without_owned_STOP')
        proof=self.owned()
        if self.stage!='HOLD' and self.clock()>=self.phase_end:raise Hold('phase_timeout_before_signal')
        self.tx(signal_command(self.spec,proof.pid,kind));self.record('owned_'+kind+'_TX')
    def tick(self):
        if self.stage in ('NEW','PASS','HOLD'):return
        try:
            self.fresh()
            previous=set(self.rx.values)
            for chunk in self.backend.uart_capture(self.rx.next):self.rx.feed(chunk)
            v=self.rx.values;now=self.clock()
            if now>=self.phase_end:raise Hold('phase_timeout')
            new=set(v)-previous
            if new.intersection({'revoke_sent','revoke_ack','revoke_status'}) and self.stage!='ACK':raise Hold('ACK_without_owned_revoke')
            if new.intersection({'stop_status','lineage','retired'}) and self.stage!='RETIRE':raise Hold('retirement_without_owned_STOP')
            if new.intersection({'lineage_absent','listener_absent'}) and self.stage!='POST':raise Hold('poststate_before_post_TX')
            if any(v.get(k,0)!=0 for k in ['root_exit','revoke_status','stop_status']):raise Hold('nonzero_native_status')
            if 'root_exit' in v and self.stage not in ('WAIT_READY','RETIRE','POST'):raise Hold('service_ended_before_client_STOP')
            if self.stage=='WAIT_READY' and all(v.get(k) is True for k in ['class','locked','ready']) and 'boot' in v and 'parent' in v:
                if 'root_exit' in v or self.end-now<155:raise Hold('runtime_left_insufficient_or_already_retired')
                self.owned();now=self.clock()
                if now>=self.phase_end or self.end-now<155:raise Hold('proof_or_runtime_window_expired')
                self.client_started=True;self.client.start(self.spec.pc_private_directory)
                self.stage='CLIENT';self.client_end=min(now+140,self.end-15);self.phase_end=self.client_end;self.record('client_started_on_bound_READY')
            if self.client_started and self.stage in ('CLIENT','ACK'):
                for line in self.client.lines():
                    if line not in (CLIENT_PASS,CLIENT_ASSETS,CLIENT_REVOKE):raise Hold('client_protocol_or_error')
                    if line in self.client_flags:raise Hold('duplicate_client_receipt')
                    self.client_flags.add(line)
                    if line==CLIENT_REVOKE:
                        if self.stage!='CLIENT':raise Hold('late_revoke_window')
                        self.native_signal('REVOKE');self.stage='ACK';self.phase_end=min(now+2,self.client_end)
                code=self.client.poll()
                if code is not None and (code!=0 or CLIENT_PASS not in self.client_flags or CLIENT_ASSETS not in self.client_flags):raise Hold('client_exit_without_real_PASS_assets')
                if self.stage=='ACK' and v.get('revoke_ack') and v.get('revoke_sent') and v.get('revoke_status')==0:
                    self.stage='CLIENT';self.phase_end=self.client_end;self.record('actual_owner_ACK')
                if code==0 and self.stage=='CLIENT' and 'actual_owner_ACK' in [x['event'] for x in self.history] and CLIENT_PASS in self.client_flags:
                    self.stop_authorized=True
                    self.native_signal('STOP');self.stage='RETIRE';self.phase_end=min(now+8,self.end)
            if self.stage=='RETIRE' and all(v.get(k) is True for k in ['lineage','retired','guard_exec','guard_ping','guard_wait']) and v.get('stop_status')==0 and v.get('root_exit')==0:
                self.tx(post_command(self.spec,v['boot'],v['parent']));self.stage='POST';self.phase_end=min(now+5,self.end)
            if self.stage=='POST' and v.get('lineage_absent') and v.get('listener_absent'):
                # Parent probe must derive these from fresh post-command RX, not success booleans.
                post=self.backend.poststate(self.rx.next)
                expected={'watchdog':'inactive','uart_rx_error_events':0,'uart_mem_busy_ticks':0,'isp_readable':0,'guard_errors':0,
                  'irq':0,'irq_frames':0,'queue_started':0,'queue_held':0,'wb_owner_active':0,'remaining_owned_lineage':0,'listener_18443':0}
                if post!=expected:raise Hold('actual_poststate_missing')
                self.stage='PASS';self.record('client_ACK_STOP_retirement_rootwait_poststate_observed')
            if self.stage!='PASS' and self.clock()>=self.phase_end:raise Hold('phase_timeout')
        except Exception as e:self.abort(redacted_reason(e))
    def abort(self,reason):
        if self.stage=='HOLD':return
        if reason not in SAFE_CODES:reason='callback_error'
        self.failed=reason;self.record('HOLD_'+reason)
        self.stage='HOLD'
        if self.client_started:
            try:self.client.cancel() # Must be exact owned PC pidfd, never PID-based.
            except Exception:self.record('HOLD_PC_pidfd_cancel_unconfirmed')
        # Stop only this freshly RX-bound bootstrap if live admission still validates.
        if self.launched and hasattr(self,'rx'):
            try:self.native_signal('STOP')
            except Exception:self.record('HOLD_no_current_source_authority_for_target_STOP')
        # No magic-close/reset/rearm/restart and no second service.
