#!/usr/bin/env python3
"""Explicit-target UART RX recording with logged, separately requested TX.

No reset/power/boot-control operation. Private control socket. Raw RX is never
trimmed or decoded in place. Optional transmissions are recorded beforehand,
except explicitly private credential provisioning (metadata-only TX and a
bounded, documented RX redaction window). Firmware acquisitions never use it.
"""
import argparse,datetime,fcntl,hashlib,json,os,re,select,socket,stat,sys,time
from pathlib import Path
import serial

ROOT=Path('/home/gadorach/Desktop/AnykaOpenCam/work/lightbulb-camera/implementation-20261008T205404Z')
CONTROL=ROOT.parents[1]/'qc-uart-live'
SOCKET=CONTROL/'uart.sock'
def utc():return datetime.datetime.now(datetime.timezone.utc).isoformat()
def newest_boot_bytes(data):
    """Discard prior Linux/SF handoff text before a newly observed U-Boot banner."""
    marker=b'\r\n\r\nU-Boot 2013.10.0-AK_V2.0.03'
    index=data.rfind(marker)
    return data[index:] if index>0 else data

def checked_tx_ready(request, rx_bytes, pending_rx):
    """Optional compare-before-send for cooperating framed-upload clients.

    No serial mutation on a stale RX watermark or already queued UART input.
    This cannot prevent a physical reset arriving after the final queue check.
    """
    if 'expected_rx_bytes' not in request:return True
    expected=request['expected_rx_bytes']
    if type(expected) is not int or expected<0:raise ValueError('Invalid checked TX watermark')
    return expected==rx_bytes and not pending_rx

def private_directory(path, fresh=False):
    """Private capture only: canonical, no symlink/Git, trusted nonwritable ancestors."""
    value=str(path);p=Path(value);uid=os.getuid()
    if not p.is_absolute() or str(p)!=value or any(s in ('.','..') for s in value.split('/')):
        raise ValueError('Unsafe private envelope')
    for parent in reversed(p.parents):
        st=parent.lstat()
        if not stat.S_ISDIR(st.st_mode) or st.st_uid not in (0,uid) or st.st_mode&0o022:
            raise ValueError('Unsafe private ancestors')
        if os.path.lexists(parent/'.git'):raise ValueError('Private capture must be outside Git')
    parent=p.parent.lstat()
    if parent.st_uid!=uid or stat.S_IMODE(parent.st_mode)!=0o700:
        raise ValueError('Private envelope requires protected owned parent')
    if fresh:
        if os.path.lexists(p):raise ValueError('Private capture must be fresh')
    else:
        st=p.lstat()
        if not stat.S_ISDIR(st.st_mode) or st.st_uid!=uid or stat.S_IMODE(st.st_mode)!=0o700 or os.path.lexists(p/'.git'):
            raise ValueError('Unsafe private capture directory')
    return p

def protected_tx_ready(request, rx_bytes, pending_rx, private_capture):
    if not private_capture or 'expected_rx_bytes' not in request:
        raise ValueError('Protected send requires private capture and exact RX watermark')
    return checked_tx_ready(request,rx_bytes,pending_rx)

def capture_status(state, recent, private_capture, private_status=False):
    """Default status never exports private RX, even to a legacy public client."""
    return {**state,'private_capture_v1':bool(private_capture),
            'recent_rx':recent.decode('utf-8','backslashreplace') if not private_capture or private_status is True else ''}

def _native_capture_timer_status(start,duration):
    """Stored values only; no new sample, expiry bound or coverage grant."""
    if type(start) is not float or type(duration) is not int or start<0 or not 120<=duration<=14400:
        raise ValueError('Native capture timer unavailable')
    try:
        start.as_integer_ratio()
    except (ValueError,OverflowError):
        raise ValueError('Native capture timer unavailable') from None
    return {'schema':'qc07f.recorder.native_capture_timer','version':1,
            'start_encoding':'python.float.hex','start_hex':start.hex(),
            'duration_seconds':duration,'clock_api':'time.monotonic()',
            'loop_predicate':'running and time.monotonic()-start<duration',
            'scope':'STORED_VALUES_ONLY_NOT_DEADLINE_OR_ADMISSION',
            'arm_timer_role':'CAPTURE_TIMER_ONLY_NO_INDEPENDENT_ARM_DEADLINE'}

def client(request):
    with socket.socket(socket.AF_UNIX,socket.SOCK_STREAM) as s:
        s.settimeout(130);s.connect(str(SOCKET));s.sendall(json.dumps(request).encode()+b'\n');chunks=[]
        while data:=s.recv(65536):chunks.append(data)
        return json.loads(b''.join(chunks))

def _checked_selected_port(selected_port):
    """Lexical role check only; no lookup, normalization or identity proof."""
    if type(selected_port) is not str or not 0<len(selected_port)<=512:
        raise ValueError('Capture requires an explicit valid by-id port')
    prefix='/dev/serial/by-id/'
    if not selected_port.startswith(prefix):
        raise ValueError('Capture requires an explicit valid by-id port')
    component=selected_port[len(prefix):]
    if component=='.' or '..' in component or not re.fullmatch(r'[A-Za-z0-9_.:+-]+',component):
        raise ValueError('Capture requires an explicit valid by-id port')

def capture(duration,part,selected_port,private_capture_dir=None):
    _checked_selected_port(selected_port)
    if part and not re.fullmatch(r'[a-z0-9-]+',part):raise ValueError('Unsafe part name')
    suffix='-'+part if part else ''
    private_capture=private_capture_dir is not None
    envelope=None
    if private_capture:
        if not 120<=duration<=14400 or not re.fullmatch(r'foundation-\d{8}[a-z]',part):
            raise ValueError('Private capture requires bounded duration and explicit foundation part')
        envelope=private_directory(private_capture_dir,fresh=True)
        envelope.mkdir(mode=0o700,exist_ok=False)
        private_directory(envelope)
    CONTROL.mkdir(mode=0o700,exist_ok=True)
    st=CONTROL.lstat()
    if not stat.S_ISDIR(st.st_mode) or st.st_uid!=os.getuid() or st.st_mode&0o777!=0o700:raise ValueError('Unsafe control directory')
    if os.path.lexists(SOCKET):raise FileExistsError('Existing UART owner/socket; do not take over')
    rawpath=envelope/('uart-rx'+suffix+'.bin') if private_capture else ROOT/('raw/uart-rx'+suffix+'.bin')
    logpath=envelope/('uart-events'+suffix+'.jsonl') if private_capture else ROOT/('logs/uart-events'+suffix+'.jsonl')
    finalpath=envelope/('capture-final'+suffix+'.json') if private_capture else ROOT/('analysis/capture-final'+suffix+'.json')
    def private_open(path,mode,buffering=-1):
        fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW|os.O_CLOEXEC,0o600)
        os.fchmod(fd,0o600)
        return os.fdopen(fd,mode,buffering=buffering)
    raw=private_open(rawpath,'wb',0) if private_capture else rawpath.open('xb',buffering=0)
    log=private_open(logpath,'w',1) if private_capture else logpath.open('x',buffering=1)
    def event(kind,**fields):log.write(json.dumps({'utc':utc(),'monotonic_ns':time.monotonic_ns(),'kind':kind,**fields})+'\n')
    port=serial.Serial(port=None,baudrate=115200,bytesize=8,parity='N',stopbits=1,timeout=0,write_timeout=2,xonxoff=False,rtscts=False,dsrdtr=False,exclusive=True)
    port.dtr=False;port.rts=False;port.port=selected_port
    event('open_intent',port=selected_port,realpath=str(Path(selected_port).resolve()),settings=port.get_settings(),dtr_requested=False,rts_requested=False,
          warning='FTDI/OS may transiently assert modem lines at open; no electrical waveform measured. pyserial open flushes pending input once. Capture cannot recover earlier bytes.')
    server=socket.socket(socket.AF_UNIX,socket.SOCK_STREAM);owned_socket=None
    source_sha=hashlib.sha256(Path(__file__).read_bytes()).hexdigest() if private_capture else None
    running=True;rxbytes=0;txbytes=0;start=time.monotonic();pending=[];recent=bytearray()
    private_until=0;private_rxbytes=0
    arm=None;burst=[];burst_key=b'U-C!FItA\r';burst_reason='One-shot next-boot QC07F keyed-autoboot RX probe; REA default key, no environment or flash write';bootbuf=bytearray();inspect_commands=[];inspecting=False;promptbuf=bytearray()
    try:
        port.open();event('opened',fd=port.fileno(),pid=os.getpid(),cts=port.cts,dsr=port.dsr)
        server.bind(str(SOCKET))
        socket_stat=SOCKET.lstat();owned_socket=(socket_stat.st_dev,socket_stat.st_ino)
        os.chmod(SOCKET,0o600);server.listen();server.setblocking(False)
        print('Private UART capture armed; data withheld' if private_capture else 'Receive-only UART recording armed: '+selected_port+' 1152008N1',flush=True)
        def transmit(data,reason,private=False):
            nonlocal txbytes
            if private:
                event('tx_private_intent',bytes=len(data),reason='Authorized runtime credential provisioning; payload intentionally not retained')
            elif private_capture:
                event('tx_protected_intent',bytes=len(data),reason='Protected capture transport; no TX payload in events')
            else:
                event('tx_intent',hex=data.hex(),text=data.decode('utf-8','backslashreplace'),reason=reason)
            if private:
                # The target's polled RX ring is128bytes. Credential privacy
                # does not authorize an unpaced multi-ring burst. Keep every
                # chunk private and below the qualified public64-byte bound.
                n=0
                for offset in range(0,len(data),64):
                    chunk=data[offset:offset+64]
                    written=port.write(chunk);port.flush()
                    if written!=len(chunk):raise IOError('Incomplete private UART chunk')
                    n+=written
                    if offset+64<len(data):time.sleep(.025)
            else:
                n=port.write(data);port.flush()
            txbytes+=n;event('tx_complete',bytes=n);return n
        def response(c,obj):
            try:c.sendall(json.dumps(obj).encode()+b'\n')
            finally:c.close()
        while running and time.monotonic()-start<duration:
            ready,_,_=select.select([port.fileno(),server],[],[],0.005 if burst else 0.05)
            if port.fileno() in ready:
                data=port.read(65536)
                if data:
                    if time.monotonic()<private_until:
                        private_rxbytes+=len(data)
                        event('rx_private_redacted',bytes=len(data),reason='Bounded credential provisioning window; raw/runtime RX intentionally not retained')
                        continue
                    raw.write(data)
                    if private_capture:event('rx',offset=rxbytes,bytes=len(data))
                    else:event('rx',offset=rxbytes,bytes=len(data),hex=data.hex(),text=data.decode('utf-8','backslashreplace'))
                    rxbytes+=len(data)
                    recent.extend(data);del recent[:-32768]
                    bootbuf.extend(data);del bootbuf[:-32768]
                    if arm is not None:
                        fresh=newest_boot_bytes(bootbuf)
                        if len(fresh)!=len(bootbuf):
                            event('new_boot_window_reset',discarded_prior_bytes=len(bootbuf)-len(fresh))
                            bootbuf=bytearray(fresh)
                    if not private_capture:print(data.decode('utf-8','backslashreplace'),end='',flush=True)
                    for observer in pending:observer['data'].extend(data)
                    early_hold = arm is not None and arm['mode']=='hold-cli' and b'U-Boot 2013.10.0-AK_V2.0.03' in bootbuf and b'DRAM:' in bootbuf
                    net_ready = arm is not None and b'Net:' in bootbuf and b'\n' in bootbuf[bootbuf.find(b'Net:'):]
                    if arm is not None and (early_hold or net_ready):
                        if b'Starting kernel' in bootbuf or b'SF:' in bootbuf:
                            event('abort_arm_missed_window',reason='Network marker and kernel-read/handoff already received together; no late transmission')
                            arm=None
                        else:
                            burst_key=arm['key']
                            prequeue=arm['mode'] in ['delay-key-prequeue','stock-uart-debug','stock-rescue-shell','hold-cli']
                            attempts=3 if early_hold else (1 if prequeue else 5)
                            if arm['mode']=='hold-cli':
                                burst_reason='REA-verified default delay key, stop key and delay key; stop-key path sets volatile retry_time=-1 so U-Boot CLI can be inspected without its faulty command timeout; no flash or environment save'
                            elif arm['mode']=='stock-rescue-shell':
                                burst_reason='Password-patch acquisition: unchanged stock image, VOLATILE uart_debug=1 init=/bin/sh, skip stock app/PTZ startup; no credential or flash change; power-cycle restores normal boot'
                            elif arm['mode']=='stock-uart-debug':
                                burst_reason='Verified stock image/load policy: append uart_debug=1 to VOLATILE bootargs and run stock boot_normal; no saveenv or flash write; recovery by power-cycle'
                            else:
                                burst_reason='Prior corrected-wiring CLI verified; two boot keys and fixed read-only printenv/help/version batch prequeued to avoid defective command timeout' if prequeue else 'One-shot next-boot QC07F keyed-autoboot RX probe; REA default key, no environment or flash write'
                            event('abort_arm_triggered',marker='early U-Boot banner/DRAM for bounded key prequeue' if early_hold else 'complete Net line from new boot',payload_hex=burst_key.hex(),attempts=attempts,prequeued_commands=prequeue,volatile_bootargs=arm['mode'] in ['stock-uart-debug','stock-rescue-shell'])
                            burst=[time.monotonic()+i*(0.1 if early_hold else 0.02) for i in range(attempts)]
                            inspect_commands=['version','help','printenv'] if arm['inspect'] and not prequeue else []
                            inspecting=bool(inspect_commands);promptbuf.clear();arm=None
                    if burst and (b'Starting kernel' in data or b'SF:' in data or b'anyka$' in data or b'Please enter password:' in bootbuf):
                        event('abort_burst_cancelled',reason='boot read/handoff, CLI or password prompt observed');burst=[]
                    if inspecting:
                        promptbuf.extend(data);del promptbuf[:-8192]
                        if promptbuf.rstrip().endswith(b'anyka$'):
                            if inspect_commands:
                                command=inspect_commands.pop(0)
                                transmit(command.encode('ascii')+b'\r','Automatic read-only U-Boot inspection at REA-identified anyka$ prompt: '+command)
                                promptbuf.clear()
                            else:
                                event('read_only_inspection_completed');inspecting=False
                        elif b'Starting kernel' in data:
                            event('inspection_cancelled',reason='Kernel handoff, no CLI');inspecting=False;inspect_commands=[]
            while burst and time.monotonic()>=burst[0]:
                burst.pop(0)
                transmit(burst_key,burst_reason)
            if server in ready:
                c,_=server.accept();c.settimeout(2);buf=b''
                try:
                    while b'\n' not in buf and len(buf)<65536:
                        chunk=c.recv(4096)
                        if not chunk:break
                        buf+=chunk
                    req=json.loads(buf.split(b'\n')[0]);action=req.get('action')
                    if action=='status':
                        state={'utc':utc(),'rx_bytes':rxbytes,'tx_bytes':txbytes,'session':str(envelope if private_capture else ROOT),'part':part,'adapter':selected_port,'abort_armed':arm is not None,'read_only_inspection_active':inspecting,'conditional_send_v1':True,'tx_busy':bool(burst or inspecting or time.monotonic()<private_until)}
                        if private_capture:state.update({'private_capture_dir':str(envelope),'raw_path':str(rawpath),'capture_source_sha256':source_sha,'protected_send_v1':True})
                        if private_capture and req.get('private_status') is True:
                            state['native_capture_timer_v1']=_native_capture_timer_status(start,duration)
                        response(c,capture_status(state,recent,private_capture,req.get('private_status',False)))
                    elif action=='arm_abort':
                        if arm is not None or burst or inspecting:raise ValueError('Already armed/active')
                        mode=req.get('mode','delay-key')
                        if private_capture and (mode!='hold-cli' or req.get('inspect',False)):
                            raise ValueError('Private capture permits only the audited hold-cli catch')
                        if mode not in ['delay-key','stop-key-probe','delay-key-prequeue','stock-uart-debug','stock-rescue-shell','hold-cli']:raise ValueError('Unknown audited key mode')
                        key=b'\x11' if mode=='stop-key-probe' else b'U-C!FItA\r'
                        if mode=='hold-cli':
                            if req.get('volatile_cli_hold',False) is not True:raise ValueError('Volatile console hold must be explicitly requested')
                            prior=(ROOT/'raw/uart-rx-rx-fixed-a25m86cw.bin').read_bytes()
                            if b'\r\nanyka$' not in prior:raise ValueError('No confirmed corrected-wire CLI')
                            key=b'U-C!FItA\r'+b'\x11'+b'U-C!FItA\r'
                        if mode=='delay-key-prequeue':
                            prior=ROOT/'raw/uart-rx-rx-fixed-a25m86cw.bin'
                            if not prior.exists() or b'\r\nanyka$' not in prior.read_bytes():raise ValueError('No verified prior corrected-wiring CLI; refuse command prequeue')
                            if req.get('inspect',False) is not True:raise ValueError('Read-only inspection must be explicitly requested')
                            key=key*2+b'printenv\rhelp\rversion\r'
                        if mode=='stock-uart-debug':
                            if req.get('volatile_uart_debug',False) is not True:raise ValueError('Volatile stock boot-argument test must be explicitly requested')
                            prior=ROOT/'raw/uart-rx-cli-batch-a25m86cw.bin'
                            if not prior.exists():raise ValueError('No live environment evidence')
                            prior_bytes=prior.read_bytes()
                            for required in [b'anyka$printenv',b'bootcmd=run boot_normal',b'loadaddr=0x81808000',b'kernel_addr=0x80000',b'kernel_size=0x200000',b'stdin=serial']:
                                if required not in prior_bytes:raise ValueError('Stock policy not verified: '+required.decode())
                            plan=json.loads((ROOT/'analysis/stock-uart-debug-experiment-plan.json').read_text())
                            key=bytes.fromhex(plan['planned_packet_hex'])
                            if len(key)>=128:raise ValueError('Packet exceeds verified RX ring bounds')
                        if mode=='stock-rescue-shell':
                            if req.get('volatile_rescue',False) is not True:raise ValueError('Volatile read-only rescue acquisition must be explicitly requested')
                            prior=ROOT/'raw/uart-rx-cli-batch-a25m86cw.bin'
                            evidence=prior.read_bytes()
                            for expected in [b'boot_normal=run read_kernel; bootm ${loadaddr}\r\n',b'loadaddr=0x81808000\r\n',b'kernel_addr=0x80000\r\n',b'kernel_size=0x200000\r\n',b'stdin=serial\r\n']:
                                if expected not in evidence:raise ValueError('Actual stock environment not verified')
                            plan=json.loads((ROOT/'analysis/stock-rescue-experiment-plan.json').read_text())
                            key=bytes.fromhex(plan['packet_hex'])
                            fixed=b'U-C!FItA\r'*2+b'setenv bootargs ${bootargs} uart_debug=1 init=/bin/sh; run boot_normal\r'
                            if key!=fixed or len(key)>=128:raise ValueError('Unexpected rescue packet')
                        bootbuf.clear();arm={'inspect':req.get('inspect',False) is True,'key':key,'mode':mode}
                        event('abort_armed',key_hex=key.hex(),mode=mode,trigger='Early banner/DRAM bounded3-packet prequeue' if mode=='hold-cli' else 'Net line in future received bytes; do not act on existing boot',inspect_read_only=arm['inspect'])
                        response(c,{'armed':True,'mode':mode,'read_only_commands':(['printenv','help','version'] if mode=='delay-key-prequeue' else ['version','help','printenv']) if arm['inspect'] else [],'volatile_bootargs':mode in ['stock-uart-debug','stock-rescue-shell'],'persistent_write':False,'utc':utc(),'ends_by':(datetime.datetime.now(datetime.timezone.utc)+datetime.timedelta(seconds=max(0,duration-(time.monotonic()-start)))).isoformat()})
                    elif action=='observe':
                        if private_capture:raise ValueError('Private capture observation export refused')
                        seconds=float(req.get('seconds',30));assert 0<seconds<=120
                        event('observation_started',seconds=seconds)
                        pending.append({'socket':c,'until':time.monotonic()+seconds,'data':bytearray(),'start_offset':rxbytes})
                    elif action=='send':
                        if private_capture:raise ValueError('Public send refused on private capture')
                        data=bytes.fromhex(req['hex']);reason=req['reason'];assert 0<len(data)<=4096 and isinstance(reason,str) and reason
                        pending_rx=(port.in_waiting or burst or inspecting or time.monotonic()<private_until) if 'expected_rx_bytes' in req else 0
                        if not checked_tx_ready(req,rxbytes,pending_rx):
                            response(c,{'not_sent':True,'rx_bytes':rxbytes,'conditional_send_v1':True});continue
                        n=transmit(data,reason)
                        response(c,{'transmitted_bytes':n,'utc':utc()})
                    elif action=='send_protected':
                        data=bytes.fromhex(req['hex'])
                        if not 0<len(data)<=128:raise ValueError('Protected frame outside checked bound')
                        pending_rx=port.in_waiting or burst or inspecting or pending or time.monotonic()<private_until
                        if not protected_tx_ready(req,rxbytes,pending_rx,private_capture):
                            response(c,{'not_sent':True,'rx_bytes':rxbytes,'conditional_send_v1':True});continue
                        n=transmit(data,'Protected RAM QCL transport')
                        response(c,{'transmitted_bytes':n,'utc':utc()})
                    elif action=='send_private':
                        if private_capture:raise ValueError('Redaction windows are not allowed in full private capture')
                        if pending or burst or inspecting or time.monotonic()<private_until:
                            raise ValueError('Private provisioning requires idle transport and no observers')
                        if req.get('tty_echo_disabled_verified') is not True:
                            raise ValueError('Verify target TTY echo is disabled before private provisioning')
                        data=bytes.fromhex(req['hex'])
                        if not 0<len(data)<=4096:raise ValueError('Private packet length outside bounds')
                        private_until=time.monotonic()+5
                        event('private_provisioning_window',seconds=5,credentials_in_source_or_logs=False)
                        n=transmit(data,'Private provisioning',private=True)
                        response(c,{'transmitted_bytes':n,'utc':utc(),'private_rx_window_seconds':5,'payload_retained':False})
                    elif action=='stop':event('stop_requested');running=False;response(c,{'stopping':True})
                    else:response(c,{'error':'Unknown control action'})
                except Exception as ex:
                    message='Private control request refused' if private_capture else str(ex)
                    event('control_error',error=message);response(c,{'error':message})
            for observer in pending[:]:
                if time.monotonic()>=observer['until']:
                    data=bytes(observer['data']);response(observer['socket'],{'utc':utc(),'start_offset':observer['start_offset'],'new_rx_bytes':len(data),'hex':data.hex(),'text':data.decode('utf-8','backslashreplace')});pending.remove(observer)
        event('capture_end',rx_bytes=rxbytes,tx_bytes=txbytes,private_rx_bytes_intentionally_omitted=private_rxbytes,reason='stop/duration')
    except BaseException as ex:
        event('capture_error',error=str(ex));raise
    finally:
        for observer in pending:
            try:response(observer['socket'],{'error':'Capture ended'})
            except OSError:pass
        if port.is_open:port.close()
        server.close()
        if owned_socket is not None and os.path.lexists(SOCKET):
            socket_stat=SOCKET.lstat()
            if (socket_stat.st_dev,socket_stat.st_ino)==owned_socket:SOCKET.unlink()
        raw.close();log.close()
        if not private_capture:rawpath.chmod(0o444)
        paths=[rawpath,logpath]
        if not private_capture and (ROOT/'raw/user-supplied-bootlog.txt').exists():paths.append(ROOT/'raw/user-supplied-bootlog.txt')
        with (private_open(finalpath,'w') if private_capture else finalpath.open('x')) as f:json.dump({'utc':utc(),'argv':sys.argv,'cwd':str(Path.cwd()),'rx_bytes':rxbytes,'tx_bytes':txbytes,'private_capture_v1':private_capture,
           'files':[{'path':str(p) if private_capture else str(p.relative_to(ROOT)),'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} for p in paths]},f,indent=2)

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--capture',action='store_true');parser.add_argument('--seconds',type=int,default=1800);parser.add_argument('--part',default='');parser.add_argument('--port');parser.add_argument('--request');parser.add_argument('--private-capture-dir');args=parser.parse_args()
    if args.capture:
        try:_checked_selected_port(args.port)
        except ValueError:parser.error('Capture requires an explicit valid by-id port')
        if args.private_capture_dir is not None:
            try:capture(args.seconds,args.part,args.port,args.private_capture_dir)
            except BaseException:
                print('Private capture HOLD; inspect protected evidence',file=sys.stderr);raise SystemExit(1)
        else:capture(args.seconds,args.part,args.port)
    else:
        req=json.loads(args.request)
        if req.get('private_status') is True or req.get('action')=='send_protected':
            raise SystemExit('Protected transport requires its explicit private client')
        result=client(req)
        if result.get('private_capture_v1'):result['recent_rx']=''
        print(json.dumps(result,indent=2))
