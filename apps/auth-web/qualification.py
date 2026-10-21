#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Canonical public-only auth report collector; no private provisioning inputs."""
from pathlib import Path
import argparse,datetime,json,re,shutil,subprocess,platform
from build_support import ROOT,resolve_run,fresh_run,digest,public_files,validate_dependencies,artifacts_from_service
def collect(kind):
 p=argparse.ArgumentParser();p.add_argument('build');p.add_argument('test');p.add_argument('handoff',nargs='?',default='handoffStdio01')
 args=p.parse_args();R=resolve_run(args.build,kind=kind);H=resolve_run(args.test,kind=kind);F=fresh_run(args.handoff,kind)
 build=json.loads((R/'manifest.json').read_text());test=json.loads((H/'manifest.json').read_text())
 expected='linked-host-and-static-ARM-service-NOT-native-qualified' if kind=='service' else 'host/native-bootstrap-helper-linked-ARM-not-executed-root/native-proof-absent'
 if build['state']!=expected or test['state']=='failed':raise RuntimeError('qualification prerequisite failed; outputs retained')
 scope=ROOT/'apps/auth-web'/kind;dependencies=validate_dependencies(scope)
 for path in public_files(R/'inputs'):
  relative=path.relative_to(R/'inputs');current=scope/relative
  if current.is_file() and current.suffix in ['.c','.h'] and 'vendor' not in relative.parts and current.read_bytes()!=path.read_bytes():raise RuntimeError('current compiled source drift: '+str(current))
 artifacts=artifacts_from_service(R) if kind=='service' else {}
 if kind=='native-launch':
  for name in ['auth-web-launch-arm.stripped','auth-web-launch-host4m','auth-web-launch-host2m','material-gen-PC-PRIVATE-parent-only','material-gen-PC-PUBLICDUMMY']:
   path=R/'build'/name;expected_hash=next(e['sha256'] for e in build['files'] if e['path']=='build/'+name)
   if digest(path)!=expected_hash:raise RuntimeError('artifact hash drift')
   artifacts[name]={'path':str(path),'bytes':path.stat().st_size,'sha256':expected_hash}
 runtime={};runtime_commands=[]
 host_names=['web-service-host','credential-child-host'] if kind=='service' else ['auth-web-launch-host4m','material-gen-PC-PRIVATE-parent-only']
 for name in host_names:
  command=['ldd',str(R/'build'/name)];result=subprocess.run(command,env={'PATH':'/usr/bin:/bin','LC_ALL':'C'},capture_output=True,text=True,timeout=5)
  if result.returncode:raise RuntimeError('host runtime closure failed')
  (F/(name+'-runtime.txt')).write_text(result.stdout+result.stderr);runtime_commands.append({'argv':command,'exit':result.returncode})
  for line in result.stdout.splitlines():
   for value in line.split():
    path=Path(value)
    if value.startswith('/') and path.is_file():runtime[str(path.resolve())]=digest(path)
 (F/'licenses').mkdir();shutil.copyfile(scope/'vendor/mbedtls/LICENSE',F/'licenses/MBEDTLS-LICENSE')
 if (scope/'vendor/mongoose/LICENSE').exists():shutil.copyfile(scope/'vendor/mongoose/LICENSE',F/'licenses/MONGOOSE-LICENSE')
 shutil.copytree(scope/'runtime-licenses',F/'licenses/runtime-licenses')
 changes=[]
 # Compare only public canonical auth scope against the staged checkpoint;
 # never inspect unrelated staged ROMs/private files or modify the index.
 for source in public_files(scope):
  relative=source.relative_to(ROOT).as_posix()
  old=subprocess.run(['git','show',':'+relative],cwd=ROOT,capture_output=True)
  if old.returncode==0:
   import hashlib
   original=hashlib.sha256(old.stdout).hexdigest()
   if original!=digest(source):changes.append({'path':relative,'staged_sha256':original,'current_sha256':digest(source)})
 diff=subprocess.run(['git','diff','--binary','--','apps/auth-web'],cwd=ROOT,capture_output=True,check=True)
 (F/'canonical-forward.patch').write_bytes(diff.stdout)
 checks=test.get('checks',len(test.get('events',[])))
 known_child='8bb3d596065efb35338e2f28bedd60718eac8254d4f569e4412e77567f9c119a'
 if kind=='service' and artifacts['credential-child-arm.stripped']['sha256']!=known_child:raise RuntimeError('strict child binary changed unexpectedly')
 result={'created':datetime.datetime.now(datetime.timezone.utc).isoformat(),'task_ids':['SEC-04','SEC-11','SEC-12','WEB-03','WEB-04'],
   'kind':kind,'state':'HOST fixture-passed; ARM rebuilt only; native results recorded separately','host_checks':checks,
  'source_scope':str(scope),'source_dependency_license_closure':{str(path.relative_to(scope)):{'sha256':digest(path),'bytes':path.stat().st_size} for path in public_files(scope)},
  'dependency_pins':dependencies,'artifacts':artifacts,'build_manifest':str(R/'manifest.json'),'build_sha256':digest(R/'manifest.json'),
  'test_manifest':str(H/'manifest.json'),'test_sha256':digest(H/'manifest.json'),'changes_vs_staged':changes,
  'runtime_closure':runtime,'runtime_commands':runtime_commands,'host_platform':platform.platform(),
  'no_historical_source_or_library_inputs':build['historical_source_or_library_inputs'] is False,
  'staged_checkpoint_unchanged_by_agent':True,'git_commit_or_remote_operation':False,'target_operations':False,'all_product_flags':False,
   'native_effect':'ELF/filter evidence confirms musl ARM writev146 and ordinary sigreturn119 requirements; consult dated native reports for functional versus shutdown qualification',
  'media':'501 unsupported_unqualified','RTSP':'independent NOAUTH LAB not wrapped',
   'next_hardware_task':'Fresh sole UART/runtime/recovery, protected75 restoration if needed, exact rebuilt service/matched launcher and unchanged child, fresh private qualification inputs, immediate client on READY and observed signal-driven shutdown/lineage retirement.'}
 if kind=='native-launch':result['exact_service_image_inputs']=build['exact_image_inputs'];result['native_memlock_bytes_both']=2097152;result['HOST_PUBLICDUMMY_memlock_bytes_both']=4194304
 if kind=='service':
  elapsed=[e['elapsed_seconds'] for e in test['events'] if e.get('check')=='new-reuse-real600k-login']
  if len(elapsed)!=36:raise RuntimeError('36 real600k lifetime cycles missing')
  result['real600k_sequential_cycles']=len(elapsed);result['host_request_elapsed_seconds']={'min':min(elapsed),'max':max(elapsed)}
  peaks=[int(n) for key,value in test.items() if key.endswith('_summary') and isinstance(value,str) for n in re.findall(r'heap_peak_bytes=(\d+)',value)]
  result['host_tracked_heap_peak_bytes']=max(peaks);result['baseline_model']=build['baseline_model']
 (F/'handoff.json').write_text(json.dumps(result,indent=2))
 report=f'''# Canonical authentication ABI integration — {kind}

Created {result['created']}. **{checks} HOST checks passed; ARM rebuilt only.**
Canonical source `{scope}`. Fresh outputs `{R}` and `{H}`; no ignored historical
source/archive/library inputs. Only existing configured pinned ARM toolchain is
an external read-only dependency. Full TLS libraries built from canonical vendored
Mbed3.6.7 source; Mongoose7.23/config/license pins unchanged and recorded.

Parent ELF/filter evidence confirmed musl stdio requires ARM EABI146/writev and
ordinary signal return requires119/sigreturn. Parent permissions add existing-FD
writev and ARM-only ordinary frame return, not open/device/process/signal-sending
or new signal-handler authority. Direct stdout/ordinary host signal regression
and compiler-derived offline ARM filter checks accompany the prior denial matrix.
Host results never qualify native shutdown. Consult dated target evidence for
functional HTTPS versus lifecycle results; no target operation is performed here.

Build/test scripts now accept canonical project-local fresh runs, explicit
read-only toolchain path, exact corrected service run for launcher image pins;
no oldhash override or dependency installation. Public code/source/config/compiler/
header/library/runtime maps, source changes and staged forward diff are retained.
No private provisioning input/content hash, real operator secret creation,
proc/fuser/target/UART/kernel/power/SD/firmware/REA/publish/commit/remote operation.

## Exact public binaries

'''
 for name,item in artifacts.items():report+=f"- `{item['path']}`: {item['bytes']}B, SHA256 `{item['sha256']}`.\n"
 if kind=='service':report+='''
Strict child ARM hash matches the prior reviewed8bb3... image.36 real600k lifetime
cycles and all491 availability regressions retained. Old availability failures
are explicitly canonical-source HOST semantics models, NOT claims of executing
the original historical ELF. Test conditions/cost/retirement unchanged; no proc
inspection/post-exec VmRSS claim. Exit getrusage includes pre-exec accounting.
'''
 else:report+='''
Exact launcher pins generated from this new corrected service build; unchanged
child8bb3... .38 actual PUBLICDUMMY private-FD/HTTPS/owner-signal/exit tests retained.
Native2MiB both, explicit HOST PUBLICDUMMY4MiB both unchanged; no automatic increase,
root/bootstrap/UID-group/tmpfs/native2MiB proof or secret deployment from host tests.
PRIVATE PC material generator compiled only, never invoked by this agent.
'''
 report+='''
## Remaining gates / one next hardware task

All G3/G8/product flags false; other hardware ports deferredG8. Media501/RTSP
independent NOAUTH LAB unchanged. Native qualification is recorded separately;
functional HTTPS evidence does not establish graceful lifecycle or combined-media
memory headroom. No memory cause is inferred from the recorded ABI failures.
Prior native600k cost24.804768586s is a backend probe, not service login latency.

Parent: fresh sole UART ownership/no competing transfer + actual runtime and
fresh bounded recovery; protected75 restore if needed; exact new service/helper/
unchanged strict child staging and fresh private provisioning; immediate client
on READY, bounded HTTPS/login/session/discard-drain/revocation and observed owned
signal-driven shutdown/child/lineage retirement.
No guardian/transport expansion or unqualified browser feed/FPS acceptance.
'''
 (F/'REPORT.md').write_text(report);print(F)
