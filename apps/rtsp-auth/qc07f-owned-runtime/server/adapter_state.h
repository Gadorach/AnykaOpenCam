/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef IS_ADAPTER_STATE_H
#define IS_ADAPTER_STATE_H
#include "verifier.h"
#ifdef __cplusplus
extern "C" {
#endif
/*Not auth/liveness. Pending is cleared ONLY after reviewed rv_complete checks
 *actual result + exact RETIRE_RESULT ACK. Needed because WIRE_MISSING/RV_LOST
 *and WIRE_SPACE/RV_UNCERTAIN share numeric return codes in V2.*/
int is_rv_pending_result(const struct rv_client *);
size_t is_rv_context_bytes(void); /*public allocation shape only, no credential contents*/
int is_rv_wait0(const struct rv_client *); /*actual OWN kernel wait, not stop request*/
/* Scoped owned-child wait observation only: no policy/media grant, signals,
 * retention guarantee or ability to prevent exit after the observation. */
int is_rv_running(struct rv_client *);
/* Read-only scalar diagnostic, independent of rv_retired's actual child exit.
 * Stable/disjoint caller output; former number is NOT an owned description and
 * NEVER usable for retry/close/I/O. RV_UNCERTAIN on sticky close error even when
 * fd was cleared and actual child exit is known. No native/DMA retirement grant.
 */
struct is_rv_ipc_close {unsigned attempted;int last_fd_not_owned,error;};
int is_rv_ipc_close_outcome(const struct rv_client *,struct is_rv_ipc_close *);
#ifdef __cplusplus
}
#endif
#endif
