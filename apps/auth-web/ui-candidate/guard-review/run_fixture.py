#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Finite existing-guard forensic tests: ordinary FD3 file, no actual watchdog/proc."""
from pathlib import Path
import datetime,hashlib,json,os,shutil,subprocess,sys,time
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3]
sys.path.insert(0,str(HERE));from candidate import candidate,diff,original_from_receipt
E=ROOT/'evidence/delegated/20261012-native-ui/guard-review'
if len(sys.argv)!=2 or not sys.argv[1].isalnum():raise SystemExit('fresh simple run name required')
OUT=E/sys.argv[1];OUT.mkdir()
original=original_from_receipt(E/'inputs/native04-observed.json');new=candidate(original)
(OUT/'original-native04-guard.txt').write_text(original+'\n');(OUT/'candidate-unadopted-guard.txt').write_text(new+'\n');(OUT/'candidate.diff').write_text(diff(original,new))
native02=(E/'inputs/native02-public-uart.bin').read_bytes().decode('ascii','backslashreplace')
native02line=next(line.removeprefix('# ') for line in native02.splitlines() if line.startswith('# (exec 3>/dev/watchdog0'))
(OUT/'observed-native02-prior-guard.txt').write_text(native02line+'\n')
results=[];started=time.monotonic();absolute_end=started+90
def run(label,source,mode,delay,runtime,rc):
    d=OUT/label;d.mkdir();bin=d/'bin';bin.mkdir();fd=d/'PUBLICDUMMY-FD3-file';fd.touch()
    for tool in ['ls','cat']:
        p=bin/tool;p.write_text('#!/bin/sh\nPUBLICDUMMY_ROLE=inspector exec '+shutil.which('python3')+' -B '+str(HERE/'fixture_process.py')+' "$@"\n');p.chmod(0o700)
    launcher=d/'PUBLICDUMMY-launcher';launcher.write_text('#!/bin/sh\nPUBLICDUMMY_ROLE=launcher exec '+shutil.which('python3')+' -B '+str(HERE/'fixture_process.py')+'\n');launcher.chmod(0o700)
    transformed=source.replace('/dev/watchdog0',str(fd)).replace('/run/auth-launch-signal04.elf',str(launcher))
    # Background job is waited by this finite HOST shell. No inherited native CLI.
    transformed+='; wait\n'
    script=d/'fixture.sh';script.write_text(transformed)
    env={'PATH':str(bin)+':/usr/bin:/bin','PUBLICDUMMY_DIR':str(d),'PUBLICDUMMY_MODE':mode,
      'PUBLICDUMMY_EXPECTED_EXE':str(launcher),'PUBLICDUMMY_EXEC_DELAY':str(delay),'PUBLICDUMMY_CHILD_RUNTIME':str(runtime),'PUBLICDUMMY_CHILD_EXIT':str(rc)}
    t=time.monotonic();p=subprocess.run(['/bin/sh',str(script)],env=env,capture_output=True,text=True,timeout=min(20,absolute_end-t))
    (d/'stdout.txt').write_text(p.stdout);(d/'stderr.txt').write_text(p.stderr)
    data=fd.read_bytes();events=[json.loads(x) for x in (d/'events.jsonl').read_text().splitlines()]
    row={'label':label,'exit':p.returncode,'elapsed_seconds':time.monotonic()-t,'FD3_bytes_hex':data.hex(),
      'periodic_pings':data.count(b'.\n')-1,'magicclose_V':b'V' in data,'actual_owned_delayed_exec_events':events,
      'scope':'HOST ordinary file + controlled fake proc inspector only; NOT watchdog/kernel hardware'}
    results.append(row);return row
try:
    # Native02's first ls was NOT recorded: this is a controlled counterexample only.
    hazard=run('old-controlled-preexec-break',original,'old-race',.1,5.2,0)
    first=next(x for x in hazard['actual_owned_delayed_exec_events'] if x['event']=='fake_proc_exe')
    exec_event=next(x for x in hazard['actual_owned_delayed_exec_events'] if x['event']=='actual_delayed_exec_completed')
    assert first['stage']=='preexec' and first['time']<exec_event['time']
    assert hazard['periodic_pings']==0 and hazard['elapsed_seconds']>5
    control=run('old-normal-exec-control',original,'old-control',.05,.5,0)
    assert control['periodic_pings']>=1 and control['magicclose_V']
    fixed=run('candidate-delayed-exec-control',new,'candidate-good',.1,5.,0)
    assert fixed['periodic_pings']>=1 and fixed['magicclose_V']
    fast=run('candidate-fast-zero-no-admission',new,'candidate-good',.05,.05,0)
    assert fast['periodic_pings']==0 and not fast['magicclose_V']
    wrong=run('candidate-unexpected-exe',new,'candidate-wrong',.05,5.,0)
    assert wrong['periodic_pings']==0 and not wrong['magicclose_V']
    reuse=run('candidate-birth-reuse',new,'candidate-reuse',.05,5.,0)
    assert reuse['periodic_pings']==0 and not reuse['magicclose_V']
    nonzero=run('candidate-nonzero',new,'candidate-good',.05,.05,20)
    assert nonzero['periodic_pings']==0 and not nonzero['magicclose_V']
    state='HOST-controlled-counterexample-and-candidate-cases-PASS-NOT-native-cause-proof'
except BaseException:
    state='failed';raise
finally:
    pins={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in HERE.iterdir() if p.is_file()}
    (OUT/'manifest.json').write_text(json.dumps({'created':datetime.datetime.now(datetime.timezone.utc).isoformat(),'state':state,
      'command':sys.argv,'python':sys.version,'source_pins':pins,'results':results,'elapsed_seconds':time.monotonic()-started,
      'max_fixture_seconds':90,'native02_cause':'UNPROVED; first native guard inspection not logged',
      'actual_device_root_images_proc_private_or_parent_operations':False},indent=2))
print(json.dumps({'state':state,'cases':len(results),'output':str(OUT)}))
