/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef MOTION_CANDIDATE_H
#define MOTION_CANDIDATE_H
#include <camera/access_policy.h>
#ifdef __cplusplus
extern "C" {
#endif
#define MM_SCHEMA 1U /* settings key motion.tile-luma.v1; no implicit old-mask migration */
#define MM_COLS 32U
#define MM_ROWS 18U
#define MM_CELLS (MM_COLS*MM_ROWS)
#define MM_MASK_BYTES (MM_CELLS/8U)
#define MM_POLYS 8U
#define MM_VERTICES 16U
#define MM_STORAGE_CAP 4096U
#define MM_CALLBACK_NS 1000000ULL
enum mm_result {MM_OK=0,MM_EMPTY=1,MM_BUSY=2,MM_INVALID=-1,MM_DENIED=-2,MM_STALE=-3,MM_UNAVAILABLE=-4,MM_RETAINED=-5,MM_EXHAUSTED=-6};
enum mm_status {MM_NO_CONFIG=0,MM_BASELINE,MM_QUIET,MM_CANDIDATE,MM_MOVING,MM_INHIBITED,MM_UNKNOWN,MM_QUARANTINE};
enum mm_reason {MM_NONE=0,MM_CONFIG_CHANGE,MM_PRIVACY,MM_SOURCE,MM_CLOCK,MM_GAP,
 MM_EXPOSURE,MM_GLOBAL_LUMA,MM_PTZ,MM_DAYNIGHT,MM_DIGEST,MM_COVERAGE,MM_QUEUE_FULL,MM_COUNTER,MM_RETURN_UNKNOWN};
enum mm_kind {MM_BEGIN=1,MM_END,MM_TERMINATED_GAP};
enum mm_flags {MM_INHIBIT_PTZ=1U,MM_INHIBIT_DAYNIGHT=2U,MM_INHIBIT_EXPOSURE=4U};
struct mm_point {uint16_t x,y;};
struct mm_polygon {uint32_t vertices;struct mm_point point[MM_VERTICES];};
struct mm_geometry {uint16_t native_w,native_h,crop_x,crop_y,crop_w,crop_h,display_w,display_h;uint32_t flip_x,flip_y;};
struct mm_rational_point {int64_t x_num,y_num;uint32_t x_den,y_den;};
struct mm_config {
 uint32_t schema,polygon_count;uint64_t generation;
 struct mm_polygon polygons[MM_POLYS];
 uint32_t delta_min,noise_multiplier,area_permille,global_delta,global_permille;
 uint64_t debounce_ns,release_ns,min_event_ns,max_gap_ns,event_ttl_ns;
};
struct mm_identity {char sensor[CAMERA_ID_BYTES],profile[CAMERA_ID_BYTES],head[CAMERA_ID_BYTES];
 uint64_t graph_epoch,source_generation,track_generation,head_epoch;struct mm_geometry geometry;};
/* Mean/noise in RAW luma-code units0..255. Pixels is exact whole-cell native
 * crop coverage, NOT an arbitrary vote/claimed complete frame. No RGB pixels. */
struct mm_cell {uint16_t pixels;uint8_t mean,noise;};
struct mm_ticket {uint64_t namespace_epoch,source_generation,sequence;};
struct mm_span {struct mm_ticket ticket;struct mm_identity identity;const struct mm_cell *cells;
 uint64_t frame_sequence,sampled_ns,expires_ns,exposure_epoch,inhibit_epoch;
 uint32_t cell_count,digest_crc32,inhibit_flags,valid,native_qualified;};
/* Trusted cached stats-owner callbacks only, never deserialized from a client.
 * Resource admission BEFORE acquire; actual shared policy is checked in unit.
 * acquire0 registers a CPU-readable immutable exact loan;1=no loan; negative
 * uncertainty retains context/backing. If uncertainty lacks a known ticket,
 * consumer cannot invent a forced return. release0 is normal exact consumer
 * return ACK, not DMA STOP/free. Other results quarantine; no automatic retry.
 * Every external callback is bounded in SOFTWARE using actual pre/post clocks:
 * <=1ms (equality allowed). A delayed normal acquire still owns a known loan
 * until checked cancel; delayed no-loan never fabricates ownership. A delayed
 * normal release ACK is preserved exactly once, with fault/no further acquire,
 * not relabelled uncertain DMA. No portable callback preemption is implied.
 * After the final external callback, direct real policy proof is required with
 * no subsequent callback before effect; statistics/background always need LIVE
 * even if an EVENTS check just succeeded. Clock callbacks are trusted read-only
 * time sources, not wire flags or permission outputs.
 */
struct mm_provider {void *context;uint64_t (*clock_ns)(void *);
 int (*admit)(void *,const struct mm_identity *,uint64_t,uint64_t *valid_until_ns);
 int (*acquire)(void *,const struct mm_identity *,struct mm_span *);
 int (*valid)(void *,const struct mm_span *,uint64_t);
 int (*release)(void *,const struct mm_ticket *,uint64_t);};
struct mm_event {uint64_t namespace_epoch,sequence,source_generation,config_generation,
 started_ns,observed_ns,expires_ns;uint32_t kind,reason,native_qualified;};
struct motion_engine {
 struct motion_engine *self;struct mm_provider provider;struct camera_access_policy *policy;
 struct camera_access_principal observer,pending_admin;
 struct mm_identity identity;struct mm_config config,pending_config;struct mm_span span;
 uint8_t background[MM_CELLS],staging[MM_CELLS],mask[MM_MASK_BYTES],pending_mask[MM_MASK_BYTES];
 struct mm_event events[2];
 uint64_t namespace_epoch,next_event,last_ns,last_frame_ns,last_frame_sequence,exposure_epoch,
 candidate_ns,active_ns,quiet_ns,highest_config_generation;
 uint32_t configured,pending,have_background,have_frame,candidate,active,quiet,
 owned,uncertain,return_attempted,event_count,status,reason,busy,fault,native_qualified,changed_permille,tiles_read;
};
int mm_config_validate(const struct mm_config *,const struct mm_geometry *);
int mm_mask_rasterize(const struct mm_config *,const struct mm_geometry *,uint8_t mask[MM_MASK_BYTES]);
int mm_display_to_sensor(const struct mm_geometry *,struct mm_point,struct mm_rational_point *);
int mm_sensor_to_display(const struct mm_geometry *,struct mm_point,struct mm_rational_point *);
uint32_t mm_stats_digest(const struct mm_cell cells[MM_CELLS]); /* trusted complete-array helper, NOT source acquisition */
int mm_init(struct motion_engine *,const struct mm_provider *,struct camera_access_policy *,
 const struct camera_access_principal *observer,const struct camera_graph *,const struct mm_identity *,uint64_t namespace_epoch,uint64_t now_ns);
int mm_stage_config(struct motion_engine *,const struct camera_access_principal *admin,const struct mm_config *,uint64_t now_ns);
int mm_apply_config(struct motion_engine *,uint64_t now_ns); /* between loans only; recheck pending admin before effect */
int mm_poll(struct motion_engine *,uint64_t now_ns); /* actual READ before acquire, each tile, background/event effect */
int mm_cancel(struct motion_engine *,uint64_t now_ns); /* exact own-consumer safety cleanup, no source stop/reset */
int mm_event_peek(struct motion_engine *,uint64_t now_ns,struct mm_event *);
int mm_event_take(struct motion_engine *,const struct mm_event *,uint64_t now_ns);
/* Internal owner-only observations/events. NO file writer/scheduler/delete,
 * retention/SD/network/public notify or registered wire source callback. Fresh
 * qualified native cheap statistics, tuning and recording/NVR remain blockers.
 * Complete accessible typed objects; caller serializes policy/source/effects.
 */
#ifdef __cplusplus
}
#endif
#endif
