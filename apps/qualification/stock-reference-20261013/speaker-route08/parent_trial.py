#!/usr/bin/env python3
"""One parent-admitted enable-low trial. No automatic repeats, gain sweep or UI.

User must have physically isolated BOTH UART signals. Uses the existing stock60s
outer-watchdog pattern. Unknown output/exit retains the operation lease and child;
only parent-confirmed retirement/recovery can release that HOLD.
"""
from pathlib import Path
import datetime as dt
import fcntl
import json
import os
import subprocess
import sys
import uuid

ROOT=Path(__file__).resolve().parents[4]
BASE=Path('/home/gadorach/.local/state/anyka-bench-ssh/speaker-route08-20261016T0500Z')
CONFIG=Path('/home/gadorach/.local/state/anyka-bench-ssh/persistent-root-20261015T223400Z/runtime15/positive.conf')
BINARY='/tmp/qc-speaker-route08-armv5'
SHA='078e1a6adabcc507af1dfe2f3a9790a4e1ff3962faec4d916ea0c9974113cf2e'

def save(path,value):
    with path.open('x') as f:json.dump(value,f,indent=2);f.write('\n')
def utc():return dt.datetime.now(dt.timezone.utc).isoformat()

def main():
    os.umask(0o077)
    if sys.argv[1:]==['--both-uart-signals-physically-isolated']:
        attempt='trial01';duration=224
    elif sys.argv[1:]==['--both-uart-signals-physically-isolated','--user-requested-confirmation-repeat']:
        attempt='trial02';duration=224
    elif sys.argv[1:]==['--both-uart-signals-physically-isolated','--user-approved-five-second-refill-test']:
        attempt='trial03-five-seconds';duration=5000
    else:
        raise RuntimeError('Explicit physical admission required')
    out=BASE/attempt;out.mkdir(mode=0o700,exist_ok=False)
    boot=(BASE/'BOOT_ID.private').read_text().strip()
    if len(boot)!=36 or any(c not in '0123456789abcdef-' for c in boot):raise RuntimeError('Invalid private boot pin')
    tag=uuid.uuid4().hex
    command=(f'trap \'\' 1 2 13 15; '
             f'test "$(cat /proc/sys/kernel/random/boot_id)" = {boot} || exit 90; '
             f'test "$(/bin/busybox sha256sum {BINARY} | /bin/busybox cut -d \' \' -f 1)" = {SHA} || exit 90; '
             '/bin/busybox fuser /dev/watchdog /dev/akpcm_cdev0 /dev/akpcm_cdev1 /dev/mem /dev/akgpio >/dev/null 2>&1; '
             'test "$?" = 1 || exit 90; /bin/busybox sleep 3; '
             'exec 3>/dev/watchdog || exit 90; echo SPK08_GUARD_PID:$$; '
             f'(exec 3>&-; {BINARY} --tone 1000 1 25 {duration} --uart-isolated); x=$?; '
             'echo SPK08_HELPER_EXIT:$x; case "$x" in 0|1|2) echo . >&3; exec 3>&-;; '
             '*) echo SPK08_UNKNOWN_RETAINED; kill -STOP $$; exec /bin/busybox sleep 3600;; esac; '
             f'test "$(cat /proc/sys/kernel/random/boot_id)" = {boot} || exit 91; '
             f'echo SPK08_EXIT:{tag}:$x; exit "$x"')
    save(out/'INTENT.private.json',{'utc':utc(),'command':command,'public_code_sha256':SHA,
         'one_trial':True,'attempt':attempt,'repeat_requested_by_user':attempt=='trial02',
         'physical_UART_isolation_user_confirmed':True,'nominal_ms':duration,
         'five_second_refill_explicitly_approved':duration==5000,
         'frequency_hz':1000,'raw_gain':1,'volume_percent':25,'digital_peak':8192,
         'outer_WD_seconds':60,'no_UART_recovery':'physical leads disconnected',
         'unknown':'retain; manual OFF/leaveOFF if control lost; never kill/free/repair active PCM'})
    lock=ROOT/'work/qc-uart-live/peripheral-gui-operation.lock'
    with lock.open('a+') as lease:
        fcntl.flock(lease,fcntl.LOCK_EX|fcntl.LOCK_NB);lease.seek(0)
        if lease.read(1):raise RuntimeError('Existing operation HOLD')
        lease.write(json.dumps({'state':'HOLD','request':tag,'action':f'speaker08_one_{duration}ms',
                                'private_intent':str(out/'INTENT.private.json')})+'\n');lease.flush();os.fsync(lease.fileno())
        with (out/'stdout.private').open('xb') as so,(out/'stderr.private').open('xb') as se:
            p=subprocess.Popen(['ssh','-F',str(CONFIG),'qc07f15',command],stdin=subprocess.DEVNULL,stdout=so,stderr=se)
            save(out/'HOST_CHILD.private.json',{'utc':utc(),'PID':p.pid,'no_automatic_kill':True})
            try:rc=p.wait(timeout=85)
            except subprocess.TimeoutExpired:
                save(out/'UNCERTAIN_CLIENT.private.json',{'utc':utc(),'PID':p.pid,'not_killed':True})
                rc=p.wait() # Preserve watchdog/remote owner; no timeout kill.
        lines=(out/'stdout.private').read_text().splitlines()
        rows=[json.loads(x) for x in lines if x.startswith('{')]
        valid=(rc==0 and lines[-1:]==[f'SPK08_EXIT:{tag}:0'] and len(rows)==1)
        row=rows[0] if len(rows)==1 else {}
        required=('speaker_enable_intent','speaker_enable_return_zero','speaker_enabled_low',
                  'speaker_low_after_RESUME','speaker_low_after_PAUSE','speaker_low_after_close',
                  'OUT_restore_after_close','mux_restore_after_OUT_restore','pause_completed',
                  'close_ok','gain_restored','mux_restoration_verified','latch_restoration_verified')
        valid=bool(valid and row.get('record')=='stock-speaker-route-v8' and
                   row.get('state')=='TONE_COMPLETED_AWAIT_USER' and row.get('phase')==10 and
                   all(row.get(k) is True for k in required) and
                   row.get('speaker_enable_attempts')==1 and row.get('speaker_enable_word')==1 and
                   row.get('speaker_enable_bytes')==4 and row.get('stream_error')==0 and
                   row.get('frequency_hz')==1000 and row.get('gain_test')==1 and
                   row.get('volume_percent')==25 and row.get('duration_ms')==duration and
                   row.get('frames_generated')==(row.get('calculated_DAC_rate',0)*duration+500)//1000 and
                   row.get('HP_source_during_valid') is True and row.get('HP_source_during')==4 and
                   row.get('gain_before')==row.get('gain_after')==1)
        save(out/'RESULT.private.json',{'utc':utc(),'SSH_exit':rc,'verified_normal_completion':valid,
             'receipt':row,'physical_heard':'AWAIT_USER','no_automatic_retry':True})
        if valid:
            lease.seek(0);lease.truncate();lease.flush();os.fsync(lease.fileno())
            print('One speaker trial completed and restored; audibility awaits user',flush=True)
        else:
            print('Speaker trial HOLD; retain owner and inspect private evidence',flush=True)
            while not (out/'PARENT_VERIFIED_RETIREMENT').is_file():
                import time
                time.sleep(1)

if __name__=='__main__':
    try:main()
    except (Exception,KeyboardInterrupt):
        print('Speaker trial HOLD; no retry or active cleanup',file=sys.stderr)
        raise SystemExit(1)
