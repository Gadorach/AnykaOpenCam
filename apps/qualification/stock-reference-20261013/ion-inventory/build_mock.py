#!/usr/bin/env python3
"""Scoped fresh build and host-only MOCK execution; never execute ARM payload."""
from pathlib import Path
import datetime, hashlib, json, re, struct, subprocess, sys

SRC=Path(__file__).resolve().parent
ROOT=SRC.parents[3]
BASE=ROOT/'evidence/delegated/20261013-stock-reference/ion-inventory'
TC=ROOT/'work/lightbulb-camera/implementation-20261008T205404Z/sources/toolchain-armv5-musl-202508/armv5-eabi--musl--stable-2025.08-1'
PUB=ROOT/'work/lightbulb-camera/deep-bringup-20261008T055753Z/sources/ak3918ev200'
K=PUB/'vendor/linux-3.4.35'
KD=PUB/'vendor/linux-3.4.35-ak3918-diff'

def sha(p):
    h=hashlib.sha256()
    with p.open('rb') as f:
        for b in iter(lambda:f.read(1048576),b''): h.update(b)
    return h.hexdigest()

def main():
    if len(sys.argv)!=2 or not re.fullmatch(r'build[0-9]{2}',sys.argv[1]):
        raise SystemExit('usage: build_mock.py buildNN (fresh output)')
    BASE.mkdir(parents=True,exist_ok=True)
    out=BASE/sys.argv[1];out.mkdir(exist_ok=False);(out/'tmp').mkdir()
    env={'PATH':'/usr/bin:/bin','LC_ALL':'C','TMPDIR':str(out/'tmp'),'SOURCE_DATE_EPOCH':'0','PYTHONDONTWRITEBYTECODE':'1'}
    commands=[];tests=[]
    report={'started_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'state':'running',
            'commands':commands,'tests':tests,'native_executed':False,'target_operations':False,
            'host_device_or_proc_access':False}
    def run(args,label,check=True,case=None,exe=None):
        e=dict(env)
        if case:e['ION_MOCK_CASE']=case
        p=subprocess.run([str(a) for a in args],cwd=out,env=e,executable=exe,capture_output=True,text=True,timeout=30)
        (out/(label+'.stdout.txt')).write_text(p.stdout);(out/(label+'.stderr.txt')).write_text(p.stderr)
        commands.append({'argv':[str(a) for a in args],'label':label,'returncode':p.returncode,'mock_case':case,'executable':exe})
        if check and p.returncode:raise RuntimeError(label+': '+p.stderr)
        return p
    try:
        tools={n:TC/('bin/arm-buildroot-linux-musleabi-'+n) for n in ['gcc','readelf','nm','objdump','size']}
        cc=tools['gcc']
        run([cc,'--version'],'compiler');run([cc,'-v'],'compiler-config')
        run([tools['readelf'],'--version'],'binutils');run(['/usr/bin/cc','--version'],'host-compiler')
        flags=['-std=c11','-Os','-Wall','-Wextra','-Werror','-fno-common','-ffunction-sections','-fdata-sections','-fstack-usage','-fno-ident']
        arm=['-march=armv5te','-marm','-mfloat-abi=soft']
        run([cc,*flags,*arm,'-D__EXPORTED_HEADERS__','-I'+str(K/'include'),
             '-DORIGINAL_ION_HEADER="'+str(KD/'include/linux/ion.h')+'"','-MD','-MF',out/'abi.d',
             '-c',SRC/'abi_check.c','-o',out/'abi.o'],'abi-compile')
        run([tools['readelf'],'-x','.rodata.abi_values',out/'abi.o'],'abi-values')
        run([cc,*flags,*arm,'-MD','-MF',out/'native.d','-c',SRC/'ion.c','-o',out/'native.o'],'native-compile')
        native=out/'stock-ion-inventory-armv5'
        run([cc,*arm,'-static','-no-pie','-Wl,--gc-sections,--build-id=none,-z,noexecstack',
             '-Wl,-Map='+str(out/'native.map'),out/'native.o','-o',native],'native-link')
        run([tools['readelf'],'-h','-l','-A',native],'native-elf')
        run([tools['size'],'-A',native],'native-size')
        imports=run([tools['nm'],'-u',out/'native.o'],'application-imports').stdout
        assert not re.search(r'\b(malloc|calloc|realloc|free|fork|execve|dlopen|socket)\b',imports)
        run([tools['nm'],'-a',native],'native-symbols');run([tools['objdump'],'-d',native],'native-disassembly')
        b=native.read_bytes();assert b[:6]==b'\x7fELF\x01\x01' and struct.unpack_from('<H',b,18)[0]==40
        po=struct.unpack_from('<I',b,28)[0];ps,pn=struct.unpack_from('<HH',b,42)
        ph=[struct.unpack_from('<8I',b,po+ps*i) for i in range(pn)]
        assert all(x[0] not in (2,3) for x in ph)
        assert any(x[0]==0x6474e551 and not x[6]&1 for x in ph)
        rw=sum(x[5] for x in ph if x[0]==1 and x[6]&2)
        frames=[int(x.split('\t')[1]) for x in (out/'native.su').read_text().splitlines()]
        assert sum(frames)<=4096 and rw<=4096,(frames,rw)
        report['application_frame_sum_bytes']=sum(frames);report['writable_load_bytes']=rw
        host=out/'stock-ion-inventory-host-mock'
        run(['/usr/bin/cc',*flags,'-DSTOCK_ION_MOCK',SRC/'ion.c',SRC/'mock.c','-o',host],'host-build')
        h=str(host)
        full=['open_ion','alloc','map','mmap','open_pm','close_pm','munmap','close_dma','free','close_ion']
        def test(case,code=1,events=None,readcount=0,args=None):
            label=f'mock-{len(tests):02d}-{case}'
            p=run(args or [h,'--mock-stock-ion-inventory'],label,check=False,case=case,exe=h)
            assert p.returncode==code,(label,p.returncode,p.stderr)
            ev=re.findall(r'^MOCK (\w+)$',p.stderr,re.M)
            assert ev.count('pread')==readcount,(label,ev.count('pread'),readcount)
            assert [x for x in ev if x!='pread']==(events or []),(label,ev,events)
            d=json.loads(p.stdout)
            assert d['payload_accessed'] is False
            if code==125:
                assert d['state']=='UNCERTAIN_STOP' and not d['known_allocator_cleanup']
                assert d['remaining_ownership']=='retained_or_unknown'
            else:
                assert d['known_allocator_cleanup'] is True and not d['cache_or_capture_proved']
                assert (d['bytes'],d['alignment'],d['mask'],d['flags'])==(8388608,4096,2,3)
                assert d['state']==('PASS' if code==0 else 'HOLD' if code==2 else 'ERROR')
                if code==0:
                    assert d['allocation_ok'] and d['mapping_ok'] and d['pfn_contiguity_proved']
                    assert (d['pages_examined'],d['first_pfn'],d['last_pfn'])==(2048,0x80400,0x80bff)
            tests.append({'case':case,'exit':code,'events':events or [],'pagemap_reads':readcount,'passed':True})
        for c,n in [('success',64),('shortread',65),('eintronce',65)]:test(c,0,full,n)
        for c in ['uid','euid','pid1','kernel','uname','pagesize','sigsetup','sigmask','clockstart','clockinvalid','missing','symlink','regular']:
            test(c)
        test('open',events=['open_ion'])
        for c in ['fstat','race','clockback']:test(c,events=['open_ion','close_ion'])
        for c in ['allocnomem','alloceinval','allocefault']:test(c,events=['open_ion','alloc','close_ion'])
        for c in ['alloceintr','badhandle','errhandle','allocpacket','allocoverrun','allocpositive']:
            test(c,125,['open_ion','alloc','STOP'])
        for c in ['mapnomem','mapeinval']:test(c,events=['open_ion','alloc','map','free','close_ion'])
        for c in ['mapeintr','mapeFAULT','badfd','aliasfd','mappacket','mapoverrun','mappositive','dmachr','dmafstat']:
            test(c,125,['open_ion','alloc','map','STOP'])
        test('mmapfail',events=['open_ion','alloc','map','mmap','close_dma','free','close_ion'])
        for c in ['mapunaligned','mapoverflow']:test(c,125,['open_ion','alloc','map','mmap','STOP'])
        test('pmopen',2,['open_ion','alloc','map','mmap','open_pm','munmap','close_dma','free','close_ion'])
        for c,n in [('readerror',1),('readeintr',2),('readzero',1),('readmisaligned',1),('tinyreads',2),
                    ('noncontig',1),('outofrange',1),('badend',1),('missingpfn',1),('swapped',1),('format',1),('reservedstatus',1)]:
            test(c,2,full,n)
        for c in ['clocklater','deadline']:test(c,1,full,0)
        for c,code,ev,n in [
            ('alarmalloc',124,['open_ion','alloc','free','close_ion'],0),
            ('alarmmap',124,['open_ion','alloc','map','close_dma','free','close_ion'],0),
            ('alarmmmap',124,['open_ion','alloc','map','mmap','munmap','close_dma','free','close_ion'],0),
            ('alarmread',124,full,1),('interrupt',130,full,1),('terminate',143,full,1),('alarmfree',124,full,64)]:
            test(c,code,ev,n)
        for c,ev in [
            ('pmclose',full[:6]+['STOP']),('munmapfail',full[:7]+['STOP']),
            ('dmaclose',full[:8]+['STOP']),('freefail',full[:9]+['STOP']),
            ('freeoverrun',full[:9]+['STOP']),('freepositive',full[:9]+['STOP']),
            ('ionclose',full+['STOP'])]:test(c,125,ev,64)
        for n,args in enumerate([[h],[h,'--native-stock-ion-inventory'],[h,'--mock-stock-ion-inventory','16777216'],
                                 [h,'--mock-stock-ion-inventory','/dev/mem'],['./ion','--mock-stock-ion-inventory'],[h,'--mock-stock-get']]):
            test('entry'+str(n),args=args)
        inputs={p.resolve() for p in SRC.iterdir() if p.is_file()}
        for dep in out.glob('*.d'):
            inputs.update(Path(p).resolve() for p in dep.read_text().replace('\\\n',' ').split(':',1)[1].split())
        inputs.update(p.resolve() for p in tools.values())
        inputs.update([TC/'summary.csv',TC/'buildroot.config',TC/'README.txt',K/'fs/proc/task_mmu.c',K/'fs/anon_inodes.c',
                       KD/'drivers/gpu/ion/ion.c',KD/'drivers/gpu/ion/ion_carveout_heap.c'])
        for p in ['retail-ION-pdata-static-binding01.json','actual-stock-ION-public-pdata-readback01.json']:
            inputs.add(ROOT/'evidence/qualification/qc07f/stock-reference-20261013'/p)
        for name in ['cc1','as','ld']:
            s=run([cc,'-print-prog-name='+name],'tool-'+name).stdout.strip();inputs.add(Path(s).resolve())
        for line in (out/'native.map').read_text().splitlines():
            if line.startswith('LOAD '):
                p=Path(line[5:].strip()).resolve()
                if p.is_file():inputs.add(p)
        report['input_closure']=[{'path':str(p),'bytes':p.stat().st_size,'sha256':sha(p)} for p in sorted(inputs)]
        report['environment']=env;report['native_sha256']=sha(native);report['native_bytes']=native.stat().st_size
        report['state']='host-mocks-pass-ARM-built-not-native-executed'
        (out/'source').mkdir()
        for p in SRC.iterdir():
            if p.is_file():(out/'source'/p.name).write_bytes(p.read_bytes())
    except Exception as e:
        report['state']='failed';report['failure']=repr(e);raise
    finally:
        report['finished_utc']=datetime.datetime.now(datetime.timezone.utc).isoformat()
        (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
        (out/'SHA256SUMS').write_text(''.join(f'{sha(p)}  {p.relative_to(out)}\n' for p in sorted(out.rglob('*')) if p.is_file()))
    print(json.dumps({'state':report['state'],'tests':len(tests),'payload':str(native),'bytes':report['native_bytes'],'sha256':report['native_sha256']}))

if __name__=='__main__':main()
