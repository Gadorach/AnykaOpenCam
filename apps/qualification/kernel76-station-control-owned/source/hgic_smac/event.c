
#ifdef __RTOS__
#include <linux/types.h>
#include <linux/unaligned.h>
#include <linux/bitops.h>
#include <linux/jiffies.h>
#include <linux/string.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/hrtimer.h>
#else
#include <linux/version.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/net.h>
#include <linux/slab.h>
#include <linux/skbuff.h>
#include <linux/etherdevice.h>
#include <linux/wireless.h>
#endif

#include "hgics.h"
#include "event.h"

#include "../utils/control_receipt.h"

static_assert(sizeof(struct hgic_hdr) == 8, "selected outer ABI");
static_assert(sizeof(struct hgic_ctrl_hdr) == 12, "selected event ABI");
static_assert(offsetof(struct hgic_ctrl_hdr, event.event_id) == 8, "selected EVENT ID");
static_assert(offsetof(struct hgic_ctrl_hdr, event2.event_id) == 8, "selected EVENT2 ID");
static_assert(offsetof(struct hgic_ctrl_hdr, event.value) == 10, "selected EVENT value");
static_assert(offsetof(struct hgic_ctrl_hdr, event2.value) == 10, "selected EVENT2 value");

void hgics_rx_fw_event(struct hgic_fwctrl *ctrl, struct sk_buff *skb)
{
    u16 evt_id;

    /* Live ctrl/bus is the callback precondition. Already denied work is only
     * disposed: no repair, response association or event delivery is permitted.
     */
    if (READ_ONCE(ctrl->receipt_error) || READ_ONCE(ctrl->bus->rx_failed))
        goto dispose;
    if (!skb || skb_is_nonlinear(skb) ||
        !hgic_ctrl_frame_valid(skb->data, skb->len))
        goto deny;
    if (skb->data[2] != HGIC_HDR_TYPE_EVENT &&
        skb->data[2] != HGIC_HDR_TYPE_EVENT2)
        goto deny;

    /* Outer magic/length are normalized by rx.c; EVENT2 ID remains wire LE.
     * EVENT byte9 is padding, not an extended ID. No payload pointer is formed.
     */
    evt_id = skb->data[2] == HGIC_HDR_TYPE_EVENT2 ?
             get_unaligned_le16(skb->data + offsetof(struct hgic_ctrl_hdr, event2.event_id)) :
             skb->data[offsetof(struct hgic_ctrl_hdr, event.event_id)];
    if (evt_id != HGIC_EVENT_FWDBG_INFO)
        goto deny;
    goto dispose;

deny:
    /* Async denial may complete CMD waiters as ERROR, never as event success.
     * It cannot undo a prior USB resubmit or queued TX. No structured event ABI
     * is selected: even FW_INIT_DONE is unsupported in this qualification lane.
     */
    hgic_fwctrl_rx_error(ctrl);
dispose:
    if (skb)
        dev_kfree_skb(skb);
}

