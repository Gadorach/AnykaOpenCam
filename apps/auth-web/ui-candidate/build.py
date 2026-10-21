#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Use the pinned canonical HOST/ARM builder, with only UI source/vendor path overlay."""
from pathlib import Path
import json,shutil,sys
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
SERVICE=ROOT/'apps/auth-web/service';SNAP=HERE/'snapshot'
sys.path.insert(0,str(SERVICE.parent))
from build_support import digest
def owned_output(value):
 base=ROOT/'local/build/auth-web/ui-candidate';p=Path(value)
 if len(p.parts)==1:p=base/p
 elif not p.is_absolute():p=ROOT/p
 p=p.resolve()
 if not p.is_relative_to(base) or p==base:raise ValueError('fresh UI-candidate output only')
 return p
def copy_ui_scope(source,destination,manifest):
 pins=json.loads((SNAP/'manifest.json').read_text())
 for rel,pin in pins['canonical_inputs'].items():
  current=source/rel
  if digest(current)!=pin['sha256']:raise RuntimeError('canonical source drift '+rel)
 for p in (SNAP/'canonical-service').rglob('*'):
  if p.is_file():
   dest=destination/p.relative_to(SNAP/'canonical-service');dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,dest)
 for p in (SNAP/'canonical-support').iterdir():shutil.copyfile(p,destination/p.name)
 for name in ['ui_assets.h','web_service.c']:
  p=SNAP/'generated'/name
  if digest(p)!=pins['generated'][name]:raise RuntimeError('generated source drift')
  shutil.copyfile(p,destination/name)
 manifest['ui_candidate']={'snapshot_manifest':str(SNAP/'manifest.json'),'sha256':digest(SNAP/'manifest.json'),
  'canonical_inputs_verified':True,'only_service_changes':'static include/allowlist/reply headers',
  'vendor_input':'pinned canonical read-only trees, not duplicated into candidate source',
  'assets':pins['assets'],'non_static_service_byte_identical':True,'native_qualified':False}
def main():
 if len(sys.argv)<2:raise ValueError('fresh run path and --toolchain required')
 sys.argv[1]=str(owned_output(sys.argv[1]))
 if '--output-root' in sys.argv:raise ValueError('output-root override not permitted in isolated wrapper')
 pins=json.loads((SNAP/'manifest.json').read_text());source=SNAP/'canonical-service/build.py'
 if digest(SERVICE/'build.py')!=pins['canonical_inputs']['build.py']['sha256']:raise RuntimeError('builder drift')
 text=source.read_text()
 changes={'copy_scope(A,I)':'copy_ui_scope(A,I,m)',
  "I/'vendor/mongoose'":"A/'vendor/mongoose'","I/'vendor/mbedtls/include'":"A/'vendor/mbedtls/include'",
  "I/'vendor/mbedtls/library'":"A/'vendor/mbedtls/library'","I/'vendor/mongoose/mongoose.c'":"A/'vendor/mongoose/mongoose.c'"}
 for old,new in changes.items():
  if old not in text:raise RuntimeError('builder overlay marker missing '+old)
  text=text.replace(old,new)
 # Canonical compound source naming now includes external vendor paths.
 text=text.replace('str(s.relative_to(I))','str(s.relative_to(I)) if s.is_relative_to(I) else str(s.relative_to(A))')
 # No hidden argument/environment overrides.
 exec(compile(text,str(SERVICE/'build.py'),'exec'),{'__name__':'__main__','__file__':str(SERVICE/'build.py'),'copy_ui_scope':copy_ui_scope})
if __name__=='__main__':main()
