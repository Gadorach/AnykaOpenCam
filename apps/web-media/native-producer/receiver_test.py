#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""PUBLICDUMMY parser/owned localhost tests only; no camera/private data."""
from pathlib import Path
import argparse,importlib.util,json,socket,subprocess,sys,struct
from receiver import validate_prefix,CAP
from box_reference import fragment_spans
def main():
 p=argparse.ArgumentParser();p.add_argument('output',type=Path);p.add_argument('sender',type=Path);p.add_argument('reference',type=Path);a=p.parse_args()
 a.output.mkdir();spec=importlib.util.spec_from_file_location('independent_mux',a.reference);ref=importlib.util.module_from_spec(spec);spec.loader.exec_module(ref)
 init=ref.init_segment(bytes.fromhex('6742e01f'),b'\x68\x80',0)
 nal=memoryview(b'\x65PUBLICDUMMY-NOT-FOOTAGE');h,n=fragment_spans(nal,0,0,40500,True);wire=init+h+bytes(n)
 cases=0
 r=validate_prefix(wire);assert len(r['samples'])==1 and r['complete_prefix_bytes']==len(wire);cases+=1
 for cut in [0,4,len(init)-1,len(init),len(init)+111,len(wire)-1]:
  r=validate_prefix(wire[:cut]);assert not r['samples'] and r['complete_prefix_bytes'] in (0,len(init));cases+=1
 for offset in [len(init)+20,len(init)+60,len(init)+84,len(init)+92,len(init)+96]:
  bad=bytearray(wire);bad[offset]^=1;r=validate_prefix(bytes(bad));assert not r['samples'] and r['complete_prefix_bytes']==len(init);cases+=1
 # Preserve incomplete wire; do not pad/truncate it to make a media sample.
 partial=wire+wire[len(init):len(init)+120];r=validate_prefix(partial)
 assert len(r['samples'])==1 and r['complete_prefix_bytes']==len(wire);cases+=1
 # Only explicit owned localhost binds are run by this test.
 reserve=socket.socket();reserve.bind(('127.0.0.1',0));port=reserve.getsockname()[1];reserve.close()
 out=a.output/'PUBLICDUMMY-localhost'
 cmd=[sys.executable,'-B',str(Path(__file__).with_name('receiver.py')),'--bind','127.0.0.1','--port',str(port),'--peer','127.0.0.1','--output',str(out),'--PUBLICDUMMY-localhost']
 receiver=subprocess.Popen(cmd,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
 try:
  ready=json.loads(receiver.stdout.readline());assert ready['READY'] and ready['PUBLICDUMMY']
  sender=subprocess.run([str(a.sender),str(port)],capture_output=True,text=True,timeout=15);assert sender.returncode==0,sender.stderr
  stdout,stderr=receiver.communicate(timeout=20);assert receiver.returncode==0,stderr
  report=json.loads((out/'report.json').read_text());assert report['PUBLICDUMMY'] and len(report['samples'])==12
  assert report['wire_bytes']==report['complete_prefix_bytes']==644+12*(112+32) and report['wire_bytes']<CAP
  assert (out/'wire.bin').read_bytes()==(out/'complete-prefix.mp4').read_bytes()
  cases+=1
  # Fresh output refusal occurs before opening any network socket.
  retry=subprocess.run(cmd,capture_output=True,text=True,timeout=3);assert retry.returncode!=0;cases+=1
  # Real owned localhost peer closes during a generated fragment; wire tail
  # remains intact, separately staged prefix contains only first complete sample.
  reserve=socket.socket();reserve.bind(('127.0.0.1',0));port=reserve.getsockname()[1];reserve.close()
  partial_out=a.output/'PUBLICDUMMY-partial-close';partial_cmd=cmd.copy()
  partial_cmd[partial_cmd.index('--port')+1]=str(port);partial_cmd[partial_cmd.index('--output')+1]=str(partial_out)
  partial_receiver=subprocess.Popen(partial_cmd,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
  try:
   assert json.loads(partial_receiver.stdout.readline())['READY']
   connection=socket.create_connection(('127.0.0.1',port),timeout=2)
   connection.sendall(partial);connection.close()
   _,errors=partial_receiver.communicate(timeout=5);assert partial_receiver.returncode==0,errors
   pr=json.loads((partial_out/'report.json').read_text());assert pr['wire_bytes']==len(partial) and pr['complete_prefix_bytes']==len(wire)
   assert (partial_out/'wire.bin').read_bytes()==partial and (partial_out/'complete-prefix.mp4').read_bytes()==wire
   cases+=1
  finally:
   if partial_receiver.poll() is None:partial_receiver.kill();partial_receiver.communicate()
 finally:
  if receiver.poll() is None:receiver.kill();receiver.communicate()
 print(f'PUBLICDUMMY receiver {cases} parser/refuse-overwrite/owned-localhost13-12 cases PASS; NO_CAMERA_NETWORK/NO_PRIVATE_FOOTAGE/NO_DECODE_PROOF')
if __name__=='__main__':main()
