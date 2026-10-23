/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef QC_NATIVE_WATCHDOG_OWNER_H
#define QC_NATIVE_WATCHDOG_OWNER_H
#include <sys/types.h>
#include "native_adapter.h"
/* Fresh singleton init -> take UNKNOWN actual preopened watchdog -> old explicit
 * storage/exact139/memory preparation -> bounded next/release/normal retire.
 * Opening occurred elsewhere and is already an effect. Sole actual description,
 * This derivative additionally requires mandatory provider adoption AFTER storage/
 * exact139 opt-in and BEFORE memory setup; see owned-provider.h for real staged
 * transfer/partial-HOLD semantics. No preloaded-FD setup guard is disabled.
 * no aliases or external operations; exact core/driver/image/current recovery and
 * provider/backing admission remain trusted prerequisites, NOT metadata proof.
 * No native acceptance or permission to call any old allocation/capture helper.
 * Memory start keeps CAMERA_OK/CAMERA_NOT_READY; setup is one-shot, using a real
 * monotonic start no later than the actual clock and less than2s old. That start
 * seals the sole10s deadline, including setup cost, even if setup later fails.
 * Repeated setup refuses without touching original memory accounting/budget.
 * All arguments/slots/outputs must be stable and disjoint from each other and
 * native private owner/expected packet/backing and any still-owned loan bytes.
 * This is a trusted caller contract, NOT a runtime alias/custody proof. NEXT
 * clears each non-NULL output before refusals; known held/parked loan is BUSY
 * without clock/control/query, after sticky uncertainty/HOLD precedence.
 */
int nh_native_watchdog_take(int *bootstrap_slot,dev_t expected_device);
/* Only after successful logical disable + known local source normal retirement,
 * no source/codec FDs/loans/resident backing left. Custody move, not final close,
 * electrical stop, DMA proof or permission to dispose other owned resources. */
int nh_native_watchdog_give_back(int *recipient_slot);
#endif
