#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""PUBLICDUMMY only. Never invoke PRIVATE generation/native ARM/root flow.
No generated file bytes/nonce/username/HA1/password/payload hashes are logged.
"""
from pathlib import Path
import json,os,shutil,subprocess,sys
checks=0
def check(value):
 global checks
 checks+=1
 if not value:raise RuntimeError('scalar_check='+str(checks))
def main():
 generator,host_helper,native_helper,hardfault_helper,child,work=map(Path,sys.argv[1:])
 work.mkdir();counter=0
 def run(argv,label,success=True):
  argv=list(map(str,argv));log=work/(label+'.txt')
  with log.open('xb') as f:r=subprocess.run(argv,stdout=f,stderr=subprocess.STDOUT,timeout=15)
  check((r.returncode==0)==success);text=log.read_text()
  for marker in ('Authorization:','WWW-Authenticate:','username=','nonce=','response=','rtsp-password:','HA1='):
   check(marker not in text)
  return text
 def scopes(label,alg='SHA-256'):
  nonlocal counter
  counter+=1;private=work/(label+'-private');code=work/(label+'-public');private.mkdir(mode=0o700);code.mkdir(mode=0o700)
  shutil.copyfile(child,code/'verifier');(code/'verifier').chmod(0o555);code.chmod(0o555)
  args=[generator,'--PUBLICDUMMY',private,alg,'PUBLICDUMMY-device','PUBLICDUMMY-rtsp','camera0','camera0-main','888','777','7','rtsp://publicdummy.invalid/live']
  text=run(args,label+'-generate');check('material_class=PUBLICDUMMY_ONLY' in text)
  check((private/'rtsp-material.bin').stat().st_size==1536)
  check((private/'rtsp-material.bin').stat().st_mode&0o777==0o600)
  check((private/'rtsp-password.pc').stat().st_mode&0o777==0o600)
  return private,code,args
 for alg in ('SHA-256','MD5'):
  private,code,args=scopes(alg.lower(),alg)
  text=run([host_helper,private,code],alg.lower()+'-entry');check('entry_owned_SIGTERM_BYE_wipe_wait0=observed' in text)
  check('functional_RTSP_verify_native_NOTtested=1' in text and 'positive_crypto_NOTtested=1' in text)
  run(args,alg.lower()+'-no-overwrite',False)
  run([generator,'--PRIVATE-PC-ONLY',*args[2:]],alg.lower()+'-DUMMY-reject-private-class',False)
  run([native_helper,private,code],alg.lower()+'-native-root-refusal',False) #own UID nonroot; no input material read
 private,code,args=scopes('metadata')
 text=run([hardfault_helper,private,code],'owned-hardfault-HOLD',False);check('entry_result=HOLD' in text and 'entry_owned_SIGTERM_BYE_wipe_wait0=observed' not in text)
 #Only OWN PUBLICDUMMY length field changes, never a read/dump/hash of material.
 with (private/'rtsp-material.bin').open('r+b') as f:f.seek(56);f.write((0).to_bytes(4,'little'))
 text=run([host_helper,private,code],'actual-provisioning-ACK-failure',False);check('entry_start=denied' in text)
 with (private/'rtsp-material.bin').open('r+b') as f:f.seek(56);f.write((32).to_bytes(4,'little'))
 (private/'rtsp-material.bin').chmod(0o644);run([host_helper,private,code],'material-mode-denied',False);(private/'rtsp-material.bin').chmod(0o600)
 linked=private/'duplicate-publicdummy';os.link(private/'rtsp-material.bin',linked);run([host_helper,private,code],'material-hardlink-denied',False);linked.unlink()
 #Replace only OWN dummy test-file naming with a symlink; original content remains preserved.
 (private/'rtsp-material.bin').rename(private/'preserved-material.bin');(private/'rtsp-material.bin').symlink_to('preserved-material.bin')
 run([host_helper,private,code],'material-symlink-denied',False);(private/'rtsp-material.bin').unlink();(private/'preserved-material.bin').rename(private/'rtsp-material.bin')
 #Reject a FIFO/directory without opening/reading either special inode. Preserve
 #the original OWN PUBLICDUMMY record; this is not private input manipulation.
 (private/'rtsp-material.bin').rename(private/'preserved-material.bin')
 os.mkfifo(private/'rtsp-material.bin',0o600);run([host_helper,private,code],'material-FIFO-denied-no-block',False)
 (private/'rtsp-material.bin').unlink();(private/'rtsp-material.bin').mkdir(mode=0o600)
 run([host_helper,private,code],'material-directory-denied',False)
 (private/'rtsp-material.bin').rmdir();(private/'preserved-material.bin').rename(private/'rtsp-material.bin')
 alias=work/'scope-alias';alias.symlink_to(private);run([host_helper,alias,code],'scope-symlink-denied',False)
 run([host_helper,str(private)+'/../'+private.name,code],'scope-dotdot-denied',False)
 code.chmod(0o755);run([host_helper,private,code],'code-scope-mode-denied',False);code.chmod(0o555)
 (code/'verifier').chmod(0o500);run([host_helper,private,code],'code-0500-denied',False);(code/'verifier').chmod(0o555)
 #Same-size public executable with intact ELF metadata but wrong IMAGE content
 #must be rejected by the reviewed adapter pin, not just the file-size guard.
 (code/'verifier').chmod(0o755)
 with (code/'verifier').open('r+b') as f:
  f.seek(-1,2);tail=f.read(1);f.seek(-1,2);f.write(bytes([tail[0]^1]))
 (code/'verifier').chmod(0o555)
 text=run([host_helper,private,code],'same-size-image-pin-denied',False);check('entry_start=denied' in text)
 #Known image pin cannot be replaced by a different child class without failure.
 code.chmod(0o700);(code/'verifier').unlink();shutil.copyfile(native_helper,code/'verifier');(code/'verifier').chmod(0o555);code.chmod(0o555)
 run([host_helper,private,code],'image-source-pin-class-denied',False)
 bad=work/'bad-uri';bad.mkdir(mode=0o700)
 run([generator,'--PUBLICDUMMY',bad,'SHA-256','PUBLICDUMMY-device','PUBLICDUMMY-rtsp','camera0','camera0-main','888','777','7','rtsp://PUBLICDUMMY@publicdummy.invalid/live'],'generator-userinfo-denied',False)
 check(not (bad/'rtsp-material.bin').exists())
 result={'evidence_state':'HOST PUBLICDUMMY ONLY','checks':checks,'result':'PASS','PRIVATE_generation_executed':False,'native_ROOT_2M_ready':'HOLD',
  'positive_native_crypto_NOTtested':True,'G3':False,'G8':False,'generated_payload_hashes_recorded':False}
 with (work/'RESULT.json').open('x') as f:json.dump(result,f,indent=2);f.write('\n')
 print('HOST_ONLY PASS helper_generator_checks='+str(checks)+' PRIVATE_execution=0 native_ROOT_2M=HOLD positive_native_crypto_NOTtested=1')
if __name__=='__main__':
 try:main()
 except BaseException as e:print('HOST_ONLY FAIL checks='+str(checks)+' error_type='+type(e).__name__);sys.exit(1)
