/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef QC07F_RTSP_EXEC_OWNER_H
#define QC07F_RTSP_EXEC_OWNER_H
#include "launch_security.h"
enum pe_phase { PE_EMPTY,PE_PENDING,PE_READY,PE_BOUND,PE_HOLD };
enum pe_error { PE_E_NONE,PE_E_PREFLIGHT,PE_E_CLOCK,PE_E_IMAGE,PE_E_STAGE,
 PE_E_SOCKET,PE_E_FORK,PE_E_CLOSE_UNKNOWN,PE_E_SEND,PE_E_RECEIVE,PE_E_BINDING,
 PE_E_METADATA,PE_E_CHANGED,PE_E_WAIT_UNKNOWN,PE_E_CHILD };
enum pe_result { PE_OK=0,PE_MORE=1,PE_REFUSED=-1 };
/* Stable caller-owned private fields after first begin. Never copy/reset/free/
 * destruct/retry. No aliases/threads/signals/later fork/FD mutation/other reaper;
 * native/media FDs and prior secret history forbidden BEFORE admission. External
 * authorized writer/namespace/image provenance are trusted, not flags proved here.
 * Borrowed config/routes/graph/unused owner/directory/image and external root live
 * through HOLD. Received queue/file/refs/actual child remain retained on failure.
 * close_unknown holds uncertainty, NOT numbers to retry. No cleanup/kill API. */
struct pe_exec_owner {
 const struct pe_exec_owner *self;
 struct candidate_config *config;
 const struct camera_rtsp_registry *routes;
 const struct candidate_prestarted *unused;
 const struct pe_worker_image *image;
 enum pe_phase phase;enum pe_error error;
 int ingress,directory,control[2],stage[4],received[PE_RIGHTS_MAX];
 unsigned received_count,message_received,close_unknown,possible_file;
 pid_t parent,child;unsigned unreaped,wait_unknown,wait_observed,wait0;int wait_status;
 struct stat directory_inode,ingress_inode,control_inode,image_inode,verifier_inode;
 struct candidate_config config_snapshot;
 struct camera_rtsp_registry routes_snapshot;
 struct camera_graph graph_snapshot;
 struct candidate_prestarted unused_snapshot;
 struct pe_worker_image image_snapshot;
 struct pe_bindings binding;
 uint64_t last;
 size_t request_bytes;
 unsigned char request[PE_PACKET],reply[PE_REPLY];
};
#define PE_EXEC_OWNER_INIT { .phase=PE_EMPTY,.ingress=-1,.directory=-1, \
 .control={-1,-1},.stage={-1,-1,-1,-1},.received={-1,-1,-1,-1,-1,-1,-1,-1} }
/* One process-local attempt; moves canonical ingress BEFORE fallible preflight.
 * Dedicated preopened worker FD/policy is BORROWED, never verifier image or path.
 * No parent credential/material reads. No backend/prepare/start is invoked. */
int qc07f_rtsp_exec_begin_v1(struct pe_exec_owner *,int *ingress_slot,int directory,
 struct candidate_config *,const struct camera_rtsp_registry *,
 const struct candidate_prestarted *actual_unused,const struct pe_worker_image *,uint32_t budget_ms);
/* Bounded nonblocking recvmsg + WNOHANG OWN child only. HOLD may collect still-
 * known unreaped child, never retry unknown wait or clear HOLD/close anything. */
int qc07f_rtsp_exec_observe_v1(struct pe_exec_owner *);
/* Rechecks exact public/config/unused-owner/image namespace + received readonly
 * metadata; explicit C slot move only after own wait0/all closes, no forged h. */
int qc07f_rtsp_exec_bind_v1(struct pe_exec_owner *,struct candidate_config *,
 const struct candidate_prestarted *);
#endif
