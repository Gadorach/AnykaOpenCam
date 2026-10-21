/* Host-only deterministic kernel primitive model. Not a concurrency model. */
#ifndef A1_HOST_SHIM_H
#define A1_HOST_SHIM_H
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int16_t s16;
typedef int8_t s8;
typedef int spinlock_t;
#define __aligned(n) __attribute__((aligned(n)))
#define ETH_ALEN 6
#define static_assert _Static_assert
struct device;
struct hgic_sta_info;
#ifndef A2A_BUS_DECL
struct hgic_bus { unsigned int drv_tx_headroom; int rx_failed; };
#endif
struct completion { unsigned int done; };
struct work_struct { unsigned int unused; };
struct workqueue_struct { unsigned int unused; };
struct list_head { struct list_head *next, *prev; };
#define container_of(ptr, type, member) ((type *)((char *)(ptr) - offsetof(type, member)))
#define INIT_LIST_HEAD(h) ((h)->next = (h), (h)->prev = (h))
static inline void list_add(struct list_head *n, struct list_head *h)
{ n->next = h->next; n->prev = h; h->next->prev = n; h->next = n; }
static inline void list_del(struct list_head *n)
{ n->prev->next = n->next; n->next->prev = n->prev; }
#define list_for_each_entry(pos, head, member) \
    for (pos = container_of((head)->next, __typeof__(*pos), member); \
         &pos->member != (head); pos = container_of(pos->member.next, __typeof__(*pos), member))
#define spin_lock_irqsave(lock, flags) do { (flags) = 0; assert(!*(lock)); *(lock) = 1; } while (0)
#define spin_unlock_irqrestore(lock, flags) do { (void)(flags); assert(*(lock)); *(lock) = 0; } while (0)
#define READ_ONCE(x) (x)
#define WRITE_ONCE(x, v) ((x) = (v))
static inline u16 host_get_u16(const void *p) { u16 v; memcpy(&v, p, 2); return v; }
#define get_unaligned(p) host_get_u16(p)
static inline u16 get_unaligned_le16(const void *p)
{ const u8 *b = p; return (u16)b[0] | ((u16)b[1] << 8); }
static inline u32 get_unaligned_le32(const void *p)
{ const u8 *b = p; return (u32)b[0] | ((u32)b[1] << 8) | ((u32)b[2] << 16) | ((u32)b[3] << 24); }
static inline void put_unaligned_le16(u16 v, void *p)
{ u8 *b = p; b[0] = v; b[1] = v >> 8; }
static inline void put_unaligned_le32(u32 v, void *p)
{ u8 *b = p; b[0] = v; b[1] = v >> 8; b[2] = v >> 16; b[3] = v >> 24; }
#define cpu_to_le16(x) ((u16)(x))
#define le16_to_cpu(x) ((u16)(x))
static inline bool is_valid_ether_addr(const u8 *p)
{ u8 any = 0; for (int i = 0; i < 6; i++) any |= p[i]; return any && !(p[0] & 1); }
static inline void init_completion(struct completion *c) { c->done = 0; }
static inline void complete(struct completion *c) { c->done++; }
static inline unsigned long wait_for_completion_timeout(struct completion *c, unsigned long t)
{ (void)t; return c->done != 0; }
#define msecs_to_jiffies(x) (x)
static inline int host_no_log(const char *fmt, ...) { (void)fmt; return 0; }
#define printk host_no_log
#define hgic_err host_no_log

struct sk_buff {
    u8 *base, *data;
    size_t capacity;
    unsigned int len;
    struct sk_buff *next;
};
struct sk_buff_head { struct sk_buff *head, *tail; unsigned int qlen; };
static unsigned int live_skb;
static struct sk_buff *dev_alloc_skb(size_t n)
{
    struct sk_buff *s = calloc(1, sizeof(*s));
    assert(s);
    s->base = malloc(n ? n : 1);
    assert(s->base);
    s->data = s->base; s->capacity = n; live_skb++;
    return s;
}
static void dev_kfree_skb_any(struct sk_buff *s)
{ if (s) { assert(live_skb); free(s->base); free(s); live_skb--; } }
#define dev_kfree_skb dev_kfree_skb_any
#define kfree_skb dev_kfree_skb_any
static void skb_reserve(struct sk_buff *s, size_t n)
{ assert((size_t)(s->data - s->base) + n + s->len <= s->capacity); s->data += n; }
static u8 *skb_put(struct sk_buff *s, size_t n)
{ u8 *p = s->data + s->len; assert((size_t)(p - s->base) + n <= s->capacity); s->len += n; return p; }
static u8 *skb_push(struct sk_buff *s, size_t n)
{ assert((size_t)(s->data - s->base) >= n); s->data -= n; s->len += n; return s->data; }
static void skb_queue_tail(struct sk_buff_head *q, struct sk_buff *s)
{ s->next = NULL; if (q->tail) q->tail->next = s; else q->head = s; q->tail = s; q->qlen++; }
static struct sk_buff *skb_dequeue(struct sk_buff_head *q)
{ struct sk_buff *s = q->head; if (s) { q->head = s->next; if (!q->head) q->tail = NULL; q->qlen--; s->next = NULL; } return s; }
#define skb_queue_empty(q) (!(q)->qlen)
#define skb_queue_len(q) ((q)->qlen)
static inline int queue_work(struct workqueue_struct *q, struct work_struct *w)
{ (void)q; (void)w; return 1; }
#endif
