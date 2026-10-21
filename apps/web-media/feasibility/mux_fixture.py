#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Offline WEB-01 fixture: retained complete hardware AUs -> fMP4, no re-encode.

Not the product muxer or a live endpoint. Headers and borrowed NAL spans model
bounded on-device output; this PC fixture reads an immutable finite capture.
The final complete AU supplies the next timestamp, not an invented last duration.
"""
from pathlib import Path
import argparse,hashlib,json,struct,datetime,re

U16=lambda n:struct.pack('>H',n)
U32=lambda n:struct.pack('>I',n)
U64=lambda n:struct.pack('>Q',n)
MATRIX=struct.pack('>9I',0x10000,0,0,0,0x10000,0,0,0,0x40000000)
def box(kind,*parts):
 body=b''.join(parts)
 if len(kind)!=4 or len(body)>0xffffffff-8:raise ValueError('box bound')
 return U32(8+len(body))+kind+body
def full(kind,version,flags,*parts):return box(kind,bytes([version])+flags.to_bytes(3,'big'),*parts)
def init_segment(sps,pps,duration):
 if bytes(sps[1:4])!=bytes.fromhex('42e01f'):raise ValueError('unqualified AVC profile')
 if not 0<len(sps)<=65535 or not 0<len(pps)<=65535:raise ValueError('parameter bound')
 avcc=box(b'avcC',b'\1'+bytes(sps[1:4])+b'\xff\xe1',U16(len(sps)),sps,b'\1',U16(len(pps)),pps)
 avc1=box(b'avc1',bytes(6),U16(1),bytes(16),U16(1280),U16(720),
          U32(0x480000),U32(0x480000),bytes(4),U16(1),bytes(32),U16(24),U16(65535),avcc)
 stbl=box(b'stbl',full(b'stsd',0,0,U32(1),avc1),
          full(b'stts',0,0,U32(0)),full(b'stsc',0,0,U32(0)),
          full(b'stsz',0,0,U32(0),U32(0)),full(b'stco',0,0,U32(0)))
 dinf=box(b'dinf',full(b'dref',0,0,U32(1),full(b'url ',0,1)))
 minf=box(b'minf',full(b'vmhd',0,1,bytes(8)),dinf,stbl)
 mdhd=full(b'mdhd',0,0,bytes(8),U32(90000),U32(duration),U16(0x55c4),U16(0))
 hdlr=full(b'hdlr',0,0,bytes(4),b'vide',bytes(12),b'QC07F video\0')
 tkhd=full(b'tkhd',0,3,bytes(8),U32(1),bytes(4),U32(duration),bytes(16),
           MATRIX,U32(1280<<16),U32(720<<16))
 trak=box(b'trak',tkhd,box(b'mdia',mdhd,hdlr,minf))
 mvhd=full(b'mvhd',0,0,bytes(8),U32(90000),U32(duration),U32(0x10000),
           U16(0x100),bytes(10),MATRIX,bytes(24),U32(2))
 mvex=box(b'mvex',full(b'trex',0,0,U32(1),U32(1),U32(0),U32(0),U32(0x1010000)))
 result=box(b'ftyp',b'iso6',U32(1),b'iso6avc1mp41')+box(b'moov',mvhd,trak,mvex)
 if len(result)>2048:raise ValueError('init header bound')
 return result
def fragment_spans(nal,index,dts,duration,keyframe):
 if not 0<len(nal)<=1048576 or not 0<duration<0x80000000:raise ValueError('AU/duration bound')
 if nal[0]&31 not in (1,5) or bool((nal[0]&31)==5)!=keyframe or nal[0]&0x80:raise ValueError('NAL identity')
 def moof(offset):
  return box(b'moof',full(b'mfhd',0,0,U32(index+1)),box(b'traf',
   full(b'tfhd',0,0x20000,U32(1)),full(b'tfdt',1,0,U64(dts)),
   full(b'trun',0,0x701,U32(1),struct.pack('>i',offset),U32(duration),U32(len(nal)+4),
        U32(0x2000000 if keyframe else 0x1010000))))
 header=moof(0);header=moof(len(header)+8)+U32(len(nal)+12)+b'mdat'+U32(len(nal))
 if len(header)>256:raise ValueError('fragment header bound')
 return header,nal
def read_capture(capture):
 data=(capture/'complete.h264').read_bytes();rows=[json.loads(s) for s in (capture/'units.jsonl').read_text().splitlines()]
 manifest=json.loads((capture/'manifest.json').read_text())
 expected=next(e for e in manifest['outputs'] if e['path']=='complete.h264')
 if len(data)!=expected['bytes'] or hashlib.sha256(data).hexdigest()!=expected['sha256']:raise ValueError('capture provenance mismatch')
 starts=list(re.finditer(b'\x00\x00\x00\x01',data))
 if not starts or starts[0].start()!=0:raise ValueError('expected qualified four-byte start codes')
 nals=[memoryview(data)[s.end():starts[i+1].start() if i+1<len(starts) else len(data)] for i,s in enumerate(starts)]
 if any(not n or n[0]&0x80 or n[0]&31 not in (1,5,7,8) for n in nals):raise ValueError('unsupported NAL set')
 sps=[n for n in nals if n[0]&31==7];pps=[n for n in nals if n[0]&31==8];vcl=[n for n in nals if n[0]&31 in (1,5)]
 if len(sps)!=1 or len(pps)!=1 or len(vcl)!=len(rows) or len(rows)<2:raise ValueError('finite one-VCL source contract')
 for i,(n,row) in enumerate(zip(vcl,rows)):
  if not isinstance(row['timestamp'],int) or not 0<=row['timestamp']<=0xffffffff:raise ValueError('RTP timestamp range')
  if row['index']!=i or not row['complete'] or len(n)!=row['nal_bytes'] or hashlib.sha256(n).hexdigest()!=row['sha256']:raise ValueError('complete AU identity mismatch')
  if row['keyframe']!=bool(n[0]&31==5):raise ValueError('keyframe mismatch')
 if not rows[0]['keyframe']:raise ValueError('initial IDR required')
 ticks=[0]
 for a,b in zip(rows,rows[1:]):
  delta=(b['timestamp']-a['timestamp'])&0xffffffff
  if not 0<delta<0x80000000:raise ValueError('non-forward/ambiguous RTP cadence')
  ticks.append(ticks[-1]+delta)
 if ticks[-1]>0xffffffff:raise ValueError('finite fixture duration bound')
 return data,rows,sps[0],pps[0],vcl,ticks
def build(capture,out):
 data,rows,sps,pps,vcl,ticks=read_capture(capture)
 out.parent.mkdir(parents=True,exist_ok=True);out.mkdir(mode=0o700)
 init=init_segment(sps,pps,ticks[-1]);(out/'init.mp4').open('xb').write(init)
 samples=[]
 with (out/'complete.mp4').open('xb') as combined:
  combined.write(init)
  for i,nal in enumerate(vcl[:-1]):
   header,span=fragment_spans(nal,i,ticks[i],ticks[i+1]-ticks[i],rows[i]['keyframe']);name=f'frame-{i:02d}.m4s'
   with (out/name).open('xb') as f:f.write(header);f.write(span)
   combined.write(header);combined.write(span)
   samples.append({'file':name,'source_index':i,'dts_ticks':ticks[i],'duration_ticks':ticks[i+1]-ticks[i],
                   'source_rtp_timestamp':rows[i]['timestamp'],'payload_bytes':len(span),'header_bytes':len(header),
                   'source_nal_sha256':hashlib.sha256(span).hexdigest(),'keyframe':rows[i]['keyframe']})
 report={'utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'task':'WEB-01','scope':'offline retained-hardware browser fixture NOT on-camera live/protected mux',
  'codec':'avc1.42e01f','width':1280,'height':720,'timescale':90000,'init_bytes':len(init),'samples':samples,
  'duration_ticks':ticks[-1],'source_AUs':len(vcl),'last_source_AU_timestamp_only_for_prior_sample_duration':True,
  'no_partial_NAL_or_nominal_FPS_invented_duration_transcoding_or_orientation_tint_edit':True,
  'source_capture':str(capture.resolve()),'source_h264_sha256':hashlib.sha256(data).hexdigest(),
  'prototype_headers_and_borrowed_NAL_spans_not_wholeAU_copy':True,'future_camera_send_chunk_bound':8192,
  'fixture_only_not_general_SPS_or_multiVCL_mux_not_native_acceptance':True}
 (out/'manifest.json').open('x').write(json.dumps(report,indent=2));return report
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--capture',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
 a=p.parse_args();r=build(a.capture,a.output);print(json.dumps({'samples':len(r['samples']),'duration_ticks':r['duration_ticks'],'init_bytes':r['init_bytes'],'max_fragment_header':max(s['header_bytes'] for s in r['samples']),'scope':r['scope']}))
