#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Finite parent-owned qualification receiver, NOT UI/bridge/media publisher.
Wire remains byte-exact; only an independently validated complete prefix is
staged in a SEPARATE fresh file. No reads/serving of project/raw/credential dirs.
Actual private capture/network belongs to parent. Tests use PUBLICDUMMY localhost.
"""
from pathlib import Path
import argparse,datetime,hashlib,ipaddress,json,os,select,signal,socket,struct,time
from box_reference import fragment_spans
CAP=2097152
def boxes(data,start,end):
 while start<end:
  if end-start<8:raise ValueError('incomplete box header')
  n=struct.unpack_from('>I',data,start)[0]
  if n<8 or n>end-start:raise ValueError('incomplete/extended/unbounded box')
  yield data[start+4:start+8],start,start+n
  start+=n
def crc_bzip2(data):
 table=[]
 for i in range(256):
  c=i<<24
  for _ in range(8):c=((c<<1)^ (0x04c11db7 if c&0x80000000 else 0))&0xffffffff
  table.append(c)
 c=0xffffffff
 for b in data:c=((c<<8)^table[((c>>24)^b)&255])&0xffffffff
 return c^0xffffffff
def validate_prefix(data):
 """Narrow one-track bounded C format, not arbitrary MP4 or decoder proof."""
 end=0;samples=[];reason='incomplete init'
 if len(data)>CAP:return {'complete_prefix_bytes':0,'samples':[],'reason':'wire cap','decode_proved':False,'source_crc_correlation_proved':False}
 try:
  if len(data)<8:raise ValueError('incomplete init')
  ftyp=struct.unpack_from('>I',data,0)[0]
  if ftyp<8 or ftyp>2048 or data[4:8]!=b'ftyp' or len(data)<ftyp+8:raise ValueError('invalid/incomplete ftyp')
  moov=struct.unpack_from('>I',data,ftyp)[0];init_end=ftyp+moov
  if moov<8 or init_end>2048 or init_end>len(data) or data[ftyp+4:ftyp+8]!=b'moov':raise ValueError('invalid/incomplete moov')
  nodes=[]
  def walk(start,stop):
   for kind,a,b in boxes(data,start,stop):
    nodes.append((kind,a,b))
    if kind in (b'moov',b'trak',b'mdia',b'minf',b'dinf',b'stbl',b'mvex'):walk(a+8,b)
    if kind==b'stsd':
     if b-a<16 or struct.unpack_from('>I',data,a+12)[0]!=1:raise ValueError('sample entry count')
     walk(a+16,b)
    if kind==b'avc1':
     if b-a<86 or struct.unpack_from('>HH',data,a+32)!=(1280,720):raise ValueError('geometry')
     walk(a+86,b)
  walk(ftyp,init_end)
  if sum(k==b'trak' for k,_,_ in nodes)!=1:raise ValueError('track count')
  mdhd=[(a,b) for k,a,b in nodes if k==b'mdhd'];avcc=[(a,b) for k,a,b in nodes if k==b'avcC']
  if len(mdhd)!=1 or len(avcc)!=1 or struct.unpack_from('>I',data,mdhd[0][0]+20)[0]!=90000:raise ValueError('timescale/config')
  a,b=avcc[0];p=a+8
  if data[p:p+6]!=bytes.fromhex('0142e01fffe1'):raise ValueError('AVC profile')
  ns=struct.unpack_from('>H',data,p+6)[0];q=p+8+ns
  if not 0<ns<=256 or q+3>b or data[p+8]&31!=7 or data[q]!=1:raise ValueError('SPS')
  np=struct.unpack_from('>H',data,q+1)[0]
  if not 0<np<=256 or q+3+np!=b or data[q+3]&31!=8:raise ValueError('PPS')
  end=init_end;dts=0
  while end<len(data):
   if len(samples)>=12:raise ValueError('extra output')
   if len(data)-end<112:raise ValueError('incomplete fragment header')
   h=data[end:end+112]
   if h[4:8]!=b'moof' or struct.unpack_from('>I',h,0)[0]!=100 or h[104:108]!=b'mdat':raise ValueError('fragment boxes')
   nal_bytes=struct.unpack_from('>I',h,108)[0];duration=struct.unpack_from('>I',h,88)[0]
   if not 1<nal_bytes<=1048576 or not 0<duration<180000 or end+112+nal_bytes>len(data):raise ValueError('incomplete/bounded NAL/duration')
   nal=memoryview(data)[end+112:end+112+nal_bytes];key=(nal[0]&31)==5
   if not samples and not key:raise ValueError('first IDR')
   expected,_=fragment_spans(nal,len(samples),dts,duration,key)
   if h!=expected:raise ValueError('independent fragment header/sequence/DTS/size/flags mismatch')
   samples.append({'sequence':len(samples)+1,'dts':dts,'duration':duration,'nal_bytes':nal_bytes,'crc_bzip2':f'{crc_bzip2(nal):08x}'})
   dts+=duration;end+=112+nal_bytes
  reason='complete wire prefix'
 except (ValueError,IndexError,struct.error) as e:reason=str(e)
 return {'complete_prefix_bytes':end,'samples':samples,'reason':reason,'decode_proved':False,'source_crc_correlation_proved':False}
def receive(bind,port,peer,output,public_dummy=False,ready_callback=None):
 if public_dummy:
  if bind!='127.0.0.1' or peer!='127.0.0.1':raise ValueError('PUBLICDUMMY localhost only')
 else:
  actual=ipaddress.IPv4Address(peer)
  if bind!='192.168.1.15' or port!=17891 or actual not in ipaddress.IPv4Network('192.168.1.0/24') or str(actual) in ('192.168.1.0','192.168.1.1','192.168.1.15','192.168.1.255'):raise ValueError('explicit parent-selected camera IPv4 peer/port policy')
 for ip in (bind,peer):ipaddress.IPv4Address(ip)
 if not 1024<=port<=65535:raise ValueError('explicit unprivileged port')
 output=Path(output);output.mkdir(mode=0o700);os.chmod(output,0o700)
 start=time.monotonic();wire=output/'wire.bin';report={'UTC':datetime.datetime.now(datetime.timezone.utc).isoformat(),'bind':bind,'port':port,'peer':peer,'cap':CAP,'deadline_s':45,'max_connections':1,'PUBLICDUMMY':public_dummy,'remote_ACK_protocol':False}
 def expired(signum,frame):raise TimeoutError('finite45s receiver deadline')
 old_handler=signal.signal(signal.SIGALRM,expired);signal.setitimer(signal.ITIMER_REAL,45)
 count=0;reason='no connection';connection=None;listener=None
 try:
  with wire.open('xb') as f:
   listener=socket.socket(socket.AF_INET,socket.SOCK_STREAM);listener.setsockopt(socket.SOL_SOCKET,socket.SO_RCVBUF,8192)
   listener.bind((bind,port));listener.listen(1);listener.setblocking(False)
   if ready_callback:ready_callback(listener.getsockname()[1])
   print(json.dumps({'READY':True,'bind':bind,'port':listener.getsockname()[1],'peer':peer,'PUBLICDUMMY':public_dummy}),flush=True)
   while time.monotonic()-start<45 and connection is None:
    if select.select([listener],[],[],max(0,45-(time.monotonic()-start)))[0]:
     connection,address=listener.accept();listener.close();listener=None
     if address[0]!=peer:reason='wrong peer, one connection consumed';connection.close();connection=None;break
     connection.setblocking(False);reason='connected'
   if connection:
    report['actual_receive_buffer']=connection.getsockopt(socket.SOL_SOCKET,socket.SO_RCVBUF)
    while count<CAP and time.monotonic()-start<45:
     if not select.select([connection],[],[],max(0,45-(time.monotonic()-start)))[0]:reason='45s deadline';break
     try:data=connection.recv(min(8192,CAP-count))
     except (BlockingIOError,InterruptedError):continue
     if not data:reason='EOF';break
     f.write(data);count+=len(data)
    if count==CAP:reason='hard byte cap reached, no additional read'
 except (OSError,ValueError) as e:reason=type(e).__name__+': '+str(e)
 finally:
  if connection:connection.close()
  if listener:listener.close()
 data=wire.read_bytes();assert len(data)==count and count<=CAP
 try:parsed=validate_prefix(data)
 except TimeoutError:parsed={'complete_prefix_bytes':0,'samples':[],'reason':'validation deadline; wire preserved','decode_proved':False,'source_crc_correlation_proved':False}
 finally:signal.setitimer(signal.ITIMER_REAL,0);signal.signal(signal.SIGALRM,old_handler)
 # This is a NEW derived prefix file, never truncation/repair of wire/acquisition.
 with (output/'complete-prefix.mp4').open('xb') as f:f.write(memoryview(data)[:parsed['complete_prefix_bytes']])
 report.update(parsed);report.update({'wire_bytes':count,'wire_sha256':hashlib.sha256(data).hexdigest(),'outcome':reason,'elapsed_s':time.monotonic()-start,'original_wire_unchanged':True})
 with (output/'report.json').open('x') as f:json.dump(report,f,indent=2)
 return report
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--bind',required=True);p.add_argument('--port',required=True,type=int);p.add_argument('--peer',required=True);p.add_argument('--output',required=True,type=Path);p.add_argument('--PUBLICDUMMY-localhost',action='store_true')
 a=p.parse_args();os.umask(0o077);r=receive(a.bind,a.port,a.peer,a.output,a.PUBLICDUMMY_localhost)
 print(json.dumps({'wire_bytes':r['wire_bytes'],'complete_prefix_bytes':r['complete_prefix_bytes'],'samples':len(r['samples']),'reason':r['reason'],'remote_ACK_proved':False}))
