/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef QC_NATIVE_WATCHDOG_OWNER_H
#define QC_NATIVE_WATCHDOG_OWNER_H
#include <sys/types.h>
#include "native_adapter.h"
/* Fresh singleton init -> take UNKNOWN actual preopened watchdog -> old explicit
 * storage/exact139/memory preparation -> bounded next/release/normal retire.
 * Opening occurred elsewhere and is already an effect. Sole actual description,
 * no aliases or external operations; exact core/driver/image/current recovery and
 * provider/backing admission remain trusted prerequisites, NOT metadata proof.
 * No native acceptance or permission to call any old allocation/capture helper.
 */
int nh_native_watchdog_take(int *bootstrap_slot,dev_t expected_device);
/* Only after successful logical disable + known local source normal retirement,
 * no source/codec FDs/loans/resident backing left. Custody move, not final close,
 * electrical stop, DMA proof or permission to dispose other owned resources. */
int nh_native_watchdog_give_back(int *recipient_slot);
#endif
