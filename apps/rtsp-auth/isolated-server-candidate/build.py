#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Only owned PUBLICDUMMY loopback/child executes; ARM + strict native SOURCE HOLD."""
from pathlib import Path
import argparse,datetime,difflib,hashlib,importlib.util,json,shlex,shutil,subprocess
ROOT=Path(__file__).resolve().parents[3];APP=Path(__file__).resolve().parent;VER=APP.parent/'isolated-verifier';V=ROOT/'apps/auth-web/service/vendor/mbedtls'
spec=importlib.util.spec_from_file_location('readonly_rtsp_crypto_build',APP.parent/'build.py');BASE=importlib.util.module_from_spec(spec);spec.loader.exec_module(BASE)
FRAME=[ROOT/'framework/src'/n for n in ('rtsp_digest_wire.c','rtsp_request.c','rtsp_input.c','backend.c','packet_hub.c','producer.c','rtp_h264.c')]
SERVER=[APP/'rtsp_server_candidate.c',APP/'candidate_auth.c',APP/'verifier_adapter_overlay.c',VER/'ipc.c',*FRAME]+[V/'library'/n for n in ('sha256.c','constant_time.c','platform_util.c')]
GEN=[APP/'public_dummy_material.c',VER/'ipc.c',ROOT/'framework/src/rtsp_digest.c']+[V/'library'/n for n in ('sha256.c','md5.c','constant_time.c','platform_util.c')]
CHILD=[VER/n for n in ('ipc.c','engine.c','isolation.c','child.c')]+[ROOT/'framework/src'/n for n in ('rtsp_digest.c','rtsp_digest_wire.c','rtsp_request.c','rtsp_input.c','access_policy.c')]+[V/'library'/n for n in ('sha256.c','md5.c','constant_time.c','platform_util.c')]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def now():return datetime.datetime.now(datetime.timezone.utc).isoformat()
def main():
 p=argparse.ArgumentParser();p.add_argument('run');p.add_argument('--toolchain',type=Path);a=p.parse_args()
 if not a.run or not all(c.isalnum() or c in '-_' for c in a.run):p.error('fresh simple name')
 out=ROOT/'evidence/delegated/20261012-rtsp-isolated-server/runs'/a.run;out.parent.mkdir(parents=True,exist_ok=True)
 if out.parent.resolve()!=out.parent:raise RuntimeError('output symlink')
 out.mkdir()
 for n in ('logs','objects','inputs','tmp'):(out/n).mkdir()
 env={'PATH':'/usr/bin:/bin','LC_ALL':'C','SOURCE_DATE_EPOCH':'1791763200','TMPDIR':str(out/'tmp')}
 r={'started':now(),'evidence_state':'SOURCE / HOST PUBLICDUMMY ONLY','status':'RUNNING','tasks':['SEC-05','SEC-11','STREAM-04','STREAM-07'],
    'native_executed':False,'PRIVATE_generation':False,'G3':False,'G8':False,'NVR_qualified':False,'product_gates':False,'commands':[],'environment':env}
 def run(argv,label,timeout=120):
  argv=list(map(str,argv));log=out/'logs'/(label+'.txt');entry={'argv':argv,'cwd':str(ROOT),'log':str(log),'started':now(),'timeout_seconds':timeout};r['commands'].append(entry)
  with log.open('xb') as f:v=subprocess.run(argv,cwd=ROOT,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=timeout)
  entry.update(exit=v.returncode,ended=now())
  if v.returncode:raise RuntimeError('failed '+label+' see '+str(log))
  return log.read_text()
 def compiler(cc,label):
  r.setdefault('compiler',{})[label]={'version':run([cc,'--version'],label+'-version'),'target':run([cc,'-dumpmachine'],label+'-target').strip()}
  for n in ('cc1','as','ld','libgcc.a','crt1.o','Scrt1.o','crti.o','crtbeginS.o','crtbeginT.o','crtend.o','crtn.o','libc.a','libc.so'):
   s=run([cc,('-print-prog-name=' if n in ('cc1','as','ld') else '-print-file-name=')+n],label+'-runtime-'+n.replace('.','_')).strip();f=Path(s) if '/' in s else Path(shutil.which(s,path=env['PATH']) or s)
   if f.is_file():r.setdefault('runtime_sha256',{})[str(f.resolve())]=sha(f)
 common=['-std=c99','-Wall','-Wextra','-Werror','-fno-common','-fno-strict-aliasing','-ffunction-sections','-fdata-sections','-fstack-usage','-I'+str(APP),'-I'+str(VER),'-I'+str(ROOT/'framework/include'),'-I'+str(V/'include'),'-I'+str(V/'library')]
 def build(cc,label,opts,sources,defines=(),wrap=(),server=False):
  d=out/'objects'/label;d.mkdir();objs=[]
  config='server_crypto_config.h' if server else 'rtsp_crypto_config.h'
  for i,s in enumerate(sources):
   obj=d/(str(i)+'-'+s.stem+'.o');legacy=['-Wno-misleading-indentation'] if s.name in ('rtsp_server_candidate.c','memory_shape.c','rtsp_input.c') else []
   if s.name=='rtsp_input.c':legacy+=['-Wno-error=nonnull']
   run([cc,*common,*opts,*defines,*legacy,'-DMBEDTLS_CONFIG_FILE="'+config+'"','-MD','-MF',obj.with_suffix('.d'),'-c',s,'-o',obj],label+'-compile-'+str(i));objs.append(obj)
  keep=['-Wl,--undefined=is_native_shape_values'] if 'memory-shape' in label else []
  binary=out/label;run([cc,*opts,*objs,*['-Wl,--wrap='+n for n in wrap],*keep,'-Wl,--gc-sections','-Wl,-t','-Wl,-Map='+str(d/'link.map'),'-o',binary],label+'-link')
  r.setdefault('binaries',{})[label]={'path':str(binary),'bytes':binary.stat().st_size,'sha256':sha(binary),'executed':False};return binary
 try:
  r['dependency']=BASE.vendor_pin();pins=json.loads((APP/'PROVENANCE.json').read_text());r['provenance']=pins
  for rec in pins['derived_V3']:
   if sha(Path(rec['copy']))!=rec['sha256'] or sha(Path(rec['source']))!=rec['sha256']:raise RuntimeError('reviewed V3 changed')
  for rec in pins['reviewed_V2']:
   if sha(Path(rec['source']))!=rec['sha256']:raise RuntimeError('reviewed V2 changed')
  for rec in pins['images'].values():
   if sha(Path(rec['path']))!=rec['sha256']:raise RuntimeError('reviewed image changed')
  inputs=list(dict.fromkeys([f for f in sorted(APP.rglob('*')) if f.is_file()]+SERVER+GEN+CHILD+[VER/'verifier.h',APP.parent/'build.py']))
  before={str(f):sha(f) for f in inputs};r['source_sha256']=before;r['snapshots']={}
  for f in inputs:
   dest=out/'inputs'/f.relative_to(ROOT);dest.parent.mkdir(parents=True,exist_ok=True)
   with dest.open('xb') as g:g.write(f.read_bytes())
   dest.chmod(0o444);r['snapshots'][str(dest)]=sha(dest)
  for n in ('rtsp_server_candidate.c','candidate_auth.c','candidate_auth.h','fixture.c','client_test.py'):
   diff=out/(n+'.diff');lines=difflib.unified_diff((APP/'upstream'/n).read_text().splitlines(keepends=True),(APP/n).read_text().splitlines(keepends=True),fromfile='reviewed-V3/'+n,tofile='isolated/'+n)
   with diff.open('x') as f:f.writelines(lines)
  with (out/'verifier-adapter-overlay.diff').open('x') as f:
   f.writelines(difflib.unified_diff((VER/'adapter.c').read_text().splitlines(keepends=True),(APP/'verifier_adapter_overlay.c').read_text().splitlines(keepends=True),fromfile='reviewed-V2/adapter.c',tofile='isolated/verifier_adapter_overlay.c'))
  compiler('/usr/bin/cc','host')
  gen=build('/usr/bin/cc','PUBLICDUMMY-bootstrap',['-O2'],GEN,['-DIS_PUBLICDUMMY_GENERATOR'])
  opts=['-O2'];fixture=[APP/n for n in ('fixture.c','fault_clock.c','wait_receipt.c','pread_receipt.c')]
  strict=build('/usr/bin/cc','strict-native-refusal-SOURCE',opts,SERVER+fixture,['-DIS_SYNTHETIC_SOURCE_ONLY'],('clock_gettime','waitpid','pread'),True)
  for label,flags in [('host',opts),('sanitized',['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie'])]:
   binary=build('/usr/bin/cc',label+'-server',flags,SERVER+fixture,['-DIS_HOST_PUBLIC_ONLY','-DIS_SYNTHETIC_SOURCE_ONLY'],('clock_gettime','waitpid','pread'),True)
   r.setdefault('tests',{})[label+'-loopback']=run(['python3','-B',APP/'client_test.py',binary,gen,pins['images']['host']['path'],out/(label+'-campaign')],label+'-loopback',240)
   r['binaries'][label+'-server']['executed']=True
   symbols=run(['/usr/bin/nm','--defined-only',binary],label+'-server-code-symbols')
   if any(n in symbols for n in ('camera_rtsp_digest_ha1','camera_rtsp_digest_provision','camera_rtsp_digest_verify','camera_access_issue')):raise RuntimeError('in-process credential/principal crypto linked into server')
   r.setdefault('server_custody',{})[label]='PASS no local Digest HA1/provision/verify or principal issue symbols; child only'
   delay=build('/usr/bin/cc',label+'-delay-server',flags,SERVER+fixture,['-DIS_HOST_PUBLIC_ONLY','-DIS_HOST_DELAY_FIXTURE','-DIS_SYNTHETIC_SOURCE_ONLY'],('clock_gettime','waitpid','pread'),True)
   r['tests'][label+'-integration']=run(['python3','-B',APP/'integration_test.py',binary,delay,strict,gen,pins['images']['host']['path'],pins['images']['delay']['path'],out/(label+'-integration')],label+'-integration',180)
   r['binaries'][label+'-delay-server']['executed']=True
  r['binaries']['PUBLICDUMMY-bootstrap']['executed']=True
  r['binaries']['strict-native-refusal-SOURCE']['executed']=True
  r['binaries']['strict-native-refusal-SOURCE']['execution_scope']='HOST nonroot before material refusal ONLY, not native entry'
  shape=build('/usr/bin/cc','host-memory-shape',opts,[APP/'memory_shape.c',*SERVER[1:]],['-DIS_HOST_PUBLIC_ONLY'],(),True)
  r['tests']['host-memory-shape']=run([shape],'host-memory-shape',30)
  r['binaries']['host-memory-shape']['executed']=True
  cpp=out/'header.cpp';cpp.write_text('#include "candidate_auth.h"\nint main(){candidate_backend b{};return candidate_backend_destroy(&b);}\n')
  objs=[f for f in sorted((out/'objects/host-server').glob('*.o')) if not any(f.name.endswith('-'+n+'.o') for n in ('fixture','fault_clock','wait_receipt'))]
  run(['/usr/bin/c++','-std=c++11','-Wall','-Wextra','-Werror','-I'+str(APP),'-I'+str(VER),'-I'+str(ROOT/'framework/include'),cpp,*objs,'-Wl,--gc-sections','-o',out/'cxx-test'],'cxx-link');run([out/'cxx-test'],'cxx-test',30);r['cxx']='PASS'
  if a.toolchain:
   a.toolchain=a.toolchain.expanduser().resolve()
   for p,h in BASE.ARM_PINS.items():
    if sha(a.toolchain/p)!=h:raise RuntimeError('ARM pin mismatch')
   cc=a.toolchain/'bin/arm-buildroot-linux-musleabi-gcc';compiler(cc,'arm');r['ARM_toolchain_pins']=BASE.ARM_PINS
   flags=['-Os','-march=armv5te','-marm','-mfloat-abi=soft','-static']
   for label,sources,defines,wrap,server in [('ARM-server-adapter',SERVER+fixture,('-DIS_SYNTHETIC_SOURCE_ONLY',),('clock_gettime','waitpid','pread'),True),('ARM-child-SOURCE',CHILD,(),(),False),('ARM-memory-shape-SOURCE',[APP/'memory_shape.c',*SERVER[1:]],(),(),True)]:
    binary=build(cc,label,flags,sources,defines,wrap,server);run([a.toolchain/'bin/arm-buildroot-linux-musleabi-readelf','-h','-A','-l',binary],label+'-metadata')
    if 'memory-shape' in label:r['ARM_memory_shape_section']=run([a.toolchain/'bin/arm-buildroot-linux-musleabi-readelf','-x','.is_shape',binary],label+'-public-scalar-shape-section')
   r['ARM_link']='server/adapter + child PASS NEVER EXECUTED; reviewed native IMAGE pin unchanged'
  closure={}
  for p in (out/'objects').rglob('*.d'):
   for n in shlex.split(p.read_text().replace('\\\n','').split(':',1)[1]):
    f=Path(n)
    if f.is_file():closure[str(f.resolve())]=sha(f)
  r['source_header_sha256']=closure;r['stack_usage']={str(f):f.read_text() for f in (out/'objects').rglob('*.su')}
  links={}
  for log in (out/'logs').glob('*link*.txt'):
   for line in log.read_text().splitlines():
    f=Path(line)
    if f.is_absolute() and f.is_file():links[str(f.resolve())]=sha(f)
  r['link_closure_sha256']=links
  if before!={str(f):sha(f) for f in inputs} or r['dependency']!=BASE.vendor_pin():raise RuntimeError('source changed')
  r['status']='SOURCE/HOST PASS; native secret/root/2M/producer/NVR HOLD'
 except BaseException as e:r['status']='FAIL/HOLD';r['error_type']=type(e).__name__;raise
 finally:
  r['ended']=now()
  with (out/'REPORT.json').open('x') as f:json.dump(r,f,indent=2);f.write('\n')
  print('evidence='+str(out/'REPORT.json'))
if __name__=='__main__':main()
