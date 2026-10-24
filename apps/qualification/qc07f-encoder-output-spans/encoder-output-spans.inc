/* SPDX-License-Identifier: GPL-2.0-only
 * Included in the actual driver before its ONE H.264 body. All callers hold
 * enc.lock. Only the real new dispatcher branch supplies the validated snapshot.
 */
enum qc_h264_destination_kind { QC_H264_CONTIGUOUS,QC_H264_SPANS };
struct qc_h264_destination {
 enum qc_h264_destination_kind kind;
 const struct qc_hwenc_output_span *spans;
};
static const struct qc_h264_destination qc_h264_contiguous={QC_H264_CONTIGUOUS,NULL};

static int qc_h264_user_range(u32 address,u32 bytes)
{
 /* Conservative u32 endpoint, never a wrapping addition or truncated pointer. */
 if(!address||!bytes||bytes>~(u32)0-address)return -EINVAL;
 if(!access_ok((void __user *)(unsigned long)address,bytes))return -EFAULT;
 return 0;
}
static bool qc_h264_overlap(u32 a,u32 a_bytes,u32 b,u32 b_bytes)
{return (u64)a+(u64)a_bytes>b&&(u64)b+(u64)b_bytes>a;}

static int qc_h264_spans_snapshot(unsigned long argument,struct qc_hwenc_h264_spans_job *job)
{
 u32 i,j,control;
 int result;
 if(!argument||argument>~(u32)0)return -EINVAL;
 control=argument;
 result=qc_h264_user_range(control,sizeof(*job));if(result)return result;
 /* The sole request/descriptor read. No later userspace descriptor reload. */
 if(copy_from_user(job,(void __user *)argument,sizeof(*job)))return -EFAULT;
 if(job->version!=QC_HWENC_OUTPUT_SPANS_VERSION||job->struct_bytes!=sizeof(*job)||
    job->keyframe>1||job->span_count!=QC_HWENC_OUTPUT_SPANS_COUNT||
    job->width!=1280||job->height!=720||job->input_bytes!=1382400||
    job->output_bytes||job->status||job->frame_num)return -EINVAL;
 for(i=0;i<ARRAY_SIZE(job->reserved);i++)if(job->reserved[i])return -EINVAL;
 result=qc_h264_user_range(job->input_ptr,job->input_bytes);if(result)return result;
 for(i=0;i<QC_HWENC_OUTPUT_SPANS_COUNT;i++){
  const struct qc_hwenc_output_span *span=&job->spans[i];
  if(span->bytes!=QC_HWENC_OUTPUT_SPAN_BYTES)return -EINVAL;
  result=qc_h264_user_range(span->user_address,span->bytes);if(result)return result;
  if(qc_h264_overlap(span->user_address,span->bytes,job->input_ptr,job->input_bytes)||
     qc_h264_overlap(span->user_address,span->bytes,control,sizeof(*job)))return -EINVAL;
  for(j=0;j<i;j++)if(qc_h264_overlap(span->user_address,span->bytes,
     job->spans[j].user_address,job->spans[j].bytes))return -EINVAL;
 }
 /* Fixed checked count * fixed checked capacity cannot undercount or overflow. */
 static_assert(QC_HWENC_OUTPUT_SPANS_COUNT*QC_HWENC_OUTPUT_SPAN_BYTES==OUTPUT_LIMIT);
 return 0;
}

static int qc_h264_scatter_copy(const struct qc_hwenc_output_span *spans,u32 bytes,u32 *copied)
{
 u32 i,offset=0;
 *copied=0;
 /* Caller already retired real DMA, dma_rmb'd, checked every guard and admitted
  * 0<bytes<=OUTPUT_LIMIT. Descriptor snapshot/capacities passed preflight.
  * No tail writes, no new output allocation, no retry after a partial fault.
  */
 for(i=0;i<QC_HWENC_OUTPUT_SPANS_COUNT&&offset<bytes;i++){
  u32 chunk=min(bytes-offset,QC_HWENC_OUTPUT_SPAN_BYTES);
  unsigned long left=copy_to_user((void __user *)(unsigned long)spans[i].user_address,
                                enc.output+offset,chunk);
  offset+=chunk-left;*copied=offset;
  if(left)return -EFAULT;
 }
 return offset==bytes?0:-EIO;
}
