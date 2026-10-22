/* SPDX-License-Identifier: GPL-2.0-only
 * Included AFTER the actual frozen native adapter bodies (renamed _legacy).
 * One real singleton/context, no synthetic producer or success callbacks.
 */
#include "native_watchdog.h"
_Static_assert(NH_SESSION_NS==QW_SESSION_NS,"original fixed 10s session");
static unsigned native_initialized;
static int native_guard_failure(void)
{nh_native_fence(NULL);return CAMERA_IO_ERROR;}

void nh_native_init(void)
{
 if(native_initialized){(void)native_guard_failure();return;}
 native_init_legacy();native_initialized=1;
}
int nh_native_watchdog_take(int *slot,dev_t device)
{
 if(!native_initialized||native.retirement_uncertain||source_clock.failed||
    native.continuous_sensor!=-1||native.continuous_frame!=-1||native.h264_stream_fd!=-1||
    native.encoder_hold!=-1||native.produced_storage||native.resident_storage||
    parked_after_stop||memory_state.ledger.initialized)return CAMERA_NOT_READY;
 return qw_owner_take(&native.watchdog,slot,device)?CAMERA_NOT_READY:CAMERA_OK;
}
int nh_native_memory_start(uint64_t start)
{
 /* Keep the original CAMERA_OK / CAMERA_NOT_READY API, not NH_MEM_* reasons.
  * Refuse repeats BEFORE the unchanged initializer can memset accounting. */
 if(stop||native.retirement_uncertain||source_clock.failed||native.watchdog.state==QW_HOLD){
  (void)native_guard_failure();return CAMERA_NOT_READY;
 }
 if(!native_initialized||!native_guard_present()||!storage_selected)return CAMERA_NOT_READY;
 if(memory_state.ledger.initialized||native.watchdog.session_deadline_ns)return CAMERA_NOT_READY;
 if(lifetime()){(void)native_guard_failure();return CAMERA_NOT_READY;}
 /* Bind real-clock-checked original start BEFORE setup; even failed setup cannot
  * obtain a fresh budget. UNKNOWN/future/backward/late time is not a new epoch. */
 if(qw_owner_seal_session(&native.watchdog,start)||native_memory_start_legacy(start)){
  (void)native_guard_failure();return CAMERA_NOT_READY;
 }
 return CAMERA_OK;
}
static int native_guard_begin(enum qw_phase phase)
{
 if(native_initialized&&(stop||native.retirement_uncertain||source_clock.failed))
  return native_guard_failure();
 if(!native_initialized||native.watchdog.fd<3||
    native.watchdog.state==QW_EMPTY||native.watchdog.state==QW_HOLD)
  return CAMERA_NOT_READY;
 if(!provider_shape(&native))return CAMERA_NOT_READY;
 uint64_t start=nh_native_now(NULL);
 if(start==UINT64_MAX||!memory_state.ledger.initialized||
    !native.watchdog.session_deadline_ns||
    memory_state.session_start!=native.watchdog.session_start_ns||
    start<native.watchdog.session_start_ns||start>=native.watchdog.session_deadline_ns)
  return native_guard_failure();
 /* Observe exact adopted roles/global facts BEFORE watchdog control. Charge
  * query cost to the same original outer start; legacy repeats inside guard. */
 if(lifetime())return native_guard_failure();
 if(qw_owner_begin(&native.watchdog,start,phase,native.watchdog.session_deadline_ns))
  return native_guard_failure();
 return CAMERA_OK;
}
int nh_native_next(void *context,struct camera_packet *packet,struct nh_key *key)
{
 /* Trusted caller-owned, stable, disjoint outputs, never private loan/backing. */
 if(packet)memset(packet,0,sizeof(*packet));
 if(key)memset(key,0,sizeof(*key));
 if(!packet||!key)return CAMERA_INVALID;
 /* Sticky uncertainty precedes known BUSY. No clock/FD/control/query/service
  * on the known-loan path, and no private expected/loan/custody mutation. */
 if(stop||native.retirement_uncertain||source_clock.failed||native.watchdog.state==QW_HOLD)
  return native_guard_failure();
 if(native.produced_storage||parked_after_stop)return CAMERA_BUSY;
 /* No lifetime open/query, malloc, sensor/config/capture or codec effect until
  * actual owned guard and original preparation/paid-buffer prerequisites exist. */
 if(!provider_shape(&native)||!sensor_baseline_requested||!storage_selected||!memory_state.ledger.initialized||
    !memory_state.ledger.paid[0]||!memory_state.ledger.paid[1])return CAMERA_NOT_READY;
 int result=native_guard_begin(QW_NEXT);if(result)return result;
 result=native_next_legacy(context,packet,key);
 /* The real legacy body includes BOTH lifetime calls, cancellation/receipt and
  * encoded-begin<2s checks. Its earlier inner guard closes have been removed. */
 if(result||native.retirement_uncertain||source_clock.failed||
    qw_owner_disable_boundary(&native.watchdog)){
  memset(packet,0,sizeof(*packet));memset(key,0,sizeof(*key));
  return native_guard_failure(); /* actual unreturned backing/FDs retained */
 }
 return CAMERA_OK;
}
int nh_native_release(void *context,const struct camera_packet *packet)
{
 if(!packet||!native_initialized||native.retirement_uncertain||source_clock.failed||
    qw_owner_idle_observe(&native.watchdog))return native_guard_failure();
 int result=native_release_legacy(context,packet);
 return result?native_guard_failure():CAMERA_OK;
}
static void native_retired_cpu_release(void)
{
 /* Called ONLY after original normal retirement and successful logical guard
  * disable. A failed disable keeps resident/parked backing in the actual owner. */
 if(native.resident_storage!=raw)free(native.resident_storage);
 native.resident_storage=NULL;parked_after_stop=NULL;memset(&expected,0,sizeof(expected));
}
int nh_native_retire(void *context)
{
 if(native.produced_storage)return CAMERA_BUSY; /* known held CPU loan, no effect */
 /* No invented END/ACK proof for preloaded but never-activated roles. A healthy
  * unactivated handback API is not implemented; retain custody without rearm. */
 if(native.providers.phase==QP_COMMITTED)return CAMERA_NOT_READY;
 int result=native_guard_begin(QW_RETIRE);if(result)return result;
 result=native_retire_body(context);
 if(result||native.retirement_uncertain||source_clock.failed||
    qw_owner_disable_boundary(&native.watchdog))return native_guard_failure();
 native_retired_cpu_release();
 return CAMERA_OK;
}
int nh_native_retire_held(void *context,const struct camera_packet *packet)
{
 if(native.retirement_uncertain||source_clock.failed||parked_after_stop)return native_guard_failure();
 if(packet){
  if(!native.produced_storage||packet->data!=expected.data||packet->bytes!=expected.bytes||
     packet->monotonic_ns!=expected.monotonic_ns)return native_guard_failure();
 }else if(native.produced_storage)return native_guard_failure();
 if(native.providers.phase==QP_COMMITTED)return CAMERA_NOT_READY;
 int result=native_guard_begin(QW_RETIRE);if(result)return result;
 result=native_retire_held_legacy(context,packet);
 if(result||native.retirement_uncertain||source_clock.failed||
    qw_owner_disable_boundary(&native.watchdog))return native_guard_failure();
 native_retired_cpu_release();
 return CAMERA_OK;
}
int nh_native_watchdog_give_back(int *slot)
{
 if(!native_initialized||native.retirement_uncertain||source_clock.failed||
    native.providers.sensor_staged!=-1||native.providers.frame_staged.fd!=-1||
    native.providers.encoder_staged.fd!=-1||native.providers.phase==QP_MOVING||native.providers.phase==QP_HOLD||
    native.receipt_armed||native.continuous_frame!=-1||native.continuous_sensor!=-1||
    native.h264_stream_fd!=-1||native.encoder_hold!=-1||native.produced_storage||
    native.resident_storage||parked_after_stop||native.close_unknown)return CAMERA_NOT_READY;
 return qw_owner_give_back(&native.watchdog,slot)?CAMERA_NOT_READY:CAMERA_OK;
}
