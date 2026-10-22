/* SPDX-License-Identifier: GPL-2.0-only
 * Narrow source integration, not a camera main or admitted native provider.
 * native_adapter.c and its generated rtspd-native-import.c are immutable run34
 * v6 inputs selected/pinned by build.py. Including the original C source keeps
 * its single static owner: no second capture/encoder stack or payload backing.
 */
#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
#include "native_adapter.c"
#include "rtsp_native_source.h"

static int wired;
static int (*rtsp_guard)(void *);
static void *rtsp_guard_context;

static int prerequisites(void)
{
 if(native.retirement_uncertain||source_clock.failed)return CAMERA_IO_ERROR;
 if(stop||!storage_selected||!sensor_baseline_requested||
    !memory_state.ledger.initialized)return CAMERA_NOT_READY;
 return CAMERA_OK;
}

static void bind_guard(void *context,int (*guard)(void *),void *guard_context)
{
 (void)context; /* v6 is one process-local singleton, not a selectable provider. */
 if(guard&&rtsp_guard){nh_native_fence(NULL);return;}
 rtsp_guard=guard;
 rtsp_guard_context=guard?guard_context:NULL;
}

static int logical_start(void *context)
{
 (void)context;
 int result=prerequisites();
 if(result)return result;
 /* The server binds its actual current-peer CHECK gate. START is logical only:
  * no receipt, sensor, capture, allocation, lifetime query or codec operation. */
 return rtsp_guard?rtsp_guard(rtsp_guard_context):CAMERA_NOT_READY;
}

static int next_packet(void *context,uint32_t force,struct camera_packet *packet)
{
 (void)context;
 if(!packet||force>1)return CAMERA_INVALID;
 int result=prerequisites();
 if(result)return result;
 if(!rtsp_guard||rtsp_guard(rtsp_guard_context)!=CAMERA_OK)return CAMERA_NOT_READY;

 /* Exact nh_native_next sequence from v6, except forwarding the real RTSP
  * keyframe demand to the ALREADY implemented encoder job. The old finite
  * nh_native_next always passed zero. No receipt/generation is fabricated. */
 if(lifetime())return CAMERA_IO_ERROR;
 result=produce_h264_frame(&native,force,packet);
 if(result){nh_native_fence(NULL);return result;}
 if(lifetime())return CAMERA_IO_ERROR;
 if(nh_native_cancelled(NULL))return CAMERA_IO_ERROR;
 if(native.produced_timing.encoded-native.produced_timing.begin>=2000){
  nh_native_fence(NULL);return CAMERA_IO_ERROR;
 }
 expected=*packet;
 return CAMERA_OK;
}

static void release_packet(void *context,const struct camera_packet *packet)
{
 (void)context;
 /* RTSP release is void, native release returns status. Never lose that error
  * and then report a successful common END. This adds no release/free retry. */
 if(!packet||nh_native_release(NULL,packet)!=CAMERA_OK)nh_native_fence(NULL);
}

static int frame_end(void *context,struct camera_rtsp_frame_handle source,
                     int result,uint32_t keyframe,uint64_t now)
{
 /* Server copied scalar facts BEFORE release; no packet/payload is used here.
  * The original common helper retains exact frame/keyframe operation identity. */
 if(native.retirement_uncertain||source_clock.failed)result=CAMERA_IO_ERROR;
 return camera_rtsp_common_end(context,source,result,keyframe,now);
}

int qc07f_rtsp_native_wire(struct camera_rtsp_producer *output,
                         struct camera_rtsp_lifecycle *lifecycle,
                         struct camera_rtsp_common_producer *bridge,
                         struct camera_producer *producer,
                         struct candidate_config *config)
{
 if(!output||!lifecycle||!bridge||!producer||!config)return CAMERA_INVALID;
 if(wired||!producer->initialized||producer->state!=CAMERA_PRODUCER_STOPPED||
    producer->shutdown||producer->demand_count||producer->active_present||
    !producer->config.keyframe_supported)return CAMERA_NOT_READY;
 /* v6's static zero defaults are NOT an initialized owner (FD0 is real).
  * Require its explicit nh_native_init state, with no existing device or loan.
  * Never initialize/reset somebody else's static owner on the caller's behalf. */
 if(native.h264_stream_fd!=-1||native.continuous_sensor!=-1||
    native.continuous_frame!=-1||native.watchdog_hold!=-1||
    native.encoder_hold!=-1||native.legacy_control!=-1||
    native.retirement_uncertain||native.produced_storage||native.resident_storage||
    source_clock.failed)return CAMERA_NOT_READY;
 /* Actual retained codec SPS/PPS generator, no synthetic parameter sets. */
 struct camera_fmp4_config parameters_config;
 int result=nh_native_config(&parameters_config);
 if(result)return result;
 size_t bytes=parameters_config.sps_bytes+parameters_config.pps_bytes+8;
 if(bytes>sizeof(parameters))return CAMERA_INVALID;

 *bridge=(struct camera_rtsp_common_producer){.producer=producer,
  .start=logical_start,.idle=nh_native_retire,.clock_ns=nh_native_now};
 *lifecycle=camera_rtsp_common_hooks(bridge);
 lifecycle->end=frame_end;
 *output=(struct camera_rtsp_producer){.next=next_packet,.release=release_packet,
  .parameter_sets=parameters,.parameter_set_bytes=bytes,.lifecycle=lifecycle};
 config->source_guard_bind=bind_guard;
 wired=1;
 return CAMERA_OK; /* callback wiring ONLY, never provider/native readiness */
}
