#!/usr/bin/env python3
"""Fresh scoped builds; run ONLY public-synthetic host tests, never ARM firmware."""
from pathlib import Path
import base64
import datetime
import hashlib
import json
import re
import struct
import subprocess
import sys
import zlib

SRC=Path(__file__).resolve().parent
ROOT=SRC.parents[3]
BASE=ROOT/'evidence/delegated/20261013-stock-reference/capture-reference'
TC=ROOT/'work/lightbulb-camera/implementation-20261008T205404Z/sources/toolchain-armv5-musl-202508/armv5-eabi--musl--stable-2025.08-1'
K=ROOT/'work/lightbulb-camera/deep-bringup-20261008T055753Z/sources/ak3918ev200/vendor/linux-3.4.35'
MB=ROOT/'apps/auth-web/native-launch/vendor/mbedtls'
RECEIPT=ROOT/'evidence/qualification/qc07f/stock-reference-20261013/parent24-tuning-copyextent-review01.json'

def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()

def validate_output(text,dark=False):
    assert len(text.encode())<=61440
    rows=[json.loads(x) for x in text.splitlines()]
    assert len(rows)==4 and rows[-1]['state']=='RETAINED_STOP' and rows[-1]['reason']==0
    assert rows[-1]['ir_restored'] and not rows[-1]['cleanup']
    for i,r in enumerate(rows[:3]):
        assert r['phase']==i and r['samples']==8 and r['class']=='QUALITATIVE_AE_CONFOUNDED'
        assert r['record']=='stock-ir-thumb-v2' and r['level']==(1 if i==1 else 0)
        assert r['verified_transition_ms']==3000+i*3000 and r['source_clock_error_us']==0
        assert r['sec']*1000000+r['usec']>=r['verified_transition_ms']*1000+500000+r['source_clock_error_us']
        assert r['zero']==(dark and i!=1) and r['flat']==(dark and i!=1) and r['image_confirmed'] is False
        thumb=base64.b64decode(r['data'],validate=True)
        assert len(thumb)==14400 and hashlib.sha256(thumb).hexdigest()==r['sha256']
        seed=0x12345678^r['ordinal'];pixels=bytearray(921600);base=100 if i==1 else 20
        for j in range(len(pixels)):
            seed=(seed*1664525+1013904223)&0xffffffff
            pixels[j]=0 if dark and i!=1 else base+((seed>>24)&31)
        assert zlib.crc32(pixels)==int(r['raw_y_crc32'],16)
        assert sum(pixels)*1000//len(pixels)==r['mean_x1000']
        hist=[0]*16
        for v in pixels:hist[v>>4]+=1
        assert hist==r['hist16']
        expected=bytearray()
        for y in range(90):
            for x in range(160):
                total=sum(pixels[(y*8+k)*1280+x*8:(y*8+k)*1280+x*8+8][j] for k in range(8) for j in range(8))
                expected.append((total+32)//64)
        assert thumb==expected
    return {'bytes':len(text.encode()),'three_base64_sha256_crc_hist_boxmean_oracles':True,
            'verified_level_and_source_window_oracle':True,
            'all_data_public_synthetic':True,'high_phase_mean_greater':rows[1]['mean_x1000']>rows[0]['mean_x1000']}

def main():
    if len(sys.argv)!=2 or not re.fullmatch(r'build[0-9]{2}',sys.argv[1]):raise SystemExit('usage: build.py buildNN (fresh only)')
    BASE.mkdir(parents=True,exist_ok=True);out=BASE/sys.argv[1];out.mkdir(exist_ok=False);(out/'tmp').mkdir()
    env={'PATH':'/usr/bin:/bin','LC_ALL':'C','TMPDIR':str(out/'tmp'),'SOURCE_DATE_EPOCH':'0','PYTHONDONTWRITEBYTECODE':'1'}
    report={'started_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'status':'running','commands':[],
            'native_executed':False,'target_or_host_device_proc_access':False,'native_admission':'NO_NATIVE_LAUNCH',
            'host_images':'PUBLIC_DUMMY_ONLY'}
    report['source_snapshot']=[]
    for p in sorted(SRC.rglob('*')):
        if p.is_file():
            q=out/'source-snapshot'/p.relative_to(SRC);q.parent.mkdir(parents=True,exist_ok=True)
            with q.open('xb') as f:f.write(p.read_bytes())
            report['source_snapshot'].append({'path':str(p),'snapshot':str(q),'sha256':sha(q)})
    def run(args,label,timeout=180,extra=None):
        e=dict(env);e.update(extra or {})
        p=subprocess.run([str(a) for a in args],cwd=out,env=e,capture_output=True,text=True,timeout=timeout)
        (out/(label+'.stdout.txt')).write_text(p.stdout);(out/(label+'.stderr.txt')).write_text(p.stderr)
        report['commands'].append({'argv':[str(a) for a in args],'cwd':str(out),'label':label,'returncode':p.returncode})
        if p.returncode:raise RuntimeError(label+': '+p.stderr[-4000:])
        return p.stdout
    try:
        tools={n:TC/('bin/arm-buildroot-linux-musleabi-'+n) for n in ['gcc','readelf','nm','size','objdump']}
        run([tools['gcc'],'--version'],'arm-compiler');run([tools['gcc'],'-v'],'arm-config')
        run([tools['readelf'],'--version'],'binutils');run(['/usr/bin/cc','--version'],'host-compiler')
        receipt=json.loads(RECEIPT.read_text())
        expected=[(b['block_id'],o['block_offset'],o['copy_bytes'],int(o['request'],16)) for b in receipt['blocks'] for o in b['operations'] if 'request' in o]
        actual=[tuple(int(v,0) for v in row) for row in re.findall(r'\{(\d+),(\d+),(\d+),(0x[0-9a-f]+)\}',(SRC/'capture.c').read_text())]
        assert actual==expected and len(actual)==34
        report['parent34_exact_request_extent_match']=True
        flags=['-std=c11','-D_POSIX_C_SOURCE=200809L','-Os','-Wall','-Wextra','-Werror','-fno-common','-ffunction-sections','-fdata-sections','-fstack-usage','-fno-ident']
        inc=['-I'+str(SRC),'-I'+str(MB/'include'),'-DMBEDTLS_CONFIG_FILE="'+str(SRC/'mbed_config.h')+'"']
        arm=['-march=armv5te','-marm','-mfloat-abi=soft']
        files=[SRC/'capture.c',MB/'library/sha256.c',MB/'library/base64.c',MB/'library/platform_util.c',MB/'library/constant_time.c']
        for mode,opt in [('optimized',[]),('sanitized',['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'])]:
            exe=out/('host-'+mode)
            run(['/usr/bin/cc',*flags,*inc,*opt,'-DCR_HOST',*files,SRC/'host_test.c','-o',exe],'host-'+mode+'-build')
            result=run([exe,'--host-synthetic-only'],'host-'+mode+'-run',timeout=240,
                       extra={'ASAN_OPTIONS':'detect_leaks=1:abort_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'})
            report[mode]=validate_output(result)
            report[mode]['fixture_summary']=(out/('host-'+mode+'-run.stderr.txt')).read_text().splitlines()[-1]
            report[mode]['corrected_counterexample']=next(x for x in (out/('host-'+mode+'-run.stderr.txt')).read_text().splitlines() if x.startswith('CORRECTED_PARENT_COUNTEREXAMPLE'))
            dark=run([exe,'--host-dark-only'],'host-'+mode+'-dark',timeout=240,
                     extra={'ASAN_OPTIONS':'detect_leaks=1:abort_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'})
            report[mode]['dark_zero_ABA']=validate_output(dark,dark=True)
            adapter=out/('adapter-'+mode)
            run(['/usr/bin/cc',*flags,*inc,*opt,'-DCR_HOST','-DCR_ADAPTER_MOCK',*files,
                 SRC/'native_adapter.c',SRC/'adapter_test.c','-o',adapter],'adapter-'+mode+'-build')
            report[mode]['adapter_summary']=run([adapter,'--host-adapter-synthetic-only'],'adapter-'+mode+'-run',timeout=240,
                extra={'ASAN_OPTIONS':'detect_leaks=1:abort_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'}).strip()
        objs=[]
        for i,p in enumerate(files+[SRC/'native_adapter.c',SRC/'native.c']):
            obj=out/f'arm{i}.o';objs.append(obj)
            run([tools['gcc'],*flags,*inc,*arm,'-MD','-MF',out/f'arm{i}.d','-c',p,'-o',obj],f'arm{i}-compile')
        run([tools['gcc'],*flags,*arm,'-D__EXPORTED_HEADERS__','-I'+str(SRC/'abi-time32'),'-I'+str(K/'include'),
             '-DORIGINAL_V4L2_HEADER="'+str(K/'include/linux/videodev2.h')+'"','-MD','-MF',out/'abi.d','-c',SRC/'abi_compare.c','-o',out/'abi.o'],'original-header-abi')
        native=out/'capture-reference-HOLD-armv5'
        run([tools['gcc'],*arm,'-static','-no-pie','-Wl,--gc-sections,--build-id=none,-z,noexecstack,-u,cr_run,-u,cr_adapter_bind',
             '-Wl,-Map='+str(out/'native.map'),*objs,'-o',native],'native-link')
        run([tools['readelf'],'-h','-l','-A',native],'native-elf');run([tools['size'],'-A',native],'native-size')
        syms=run([tools['nm'],'-a',native],'native-symbols')
        assert not re.search(r'\b(munmap|socket|connect|fork|execve|system|fopen|dlopen)\s*$',syms,re.M)
        assert all(re.search(r'\b'+n+r'\s*$',syms,re.M) for n in ['open','ioctl','mmap','cr_adapter_bind'])
        entry_asm=run([tools['objdump'],'-d','--disassemble=main',native],'native-main-disassembly')
        assert not re.search(r'<(?:cr_run|cr_adapter_bind|cr_adapter_call)>',entry_asm)
        assert 'NO_NATIVE_LAUNCH' in native.read_bytes().decode('latin1')
        imports=run([tools['nm'],'-u',out/'arm0.o'],'core-imports')
        assert not re.search(r'\b(malloc|calloc|realloc|free|close|munmap|ioctl|open)\b',imports)
        b=native.read_bytes();assert b[:6]==b'\x7fELF\x01\x01' and struct.unpack_from('<H',b,18)[0]==40
        po=struct.unpack_from('<I',b,28)[0];ps,pn=struct.unpack_from('<HH',b,42)
        ph=[struct.unpack_from('<8I',b,po+ps*i) for i in range(pn)]
        assert all(x[0] not in (2,3) for x in ph)
        load=sum(x[5] for x in ph if x[0]==1);assert load<1024*1024
        assert any(x[0]==0x6474e551 and not x[6]&1 for x in ph)
        frames=[]
        for p in out.glob('arm*.su'):
            frames.extend(int(line.split('\t')[1]) for line in p.read_text().splitlines())
        report['max_function_stack_bytes']=max(frames);assert max(frames)<=4096
        report['native_load_bytes']=load;report['native_bytes']=len(b);report['native_sha256']=sha(native)
        inputs=set(SRC.rglob('*'));inputs={p.resolve() for p in inputs if p.is_file()}
        for dep in out.glob('*.d'):
            for p in dep.read_text().replace('\\\n',' ').split(':',1)[1].split():inputs.add(Path(p).resolve())
        inputs.update([RECEIPT,MB/'LICENSE',TC/'summary.csv',TC/'buildroot.config',*tools.values()])
        inputs.update((ROOT/'apps/auth-web/native-launch/runtime-licenses').glob('*'))
        for n in ['cc1','as','ld']:
            inputs.add(Path(run([tools['gcc'],'-print-prog-name='+n],'tool-'+n).strip()).resolve())
        for line in (out/'native.map').read_text().splitlines():
            if line.startswith('LOAD '):
                p=Path(line[5:].strip());
                if p.is_file():inputs.add(p.resolve())
        sources=[ROOT/'docs/qualification-plans/stock-reference-20261013/feed-entry/retail-mapping-rea01/CONTRACT.md',
                 ROOT/'evidence/qualification/qc07f/stock-reference-20261013/immutable-stock-sensor-tail-boundary-discovery01.json',
                 ROOT/'evidence/delegated/20261013-stock-reference/feed-entry/rea-start04/selection.json']
        sources.extend([
            ROOT/'evidence/qualification/qc07f/stock-reference-20261013/capture-parent-review01/REVIEW.json',
            ROOT/'evidence/qualification/qc07f/stock-reference-20261013/capture-parent-review01/phase-delay-test.c',
            ROOT/'work/lightbulb-camera/driver-re-20261008T110053Z/contracts/pseudocode/akcamera-ko/006-ak_camera_dma_irq.c.txt',
            K/'include/linux/jiffies.h'])
        sources.extend([
            ROOT/'docs/qualification-plans/stock-reference-20261013/feed-entry/retail-reference-rea01/CONTRACT.md',
            ROOT/'docs/qualification-plans/stock-reference-20261013/feed-entry/retail-init-rea01/REPORT.md',
            ROOT/'docs/qualification-plans/stock-reference-20261013/feed-entry/PARENT_NATIVE_ENTRY_PROPOSAL01.md',
            ROOT/'work/lightbulb-camera/driver-re-20261008T110053Z/contracts/pseudocode/kernel-filtered/441-akisp_open.c.txt'])
        sources.append(ROOT/'docs/qualification-plans/stock-reference-20261013/feed-entry/retail-clock-rea01/CONTRACT.md')
        inputs.update(sources)
        report['input_closure']=[{'path':str(p),'bytes':p.stat().st_size,'sha256':sha(p)} for p in sorted(inputs)]
        report['toolchain']={'gcc':'14.3.0','musl':'1.2.5','sysroot_headers':'5.4.296','target_wire':'3.4.35 + explicit time32',
                             'archive_retained_sha256':'8cdb4ad70c6b5a66427fa3315fe3ddde1c19c90232674dec59045e04d2a36cf1'}
        report['status']='host-optimized-and-sanitized-pass-static-ARM-HOLD-built'
    except Exception as e:
        report['status']='failed';report['failure']=repr(e);raise
    finally:
        report['finished_utc']=datetime.datetime.now(datetime.timezone.utc).isoformat()
        (out/'REPORT.json').write_text(json.dumps(report,indent=2)+'\n')
        (out/'SHA256SUMS').write_text(''.join(sha(p)+'  '+p.name+'\n' for p in sorted(out.iterdir()) if p.is_file() and p.name!='SHA256SUMS'))
    print(json.dumps({k:report[k] for k in ['status','native_bytes','native_load_bytes','max_function_stack_bytes','native_sha256','optimized','sanitized']},indent=2))

if __name__=='__main__':main()
