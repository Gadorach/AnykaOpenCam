#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Bounded public synthetic responses, in-memory fake peer; no wire logs."""
import io
import unittest
from rtsp_capture import Capture
class Peer:
    def __init__(self, data, fragment=7):self.data, self.fragment=data, fragment
    def recv(self, count):
        n=min(count, self.fragment);data,self.data=self.data[:n],self.data[n:];return data
def capture():return Capture(io.BytesIO(),io.BytesIO(),io.StringIO())
def response(fields,body=b''):
    return ('RTSP/1.0 200 OK\r\n'+fields+'\r\n').encode('ascii')+body
class Replies(unittest.TestCase):
    def test_case_and_token(self):
        for name in ['Session','session','SeSsIoN']:
            for suffix in ['', ';timeout=30', ';TIMEOUT=4294967295']:
                self.assertEqual(capture().reply(Peer(response(f'cSeQ: 1\r\ncOnTeNt-LeNgTh: 0\r\n{name}: CAM1{suffix}\r\n')),1),'CAM1')
    def test_empty_and_body_boundary(self):
        c=capture();peer=Peer(response('CSeq: 0\r\nContent-Length: 4\r\n',b'$\r\n\0TAIL'),99)
        self.assertEqual(c.reply(peer,0),'');self.assertEqual(c.exact(peer,4),b'TAIL')
        self.assertEqual(capture().reply(Peer(response('CSeq: 4294967295\r\nSession: CAM1\r\n')),0xffffffff),'CAM1')
    def test_critical_duplicates(self):
        for name,value in [('CSeq','1'),('Content-Length','0'),('Session','CAM1'),('Transport','RTP/AVP/TCP'),('Authorization','DUMMY-PUBLIC')]:
            fields='' if name=='CSeq' else 'CSeq: 1\r\n'
            with self.assertRaises(ValueError):capture().reply(Peer(response(fields+f'{name}: {value}\r\n{name.lower()}: {value}\r\n')),1)
    def test_negative_bounds_and_syntax(self):
        fields=['cseq: -1\r\n','cseq: +1\r\n','cseq: 4294967296\r\n','cseq: 1x\r\n','Other: public\r\n',
                'CSeq : 1\r\n','CSeq: 1\r\n folded\r\n','CSeq: 1\r\nOther:\tpublic\r\n',
                'CSeq: 1\r\nContent-Length: 8193\r\n','CSeq: 1\r\nContent-Length: -1\r\n',
                'CSeq: 1\r\nTransfer-Encoding: chunked\r\n','CSeq: 1\r\nTrailer: Other\r\n',
                'CSeq: 1\r\nSession: \r\n','CSeq: 1\r\nSession: CAM1;timeout=0\r\n',
                'CSeq: 1\r\nSession: CAM1;timeout=4294967296\r\n','CSeq: 1\r\nSession: CAM1;extra=1\r\n',
                'CSeq: 1\r\nSession: '+('X'*65)+'\r\n','CSeq: 1\r\nTransport: '+('X'*513)+'\r\n',
                'CSeq: 1\r\nOther: '+('X'*1025)+'\r\n','CSeq: 1\r\n'+('Other: public\r\n'*24)]
        for text in fields:
            with self.assertRaises(ValueError):capture().reply(Peer(response(text)),1)
    def test_header_total_and_reconnect(self):
        with self.assertRaises(ValueError):capture().reply(Peer(response('CSeq: 1\r\n'+('Other: '+('X'*1000)+'\r\n')*9),8192),1)
        for token in ['CAM1','CAM2']:
            self.assertEqual(capture().reply(Peer(response(f'CSeq: 1\r\nSession: {token}\r\n')),1),token)
if __name__=='__main__':unittest.main()
