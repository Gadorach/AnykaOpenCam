#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""HOST PUBLICDUMMY only. Real owned delayed exec; fake proc inspector, no /proc IO."""
from pathlib import Path
import json,os,sys,time

def event(directory,name,**values):
    with (directory/'events.jsonl').open('a') as out:out.write(json.dumps({'time':time.monotonic(),'event':name,**values})+'\n')
def state(directory):
    return json.loads((directory/'state.json').read_text())
def write_state(directory,value):
    tmp=directory/'state.tmp';tmp.write_text(json.dumps(value));tmp.replace(directory/'state.json')

def main():
    directory=Path(os.environ['PUBLICDUMMY_DIR']);role=os.environ['PUBLICDUMMY_ROLE']
    mode=os.environ['PUBLICDUMMY_MODE'];expected=os.environ['PUBLICDUMMY_EXPECTED_EXE']
    if role=='launcher':
        # The shell actually forked/exec'd this intermediate process. It retains
        # the SAME PID across the controlled delayed second exec below.
        value={'pid':os.getpid(),'ppid':os.getppid(),'birth':time.monotonic_ns(),'stage':'preexec','alive':True}
        write_state(directory,value);event(directory,'actual_preexec_owned_child',pid=value['pid'],ppid=value['ppid'])
        if mode=='old-race':
            until=time.monotonic()+2
            while not (directory/'release-exec').exists():
                if time.monotonic()>until:raise SystemExit(71)
                time.sleep(.001)
        time.sleep(float(os.environ['PUBLICDUMMY_EXEC_DELAY']))
        os.execv(sys.executable,[sys.executable,'-B',__file__,'child'])
    elif role=='inspector':
        # Every queried PID is the actual single child from this fixture's record.
        until=time.monotonic()+2
        while not (directory/'state.json').exists():
            if time.monotonic()>until:raise SystemExit(72)
            time.sleep(.001)
        value=state(directory);args=sys.argv[1:];query=args[-1]
        pid=int(query.split('/')[2])
        if pid!=value['pid']:raise SystemExit('PUBLICDUMMY refuse non-owned selector')
        kind='exe' if query.endswith('/exe') else 'stat'
        if kind=='exe' and mode=='old-control':
            until=time.monotonic()+2
            while value['stage']=='preexec' and time.monotonic()<until:time.sleep(.001);value=state(directory)
        event(directory,'fake_proc_'+kind,pid=pid,stage=value['stage'],alive=value['alive'])
        if not value['alive']:raise SystemExit(1)
        if kind=='stat':
            birth=value['birth']+(1 if mode=='candidate-reuse' and value['stage']=='exec' else 0)
            fields=['S',str(value['ppid'])]+['0']*17+[str(birth)]+['0']*8
            print(str(pid)+' (PUBLICDUMMY child) '+' '.join(fields))
        else:
            image='/bin/busybox' if value['stage']=='preexec' else expected
            if mode=='candidate-wrong' and value['stage']=='exec':image='/PUBLICDUMMY/unexpected-exe'
            print('lrwxrwxrwx 1 PUBLICDUMMY 0 /proc/'+str(pid)+'/exe -> '+image)
            if mode=='old-race' and value['stage']=='preexec':(directory/'release-exec').write_text('PUBLICDUMMY')
    else:raise SystemExit('PUBLICDUMMY role required')

if __name__=='__main__':
    if len(sys.argv)>1 and sys.argv[1]=='child':
        d=Path(os.environ['PUBLICDUMMY_DIR']);s=state(d);s['stage']='exec';write_state(d,s);event(d,'actual_delayed_exec_completed',pid=os.getpid())
        time.sleep(float(os.environ['PUBLICDUMMY_CHILD_RUNTIME']));s=state(d);s['alive']=False;write_state(d,s);event(d,'actual_owned_child_exit',pid=os.getpid())
        raise SystemExit(int(os.environ['PUBLICDUMMY_CHILD_EXIT']))
    main()
