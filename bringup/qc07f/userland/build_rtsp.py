#!/usr/bin/env python3
"""Owned linked RTSP build. Existing public JPEG archive only; never runs ARM.

Copies full transitive compiler dependencies; refuses existing output. No shared
JPEG/kernel build, install, network, device or target operation is performed.
"""
import argparse,datetime,hashlib,json,os,shutil,subprocess
from pathlib import Path
PROJECT=Path(__file__).resolve().parents[3]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def utc():return datetime.datetime.now(datetime.timezone.utc).isoformat()
def main():
 parser=argparse.ArgumentParser(description=__doc__)
 parser.add_argument('--output',type=Path,required=True)
 parser.add_argument('--cc',required=True)
 parser.add_argument('--jpeg-archive',type=Path,required=True)
 parser.add_argument('--jpeg-include',type=Path,action='append',default=[])
 parser.add_argument('--arm',action='store_true')
 args=parser.parse_args();out=args.output.resolve();out.mkdir(mode=0o700)
 for d in ['tmp','snapshot','logs']:(out/d).mkdir()
 env=os.environ.copy();env.update(TMPDIR=str(out/'tmp'),TMP=str(out/'tmp'),TEMP=str(out/'tmp'),LC_ALL='C')
 report={'start':utc(),'hardware_test':False,'arm_executed':False,'commands':[],'inputs':[],'passed':False}
 def run(argv,name):
  e={'argv':[str(x) for x in argv],'cwd':str(PROJECT),'start':utc(),'log':str(out/'logs'/name)};report['commands'].append(e)
  with Path(e['log']).open('xb') as f:r=subprocess.run(e['argv'],cwd=PROJECT,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=60)
  e.update(returncode=r.returncode,end=utc())
  if r.returncode:raise RuntimeError('bounded build command failed: '+name)
 try:
  # Bootlin dispatches its wrapper from argv[0]. Preserve the compiler entry
  # symlink for invocation; resolve only for byte provenance, never execution.
  cc=Path(shutil.which(args.cc) or args.cc).absolute();archive=args.jpeg_archive.resolve()
  units=[PROJECT/'bringup/qc07f/userland/rtspd.c',*[PROJECT/'framework/src'/f'{n}.c' for n in ['rtp_h264','packet_hub','producer','rtsp_request','rtsp_input','rtsp_server']]]
  flags=['-std=c11','-Os','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-I'+str(PROJECT/'framework/include'),*['-I'+str(p.resolve()) for p in args.jpeg_include]]
  if args.arm:flags+=['-static','-march=armv5te','-mcpu=arm926ej-s','-marm','-mfloat-abi=soft','-fstack-usage']
  run([cc,'--version'],'compiler-version.txt')
  dependencies={cc.resolve(),archive,Path(__file__).resolve()}
  real=Path(str(cc)+'.br_real')
  if real.is_file():dependencies.add(real.resolve())
  report['compiler_invocation_path']=str(cc)
  for program in ['cc1','collect2','as','ld']:
   argv=[str(cc),'-print-prog-name='+program]
   e={'argv':argv,'cwd':str(PROJECT),'start':utc()};report['commands'].append(e)
   r=subprocess.run(argv,cwd=PROJECT,env=env,capture_output=True,timeout=10,check=True)
   e.update(returncode=r.returncode,end=utc())
   prog=Path(r.stdout.decode().strip())
   if not prog.is_absolute():
    located=shutil.which(str(prog))
    if located:prog=Path(located)
   if prog.is_file():dependencies.add(prog.resolve())
  for i,unit in enumerate(units):
   dep=out/f'dependencies-{i}.d';run([cc,*flags,'-M','-MF',dep,unit],f'dependencies-{i}.txt')
   text=dep.read_text().replace('\\\n',' ');dependencies.update(Path(x).resolve() for x in text.split(':',1)[1].split())
  for library in ['libc.a','libgcc.a','libgcc_eh.a']:
   argv=[str(cc),'-print-file-name='+library]
   e={'argv':argv,'cwd':str(PROJECT),'start':utc()};report['commands'].append(e)
   r=subprocess.run(argv,cwd=PROJECT,env=env,capture_output=True,timeout=10,check=True);e.update(returncode=r.returncode,end=utc())
   p=Path(r.stdout.decode().strip());
   if p.is_file():dependencies.add(p.resolve())
  for i,p in enumerate(sorted(dependencies)):
   dest=out/'snapshot'/f'{i:04d}-{p.name}'
   with dest.open('xb') as f:f.write(p.read_bytes())
   report['inputs'].append({'path':str(p),'sha256':sha(p),'snapshot':str(dest),'bytes':p.stat().st_size})
  target=out/'rtspd';run([cc,*flags,*units,archive,'-o',target],'link.txt')
  readelf=Path(shutil.which('readelf')).resolve();report['readelf_sha256']=sha(readelf)
  run([readelf,'-h','-A','-W','-S',target],'linked-metadata.txt')
  report['binary']={'path':str(target),'sha256':sha(target),'bytes':target.stat().st_size}
  for item in report['inputs']:
   if sha(Path(item['path']))!=item['sha256']:raise RuntimeError('input changed during build')
  report['passed']=True
 except Exception as error:report['error']=str(error)
 finally:
  report['end']=utc();report['environment']={k:env[k] for k in ['TMPDIR','TMP','TEMP','LC_ALL']}
  report['outputs']={str(p.relative_to(out)):sha(p) for p in sorted(out.rglob('*')) if p.is_file()}
  with (out/'manifest.json').open('x') as f:json.dump(report,f,indent=2);f.write('\n')
 print(json.dumps({'passed':report['passed'],'error':report.get('error'),'output':str(out)}))
 return 0 if report['passed'] else 1
if __name__=='__main__':raise SystemExit(main())
