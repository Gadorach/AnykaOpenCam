#!/usr/bin/env python3
"""Offline only: new sources, host fixtures, freestanding ARM GO artifact/audit."""
from pathlib import Path
import datetime as dt
import hashlib
import json
import re
import struct
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[4]
SRC=Path(__file__).resolve().parent
EVIDENCE=ROOT/'evidence/delegated/20261016-ircut/uboot-probe01'
TC=ROOT/'work/lightbulb-camera/implementation-20261008T205404Z/sources/toolchain-armv5-musl-202508/armv5-eabi--musl--stable-2025.08-1/bin'
BOOT=ROOT/'work/lightbulb-camera/password-update-20261008T152221Z/inputs/uboot-reference.elf'

def now():return dt.datetime.now(dt.timezone.utc).isoformat()
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()

def main():
    if len(sys.argv)!=2 or not re.fullmatch(r'run[0-9]{2}',sys.argv[1]):raise SystemExit('Fresh runNN required')
    out=EVIDENCE/sys.argv[1];out.mkdir(parents=True,exist_ok=False)
    (out/'source').mkdir();(out/'tmp').mkdir()
    report={'start_utc':now(),'hardware_access':False,'native_executed':False,
            'commands':[],'inputs':{},'existing_kernel_unit_unchanged':{}}
    env={'PATH':'/usr/bin:/bin','LC_ALL':'C','TMPDIR':str(out/'tmp'),
         'ASAN_OPTIONS':'detect_leaks=1:abort_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'}
    def run(label,args):
        args=[str(x) for x in args];start=now()
        p=subprocess.run(args,cwd=ROOT,env=env,capture_output=True,timeout=90)
        (out/(label+'.stdout')).write_bytes(p.stdout);(out/(label+'.stderr')).write_bytes(p.stderr)
        report['commands'].append({'label':label,'argv':args,'cwd':str(ROOT),'start_utc':start,'end_utc':now(),
                                  'returncode':p.returncode,'stdout_sha256':hashlib.sha256(p.stdout).hexdigest(),
                                  'stderr_sha256':hashlib.sha256(p.stderr).hexdigest()})
        if p.returncode:raise RuntimeError(label+' failed; attempt retained')
        return p.stdout.decode()
    def pin(p):report['inputs'][str(p.relative_to(ROOT))]=sha(p)
    try:
        for name in ('uboot_probe.c','uboot_cycle.S','uboot_ram.ld','uboot_host_test.c','uboot_build.py'):
            pin(SRC/name);(out/'source'/name).write_bytes((SRC/name).read_bytes())
        for name in ('pulse_sequence.h','kernel_disabled.c','host_test.c'):
            report['existing_kernel_unit_unchanged'][name]=sha(SRC/name)
        old=ROOT/'evidence/delegated/20261016-ircut/pulse-design01/run01/REPORT.json'
        old_report=json.loads(old.read_text())
        for name,digest in report['existing_kernel_unit_unchanged'].items():
            assert old_report['inputs'][str((SRC/name).relative_to(ROOT))]==digest
        pin(old);pin(BOOT)
        assert sha(BOOT)=='5a9bc757d88fed6c2c0e3dba3bb9788c161f3eea3c89adff2939fd9d8d6a8934'
        for name in ('ram.ld','uart_ram_loader.c'):pin(ROOT/'bringup/qc07f/diagnostics'/name)
        pin(ROOT/'evidence/delegated/20261013-stock-reference/ircut-source01/reused/bringup/qc07f/mainline/arch/arm/mach-ak3918ev200/gpio-data.h')
        cc=TC/'arm-buildroot-linux-musleabi-gcc';objdump=TC/'arm-buildroot-linux-musleabi-objdump'
        objcopy=TC/'arm-buildroot-linux-musleabi-objcopy';nm=TC/'arm-buildroot-linux-musleabi-nm'
        run('arm-version',[cc,'--version']);run('host-version',['/usr/bin/cc','--version'])
        run('binutils-version',[objdump,'--version']);pin(cc.resolve());pin(objdump.resolve())
        run('original-elf',['/usr/bin/readelf','-hlW',BOOT])
        # Original ELF has a single segment and no sections. Treat the *file* as
        # binary with the exact file-address displacement (not a guessed VMA).
        b=BOOT.read_bytes();off,vma=struct.unpack_from('<II',b,52+4)
        assert (off,vma)==(0x1000,0x80dffe00)
        for label,start,end in [('original-timer',0x80e00380,0x80e0046c),
                                ('original-puts-printf',0x80e0a324,0x80e0a448),
                                ('original-serial-puts',0x80e145dc,0x80e145f8),
                                ('original-serial-current',0x80e14278,0x80e142b8),
                                ('original-serial-transmit',0x80e146bc,0x80e147d8),
                                ('original-serial-putstring',0x80e14830,0x80e1488c),
                                ('original-division',0x80e1eef4,0x80e1f0c0)]:
            run(label,[objdump,'-D','-b','binary','-m','arm','--adjust-vma='+hex(vma-off),
                       '--start-address='+hex(start),'--stop-address='+hex(end),BOOT])
        # Retain numeric callback table fields only, not adjacent stored strings.
        table={hex(0x80e2a0e0+i):hex(struct.unpack_from('<I',b,0x80e2a0e0+i-(vma-off))[0])
               for i in range(0,32,4)}
        (out/'original-serial-table.json').write_text(json.dumps(table,indent=2)+'\n')
        for label,extra in [('optimized',['-O2']),('sanitized',['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie'])]:
            binary=out/('host-'+label)
            run(label+'-build',['/usr/bin/cc','-std=c11','-Wall','-Wextra','-Werror',*extra,
                                out/'source/uboot_host_test.c','-o',binary])
            result=json.loads(run(label+'-test',[binary]));assert result['cases']>=60 and not result['hardware_access']
            report[label]=result
        flags=['-Os','-march=armv5te','-marm','-mfloat-abi=soft','-ffixed-r9','-ffreestanding',
               '-fno-builtin','-fno-common','-fno-pic','-fno-pie','-fno-stack-protector',
               '-fno-unwind-tables','-fno-asynchronous-unwind-tables','-Wall','-Wextra','-Werror']
        run('arm-c',[cc,*flags,'-fstack-usage','-c',out/'source/uboot_probe.c','-o',out/'probe.o'])
        run('arm-asm',[cc,*flags,'-c',out/'source/uboot_cycle.S','-o',out/'cycle.o'])
        elf=out/'ircut-uboot.elf';binary=out/'ircut-uboot.bin'
        run('arm-link',[cc,*flags,'-nostdlib','-static','-no-pie','-Wl,--build-id=none',
                        '-Wl,-T,'+str(out/'source/uboot_ram.ld'),'-Wl,-Map,'+str(out/'probe.map'),
                        out/'probe.o',out/'cycle.o','-o',elf])
        imports=run('arm-imports',[nm,'-u',elf]);assert not imports.strip()
        symbols=run('arm-symbols',[nm,'-nS',elf]);disasm=run('arm-disassembly',[objdump,'-dr',elf])
        run('arm-elf',['/usr/bin/readelf','-hAlSWd',elf]);run('arm-binary',[objcopy,'-O','binary',elf,binary])
        # The energized interval is within this exact hand-written function.
        body=disasm.split('<uboot_assert_site>:',1)[1].split('<uboot_release_site>:',1)[0]
        assert not re.search(r'\b(bl|blx|svc|swi)\b',body)
        assert 'subs' in body and 'bne' in body
        assert not re.search(r'\b(r9|r4|r5|r6|r7|r8|sl|fp|sp)\b',body)
        assert binary.stat().st_size<=4096
        state_line=next(line for line in symbols.splitlines() if line.endswith(' state'))
        assert ' d ' in state_line
        report['artifact']={'elf':str(elf.relative_to(ROOT)),'elf_sha256':sha(elf),
                            'bin':str(binary.relative_to(ROOT)),'bin_sha256':sha(binary),
                            'bytes':binary.stat().st_size,'load_address':'0x82001000',
                            'end_exclusive':hex(0x82001000+binary.stat().st_size),
                            'within_1872_bytes':binary.stat().st_size<=1872,
                            'parent_staging_range_review_required':binary.stat().st_size>1872}
        report['critical_interval_no_calls']=True
        report['result']='HOST_AND_STATIC_BUILD_PASS_NATIVE_UNEXECUTED'
    except Exception as e:
        report['result']='HOLD_ATTEMPT_RETAINED';report['error']=str(e);raise
    finally:
        report['end_utc']=now()
        report['outputs']={str(p.relative_to(out)):sha(p) for p in sorted(out.rglob('*')) if p.is_file()}
        (out/'REPORT.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({'result':report['result'],'host_cases_each':report['optimized']['cases'],'artifact':report['artifact']},indent=2))

if __name__=='__main__':main()
