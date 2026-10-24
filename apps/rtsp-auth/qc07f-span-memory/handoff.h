/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef QC_NATIVE_HANDOFF_H
#define QC_NATIVE_HANDOFF_H
#include <camera/fmp4.h>
#include <camera/packet_hub.h>
#include "memory_reserve.h"
#define NH_OUTPUT_LIMIT (2U * 1024U * 1024U)
#define NH_PRODUCED_LIMIT 13U
#define NH_SESSION_NS 10000000000ULL
#define NH_OUTPUT_NS 2000000000ULL
/* NAL CRC is scalar producer evidence, not a new kernel query/auth token. */
struct nh_key {uint64_t source,file,generation;uint32_t sequence,nal_crc32_bzip2;};
struct nh_backend {
 void *context;
 int (*next)(void *,struct camera_packet *,struct nh_key *);
 int (*release)(void *,const struct camera_packet *);
 int (*retire)(void *); /* exact normal END/QUERY/ACK or error; never retry */
 uint64_t (*now)(void *); /* UINT64_MAX = sticky failure */
 int (*cancelled)(void *);
 int (*write)(void *,const uint8_t *,size_t,size_t *);
 int (*discard_output)(void *,size_t); /* sink close/discard; NEVER remote rollback */
 void (*quarantine)(void *); /* sticky admission fence only; no reset/free/retry */
 uint64_t session_start_ns; /* optional earlier connect/start budget; never later */
 int (*output_begin)(void *,uint64_t,size_t); /* one fixed whole-output deadline */
 int (*retire_held)(void *,const struct camera_packet *); /* optional safe stop retains CPU backing until END/QUERY/ACK */
 int (*memory_gate)(void *,enum nh_memory_stage);
};
struct nh_report {
 unsigned produced,output,boundary_retired,retired,quarantined;
 /* committed = complete generated-file prefix (including init).
  * accepted = cumulative bytes acknowledged by write, including discarded tail;
  * rollback never decrements accepted. Neither field counts acquisition reads. */
 size_t committed,accepted,max_nal;
 uint64_t started,ended,last_clock,hub_copy_ns,output_ns;
 struct nh_key key;
 unsigned integrity_count,retained_hub,retained_producer,failed_next;
 uint64_t integrity_ns;
 struct {
  uint32_t sequence,source_crc,before_crc,after_crc,after_checked;
  uint64_t before_begin_ns,before_end_ns,after_begin_ns,after_end_ns;
 } integrity[NH_PRODUCED_LIMIT-1];
 struct {
  struct nh_key key;
  uint64_t source_ns,next_source_ns,next_begin_ns,next_end_ns,output_begin_ns,output_end_ns;
  size_t nal_bytes,wire_bytes;
 } samples[NH_PRODUCED_LIMIT-1];
};
int nh_run(const struct nh_backend *,const struct camera_fmp4_config *,
           uint8_t *hub_storage,size_t,struct nh_report *);
#endif
