#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Parent-run private PC qualification recipe; no transport or camera contact.
Agent use is permitted ONLY with --public-host-fixture, never --create-private.
Secret files/content/hashes are deliberately absent from stdout/manifests.
"""
from pathlib import Path
import argparse, os, stat, struct, subprocess, sys
p=argparse.ArgumentParser()
p.add_argument('--create-private',action='store_true')
p.add_argument('--public-host-fixture',action='store_true')
p.add_argument('--material-tool',type=Path,required=True)
p.add_argument('--directory',type=Path,required=True)
p.add_argument('--address',default='192.168.1.73')
p.add_argument('--port',type=int,default=18443)
p.add_argument('--device-id',default='qc07f-qualification')
p.add_argument('--sensor-id',default='qc07f-main-qualification')
p.add_argument('--profile-id',default='qc07f-main-h264-qualification')
p.add_argument('--account',default='qualification-operator')
p.add_argument('--boot-generation',type=int,required=True)
p.add_argument('--child-generation',type=int,required=True)
p.add_argument('--runtime',type=int,choices=[120,180],default=180)
args=p.parse_args()
if args.create_private==args.public_host_fixture:p.error('explicitly select exactly one qualification class')
if args.public_host_fixture and args.address!='127.0.0.1':p.error('PUBLICDUMMY keys/vectors restricted to host loopback')
if not args.public_host_fixture and args.address!='192.168.1.73':p.error('review a fresh explicit native IP before changing this recipe')
if not (1024<=args.port<=65535) or min(args.boot_generation,args.child_generation)<1:p.error('invalid public launch metadata')
for text in [args.device_id,args.sensor_id,args.profile_id,args.account]:
 if not 1<=len(text)<64 or not all(c.isascii() and (c.isalnum() or c in '._-') for c in text):p.error('canonical CORE/qualification IDs required')
os.umask(0o077)
directory=args.directory.resolve()
directory.mkdir(mode=0o700,parents=False)
env={'PATH':'/usr/bin:/bin','LC_ALL':'C','TMPDIR':str(directory),'TEMP':str(directory),'TMP':str(directory)}
def run(command):
 # No private arguments, env, request stdout, key output or payload hash logs.
 result=subprocess.run(list(map(str,command)),env=env,stdin=subprocess.DEVNULL,
  stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,timeout=15)
 if result.returncode:raise SystemExit('private qualification provisioning failed; preserve scoped files, do not retry/overwrite')
tool=args.material_tool.resolve();s=tool.stat()
if not stat.S_ISREG(s.st_mode) or s.st_nlink!=1 or s.st_mode&0o022:raise SystemExit('trusted material-tool metadata failure')
result=subprocess.run([str(tool),str(directory)],env=env,stdin=subprocess.DEVNULL,
 stdout=subprocess.PIPE,stderr=subprocess.DEVNULL,timeout=15)
expected='PC_material_generation_result=0 class='+('PUBLICDUMMY_HOST_ONLY' if args.public_host_fixture else 'PRIVATE_PARENT_ONLY')+' work_factor=600000 files_private=1\n'
if result.returncode or result.stdout!=expected.encode():raise SystemExit('qualification material class/result mismatch; preserve private files')
run(['openssl','req','-new','-x509','-newkey','ec','-pkeyopt','ec_paramgen_curve:P-256','-sha256','-nodes',
 '-keyout',directory/'key.pem','-out',directory/'cert.pem','-days','2',
 '-subj','/CN='+('PUBLICDUMMY-HOST-ONLY' if args.public_host_fixture else 'QC07F-PRIVATE-QUALIFICATION'),
 '-addext','subjectAltName=IP:'+args.address])
for name in ['key.pem','cert.pem','credentials.bin','operator.password']:
 f=directory/name;os.chmod(f,0o600);s=f.lstat()
 if not stat.S_ISREG(s.st_mode) or s.st_nlink!=1 or s.st_uid!=os.getuid():raise SystemExit('private file policy failure')
text=lambda s,n:s.encode('ascii')+bytes(n-len(s))
authority=f'{args.address}:{args.port}';origin='https://'+authority
config=struct.pack('<IIII',0x41575331,2,768,1)+text(origin,128)+text(authority,128)+text(origin,160)
config+=b''.join(text(s,64) for s in [args.device_id,args.sensor_id,args.profile_id,args.account])
config+=struct.pack('<QQQQQQQ',args.boot_generation,args.child_generation,1,120*10**9,60*10**9,args.runtime*10**9,35*10**9)+bytes(24)
assert len(config)==768
fd=os.open(directory/'config.bin',os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600)
try:
 if os.write(fd,config)!=len(config):raise SystemExit('private config write failed')
 os.fsync(fd)
finally:os.close(fd)
print('qualification_input_created=1 class='+('PUBLICDUMMY_HOST_ONLY' if args.public_host_fixture else 'PRIVATE_PARENT_ONLY')+
      ' private_transfer_required=1 native_execution=0 product_accepted=0')
