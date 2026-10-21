#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Offline pinned root-entry SOURCE build + PUBLICDUMMY tests only. Never PRIVATE/ARM execute."""
from pathlib import Path
import argparse,datetime,difflib,hashlib,importlib.util,json,shlex,shutil,subprocess
ROOT=Path(__file__).resolve().parents[3];APP=Path(__file__).resolve().parent;VER=APP.parent/'isolated-verifier';CONFIG=APP.parent;V=ROOT/'apps/auth-web/service/vendor/mbedtls'
spec=importlib.util.spec_from_file_location('readonly_digest_build',CONFIG/'build.py');BASE=importlib.util.module_from_spec(spec);spec.loader.exec_module(BASE)
LIB=[VER/'ipc.c',VER/'adapter.c']+[V/'library'/p for p in ('sha256.c','md5.c','constant_time.c','platform_util.c')]
CRYPTO=[VER/'ipc.c',ROOT/'framework/src/rtsp_digest.c',ROOT/'framework/src/rtsp_request.c',ROOT/'framework/src/rtsp_input.c']+[V/'library'/p for p in ('sha256.c','md5.c','constant_time.c','platform_util.c')]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def now():return datetime.datetime.now(datetime.timezone.utc).isoformat()
def main():
 p=argparse.ArgumentParser();p.add_argument('run');p.add_argument('--toolchain',type=Path);a=p.parse_args()
 if not a.run or not all(c.isalnum() or c in '-_' for c in a.run):p.error('fresh simple name')
 out=ROOT/'evidence/delegated/20261012-native-rtsp-verifier/runs'/a.run;out.parent.mkdir(parents=True,exist_ok=True);out.mkdir()
 for n in ('logs','objects','inputs','tmp'):(out/n).mkdir()
 env={'PATH':'/usr/bin:/bin','LC_ALL':'C','SOURCE_DATE_EPOCH':'1791763200','TMPDIR':str(out/'tmp')}
 report={'started':now(),'evidence_state':'SOURCE / HOST PUBLICDUMMY ONLY','status':'RUNNING','native_ROOT_2M':'HOLD parent only',
  'PRIVATE_generation_executed':False,'ARM_executed':False,'positive_native_RTSP_verify_NOTtested':True,'G3':False,'G8':False,'product_gates':False,
  'environment':env,'commands':[]}
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
 common=['-std=c99','-Wall','-Wextra','-Werror','-fno-common','-fno-strict-aliasing','-fstack-usage','-DMBEDTLS_CONFIG_FILE="rtsp_crypto_config.h"','-I'+str(APP),'-I'+str(VER),'-I'+str(CONFIG),'-I'+str(ROOT/'framework/include'),'-I'+str(V/'include'),'-I'+str(V/'library')]
 def build(cc,label,opts,sources,defines=(),wrap=False):
  d=out/'objects'/label;d.mkdir();objs=[]
  for i,src in enumerate(sources):
   obj=d/(str(i)+'-'+src.stem+'.o');legacy=['-Wno-misleading-indentation','-Wno-error=nonnull'] if src.name=='rtsp_input.c' else []
   run([cc,*common,*opts,*defines,*legacy,'-MD','-MF',obj.with_suffix('.d'),'-c',src,'-o',obj],label+'-compile-'+str(i));objs.append(obj)
  binary=out/label;run([cc,*opts,*objs,*(['-Wl,--wrap=waitpid'] if wrap else []),'-Wl,-t','-Wl,-Map='+str(d/'link.map'),'-o',binary],label+'-link')
  report.setdefault('binaries',{})[label]={'path':str(binary),'sha256':sha(binary),'bytes':binary.stat().st_size,'executed':False};return binary
 try:
  report['dependency']=BASE.vendor_pin();pins=json.loads((APP/'PROVENANCE.json').read_text());report['provenance']=pins
  for p in pins['reviewed_isolated_v2_source_pins']:
   if sha(Path(p['source']))!=p['sha256']:raise RuntimeError('reviewed verifier source changed')
  for b in pins['image_pins'].values():
   if sha(Path(b['path']))!=b['sha256']:raise RuntimeError('exact reviewed image pin changed')
  sources=[p for p in sorted(APP.rglob('*')) if p.is_file()]+LIB+CRYPTO+[VER/'verifier.h',CONFIG/'rtsp_crypto_config.h',VER/'authority/launch.c']
  before={str(p):sha(p) for p in sources};report['source_sha256']=before;report['snapshots']={}
  for p in dict.fromkeys(sources):
   dst=out/'inputs'/p.relative_to(ROOT);dst.parent.mkdir(parents=True,exist_ok=True)
   with dst.open('xb') as f:f.write(p.read_bytes())
   dst.chmod(0o444);report['snapshots'][str(dst)]=sha(dst)
  #Minimal path/wait source adaptation only; original launcher is read-only.
  for name in ('paths.c','wait_receipt.c'):
   lines=list(difflib.unified_diff((VER/'authority/launch.c').read_text().splitlines(keepends=True),(APP/name).read_text().splitlines(keepends=True),fromfile='canonical/launch.c',tofile='qualification/'+name));diff=out/(name+'.diff')
   with diff.open('x') as f:f.writelines(lines)
   report.setdefault('adaptation_diffs',{})[name]={'path':str(diff),'sha256':sha(diff)}
  compiler('/usr/bin/cc','host')
  helper=build('/usr/bin/cc','HOST-PUBLICDUMMY-helper',['-O2'],LIB+[APP/n for n in ('paths.c','entry.c','wait_receipt.c','helper.c')],['-DNQ_HOST_PUBLIC_ONLY'],True)
  strict=build('/usr/bin/cc','ROOT-helper-SOURCE',['-O2'],LIB+[APP/n for n in ('paths.c','entry.c','wait_receipt.c','helper.c')],(),True)
  dummy=build('/usr/bin/cc','PUBLICDUMMY-material-tool',['-O2'],CRYPTO+[APP/'paths.c',APP/'material_tool.c'],['-DNQ_HOST_PUBLIC_ONLY'])
  private=build('/usr/bin/cc','PRIVATE-PC-material-tool-SOURCE',['-O2'],CRYPTO+[APP/'paths.c',APP/'material_tool.c']) #NEVER execute
  hardfault=build('/usr/bin/cc','HOST-PUBLICDUMMY-hardfault-helper',['-O2'],LIB+[APP/n for n in ('paths.c','entry.c','wait_receipt.c','helper.c')],['-DNQ_HOST_PUBLIC_ONLY','-DNQ_HOST_ENTRY_HARDFAULT_TEST'],True)
  report['tests']={'host':run(['python3','-B',APP/'test_host.py',dummy,helper,strict,hardfault,pins['image_pins']['HOST_PUBLICDUMMY']['path'],out/'HOST-campaign'],'HOST-campaign',60)}
  for label in ('HOST-PUBLICDUMMY-helper','ROOT-helper-SOURCE','PUBLICDUMMY-material-tool','HOST-PUBLICDUMMY-hardfault-helper'):
   report['binaries'][label]['executed']=True
  report['binaries']['ROOT-helper-SOURCE']['execution_scope']='HOST nonroot pre-input refusal ONLY, not native root entry'
  san=['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie']
  helper=build('/usr/bin/cc','SAN-PUBLICDUMMY-helper',san,LIB+[APP/n for n in ('paths.c','entry.c','wait_receipt.c','helper.c')],['-DNQ_HOST_PUBLIC_ONLY'],True)
  dummy=build('/usr/bin/cc','SAN-PUBLICDUMMY-material-tool',san,CRYPTO+[APP/'paths.c',APP/'material_tool.c'],['-DNQ_HOST_PUBLIC_ONLY'])
  hardfault=build('/usr/bin/cc','SAN-PUBLICDUMMY-hardfault-helper',san,LIB+[APP/n for n in ('paths.c','entry.c','wait_receipt.c','helper.c')],['-DNQ_HOST_PUBLIC_ONLY','-DNQ_HOST_ENTRY_HARDFAULT_TEST'],True)
  report['tests']['sanitized-host']=run(['python3','-B',APP/'test_host.py',dummy,helper,strict,hardfault,pins['image_pins']['HOST_PUBLICDUMMY']['path'],out/'SAN-campaign'],'SAN-campaign',60)
  pure=build('/usr/bin/cc','SAN-path-ABI-test',san,[APP/'paths.c',APP/'path_test.c']);report['tests']['path-ABI']=run([pure],'path-ABI-test',30)
  for label in ('SAN-PUBLICDUMMY-helper','SAN-PUBLICDUMMY-material-tool','SAN-PUBLICDUMMY-hardfault-helper','SAN-path-ABI-test'):
   report['binaries'][label]['executed']=True
  cpp=out/'header.cpp';cpp.write_text('#include "source_abi.h"\nint main(){return sizeof(rv_material)!=1536;}\n')
  run(['/usr/bin/c++','-std=c++11','-Wall','-Wextra','-Werror','-I'+str(APP),'-I'+str(VER),cpp,'-o',out/'cxx-ABI-test'],'cxx-ABI-link');run([out/'cxx-ABI-test'],'cxx-ABI-run',30);report['cxx']='PASS'
  if a.toolchain:
   a.toolchain=a.toolchain.expanduser().resolve()
   for p,h in BASE.ARM_PINS.items():
    if sha(a.toolchain/p)!=h:raise RuntimeError('ARM pinned toolchain mismatch')
   cc=a.toolchain/'bin/arm-buildroot-linux-musleabi-gcc';compiler(cc,'arm');report['ARM_toolchain_pins']=BASE.ARM_PINS
   binary=build(cc,'ARM-ROOT-native-entry-SOURCE',['-Os','-march=armv5te','-marm','-mfloat-abi=soft','-static'],LIB+[APP/n for n in ('paths.c','entry.c','wait_receipt.c','helper.c')],(),True)
   run([a.toolchain/'bin/arm-buildroot-linux-musleabi-readelf','-h','-A','-l',binary],'ARM-metadata');report['ARM_link']='PASS NEVER EXECUTED; exact V2 child image remains distinct'
  closure={}
  for p in (out/'objects').rglob('*.d'):
   for n in shlex.split(p.read_text().replace('\\\n','').split(':',1)[1]):
    f=Path(n)
    if f.is_file():closure[str(f.resolve())]=sha(f)
  report['source_header_sha256']=closure;report['stack_usage']={str(p):p.read_text() for p in (out/'objects').rglob('*.su')}
  link={}
  for p in (out/'logs').glob('*link.txt'):
   for n in p.read_text().splitlines():
    f=Path(n)
    if f.is_absolute() and f.is_file():link[str(f.resolve())]=sha(f)
  report['link_closure_sha256']=link
  if before!={str(p):sha(p) for p in sources} or report['dependency']!=BASE.vendor_pin():raise RuntimeError('source changed during build')
  report['status']='OFFLINE SOURCE/HOST PUBLICDUMMY PASS; NATIVE ROOT ENTRY HOLD'
 except BaseException as e:report['status']='FAIL/HOLD';report['error_type']=type(e).__name__;raise
 finally:
  report['ended']=now()
  with (out/'REPORT.json').open('x') as f:json.dump(report,f,indent=2);f.write('\n')
  print('evidence='+str(out/'REPORT.json'))
if __name__=='__main__':main()
