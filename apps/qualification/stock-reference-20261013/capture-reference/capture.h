#ifndef CR_CAPTURE_H
#define CR_CAPTURE_H
#include <signal.h>
#include "wire.h"

/* Fixed candidate with real syscall adapter; native entry is still unconditionally
 * HOLD pending parent order/clock/instance admission. No launch flag bypass. */
enum cr_op { CR_ENTRY, CR_READ_TUNING, CR_IR_BASELINE, CR_OPEN_ISP, CR_OPEN_VIDEO,
    CR_ALLOC, CR_EXPORT, CR_MAP_RW, CR_PFN_CHUNK, CR_TUNING_BLOCK, CR_ISP_SET,
    CR_REF_ALLOC, CR_REF_EXPORT, CR_REF_MAP_RW, CR_REF_PFN_CHUNK, CR_REF_INSTALL,
    CR_PRE_SENSOR_CHECK, CR_LOAD_SENSOR, CR_SET_MAIN,
    CR_SET_SUB, CR_GEOMETRY, CR_REQUEST, CR_QUEUE, CR_START, CR_POLL,
    CR_DEQUEUE, CR_READ_SENSOR, CR_IR_SET, CR_IR_VERIFY, CR_STOP_STREAM };
/* Raw completion timeval and clock_ms must share a proved monotonic epoch,
 * units and wrap-free interval. No REALTIME conversion or dequeue-time fallback.
 * The explicitly admitted symmetric error bound includes quantization, epoch
 * correspondence and observation error. It is NOT inferred from HZ or dequeue
 * latency; uncertainty is added to the lower sample boundary, never waived. */
struct cr_entry {
    int root_child,stock_instance_pins,fd3_closed;
    int source_clock_admitted;
    uint32_t source_clock_error_us;
};
#define CR_TUNING_PATH "/usr/local/isp_sc1346.conf"
#define CR_IR_PATH "/sys/user-gpio/gpio-ir"
struct cr_file { uint8_t *data; size_t capacity,length; int root_regular_nofollow_same_inode; const char *path; };
struct cr_mapping { uint8_t *data; uint32_t user_base; size_t length; };
struct cr_pages { uint32_t first,count; uint64_t entries[32]; };
struct cr_block { uint32_t id,offset,length; const uint8_t *data; };
struct cr_binding { uint16_t block,offset,bytes; uint32_t request; };
struct cr_set { uint32_t request,bytes; const uint8_t *data; };
struct cr_load { struct cr_sensor wire; const uint8_t *payload; size_t bytes; };
struct cr_sensor_read { uint16_t reg,value; };
/* sub_evidence1 = exact source-bound successful SET; NOT a cached GET. */
struct cr_geometry { uint32_t main_w,main_h,sub_w,sub_h,sub_evidence; };
struct cr_ops {
    void *opaque;
    int (*call)(void *,enum cr_op,void *);
    int (*clock_ms)(void *,uint64_t *);
    int (*cancelled)(void *);
    int (*emit)(void *,const char *,size_t); /* bounded <=512-byte pieces */
    void (*retained_stop)(void *); /* native must not return; MOCK may return */
#ifdef CR_HOST
    uint8_t synthetic_sha[32]; /* compile-time host-only fixture trust root */
#endif
};
struct cr_phase {
    uint8_t thumb[CR_THUMB];
    uint32_t histogram[16],crc,fold,mean_x1000,ordinal,slot,generation,sequence;
    uint32_t samples;
    uint32_t flat,zero;
    uint64_t verified_transition_ms;
    uint32_t verified_level,verified;
    int32_t sec,usec;
    uint16_t sensor_before[CR_REGS],sensor_after[CR_REGS];
};
struct cr_state {
    uint8_t blob[CR_BLOB];
    struct cr_phase phase[3];
    uint32_t crc_table[256],block_offsets[24],slot_gen[4],slot_seq[4];
    uint64_t slot_stamp[4],last_stamp,start_ms,last_ms,stream_ms;
    uint8_t *mapping;
    uint8_t *reference_mapping;
    uint32_t reference_user_base,reference_first_pfn,reference_last_pfn;
    struct cr_reference reference;
    uint32_t user_base,ordinal,gaps,first_pfn,last_pfn,output_bytes;
    uint32_t source_clock_error_us;
    int source_clock_admitted;
    int retained,active_attempt,ir_attempt,ir_restored,ir_baseline,stream_attempt;
    int cancel_seen,reason; /* source/error codes, never optical acknowledgement */
};
_Static_assert(sizeof(struct cr_state)<131072, "single fixed CPU context under128KiB");
extern const uint16_t cr_sensor_regs[CR_REGS];
extern const uint16_t cr_block_lengths[24];
extern const struct cr_binding cr_bindings[34];
int cr_validate_blob(const uint8_t *,size_t,const uint8_t[32],uint32_t[24]);
int cr_validate_pfn(uint64_t,uint32_t,uint32_t,uint32_t *);
int cr_make_reference(uint32_t,uint32_t,struct cr_reference *);
int cr_validate_dq(struct cr_state *,const struct cr_buffer *);
/* -1=unadmitted/future/overflow,0=drain only,1=source completed in verified phase. */
int cr_frame_attribution(const struct cr_state *,int);
uint32_t cr_crc(struct cr_state *,const uint8_t *,size_t);
int cr_run(struct cr_state *,const struct cr_ops *);
int cr_entry_allowed(int,char **,int);
#endif
