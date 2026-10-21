/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef QC_USERLAND_CAPTURE_RECEIPT_H
#define QC_USERLAND_CAPTURE_RECEIPT_H
#include "capture-retirement-uapi.h"
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
/* This is an additive copied public ABI. It does not replace isp-stream.h.
 * A successful syscall and exact counters are prerequisites, not auth tokens.
 * Nonzero syscall replies are NEVER inspected, including partial-copy EFAULT.
 */
static inline struct qc_capture_receipt qc_receipt_request(const struct qc_capture_receipt *key)
{
 struct qc_capture_receipt q;memset(&q,0,sizeof(q));q.version=QC_CAPTURE_RECEIPT_VERSION;
 if(key){q.source_instance=key->source_instance;q.file_instance=key->file_instance;q.generation=key->generation;}
 return q;
}
static inline int qc_receipt_same(const struct qc_capture_receipt *a,const struct qc_capture_receipt *b)
{return a->source_instance==b->source_instance&&a->file_instance==b->file_instance&&a->generation==b->generation;}
static inline int qc_receipt_header(const struct qc_capture_receipt *q)
{return q->version==QC_CAPTURE_RECEIPT_VERSION&&!q->reserved&&!q->reserved2&&q->source_instance&&q->file_instance;}
static inline int qc_receipt_query(int fd,const struct qc_capture_receipt *key,struct qc_capture_receipt *out)
{
 struct qc_capture_receipt q=qc_receipt_request(key);
 if(ioctl(fd,QC_ISP_RECEIPT_QUERY,&q))return -1;
 if(!qc_receipt_header(&q)||!qc_receipt_same(&q,key)||q.state>QC_RECEIPT_UNCERTAIN||q.reason>QC_STOP_CLOSE){errno=EPROTO;return -1;}
 *out=q;return 0;
}
static inline int qc_receipt_retired(const struct qc_capture_receipt *q)
{return q->state==QC_RECEIPT_RETIRED&&!q->stop_result&&q->reason>=QC_STOP_END&&q->reason<=QC_STOP_CLOSE&&q->stopped_ns;}
#endif
