#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Pinned copied-server HOST qualification and optional ARM static link (NEVER run)."""
from pathlib import Path
import argparse
import datetime
import difflib
import hashlib
import importlib.util
import json
import shlex
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[3]
APP = Path(__file__).resolve().parent
CONFIG = APP.parent
VENDOR = ROOT / 'apps/auth-web/service/vendor/mbedtls'
spec = importlib.util.spec_from_file_location('readonly_crypto_build', CONFIG / 'build.py')
BASE = importlib.util.module_from_spec(spec);spec.loader.exec_module(BASE)
SOURCES = [APP / name for name in ('rtsp_server_candidate.c','candidate_auth.c')] + [ROOT / name for name in (
 'framework/src/rtsp_digest.c','framework/src/rtsp_digest_wire.c','framework/src/rtsp_request.c',
 'framework/src/rtsp_input.c','framework/src/access_policy.c','framework/src/backend.c',
 'framework/src/packet_hub.c','framework/src/producer.c','framework/src/rtp_h264.c')] + [
 VENDOR/'library'/name for name in ('sha256.c','md5.c','constant_time.c','platform_util.c')]


def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def now():return datetime.datetime.now(datetime.timezone.utc).isoformat()


def main():
 p=argparse.ArgumentParser();p.add_argument('run');p.add_argument('--toolchain',type=Path);a=p.parse_args()
 if not a.run or not all(c.isalnum() or c in '-_' for c in a.run):p.error('fresh simple name required')
 out=ROOT/'local/validation/rtsp-server-candidate'/a.run;out.parent.mkdir(parents=True,exist_ok=True)
 if out.parent.resolve()!=out.parent:raise RuntimeError('output symlink refused')
 out.mkdir()
 for name in ('logs','objects','inputs','tmp'):(out/name).mkdir()
 env={'PATH':'/usr/bin:/bin','LC_ALL':'C','SOURCE_DATE_EPOCH':'1791763200','TMPDIR':str(out/'tmp')}
 report={'started':now(),'tasks':['SEC-05','STREAM-04','STREAM-07'],'evidence_state':'HOST ONLY','status':'RUNNING',
  'native_executed':False,'NVR_qualified':False,'existing_server_modified':False,'G3':False,'G8':False,'product_gates':False,
  'ports':'T34/Fulhan deferred','environment':env,'commands':[]}
 def run(argv,label,timeout=120):
  argv=list(map(str,argv));log=out/'logs'/(label+'.txt');entry={'argv':argv,'cwd':str(ROOT),'log':str(log),'started':now(),'timeout_seconds':timeout};report['commands'].append(entry)
  with log.open('xb') as f:r=subprocess.run(argv,cwd=ROOT,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=timeout)
  entry.update(exit=r.returncode,ended=now())
  if r.returncode:raise RuntimeError('failed '+label+'; see '+str(log))
  return log.read_text()
 def compiler(cc,label):
  report.setdefault('compilers',{})[label]={'version':run([cc,'--version'],label+'-version'),'target':run([cc,'-dumpmachine'],label+'-target').strip()}
  for item in ('cc1','as','ld','libgcc.a','crt1.o','Scrt1.o','crti.o','crtbeginS.o','crtbeginT.o','crtend.o','crtn.o','libc.a','libc.so'):
   flag='-print-prog-name=' if item in ('cc1','as','ld') else '-print-file-name='
   value=run([cc,flag+item],label+'-runtime-'+item.replace('.','_')).strip();f=Path(value) if '/' in value else Path(shutil.which(value,path=env['PATH']) or value)
   if f.is_file():report.setdefault('compiler_runtime_sha256',{})[str(f.resolve())]=sha(f)
 common=['-std=c99','-Wall','-Wextra','-Werror','-fno-common','-fno-strict-aliasing','-fstack-usage',
  '-DMBEDTLS_CONFIG_FILE="rtsp_crypto_config.h"','-I'+str(APP),'-I'+str(CONFIG),'-I'+str(ROOT/'framework/include'),'-I'+str(VENDOR/'include'),'-I'+str(VENDOR/'library')]
 def build(cc,label,flags,execute):
  directory=out/'objects'/label;directory.mkdir();objects=[]
  for i,src in enumerate(SOURCES):
   obj=directory/(str(i)+'-'+src.stem+'.o');legacy=[]
   if src.name in ('rtsp_server_candidate.c','rtsp_input.c'):legacy=['-Wno-misleading-indentation']
   if src.name=='rtsp_input.c':legacy+=['-Wno-error=nonnull'] # retained read-only dependency warning, not new-code exception
   run([cc,*common,*flags,*legacy,'-MD','-MF',obj.with_suffix('.d'),'-c',src,'-o',obj],label+'-compile-'+str(i));objects.append(obj)
  for name in ('fixture','backend_test','startup_wipe_test'):
   obj=directory/(name+'.o');run([cc,*common,*flags,'-MD','-MF',obj.with_suffix('.d'),'-c',APP/(name+'.c'),'-o',obj],label+'-compile-'+name)
   linked=objects if name!='startup_wipe_test' else objects[1:] # test includes copied server to instrument cleanup boundary only
   wrap=[]
   if name=='fixture':
    clock_obj=directory/'fault_clock.o';run([cc,*common,*flags,'-MD','-MF',clock_obj.with_suffix('.d'),'-c',APP/'fault_clock.c','-o',clock_obj],label+'-compile-fault-clock')
    linked=[*linked,clock_obj];wrap=['-Wl,--wrap=clock_gettime']
   binary=out/(label+'-'+name);run([cc,*flags,*wrap,*linked,obj,'-Wl,-t','-Wl,-Map='+str(directory/(name+'.map')),'-o',binary],label+'-link-'+name)
   report.setdefault('binaries',{})[label+'-'+name]={'path':str(binary),'sha256':sha(binary),'bytes':binary.stat().st_size,'executed':execute}
   if execute:
    if name!='fixture':report.setdefault('tests',{})[label+'-'+name]=run([binary],label+'-'+name,30).strip()
    else:report.setdefault('tests',{})[label+'-loopback']=run(['python3','-B',APP/'client_test.py',binary,out/(label+'-campaign')],label+'-loopback',180).strip()
   else:
    run([a.toolchain/'bin/arm-buildroot-linux-musleabi-readelf','-h','-A','-l',binary],label+'-'+name+'-metadata')
 try:
  report['dependency']=BASE.vendor_pin()
  provenance=json.loads((APP/'PROVENANCE.json').read_text());frozen=APP/'upstream/rtsp_server.c'
  if sha(frozen)!=provenance['sha256']:raise RuntimeError('copied upstream server pin changed')
  report['provenance']=provenance
  diff=list(difflib.unified_diff(frozen.read_text().splitlines(keepends=True),(APP/'rtsp_server_candidate.c').read_text().splitlines(keepends=True),fromfile='upstream/rtsp_server.c',tofile='candidate/rtsp_server_candidate.c'))
  with (out/'server.diff').open('x') as f:f.writelines(diff)
  report['diff']={'path':str(out/'server.diff'),'sha256':sha(out/'server.diff'),'added':sum(x.startswith('+') and not x.startswith('+++') for x in diff),'removed':sum(x.startswith('-') and not x.startswith('---') for x in diff)}
  own=[f for f in sorted(APP.rglob('*')) if f.is_file()];inputs=own+SOURCES+[CONFIG/'build.py',CONFIG/'rtsp_crypto_config.h']
  before={str(f):sha(f) for f in inputs};report['source_sha256']=before
  report['snapshots']={}
  for f in dict.fromkeys(inputs):
   dest=out/'inputs'/f.relative_to(ROOT);dest.parent.mkdir(parents=True,exist_ok=True)
   with dest.open('xb') as g:g.write(f.read_bytes())
   dest.chmod(0o444);report['snapshots'][str(dest)]=sha(dest)
  compiler('/usr/bin/cc','host');build('/usr/bin/cc','host',['-O2'],True)
  build('/usr/bin/cc','sanitized',['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie'],True)
  cpp=out/'header.cpp';cpp.write_text('#include "candidate_auth.h"\nint main(){candidate_backend b{};candidate_backend_destroy(&b);return b.initialized;}\n')
  compiler('/usr/bin/c++','cxx');objects=sorted((out/'objects/host').glob('*.o'));objects=[o for o in objects if o.name not in ('fixture.o','backend_test.o','startup_wipe_test.o','fault_clock.o')]
  run(['/usr/bin/c++','-std=c++11','-Wall','-Wextra','-Werror','-I'+str(APP),'-I'+str(ROOT/'framework/include'),cpp,*objects,'-Wl,-t','-o',out/'cxx-test'],'cxx-link');run([out/'cxx-test'],'cxx-test',30);report['cxx']='PASS'
  if a.toolchain:
   a.toolchain=a.toolchain.expanduser().resolve()
   for rel,pin in BASE.ARM_PINS.items():
    if sha(a.toolchain/rel)!=pin:raise RuntimeError('pinned ARM toolchain mismatch')
   report['ARM_toolchain_pins']=BASE.ARM_PINS;cc=a.toolchain/'bin/arm-buildroot-linux-musleabi-gcc';compiler(cc,'arm')
   build(cc,'arm',['-Os','-march=armv5te','-marm','-mfloat-abi=soft','-static'],False);report['ARM_link']='PASS, NEVER EXECUTED'
  closure={}
  for dep in (out/'objects').rglob('*.d'):
   for name in shlex.split(dep.read_text().replace('\\\n','').split(':',1)[1]):
    f=Path(name)
    if f.is_file():closure[str(f.resolve())]=sha(f)
  report['source_header_sha256']=closure;report['stack_usage']={str(f):f.read_text() for f in (out/'objects').rglob('*.su')}
  links={}
  for log in (out/'logs').glob('*link*.txt'):
   for line in log.read_text().splitlines():
    f=Path(line)
    if f.is_absolute() and f.is_file():links[str(f.resolve())]=sha(f)
  report['link_closure_sha256']=links
  if before!={str(f):sha(f) for f in inputs} or report['dependency']!=BASE.vendor_pin():raise RuntimeError('source/dependency changed during build')
  report['status']='HOST PASS; ARM LINK PASS' if a.toolchain else 'HOST PASS; ARM NOT REQUESTED'
 except BaseException as e:
  report['status']='FAIL';report['error_type']=type(e).__name__;raise
 finally:
  report['ended']=now()
  with (out/'REPORT.json').open('x') as f:json.dump(report,f,indent=2);f.write('\n')
  print('evidence='+str(out/'REPORT.json'))
if __name__=='__main__':main()
