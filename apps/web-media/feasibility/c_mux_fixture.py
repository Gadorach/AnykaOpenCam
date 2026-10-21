#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""WEB-02 retained-capture oracle using actual shared C init/timeline/headers.
Host compiled code only; no target, credential input, re-encode or driver calls.
"""
from pathlib import Path
import argparse,ctypes,datetime,hashlib,json,os,re,subprocess
from mux_fixture import read_capture
ROOT=Path(__file__).resolve().parents[3]
class Profile(ctypes.Structure):
 _fields_=[('width',ctypes.c_uint32),('height',ctypes.c_uint32),('sps_id',ctypes.c_uint32),('pps_id',ctypes.c_uint32),('profile',ctypes.c_uint8),('compatibility',ctypes.c_uint8),('level',ctypes.c_uint8)]
class Config(ctypes.Structure):
 _fields_=[('sps',ctypes.c_void_p),('pps',ctypes.c_void_p),('sps_bytes',ctypes.c_size_t),('pps_bytes',ctypes.c_size_t),('generation',ctypes.c_uint64),('duration_ticks',ctypes.c_uint32)]
class Init(ctypes.Structure):
 _fields_=[('data',ctypes.c_uint8*2048),('bytes',ctypes.c_size_t),('generation',ctypes.c_uint64),('profile',Profile)]
class Timeline(ctypes.Structure):
 _fields_=[('generation',ctypes.c_uint64),('decode_ticks',ctypes.c_uint64),('expected_rtp',ctypes.c_uint32),('next_sequence',ctypes.c_uint32),('initialized',ctypes.c_uint32)]
class Fragment(ctypes.Structure):
 _fields_=[('header',ctypes.c_uint8*112),('nal',ctypes.c_void_p),('nal_bytes',ctypes.c_size_t)]
def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def build(capture,out,cc):
 data,rows,sps,pps,nals,ticks=read_capture(capture)
 out.parent.mkdir(parents=True,exist_ok=True);out.mkdir(mode=0o700);(out/'tmp').mkdir()
 sources=[ROOT/'framework/src'/n for n in ['fmp4.c','fmp4_init.c','fmp4_timeline.c']]
 args=[cc,'-std=c11','-Os','-Wall','-Wextra','-Werror','-fPIC','-shared','-I'+str(ROOT/'framework/include'),*map(str,sources),'-o',str(out/'libmux-HOST.so')]
 env=dict(os.environ,TMPDIR=str(out/'tmp'))
 q=subprocess.run(args,cwd=ROOT,env=env,capture_output=True,text=True,timeout=30)
 (out/'compile.log').open('x').write(q.stdout+q.stderr)
 if q.returncode:raise RuntimeError('host fixture compiler failed; log retained')
 lib=ctypes.CDLL(str(out/'libmux-HOST.so'))
 lib.camera_fmp4_init_prepare.argtypes=[ctypes.POINTER(Config),ctypes.POINTER(Init)]
 lib.camera_fmp4_timeline_init.argtypes=[ctypes.POINTER(Timeline),ctypes.c_uint64,ctypes.c_uint32]
 lib.camera_fmp4_timeline_step.argtypes=[ctypes.POINTER(Timeline),ctypes.c_uint64,ctypes.c_uint32,ctypes.c_uint32,ctypes.c_void_p,ctypes.c_size_t,ctypes.c_uint32,ctypes.POINTER(Fragment)]
 # ctypes borrows the immutable Python bytes (kept alive through this function).
 # C header preparation neither copies nor owns the VCL payload.
 base=ctypes.cast(ctypes.c_char_p(data),ctypes.c_void_p).value
 starts=list(re.finditer(b'\x00\x00\x00\x01',data));spans=[]
 for i,s in enumerate(starts):
  a=s.end();b=starts[i+1].start() if i+1<len(starts) else len(data);spans.append((data[a]&31,a,b-a))
 ps=next(s for s in spans if s[0]==7);pp=next(s for s in spans if s[0]==8);vcl=[s for s in spans if s[0] in (1,5)]
 cfg=Config(base+ps[1],base+pp[1],ps[2],pp[2],1,0);init=Init()
 assert lib.camera_fmp4_init_prepare(ctypes.byref(cfg),ctypes.byref(init))==0
 assert init.profile.width==1280 and init.profile.height==720 and init.bytes==644
 init_bytes=bytes(init.data[:init.bytes]);(out/'init.mp4').open('xb').write(init_bytes)
 time=Timeline();assert lib.camera_fmp4_timeline_init(ctypes.byref(time),1,rows[0]['timestamp'])==0
 samples=[]
 with (out/'complete.mp4').open('xb') as combined:
  combined.write(init_bytes)
  for i,(row,span) in enumerate(zip(rows[:-1],vcl[:-1])):
   frame=Fragment();assert lib.camera_fmp4_timeline_step(ctypes.byref(time),1,row['timestamp'],rows[i+1]['timestamp'],base+span[1],span[2],int(row['keyframe']),ctypes.byref(frame))==0
   assert frame.nal==base+span[1] and frame.nal_bytes==row['nal_bytes']
   header=bytes(frame.header);payload=memoryview(data)[span[1]:span[1]+span[2]];name=f'frame-{i:02d}.m4s'
   with (out/name).open('xb') as f:f.write(header);f.write(payload)
   combined.write(header);combined.write(payload)
   samples.append({'file':name,'source_index':i,'dts_ticks':ticks[i],'duration_ticks':ticks[i+1]-ticks[i],
    'source_rtp_timestamp':row['timestamp'],'payload_bytes':span[2],'header_bytes':112,'source_nal_sha256':hashlib.sha256(payload).hexdigest(),'keyframe':row['keyframe']})
 assert time.decode_ticks==ticks[-1]
 report={'utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'task':'WEB-02','scope':'actual shared C HOST fixture from retained hardware capture NOT native live/authenticated media',
  'codec':'avc1.42e01f','width':1280,'height':720,'timescale':90000,'init_bytes':init.bytes,'samples':samples,'duration_ticks':ticks[-1],
  'live_init_duration_zero':True,'SPS_geometry_and_PPS_association_validated_in_C':True,'C_timeline_actual_RTP_no_nominalFPS':True,'source_AUs':len(rows),
  'last_complete_source_AU_timestamp_only_for_prior_duration':True,'no_reencode_tint_partialNAL_or_wholeAU_C_copy':True,'source_capture':str(capture.resolve()),
  'source_h264_sha256':hashlib.sha256(data).hexdigest(),'build_argv':args,'compiler_version':subprocess.run([cc,'--version'],capture_output=True,text=True,check=True).stdout,
  'source_pins':{str(p.relative_to(ROOT)):digest(p) for p in [*sources,ROOT/'framework/include/camera/fmp4.h',ROOT/'framework/include/camera/backend.h',Path(__file__).resolve()]},
  'no_driver_or_native_DMA_TLS_auth_handoff_qualification':True}
 (out/'manifest.json').open('x').write(json.dumps(report,indent=2));return report
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--capture',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--cc',default='clang-23')
 a=p.parse_args();r=build(a.capture,a.output.resolve(),a.cc);print(json.dumps({'init_bytes':r['init_bytes'],'samples':len(r['samples']),'duration_ticks':r['duration_ticks'],'scope':r['scope']}))
