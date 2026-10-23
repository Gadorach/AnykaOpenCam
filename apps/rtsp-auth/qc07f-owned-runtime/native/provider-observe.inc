/* SPDX-License-Identifier: GPL-2.0-only
 * Real observational refusals for the actual adopted descriptions, not grants.
 */
static int provider_terminal(const struct qc_rtsp_context *c)
{return c->providers.phase==QP_RETURNING||c->providers.phase==QP_RETURNED;}
static void provider_hold(struct qc_rtsp_context *c)
{
 if(c->providers.phase!=QP_RETURNED)c->providers.phase=QP_HOLD;
 if(c->providers.frame_staged.fd>=3)c->providers.frame_staged.state=QC_FRAME_FD_HOLD;
 if(c->providers.encoder_staged.fd>=3)c->providers.encoder_staged.state=QC_ENCODER_FD_HOLD;
}
static int provider_shape(const struct qc_rtsp_context *c)
{
 return (c->providers.phase==QP_COMMITTED||c->providers.phase==QP_ACTIVE)&&
  h264_p_frames==1&&width==1280&&height==720&&
  c->continuous_sensor>=3&&c->continuous_frame>=3&&c->h264_stream_fd>=3&&
  c->providers.sensor_staged==-1&&c->providers.frame_staged.fd==-1&&
  c->providers.encoder_staged.fd==-1&&c->providers.sensor_device&&
  c->providers.frame_identity.source_instance&&c->providers.frame_identity.file_instance&&
  c->providers.encoder_identity.session_identity;
}
static int provider_node(int fd,int mode,dev_t device,dev_t filesystem,ino_t inode)
{
 struct stat st;int flags=fcntl(fd,F_GETFL),fdflags=fcntl(fd,F_GETFD);
 return fd>=3&&flags>=0&&fdflags>=0&&!fstat(fd,&st)&&(flags&O_ACCMODE)==mode&&
  (fdflags&FD_CLOEXEC)&&S_ISCHR(st.st_mode)&&st.st_rdev==device&&
  st.st_dev==filesystem&&st.st_ino==inode;
}
static int provider_store_frame(struct qc_rtsp_context *c,const struct qc_capture_receipt *q)
{
 if(!qc_receipt_header(q)||q->state>QC_RECEIPT_UNCERTAIN||q->reason>QC_STOP_CLOSE||
    q->source_instance!=c->providers.frame_identity.source_instance||
    q->file_instance!=c->providers.frame_identity.file_instance)return -1;
 c->providers.frame_last=*q;return 0;
}
static int provider_receipt_query(struct qc_rtsp_context *c,const struct qc_capture_receipt *key,
 struct qc_capture_receipt *out)
{
 if(qc_receipt_query(c->continuous_frame,key,out))return -1;
 return provider_store_frame(c,out);
}
static int provider_encoder_observe(struct qc_rtsp_context *c)
{
 struct qp_roles *p=&c->providers;
 if(!provider_node(c->h264_stream_fd,O_RDWR,p->encoder_identity.device,
                   p->encoder_filesystem,p->encoder_inode))return capture_failed(c);
 struct qc_hwenc_open_query q={0};
 if(ioctl(c->h264_stream_fd,QC_HWENC_OPEN_QUERY,&q))return capture_failed(c);
 /* Same complete query schema as imported custody. No partial reply, old global
  * sequence fallback, relation/telemetry guess or saturation uniqueness claim. */
 if(q.version!=QC_HWENC_OPEN_QUERY_VERSION||q.struct_bytes!=sizeof(q)||q.reserved||
    !q.session_identity||q.session_identity!=p->encoder_identity.session_identity||
    q.reference_frame_num>65535||q.quarantined>1||q.dma_uncertain>1||
    q.guard_poisoned>1||q.ledger_active>1)return capture_failed(c);
 switch(q.reference_relation){
 case QC_HWENC_REFERENCE_NONE:
  if(q.reference_owner_identity||q.reference_frame_num)return capture_failed(c);break;
 case QC_HWENC_REFERENCE_THIS_OPEN:
  if(q.reference_owner_identity!=q.session_identity)return capture_failed(c);break;
 case QC_HWENC_REFERENCE_OTHER_OPEN:
  if(!q.reference_owner_identity||q.reference_owner_identity==q.session_identity)return capture_failed(c);break;
 default:return capture_failed(c);
 }
 p->encoder_last=q; /* retain valid THIS/OTHER/fault snapshot BEFORE refusing */
 if(q.quarantined||q.dma_uncertain||q.guard_poisoned||q.ledger_active||
    (q.telemetry_flags&~QC_STAGE_F_SATURATED))return capture_failed(c);
 if(c->h264_stream_frames){
  if(q.reference_relation!=QC_HWENC_REFERENCE_THIS_OPEN||q.reference_frame_num!=p->codec_frame_num)
   return capture_failed(c);
 }else if(q.reference_relation!=QC_HWENC_REFERENCE_NONE)return capture_failed(c);
 return CAMERA_OK;
}
static int provider_observe(struct qc_rtsp_context *c)
{
 struct qp_roles *p=&c->providers;
 if(!provider_shape(c)||!provider_node(c->continuous_sensor,O_RDWR,p->sensor_device,
       p->sensor_filesystem,p->sensor_inode)||
    !provider_node(c->continuous_frame,O_RDONLY,p->frame_identity.device,
       p->frame_filesystem,p->frame_inode))return capture_failed(c);
 struct qc_capture_receipt bound=qc_receipt_request(NULL);
 if(ioctl(c->continuous_frame,QC_ISP_RECEIPT_BIND,&bound)||provider_store_frame(c,&bound))
  return capture_failed(c);
 if(p->phase==QP_COMMITTED){
  if(bound.generation||bound.state!=QC_RECEIPT_NONE||bound.reason||bound.stop_result||bound.stopped_ns)
   return capture_failed(c);
 }else if(!c->receipt_armed||!qc_receipt_same(&bound,&c->receipt_key)||
    bound.state!=QC_RECEIPT_ACTIVE||bound.reason||bound.stop_result||bound.stopped_ns)
  return capture_failed(c);
 return provider_encoder_observe(c);
}
