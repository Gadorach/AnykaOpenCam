#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Fresh offline helper build + pinned standalone script, no target/current recipe imports."""
from pathlib import Path
import argparse,datetime,difflib,hashlib,json,os,shlex,shutil,subprocess,sys
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3]
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
p=argparse.ArgumentParser();p.add_argument('name');p.add_argument('--toolchain',type=Path,required=True);a=p.parse_args()
if not a.name.isalnum():raise ValueError('fresh simple evidence name')
E=ROOT/'evidence/delegated/20261012-native-ui/guard-qualification';E.mkdir(exist_ok=True);R=E/a.name;R.mkdir()
for name in ['inputs','build','logs','tmp','licenses']:(R/name).mkdir()
env={'PATH':'/usr/bin:/bin','LC_ALL':'C','SOURCE_DATE_EPOCH':'1791694800','TMPDIR':str(R/'tmp')}
m={'started':datetime.datetime.now(datetime.timezone.utc).isoformat(),'commands':[],'state':'building','target_operations':False}
def run(argv,label,timeout=30):
 record={'argv':list(map(str,argv)),'cwd':str(R),'log':str(R/'logs'/(label+'.txt')),'timeout':timeout};m['commands'].append(record)
 with Path(record['log']).open('xb') as f:q=subprocess.run(record['argv'],cwd=R,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=timeout)
 record['exit']=q.returncode
 if q.returncode:raise RuntimeError('build command failed '+label)
try:
 pins=json.loads((ROOT/'apps/auth-web/dependency-pins.json').read_text());tc=a.toolchain.resolve()
 for name,pin in pins['arm_toolchain_files'].items():
  if sha(tc/name)!=pin:raise ValueError('toolchain pin mismatch '+name)
 m['toolchain_pins']={name:sha(tc/name) for name in pins['arm_toolchain_files']}
 prefix=str(tc/'bin/arm-buildroot-linux-musleabi-')
 for name in ['helper.c','clock_fault_host.c']:shutil.copyfile(HERE/name,R/'inputs'/name)
 review=ROOT/'evidence/delegated/20261012-native-ui/guard-review';reviewed=json.loads((review/'final-source.json').read_text())
 source=ROOT/'apps/auth-web/ui-candidate/guard-review/candidate.py'
 if sha(source)!=reviewed['source']['candidate.py']['sha256']:raise ValueError('reviewed guard builder changed')
 observed=review/'inputs/native04-observed.json';old=json.loads(observed.read_text())['text_public_scalars'].split('\r\n')[0]
 g={'__name__':'__reviewed_source__'};exec(compile(source.read_text(),str(source),'exec'),g);guard=g['candidate'](old)
 if hashlib.sha256((guard+'\n').encode()).hexdigest()!=reviewed['candidate_guard_sha256']:raise ValueError('reviewed exact guard differs')
 for name,path in [('native04-observed.json',observed),('reviewed-candidate.py',source),('reviewed-guard-seal.json',review/'final-source.json'),('dependency-pins.json',ROOT/'apps/auth-web/dependency-pins.json')]:shutil.copyfile(path,R/'inputs'/name)
 # Fixed helper path/empty args/prefix only; guard body/time/counter/gates untouched.
 old_exec='exec /run/auth-launch-signal04.elf /run/auth-web-signal04 /run/auth-web-signal04.elf /run/auth-child-signal04.elf'
 if guard.count(old_exec)!=1:raise ValueError('observed exec args boundary')
 script=guard.replace(old_exec,'exec /run/guardqual-native01.elf').replace('/run/auth-launch-signal04.elf','/run/guardqual-native01.elf')
 for oldmarker,newmarker in [('AUTHSIGNAL_BOOT04_PID','GUARDQUAL01_BOOT_PID'),('AUTHSIGNAL_PARENT04_PID','GUARDQUAL01_PARENT_PID'),('AUTHSIGNAL_BOOT04_OWN_EXIT','GUARDQUAL01_BOOT_OWN_EXIT'),('AUTHSIGNAL04_NONZERO_NO_RETRY_QUALIFIED_RECOVERY','GUARDQUAL01_NONZERO_NO_RETRY_QUALIFIED_RECOVERY')]:script=script.replace(oldmarker,newmarker)
 (R/'guardqual-native01-public-command.txt').write_text(script+'\n')
 (R/'guardqual-native01.sh').write_text('#!/bin/sh\n'+script+'\n')
 (R/'reviewed-to-qualification.diff').write_text(''.join(difflib.unified_diff([guard+'\n'],[script+'\n'],fromfile='reviewed-guard',tofile='fixed-guardqual01')))
 common=['-std=gnu11','-Wall','-Wextra','-Werror','-Wpedantic','-fstack-protector-strong','-D_FORTIFY_SOURCE=2','-fno-strict-aliasing','-ffunction-sections','-fdata-sections']
 for label,cc,flags in [('host','cc',['-O2']),('sanitized','cc',['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer']),('arm',prefix+'gcc',['-Os','-march=armv5te','-marm','-mfloat-abi=soft','-static'])]:
  run([cc,'--version'],label+'-compiler');run([cc,'-dumpmachine'],label+'-machine')
  run([cc,*common,*flags,'-MD','-MF',R/'build'/(label+'.d'),'-fstack-usage',R/'inputs/helper.c','-Wl,--gc-sections','-Wl,-z,noexecstack','-Wl,--trace','-Wl,-Map='+str(R/'build'/(label+'.map')),'-o',R/'build'/('guardqual-'+label)],label+'-link')
  tools=prefix if label=='arm' else ''
  run([tools+'readelf','-h','-A','-l','-d',R/'build'/('guardqual-'+label)],label+'-ELF');run([tools+'size',R/'build'/('guardqual-'+label)],label+'-size')
  if label=='arm':run([prefix+'strip','-o',R/'build/guardqual-arm.stripped',R/'build/guardqual-arm'],'arm-strip')
  for item in ['libc.a','libgcc.a','crt1.o','crti.o','crtbeginT.o','crtend.o','crtn.o']:
   run([cc,'-print-file-name='+item],label+'-runtime-'+item.replace('.','_'))
   value=(R/'logs'/(label+'-runtime-'+item.replace('.','_')+'.txt')).read_text().strip()
   if Path(value).is_file():m.setdefault('runtime_pin_inputs',{})[value]=sha(value)
 for kind in [1,2]:
  run(['cc',*common,'-O2','-DHOST_CLOCK_FAULT='+str(kind),R/'inputs/helper.c',R/'inputs/clock_fault_host.c','-Wl,--wrap=clock_gettime','-Wl,-z,noexecstack','-o',R/'build'/('guardqual-host-clock'+str(kind))],'HOST-clock-fault'+str(kind))
 for name in ['GCC-14.3.0-COPYING.RUNTIME','GCC-14.3.0-COPYING3','MUSL-1.2.5-COPYRIGHT']:shutil.copyfile(ROOT/'apps/auth-web/service/runtime-licenses'/name,R/'licenses'/name)
 headers={}
 for dep in (R/'build').glob('*.d'):
  for name in shlex.split(dep.read_text().replace('\\\n','').split(':',1)[1]):
   if Path(name).is_file():headers[name]=sha(name)
 m['header_source_closure']=headers;m['source_pins']={p.name:sha(p) for p in HERE.iterdir() if p.is_file()}
 m['artifacts']={str(p.relative_to(R)):{'bytes':p.stat().st_size,'sha256':sha(p)} for p in (R/'build').iterdir() if p.is_file() and p.name.startswith('guardqual-')}
 m['script_sha256']=sha(R/'guardqual-native01.sh');m['state']='HOST-optimized-sanitized-and-static-ARM-built-NOT-native'
except BaseException:
 m['state']='failed';raise
finally:
 m['ended']=datetime.datetime.now(datetime.timezone.utc).isoformat();(R/'build-manifest.json').write_text(json.dumps(m,indent=2))
print(json.dumps({'state':m['state'],'run':str(R),'ARM':m['artifacts']['build/guardqual-arm.stripped']}))
