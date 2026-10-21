#ifndef CR_NATIVE_ADAPTER_H
#define CR_NATIVE_ADAPTER_H
#include "capture.h"
struct cr_owner {
    uint32_t bytes,handle,user_base,first_pfn,last_pfn,pages_seen;
    int dma;
    uint8_t *mapping;
};
struct cr_adapter {
    struct cr_state *core;
    int isp,video,ion,ir,pm,mem,tty,entered,retained;
    struct cr_owner output,reference;
    volatile const uint32_t *control;
    struct cr_reference installed;
    uint64_t start_ms,last_ms,emit_end_ms;
    uint32_t emit_bytes;
    int reference_installed,main_set,sub_set,geometry_checked,pre_sensor_checked;
#ifdef CR_ADAPTER_MOCK
    /* Synthetic assertions only; these fields do not exist in native builds. */
    int fixture_instance_sealed,fixture_clock_admitted,fixture_order_sealed;
    uint32_t fixture_clock_error_us;
#endif
};
_Static_assert(sizeof(struct cr_adapter)<1024, "fixed adapter state budget");
void cr_adapter_bind(struct cr_adapter *,struct cr_state *,struct cr_ops *);
int cr_adapter_call(void *,enum cr_op,void *);
int cr_adapter_clock(void *,uint64_t *);
int cr_adapter_cancelled(void *);
int cr_adapter_emit(void *,const char *,size_t);
void cr_adapter_stop(void *);
#endif
