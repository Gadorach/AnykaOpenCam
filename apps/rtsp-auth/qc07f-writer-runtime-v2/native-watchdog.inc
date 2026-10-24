/* SPDX-License-Identifier: GPL-2.0-only
 * Bounded derivative of owned-runtime native-watchdog.inc. Real guard/setup and
 * normal retirement unchanged; old contiguous producer routes refuse outright.
 */
_Static_assert(NH_SESSION_NS==QW_SESSION_NS,"fixed 10s session");
static unsigned native_initialized;
static int native_guard_failure(void)
{nh_native_fence(NULL);return CAMERA_IO_ERROR;}
void nh_native_init(void)
{
 if(native_initialized||!runtime_span_history_empty()){(void)native_guard_failure();return;}
 native_init_legacy();native_initialized=1;
}
int nh_native_watchdog_take(int *slot,dev_t device)
{
 if(!runtime_span_history_empty()||!native_initialized||native.retirement_uncertain||source_clock.failed||provider_terminal(&native)||
    native.continuous_sensor!=-1||native.continuous_frame!=-1||native.h264_stream_fd!=-1||
    native.encoder_hold!=-1||native.produced_storage||native.resident_storage||
    parked_after_stop||memory_state.ledger.initialized)return CAMERA_NOT_READY;
 return qw_owner_take(&native.watchdog,slot,device)?CAMERA_NOT_READY:CAMERA_OK;
}
int nh_native_memory_start(uint64_t start)
{
 if(stop||native.retirement_uncertain||source_clock.failed||native.watchdog.state==QW_HOLD){
  (void)native_guard_failure();return CAMERA_NOT_READY;
 }
 if(!runtime_span_history_empty()||!native_initialized||!native_guard_present()||!storage_selected)return CAMERA_NOT_READY;
 if(memory_state.ledger.initialized||native.watchdog.session_deadline_ns)return CAMERA_NOT_READY;
 if(lifetime()){(void)native_guard_failure();return CAMERA_NOT_READY;}
 if(qw_owner_seal_session(&native.watchdog,start)||native_memory_start_legacy(start)){
  (void)native_guard_failure();return CAMERA_NOT_READY;
 }
 return CAMERA_OK;
}
static int native_guard_begin(enum qw_phase phase)
{
 if(native_initialized&&(stop||native.retirement_uncertain||source_clock.failed))return native_guard_failure();
 if(!native_initialized||native.watchdog.fd<3||native.watchdog.state==QW_EMPTY||native.watchdog.state==QW_HOLD)return CAMERA_NOT_READY;
 if(!provider_shape(&native))return CAMERA_NOT_READY;
 uint64_t start=nh_native_now(NULL);
 if(start==UINT64_MAX||!memory_state.ledger.initialized||!native.watchdog.session_deadline_ns||
    memory_state.session_start!=native.watchdog.session_start_ns||
    start<native.watchdog.session_start_ns||start>=native.watchdog.session_deadline_ns)return native_guard_failure();
 if(lifetime())return native_guard_failure();
 if(qw_owner_begin(&native.watchdog,start,phase,native.watchdog.session_deadline_ns))return native_guard_failure();
 return CAMERA_OK;
}
int nh_native_next_keyframe(void *c,uint32_t force,struct camera_packet *p,struct nh_key *k)
{(void)c;(void)force;(void)p;(void)k;return CAMERA_NOT_READY;}
int nh_native_next(void *c,struct camera_packet *p,struct nh_key *k)
{(void)c;(void)p;(void)k;return CAMERA_NOT_READY;}
int nh_native_release(void *c,const struct camera_packet *p)
{(void)c;(void)p;return CAMERA_NOT_READY;}
int nh_native_retire_held(void *c,const struct camera_packet *p)
{(void)c;(void)p;return CAMERA_NOT_READY;}
static void native_retired_cpu_release(void)
{
 /* There is NO resident encoded allocation and hub backing is never native
  * storage. Do not free an unexpected pointer or clear a typed failed loan. */
 if(writer_native_busy()||native.resident_storage||native.produced_storage||parked_after_stop)
  nh_native_quarantine();
 memset(&expected,0,sizeof(expected));
}
int nh_native_retire(void *context)
{
 if(writer_native_busy()||native.produced_storage||native.resident_storage||parked_after_stop)return CAMERA_BUSY;
 if(native.providers.phase==QP_COMMITTED)return CAMERA_NOT_READY;
 int result=native_guard_begin(QW_RETIRE);if(result)return result;
 result=native_retire_body(context);
 if(result||native.retirement_uncertain||source_clock.failed||qw_owner_disable_boundary(&native.watchdog))return native_guard_failure();
 native_retired_cpu_release();return CAMERA_OK;
}
int nh_native_watchdog_give_back(int *slot)
{
 /* This source profile has no actual frontend borrow-end, so a paid/attempted
  * borrow cannot be surrendered through an unrelated watchdog API. */
 if(!runtime_span_history_empty()||!native_initialized||native.retirement_uncertain||source_clock.failed||provider_terminal(&native)||
    native.providers.sensor_staged!=-1||native.providers.frame_staged.fd!=-1||
    native.providers.encoder_staged.fd!=-1||native.providers.phase==QP_MOVING||native.providers.phase==QP_HOLD||
    native.receipt_armed||native.continuous_frame!=-1||native.continuous_sensor!=-1||
    native.h264_stream_fd!=-1||native.encoder_hold!=-1||native.produced_storage||
    native.resident_storage||parked_after_stop||native.close_unknown)return CAMERA_NOT_READY;
 return qw_owner_give_back(&native.watchdog,slot)?CAMERA_NOT_READY:CAMERA_OK;
}
