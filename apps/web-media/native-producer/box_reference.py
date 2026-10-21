#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Narrow pure fragment oracle from canonical feasibility/mux_fixture.py.
No capture reading/build/publication functions are imported."""
import struct
U32=lambda n:struct.pack('>I',n)
U64=lambda n:struct.pack('>Q',n)
def box(kind,*parts):
 body=b''.join(parts)
 if len(kind)!=4 or len(body)>0xffffffff-8:raise ValueError('box bound')
 return U32(8+len(body))+kind+body
def full(kind,version,flags,*parts):return box(kind,bytes([version])+flags.to_bytes(3,'big'),*parts)
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
