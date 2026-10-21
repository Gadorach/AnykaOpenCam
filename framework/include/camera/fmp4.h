/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef CAMERA_FMP4_H
#define CAMERA_FMP4_H
#ifdef __cplusplus
extern "C" {
#endif
#include <camera/backend.h>

#define CAMERA_FMP4_TIMESCALE 90000U
#define CAMERA_FMP4_FRAGMENT_HEADER_BYTES 112U
#define CAMERA_FMP4_MAX_NAL_BYTES 1048576U
#define CAMERA_FMP4_PARAMETER_BYTES 256U
#define CAMERA_FMP4_INIT_BYTES 2048U
#define CAMERA_FMP4_SEND_CHUNK_BYTES 8192U

/* Bounded syntax subset matching the retained QC07F source: constrained
 * baseline42/e0/1f, frame-only, POC type2, CAVLC, no VUI/FMO/extensions.
 * Parsing container metadata is not a hardware capability qualification. */
struct camera_fmp4_profile {
 uint32_t width,height,sps_id,pps_id;
 uint8_t profile,compatibility,level;
};
struct camera_fmp4_config {
 const uint8_t *sps,*pps;
 size_t sps_bytes,pps_bytes;
 uint64_t generation;
 uint32_t duration_ticks; /* 0 for live; finite fixture duration otherwise */
};
struct camera_fmp4_init {
 uint8_t data[CAMERA_FMP4_INIT_BYTES];
 size_t bytes;
 uint64_t generation;
 struct camera_fmp4_profile profile;
};
int camera_fmp4_profile_read(const uint8_t *sps,size_t sps_bytes,
                             const uint8_t *pps,size_t pps_bytes,
                             struct camera_fmp4_profile *profile);
/* Atomic bounded init: no allocation/I/O; parameters must be bare complete NALs,
 * not Annex-B prefixed. New parameters require a new source/profile generation,
 * fresh init/IDR and retirement of all old client exports before restart. */
int camera_fmp4_init_prepare(const struct camera_fmp4_config *config,
                             struct camera_fmp4_init *init);

/* Narrow single-video-track, one complete VCL NAL/sample, no B-frame contract.
 * Config/SPS/PPS init generation and source/profile validation are separate;
 * this formatter does not claim support for arbitrary multi-NAL/B-frame AUs.
 * decode_ticks is relative, monotonically assigned by the owning media service.
 * duration comes from the next qualified timestamp, never a nominal FPS guess.
 */
struct camera_fmp4_sample {
 const uint8_t *nal;
 size_t nal_bytes;
 uint64_t decode_ticks;
 uint32_t duration_ticks,sequence,keyframe;
};
struct camera_fmp4_fragment {
 uint8_t header[CAMERA_FMP4_FRAGMENT_HEADER_BYTES];
 const uint8_t *nal;
 size_t nal_bytes;
};

/* Pure bounded formatting: no allocation, I/O, driver, clock or authorization.
 * Failure leaves output untouched. Output cannot overlap its input description
 * or borrowed payload. Success borrows the original NAL, without copying it.
 * Caller retains its exact producer/encoded lease until all sends or retirement.
 * Every header/payload continuation MUST independently pass auth, expiry/source
 * generation and child-health checks. On denial close/discard safely; do not
 * continue a partial fragment or free/unload unretired source resources.
 * Send header and NAL as bounded chunks; do not combine them into a whole-AU
 * mg_send/allocation. This helper does not enable the currently501 media route.
 */
int camera_fmp4_fragment_prepare(const struct camera_fmp4_sample *sample,
                                 struct camera_fmp4_fragment *fragment);
/* Pure borrowed view, no write or cursor commit. Caller independently gates
 * EACH real send/queued-buffer continuation, then advances only accepted bytes.
 * A denial after partial output closes/discards that connection; do not resume
 * a partial fragment under a new principal/generation. offset==total is done.
 */
int camera_fmp4_fragment_span(const struct camera_fmp4_fragment *fragment,size_t offset,
                              size_t chunk_bytes,const uint8_t **data,size_t *bytes);

struct camera_fmp4_timeline {
 uint64_t generation,decode_ticks;
 uint32_t expected_rtp,next_sequence,initialized;
};
/* Fresh storage only; no resource ownership or automatic profile restart.
 * Caller must secure old CPU storage while obtaining the NEXT real source RTP
 * boundary, without retaining the producer's only output slot. Neither function
 * acquires a DMA lease nor permits publishing over an acquired hub slot.
 * Atomic failure leaves timeline/output untouched. Equal/backward/ambiguous
 * half-range timestamps, stale generation/current timestamp and overflow deny.
 */
int camera_fmp4_timeline_init(struct camera_fmp4_timeline *timeline,
                              uint64_t generation,uint32_t first_rtp);
int camera_fmp4_timeline_step(struct camera_fmp4_timeline *timeline,
                              uint64_t generation,uint32_t current_rtp,uint32_t next_rtp,
                              const uint8_t *nal,size_t nal_bytes,uint32_t keyframe,
                              struct camera_fmp4_fragment *fragment);
#ifdef __cplusplus
}
#endif
#endif
