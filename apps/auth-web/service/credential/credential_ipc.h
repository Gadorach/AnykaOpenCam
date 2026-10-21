/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef CREDENTIAL_IPC_H
#define CREDENTIAL_IPC_H
#include "credential_worker.h"
#define CP_FD 3
#define CP_HEADER 64U
#define CP_PAYLOAD_MAX 256U
#define CP_FRAME_MAX (CP_HEADER+CP_PAYLOAD_MAX)
enum cp_wire_kind {CP_READY=1,CP_INIT,CP_PUT,CP_REVOKE,CP_VERIFY,CP_ACK,CP_RESULT,CP_SHUTDOWN};
enum cp_hardening {CP_NODUMP=1U,CP_LOCKED=2U,CP_FILTERED=4U,CP_FDS_CLOSED=8U,CP_FIXTURE=16U};
struct cp_message {
 uint32_t kind,domain,status,bytes;
 uint64_t epoch,sequence,request_generation,credential_generation,deadline;
 unsigned char payload[CP_PAYLOAD_MAX];
};
/* Authoritative status-returning clock. On failure leaves output unchanged;
 * no synthetic zero/MAX time. No sentinel-valued production clock API. */
int cp_clock_read(uint64_t *);
#ifdef CP_TEST_ONLY
uint64_t cp_now(void); /* legacy fixture convenience, never authority/callback */
#endif
void cp_wipe(void *,size_t);
void cp_u32(unsigned char *,uint32_t);
void cp_u64(unsigned char *,uint64_t);
uint32_t cp_get32(const unsigned char *);
uint64_t cp_get64(const unsigned char *);
int cp_id_valid(const char [CAMERA_ID_BYTES]);
void cp_pack_material(unsigned char *,const struct cw_ui_material *);
void cp_unpack_material(struct cw_ui_material *,const unsigned char *);
/* SOCK_SEQPACKET: one canonical explicit-length frame per atomic send.
 * 0 success, 1 would-block, -1 EOF/error/malformed/ancillary/truncation.
 * Never logs; receive closes ALL delivered unexpected SCM_RIGHTS FDs. */
int cp_send(int,const struct cp_message *);
int cp_receive(int,struct cp_message *);
#endif
