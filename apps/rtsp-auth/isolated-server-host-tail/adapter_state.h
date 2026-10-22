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
#ifdef __cplusplus
}
#endif
#endif
