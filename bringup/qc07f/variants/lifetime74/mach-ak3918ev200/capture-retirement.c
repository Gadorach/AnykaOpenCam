/* SPDX-License-Identifier: GPL-2.0-only */
#include "capture-retirement.h"
#ifdef __KERNEL__
#include <linux/errno.h>
#include <linux/string.h>
#else
#include <errno.h>
#include <string.h>
#endif
#define QC_U64_MAX (~(__u64)0)
static int valid(const struct qc_retirement_file *f)
{return f&&f->source&&f->source->id&&f->id;}
static int held(const struct qc_retirement_file *f)
{return valid(f)&&f->source->hold==f&&f->generation;}
int qc_retirement_source_init(struct qc_retirement_registry *r,struct qc_retirement_source *s)
{
 if(!r||!s)return -EINVAL;
 if(r->next_source==QC_U64_MAX)return -EOVERFLOW;
 memset(s,0,sizeof(*s));s->id=++r->next_source;return 0;
}
int qc_retirement_open(struct qc_retirement_source *s,struct qc_retirement_file *f)
{
 if(!s||!s->id||!f)return -EINVAL;
 if(s->next_file==QC_U64_MAX||s->open_files==~0U)return -EOVERFLOW;
 memset(f,0,sizeof(*f));f->source=s;f->id=++s->next_file;f->open=1;s->open_files++;return 0;
}
int qc_retirement_admission(const struct qc_retirement_source *s)
{return !s||!s->id?-EINVAL:s->hold?-EBUSY:0;}
static int input_clean(const struct qc_capture_receipt *q)
{
 return q->version==QC_CAPTURE_RECEIPT_VERSION&&!q->reserved&&!q->reserved2&&
  !q->state&&!q->reason&&!q->stop_result&&!q->stopped_ns;
}
static int binding(const struct qc_retirement_file *f,const struct qc_capture_receipt *q)
{return q->source_instance==f->source->id&&q->file_instance==f->id;}
static void reply(const struct qc_retirement_file *f,struct qc_capture_receipt *q)
{
 memset(q,0,sizeof(*q));q->version=QC_CAPTURE_RECEIPT_VERSION;
 q->source_instance=f->source->id;q->file_instance=f->id;q->generation=f->generation;
 q->state=f->state;q->reason=f->reason;q->stop_result=f->stop_result;q->stopped_ns=f->stopped_ns;
}
int qc_retirement_ioctl(struct qc_retirement_file *f,unsigned long cmd,const struct qc_retirement_uaccess *u)
{
 struct qc_capture_receipt q;
 if(!valid(f)||!u||!u->from_user||!u->to_user)return -EINVAL;
 if(!f->open)return -EBADF;
 if(cmd!=QC_ISP_RECEIPT_BIND&&cmd!=QC_ISP_RECEIPT_ARM&&cmd!=QC_ISP_RECEIPT_QUERY&&cmd!=QC_ISP_RECEIPT_ACK)return -ENOTTY;
 if(u->from_user(u->context,&q))return -EFAULT;
 if(!input_clean(&q))return -EINVAL;
 if(cmd==QC_ISP_RECEIPT_BIND){
  if(q.source_instance||q.file_instance||q.generation)return -EINVAL;
  reply(f,&q);return u->to_user(u->context,&q)?-EFAULT:0;
 }
 if(!binding(f,&q))return -ESTALE;
 if(cmd==QC_ISP_RECEIPT_ARM){
  if(q.generation)return -EINVAL;
  if(f->state!=QC_RECEIPT_NONE||f->source->hold)return -EBUSY;
  if(f->source->next_generation==QC_U64_MAX)return -EOVERFLOW;
  __u64 generation=++f->source->next_generation; /* never recycle even on EFAULT */
   reply(f,&q);q.generation=generation;q.state=QC_RECEIPT_ARMED;
   q.reason=QC_STOP_NONE;q.stop_result=0;q.stopped_ns=0;
  /* Publish generation BEFORE any hardware begin. A bad reply must not reserve
   * the owner or run/reset hardware. Begin subsequently needs the committed ARM.
   */
  if(u->to_user(u->context,&q))return -EFAULT;
   f->generation=generation;f->state=QC_RECEIPT_ARMED;
   f->reason=QC_STOP_NONE;f->stop_result=0;f->stopped_ns=0;
   f->source->hold=f;return 0;
 }
 if(!held(f)||q.generation!=f->generation)return -ESTALE;
 if(cmd==QC_ISP_RECEIPT_QUERY){
  reply(f,&q);return u->to_user(u->context,&q)?-EFAULT:0; /* NEVER resets/consumes */
 }
 if(f->state!=QC_RECEIPT_RETIRED||f->stop_result)return -EBUSY;
 /* ACK only consumes a proven receipt. Copy-from and validation precede commit;
  * no copy-to or hardware side effect. A successor invalidates this old key.
  */
 f->source->hold=NULL;f->generation=0;f->state=QC_RECEIPT_NONE;
 f->reason=QC_STOP_NONE;f->stop_result=0;f->stopped_ns=0;return 0;
}
int qc_retirement_begin(struct qc_retirement_file *f,const struct qc_retirement_ops *ops)
{
 if(!held(f)||!f->open)return -ESTALE;
 if(!ops||!ops->begin)return -EINVAL;
 if(f->state!=QC_RECEIPT_ARMED)return -EBUSY;
 f->state=QC_RECEIPT_STARTING;
 int r=ops->begin(ops->context);
 if(r>0)r=-EIO;
 f->state=r?QC_RECEIPT_UNCERTAIN:QC_RECEIPT_ACTIVE;
 f->stop_result=r?-EINPROGRESS:0; /* no stop/reset attempt yet; never infer proof */
 return r;
}
int qc_retirement_stop_enter(struct qc_retirement_file *f,enum qc_capture_stop_reason reason,struct qc_retirement_fence *fence)
{
 if(!held(f))return -ESTALE;
 if(!fence)return -EINVAL;
 if(reason<QC_STOP_END||reason>QC_STOP_CLOSE)return -EINVAL;
 if(f->state!=QC_RECEIPT_ACTIVE&&f->state!=QC_RECEIPT_UNCERTAIN)return -EBUSY;
 fence->source_instance=f->source->id;fence->file_instance=f->id;fence->generation=f->generation;
 f->state=QC_RECEIPT_DRAINING;f->reason=reason;f->stopped_ns=0;return 0;
}
int qc_retirement_stop_finish(struct qc_retirement_file *f,const struct qc_retirement_fence *fence,int reset_result,__u64 now)
{
 if(!held(f)||f->state!=QC_RECEIPT_DRAINING)return -ESTALE;
 if(!fence||fence->source_instance!=f->source->id||fence->file_instance!=f->id||fence->generation!=f->generation)return -ESTALE;
 if(reset_result>0)reset_result=-EIO;
 /* This function is a PRIVATE integration callpoint, only after the entire
  * existing mask/synchronize/reset function returns, not user-supplied errno.
  * EPERM/timeout/etc can never mint success or discard the retained context.
  */
 f->stop_result=reset_result;f->state=reset_result?QC_RECEIPT_UNCERTAIN:QC_RECEIPT_RETIRED;
 f->stopped_ns=reset_result?0:now;return reset_result;
}
int qc_retirement_stop(struct qc_retirement_file *f,enum qc_capture_stop_reason why,const struct qc_retirement_ops *ops)
{
 if(!ops||!ops->stop_reset||!ops->clock_ns)return -EINVAL;
 struct qc_retirement_fence fence;int r=qc_retirement_stop_enter(f,why,&fence);if(r)return r;
 r=ops->stop_reset(ops->context);
 return qc_retirement_stop_finish(f,&fence,r,ops->clock_ns(ops->context));
}
int qc_retirement_close(struct qc_retirement_file *f,const struct qc_retirement_ops *ops)
{
 if(!valid(f)||!f->open)return -EBADF;
 int r=0;
 if(held(f)&&f->state==QC_RECEIPT_ACTIVE)r=qc_retirement_stop(f,QC_STOP_CLOSE,ops);
 else if(held(f)&&f->state!=QC_RECEIPT_ARMED&&f->state!=QC_RECEIPT_RETIRED)
  r=f->stop_result?f->stop_result:-EBUSY;
 if(r){
  f->open=0;f->source->open_files--;
  return r; /* source hold quarantines CLOSED context/backing; never free */
 }
 if(held(f))f->source->hold=NULL; /* ARMED never started; RETIRED reset proven */
 f->open=0;f->source->open_files--;return 0;
}
int qc_retirement_file_releasable(const struct qc_retirement_file *f)
{return valid(f)&&!f->open&&f->source->hold!=f;}
int qc_retirement_source_releasable(const struct qc_retirement_source *s)
{return s&&s->id&&!s->open_files&&!s->hold;}
