#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Canonical launcher build pinned to an exact successful canonical service run."""
from pathlib import Path
import argparse,json,shutil,sys,traceback
A=Path(__file__).resolve().parent;sys.path.insert(0,str(A.parent))
from build_support import Run,fresh_run,resolve_run,copy_scope,validate_dependencies,toolchain,digest,artifacts_from_service,public_files
p=argparse.ArgumentParser();p.add_argument('run');p.add_argument('--service-build',required=True,type=Path)
p.add_argument('--output-root',type=Path);p.add_argument('--toolchain',type=Path)
args=p.parse_args();R=fresh_run(args.run,'native-launch',args.output_root);r=Run(R,'canonical public launcher HOST/PUBLICDUMMY + ARM link only')
I=r.inputs;D=r.build;L=r.logs;m=r.manifest
try:
 m['canonical_dependencies']=validate_dependencies(A);copy_scope(A,I)
 service_root=resolve_run(args.service_build);source_manifest=json.loads((service_root/'manifest.json').read_text())
 artifacts=artifacts_from_service(service_root)
 if source_manifest.get('historical_source_or_library_inputs') is not False:raise RuntimeError('service source provenance not canonical')
 service_vendor=service_root/'inputs/vendor/mbedtls'
 if not service_vendor.is_dir():
  # UI builds consume the pinned canonical vendor tree read-only. Do not add
  # files to a sealed build or accept a manifest-selected arbitrary vendor path.
  canonical_service=A.parent/'service'
  ui=source_manifest.get('ui_candidate',{})
  if (source_manifest.get('source_scope')!=str(canonical_service) or
      ui.get('vendor_input')!='pinned canonical read-only trees, not duplicated into candidate source' or
      ui.get('canonical_inputs_verified') is not True or
      validate_dependencies(canonical_service)!=source_manifest.get('canonical_dependencies')):
   raise RuntimeError('service vendor provenance not canonical')
  service_vendor=canonical_service/'vendor/mbedtls'
 m['verified_service_vendor_scope']=str(service_vendor)
 for path in public_files(I/'vendor/mbedtls'):
  if path.read_bytes()!=(service_vendor/path.relative_to(I/'vendor/mbedtls')).read_bytes():raise RuntimeError('service/launcher full Mbed source mismatch')
 for name in ['tls12_config.h','tls13_config.h','web_service.h']:
  if (I/name).read_bytes()!=(service_root/'inputs'/name).read_bytes():raise RuntimeError('source ABI/config mismatch: '+name)
 for label,sha in source_manifest['full_consumed_header_source_closure'].items():
  if digest(Path(label))!=sha:raise RuntimeError('compiled service header/source closure changed: '+label)
 for label,sha in source_manifest.get('compiler_runtime_closure',{}).items():
  if digest(Path(label))!=sha:raise RuntimeError('compiled service compiler/runtime changed: '+label)
 for arch in ['host','arm']:
  entry=source_manifest['libraries'][arch];source=service_root/'build'/('libmbed-'+arch+'.a')
  if digest(source)!=entry['sha256']:raise RuntimeError('full selected service library pin mismatch: '+arch)
  shutil.copyfile(source,D/source.name)
 m['service_build']={'path':str(service_root),'manifest_sha256':digest(service_root/'manifest.json')}
 m['exact_image_inputs']=artifacts;m['historical_source_or_library_inputs']=False
 # Pins derive ONLY from this exact successful corrected build, no oldhash
 # override or permissive ABI fallback. Headers stay public build outputs.
 fmt=lambda value:'{'+','.join(str(n) for n in bytes.fromhex(value))+'}'
 for arch in ['arm','host']:
  service=artifacts['web-service-arm.stripped' if arch=='arm' else 'web-service-host']
  child=artifacts['credential-child-arm.stripped' if arch=='arm' else 'credential-child-host']
  path=I/('pins-'+arch);path.mkdir()
  (path/'image_pins.h').write_text('/* SPDX-License-Identifier: GPL-2.0-only; PUBLIC exact canonical image pins */\n'
   '#define SERVICE_BYTES '+str(service['bytes'])+'U\n#define CHILD_BYTES '+str(child['bytes'])+'U\n'
   'static const unsigned char service_pin[32]='+fmt(service['sha256'])+';\n'
   'static const unsigned char child_pin[32]='+fmt(child['sha256'])+';\n')
 TC,prefix,pins=toolchain(args.toolchain);m['verified_toolchain_components']=pins
 common=['-std=gnu11','-Os','-fstack-protector-strong','-D_FORTIFY_SOURCE=2','-ffunction-sections','-fdata-sections','-fno-strict-aliasing',
  '-Wall','-Wextra','-Werror','-Wpedantic','-I'+str(I),'-I'+str(I/'vendor/mbedtls/include'),'-DMBEDTLS_CONFIG_FILE="'+str(I/'tls13_config.h')+'"']
 for target,cc,flags,library in [
  ('arm',prefix+'gcc',['-march=armv5te','-marm','-mfloat-abi=soft','-static','-I'+str(I/'pins-arm')],D/'libmbed-arm.a'),
  ('host4m','cc',['-DLAUNCH_HOST_PUBLIC_ONLY','-DLAUNCH_MEMLOCK=4194304','-I'+str(I/'pins-host')],D/'libmbed-host.a'),
  ('host2m','cc',['-DLAUNCH_HOST_PUBLIC_ONLY','-I'+str(I/'pins-host')],D/'libmbed-host.a')]:
  binary=D/('auth-web-launch-'+target);r.compiler_closure(cc,target)
  r.run([cc,*common,*flags,'-fstack-usage','-MD','-MF',D/(target+'.d'),I/'launch.c',library,
   '-Wl,--trace','-Wl,--gc-sections','-Wl,-z,noexecstack','-Wl,-Map='+str(binary)+'.map','-o',binary],target+'-link')
  tools=prefix if target=='arm' else ''
  r.run([tools+'size',binary],target+'-size');r.run([tools+'readelf','-h','-A','-l','-d',binary],target+'-elf');r.run([tools+'nm','-u',binary],target+'-undefined')
  if target=='arm':
   assert not (L/(target+'-undefined.txt')).read_text().strip()
   r.run([prefix+'strip','-o',str(binary)+'.stripped',binary],target+'-strip')
 for fixture in [False,True]:
  name='material-gen-PC-'+('PUBLICDUMMY' if fixture else 'PRIVATE-parent-only')
  r.run(['cc',*common,*(['-DMATERIAL_PUBLIC_HOST_FIXTURE'] if fixture else []),'-MD','-MF',D/(name+'.d'),I/'material_gen.c',D/'libmbed-host.a',
   '-Wl,--trace','-Wl,--gc-sections','-Wl,-z,noexecstack','-o',D/name],name+'-link')
 m['state']='host/native-bootstrap-helper-linked-ARM-not-executed-root/native-proof-absent'
except BaseException:m['state']='failed';m['failure']=traceback.format_exc();print(m['failure'])
finally:r.finish();print(m['state'])
if m['state']=='failed':raise SystemExit(1)
