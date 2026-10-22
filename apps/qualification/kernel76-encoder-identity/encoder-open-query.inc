/* SPDX-License-Identifier: GPL-2.0-only
 * Included AFTER the real enc/session/trace definitions, before encoder_ioctl.
 * No mock/shadow provider. Both ioctl and final file release use enc.lock.
 */
static long encoder_open_query(struct file *file,unsigned long argument)
{
 struct qc_hwenc_open_query query={0};
 const struct qc_encoder_session *session,*reference=NULL;
 const struct qc_stage_context *trace;
 int result;
 if(mutex_lock_interruptible(&enc.lock))return -ERESTARTSYS;
 session=file->private_data;
 /* Open initializes this before publishing the FD. Its VFS reference keeps
  * the session alive through this ioctl; final release takes the same lock. */
 if(!session||!session->identity){result=-EBADF;goto out;}
 if(enc.h264_owner){
  /* Owner is cleared under this lock BEFORE its private session is freed. */
  reference=enc.h264_owner->private_data;
  if(!reference||!reference->identity){result=-EIO;goto out;}
 }
 trace=qc_codec_trace();
 /* Init installs telemetry before misc_register. No supported unload/reprobe
  * can reset the boot-lifetime session namespace or free this provider. */
 if(!trace){result=-EIO;goto out;}
 query.version=QC_HWENC_OPEN_QUERY_VERSION;query.struct_bytes=sizeof(query);
 query.session_identity=session->identity;
 if(reference){
  query.reference_owner_identity=reference->identity;
  query.reference_relation=enc.h264_owner==file?
   QC_HWENC_REFERENCE_THIS_OPEN:QC_HWENC_REFERENCE_OTHER_OPEN;
  query.reference_frame_num=enc.h264_frame_num;
 }
 query.reference_generation=trace->owner_generation;
 query.quarantined=enc.quarantined;query.dma_uncertain=enc.dma_uncertain;
 query.guard_poisoned=enc.guard_poisoned;query.telemetry_flags=trace->flags;
 query.last_dispatch_identity=enc.active_session;
 query.operation_sequence=trace->operations;
 query.last_operation_result=trace->last.result;
 query.last_operation_kind=trace->last.kind;query.ledger_active=trace->active;
 /* Fixed bounded copy only. No counter, owner, reference, reset or MMIO write;
  * failed/partial copy is EFAULT and supplies no usable snapshot to the caller. */
 result=copy_to_user((void __user *)argument,&query,sizeof(query))?-EFAULT:0;
out:
 mutex_unlock(&enc.lock);return result;
}
