/* Synthetic deterministic HOST primitives; never evaluates firmware formats. */
#include "ingress_shim.h"
static unsigned int formats_payload, formats_constant, event_queue, semaphore_ups;
static unsigned int event_frees, all_frees, success_completions, error_completions;
static unsigned int exports, rtos_calls, tx_effects, recovery, scans, keys, init_success;
static uintptr_t payload_begin, payload_end;
static struct sk_buff_head *watched_queue;
static struct sk_buff *nonlinear_skb, *watched_skb;
static u8 *watched_guard;
static int *watched_error;
static bool host_immediate, event_phase;
static unsigned int fake_logs;
static struct sk_buff *owned[128];
static struct sk_buff *tracked_alloc(size_t n)
{
    struct sk_buff *s = dev_alloc_skb(n);
    for (size_t i=0;i<128;i++) if (!owned[i]) { owned[i]=s; return s; }
    abort();
}
#define dev_alloc_skb tracked_alloc
/* Classify address origin BEFORE any possible string access. No va_arg, format
 * dereference, strlen, printf/vprintf forwarding, or payload-derived output. */
static int safe_printk(const char *fmt, ...)
{
    uintptr_t p = (uintptr_t)fmt;
    bool from_payload = p >= payload_begin && p <= payload_end;
    for (size_t i=0;i<128;i++) if (owned[i] && owned[i]->data && owned[i]->len>=12) {
        uintptr_t begin=(uintptr_t)owned[i]->data+12, end=(uintptr_t)owned[i]->data+owned[i]->len;
        if (p>=begin && p<=end) from_payload=true;
    }
    if (from_payload) formats_payload++;
    else formats_constant++;
    fake_logs++;
    return 0;
}
#undef printk
#undef hgic_err
#define printk safe_printk
#define hgic_err safe_printk
static void tracked_queue(struct sk_buff_head *q, struct sk_buff *s)
{ if (q == watched_queue) event_queue++; skb_queue_tail(q, s); }
#define skb_queue_tail tracked_queue
static void up(struct semaphore *s) { semaphore_ups++; s->count++; }
static void tracked_free(struct sk_buff *s)
{
    if (s) {
        bool found=false;
        for(size_t i=0;i<128;i++) if(owned[i]==s) { owned[i]=NULL; found=true; break; }
        assert(found); all_frees++;
    }
    if (s && s == watched_skb) {
        assert(!watched_guard || *watched_guard == 0xa6);
        event_frees++; watched_skb = NULL;
    }
    dev_kfree_skb_any(s);
}
#undef dev_kfree_skb
#undef kfree_skb
#define dev_kfree_skb tracked_free
#define kfree_skb tracked_free
#define dev_kfree_skb_any tracked_free
static bool skb_is_nonlinear(const struct sk_buff *s) { return s == nonlinear_skb; }
static void tracked_complete(struct completion *c)
{
    if (event_phase) {
        if (watched_error && *watched_error) error_completions++;
        else success_completions++;
    }
    complete(c);
}
#define complete tracked_complete
static void hgic_fwctrl_work(struct work_struct *w);
static int event_queue_work(struct workqueue_struct *q, struct work_struct *w)
{ (void)q; if (host_immediate) hgic_fwctrl_work(w); return 1; }
#define queue_work event_queue_work
static int copy_to_user(void *a, const void *b, size_t n)
{ (void)a; (void)b; (void)n; exports++; abort(); }
static void hgics_event(void *h, const char *n, u32 id, void *p, u32 len)
{ (void)h; (void)n; (void)id; (void)p; (void)len; rtos_calls++; abort(); }
