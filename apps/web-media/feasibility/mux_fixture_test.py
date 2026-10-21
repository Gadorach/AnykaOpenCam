#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Synthetic header/span regressions; not a decoder or hardware claim."""
import struct,unittest
from mux_fixture import init_segment,fragment_spans

class Headers(unittest.TestCase):
 def test_native_sized_payload_borrowed_not_copied(self):
  storage=bytes([0x65])+bytes(137003);nal=memoryview(storage)
  header,span=fragment_spans(nal,0,0,45720,True)
  self.assertIs(span,nal);self.assertIs(span.obj,storage)
  self.assertEqual(len(header),112);self.assertGreater(len(span),128*1024)
  k=header.index(b'trun');fields=struct.unpack('>iIII',header[k+12:k+28])
  self.assertEqual(fields,(len(header)-4,45720,len(nal)+4,0x2000000))
  self.assertEqual(header[-4:],struct.pack('>I',len(nal)))
 def test_P_frame_flags_and_64bit_decode_time(self):
  h,n=fragment_spans(memoryview(b'\x41\x80'),7,0x100000123,39150,False)
  k=h.index(b'tfdt');self.assertEqual(h[k+4],1)
  self.assertEqual(struct.unpack('>Q',h[k+8:k+16])[0],0x100000123)
  self.assertEqual(struct.unpack('>I',h[h.index(b'trun')+24:h.index(b'trun')+28])[0],0x1010000)
 def test_bad_duration_rejected(self):
  for duration in [0,-1,0x80000000]:
   with self.assertRaises(ValueError):fragment_spans(memoryview(b'\x65\x80'),0,0,duration,True)
 def test_incomplete_oversized_or_wrong_NAL_not_clipped(self):
  for n in [b'',b'\xe5\x80',b'\x67\x80',b'\x65'+bytes(1048576)]:
   with self.assertRaises(ValueError):fragment_spans(memoryview(n),0,0,45000,True)
 def test_keyframe_identity_rejected(self):
  with self.assertRaises(ValueError):fragment_spans(memoryview(b'\x41\x80'),0,0,45000,True)
 def test_profile_change_is_not_silently_accepted(self):
  with self.assertRaises(ValueError):init_segment(memoryview(bytes.fromhex('6764001f')),memoryview(b'\x68\x80'),90000)
 def test_header_box_accounting(self):
  data=init_segment(memoryview(bytes.fromhex('6742e01f')),memoryview(b'\x68\x80'),90000)
  offset=0;names=[]
  while offset<len(data):
   size=struct.unpack('>I',data[offset:offset+4])[0];self.assertGreaterEqual(size,8)
   names.append(data[offset+4:offset+8]);offset+=size
  self.assertEqual(offset,len(data));self.assertEqual(names,[b'ftyp',b'moov'])
  self.assertLessEqual(len(data),2048)

if __name__=='__main__':unittest.main()
