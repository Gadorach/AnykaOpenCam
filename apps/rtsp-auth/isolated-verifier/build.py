#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Fresh pinned RTSP child/adapter HOST qualification; strict native ROOT/2M HOLD."""
from pathlib import Path
import argparse,datetime,difflib,hashlib,importlib.util,json,shlex,shutil,subprocess
ROOT=Path(__file__).resolve().parents[3];APP=Path(__file__).resolve().parent;CONFIG=APP.parent;V=ROOT/'apps/auth-web/service/vendor/mbedtls'
spec=importlib.util.spec_from_file_location('readonly_digest_build',CONFIG/'build.py');BASE=importlib.util.module_from_spec(spec);spec.loader.exec_module(BASE)
COMMON=[APP/'ipc.c',APP/'engine.c']+[ROOT/p for p in ('framework/src/rtsp_digest.c','framework/src/rtsp_digest_wire.c','framework/src/rtsp_request.c','framework/src/rtsp_input.c','framework/src/access_policy.c')]+[V/'library'/p for p in ('sha256.c','md5.c','constant_time.c','platform_util.c')]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def now():return datetime.datetime.now(datetime.timezone.utc).isoformat()
def main():
 p=argparse.ArgumentParser();p.add_argument('run');p.add_argument('--toolchain',type=Path);a=p.parse_args()
 if not a.run or not all(c.isalnum() or c in '-_' for c in a.run):p.error('fresh simple name')
 out=ROOT/'local/validation/rtsp-verifier'/a.run;out.parent.mkdir(parents=True,exist_ok=True);out.mkdir()
 for n in ('logs','objects','inputs','tmp'):(out/n).mkdir()
 env={'PATH':'/usr/bin:/bin','LC_ALL':'C','SOURCE_DATE_EPOCH':'1791763200','TMPDIR':str(out/'tmp')}
 report={'started':now(),'status':'RUNNING','evidence_state':'HOST ONLY','native_ROOT_2M':'HOLD no root creator/groups0 authority in this UID1000 session',
  'native_executed':False,'G3':False,'G8':False,'product_gates':False,'ports':'T34/Fulhan deferred','commands':[],'environment':env}
 def run(argv,label,timeout=120):
  argv=list(map(str,argv));log=out/'logs'/(label+'.txt');entry={'argv':argv,'cwd':str(ROOT),'log':str(log),'started':now(),'timeout_seconds':timeout};report['commands'].append(entry)
  with log.open('xb') as f:r=subprocess.run(argv,cwd=ROOT,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=timeout)
  entry.update(exit=r.returncode,ended=now())
  if r.returncode:raise RuntimeError('failed '+label+' see '+str(log))
  return log.read_text()
 def compiler(cc,label):
  report.setdefault('compiler',{})[label]={'version':run([cc,'--version'],label+'-version'),'target':run([cc,'-dumpmachine'],label+'-target').strip()}
  for n in ('cc1','as','ld','libgcc.a','crt1.o','Scrt1.o','crti.o','crtbeginS.o','crtbeginT.o','crtend.o','crtn.o','libc.a','libc.so'):
   value=run([cc,('-print-prog-name=' if n in ('cc1','as','ld') else '-print-file-name=')+n],label+'-runtime-'+n.replace('.','_')).strip();f=Path(value) if '/' in value else Path(shutil.which(value,path=env['PATH']) or value)
   if f.is_file():report.setdefault('compiler_runtime_sha256',{})[str(f.resolve())]=sha(f)
 flags=['-std=c99','-Wall','-Wextra','-Werror','-fno-common','-fno-strict-aliasing','-fstack-usage','-DMBEDTLS_CONFIG_FILE="rtsp_crypto_config.h"','-I'+str(APP),'-I'+str(CONFIG),'-I'+str(ROOT/'framework/include'),'-I'+str(V/'include'),'-I'+str(V/'library')]
 def compile_set(cc,label,opts,sources,defines=()):
  d=out/'objects'/label;d.mkdir();objs=[]
  for i,s in enumerate(sources):
   obj=d/(str(i)+'-'+s.stem+'.o');legacy=['-Wno-misleading-indentation','-Wno-error=nonnull'] if s.name=='rtsp_input.c' else []
   run([cc,*flags,*opts,*defines,*legacy,'-MD','-MF',obj.with_suffix('.d'),'-c',s,'-o',obj],label+'-compile-'+str(i));objs.append(obj)
  binary=out/label;run([cc,*opts,*objs,'-Wl,-t','-Wl,-Map='+str(d/'link.map'),'-o',binary],label+'-link')
  report.setdefault('binaries',{})[label]={'path':str(binary),'bytes':binary.stat().st_size,'sha256':sha(binary),'executed':False};return binary
 try:
  report['dependency']=BASE.vendor_pin();provenance=json.loads((APP/'PROVENANCE.json').read_text());report['provenance']=provenance
  for entry in provenance['records']:
   if sha(Path(entry['copy']))!=entry['sha256']:raise RuntimeError('canonical authority snapshot changed')
  inputs=[f for f in sorted(APP.rglob('*')) if f.is_file()]+COMMON+[CONFIG/'rtsp_crypto_config.h',CONFIG/'build.py']
  before={str(f):sha(f) for f in inputs};report['source_sha256']=before;report['snapshots']={}
  for f in dict.fromkeys(inputs):
   dst=out/'inputs'/f.relative_to(ROOT);dst.parent.mkdir(parents=True,exist_ok=True)
   with dst.open('xb') as g:g.write(f.read_bytes())
   dst.chmod(0o444);report['snapshots'][str(dst)]=sha(dst)
  report['adaptation_diffs']={}
  for authority,new in (('credential_ipc.c','ipc.c'),('credential_child.c','isolation.c'),('credential_process.c','adapter.c')):
   lines=list(difflib.unified_diff((APP/'authority'/authority).read_text().splitlines(keepends=True),(APP/new).read_text().splitlines(keepends=True),fromfile='canonical/'+authority,tofile='RTSP/'+new));d=out/(new+'.diff')
   with d.open('x') as f:f.writelines(lines)
   report['adaptation_diffs'][new]={'path':str(d),'sha256':sha(d),'added':sum(x.startswith('+') and not x.startswith('+++') for x in lines),'removed':sum(x.startswith('-') and not x.startswith('---') for x in lines)}
  compiler('/usr/bin/cc','host')
  children={}
  for label,defines in (('native-strict',()),('HOST-PUBLICDUMMY-4M',('-DRV_HOST_PUBLIC_ONLY',)),('HOST-PUBLICDUMMY-delay-4M',('-DRV_HOST_PUBLIC_ONLY','-DRV_HOST_DELAY_TEST'))):
   children[label]=compile_set('/usr/bin/cc',label,['-O2'],COMMON+[APP/'isolation.c',APP/'child.c'],defines)
   children[label].chmod(0o555) #ONLY fresh build output; never rv_start supplied inode/runtime source permissions
   report['binaries'][label]['code_mode']='0555 fresh produced public code'
  test=compile_set('/usr/bin/cc','host-adapter-test',['-O2'],COMMON+[APP/'adapter.c',APP/'test.c'],['-DRV_TEST_PUBLIC_ONLY'])
  report['tests']={'host-adapter':run([test,*children.values()],'host-adapter-test',60)};report['binaries']['host-adapter-test']['executed']=True
  startup=compile_set('/usr/bin/cc','host-startup-test',['-O2'],COMMON+[APP/'adapter.c',APP/'startup_test.c'],['-DRV_TEST_PUBLIC_ONLY'])
  (out/'host-startup-artifacts').mkdir()
  report['tests']['host-startup-boundaries']=run([startup,children['HOST-PUBLICDUMMY-4M'],children['native-strict'],out/'host-startup-artifacts'],'host-startup-test',60);report['binaries']['host-startup-test']['executed']=True
  for label in children:report['binaries'][label]['executed']=label!='native-strict' #native profile rejects nonroot/x86 image BEFORE fork; not executed
  engine=compile_set('/usr/bin/cc','host-engine-test',['-O2'],COMMON+[APP/'engine_test.c']);report['tests']['engine']=run([engine],'host-engine-test',30);report['binaries']['host-engine-test']['executed']=True
  san=['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie']
  engine=compile_set('/usr/bin/cc','sanitized-engine-test',san,COMMON+[APP/'engine_test.c']);report['tests']['sanitized-engine']=run([engine],'sanitized-engine-test',30);report['binaries']['sanitized-engine-test']['executed']=True
  test=compile_set('/usr/bin/cc','sanitized-adapter-test',san,COMMON+[APP/'adapter.c',APP/'test.c'],['-DRV_TEST_PUBLIC_ONLY']);report['tests']['sanitized-adapter-real-child']=run([test,*children.values()],'sanitized-adapter-test',60);report['binaries']['sanitized-adapter-test']['executed']=True
  startup=compile_set('/usr/bin/cc','sanitized-startup-test',san,COMMON+[APP/'adapter.c',APP/'startup_test.c'],['-DRV_TEST_PUBLIC_ONLY']);(out/'sanitized-startup-artifacts').mkdir()
  report['tests']['sanitized-startup-boundaries']=run([startup,children['HOST-PUBLICDUMMY-4M'],children['native-strict'],out/'sanitized-startup-artifacts'],'sanitized-startup-test',60);report['binaries']['sanitized-startup-test']['executed']=True
  cpp=out/'header.cpp';cpp.write_text('#include "verifier.h"\nint main(){return rv_engine_bytes()>32768;}\n')
  # Link actual C API via C++ with child-free common objects, never a C++ mock.
  objs=[f for f in sorted((out/'objects/host-engine-test').glob('*.o')) if not f.name.endswith('-engine_test.o')]
  run(['/usr/bin/c++','-std=c++11','-Wall','-Wextra','-Werror','-I'+str(APP),cpp,*objs,'-o',out/'cxx-test'],'cxx-link');run([out/'cxx-test'],'cxx-test',30);report['cxx']='PASS'
  if a.toolchain:
   a.toolchain=a.toolchain.expanduser().resolve()
   for rel,pin in BASE.ARM_PINS.items():
    if sha(a.toolchain/rel)!=pin:raise RuntimeError('pinned ARM toolchain mismatch')
   cc=a.toolchain/'bin/arm-buildroot-linux-musleabi-gcc';compiler(cc,'arm');report['ARM_toolchain_pins']=BASE.ARM_PINS
   binary=compile_set(cc,'ARM-native-strict',['-Os','-march=armv5te','-marm','-mfloat-abi=soft','-static'],COMMON+[APP/'isolation.c',APP/'child.c'])
   run([a.toolchain/'bin/arm-buildroot-linux-musleabi-readelf','-h','-A','-l',binary],'ARM-metadata')
   adapter=compile_set(cc,'ARM-adapter-candidate',['-Os','-march=armv5te','-marm','-mfloat-abi=soft','-static'],COMMON+[APP/'adapter.c',APP/'test.c'],['-DRV_TEST_PUBLIC_ONLY'])
   run([a.toolchain/'bin/arm-buildroot-linux-musleabi-readelf','-h','-A','-l',adapter],'ARM-adapter-metadata')
   report['ARM_link']='child + adapter PASS, NOT EXECUTED; native ROOT/2M readiness remains HOLD'
  closure={}
  for dep in (out/'objects').rglob('*.d'):
   for n in shlex.split(dep.read_text().replace('\\\n','').split(':',1)[1]):
    f=Path(n)
    if f.is_file():closure[str(f.resolve())]=sha(f)
  report['source_header_sha256']=closure;report['stack_usage']={str(f):f.read_text() for f in (out/'objects').rglob('*.su')}
  link={}
  for log in (out/'logs').glob('*link.txt'):
   for n in log.read_text().splitlines():
    f=Path(n)
    if f.is_absolute() and f.is_file():link[str(f.resolve())]=sha(f)
  report['link_closure_sha256']=link
  if before!={str(f):sha(f) for f in inputs} or report['dependency']!=BASE.vendor_pin():raise RuntimeError('source changed during build')
  report['status']='HOST PUBLICDUMMY PASS; NATIVE ROOT/2M HOLD'
 except BaseException as e:report['status']='FAIL/HOLD';report['error_type']=type(e).__name__;raise
 finally:
  report['ended']=now()
  with (out/'REPORT.json').open('x') as f:json.dump(report,f,indent=2);f.write('\n')
  print('evidence='+str(out/'REPORT.json'))
if __name__=='__main__':main()
