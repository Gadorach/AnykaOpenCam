#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Public-only candidate handoff, without Git/proc/private-fixture enumeration."""
from pathlib import Path
import datetime,json,re,shutil,subprocess,sys
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];SERVICE=ROOT/'apps/auth-web/service'
sys.path.insert(0,str(HERE));from build import owned_output
sys.path.insert(0,str(SERVICE.parent));from build_support import digest,artifacts_from_service
if len(sys.argv)!=4:raise ValueError('build,491 campaign,asset TLS campaign required')
B=owned_output(sys.argv[1]);H=owned_output(sys.argv[2]);T=owned_output(sys.argv[3])
E=ROOT/'evidence/delegated/20261012-native-ui';E.mkdir(exist_ok=True)
out=E/'handoff01';out.mkdir()
build=json.loads((B/'manifest.json').read_text());availability=json.loads((H/'manifest.json').read_text());assets=json.loads((T/'manifest.json').read_text())
snapshot=json.loads((HERE/'snapshot/manifest.json').read_text())
if build['state']!='linked-host-and-static-ARM-service-NOT-native-qualified':raise RuntimeError('build not passed')
if availability['state']=='failed' or availability['checks']!=491 or any(not e['pass'] for e in availability['events']):raise RuntimeError('full491 prerequisite not passed')
if assets['state']!='real-TLS-v3-assets-API501-PASS-HOST-only' or assets['checks']!=127 or any(not e['pass'] for e in assets['events']):raise RuntimeError('assets prerequisite not passed')
images=artifacts_from_service(B)
if images['credential-child-arm.stripped']['sha256']!='8bb3d596065efb35338e2f28bedd60718eac8254d4f569e4412e77567f9c119a':raise RuntimeError('strict child byte drift')
for name,pin in snapshot['canonical_inputs'].items():
 if digest(SERVICE/name)!=pin['sha256']:raise RuntimeError('canonical read-only input drift '+name)
for row in snapshot['assets']:
 if digest(ROOT/'apps/web-media/player'/row['name'])!=row['sha256']:raise RuntimeError('player source drift')
for name,pin in snapshot['generated'].items():
 if digest(HERE/'snapshot/generated'/name)!=pin:raise RuntimeError('generated source drift')
runtime={};runtime_commands=[]
for name in ['web-service-host','credential-child-host']:
 command=['ldd',str(B/'build'/name)]
 p=subprocess.run(command,capture_output=True,text=True,timeout=5,env={'PATH':'/usr/bin:/bin','LC_ALL':'C'})
 if p.returncode:raise RuntimeError('HOST runtime closure failed')
 (out/(name+'-runtime.txt')).write_text(p.stdout+p.stderr);runtime_commands.append({'argv':command,'exit':p.returncode})
 for line in p.stdout.splitlines():
  for value in line.split():
   if value.startswith('/') and Path(value).is_file():runtime[str(Path(value).resolve())]=digest(value)
for name,source in [('build-manifest.json',B/'manifest.json'),('availability491.json',H/'manifest.json'),('assets127.json',T/'manifest.json'),
 ('source-snapshot.json',HERE/'snapshot/manifest.json'),('canonical-to-ui.patch',HERE/'snapshot/generated/canonical-to-ui.patch')]:shutil.copyfile(source,out/name)
shutil.copytree(HERE/'snapshot/licenses',out/'licenses');shutil.copytree(HERE/'snapshot/canonical-service/runtime-licenses',out/'licenses/runtime-licenses')
metrics=[]
for test in [availability,assets]:
 for key,value in test.items():
  if not key.endswith('_summary') or not isinstance(value,str):continue
  record={'campaign':key,'heap_peak_bytes':int(re.search(r'heap_peak_bytes=(\d+)',value)[1])}
  for field in ['self_peak_rss_kb','child_peak_rss_kb','self_cpu_us','child_cpu_us','peers_peak','handshake_peak','login_success','requests']:
   match=re.search(r'\b'+field+r'=(\d+)',value)
   if match:record[field]=int(match[1])
  metrics.append(record)
elapsed=[e['elapsed_seconds'] for e in availability['events'] if e['check']=='new-reuse-real600k-login']
if len(elapsed)!=36:raise RuntimeError('real600k36-cycle proof absent')
launcher={'service_run':str(B),'service_arm':images['web-service-arm.stripped'],'child_arm':images['credential-child-arm.stripped'],
 'matched_launcher':'Parent builds reviewed canonical native-launch with this exact --service-run; no old-hash runtime override',
 'native_memlock_both_bytes':2097152,'native_qualified':False}
(out/'launcher-image-inputs.json').write_text(json.dumps(launcher,indent=2)+'\n')
result={'created':datetime.datetime.now(datetime.timezone.utc).isoformat(),'tasks':['WEB-03','WEB-05','SEC-04-regression'],
 'state':'HOST realTLS/full600k fixture-passed; HOST+ARM built; parent adoption/native acceptance pending',
 'images':images,'build_command_receipts':str(out/'build-manifest.json'),'build_manifest_sha256':digest(B/'manifest.json'),
 'source_snapshot_sha256':digest(HERE/'snapshot/manifest.json'),
 'availability491_sha256':digest(H/'manifest.json'),'assets127_sha256':digest(T/'manifest.json'),
 'source_scripts':{p.name:digest(p) for p in HERE.iterdir() if p.is_file()},
 'historical_source_or_binary_inputs':False,'read_only_vendor_pins':build['canonical_dependencies'],
 'compiler_runtime_header_library_closure':'Full build-manifest.json: GCC/header .d/link --trace/library/tool hashes; HOST runtime records here',
 'runtime':runtime,'runtime_commands':runtime_commands,
 'checks':{'availability':491,'assets':127,'real600k_same-instance_cycles':36},'metrics':metrics,
 'host_real600k36_elapsed_seconds':{'min':min(elapsed),'max':max(elapsed)},
 'host_rlimits':{'availability':availability['host_limits'],'assets':assets['host_limits']},
 'rss_caveat':'Owned exit getrusage includes pre-exec inherited accounting; no proc/VmRSS/native locked-headroom claim',
 'limits':{'HTTP_full_response':8192,'UI_body':7168,'single_allocation':131072,'tracked_heap':1048576,
  'native_MEMLOCK_both':2097152,'private_ABI':2,'config_bytes':768,'KDF_iterations':600000,
  'HTTP_login_seconds':35,'CPU_job_seconds':40,'TLS_peers':2,'concurrent_handshakes':1,'handshake_header_seconds':5,'send_drain_seconds':2},
 'max_actual_UI_wire_bytes':max(e['wire_bytes'] for e in assets['wire_measurements']),
 'code_change_scope':'include + fixed static branch only; canonical APIs/IPC/isolation/filter/worker byte-identical',
 'strict_child_ARM_byte_identical':True,'original_sources_and_ELFs_modified':False,'parent_tab_process_or_hardware_access':False,
 'browser_parallel_module_graph':'Not tested here; canonical two-peer/one-handshake admission is unchanged and requires actual browser qualification',
 'native_warm_HTTPS_acceptance_transferred':False,'all_G3_G8_product_flags':False,'media':'501 unsupported_unqualified','RTSP':'independent NOAUTH LAB',
 'next_parent_native_step':'Fresh protected exact candidate/matched-launcher2MiB native HTTPS UI campaign: all fixed assets/browser login/status/logout/replay and observed owned service/child exit; media501 retained.'}
(out/'REPORT.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps({'handoff':str(out),'checks':result['checks'],'max_wire':result['max_actual_UI_wire_bytes'],'max_tracked_heap':max(e['heap_peak_bytes'] for e in metrics)}))
