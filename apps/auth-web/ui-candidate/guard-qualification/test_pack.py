#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""HOST payload/parser checks only; never source target/proc/device/other jobs."""
from pathlib import Path
import datetime,hashlib,json,os,re,resource,signal,subprocess,sys,time,unittest
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3]
sys.path.insert(0,str(HERE));from parser import Parser,ParseError
if len(sys.argv)!=2:raise SystemExit('fresh built run name required')
R=ROOT/'evidence/delegated/20261012-native-ui/guard-qualification'/sys.argv.pop(1)
build=json.loads((R/'build-manifest.json').read_text());assert build['state']=='HOST-optimized-sanitized-and-static-ARM-built-NOT-native'
T=R/'host-tests';T.mkdir();events=[]
def digest(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def actual(name,arguments=(),cancel=False,fd3=False):
 p=R/'build'/name;assert digest(p)==build['artifacts']['build/'+name]['sha256']
 env={'PATH':'/usr/bin:/bin','LC_ALL':'C','ASAN_OPTIONS':'detect_leaks=0:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'}
 before=resource.getrusage(resource.RUSAGE_CHILDREN);start=time.monotonic()
 def setup():
  if fd3:os.dup2(fd3,3,inheritable=True)
 proc=subprocess.Popen([str(p),*arguments],stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.PIPE,
                       env=env,close_fds=not bool(fd3),preexec_fn=setup if fd3 else None)
 pidfd=os.pidfd_open(proc.pid,0)
 try:
  if cancel:
   ready=proc.stdout.readline();assert ready.startswith(b'GUARDQUAL_HELPER_READY ')
   signal.pidfd_send_signal(pidfd,signal.SIGTERM)
   tail,err=proc.communicate(timeout=3);out=ready+tail
  else:out,err=proc.communicate(timeout=10)
 except BaseException:
  try:signal.pidfd_send_signal(pidfd,signal.SIGKILL)
  except ProcessLookupError:pass
  proc.wait(timeout=2);raise
 finally:os.close(pidfd)
 after=resource.getrusage(resource.RUSAGE_CHILDREN);label=name+('-cancel' if cancel else '-args' if arguments else '-FD3' if fd3 else '')
 (T/(label+'.out')).write_bytes(out);(T/(label+'.err')).write_bytes(err)
 events.append({'label':label,'argv':[str(p),*arguments],'exit':proc.returncode,'elapsed_seconds':time.monotonic()-start,
  'child_cpu_seconds_delta':after.ru_utime+after.ru_stime-before.ru_utime-before.ru_stime,'child_peak_RSS_KiB_cumulative':after.ru_maxrss,
  'RSS_caveat':'HOST cumulative getrusage including inherited pre-exec/sanitizer; not native locked/resident fit',
  'owned_pidfd':True,'stderr_bytes':len(err)})
 return proc.returncode,out,err

class Pack(unittest.TestCase):
 def test_optimized_actual_seven_seconds(self):
  code,out,err=actual('guardqual-host');q=Parser();q.feed(out);self.assertTrue(q.normal(code));self.assertEqual(err,b'')
 def test_sanitized_actual_seven_seconds(self):
  code,out,err=actual('guardqual-sanitized');q=Parser();q.feed(out);self.assertTrue(q.normal(code));self.assertEqual(err,b'')
 def test_owned_pidfd_SIGTERM_early_exit20(self):
  code,out,err=actual('guardqual-host',cancel=True);q=Parser();q.feed(out);self.assertTrue(q.cancelled(code));self.assertLess(q.last_elapsed,7000000000);self.assertEqual(err,b'')
 def test_native_and_HOST_arbitrary_arguments_rejected(self):
  code,out,err=actual('guardqual-host',['PUBLICDUMMY']);self.assertEqual(code,21);q=Parser();q.feed(out);self.assertFalse(q.normal(code));self.assertNotIn('ready',q.records)
  source=(R/'inputs/helper.c').read_text();self.assertNotIn('getenv',source);self.assertIn('if(argc!=1)',source)
 def test_FD3_must_be_closed(self):
  file=T/'PUBLICDUMMY-open-FD3';fd=os.open(file,os.O_RDWR|os.O_CREAT|os.O_EXCL,0o600)
  try:code,out,err=actual('guardqual-host',fd3=fd);self.assertEqual(code,22);self.assertEqual(file.read_bytes(),b'')
  finally:os.close(fd)
 def test_clock_failure_and_backward_failclosed(self):
  for kind,expected in [(1,24),(2,25)]:
   code,out,err=actual('guardqual-host-clock'+str(kind));self.assertEqual(code,expected);q=Parser();q.feed(out);self.assertFalse(q.normal(code));self.assertIn('error',q.records)
 def test_every_chunk_split_and_echo_not_native_success(self):
  data=b'# GUARDQUAL_HELPER_READY version=1 lifetime_ns=7000000000 start_ns=100\r\nGUARDQUAL_HELPER_LIFETIME version=1 elapsed_ns=4000000000\r\nGUARDQUAL_HELPER_EXIT version=1 status=0 elapsed_ns=7000000000\r\n'
  for n in range(len(data)+1):
   q=Parser();q.feed(data[:n]);q.feed(data[n:]);self.assertTrue(q.normal(0))
  q=Parser();q.feed(b'echo GUARDQUAL_HELPER_EXIT version=1 status=0 elapsed_ns=7000000000\n');self.assertFalse(q.normal(0))
 def test_duplicates_wrongexit_and_partial_records_not_success(self):
  line=b'GUARDQUAL_HELPER_READY version=1 lifetime_ns=7000000000 start_ns=100\n'
  q=Parser();q.feed(line)
  with self.assertRaises(ParseError):q.feed(line)
  q=Parser();q.feed(line+b'GUARDQUAL_HELPER_EXIT version=1 status=0 elapsed_ns=1');self.assertFalse(q.normal(0))
 def test_script_fixed_emptyargs_pin_path_and_guard_only(self):
  text=(R/'guardqual-native01-public-command.txt').read_text();self.assertIn('(exec 3>&-; exec /run/guardqual-native01.elf)',text)
  self.assertEqual(text.count('/dev/watchdog0'),1);self.assertIn('sleep 4;',text);self.assertIn('45; do sleep 4;',text)
  self.assertIn('GUARDQUAL01_BOOT_PID',text);self.assertIn('guard_exec_seen',text);self.assertNotIn('auth-web',text)
 def test_no_file_device_radio_auth_or_wdogopen_in_payload(self):
  text=(R/'inputs/helper.c').read_text()
  code=re.sub(r'/\*.*?\*/','',text,flags=re.S)
  for word in ['open(','socket(','ioctl(','getenv(','/dev/','/proc/','operator.password']:self.assertNotIn(word,code)
  self.assertIn('FD3_not_closed',text);self.assertIn('SYS_capset',text);self.assertIn('PR_SET_DUMPABLE',text)

start=time.monotonic();suite=unittest.defaultTestLoader.loadTestsFromTestCase(Pack)
with (T/'tests.txt').open('x') as log:result=unittest.TextTestRunner(stream=log,verbosity=2).run(suite)
(T/'manifest.json').write_text(json.dumps({'created':datetime.datetime.now(datetime.timezone.utc).isoformat(),'tests':result.testsRun,'pass':result.wasSuccessful(),
 'failures':len(result.failures),'errors':len(result.errors),'events':events,'elapsed_seconds':time.monotonic()-start,
 'source_pins':{p.name:digest(p) for p in HERE.iterdir() if p.is_file()},'qualified_scope':'HOST optimized/sanitized/owned cancellation/parser/source only',
 'C++header_API':'N/A no exported API/header','native_execution':False,'G3_G8_security_UI_fps_trust_rootisolation_passed':False},indent=2))
print(json.dumps({'tests':result.testsRun,'pass':result.wasSuccessful(),'output':str(T)}));raise SystemExit(0 if result.wasSuccessful() else 1)
