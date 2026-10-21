#!/usr/bin/env python3
"""One fixed passive GPIO0 press window over pinned SSH. No outputs/reset actions.

READY.json is a startup barrier, not a completion receipt. Unknown SSH retirement
retains the cooperative operation lease until parent proves retirement/recovery.
"""
from pathlib import Path
import sys
import os
import json
import time
import datetime as dt
import subprocess
import fcntl

ROOT=Path(__file__).resolve().parents[4]
sys.path.insert(0,str(ROOT/'scripts'))
import qc07f_private_root_transfer as private

CAPTURE=Path('/home/gadorach/.local/state/anyka-bench-ssh/reset-button-20261016T0253Z/uart-capture16a')
CONFIG=Path('/home/gadorach/.local/state/anyka-bench-ssh/persistent-root-20261015T223400Z/runtime15/positive.conf')
SHA='026ad53c2a89d9d67dadf44887ba8a29f17601fb6d844d80e13e78dc3665b002'

def utc():return dt.datetime.now(dt.timezone.utc).isoformat()
def save(path,value):
    with path.open('x') as f:json.dump(value,f,indent=2);f.write('\n')
def receipts(path):
    result=[]
    for line in path.read_bytes().splitlines():
        try:result.append(json.loads(line))
        except (ValueError,UnicodeError):pass # An unfinished final line is not admission.
    return result

def main():
    os.umask(0o077)
    parser=private.PrivateParser(description=__doc__,allow_abbrev=False)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();private.private_path(args.output,directory=True,fresh=True)
    args.output.mkdir(mode=0o700);out=args.output
    with (private.boot.CONTROL/'peripheral-gui-operation.lock').open('a') as lease:
        fcntl.flock(lease,fcntl.LOCK_EX|fcntl.LOCK_NB)
        owner=private.PrivateBoot(out/'owner-check',CAPTURE)
        state=owner.owner(margin=300)
        if state['part']!='foundation-20261016a' or not state['abort_armed'] or state['tx_busy']:
            raise RuntimeError('Fresh quiet UART/catch admission failed')
        command=('set -- $(/bin/busybox sha256sum /tmp/reset-button16a/gpio0-observer02); '
                 'test "$1" = '+SHA+' || exit 90; '
                 'printf \'{"record":"reset-owner","pid":%s}\\n\' "$$"; '
                 'exec /tmp/reset-button16a/gpio0-observer02 --sample-ms 60000')
        stdout=out/'stdout.private';stderr=out/'stderr.private'
        with stdout.open('xb') as so,stderr.open('xb') as se:
            p=subprocess.Popen(['ssh','-F',str(CONFIG),'qc07f15',command],stdin=subprocess.DEVNULL,stdout=so,stderr=se)
            save(out/'HOST_CHILD.json',{'utc':utc(),'PID':p.pid,'read_only_GPIO0_only':True,'duration_ms':60000})
            admitted=False;end=time.monotonic()+10
            while p.poll() is None and time.monotonic()<end:
                rows=receipts(stdout)
                ready=[x for x in rows if x.get('state')=='READY']
                beats=[x for x in rows if x.get('state')=='HEARTBEAT']
                if ready and beats:
                    x=ready[0]
                    if (x.get('mux_bit')==0 and x.get('direction_bit')==0 and x.get('raw_level')==1 and
                            x.get('GET_MMIO_agree') and x.get('read_only')):
                        save(out/'READY.json',{'utc':utc(),'host_monotonic':time.monotonic(),
                                              'raw_idle_level':1,'heartbeat_observed':True,
                                              'GPIO_input_GET_MMIO_clock_admitted':True,'window_ms':60000})
                        admitted=True
                    break
                time.sleep(.02)
            if not admitted:save(out/'NOT_ADMITTED.json',{'utc':utc(),'reason':'No qualified READY+heartbeat; do not press'})
            try:rc=p.wait(timeout=80)
            except subprocess.TimeoutExpired:
                save(out/'UNCERTAIN_HOST_CHILD.json',{'utc':utc(),'PID':p.pid,'not_killed':True})
                rc=p.wait() # Preserve owner; parent recovery/retirement required.
        rows=receipts(stdout);footer=rows[-1] if rows else {}
        complete=(rc==0 and admitted and footer.get('state')=='COMPLETED' and
                  footer.get('config_unchanged') and footer.get('GET_MMIO_agree') and footer.get('close_ok'))
        save(out/'RESULT.json',{'utc':utc(),'state':'COMPLETED_READ_ONLY' if complete else 'HOLD_REVIEW_REMOTE_OWNER',
                               'SSH_exit':rc,'admitted':admitted,'footer':footer,
                               'remote_owner_retired_by_success_receipt':bool(complete),
                               'read_only':True,'no_automatic_retry':True})
        print('Passive reset-input collection complete' if complete else 'Reset-input collection HOLD; ownership review required',flush=True)
        if not complete:
            # No unrelated target campaign can take this lease on uncertain SSH exit.
            while not (out/'PARENT_VERIFIED_RETIREMENT').is_file():time.sleep(1)

if __name__=='__main__':
    try:main()
    except (Exception,KeyboardInterrupt):
        print('Reset-input collector HOLD; preserve private evidence',file=sys.stderr)
        raise SystemExit(1)
