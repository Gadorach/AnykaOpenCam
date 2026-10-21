#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Canonical source-only HOST/ARM service build; fresh local outputs."""
from pathlib import Path
import argparse,concurrent.futures,json,shutil,sys,traceback
A=Path(__file__).resolve().parent
sys.path.insert(0,str(A.parent))
from build_support import ROOT,Run,fresh_run,copy_scope,validate_dependencies,toolchain,digest
p=argparse.ArgumentParser();p.add_argument('run');p.add_argument('--output-root',type=Path);p.add_argument('--toolchain',type=Path)
args=p.parse_args();R=fresh_run(args.run,'service',args.output_root);r=Run(R,'canonical source HOST/ARM auth service; no target/private input')
I=r.inputs;D=r.build;L=r.logs;m=r.manifest
try:
 m['canonical_dependencies']=validate_dependencies(A);copy_scope(A,I)
 TC,prefix,pins=toolchain(args.toolchain);m['verified_toolchain_components']=pins
 r.run(['python3','-B',A/'signal_abi_test.py',R/'arm-filter-regression','--toolchain',TC],
       'ARM-signal-filter-regression',30)
 m['source_scope']=str(A);m['historical_source_or_library_inputs']=False
 include=['-I'+str(I),'-I'+str(I/'credential'),'-I'+str(I/'credential/reviewed-contracts'),
  '-I'+str(I/'vendor/mongoose'),'-I'+str(I/'vendor/mbedtls/include'),'-I'+str(I/'vendor/mbedtls/library'),
  '-DMBEDTLS_CONFIG_FILE="'+str(I/'tls13_config.h')+'"']
 common=['-std=gnu11','-Os','-pthread','-ffunction-sections','-fdata-sections','-fno-strict-aliasing','-fstack-protector-strong','-D_FORTIFY_SOURCE=2',*include]
 mflags=['-DMG_ENABLE_LOG=0','-DMG_ENABLE_POSIX_FS=0','-DMG_ENABLE_DIRLIST=0','-DMG_ENABLE_SSI=0','-DMG_ENABLE_TCPIP=0',
  '-DMG_ENABLE_BSD_SOCKETS=0','-DMG_ENABLE_MD5=0','-DMG_ENABLE_CUSTOM_RANDOM=1','-DMG_ENABLE_CUSTOM_MILLIS=1',
  '-DMG_IO_SIZE=1024','-DMG_MAX_RECV_SIZE=16384','-DMG_MAX_HTTP_HEADERS=24','-DMG_SOCK_LISTEN_BACKLOG_SIZE=2','-DMG_TLS=MG_TLS_MBED']
 service=[I/'web_service.c',I/'parent_sandbox.c',I/'media_admission.c',I/'bounded_heap.c',
  I/'credential/credential_mbedtls.c',I/'credential/credential_ipc.c',I/'credential/credential_process.c',
  I/'credential/reviewed-contracts/access_policy.c',I/'vendor/mongoose/mongoose.c']
 child=[I/'credential'/name for name in ['credential_child.c','credential_worker.c','credential_mbedtls.c','credential_ipc.c']]+[I/'credential/reviewed-contracts/access_policy.c']
 wraps=['-Wl,--wrap='+name for name in ['malloc','calloc','realloc','free']]
 m['production_compound_sources']={'service':[str(s.relative_to(I)) for s in service],'child':[str(s.relative_to(I)) for s in child]}
 compiled_arch={}
 for arch,cc,flags in [('host','cc',[]),('arm',prefix+'gcc',['-march=armv5te','-marm','-mfloat-abi=soft'])]:
  r.compiler_closure(cc,arch)
  if arch=='arm':
   if '14.3.0' not in (L/'arm-compiler-version.txt').read_text() or 'arm-buildroot-linux-musleabi' not in (L/'arm-compiler-target.txt').read_text():raise RuntimeError('unreviewed ARM compiler ABI')
  library_dir=D/(arch+'lib');library_dir.mkdir()
  def compile_library(source):
   obj=library_dir/(source.stem+'.o');r.run([cc,*common,*flags,'-MD','-MF',str(obj)+'.d','-c',source,'-o',obj],arch+'-mbed-'+source.stem);return obj
  with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:objects=list(pool.map(compile_library,sorted((I/'vendor/mbedtls/library').glob('*.c'))))
  library=D/('libmbed-'+arch+'.a');r.run([prefix+'ar' if arch=='arm' else 'ar','rcs',library,*objects],arch+'-archive')
  compiled={}
  for source in dict.fromkeys(service+child):
   tag=arch+'-'+source.stem;obj=D/(tag+'.o')
   warning=[] if source.name=='mongoose.c' else ['-Wall','-Wextra','-Werror','-Wpedantic']
   host_scope=['-DWEB_PUBLIC_HOST_TEST'] if arch=='host' and source.name=='web_service.c' else []
   r.run([cc,*common,*flags,*mflags,*host_scope,*warning,'-fstack-usage','-MD','-MF',D/(tag+'.d'),'-c',source,'-o',obj],tag+'-compile');compiled[source]=obj
  compiled_arch[arch]=compiled
  for target,sources in [('web-service',service),('credential-child',child)]:
   binary=D/(target+'-'+arch)
   r.run([cc,*common,*flags,*[compiled[s] for s in sources],library,*(['-static'] if arch=='arm' else []),
    *(wraps if target=='web-service' else []),'-Wl,--gc-sections','-Wl,-z,noexecstack','-Wl,--trace','-Wl,-Map='+str(binary)+'.map','-o',binary],target+'-'+arch+'-link')
   tools=prefix if arch=='arm' else ''
   r.run([tools+'size',binary],target+'-'+arch+'-size');r.run([tools+'readelf','-h','-A','-l','-d',binary],target+'-'+arch+'-elf')
   r.run([tools+'nm',binary],target+'-'+arch+'-symbols');r.run([tools+'nm','-u',binary],target+'-'+arch+'-undefined')
   if arch=='arm':
    assert not (L/(target+'-'+arch+'-undefined.txt')).read_text().strip()
    r.run([prefix+'strip','-o',str(binary)+'.stripped',binary],target+'-'+arch+'-strip')
   symbols=(L/(target+'-'+arch+'-symbols.txt')).read_text()
   assert 'cw_test_' not in symbols and 'cp_test_' not in symbols
   assert b'PUBLIC-DUMMY-NOT-A-PRODUCT-PASSWORD' not in binary.read_bytes() and b'PUBLICDUMMY-HOST-ONLY' not in binary.read_bytes()
 host_objects=compiled_arch['host']
 for kind in [1,2,3,4]:
  obj=D/('host-fault-'+str(kind)+'.o')
  r.run(['cc',*common,'-DWEB_FAULT_KIND='+str(kind),'-Wall','-Wextra','-Werror','-c',I/'service_fault_test.c','-o',obj],'host-fault-'+str(kind)+'-compile')
  r.run(['cc',*common,*[host_objects[s] for s in service],obj,D/'libmbed-host.a',*wraps,'-Wl,--wrap=getrandom',
   '-Wl,--wrap=clock_gettime','-Wl,--gc-sections','-Wl,-z,noexecstack','-o',D/('web-service-host-fault'+str(kind))],'host-fault-'+str(kind)+'-link')
 # Source-only HOST regression model: reconstruct the two OLD availability
 # behaviours, not an old ELF. Canonical source input + exact reviewed test delta;
 # no historical work source/cache/binary dependency or lower KDF/test match.
 baseline=(I/'web_service.c').read_text()
 start=baseline.index(' if(app.sessions[i].principal.snapshot_serial){')
 end=baseline.index(' wipe(&app.sessions[i],sizeof(app.sessions[i]));',start)
 baseline=baseline[:start]+''' if(app.sessions[i].principal.snapshot_serial)
  (void)camera_access_revoke_session(&app.policy,&app.sessions[i].principal);
'''+baseline[end:]
 start=baseline.index('static void cancel_pending(void)\n{');end=baseline.index('static void handler(',start)
 baseline=baseline[:start]+'''static void cancel_pending(void)
{
 if(!app.pending.active)return;
 (void)cp_cancel(app.child,&app.pending.ticket);
 clear_session(app.pending.session);wipe(&app.pending,sizeof(app.pending));fatal=1;
}
'''+baseline[end:]
 old=I/'baseline_semantics.c';old.write_text('/* HOST ONLY canonical-source old-semantics model, NOT original service ELF. */\n'+baseline)
 obj=D/'baseline-semantics.o';r.run(['cc',*common,*mflags,'-DWEB_PUBLIC_HOST_TEST','-Wall','-Wextra','-Werror','-c',old,'-o',obj],'baseline-semantics-compile')
 r.run(['cc',*common,*[obj if s.name=='web_service.c' else host_objects[s] for s in service],D/'libmbed-host.a',*wraps,
  '-Wl,--gc-sections','-Wl,-z,noexecstack','-Wl,--trace','-o',D/'web-service-host-baseline-semantics'],'baseline-semantics-link')
 m['baseline_model']={'kind':'canonical-source HOST old availability semantics; NOT original ELF','source_sha256':digest(old),'deltas':['policy-only logout leaving issued slots occupied','destructive cp_cancel + service fatal for peer abort'],'launch_ABI':2}
 for name,sources in [
  ('media-admission-test',[I/'media_admission_test.c',I/'media_admission.c',I/'credential/reviewed-contracts/access_policy.c']),
  ('parent-sandbox-test',[I/'parent_sandbox_test.c',I/'parent_sandbox.c']),
  ('credential-availability-test',[I/'credential_availability_test.c',I/'parent_sandbox.c',I/'credential/credential_process.c',
   I/'credential/credential_ipc.c',I/'credential/credential_mbedtls.c',I/'credential/reviewed-contracts/access_policy.c',D/'libmbed-host.a'])]:
  r.run(['cc',*common,'-Wall','-Wextra','-Werror','-Wpedantic',*sources,'-Wl,--gc-sections','-o',D/name],name+'-link')
  r.run([D/name,*([D/'credential-child-host'] if name=='credential-availability-test' else [])],name,15)
 m['libraries']={arch:{'path':str(D/('libmbed-'+arch+'.a')),'sha256':digest(D/('libmbed-'+arch+'.a'))} for arch in ['host','arm']}
 m['state']='linked-host-and-static-ARM-service-NOT-native-qualified'
except BaseException:m['state']='failed';m['failure']=traceback.format_exc();print(m['failure'])
finally:r.finish();print(m['state'])
if m['state']=='failed':raise SystemExit(1)
