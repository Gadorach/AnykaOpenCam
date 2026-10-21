/* SPDX-License-Identifier: GPL-2.0-only */
/* Register definitions adapted from Anyka akmci.h, Copyright (C) 2010
 * Anyka, Ltd. Original GPLv2 notice retained in upstream/akmci.h. */
#ifndef QC_MCI_HOST_CORE_H
#define QC_MCI_HOST_CORE_H
#ifdef __KERNEL__
#include <linux/types.h>
typedef u32 qc_u32;
#else
#include <stdint.h>
typedef uint32_t qc_u32;
#endif

#define QC_CLK 0x04
#define QC_ARG 0x08
#define QC_CMD 0x0c
#define QC_RESP0 0x14
#define QC_TIMER 0x24
#define QC_LENGTH 0x28
#define QC_DATA 0x2c
#define QC_COUNT 0x30
#define QC_STATUS 0x34
#define QC_MASK 0x38
#define QC_DMA 0x3c
#define QC_CMD_CRC (1u << 0)
#define QC_DATA_CRC (1u << 1)
#define QC_CMD_TIMEOUT (1u << 2)
#define QC_DATA_TIMEOUT (1u << 3)
#define QC_RESPONSE_END (1u << 4)
#define QC_COMMAND_SENT (1u << 5)
#define QC_DATA_END (1u << 6)
#define QC_STARTBIT (1u << 8)
#define QC_ACTIVE ((1u << 9) | (1u << 10) | (1u << 11))
#define QC_EVENTS 0x17fu
#define QC_RSP_PRESENT 1u
#define QC_RSP_LONG 2u
#define QC_RSP_CRC 4u
#define QC_RSP_BUSY 8u
#define QC_READ 1u
#define QC_WRITE 2u
#define QC_MAX_BYTES 512u
#define QC_DEADLINE_MS 500u
#define QC_MAX_SAMPLES 4096u

enum qc_result {
 QC_OK = 0, QC_DENIED = -1, QC_TIMEOUT = -2, QC_CRC = -3,
 QC_CANCELLED = -4, QC_IO = -5, QC_QUARANTINED = -6, QC_EXHAUSTED = -7
};
struct qc_request {
 qc_u32 opcode, arg, response_flags, data_flags, bytes, blocks;
 qc_u32 response[4];
};
struct qc_engine {
 qc_u32 generation, events, app_pending;
 unsigned int active, poisoned;
};
/* A provider is NOT included. Every callback must meet this bounded contract:
 * claim exclusively reserves the MMC common buffer/channel and shared fraction
 * DMA engine, arbitrates masked shared-register updates, and preserves running
 * UART/other peripheral buffers and clocks; release is legal
 * only after retire returned true. sample consumes only this controller's
 * IRQ snapshot (never guessed W1C). prepare_read owns a private DMA_FROM_DEVICE
 * bounce, never the caller's sg. finish_read returns exact bytes without copy.
 * retire stops controller AND L2/fraction DMA, drains IRQ/callbacks, restores
 * only owned bits, and proves no future bus write to that bounce. Failure
 * retains the provider lease/mapping/bounce until protected reboot.
 * now_ms is wrapping monotonic u32; cancelled includes remove/card removal.
 * relax sleeps/yields <=1ms. No callback may contain an unbounded wait.
 * publish_read copies to caller only after successful retirement/cache sync.
 */
struct qc_io {
 void *ctx;
 int (*claim)(void *);
 void (*release)(void *);
 void (*write)(void *, qc_u32 reg, qc_u32 value);
 qc_u32 (*read)(void *, qc_u32 reg);
 qc_u32 (*sample)(void *, qc_u32 generation);
 qc_u32 (*now_ms)(void *);
 int (*cancelled)(void *);
 void (*relax)(void *);
 int (*prepare_read)(void *, qc_u32 bytes);
 int (*finish_read)(void *, qc_u32 *bytes);
 int (*busy)(void *);
 int (*retire)(void *, qc_u32 generation);
 int (*publish_read)(void *, qc_u32 bytes);
};
int qc_admit(const struct qc_request *, unsigned int app);
qc_u32 qc_command_word(const struct qc_request *);
int qc_clock_div(qc_u32 parent, qc_u32 requested, qc_u32 *bits,
                 qc_u32 *actual);
int qc_irq_event(struct qc_engine *, qc_u32 generation, qc_u32 status);
/* Pure pre-admission gate, also used BEFORE Linux glue schedules new work.
 * Generations never wrap/reuse in one live engine. Active/poisoned state takes
 * precedence; an exhausted idle engine is refused without mutating any state.
 * Do not zero/reinitialize a live engine to evade this namespace fence. */
int qc_request_ready(const struct qc_engine *);
int qc_run(struct qc_engine *, struct qc_request *, const struct qc_io *);
#endif
