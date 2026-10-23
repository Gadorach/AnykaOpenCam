/* SPDX-License-Identifier: GPL-2.0-only
 * Included after the real native/watchdog wrappers. Stable singleton custody.
 */
#include "owned-provider.h"
int nh_native_provider_take(int *sensor_slot,dev_t sensor_device,
 int *frame_slot,const struct qc_frame_fd_identity *frame,
 int *encoder_slot,const struct qc_encoder_fd_identity *encoder)
{
 if(!sensor_slot||!frame_slot||!encoder_slot||!frame||!encoder||!sensor_device||
    !frame->device||!frame->source_instance||!frame->file_instance||
    !encoder->device||!encoder->session_identity)return CAMERA_INVALID;
 if(!native_initialized||stop||native.retirement_uncertain||source_clock.failed||
    native.watchdog.state!=QW_UNKNOWN||native.watchdog.fd<3||
    native.providers.phase!=QP_EMPTY||memory_state.ledger.initialized||
    !storage_selected||!sensor_baseline_requested||h264_p_frames!=1||width!=1280||height!=720||
    native.continuous_sensor!=-1||native.continuous_frame!=-1||native.h264_stream_fd!=-1||
    native.providers.sensor_staged!=-1||native.providers.frame_staged.fd!=-1||
    native.providers.encoder_staged.fd!=-1||native.produced_storage||native.resident_storage||
    parked_after_stop)return CAMERA_NOT_READY;
 if(*sensor_slot<3||*frame_slot<3||*encoder_slot<3||*sensor_slot==*frame_slot||
    *sensor_slot==*encoder_slot||*frame_slot==*encoder_slot||
    *sensor_slot==native.watchdog.fd||*frame_slot==native.watchdog.fd||
    *encoder_slot==native.watchdog.fd)return CAMERA_INVALID;
 /* GET-only sensor metadata; NOT I2C_SLAVE/register reads or permission. */
 struct stat sensor;
 int flags=fcntl(*sensor_slot,F_GETFL),fdflags=fcntl(*sensor_slot,F_GETFD);
 if(flags<0||fdflags<0||fstat(*sensor_slot,&sensor)||(flags&O_ACCMODE)!=O_RDWR||
    !(fdflags&FD_CLOEXEC)||!S_ISCHR(sensor.st_mode)||sensor.st_rdev!=sensor_device)
  return CAMERA_NOT_READY; /* no role moved */
 struct qp_roles *p=&native.providers;
 p->sensor_device=sensor_device;p->sensor_filesystem=sensor.st_dev;p->sensor_inode=sensor.st_ino;
 p->frame_identity=*frame;p->encoder_identity=*encoder;
 /* First helper failure leaves ALL bootstrap slots owned by bootstrap. Its
  * uncommitted reply is not exposed; no native ownership/snapshot is invented. */
 if(qc_encoder_fd_take(&p->encoder_staged,encoder_slot,&p->encoder_identity))
  return native_guard_failure();
 p->phase=QP_MOVING;p->encoder_last=p->encoder_staged.last_observed;
 p->encoder_filesystem=p->encoder_staged.node_filesystem;p->encoder_inode=p->encoder_staged.node_inode;
 if(qc_frame_fd_take(&p->frame_staged,frame_slot,&p->frame_identity))return native_guard_failure();
 p->frame_last=p->frame_staged.last_observed;
 p->frame_filesystem=p->frame_staged.node_filesystem;p->frame_inode=p->frame_staged.node_inode;
 p->sensor_staged=*sensor_slot;*sensor_slot=-1;
 /* Actual move, not alias/copy. Every destination slot is explicit -1. */
 native.continuous_sensor=p->sensor_staged;p->sensor_staged=-1;
 if(qc_frame_fd_give_back(&p->frame_staged,&native.continuous_frame)){
  p->frame_last=p->frame_staged.last_observed;return native_guard_failure();
 }
 if(qc_encoder_fd_give_back(&p->encoder_staged,&native.h264_stream_fd)){
  p->encoder_last=p->encoder_staged.last_observed;return native_guard_failure();
 }
 p->phase=QP_COMMITTED;
 /* Reobserve actual native slots after staged records became EMPTY. No role
  * pointer/qualified flag stands in for actual tuple/metadata observations. */
 return lifetime(); /* actual per-open AND original global facts, no effect */
}
