/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef QC07F_RTSP_PROVISIONER_PROCESS_H
#define QC07F_RTSP_PROVISIONER_PROCESS_H
#include "../rtsp-private-handoff/private_handoff.h"
#include <sys/types.h>

#define RTSP_PP_VERSION 1U
#define RTSP_PP_INGRESS_HEADER 16U
#define RTSP_PP_RIGHTS_MAX 8U
/* Private pipe frame: four little-endian u32: version, username bytes(1..64),
 * realm bytes(1..64), password bytes(1..128), then those bytes and EXACT EOF.
 * Independent authorized RTSP input ONLY; no default values or selectors.
 * No library helper accepts/encodes credentials in the parent address space. */
enum rtsp_pp_phase { RTSP_PP_EMPTY,RTSP_PP_PENDING,RTSP_PP_READY,
 RTSP_PP_BOUND,RTSP_PP_HOLD };
enum rtsp_pp_error { RTSP_PP_E_NONE,RTSP_PP_E_PREFLIGHT,RTSP_PP_E_CLOCK,
 RTSP_PP_E_SOCKET,RTSP_PP_E_FORK,RTSP_PP_E_CLOSE_UNKNOWN,RTSP_PP_E_RECEIVE,
 RTSP_PP_E_ENVELOPE,RTSP_PP_E_METADATA,RTSP_PP_E_WAIT_UNKNOWN,
 RTSP_PP_E_CHILD_FAILED,RTSP_PP_E_TIMEOUT,RTSP_PP_E_CHANGED };
enum rtsp_pp_result { RTSP_PP_OK=0,RTSP_PP_MORE=1,RTSP_PP_REFUSED=-1 };

/* Public-only fixed same-fork/source-ABI envelope; NEVER username/realm/HA1,
 * credential payload lengths, material readback or a credential-derived hash. */
struct rtsp_pp_envelope {
 uint64_t boot_generation,epoch,credential_generation,account,file_device,file_inode;
 uint32_t version,build_class,material_abi,material_bytes,domain,algorithm;
 uint32_t ttl,window,global_verifies,global_challenges,account_verifies;
 uint32_t account_challenges,peer_verifies,peer_challenges;
 char device[64],account_id[64],sensor[64],profile[64],live[513],track[513];
 unsigned char reserved[6]; /* explicit zero tail, not indeterminate C padding */
};

/* Stable caller storage, private fields after begin; never copy/reset/destruct.
 * See CONTRACT.md: trusted exclusive ingress/namespace, no aliases/threads/signals
 * or competing reapers, no secret/native/media ownership before fork. These are
 * real mandatory preconditions, NOT proven by metadata or permission booleans.
 * Borrowed directory/config/routes/unused owner outlive HOLD and future consumer.
 * Every nonnegative FD slot has canonical ownership, except borrowed directory.
 * close_unknown mask records uncertainty, NOT retryable numeric descriptors.
 * child_pid is ONLY this object's actual fork result; unreaped=0 is historical.
 * On HOLD retain object, borrowed directory, possible partial fixed-name file,
 * FDs/queue and actual child lifetime. No implicit cleanup/retry or scope exit.
 */
struct rtsp_provisioner_process {
 const struct rtsp_provisioner_process *self;
 struct candidate_config *config;
 const struct camera_rtsp_registry *routes;
 const struct candidate_prestarted *unused_owner;
 enum rtsp_pp_phase phase;
 enum rtsp_pp_error error;
 int directory_fd,ingress_fd,sockets[2],received[RTSP_PP_RIGHTS_MAX];
 unsigned received_count,message_received,close_unknown,possible_file;
 pid_t child_pid;
 unsigned child_unreaped,wait_unknown,wait_observed,wait0;
 int wait_status;
 uint64_t started_ms,last_ms,deadline_ms;
 struct stat directory_inode;
 struct candidate_config config_snapshot;
 struct camera_rtsp_registry routes_snapshot;
 struct rtsp_pp_envelope expected,received_envelope;
};
#define RTSP_PROVISIONER_PROCESS_INIT { .phase=RTSP_PP_EMPTY, \
 .directory_fd=-1,.ingress_fd=-1,.sockets={-1,-1}, \
 .received={-1,-1,-1,-1,-1,-1,-1,-1} }

#ifdef RTSP_HANDOFF_HOST_PUBLICDUMMY
#define rtsp_provisioner_begin qc07f_rtsp_publicdummy_provisioner_begin_v1
#define rtsp_provisioner_observe qc07f_rtsp_publicdummy_provisioner_observe_v1
#define rtsp_provisioner_bind qc07f_rtsp_publicdummy_provisioner_bind_v1
#else
#define rtsp_provisioner_begin qc07f_rtsp_provisioner_begin_v1
#define rtsp_provisioner_observe qc07f_rtsp_provisioner_observe_v1
#define rtsp_provisioner_bind qc07f_rtsp_provisioner_bind_v1
#endif

/* ONE process-local attempt. Moves *ingress_slot to owner BEFORE preflight/fork;
 * after successful fork parent closes that alias once, reads ZERO credentials.
 * Even preflight/fork failure retains moved ownership; caller MUST NOT close its
 * old number. Refusal before a valid fresh owner leaves caller slot unchanged.
 * budget_ms 1..10000 is nominal CLOCK_MONOTONIC polling budget, not a watchdog.
 * Forks ONE device-free provisioner, never starts verifier/runtime/listener.
 */
int rtsp_provisioner_begin(struct rtsp_provisioner_process *,int *ingress_slot,
 int directory_fd,struct candidate_config *,const struct camera_rtsp_registry *,
 const struct candidate_prestarted *actual_unused_owner,uint32_t budget_ms);
/* Constant bounded work per call: nonblocking receive and owned WNOHANG waitpid;
 * caller may nominally poll. HOLD calls only collect still-known OWN child, never
 * retry unknown wait/ECHILD, clear HOLD, close resources or authorize binding.
 */
int rtsp_provisioner_observe(struct rtsp_provisioner_process *);
/* Explicit ONE bind at READY, immediately before caller's device-free prepare/
 * start. Rechecks actual same unused config/owner/metadata; moves received slot
 * into real config.material_fd. No forged handoff, backend call or auto cleanup.
 */
int rtsp_provisioner_bind(struct rtsp_provisioner_process *,
 struct candidate_config *,const struct candidate_prestarted *);
#endif
