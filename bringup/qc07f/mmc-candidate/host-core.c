/* SPDX-License-Identifier: GPL-2.0-only */
/* Command/data register encoding adapted from linux-3.4.35 akmci.c/h:
 * Copyright (C) 2010 Anyka, Ltd, All Rights Reserved.
 * GNU General Public License version 2. See upstream/ originals.
 * New read-only admission, bounded completion and retirement policy. */
#include "host-core.h"

int qc_admit(const struct qc_request *r, unsigned int app)
{
 if (!r || r->opcode > 63 || r->response_flags & ~15u ||
     ((r->response_flags & (QC_RSP_LONG | QC_RSP_CRC | QC_RSP_BUSY)) &&
      !(r->response_flags & QC_RSP_PRESENT)))
  return QC_DENIED;
 if (r->data_flags & ~QC_READ || r->blocks > 1 || r->bytes > QC_MAX_BYTES)
  return QC_DENIED;
 if (!!r->data_flags != !!r->bytes || (!!r->bytes && r->blocks != 1))
  return QC_DENIED;
 if (!r->bytes && r->blocks)
  return QC_DENIED;
 if (app) {
  switch (r->opcode) {
  case 6: return !r->bytes && r->arg == 0 ? QC_OK : QC_DENIED;
  case 13: return r->bytes == 64 && r->arg == 0 ? QC_OK : QC_DENIED;
  case 41: /* No S18R/voltage switch request, only standard SD OCR/HCS. */
   return !r->bytes && !(r->arg & ~0x40ff8000u) ? QC_OK : QC_DENIED;
  case 51: return r->bytes == 8 && r->arg == 0 ? QC_OK : QC_DENIED;
  default: return QC_DENIED;
  }
 }
 switch (r->opcode) {
 case 0: return !r->bytes && r->arg == 0 ? QC_OK : QC_DENIED;
 case 2: case 3:
  return !r->bytes && r->arg == 0 ? QC_OK : QC_DENIED;
 case 7: case 9: case 10: case 13: case 55:
  return !r->bytes && !(r->arg & 0xffffu) ? QC_OK : QC_DENIED;
 case 8: return !r->bytes && r->arg == 0x1aa ? QC_OK : QC_DENIED;
 case 16: return !r->bytes && r->arg == 512 ? QC_OK : QC_DENIED;
 case 17: return r->bytes == 512 ? QC_OK : QC_DENIED;
 case 6: /* SD SWITCH_FUNC query only, never switch/forced high-speed. */
  return r->bytes == 64 && r->arg == 0x00fffff0u ? QC_OK : QC_DENIED;
 default: return QC_DENIED; /* Includes erase/write/lock/vendor/MMC/SDIO. */
 }
}

qc_u32 qc_command_word(const struct qc_request *r)
{
 qc_u32 value = 1u | (r->opcode << 1);
 if (r->response_flags & QC_RSP_PRESENT) value |= 1u << 7;
 if (r->response_flags & QC_RSP_LONG) value |= 1u << 8;
 if (r->bytes) value |= 1u << 11;
 return value;
}

int qc_clock_div(qc_u32 parent, qc_u32 requested, qc_u32 *bits,
                 qc_u32 *actual)
{
 qc_u32 cycles, low, high;
 if (!parent || !bits || !actual || requested > 400000u)
  return QC_DENIED;
 if (!requested) { *bits = 0; *actual = 0; return QC_OK; }
 cycles = parent / requested + !!(parent % requested);
 if (cycles < 2) cycles = 2;
 if (cycles > 512) return QC_DENIED;
 low = (cycles - 2) / 2;
 high = cycles - 2 - low;
 *bits = low | (high << 8) | (1u << 16);
 /* SDK f_min/f_max use sum; SDK bus_clock multiplication is inconsistent. */
 *actual = parent / (low + high + 2);
 return QC_OK;
}

int qc_irq_event(struct qc_engine *e, qc_u32 generation, qc_u32 status)
{
 if (!e || !e->active || e->poisoned || generation != e->generation)
  return 0;
 e->events |= status & QC_EVENTS;
 return !!(status & QC_EVENTS);
}

int qc_request_ready(const struct qc_engine *e)
{
 if (!e) return QC_DENIED;
 if (e->poisoned) return QC_QUARANTINED;
 if (e->active) return QC_DENIED;
 if (e->generation == ~(qc_u32)0) return QC_EXHAUSTED;
 return QC_OK;
}

static int qc_wait(struct qc_engine *e, const struct qc_io *io,
                   qc_u32 start, qc_u32 terminal, qc_u32 *status)
{
 unsigned int n;
 for (n = 0; n < QC_MAX_SAMPLES; n++) {
  if (io->cancelled(io->ctx)) return QC_CANCELLED;
  if ((qc_u32)(io->now_ms(io->ctx) - start) >= QC_DEADLINE_MS)
   return QC_TIMEOUT;
  qc_irq_event(e, e->generation, io->sample(io->ctx, e->generation));
  *status = e->events;
  if (*status & terminal) return QC_OK;
  io->relax(io->ctx);
 }
 return QC_TIMEOUT; /* Broken/stopped clock cannot make the wait unbounded. */
}

int qc_run(struct qc_engine *e, struct qc_request *r, const struct qc_io *io)
{
 qc_u32 start, status = 0, transferred = 0, command_terminal;
 unsigned int i;
 int result;
 if (!e || !io || !r) return QC_DENIED;
 result = qc_request_ready(e);
 if (result) return result;
 result = qc_admit(r, e->app_pending);
 e->app_pending = 0; /* Every attempted next command consumes CMD55 context. */
 if (result) return result;
 if (!io->claim || !io->release || !io->write || !io->read ||
     !io->sample || !io->now_ms || !io->cancelled || !io->relax ||
     !io->prepare_read || !io->finish_read || !io->busy ||
     !io->retire || !io->publish_read) return QC_DENIED;
 if (io->claim(io->ctx)) return QC_IO;
 /* qc_request_ready fenced exhaustion before claim; max is the final valid
  * transaction generation. Never skip zero and reuse an earlier namespace. */
 e->generation++;
 e->events = 0;
 e->active = 1;
 start = io->now_ms(io->ctx);
 io->write(io->ctx, QC_MASK, 0);
 io->write(io->ctx, QC_CMD, 0);
 io->write(io->ctx, QC_DATA, 0);
 io->write(io->ctx, QC_DMA, 0);
 /* Drain old controller snapshots with IRQ synchronized by the provider. */
 (void)io->sample(io->ctx, e->generation);
 result = QC_CANCELLED;
 if (io->cancelled(io->ctx)) goto retire;
 if (r->bytes) {
  result = QC_IO;
  if (io->prepare_read(io->ctx, r->bytes)) goto retire;
  io->write(io->ctx, QC_TIMER, 0x800000u); /* Not the software deadline. */
  io->write(io->ctx, QC_LENGTH, r->bytes);
  /* Provider owns L2 assignment and DMACTRL; no guessed buffer number. */
  io->write(io->ctx, QC_DATA, 3u | (r->bytes << 16)); /* 1-bit/read */
 }
 io->write(io->ctx, QC_ARG, r->arg);
 io->write(io->ctx, QC_MASK, QC_EVENTS);
 io->write(io->ctx, QC_CMD, qc_command_word(r));
 command_terminal = QC_CMD_TIMEOUT | QC_CMD_CRC |
  ((r->response_flags & QC_RSP_PRESENT) ? QC_RESPONSE_END : QC_COMMAND_SENT);
 result = qc_wait(e, io, start, command_terminal, &status);
 if (result) goto retire;
 if (status & QC_CMD_TIMEOUT) { result = QC_TIMEOUT; goto retire; }
 if ((status & QC_CMD_CRC) && (r->response_flags & QC_RSP_CRC)) {
  result = QC_CRC; goto retire;
 }
 /* CRC-less R3 may terminate with CRCFAIL, matching vendor command IRQ. */
 for (i = 0; i < 4; i++) r->response[i] = io->read(io->ctx, QC_RESP0 + 4*i);
 if (r->bytes) {
  result = qc_wait(e, io, start,
                   QC_DATA_END | QC_DATA_TIMEOUT | QC_DATA_CRC | QC_STARTBIT,
                   &status);
  if (result) goto retire;
  if (status & QC_DATA_TIMEOUT) { result = QC_TIMEOUT; goto retire; }
  if (status & (QC_DATA_CRC | QC_STARTBIT)) { result = QC_CRC; goto retire; }
  result = QC_IO;
  if (io->finish_read(io->ctx, &transferred) || transferred != r->bytes ||
      io->read(io->ctx, QC_COUNT)) goto retire;
 }
 if (r->response_flags & QC_RSP_BUSY) {
  for (i = 0; i < QC_MAX_SAMPLES; i++) {
   int busy;
   result = QC_CANCELLED;
   if (io->cancelled(io->ctx)) goto retire;
   result = QC_TIMEOUT;
   if ((qc_u32)(io->now_ms(io->ctx) - start) >= QC_DEADLINE_MS) goto retire;
   busy = io->busy(io->ctx);
   if (busy < 0) { result = QC_IO; goto retire; }
   if (!busy) break;
   io->relax(io->ctx);
  }
  if (i == QC_MAX_SAMPLES) { result = QC_TIMEOUT; goto retire; }
 }
 result = QC_OK;
retire:
 io->write(io->ctx, QC_MASK, 0);
 io->write(io->ctx, QC_CMD, 0);
 io->write(io->ctx, QC_DATA, 0);
 io->write(io->ctx, QC_DMA, 0);
 /* Mask/stop writes are NOT retirement. No release/copy after failed proof. */
 e->active = 0;
 if (!io->retire(io->ctx, e->generation)) {
  e->poisoned = 1;
  return QC_QUARANTINED;
 }
 if (!result && io->cancelled(io->ctx)) result = QC_CANCELLED;
 if (!result && (qc_u32)(io->now_ms(io->ctx) - start) >= QC_DEADLINE_MS)
  result = QC_TIMEOUT;
 if (!result && r->bytes && io->publish_read(io->ctx, r->bytes)) result = QC_IO;
 io->release(io->ctx);
 if (!result && r->opcode == 55 && (r->response[0] & (1u << 5)) &&
     !(r->response[0] & 0xfdffe008u)) /* R1 error bits, not APP_CMD/state */
  e->app_pending = 1;
 return result;
}
