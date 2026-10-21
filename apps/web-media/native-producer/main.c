/* SPDX-License-Identifier: GPL-2.0-only */
#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
#include "native_adapter.h"
#include "sensor_baseline.h"
#include "tcp_sink.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <unistd.h>
int main(int argc,char **argv)
{
 struct sockaddr_in endpoint;
 /* No target output file/listener/DNS/default endpoint. Explicit authorized
  * parent endpoint and table assertion are NOT runtime/recovery authority. */
 if(argc!=12||!nh_sensor_cli_optin(argc,argv)||strcmp(argv[1],"--parent-protected")||strcmp(argv[4],"--peer")||
    strcmp(argv[6],"--port")||nh_tcp_endpoint(argv[5],argv[7],&endpoint)||strcmp(argv[8],"--memory-reserve")||strcmp(argv[9],"normal75-placement-one-shot")||
    strcmp(argv[10],"--producer-storage")||(strcmp(argv[11],"raw-reuse-k75")&&strcmp(argv[11],"separate"))){
  fprintf(stderr,"usage: native-handoff --parent-protected --sensor-baseline exact139 --peer 192.168.1.15 --port 17891 --memory-reserve normal75-placement-one-shot --producer-storage raw-reuse-k75|separate\n");return 64;
 }
 if(geteuid()!=0){fprintf(stderr,"root RAM qualification helper only\n");return 65;}
 struct rlimit core={0,0};if(setrlimit(RLIMIT_CORE,&core))return 66;
 struct sigaction sa={0};sa.sa_handler=nh_native_cancel;sigemptyset(&sa.sa_mask);
 if(sigaction(SIGTERM,&sa,NULL)||sigaction(SIGINT,&sa,NULL))return 70;
 nh_native_init();struct camera_fmp4_config config;struct nh_report report={0};
 if(nh_native_storage_select(argv[11])||nh_native_optin_exact139()||nh_native_config(&config))return 71;
 struct nh_tcp_sink sink={.fd=-1};uint64_t start=nh_native_now(NULL);
 if(nh_native_memory_start(start)){nh_native_memory_report();if(nh_native_now(NULL)==UINT64_MAX)nh_native_quarantine();return 74;}
 uint8_t *storage=malloc(CAMERA_FMP4_MAX_NAL_BYTES);if(!storage)return 69;
 if(nh_native_memory_fixed_buffers(storage)){nh_native_memory_report();if(nh_native_now(NULL)==UINT64_MAX)nh_native_quarantine();free(storage);return 74;}
 if(nh_tcp_connect(&sink,&endpoint,start,nh_native_now,nh_native_cancelled,NULL)){
  fprintf(stderr,"NH_TCP_CONNECT_FAILED source_started=0 close_once=%d\n",sink.close_attempted);
  if(nh_native_now(NULL)==UINT64_MAX)nh_native_quarantine();
  free(storage);return 72;
 }
 fprintf(stderr,"NH_TCP_SOCKET peer=%s port=%s snd_requested=%d snd_actual=%d rcv_requested=%d rcv_actual=%d socket_memory_RSS_proved=0\n",
 argv[5],argv[7],sink.requested_send,sink.actual_send,sink.requested_receive,sink.actual_receive);
 struct nh_backend backend={.context=&sink,.next=nh_native_next,.release=nh_native_release,.retire=nh_native_retire,
 .now=nh_native_now,.cancelled=nh_native_cancelled,.write=nh_tcp_write,.discard_output=nh_tcp_discard,.quarantine=nh_native_fence,
 .session_start_ns=start,.output_begin=nh_tcp_begin,.retire_held=nh_native_retire_held,.memory_gate=nh_native_memory_gate};
 int r=nh_run(&backend,&config,storage,CAMERA_FMP4_MAX_NAL_BYTES,&report);
 nh_native_sensor_report(); /* bounded scalar batch, outside producer timing; no payload */
 nh_native_memory_report();
 for(unsigned i=0;i<report.integrity_count;i++){
  fprintf(stderr,"NH_HUB_CRC sequence=%u source_crc=%08x before_crc=%08x after_crc=%08x after_checked=%u before_begin_ns=%llu before_end_ns=%llu after_begin_ns=%llu after_end_ns=%llu\n",
  report.integrity[i].sequence,report.integrity[i].source_crc,report.integrity[i].before_crc,
  report.integrity[i].after_crc,report.integrity[i].after_checked,
  (unsigned long long)report.integrity[i].before_begin_ns,(unsigned long long)report.integrity[i].before_end_ns,
  (unsigned long long)report.integrity[i].after_begin_ns,(unsigned long long)report.integrity[i].after_end_ns);
 }
 fprintf(stderr,"NH_HUB_INTEGRITY integrity_ns=%llu retained_hub_backing=%u retained_returned_producer=%u failed_next_unknown_backing=%u extra_scalar_table_bytes=1024 measured_heap_peak=0\n",
 (unsigned long long)report.integrity_ns,report.retained_hub,report.retained_producer,report.failed_next);
 for(unsigned i=0;i<report.output;i++){
  fprintf(stderr,"NH_SAMPLE sequence=%u source=%llu file=%llu generation=%llu source_ns=%llu next_source_ns=%llu next_begin_ns=%llu next_end_ns=%llu output_begin_ns=%llu output_end_ns=%llu nal_bytes=%zu wire_bytes=%zu\n",
  report.samples[i].key.sequence,(unsigned long long)report.samples[i].key.source,
  (unsigned long long)report.samples[i].key.file,(unsigned long long)report.samples[i].key.generation,
  (unsigned long long)report.samples[i].source_ns,(unsigned long long)report.samples[i].next_source_ns,
  (unsigned long long)report.samples[i].next_begin_ns,(unsigned long long)report.samples[i].next_end_ns,
  (unsigned long long)report.samples[i].output_begin_ns,(unsigned long long)report.samples[i].output_end_ns,
  report.samples[i].nal_bytes,report.samples[i].wire_bytes);
 }
  fprintf(stderr,"NH_RESULT result=%d produced=%u outputs=%u boundary_retired=%u source_retired=%u quarantine=%u complete_local_boundary_bytes=%zu accepted_TCP_bytes=%zu max_nal=%zu start_ns=%llu end_ns=%llu hub_copy_ns=%llu output_ns=%llu CPU_payload_declared_bytes=%zu producer_storage=%s CPU_peak_measured=0 wire_cap=2097152 target_file_bytes=0 remote_ACK_proved=0 remote_rollback=0 TLS_measured=0\n",
 r,report.produced,report.output,report.boundary_retired,report.retired,report.quarantined,
 report.committed,report.accepted,report.max_nal,(unsigned long long)report.started,
  (unsigned long long)report.ended,(unsigned long long)report.hub_copy_ns,(unsigned long long)report.output_ns,
  (size_t)(1728000+1048576+(!strcmp(argv[11],"separate")?1048576:0)),argv[11]);
 int closed=nh_tcp_close(&sink);
 if(report.quarantined)nh_native_quarantine();
 free(storage);if(closed)return 72;
 return r?73:0;
}
