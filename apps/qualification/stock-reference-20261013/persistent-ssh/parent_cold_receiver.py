#!/usr/bin/env python3
"""One parent-coordinated default cold-start observation: private RX, NO catch arm.

Camera must be OFF. The intentional lack of an arm lets saved stock boot policy
exercise the installed SSH-only ROOT. On failure parent revalidates/arms a fresh
hold before a coordinated recovery power-cycle. No target TX or flash action here.
"""
from pathlib import Path
import sys
import os
import json
import stat
import subprocess
import fcntl
import time
import hashlib
import datetime as dt

ROOT=Path(__file__).resolve().parents[4]
sys.path.insert(0,str(ROOT/'scripts'))
import qc07f_private_root_transfer as private
boot=private.boot


def main():
    os.umask(0o077)
    parser=private.PrivateParser(description=__doc__,allow_abbrev=False)
    parser.add_argument('--capture-dir',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--part',required=True)
    parser.add_argument('--confirm-power-off',action='store_true')
    args=parser.parse_args()
    if not args.confirm_power_off:raise boot.Refused('Actual OFF confirmation required')
    boot.receipt_path(args.part)
    receipt=boot.EVIDENCE/f'cold{args.part[-3:]}-receive-only-owner01.json'
    private.private_path(args.capture_dir,directory=True,fresh=True)
    private.private_path(args.output,directory=True,fresh=True)
    control=boot.CONTROL.lstat()
    if not stat.S_ISDIR(control.st_mode) or control.st_uid!=os.getuid() or stat.S_IMODE(control.st_mode)!=0o700:
        raise boot.Refused('Unsafe control directory')
    with (boot.CONTROL/'peripheral-gui-operation.lock').open('a') as lease:
        fcntl.flock(lease,fcntl.LOCK_EX|fcntl.LOCK_NB)
        if os.path.lexists(receipt) or os.path.lexists(boot.receipt_path(args.part)) or os.path.lexists(boot.CONTROL/'uart.sock'):
            raise boot.Refused('Existing receipt/socket; no takeover')
        q=subprocess.run(['fuser',str(Path(boot.ADAPTER).resolve())],capture_output=True,text=True,timeout=3)
        if not Path(boot.ADAPTER).exists() or q.returncode!=1 or q.stdout.strip():
            raise boot.Refused('Adapter absent or already owned')
        source=boot.IMPL/'uart_capture.py'
        if hashlib.sha256(source.read_bytes()).hexdigest()!=boot.RECORDER_SHA:
            raise boot.Refused('Recorder source pin changed')
        args.output.mkdir(mode=0o700)
        with (args.output/'receiver-console.log').open('xb') as console:
            child=subprocess.Popen([sys.executable,'-B',str(source),'--capture','--part',args.part,
                                    '--port',boot.ADAPTER,'--seconds','10800',
                                    '--private-capture-dir',str(args.capture_dir)],
                                   stdin=subprocess.DEVNULL,stdout=console,stderr=console,start_new_session=True)
        record={'utc':dt.datetime.now(dt.timezone.utc).isoformat(),'purpose':'ONE_DEFAULT_COLD_START_RX_ONLY',
                'part':args.part,'PID':child.pid,'start_ticks':boot.ticks(child.pid),
                'source_sha256':boot.RECORDER_SHA,'private_capture_dir':str(args.capture_dir),
                'future_catch_armed':False,'target_TX_permitted':False}
        (args.output/'CHILD_IDENTITY.json').open('x').write(json.dumps(record,indent=2)+'\n')
        end=time.monotonic()+5
        while not (boot.CONTROL/'uart.sock').exists():
            if child.poll() is not None or time.monotonic()>=end:
                raise boot.Refused('Fresh receive-only capture did not become ready')
            time.sleep(.025)
        state,peer=boot.rpc({'action':'status'})
        q=subprocess.run(['fuser',str(Path(boot.ADAPTER).resolve())],capture_output=True,text=True,timeout=3)
        if (peer[0]!=child.pid or peer[1]!=os.getuid() or state.get('part')!=args.part or
                state.get('adapter')!=boot.ADAPTER or state.get('capture_source_sha256')!=boot.RECORDER_SHA or
                state.get('private_capture_v1') is not True or state.get('private_capture_dir')!=str(args.capture_dir) or
                state.get('recent_rx')!='' or state.get('abort_armed') or state.get('tx_busy') or
                state['rx_bytes']!=0 or state['tx_bytes']!=0 or q.returncode or q.stdout.split()!=[str(child.pid)]):
            raise boot.Refused('Receive-only owner admission failed; keep OFF')
        private.private_path(Path(state['raw_path']))
        remaining=boot.process_remaining(child.pid,args.part,private_capture_dir=args.capture_dir)
        if remaining<900:raise boot.Refused('Insufficient receiver lifetime')
        record['receive_ends_by']=(dt.datetime.now(dt.timezone.utc)+dt.timedelta(seconds=remaining)).isoformat()
        record['sole_owner_and_private_status_verified']=True
        with receipt.open('x') as f:json.dump(record,f,indent=2);f.write('\n')
    print('Private receive-only cold-start observer verified; catch intentionally NOT armed')


if __name__=='__main__':
    try:main()
    except (Exception,KeyboardInterrupt):
        print('Cold-start receiver HOLD; keep camera OFF and inspect protected evidence',file=sys.stderr)
        raise SystemExit(1)
