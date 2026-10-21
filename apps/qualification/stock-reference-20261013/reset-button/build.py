#!/usr/bin/env python3
"""Source/host fixtures and static ARM build only; never accesses a target."""
from pathlib import Path
import datetime as dt
import hashlib
import json
import os
import re
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[4]
SOURCE=Path(__file__).resolve().parent
TC=ROOT/'work/lightbulb-camera/implementation-20261008T205404Z/sources/toolchain-armv5-musl-202508/armv5-eabi--musl--stable-2025.08-1/bin'

def main():
    if len(sys.argv)!=2 or not re.fullmatch(r'build[0-9]{2}',sys.argv[1]):raise SystemExit('Fresh buildNN required')
    out=ROOT/'evidence/qualification/qc07f/stock-reference-20261013/reset-button01'/sys.argv[1]
    out.mkdir(mode=0o700,exist_ok=False);(out/'tmp').mkdir();(out/'source').mkdir()
    names=('gpio0_observer.c','host_test.c','native_mock.c','build.py')
    for name in names:(out/'source'/name).write_bytes((SOURCE/name).read_bytes())
    env={'PATH':'/usr/bin:/bin','LC_ALL':'C','TMPDIR':str(out/'tmp'),
         'ASAN_OPTIONS':'detect_leaks=1:abort_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'}
    report={'start_utc':dt.datetime.now(dt.timezone.utc).isoformat(),'commands':[],
            'hardware_access':False,'native_executed':False,'source_sha256':{name:hashlib.sha256((out/'source'/name).read_bytes()).hexdigest() for name in names}}
    def run(name,args):
        p=subprocess.run([str(x) for x in args],cwd=out,env=env,capture_output=True,timeout=90)
        (out/(name+'.stdout')).write_bytes(p.stdout);(out/(name+'.stderr')).write_bytes(p.stderr)
        report['commands'].append({'label':name,'argv':[str(x) for x in args],'returncode':p.returncode})
        if p.returncode:raise RuntimeError(name+' failed; attempt preserved')
        return p.stdout
    try:
        flags=['-std=c11','-Wall','-Wextra','-Werror','-fno-common']
        run('host-version',['/usr/bin/cc','--version']);run('arm-version',[TC/'arm-buildroot-linux-musleabi-gcc','--version'])
        for name,extra in [('optimized',['-O2']),('sanitized',['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie'])]:
            run(name+'-compile',['/usr/bin/cc',*flags,*extra,out/'source/host_test.c','-o',out/('host-'+name)])
            result=json.loads(run(name+'-test',[out/('host-'+name)]));assert result['host_cases']>=60 and not result['hardware_access'];report[name]=result
            mock=out/('native-mock-'+name)
            run(name+'-mock-compile',['/usr/bin/cc',*flags,*extra,out/'source/native_mock.c','-o',mock])
            scenarios=[]
            for scenario in range(10):
                text=run(name+'-mock-'+str(scenario),[mock,str(scenario)]).decode()
                rows=[json.loads(line) for line in text.splitlines() if line.strip()]
                if scenario==0:
                    assert rows[-1]['state']=='COMPLETED' and rows[-1]['edges']==2 and rows[-1]['close_ok'] and rows[-1]['max_gap_ns']==2000000
                elif scenario==3:
                    assert len(rows)==1 and rows[0]['state']=='STATUS' and rows[0]['raw_level']==0 and rows[0]['GET_MMIO_agree'] and rows[0]['close_ok']
                elif scenario==9:
                    assert rows[-1]['state']=='COMPLETED' and rows[-1]['samples']==1 and rows[-1]['max_gap_ns']==1000000000
                else:
                    assert not any(row['state'] in ('COMPLETED','STATUS') for row in rows)
                    if scenario in (4,6,7):assert not any(row['state']=='READY' for row in rows)
                scenarios.append({'scenario':scenario,'receipt_checks_passed':True})
            report[name+'_full_native_control_flow_mock']=scenarios
        cc=TC/'arm-buildroot-linux-musleabi-gcc';arch=['-march=armv5te','-marm','-mfloat-abi=soft']
        run('arm-compile',[cc,*flags,*arch,'-Os','-fstack-usage','-MD','-MF',out/'native.d','-c',out/'source/gpio0_observer.c','-o',out/'native.o'])
        imports=run('arm-imports',[TC/'arm-buildroot-linux-musleabi-nm','-u',out/'native.o']).decode()
        assert not re.search(r'\b(system|execve|fork|malloc|calloc|realloc|pwrite|mprotect|socket|connect)\b',imports)
        run('arm-assembly',[TC/'arm-buildroot-linux-musleabi-objdump','-dr',out/'native.o'])
        artifact=out/'qc-reset-gpio0-armv5'
        run('arm-link',[cc,*arch,'-static','-no-pie','-Wl,--build-id=none,-z,noexecstack','-Wl,-Map='+str(out/'native.map'),out/'native.o','-o',artifact])
        elf=run('arm-elf',[TC/'arm-buildroot-linux-musleabi-readelf','-hAlWd',artifact]).decode()
        assert 'INTERP' not in elf and 'DYNAMIC' not in elf and '(NEEDED)' not in elf
        c=(out/'source/gpio0_observer.c').read_text();assert 'PROT_WRITE' not in c and c.count('ioctl(')==1 and 'GPIO_GET 0xc001662bUL' in c
        report.update({'result':'HOST_PASS_STATIC_ARM_UNEXECUTED','artifact':{'path':str(artifact),'bytes':artifact.stat().st_size,'sha256':hashlib.sha256(artifact.read_bytes()).hexdigest()},'compiler_sha256':hashlib.sha256(cc.resolve().read_bytes()).hexdigest()})
    except Exception as e:report['result']='HOLD_ATTEMPT_PRESERVED';report['error']=str(e);raise
    finally:
        report['end_utc']=dt.datetime.now(dt.timezone.utc).isoformat();(out/'REPORT.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({'result':report['result'],'host_cases':report['optimized']['host_cases'],'artifact':report['artifact'],'native_executed':False},indent=2))

if __name__=='__main__':main()
