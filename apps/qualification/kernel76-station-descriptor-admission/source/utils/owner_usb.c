/* SPDX-License-Identifier: GPL-2.0-only
 * P4a fixed-slot owner lifetime derivative of Huge-IC USB transport.
 * Imported if_usb.c remains unchanged, unselected provenance.
 */
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/slab.h>
#include <linux/completion.h>
#include <linux/semaphore.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include "../hgic_def.h"
#include "utils.h"
#include "owned_control.h"

#define HGIC_PKT_MAX_LEN 2560
#define HGIC_TX_URB_CNT 16
#define HGIC_RX_URB_CNT 32
#define USB_TX_HEADROM 4

struct hgic_usb;

struct hgic_usb_buf {
    struct list_head list;
    struct hgic_usb *usbdev;
    struct urb *urb;
    void *data;
    /* Conservative slot leases, not active-call counts. Never cleared by
     * callback tails or submit returns. Clear only after join + poison proof.
     */
    bool submit_reserved;
    bool callback_reserved;
};

struct hgic_usb {
    struct usb_device *udev;
    unsigned int ep_in, ep_out, ep_in_size, ep_out_size;
    struct hgic_usb_buf tx[HGIC_TX_URB_CNT];
    struct hgic_usb_buf rx[HGIC_RX_URB_CNT];
    struct list_head tx_freeq, rx_freeq, used;
    struct semaphore tx_sema;
    struct hgic_bus bus;
    unsigned int allocated_urbs;
    unsigned int tx_allocated, rx_allocated;
    unsigned int submit_slots_reserved, callback_slots_reserved;
    unsigned int tx_caller_reserved;
    /* Never completed: uncertain retirement must not return/module-unload. */
    struct completion quarantine;
};

static hgic_probe probe_hdl;
static void hgic_usb_receive(struct urb *urb);
static void hgic_usb_tx_complete(struct urb *urb);

static const struct usb_device_id hgic_usb_wdev_ids[] = {
    { USB_DEVICE(HGIC_VENDOR_ID, HGIC_WLAN_8400) },
    { }
};
MODULE_DEVICE_TABLE(usb, hgic_usb_wdev_ids);

static void hgic_usb_close(struct hgic_usb *usb)
{
    unsigned long flags;
    spin_lock_irqsave(&usb->bus.owner_lock, flags);
    usb->bus.stopping = true;
    spin_unlock_irqrestore(&usb->bus.owner_lock, flags);
}

static void hgic_usb_fail(struct hgic_usb *usb, int error, unsigned int phase)
{
    hgic_attach_error(error, phase); /* preserve exact first cause before STOP */
    hgic_usb_close(usb);
}

static __noreturn void hgic_usb_quarantine(struct hgic_usb *usb)
{
    hgic_usb_fail(usb, -EUCLEAN, HGIC_ATTACH_PROBE);
    /* No allocation free, callback skip, timeout, magic success or return.
     * A blocked disconnect/module exit retains module text as well as owners.
     * Later P5 must supply actual bounded recovery; this is not a watchdog.
     */
    for (;;)
        wait_for_completion(&usb->quarantine);
}

/* owner_lock held. One sticky reservation per immutable slot covers callback
 * reentry/resubmit and a callback preceding the outer submit return. It does
 * not pretend to count accepted URBs or currently executing callbacks.
 */
static void hgic_usb_reserve_slot(struct hgic_usb_buf *buf)
{
    struct hgic_usb *usb = buf->usbdev;
    lockdep_assert_held(&usb->bus.owner_lock);
    if (!buf->submit_reserved) {
        buf->submit_reserved = true;
        usb->submit_slots_reserved++;
    }
    if (!buf->callback_reserved) {
        buf->callback_reserved = true;
        usb->callback_slots_reserved++;
    }
}

static void hgic_usb_requeue(struct hgic_usb_buf *buf, struct list_head *head)
{
    unsigned long flags;
    struct hgic_usb *usb = buf->usbdev;
    spin_lock_irqsave(&usb->bus.owner_lock, flags);
    list_move_tail(&buf->list, head);
    spin_unlock_irqrestore(&usb->bus.owner_lock, flags);
}

static void hgic_usb_init_slots(struct hgic_usb *usb)
{
    unsigned int i;
    INIT_LIST_HEAD(&usb->tx_freeq);
    INIT_LIST_HEAD(&usb->rx_freeq);
    INIT_LIST_HEAD(&usb->used);
    for (i = 0; i < HGIC_TX_URB_CNT; i++) {
        usb->tx[i].usbdev = usb;
        INIT_LIST_HEAD(&usb->tx[i].list);
    }
    for (i = 0; i < HGIC_RX_URB_CNT; i++) {
        usb->rx[i].usbdev = usb;
        INIT_LIST_HEAD(&usb->rx[i].list);
    }
}

static int hgic_usb_alloc_slots(struct hgic_usb *usb)
{
    unsigned int i;
    for (i = 0; i < HGIC_TX_URB_CNT; i++) {
        struct hgic_usb_buf *buf = &usb->tx[i];
        buf->urb = usb_alloc_urb(0, GFP_KERNEL);
        if (!buf->urb)
            return -ENOMEM;
        usb->allocated_urbs++;
        list_add_tail(&buf->list, &usb->tx_freeq);
        usb->tx_allocated++;
    }
    for (i = 0; i < HGIC_RX_URB_CNT; i++) {
        struct hgic_usb_buf *buf = &usb->rx[i];
        buf->urb = usb_alloc_urb(0, GFP_KERNEL);
        if (!buf->urb)
            return -ENOMEM;
        usb->allocated_urbs++;
        /* Keep the current URB in its fixed slot even if backing fails. */
        buf->data = kmalloc(HGIC_PKT_MAX_LEN, GFP_KERNEL);
        if (!buf->data)
            return -ENOMEM;
        list_add_tail(&buf->list, &usb->rx_freeq);
        usb->rx_allocated++;
    }
    return 0;
}

static int hgic_usb_submit_rx(struct hgic_usb_buf *buf)
{
    struct hgic_usb *usb = buf->usbdev;
    unsigned long flags;
    int error;
    spin_lock_irqsave(&usb->bus.owner_lock, flags);
    if (usb->bus.stopping || READ_ONCE(usb->bus.rx_failed) || hgic_attach_failed()) {
        list_move_tail(&buf->list, &usb->rx_freeq);
        spin_unlock_irqrestore(&usb->bus.owner_lock, flags);
        return -ESHUTDOWN;
    }
    list_move_tail(&buf->list, &usb->used);
    hgic_usb_reserve_slot(buf);
    spin_unlock_irqrestore(&usb->bus.owner_lock, flags);

    /* A pre-STOP admitted callback may still reach this entry. Retirement
     * retains this slot and permanently poisons it, waiting for actual return.
     */
    usb_fill_bulk_urb(buf->urb, usb->udev, usb_rcvbulkpipe(usb->udev, usb->ep_in),
                      buf->data, HGIC_PKT_MAX_LEN, hgic_usb_receive, buf);
    error = usb_submit_urb(buf->urb, GFP_ATOMIC);
    if (error) {
        /* HCD did not accept: no callback for this submission. This local
         * owner alone requeues it; never fabricate HCD completion/retirement.
         */
        hgic_usb_fail(usb, error, HGIC_ATTACH_RX_SUBMIT);
        hgic_usb_requeue(buf, &usb->rx_freeq);
        if (usb->bus.rx_error)
            usb->bus.rx_error(usb->bus.bus_priv);
        return error;
    }
    /* Immediate callback may have consumed/resubmitted it: no buf/URB access. */
    return 0;
}

static void hgic_usb_receive(struct urb *urb)
{
    struct hgic_usb_buf *buf = urb->context;
    struct hgic_usb *usb = buf->usbdev;
    unsigned long flags;
    int error;

    /* Serialize RX dispatch admission, but never hold it over parser/callback. */
    spin_lock_irqsave(&usb->bus.owner_lock, flags);
    if (usb->bus.stopping) {
        list_move_tail(&buf->list, &usb->rx_freeq);
        spin_unlock_irqrestore(&usb->bus.owner_lock, flags);
        return;
    }
    spin_unlock_irqrestore(&usb->bus.owner_lock, flags);
    if (READ_ONCE(usb->bus.rx_failed) || hgic_attach_failed() || urb->status ||
        !buf->data || urb->transfer_buffer != buf->data ||
        urb->transfer_buffer_length != HGIC_PKT_MAX_LEN ||
        urb->actual_length <= 0 || urb->actual_length > urb->transfer_buffer_length)
        goto bad_rx;
    error = usb->bus.rx_packet(usb->bus.bus_priv, buf->data, urb->actual_length);
    if (error == -EPROTO || READ_ONCE(usb->bus.rx_failed) || hgic_attach_failed())
        goto bad_rx;
    hgic_usb_submit_rx(buf);
    return;
bad_rx:
    hgic_usb_fail(usb, urb->status ? urb->status : -EPROTO, HGIC_ATTACH_RX_COMPLETE);
    WRITE_ONCE(usb->bus.rx_failed, 1);
    if (usb->bus.rx_error)
        usb->bus.rx_error(usb->bus.bus_priv);
    hgic_usb_requeue(buf, &usb->rx_freeq);
}

static void hgic_usb_tx_complete(struct urb *urb)
{
    struct hgic_usb_buf *buf = urb->context;
    struct hgic_usb *usb = buf->usbdev;
    struct hgic_control_request *request = hgic_control_from_skb(buf->data);
    int status = urb->status;
    if (status)
        hgic_usb_fail(usb, status, HGIC_ATTACH_TX_COMPLETE);
    usb->bus.tx_complete(usb->bus.bus_priv, buf->data, !status);
    buf->data = NULL;
    hgic_usb_requeue(buf, &usb->tx_freeq);
    up(&usb->tx_sema);
    if (request)
        hgic_control_callback(request, status); /* final request access in callback */
    /* Leases remain reserved through the return tail. Only the later kernel
     * poison return proves this callback and its module text have returned.
     */
}

static int hgic_usb_tx_packet(void *bus, struct sk_buff *skb)
{
    struct hgic_usb *usb = container_of(bus, struct hgic_usb, bus);
    struct hgic_usb_buf *buf = NULL;
    struct hgic_hdr *hdr;
    struct hgic_control_request *request = hgic_control_from_skb(skb);
    unsigned long flags;
    bool token = false;
    int length, error;

    spin_lock_irqsave(&usb->bus.owner_lock, flags);
    if (usb->bus.stopping) {
        spin_unlock_irqrestore(&usb->bus.owner_lock, flags);
        error = -ESHUTDOWN;
        goto local_failure;
    }
    /* Actual selected caller is the single ordered core TX worker. Its full
     * return is proved by owner_stop/cancel_work_sync, not this sticky scalar.
     */
    usb->tx_caller_reserved = 1;
    spin_unlock_irqrestore(&usb->bus.owner_lock, flags);
    if (!request || !usb->bus.attach_tx_admit || !usb->bus.bus_priv) {
        error = -EPERM;
        goto local_failure;
    }
    error = usb->bus.attach_tx_admit(usb->bus.bus_priv, skb);
    if (error)
        goto local_failure;
    if (!usb->ep_out_size || skb_is_nonlinear(skb) || skb->len != sizeof(struct hgic_ctrl_hdr)) {
        error = -EPROTO;
        goto local_failure;
    }
    hdr = (struct hgic_hdr *)skb->data;
    if (hdr->type != HGIC_HDR_TYPE_CMD) {
        error = -EPERM;
        goto local_failure;
    }
    length = ALIGN(skb->len, 4);
    if (IS_ALIGNED(length, usb->ep_out_size))
        length += 4;
    if (length > skb->len) {
        int padding = length - skb->len;
        if (skb_tailroom(skb) < padding && pskb_expand_head(skb, 0, padding, GFP_ATOMIC)) {
            error = -ENOMEM;
            goto local_failure;
        }
        skb_put_zero(skb, padding);
    }
    error = hgic_control_prepare(request, skb); /* remaining budget before token */
    if (error)
        goto local_failure;
    error = down_trylock(&usb->tx_sema) ? -EAGAIN : 0;
    if (error)
        goto local_failure;
    token = true;
    spin_lock_irqsave(&usb->bus.owner_lock, flags);
    if (usb->bus.stopping || list_empty(&usb->tx_freeq)) {
        spin_unlock_irqrestore(&usb->bus.owner_lock, flags);
        error = -ESHUTDOWN;
        goto local_failure;
    }
    buf = list_first_entry(&usb->tx_freeq, struct hgic_usb_buf, list);
    list_move_tail(&buf->list, &usb->used);
    buf->data = skb;
    error = hgic_control_submit_claim(request, skb);
    if (error) {
        spin_unlock_irqrestore(&usb->bus.owner_lock, flags);
        goto local_failure;
    }
    hgic_usb_reserve_slot(buf); /* before USB can call back */
    spin_unlock_irqrestore(&usb->bus.owner_lock, flags);
    usb_fill_bulk_urb(buf->urb, usb->udev, usb_sndbulkpipe(usb->udev, usb->ep_out),
                      skb->data, length, hgic_usb_tx_complete, buf);
    buf->urb->transfer_flags |= URB_ZERO_PACKET;
    error = usb_submit_urb(buf->urb, GFP_ATOMIC);
    /* Only separately referenced request memory, never skb/reused slot, is
     * touched on successful return. Error guarantees no actual callback.
     */
    hgic_control_submit_result(request, error);
    if (error)
        goto local_failure; /* no callback for a failed submit */
    /* Callback may already have freed skb/returned token/requeued buf. */
    return 0;

local_failure:
    hgic_usb_fail(usb, error, HGIC_ATTACH_TX_SUBMIT);
    if (request)
        hgic_control_local_failure(request, skb, error);
    if (buf) {
        buf->data = NULL;
        hgic_usb_requeue(buf, &usb->tx_freeq);
    }
    if (token)
        up(&usb->tx_sema);
    if (request)
        hgic_control_local_token_returned(request);
    /* Local skb failure notification, explicitly NOT HCD callback proof. */
    if (usb->bus.tx_complete && usb->bus.bus_priv)
        usb->bus.tx_complete(usb->bus.bus_priv, skb, 0);
    else
        dev_kfree_skb_any(skb);
    return error;
}

static void hgic_usb_dispose(struct hgic_usb *usb)
{
    unsigned int i, poisoned = 0, submit_slots = 0, callback_slots = 0;
    hgic_usb_close(usb);
    if (usb->bus.bus_priv) {
        if (!usb->bus.owner_stop || !usb->bus.remove)
            hgic_usb_quarantine(usb);
        /* Join producers first, keeping every callback/private owner live. */
        usb->bus.owner_stop(usb->bus.bus_priv);
    }
    /* Stable enumeration, never current used/free membership. No lock held. */
    for (i = 0; i < HGIC_TX_URB_CNT; i++) {
        if (usb->tx[i].urb) {
            usb_poison_urb(usb->tx[i].urb);
            poisoned++;
        }
    }
    for (i = 0; i < HGIC_RX_URB_CNT; i++) {
        if (usb->rx[i].urb) {
            usb_poison_urb(usb->rx[i].urb);
            poisoned++;
        }
    }
    /* All core workers and every actual callback/return tail are now joined.
     * No unpoison path exists, including for never-submitted partial slots.
     */
    for (i = 0; i < HGIC_TX_URB_CNT; i++) {
        submit_slots += usb->tx[i].submit_reserved;
        callback_slots += usb->tx[i].callback_reserved;
        if (usb->tx[i].data)
            hgic_usb_quarantine(usb); /* uncertain skb owner: never guess free */
    }
    for (i = 0; i < HGIC_RX_URB_CNT; i++) {
        submit_slots += usb->rx[i].submit_reserved;
        callback_slots += usb->rx[i].callback_reserved;
    }
    if (poisoned != usb->allocated_urbs || submit_slots != usb->submit_slots_reserved ||
        callback_slots != usb->callback_slots_reserved ||
        submit_slots > HGIC_TX_URB_CNT + HGIC_RX_URB_CNT ||
        callback_slots > HGIC_TX_URB_CNT + HGIC_RX_URB_CNT)
        hgic_usb_quarantine(usb);
    /* Counters become zero only AFTER external join/poison return proof. */
    for (i = 0; i < HGIC_TX_URB_CNT; i++) {
        usb->tx[i].submit_reserved = false;
        usb->tx[i].callback_reserved = false;
    }
    for (i = 0; i < HGIC_RX_URB_CNT; i++) {
        usb->rx[i].submit_reserved = false;
        usb->rx[i].callback_reserved = false;
    }
    usb->submit_slots_reserved = 0;
    usb->callback_slots_reserved = 0;
    usb->tx_caller_reserved = 0;
    if (usb->bus.bus_priv) {
        usb->bus.remove(usb->bus.bus_priv);
        usb->bus.bus_priv = NULL;
    }
    for (i = 0; i < HGIC_TX_URB_CNT; i++)
        usb_free_urb(usb->tx[i].urb);
    for (i = 0; i < HGIC_RX_URB_CNT; i++) {
        kfree(usb->rx[i].data);
        usb_free_urb(usb->rx[i].urb);
    }
}

/* Historical cached scalars, not fresh target admission. Pair exclusivity is
 * a restrictive SOURCE policy; exact config/interface/alt counts and the active
 * alternate number remain unqualified. Never query or switch USB state here.
 */
#define HGIC_EP_OUT_ADDRESS 0x01
#define HGIC_EP_IN_ADDRESS  0x81
#define HGIC_EP_MAX_PACKET  512

static int hgic_usb_cached_admit(struct usb_device *udev, struct usb_interface *intf)
{
    const struct usb_host_config *config = udev->actconfig;
    const struct usb_host_interface *desc = intf->cur_altsetting;
    const struct usb_interface_cache *cache = NULL;
    unsigned int i, config_matches = 0, interface_matches = 0, alt_matches = 0;
    bool have_in = false, have_out = false;

    if (!udev->bus || udev->bus->busnum != 1 || strcmp(udev->devpath, "1") ||
        le16_to_cpu(udev->descriptor.idVendor) != HGIC_VENDOR_ID ||
        le16_to_cpu(udev->descriptor.idProduct) != HGIC_WLAN_8400 ||
        udev->speed != USB_SPEED_HIGH || !config || !udev->config ||
        !udev->descriptor.bNumConfigurations)
        return -ENODEV;

    /* Core-owned arrays/counts, not raw descriptors or guessed array order. */
    for (i = 0; i < udev->descriptor.bNumConfigurations; i++)
        config_matches += config == &udev->config[i];
    if (config_matches != 1 || config->desc.bDescriptorType != USB_DT_CONFIG ||
        config->desc.bLength < USB_DT_CONFIG_SIZE ||
        le16_to_cpu(config->desc.wTotalLength) < config->desc.bLength ||
        config->desc.bConfigurationValue != 1 || !config->desc.bNumInterfaces ||
        config->desc.bNumInterfaces > ARRAY_SIZE(config->interface))
        return -ENODEV;
    for (i = 0; i < config->desc.bNumInterfaces; i++) {
        if (config->interface[i] == intf) {
            interface_matches++;
            cache = config->intf_cache[i];
        }
    }
    if (interface_matches != 1 || !cache || !intf->altsetting || !desc ||
        !intf->num_altsetting || intf->num_altsetting != cache->num_altsetting ||
        intf->altsetting != cache->altsetting)
        return -ENODEV;
    /* USB core bounds this cached allocation; no alternate-number assumption. */
    for (i = 0; i < intf->num_altsetting; i++)
        alt_matches += desc == &intf->altsetting[i];
    if (alt_matches != 1 || desc->desc.bDescriptorType != USB_DT_INTERFACE ||
        desc->desc.bLength < USB_DT_INTERFACE_SIZE ||
        desc->desc.bInterfaceNumber != 0 || desc->desc.bNumEndpoints != 2 ||
        !desc->endpoint)
        return -ENODEV;

    for (i = 0; i < desc->desc.bNumEndpoints; i++) {
        const struct usb_endpoint_descriptor *ep = &desc->endpoint[i].desc;
        if (ep->bDescriptorType != USB_DT_ENDPOINT || ep->bLength < USB_DT_ENDPOINT_SIZE ||
            ep->bmAttributes != USB_ENDPOINT_XFER_BULK ||
            le16_to_cpu(ep->wMaxPacketSize) != HGIC_EP_MAX_PACKET)
            return -ENODEV;
        if (ep->bEndpointAddress == HGIC_EP_OUT_ADDRESS && !have_out)
            have_out = true;
        else if (ep->bEndpointAddress == HGIC_EP_IN_ADDRESS && !have_in)
            have_in = true;
        else
            return -ENODEV; /* wrong address, duplicate or extra endpoint */
    }
    return have_in && have_out ? 0 : -ENODEV;
}

static int hgic_usb_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
    struct usb_device *udev = interface_to_usbdev(intf);
    struct hgic_usb *usb;
    unsigned int i;
    int error;
    bool device_ref = false, published = false;
    error = hgic_usb_cached_admit(udev, intf);
    if (error) {
        hgic_attach_error(error, HGIC_ATTACH_PROBE);
        return error;
    }
    usb = kzalloc(sizeof(*usb), GFP_KERNEL);
    if (!usb) {
        hgic_attach_error(-ENOMEM, HGIC_ATTACH_PROBE);
        return -ENOMEM;
    }
    usb->udev = udev;
    spin_lock_init(&usb->bus.owner_lock);
    init_completion(&usb->quarantine);
    hgic_usb_init_slots(usb);
    sema_init(&usb->tx_sema, HGIC_TX_URB_CNT);
    usb->bus.type = HGIC_BUS_USB;
    usb->bus.dev_id = id->idProduct;
    usb->bus.drv_tx_headroom = USB_TX_HEADROM;
    usb->bus.tx_packet = hgic_usb_tx_packet;
    usb->bus.bootdl_pktlen = 2048;
    usb->bus.bootdl_cksum = HGIC_BUS_BOOTDL_CHECK_0XFD;
    usb->bus.probe = probe_hdl;
    /* Only the exact preallocation-admitted pair; no last-match selection. */
    usb->ep_in = HGIC_EP_IN_ADDRESS & USB_ENDPOINT_NUMBER_MASK;
    usb->ep_out = HGIC_EP_OUT_ADDRESS & USB_ENDPOINT_NUMBER_MASK;
    usb->ep_in_size = usb->ep_out_size = HGIC_EP_MAX_PACKET;
    usb->bus.blk_size = usb->ep_out_size;
    error = hgic_usb_alloc_slots(usb);
    if (error)
        goto fail;
    if (usb->tx_allocated != HGIC_TX_URB_CNT || usb->rx_allocated != HGIC_RX_URB_CNT) {
        error = -ENOMEM;
        goto fail;
    }
    error = usb->bus.probe(&udev->dev, &usb->bus);
    if (error || !usb->bus.bus_priv) {
        if (!error)
            error = -EPROTO;
        goto fail;
    }
    usb_get_dev(udev);
    device_ref = true;
    usb_set_intfdata(intf, usb);
    published = true;
    /* Exactly32 initial attempts maximum, independent of callback requeues. */
    for (i = 0; i < HGIC_RX_URB_CNT; i++) {
        error = hgic_usb_submit_rx(&usb->rx[i]);
        if (error)
            goto fail;
    }
    if (usb->bus.probe_post)
        usb->bus.probe_post(usb->bus.bus_priv);
    return 0;
fail:
    hgic_attach_error(error, HGIC_ATTACH_PROBE);
    if (published)
        usb_set_intfdata(intf, NULL);
    hgic_usb_dispose(usb);
    if (device_ref)
        usb_put_dev(udev);
    kfree(usb);
    return error;
}

static void hgic_usb_disconnect(struct usb_interface *intf)
{
    struct hgic_usb *usb = usb_get_intfdata(intf);
    if (!usb)
        return;
    usb_set_intfdata(intf, NULL); /* unpublish before either owner is invalidated */
    hgic_usb_dispose(usb); /* may block; never returns with uncertain callbacks */
    usb_put_dev(usb->udev);
    kfree(usb);
}

static struct usb_driver hgic_usb_driver = {
    .name = KBUILD_MODNAME,
    .probe = hgic_usb_probe,
    .disconnect = hgic_usb_disconnect,
    .id_table = hgic_usb_wdev_ids,
    .supports_autosuspend = 1,
};

int __init hgic_usb_init(hgic_probe probe, u32 max_pkt)
{
    int error;
    if (!probe || max_pkt) {
        hgic_attach_error(-EINVAL, HGIC_ATTACH_USB_REGISTER);
        return -EINVAL; /* fixed2560 backing; no aggregation/capacity override */
    }
    probe_hdl = probe;
    error = usb_register(&hgic_usb_driver);
    if (error)
        hgic_attach_error(error, HGIC_ATTACH_USB_REGISTER);
    return error;
}

void __exit hgic_usb_exit(void)
{
    usb_deregister(&hgic_usb_driver); /* synchronous disconnects must finish */
}
