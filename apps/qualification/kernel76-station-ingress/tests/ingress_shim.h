/* Synthetic HOST only. Real control/ingress/USB functions follow this shim. */
#define A2A_BUS_DECL 1
#include "shim.h"
typedef unsigned int uint;
#define BIT(n) (1U << (n))
#define GFP_ATOMIC 0
struct semaphore { int count; };
struct usb_device { int unused; };
struct urb {
    void *context, *transfer_buffer;
    int transfer_buffer_length, actual_length, status;
    void (*complete)(struct urb *);
};
static unsigned int submissions, cancel_calls, data_calls;
static int submit_error;
static unsigned long jiffies = 1;
static int test_bit(int b, const unsigned long *v) { return !!(*v & (1UL << b)); }
static void atomic_set(int *p, int value) { *p = value; }
static void usb_fill_bulk_urb(struct urb *u, struct usb_device *d, unsigned int pipe,
                              void *data, int n, void (*cb)(struct urb *), void *ctx)
{ (void)d; (void)pipe; u->transfer_buffer = data; u->transfer_buffer_length = n; u->complete = cb; u->context = ctx; }
static unsigned int usb_rcvbulkpipe(struct usb_device *d, unsigned int e)
{ (void)d; return e; }
static int usb_submit_urb(struct urb *u, int flags)
{ (void)u; (void)flags; submissions++; return submit_error; }
static void list_add_tail(struct list_head *n, struct list_head *h)
{ n->prev = h->prev; n->next = h; h->prev->next = n; h->prev = n; }
