#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Offline actual ARM-profile filter regression; no ARM binary execution."""
from pathlib import Path
import argparse,resource,shutil,sys
S=Path(__file__).resolve().parent
sys.path.insert(0,str(S.parent))
from build_support import Run,fresh_run,toolchain,digest
p=argparse.ArgumentParser();p.add_argument('run');p.add_argument('--toolchain',type=Path)
p.add_argument('--expect-denied',action='store_true',help='preserve pre-fix counterexample')
a=p.parse_args();R=fresh_run(a.run,'signal-abi');r=Run(R,'actual ARM BPF interpreted offline; no target, secrets or firmware execution')
resource.setrlimit(resource.RLIMIT_CORE,(0,0))
try:
 _,prefix,pins=toolchain(a.toolchain);r.manifest['verified_toolchain_components']=pins
 for name in ['web_service.h','parent_sandbox.c','parent_sandbox_arm_filter_test.c']:
  (r.inputs/name).write_bytes((S/name).read_bytes())
 shutil.copytree(S/'credential/reviewed-contracts/camera',r.inputs/'camera')
 def macros(cc,label):
  r.run([cc,'-dM','-E','-include','sys/syscall.h','-x','c','/dev/null'],label)
  return {line.split()[1]:line.split(maxsplit=2)[2] for line in (r.logs/(label+'.txt')).read_text().splitlines()
          if line.startswith('#define ') and len(line.split())>=3}
 arm=macros(prefix+'gcc','ARM-syscall-macros');host=macros('cc','HOST-syscall-macros')
 # Both SYS aliases and their __NR dependencies are numeric target header macros.
 # Undefine the union so no host-only alias survives into the ARM profile.
 keys={k for k in arm.keys()|host.keys() if k.startswith(('SYS_','__NR_'))}
 overlay='/* PUBLIC compiler-derived ARM ABI, not an installed host filter */\n'
 overlay+=''.join('#undef '+k+'\n' for k in sorted(keys))
 overlay+=''.join('#define '+k+' '+arm[k]+'\n' for k in sorted(keys&arm.keys()))
 (r.inputs/'arm_syscall_overlay.h').write_text(overlay)
 r.run(['cc','-std=gnu11','-Wall','-Wextra','-Werror','-Wpedantic','-I'+str(r.inputs),
        r.inputs/'parent_sandbox_arm_filter_test.c','-o',r.build/'ARM-filter-interpreter'],'build-interpreter')
 r.run([r.build/'ARM-filter-interpreter'],'desired-signalreturn-test',expected_exit=-6 if a.expect_denied else 0)
 r.manifest['state']='pre-fix-desired-ABI-counterexample' if a.expect_denied else 'ARM-filter-ABI-regression-PASS-not-native-proof'
 r.manifest['source_hashes']={n:digest(S/n) for n in ['parent_sandbox.c','parent_sandbox_arm_filter_test.c']}
finally:r.finish()
print(r.manifest['state'])
