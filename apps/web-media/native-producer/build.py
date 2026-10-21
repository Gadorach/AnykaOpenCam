#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Offline scoped build. Only new delegated evidence directories are writable.

Uses a controlled rtspd excerpt, NOT SDK builds or historical binaries/libraries.
ARM output is inspected, never run. HOST runs only synthetic mock tests.
"""
from pathlib import Path
import argparse, ast, datetime, difflib, hashlib, json, os, re, shlex, shutil, struct, subprocess, traceback

A = Path(__file__).resolve().parent
ROOT = A.parents[2]
EVIDENCE = ROOT / 'evidence/delegated/20261012-native-handoff'
PINS = {
 'bringup/qc07f/userland/rtspd.c': '0334c27a8d6a7142d3867778f517f192f7c7447fc849a7aca18098ad306f3898',
 'bringup/qc07f/variants/security75/mach-ak3918ev200/qc07f_hwenc.h': '6723c30c76a3fbd1feaf1ed9d1676a2b9b6289b352ffe7682372bf8e95123731',
 'bringup/qc07f/variants/security75/mach-ak3918ev200/isp.c': '7d0ef80561df4ef058be1deb6698001c35066fab5533f492dec97d2b447bbd50',
 'bringup/qc07f/variants/security75/mach-ak3918ev200/qc07f_hwenc.c': 'e97744838acd469772db41fb1fe0907c95fb6e923577cdbffc84ead0141a026f',
 'bringup/qc07f/initramfs/sc1346-init.h': '98da5eb9e522975d834d99e8c0cdfbefe75ed87dbbcdf84fd9f0fb150f9efbe9',
  'bringup/qc07f/initramfs/init.c': 'b3dd4ee9689bde4bead4c7a3c862284b198b879d4470b920366c1503814598aa',
 'work/lightbulb-camera/implementation-20261008T205404Z/sources/linux-lts/mm/page_alloc.c': '0fc3313f62da1c7e51a4b3f732785a67ede6f6afb1ca9c0f5dd4a745fa6462c0',
 'work/lightbulb-camera/implementation-20261008T205404Z/sources/linux-lts/mm/vmstat.c': 'd3da55b28aefa3e2e92d7d3ed19efa809735397fe93a0696a929553c938ff06e',
 'work/lightbulb-camera/implementation-20261008T205404Z/sources/linux-lts/include/linux/mmzone.h': '8653d8ee6b0ebd419f5e553952a8db29f9ee85d2d902d651a2936ed256f1bebd',
 'work/lightbulb-camera/implementation-20261008T205404Z/sources/linux-lts/fs/binfmt_elf.c': '94de8efdef946d505dec15a2b9cf6e41dcdde21791dceeeca7d8f2e8fdf60d45',
}
TOOL_PINS = {
 'bin/toolchain-wrapper': 'f4db963a226e01244ce6f7b5d1ec6d57474dd811119ff8834429f022835ce9c1',
 'bin/arm-buildroot-linux-musleabi-gcc.br_real': 'e24ea223e7b521cce1f5889d63c25417e930cdbb35653b072f7b48d9a4b41729',
 'arm-buildroot-linux-musleabi/sysroot/lib/libc.a': '0982970abc52a36d247326a8e0b5dd487410222ff9728528cc99a4e8bd62e7b5',
 'lib/gcc/arm-buildroot-linux-musleabi/14.3.0/libgcc.a': 'f707f3a5a94033cbbde76f76640d56dbc5dc615ab421be4efc2641c14344f19a',
}
def sha(p): return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def fresh_write(p, data):
 with Path(p).open('x') as f: f.write(data)
def excerpt(text, start, end): return text[text.index(start):text.index(end)]
def replace(text, old, new):
 if text.count(old) != 1: raise ValueError('non-unique import transformation: ' + old[:80])
 return text.replace(old, new)
def import_rtspd(out):
 text=(ROOT/'bringup/qc07f/userland/rtspd.c').read_text()
 if hashlib.sha256(text.encode()).hexdigest()!=PINS['bringup/qc07f/userland/rtspd.c']:
  raise ValueError('original changed during controlled import')
 pieces=[
  excerpt(text,'struct qc_rtsp_context {','#define QC_RTSP_RECEIPT_API'),
  excerpt(text,'static void source_context_init(','static uint8_t hw_quant'),
  excerpt(text,'static void on_signal(','static int read_all('),
  excerpt(text,'static int sensor_read(','static int capture(struct qc_rtsp_context'),
  excerpt(text,'static int produce_h264_frame(','static void idle_h264_producer('),
  excerpt(text,'static uint64_t producer_clock_ns(','static void producer_event('),
 ]
 original='/* SPDX-License-Identifier: GPL-2.0-only; exact selected rtspd functions */\n'+''.join(pieces)
 modified=replace(original,'attempt<2','attempt<1')
 modified=replace(modified,'''   if(failure!=ETIMEDOUT||attempt||continuous_retire(c,1)!=CAMERA_OK)goto done;
   c->receipt_renewals++;
   if(write(watchdog,".",1)!=1)goto done;''','''   (void)failure;goto done; /* finite candidate: no renewal/retry */''')
 modified=replace(modified,'if(capture(c))','if(continuous_next(c))')
 modified=replace(modified,'}else if(!qc_receipt_retired(&proof))return capture_failed(c);',
                  '}else return capture_failed(c); /* finite normal stop requires ACTIVE -> END */')
 modified=replace(modified,'ACTUAL-CAMERA','NATIVE-CANDIDATE')
 modified=replace(modified,'unsigned h264_stream_frames;uint8_t *produced_storage;',
                   'unsigned h264_stream_frames;uint8_t *produced_storage;uint8_t *resident_storage;unsigned raw_valid;uint32_t produced_nal_crc32_bzip2;uint64_t producer_begin_ns;')
 modified=replace(modified,'static int produce_h264_frame(',
                  'static int prepare_output(struct qc_rtsp_context *,uint64_t);\nstatic int before_encode_gate(uint64_t);\nstatic int produce_h264_frame(')
 modified=replace(modified,'if(begin==UINT64_MAX){c->retirement_uncertain=1;return CAMERA_IO_ERROR;}',
                  'if(begin==UINT64_MAX){c->retirement_uncertain=1;return CAMERA_IO_ERROR;}c->producer_begin_ns=begin*1000000ULL;')
 modified=replace(modified,'if(continuous_next(c)){c->retirement_uncertain=1;return CAMERA_IO_ERROR;}',
                  'if(prepare_output(c,c->producer_begin_ns)!=CAMERA_OK)return CAMERA_NOT_READY;\n   if(continuous_next(c)){c->retirement_uncertain=1;return CAMERA_IO_ERROR;}')
 modified=replace(modified,'output=malloc(0x100000);if(!output)return CAMERA_IO_ERROR;',
                  'output=c->resident_storage;if(!output)return CAMERA_NOT_READY; /* one prefaulted persistent CPU buffer */')
 modified=replace(modified,'guard=open("/dev/watchdog0",O_WRONLY);if(guard<0||ioctl(guard,WDIOC_SETTIMEOUT,&timeout)||timeout!=5)goto done;',
                  'if(before_encode_gate(c->producer_begin_ns)!=CAMERA_OK)return CAMERA_NOT_READY; /* no encode/DMA allocation on forecast failure; raw remains owned */\n  guard=open("/dev/watchdog0",O_WRONLY);if(guard<0||ioctl(guard,WDIOC_SETTIMEOUT,&timeout)||timeout!=5)goto done;')
 modified=replace(modified,'c->receipt_evidence=active;c->capture_sequence=q.sequence;result=0;break;',
                  'c->receipt_evidence=active;c->capture_sequence=q.sequence;c->raw_valid=1;result=0;break;')
 modified=replace(modified,'if(c->produced_storage)return CAMERA_BUSY;\n',
                  'if(c->produced_storage)return CAMERA_BUSY; c->raw_valid=0; /* no stale RAW view; next complete capture required */\n')
 modified=replace(modified,'int ownership_keyframe=c->ownership_idr;',
                  'if(!c->raw_valid)goto done;int ownership_keyframe=c->ownership_idr;')
 modified=replace(modified,'if(ioctl(c->h264_stream_fd,QC_HWENC_H264_STREAM,&q))goto done;job=q.frame;',
                  'c->raw_valid=0; /* output may overwrite raw even on partial EFAULT; never claim raw after encode */\n    if(ioctl(c->h264_stream_fd,QC_HWENC_H264_STREAM,&q))goto done;job=q.frame;')
 modified=replace(modified,excerpt(modified,'static int sensor_read(','static int capture_failed('),
 '''static int sensor_read(int fd,uint16_t reg){return nh_sensor_read_byte(fd,reg);}
static int sensor_stream(int fd,uint8_t on){return nh_sensor_stream(fd,on);}
''')
 modified=replace(modified,'||sensor_read(c->continuous_sensor,0x3208)!=5||sensor_read(c->continuous_sensor,0x3209)!=0||sensor_read(c->continuous_sensor,0x320a)!=2||sensor_read(c->continuous_sensor,0x320b)!=0xd0',
                  '||sensor_read(c->continuous_sensor,0x0100)!=0')
 modified=replace(modified,'c->receipt_evidence=reserved; /* prove the exact ARM key before hardware BEGIN */',
 '''c->receipt_evidence=reserved; /* prove exact reservation BEFORE table writes */
     if(!sensor_baseline_requested||nh_sensor_exact139(c->continuous_sensor,watchdog,c->producer_begin_ns,&source_clock,&stop,&sensor_baseline_report))goto done;
     if(qc_receipt_query(c->continuous_frame,&c->receipt_key,&reserved)||reserved.state!=QC_RECEIPT_ARMED||reserved.reason||reserved.stop_result||reserved.stopped_ns)goto done;
     c->receipt_evidence=reserved; /* exact reservation still ARMED after finite preparation */''')
 modified=replace(modified,'uint32_t delivered=qc_crc_bzip2(raw,sizeof(raw));','''uint64_t raw_crc_begin=camera_rtsp_clock_ns(&source_clock);
    uint32_t delivered=qc_crc_bzip2(raw,sizeof(raw));
    uint64_t raw_crc_end=camera_rtsp_clock_ns(&source_clock);
    if(raw_crc_begin==UINT64_MAX||raw_crc_end==UINT64_MAX)goto done;
    fprintf(stderr,"NH_RAW_CRC sequence=%u bytes=%zu begin_ns=%llu end_ns=%llu crc_bzip2=%08x\\n",q.sequence,sizeof(raw),(unsigned long long)raw_crc_begin,(unsigned long long)raw_crc_end,delivered);''')
 modified=replace(modified,'uint32_t nal_crc=qc_crc_bzip2(packet->data,packet->bytes);','''uint64_t nal_crc_begin=camera_rtsp_clock_ns(&source_clock);
    uint32_t nal_crc=qc_crc_bzip2(packet->data,packet->bytes);
    uint64_t nal_crc_end=camera_rtsp_clock_ns(&source_clock);
    if(nal_crc_begin==UINT64_MAX||nal_crc_end==UINT64_MAX)goto done;
    c->produced_nal_crc32_bzip2=nal_crc; /* exact existing full-NAL scalar evidence */
    fprintf(stderr,"NH_NAL_CRC sequence=%u bytes=%zu begin_ns=%llu end_ns=%llu crc_bzip2=%08x\\n",c->capture_sequence,packet->bytes,(unsigned long long)nal_crc_begin,(unsigned long long)nal_crc_end,nal_crc);''')
 modified=replace(modified,'free(output);','''if(result!=CAMERA_OK&&output){c->produced_storage=output;output=NULL;}
    (void)output; /* persistent CPU backing survives loan release and all uncertainty */''')
 modified=replace(modified,'free(c->produced_storage);c->produced_storage=NULL;memset(packet,0,sizeof(*packet));',
                   '     /* Retain uncertain CPU backing and all descriptors. */ memset(packet,0,sizeof(*packet));')
 modified=replace(modified,'free(c->produced_storage);c->produced_storage=NULL;',
                  'c->produced_storage=NULL; /* retire CPU loan, keep charged backing until proven source stop */')
 preamble='''/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#include <errno.h>
#include <fcntl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/watchdog.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <camera/backend.h>
#include <camera/rtsp_clock.h>
#include "qc07f_hwenc.h"
#include "h264-baseline.h"
#include "isp-stream.h"
#include "capture-receipt.h"
#include "crc-bzip2.h"
#include "sensor_baseline.h"
#define MAIN_PIXELS (1280U * 720U)
#define FRAME_BYTES 1728000U
static volatile sig_atomic_t stop;
static uint8_t raw[FRAME_BYTES];
static unsigned width=1280,height=720;
static int h264_p_frames=1,capture_irq=0;
static unsigned trace_every=1;
static unsigned long frame_count;
static unsigned sensor_baseline_requested;
static struct nh_sensor_report sensor_baseline_report;
'''
 fresh_write(out/'rtspd-selected-original.c',original)
 fresh_write(out/'rtspd-native-import.c',preamble+modified)
 fresh_write(out/'rtspd-import.diff',''.join(difflib.unified_diff(original.splitlines(True),modified.splitlines(True),
   fromfile='rtspd-selected-original.c',tofile='rtspd-native-import-functions.c')))
 return {'original_excerpt_sha256':sha(out/'rtspd-selected-original.c'),
         'transformed_source_sha256':sha(out/'rtspd-native-import.c'),
         'import_diff_sha256':sha(out/'rtspd-import.diff'),
         'changes':['single attempt; no expiry renewal','continuous receipt path only',
                    'retain all uncertain CPU backing','checked raw/NAL CRC wall edges; full scalar CRC unchanged',
                    'diagnostic label NATIVE-CANDIDATE (HOST mock is not actual camera)',
                    'retain scalar producer full-NAL CRC for source-to-hub comparison; no new kernel API',
                     'finite normal stop rejects an already-expired/stale receipt instead of ACKing it as normal END',
                     'explicit exact139 preparation under existing watchdog5/producer start and ARMED reservation, then re-QUERY before BEGIN',
                      'typed STOP-separated monitor-equivalent I2C helpers; fixed address/stream values; no old-state geometry assumption',
                      'one resident prefaulted producer CPU mapping prepared BEFORE capture, no per-AU malloc/free; backing released only after safe source stop',
                      'explicit raw-reuse-k75 mode overlays raw CPU OUTPUT only after sealed kernel input-copy/reset/guards; no raw reuse while producer loan held; raw-valid revoked before ioctl',
                    'narrow includes/globals; no JPEG/software/network']}

def main():
 p=argparse.ArgumentParser();p.add_argument('run');p.add_argument('--toolchain',type=Path,required=True)
 args=p.parse_args()
 if not re.fullmatch(r'run[0-9A-Za-z]+',args.run): raise ValueError('fresh runNAME required')
 EVIDENCE.mkdir(parents=True,exist_ok=True);R=EVIDENCE/args.run;R.mkdir()
 I=R/'inputs';D=R/'build';L=R/'logs';T=R/'tmp'
 for d in (I,D,L,T):d.mkdir()
 env={'PATH':'/usr/bin:/bin','LC_ALL':'C','SOURCE_DATE_EPOCH':'1791694800',
      'TMPDIR':str(T),'TMP':str(T),'TEMP':str(T),'PYTHONDONTWRITEBYTECODE':'1'}
 m={'started':datetime.datetime.now(datetime.timezone.utc).isoformat(),'state':'building',
    'commands':[],'environment':env,'source_inputs':{},'tool_inputs':{},'fixture':'HOST synthetic bytes only',
    'native_execution':False,'output_scope_exception':'explicit WRITE ONLY rule wins over suggested local/build path'}
 def run(argv,label,timeout=120):
  argv=list(map(str,argv));log=L/(label+'.txt')
  entry={'argv':argv,'cwd':str(R),'log':str(log),'started':datetime.datetime.now(datetime.timezone.utc).isoformat()}
  m['commands'].append(entry)
  with log.open('xb') as f:r=subprocess.run(argv,cwd=R,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=timeout)
  entry['exit']=r.returncode
  if r.returncode:raise RuntimeError(label+' failed: '+str(log))
  return log.read_text(errors='replace')
 def copy(src,name=None):
  src=Path(src);dest=I/(name or src.name)
  dest.parent.mkdir(parents=True,exist_ok=True)
  if dest.exists():raise ValueError('duplicate input: '+str(dest))
  if src.name=='crc-bzip2.h':
   original=src.read_text();normalized=replace(original,'"../../../framework/include/camera/dma_integrity.h"','<camera/dma_integrity.h>')
   fresh_write(dest,normalized)
   fresh_write(I/'crc-include-only.diff',''.join(difflib.unified_diff(original.splitlines(True),normalized.splitlines(True),fromfile=str(src),tofile='crc-bzip2.h')))
  else:
   shutil.copyfile(src,dest)
   if sha(src)!=sha(dest):raise ValueError('source changed during snapshot: '+str(src))
  m['source_inputs'][str(src)]={'sha256':sha(src),'copy':str(dest),'copy_sha256':sha(dest)}
 try:
  for name,pin in PINS.items():
   if sha(ROOT/name)!=pin:raise ValueError('retained source pin changed: '+name)
   m['source_inputs'][str(ROOT/name)]={'sha256':pin,'role':'read-only original/source audit'}
  isp=(ROOT/'bringup/qc07f/variants/security75/mach-ak3918ev200/isp.c').read_text()
  sites=[i for i,line in enumerate(isp.splitlines(),1) if 'crc_ram01_static4(' in line]
  if len(sites)!=12:raise ValueError('12 STATIC4 source sites not intact')
  m['kernel_STATIC4_unchanged_sites']=sites
  m['import']=import_rtspd(I)
  for src in sorted(A.glob('*')):
   if src.is_file():copy(src)
  copy(ROOT/'apps/web-media/feasibility/mux_fixture.py','independent-mux-reference.py')
  reference=ast.parse((I/'independent-mux-reference.py').read_text());ours=ast.parse((I/'box_reference.py').read_text())
  for name in ['box','full','fragment_spans']:
   old=next(n for n in reference.body if isinstance(n,ast.FunctionDef) and n.name==name)
   new=next(n for n in ours.body if isinstance(n,ast.FunctionDef) and n.name==name)
   if ast.dump(old,include_attributes=False)!=ast.dump(new,include_attributes=False):raise ValueError('independent box oracle subset changed: '+name)
  for name in ['capture-receipt.h','capture-retirement-uapi.h','crc-bzip2.h','h264-baseline.h']:
   copy(ROOT/'bringup/qc07f/userland'/name)
  for name in ['qc07f_hwenc.h','isp-stream.h']:
   copy(ROOT/'bringup/qc07f/variants/security75/mach-ak3918ev200'/name)
  copy(ROOT/'bringup/qc07f/initramfs/sc1346-init.h')
  table=(I/'sc1346-init.h').read_text()
  pairs=[(int(reg,16),int(value,16)) for reg,value in re.findall(r'\{\s*(0x[0-9a-f]+),\s*(0x[0-9a-f]+)\s*\}',table)]
  if len(pairs)!=139 or pairs[0]!=(0x0103,1) or any(reg==0x0100 and value for reg,value in pairs):raise ValueError('exact139 table shape')
  final=dict(pairs)
  if any(final[reg]!=value for reg,value in [(0x3208,5),(0x3209,0),(0x320a,2),(0x320b,0xd0),(0x320c,7),(0x320d,8),(0x320e,2),(0x320f,0xee),(0x0100,0)]):raise ValueError('exact139 baseline geometry/timing mismatch')
  fresh_write(I/'sensor-table-map.json',json.dumps({'table_sha256':sha(I/'sc1346-init.h'),'pairs':139,'unique_registers':len(final),
   'rows':[{'index':i,'reg':reg,'programmed':value,'expected_final_read':0 if reg==0x0103 else final[reg]} for i,(reg,value) in enumerate(pairs)],
   'policy':'all139 ordered writes; reset10ms once; all139 reads, last-write final values; strict reset-complete0; no other writes'},indent=2)+'\n')
  monitor=(ROOT/'bringup/qc07f/initramfs/init.c').read_text()
  original=excerpt(monitor,'struct i2c_msg_wire {','static void sensor_state_fd(')
  fresh_write(I/'sensor-monitor-helper-original.c',original)
  fresh_write(I/'sensor-monitor-helper-adaptation.diff',''.join(difflib.unified_diff(original.splitlines(True),(A/'sensor_baseline.c').read_text().splitlines(True),fromfile='monitor-original-typed-helper.c',tofile='bounded-sensor-baseline.c')))
  for name in ['backend.h','fmp4.h','packet_hub.h','rtsp_clock.h','dma_integrity.h']:
   copy(ROOT/'framework/include/camera'/name,'framework/include/camera/'+name)
  shared=[]
  for name in ['packet_hub.c','fmp4.c','fmp4_init.c','fmp4_timeline.c']:
   copy(ROOT/'framework/src'/name,'framework/src/'+name);shared.append(I/'framework/src'/name)
  for src in sorted((ROOT/'apps/auth-web/service/runtime-licenses').glob('*')):
   if src.is_file():copy(src,'runtime-license-'+src.name)
  TC=args.toolchain.resolve();prefix=str(TC/'bin/arm-buildroot-linux-musleabi-')
  for name,pin in TOOL_PINS.items():
   if sha(TC/name)!=pin:raise ValueError('toolchain pin mismatch: '+name)
   m['tool_inputs'][str(TC/name)]=pin
  common=['-std=gnu11','-Os','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',
          '-ffunction-sections','-fdata-sections','-fstack-protector-strong',
          '-D_FORTIFY_SOURCE=2','-I'+str(I),'-I'+str(I/'framework/include')]
  def compile_link(cc,flags,sources,target,static=False,link_flags=()):
   objects=[]
   for s in sources:
    obj=D/(target+'-'+s.stem+'.o');objects.append(obj)
    run([cc,*common,*flags,'-MD','-MF',str(obj)+'.d','-fstack-usage','-c',s,'-o',obj],obj.stem)
   run([cc,*flags,*objects,*link_flags,*(['-static'] if static else []),'-Wl,--gc-sections','-Wl,-z,noexecstack',
        '-Wl,--trace','-Wl,-Map='+str(D/target)+'.map','-o',D/target],target+'-link')
  for cc,label in [('cc','host'),(prefix+'gcc','arm')]:
   run([cc,'--version'],label+'-compiler-version');run([cc,'-dumpmachine'],label+'-target')
   for name in ['cc1','as','ld','libgcc.a','crt1.o','crti.o','crtbeginT.o','crtend.o','crtn.o','libc.a']:
    value=run([cc,('-print-prog-name=' if name in ['cc1','as','ld'] else '-print-file-name=')+name],label+'-closure-'+name).strip()
    fp=Path(value) if '/' in value else Path(shutil.which(value) or value)
    if fp.is_file():m['tool_inputs'][str(fp.resolve())]=sha(fp)
  tests=[I/'handoff_test.c',I/'handoff.c',*shared]
  compile_link('cc',[],tests,'handoff-host');run([D/'handoff-host'],'HOST-regression')
  compile_link('cc',['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie'],tests,'handoff-sanitized')
  run([D/'handoff-sanitized'],'HOST-sanitized')
  memtests=[I/'memory_reserve_test.c',I/'memory_reserve.c']
  compile_link('cc',['-DNH_MEMORY_PUBLIC_FIXTURE'],memtests,'memory-reserve-host');run([D/'memory-reserve-host'],'HOST-memory-reserve')
  compile_link('cc',['-DNH_MEMORY_PUBLIC_FIXTURE','-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie'],memtests,'memory-reserve-sanitized')
  run([D/'memory-reserve-sanitized'],'HOST-memory-reserve-sanitized')
  wraps=['-Wl,--wrap='+n for n in ['open','close','write','malloc','free','clock_gettime','ioctl','nanosleep','nh_memory_read_fixed']]
  adapter_tests=[I/'native_adapter_test.c',I/'handoff.c',I/'memory_reserve.c',I/'memory_runtime.c',I/'memory_reader.c',*shared]
  compile_link('cc',['-DNH_MEMORY_PUBLIC_FIXTURE'],adapter_tests,'native-adapter-host',link_flags=wraps)
  run([D/'native-adapter-host'],'HOST-imported-adapter')
  compile_link('cc',['-DNH_MEMORY_PUBLIC_FIXTURE','-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie'],
               adapter_tests,'native-adapter-sanitized',link_flags=wraps)
  run([D/'native-adapter-sanitized'],'HOST-imported-adapter-sanitized')
  netwrap=['-Wl,--wrap='+n for n in ['socket','setsockopt','getsockopt','getpeername','connect','poll','__poll_chk','send','close']]
  nettests=[I/'tcp_sink_test.c',I/'tcp_sink.c']
  compile_link('cc',['-DNH_TCP_PUBLIC_DUMMY_HOST'],nettests,'tcp-sink-host',link_flags=netwrap);run([D/'tcp-sink-host'],'HOST-TCP-mocks')
  compile_link('cc',['-DNH_TCP_PUBLIC_DUMMY_HOST','-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie'],nettests,'tcp-sink-sanitized',link_flags=netwrap)
  run([D/'tcp-sink-sanitized'],'HOST-TCP-mocks-sanitized')
  compile_link('cc',['-DNH_TCP_PUBLIC_DUMMY_HOST'],[I/'tcp_loopback.c',I/'tcp_sink.c',I/'handoff.c',*shared],'PUBLICDUMMY-loopback')
  run(['python3','-B',I/'receiver_test.py',R/'PUBLICDUMMY-receiver-tests',D/'PUBLICDUMMY-loopback',I/'independent-mux-reference.py'],'PUBLICDUMMY-receiver-tests',45)
  native=[I/'main.c',I/'native_adapter.c',I/'sensor_baseline.c',I/'tcp_sink.c',I/'handoff.c',I/'memory_reserve.c',I/'memory_runtime.c',I/'memory_reader.c',*shared]
  compile_link(prefix+'gcc',['-march=armv5te','-marm','-mfloat-abi=soft'],native,'native-handoff-arm',True)
  for tool,flags in [('readelf',['-h','-A','-l','-d']),('nm',['-u']),('size',[])]:
   result=run([prefix+tool,*flags,D/'native-handoff-arm'],'ARM-'+tool)
   if tool=='nm' and result.strip():raise ValueError('undefined ARM symbol')
  run([prefix+'objdump','-d',D/'native-handoff-arm'],'ARM-disassembly')
  for tool in ['gcc','strip','objdump','readelf','nm','size']:
   fp=Path(prefix+tool).resolve();m['tool_inputs'][str(fp)]=sha(fp)
  run([prefix+'strip','-o',D/'native-handoff-arm.stripped',D/'native-handoff-arm'],'ARM-strip')
  image=(D/'native-handoff-arm.stripped').read_bytes();eh=struct.unpack_from('<16sHHIIIIIHHHHHH',image)
  assert eh[1]==2 and eh[2]==40 and eh[9]==32 and eh[11]==40 and eh[6]+eh[11]*eh[12]==len(image), 'sealed ELF shape/file-end differs from own-header debt calculation'
  loads=[];rx_file_pages=0
  for i in range(eh[10]):
   typ,off,vaddr,paddr,filesz,memsz,flags,align=struct.unpack_from('<IIIIIIII',image,eh[5]+i*eh[9])
   if typ==1:
    pages=(vaddr+memsz+4095)//4096-vaddr//4096;loads.append({'vaddr':vaddr,'memsz':memsz,'pages':pages,'flags':flags,'offset':off,'filesz':filesz})
    if flags&3==1:rx_file_pages+=(off+filesz+4095)//4096-off//4096
  assert len(loads)==2 and rx_file_pages
  stage_kib=((len(image)+4095)//4096-rx_file_pages)*4
  mapped_kib=sum(x['pages']*4 for x in loads)
  cpu_kib=max(3832,mapped_kib+2056+stage_kib)
  alias_cpu_kib=max(2804,mapped_kib+1028+stage_kib)
  m['strictMemoryReserve_footprint']={'loads':loads,'page_bytes':4096,'musl_two_large_mapping_capacity_kib':2056,'nonshared_staging_extra_kib':stage_kib,
   'CPU_total_unpaid_capacity_kib':cpu_kib,'nonmovable_RX_staging_unpaid_kib':rx_file_pages*4+stage_kib,'initial_A_required_kib':8256+cpu_kib+512+1024,
   'v4_floor_kib':13624,'floor_never_reduced':8256+cpu_kib+512+1024>=13624,'overhead_envelope_kib':512,'reserve_kib':1024,
   'proposal_scope':'parent-approved ONE finite experiment only, NOT peak/security ceiling/memory admission/native proof'}
  m['strictMemoryReserve_footprint']['raw_reuse_variant']={'mapped_CPU_capacity_kib':alias_cpu_kib,'removed_mapping_capacity_kib':cpu_kib-alias_cpu_kib,'producer_mmap_count':0,
   'initial_A_required_kib':8256+alias_cpu_kib+512+1024,'initial_nonCMA_requirement_before_M_kib':alias_cpu_kib+512+1024,
   'current_h_unknown_CMA_migration_upper_kib':7168,'native_admission':'HOLD until both corrected forecasts and qualified migration/placement bounds pass; no prefault to hope for rescue'}
  m['state']='BUILD-candidate-HOST-fixture-passed-NOT-native-qualified'
 except BaseException:
  m['state']='failed';m['failure']=traceback.format_exc();print(m['failure'])
 finally:
  closure={}
  for dep in D.glob('*.d'):
   for name in shlex.split(dep.read_text().replace('\\\n','').split(':',1)[1]):
    fp=Path(name)
    if fp.is_file():closure[str(fp.resolve())]=sha(fp)
  m['consumed_header_source_closure']=closure
  for log in L.glob('*-link.txt'):
   for line in log.read_text().splitlines():
    fp=Path(line.strip())
    if fp.is_absolute() and fp.is_file():m['tool_inputs'][str(fp.resolve())]=sha(fp)
  for t in ['cc','python3','as','ld']:
   fp=Path(shutil.which(t)).resolve();m['tool_inputs'][str(fp)]=sha(fp)
  m['files']=[{'path':str(fp.relative_to(R)),'bytes':fp.stat().st_size,'sha256':sha(fp)} for fp in sorted(R.rglob('*')) if fp.is_file()]
  m['ended']=datetime.datetime.now(datetime.timezone.utc).isoformat()
  fresh_write(R/'manifest.json',json.dumps(m,indent=2)+'\n');print(str(R));print(m['state'])
 return 1 if m['state']=='failed' else 0
if __name__=='__main__':raise SystemExit(main())
