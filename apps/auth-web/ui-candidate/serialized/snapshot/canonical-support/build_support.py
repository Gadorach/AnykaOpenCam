#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Canonical offline auth build helpers. No historical work source/cache inputs."""
from pathlib import Path
import hashlib,json,os,shlex,shutil,subprocess,datetime,time
ROOT=Path(__file__).resolve().parents[2]
PIN_FILE=Path(__file__).resolve().parent/'dependency-pins.json'
DEFAULT_OUTPUT=ROOT/'local/build/auth-web'
def digest(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def public_files(directory):
 files=[]
 for path in sorted(Path(directory).rglob('*')):
  if path.is_symlink():raise RuntimeError('source/dependency symlink refused: '+str(path))
  if path.is_file() and '__pycache__' not in path.parts:files.append(path)
 return files
def validate_dependencies(scope):
 pins=json.loads(PIN_FILE.read_text());records={}
 for name,record in pins['trees'].items():
  directory=scope/name
  if not directory.exists():continue # native launcher does not depend on Mongoose
  h=hashlib.sha256();files=public_files(directory)
  for p in files:h.update(p.relative_to(directory).as_posix().encode()+b'\0'+bytes.fromhex(digest(p)))
  if h.hexdigest()!=record['sha256'] or len(files)!=record['files']:raise RuntimeError('canonical dependency pin mismatch: '+name)
  records[name]={'sha256':h.hexdigest(),'files':len(files)}
 for name,expected in pins['files'].items():
  if digest(scope/name)!=expected:raise RuntimeError('measured TLS configuration changed: '+name)
 return {'pin_file':str(PIN_FILE),'pin_file_sha256':digest(PIN_FILE),'validated_trees':records,'release_provenance':{k:pins[k] for k in ['mongoose','mbedtls']}}
def resolve_run(value,output_root=None,kind=None):
 p=Path(value).expanduser()
 if not p.is_absolute() and len(p.parts)==1:
  if not p.name.isalnum() or not p.name.startswith(('run','host','handoff')):raise ValueError('fresh alphanumeric run/host/handoff name')
  p=Path(output_root or DEFAULT_OUTPUT)/(kind or 'service')/p
 elif not p.is_absolute():p=ROOT/p
 p=p.resolve()
 if not p.is_relative_to(ROOT/'local'):raise ValueError('auth outputs/build inputs must remain in project local/: '+str(p))
 return p
def fresh_run(value,kind,output_root=None):
 p=resolve_run(value,output_root,kind);p.parent.mkdir(parents=True,exist_ok=True);p.mkdir();return p
def copy_scope(source,destination):
 for p in public_files(source):
  out=destination/p.relative_to(source);out.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,out)
 for p in [Path(__file__),PIN_FILE]:shutil.copyfile(p,destination/p.name)
def toolchain(value):
 value=value or os.environ.get('AUTH_WEB_ARM_TOOLCHAIN')
 if not value:raise ValueError('--toolchain or AUTH_WEB_ARM_TOOLCHAIN required (existing read-only Bootlin ARMv5 musl14.3 toolchain)')
 path=Path(value).expanduser().resolve();prefix=str(path/'bin/arm-buildroot-linux-musleabi-')
 required=[path/'bin/toolchain-wrapper',path/'bin/arm-buildroot-linux-musleabi-gcc.br_real',
  path/'arm-buildroot-linux-musleabi/sysroot/lib/libc.a',path/'lib/gcc/arm-buildroot-linux-musleabi/14.3.0/libgcc.a']
 for p in required:
  if not p.is_file():raise ValueError('missing pinned toolchain component: '+str(p))
 expected=json.loads(PIN_FILE.read_text())['arm_toolchain_files']
 for relative,sha in expected.items():
  if digest(path/relative)!=sha:raise ValueError('pinned Bootlin component mismatch: '+relative)
 return path,prefix,{str(p):digest(p) for p in required}
class Run:
 def __init__(self,path,scope):
  self.path=path;self.inputs=path/'inputs';self.build=path/'build';self.logs=path/'logs'
  for p in [self.inputs,self.build,self.logs,path/'tmp']:p.mkdir()
  self.env={'PATH':'/usr/bin:/bin','LC_ALL':'C','SOURCE_DATE_EPOCH':'1791694800','TMPDIR':str(path/'tmp'),'TMP':str(path/'tmp'),'TEMP':str(path/'tmp')}
  self.manifest={'started':datetime.datetime.now(datetime.timezone.utc).isoformat(),'scope':scope,'environment':self.env,'commands':[],'state':'building'}
 def run(self,args,label,timeout=120,expected_exit=0):
  args=list(map(str,args));entry={'argv':args,'cwd':str(self.path),'log':str(self.logs/(label+'.txt')),'started':datetime.datetime.now(datetime.timezone.utc).isoformat(),'timeout_seconds':timeout,'expected_exit':expected_exit}
  self.manifest['commands'].append(entry);start=time.monotonic_ns()
  try:
   with Path(entry['log']).open('xb') as f:p=subprocess.run(args,cwd=self.path,env=self.env,stdout=f,stderr=subprocess.STDOUT,timeout=timeout)
   entry['exit']=p.returncode
   if p.returncode!=expected_exit:raise RuntimeError(label+' unexpected exit: '+entry['log'])
  except BaseException as e:entry['failure']=type(e).__name__+': '+str(e);raise
  finally:entry['elapsed_ns']=time.monotonic_ns()-start;entry['ended']=datetime.datetime.now(datetime.timezone.utc).isoformat()
 def compiler_closure(self,compiler,label):
  self.run([compiler,'--version'],label+'-compiler-version')
  self.run([compiler,'-dumpmachine'],label+'-compiler-target')
  for item in ['cc1','as','ld','libgcc.a','crt1.o','Scrt1.o','crti.o','crtbeginS.o','crtbeginT.o','crtend.o','crtn.o','libc.a','libc.so','libpthread.a']:
   flag='-print-prog-name=' if item in ['cc1','as','ld'] else '-print-file-name='
   tag=label+'-runtime-'+item.replace('.','_');self.run([compiler,flag+item],tag)
   value=(self.logs/(tag+'.txt')).read_text().strip();p=Path(value) if '/' in value else Path(shutil.which(value) or value)
   if p.is_file():self.manifest.setdefault('compiler_runtime_closure',{})[str(p.resolve())]=digest(p)
 def finish(self):
  closure={};traced={}
  for p in self.build.rglob('*.d'):
   for name in shlex.split(p.read_text().replace('\\\n','').split(':',1)[1]):
    f=Path(name)
    if f.is_file():closure[str(f.resolve())]=digest(f)
  for p in self.logs.glob('*link*.txt'):
   for line in p.read_text(errors='replace').splitlines():
    f=Path(line.strip())
    if line.startswith('/') and f.is_file():traced[str(f.resolve())]=digest(f)
  self.manifest['full_consumed_header_source_closure']=closure
  self.manifest['linker_runtime_input_closure']=traced
  self.manifest['tools']=[{'path':str(Path(shutil.which(t)).resolve()),'sha256':digest(Path(shutil.which(t)).resolve())} for t in ['cc','ld','ar','as','python3','size','readelf','nm']]
  self.manifest['files']=[{'path':str(p.relative_to(self.path)),'bytes':p.stat().st_size,'sha256':digest(p)} for p in public_files(self.path)]
  self.manifest['ended']=datetime.datetime.now(datetime.timezone.utc).isoformat()
  with (self.path/'manifest.json').open('x') as f:json.dump(self.manifest,f,indent=2)
def artifacts_from_service(path):
 path=resolve_run(path);manifest=json.loads((path/'manifest.json').read_text())
 if manifest['state']!='linked-host-and-static-ARM-service-NOT-native-qualified':raise ValueError('unsuccessful service build')
 output={}
 for name in ['web-service-host','credential-child-host','web-service-arm.stripped','credential-child-arm.stripped']:
  p=path/'build'/name;expected=next(e['sha256'] for e in manifest['files'] if e['path']=='build/'+name)
  if digest(p)!=expected:raise ValueError('exact service image pin mismatch: '+name)
  output[name]={'path':str(p),'sha256':expected,'bytes':p.stat().st_size}
 return output
