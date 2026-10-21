#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Synthetic tile/mask source qualification; own fresh outputs, no device/IO."""
import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess

ROOT=Path(__file__).resolve().parents[3];APP=Path(__file__).resolve().parent
LANE=ROOT / 'evidence/delegated/20261012-motion'
NAMES=['motion.h','motion.c','geometry.c','test_motion.c','test_v1_lastpolicy.c','header_link.cpp','size_probe.c','README.md','build.py']
SHARED=['framework/include/camera/backend.h','framework/include/camera/access_policy.h','framework/src/access_policy.c','framework/src/backend.c']
DOCS=['AGENTS.md','docs/plan/camera-foundation/00-INDEX.md','docs/plan/camera-foundation/13-STORAGE-RECORDING-PLAYBACK.md',
      'docs/plan/camera-foundation/02-SHARED-ARCHITECTURE.md','docs/plan/camera-foundation/04-IMAGING.md','docs/plan/camera-foundation/05-CAPTURE-AND-MEMORY.md',
      'framework/include/camera/producer.h']
ARM_PINS={'bin/toolchain-wrapper':'f4db963a226e01244ce6f7b5d1ec6d57474dd811119ff8834429f022835ce9c1',
 'bin/arm-buildroot-linux-musleabi-gcc.br_real':'e24ea223e7b521cce1f5889d63c25417e930cdbb35653b072f7b48d9a4b41729',
 'arm-buildroot-linux-musleabi/sysroot/lib/libc.a':'0982970abc52a36d247326a8e0b5dd487410222ff9728528cc99a4e8bd62e7b5',
 'lib/gcc/arm-buildroot-linux-musleabi/14.3.0/libgcc.a':'f707f3a5a94033cbbde76f76640d56dbc5dc615ab421be4efc2641c14344f19a'}
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def now():return datetime.datetime.now(datetime.timezone.utc).isoformat()

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('run');p.add_argument('--toolchain',type=Path,required=True);args=p.parse_args();args.toolchain=args.toolchain.expanduser().resolve()
 if not args.run or not all(c.isalnum() or c in '-_' for c in args.run):p.error('fresh simple run name required')
 LANE.mkdir(parents=True,exist_ok=True)
 if LANE.resolve()!=LANE:raise RuntimeError('lane symlink refused')
 out=LANE / args.run;out.mkdir()
 for n in ('input','objects','logs','tmp'):(out / n).mkdir()
 env={'PATH':os.environ.get('PATH','/usr/bin:/bin'),'LC_ALL':'C','TMPDIR':str(out / 'tmp'),
      'ASAN_OPTIONS':'detect_leaks=1:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1:print_stacktrace=1'}
 report={'command':['python3',str(Path(__file__).relative_to(ROOT)),*os.sys.argv[1:]],'started_utc':now(),'python':os.sys.version,
         'commands':[],'inputs':{},'state':'RUNNING','version':'v2-final-policy-callback-lifetime','task':'STORE-08','native_frames_precision_recordings_NVR':False,
         'footage_or_SD_IO':False,'target_operations':False,'G5':False,'G8':False}
 def run(label,argv,timeout=120,expected_exit=0):
  argv=list(map(str,argv));log=out / 'logs' / (label+'.txt');r={'argv':argv,'cwd':str(out),'started_utc':now(),'timeout_seconds':timeout,'expected_exit':expected_exit,'log':str(log.relative_to(ROOT))};report['commands'].append(r)
  with log.open('xb') as f:status=subprocess.run(argv,cwd=out,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=timeout).returncode
  r.update(exit_code=status,finished_utc=now(),sha256=sha(log))
  if status!=expected_exit:raise RuntimeError('Failed command preserved: '+str(log))
  return log.read_text()
 def compiler(cc,label):
  report.setdefault('compilers',{})[label]={'version':run(label+'-version',[cc,'--version']),'target':run(label+'-target',[cc,'-dumpmachine']),'sha256':sha(Path(cc).resolve())}
  for item in ('cc1','cc1plus','as','ld','libgcc.a','libasan.so','libubsan.so','libstdc++.so','crt1.o','Scrt1.o','crti.o','crtbeginS.o','crtbeginT.o','crtend.o','crtn.o','libc.a','libc.so'):
   value=run(label+'-closure-'+item.replace('.','_'),[cc,('-print-prog-name=' if item in ('cc1','cc1plus','as','ld') else '-print-file-name=')+item]).strip()
   file=Path(value) if '/' in value else Path(shutil.which(value) or value)
   if file.is_file():report.setdefault('compiler_runtime_closure',{})[str(file.resolve())]=sha(file)
 try:
  sources=[APP / n for n in NAMES]+[ROOT / n for n in SHARED];before={str(s):sha(s) for s in sources}
  preservation=json.loads((LANE / 'v1-snapshot/PRESERVATION.json').read_text())
  for name in ('motion.c','motion.h'):
   pin=preservation['files']['apps/recording/motion-candidate/'+name];original=ROOT / pin['copy']
   if sha(original)!=pin['sha256']:raise RuntimeError('Preserved V1 source changed')
   destination=out / 'input/v1' / name;destination.parent.mkdir(parents=True,exist_ok=True)
   with destination.open('xb') as f:f.write(original.read_bytes())
  for source in sources+[ROOT / n for n in DOCS]:
   relative=source.relative_to(ROOT);dest=out / 'input' / relative;dest.parent.mkdir(parents=True,exist_ok=True)
   with dest.open('xb') as f:f.write(source.read_bytes())
   report['inputs'][str(relative)]={'sha256':sha(dest),'bytes':dest.stat().st_size,'copy':str(dest.relative_to(ROOT)),'class':'source' if source in sources else 'requirements/contract annotation'}
  for name,digest in ARM_PINS.items():
   if sha(args.toolchain / name)!=digest:raise RuntimeError('ARM pin mismatch')
  report['arm_toolchain_pins']=ARM_PINS;report['licenses']='New owned GPL-2.0-only plus original shared GPL notices preserved; no firmware/native source/license claimed'
  app=out / 'input' / APP.relative_to(ROOT);shared=out / 'input/framework';common=['-std=c11','-Wall','-Wextra','-Werror','-Wconversion','-Wshadow','-fstack-usage','-fno-common','-I'+str(app),'-I'+str(shared / 'include')]
  units=[app / 'geometry.c',app / 'motion.c',shared / 'src/access_policy.c',shared / 'src/backend.c',app / 'test_motion.c']
  compiler('/usr/bin/cc','host');compiler('/usr/bin/c++','cxx')
  def build(cc,label,flags,execute):
   folder=out / 'objects' / label;folder.mkdir();objects=[]
   for i,source in enumerate(units):
    obj=folder / (str(i)+'-'+source.stem+'.o');run(label+'-compile-'+str(i),[cc,*common,*flags,'-MD','-MF',obj.with_suffix('.d'),'-c',source,'-o',obj]);objects.append(obj)
   binary=out / (label+'-test');run(label+'-link',[cc,*flags,*objects,'-Wl,-t','-Wl,-Map='+str(folder / 'link.map'),'-o',binary]);report.setdefault('binaries',{})[label]={'sha256':sha(binary),'bytes':binary.stat().st_size,'executed':execute}
   report.setdefault('costs',{})[label]=run(label+'-size',['size',*objects])
   if execute:report.setdefault('tests',{})[label]=run(label+'-test',[binary,*(['--host-cost'] if label=='optimized' else [])],timeout=60)
   else:run('arm-elf',[args.toolchain / 'bin/arm-buildroot-linux-musleabi-readelf','-h','-A','-l',binary])
   return objects
  host=build('/usr/bin/cc','optimized',['-O2'],True)
  v1=[]
  for name,source in (('motion',out / 'input/v1/motion.c'),('test',app / 'test_v1_lastpolicy.c')):
   obj=out / 'objects' / ('v1-'+name+'.o');run('v1-compile-'+name,['/usr/bin/cc','-I'+str(out / 'input/v1'),*common,'-O2','-MD','-MF',obj.with_suffix('.d'),'-c',source,'-o',obj]);v1.append(obj)
  negative=out / 'v1-lastpolicy-regression';run('v1-link',['/usr/bin/cc','-O2',host[0],host[2],host[3],*v1,'-Wl,-t','-o',negative])
  failure=run('v1-regression',[negative],expected_exit=1)
  if 'revoked_at=4 staging0=108 committed_bg0=100 owned=1' not in failure:raise RuntimeError('V1 did not reproduce intended unauthorized cell0 load')
  report['v1_regression']={'state':'EXPECTED HOST failure: actual revoked final-valid policy still permitted cell0 read/staging','log':failure,'binary_sha256':sha(negative)}
  build('/usr/bin/cc','sanitized',['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie'],True)
  cpp=out / 'cxx-test';run('cxx-link',['/usr/bin/c++','-std=c++11','-O2','-Wall','-Wextra','-Werror','-MD','-MF',out / 'objects/cxx.d','-I'+str(app),'-I'+str(shared / 'include'),app / 'header_link.cpp',*host[:-1],'-Wl,-t','-o',cpp]);run('cxx-test',[cpp]);report['CXX']='PASS C++ actual C API link/run'
  armcc=args.toolchain / 'bin/arm-buildroot-linux-musleabi-gcc';compiler(str(armcc),'arm');build(str(armcc),'arm',['-Os','-march=armv5te','-marm','-mfloat-abi=soft','-static'],False);report['ARM']='STATIC LINK PASS, NOT EXECUTED'
  obj=out / 'objects/size-probe.arm.o';run('arm-size-probe',[armcc,*common,'-Os','-march=armv5te','-marm','-mfloat-abi=soft','-c',app / 'size_probe.c','-o',obj]);report['arm_storage']=run('arm-storage-symbols',[args.toolchain / 'bin/arm-buildroot-linux-musleabi-nm','-S','--size-sort',obj])
  if before!={str(s):sha(s) for s in sources}:raise RuntimeError('Source changed during run')
  report['state']='SOURCE/HOST/CXX/ARM static-link PASS; native luma/precision/STORE/G5/G8 OPEN'
 except BaseException as exc:report['state']='FAIL';report['failure']=type(exc).__name__+': '+str(exc);raise
 finally:
  closure={}
  for d in (out / 'objects').rglob('*.d'):
   for token in shlex.split(d.read_text().replace('\\\n',' ').split(':',1)[1]):
    file=Path(token)
    if file.is_file():closure[str(file.resolve())]=sha(file)
  report['source_header_closure']=closure;report['stack_usage']={str(s.relative_to(ROOT)):s.read_text() for s in (out / 'objects').rglob('*.su')}
  runtime={}
  for log in (out / 'logs').glob('*-link.txt'):
   for line in log.read_text().splitlines():
    file=Path(line)
    if file.is_absolute() and file.is_file():runtime[str(file.resolve())]=sha(file)
  report['link_runtime_closure']=runtime;report['finished_utc']=now()
  with (out / 'REPORT.json').open('x') as f:json.dump(report,f,indent=2);f.write('\n')
  print('evidence='+str((out / 'REPORT.json').relative_to(ROOT)))

if __name__=='__main__':main()
