
#include <linux/version.h>
#include <linux/module.h>
#include <linux/completion.h>
#include <linux/usb.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(3,0,0)
#include <linux/atomic.h>
#else
#include <asm/atomic.h>
#endif
#include <linux/slab.h>
#include <linux/netdevice.h>

#include "../hgic_def.h"
#include "utils.h"

#define HGIC_PKT_MAX_LEN (2*1024+512)
#define HGIC_TX_URB_CNT  16
#define HGIC_RX_URB_CNT  32

#define HGIC_USB_STATUS_STOP    BIT(0)
#define HGIC_USB_STATUS_ERR     BIT(1)

#define HGIC_USB_BUF_FLAG_USED  BIT(0)
#define HGIC_USB_BUF_FLAG_RX    BIT(1)

#define USB_TX_HEADROM 4

struct hgic_usb_buf {
    struct list_head  list;
    struct hgic_usb  *usbdev;
    struct urb       *urb;
    void             *data;
    int               flag;
};

struct hgic_usb {
    struct usb_device *udev;
    uint   ep_in, ep_out;
    uint   ep_out_size, ep_in_size;
    uint   status;
    struct list_head  tx_freeq;
    struct list_head  rx_freeq;
    struct list_head  used;
    struct semaphore  tx_sema;
    spinlock_t qlock;
    struct hgic_bus bus;
};

static int txq_cnt = HGIC_TX_URB_CNT;
static int rxq_cnt = HGIC_RX_URB_CNT;
static hgic_probe probe_hdl = NULL;
static u32 max_pkt_len = HGIC_PKT_MAX_LEN;

static const struct usb_device_id hgic_usb_wdev_ids[] = {
    { USB_DEVICE(HGIC_VENDOR_ID, HGIC_WLAN_8400)},
    { /* end: all zeroes */             },
};
MODULE_DEVICE_TABLE(usb, hgic_usb_wdev_ids);
static void hgic_usb_receive(struct urb *urb);

static void hgic_usb_cancle(struct hgic_usb *usbdev)
{
    unsigned long flags;
    struct hgic_usb_buf *buf, *n;

    spin_lock_irqsave(&usbdev->qlock, flags);
    list_for_each_entry_safe(buf, n, &usbdev->used, list) {
        usb_unlink_urb(buf->urb);
    }
    spin_unlock_irqrestore(&usbdev->qlock, flags);
}

static void hgic_usb_quiesce(struct hgic_usb *usbdev)
{
    unsigned long flags;
    struct hgic_usb_buf *buf;

    /* Stop resubmission and synchronously drain callbacks OUTSIDE qlock:
     * usb_kill_urb may sleep and callbacks acquire this same spinlock.
     */
    usbdev->status |= HGIC_USB_STATUS_STOP;
    for (;;) {
        struct urb *urb;
        spin_lock_irqsave(&usbdev->qlock, flags);
        if (list_empty(&usbdev->used)) {
            spin_unlock_irqrestore(&usbdev->qlock, flags);
            break;
        }
        buf = list_first_entry(&usbdev->used, struct hgic_usb_buf, list);
        urb = usb_get_urb(buf->urb);
        spin_unlock_irqrestore(&usbdev->qlock, flags);
        usb_kill_urb(urb);
        usb_free_urb(urb);
    }
}
static void hgic_usb_free(struct hgic_usb *usbdev)
{
    unsigned long flags;
    struct hgic_usb_buf *buf, *n;

    hgic_usb_quiesce(usbdev);
    spin_lock_irqsave(&usbdev->qlock, flags);
    list_for_each_entry_safe(buf, n, &usbdev->tx_freeq, list) {
        list_del(&buf->list);
        usb_free_urb(buf->urb);
        if(buf->data){
            hgic_err("skb is not free??\r\n");
            kfree_skb(buf->data);
            buf->data = NULL;
        }
        kfree(buf);
    }
    list_for_each_entry_safe(buf, n, &usbdev->rx_freeq, list) {
        list_del(&buf->list);
        kfree(buf->data);
        usb_free_urb(buf->urb);
        kfree(buf);
    }
    WARN_ON(!list_empty(&usbdev->used));
    spin_unlock_irqrestore(&usbdev->qlock, flags);
}

static int hgic_usb_qinit(struct hgic_usb *usb, struct list_head *q, int qsize, u8 rx)
{
    int i = 0;
    struct hgic_usb_buf *buf = NULL;

    for (i = 0; i < qsize; i++) {
        buf = kzalloc(sizeof(struct hgic_usb_buf), GFP_ATOMIC);
        if (buf == NULL) {
            hgic_err("alloc fail, i=%d\r\n", i);
            break;
        }

        buf->usbdev = usb;
        buf->urb = usb_alloc_urb(0, GFP_KERNEL);
        if (buf->urb) {
            if (rx) {
                buf->data = kmalloc(max_pkt_len, GFP_KERNEL);
                if (buf->data == NULL) {
                    kfree(buf);
                    hgic_err("alloc fail, len=%d\r\n", max_pkt_len);
                    break;
                }
                buf->flag |= HGIC_USB_BUF_FLAG_RX;
            }
            list_add_tail(&buf->list, q);
        } else {
            kfree(buf);
            break;
        }
    }
    return i;
}

static struct hgic_usb_buf *hgic_usb_deq(struct hgic_usb *usbdev, struct list_head *q)
{
    struct hgic_usb_buf *buf = NULL;
    unsigned long flags;

    spin_lock_irqsave(&usbdev->qlock, flags);
    if (!list_empty(q)) {
        buf = list_first_entry(q, struct hgic_usb_buf, list);
        buf->flag |= HGIC_USB_BUF_FLAG_USED;
        list_del(&buf->list);
        list_add_tail(&buf->list, &usbdev->used);
    }
    spin_unlock_irqrestore(&usbdev->qlock, flags);
    return buf;
}

static void hgic_usb_enq(struct hgic_usb_buf *buf, struct list_head *q)
{
    unsigned long flags;

    spin_lock_irqsave(&buf->usbdev->qlock, flags);
    buf->flag &= ~HGIC_USB_BUF_FLAG_USED;
    list_del(&buf->list);
    list_add_tail(&buf->list, q);
    spin_unlock_irqrestore(&buf->usbdev->qlock, flags);
}

static int hgic_usb_submit_rx_urb(struct hgic_usb_buf *buf)
{
    int ret = -1;

    if ((buf->usbdev->status & (HGIC_USB_STATUS_STOP | HGIC_USB_STATUS_ERR)) ||
        READ_ONCE(buf->usbdev->bus.rx_failed)) {
        hgic_usb_enq(buf, &buf->usbdev->rx_freeq);
        return -1;
    }

    usb_fill_bulk_urb(buf->urb, buf->usbdev->udev,
                      usb_rcvbulkpipe(buf->usbdev->udev, buf->usbdev->ep_in),
                      buf->data, max_pkt_len, hgic_usb_receive, buf);
    ret = usb_submit_urb(buf->urb, GFP_ATOMIC);
    if (ret) {
        hgic_err("submit rx urb %p failed: %d\n", buf->urb, ret);
        hgic_usb_enq(buf, &buf->usbdev->rx_freeq);
        buf->usbdev->status |= HGIC_USB_STATUS_ERR;
        if (buf->usbdev->bus.rx_error)
            buf->usbdev->bus.rx_error(buf->usbdev->bus.bus_priv);
        hgic_usb_cancle(buf->usbdev);
        return -1;
    }
    return 0;
}

static int hgic_usb_submit_rx_urbs(struct hgic_usb *usbdev)
{
    int ret = 0;
    struct hgic_usb_buf *buf = NULL;

    while ((buf = hgic_usb_deq(usbdev, &usbdev->rx_freeq))) {
        ret = hgic_usb_submit_rx_urb(buf);
        if (ret) {
            break;
        }
    }
    return ret;
}

static void hgic_usb_receive(struct urb *urb)
{
    struct hgic_usb_buf *buf = (struct hgic_usb_buf *)urb->context;
    struct hgic_usb *usbdev = buf->usbdev;
    int ret;

    //hgic_enter();
    if (buf->usbdev->status & HGIC_USB_STATUS_STOP) {
        hgic_usb_enq(buf, &buf->usbdev->rx_freeq);
        hgic_err("STOP, drop data, status=%x\r\n", buf->usbdev->status);
        return;
    }

    /* STOP is normal cancellation above: never deliver or resubmit it. Other
     * completion failures poison A1 acceptance, including error+zero. A successful
     * zero-length completion has no established station wire meaning: reject.
     * The allocated backing is exactly max_pkt_len; verify the URB still names
     * that backing and its full extent before trusting actual_length or data.
     */
    if ((usbdev->status & HGIC_USB_STATUS_ERR) || READ_ONCE(usbdev->bus.rx_failed) ||
        urb->status || !buf->data || urb->transfer_buffer != buf->data ||
        urb->transfer_buffer_length != max_pkt_len ||
        urb->actual_length <= 0 || urb->actual_length > urb->transfer_buffer_length)
        goto bad_rx;
    ret = usbdev->bus.rx_packet(usbdev->bus.bus_priv, buf->data, urb->actual_length);
    if (ret == -EPROTO || READ_ONCE(usbdev->bus.rx_failed))
        goto bad_rx;
    hgic_usb_submit_rx_urb(buf);
    //hgic_leave();
    return;
bad_rx:
    usbdev->status |= HGIC_USB_STATUS_ERR;
    WRITE_ONCE(usbdev->bus.rx_failed, 1);
    if (usbdev->bus.rx_error)
        usbdev->bus.rx_error(usbdev->bus.bus_priv);
    /* Keep backing owned. Other in-flight URBs retire through their callbacks;
     * this is not a kill/drain/free or a global work-cancellation receipt.
     */
    hgic_usb_enq(buf, &usbdev->rx_freeq);
}

static void hgic_usb_tx_complete(struct urb *urb)
{
    struct hgic_usb_buf *buf = (struct hgic_usb_buf *)urb->context;
    struct hgic_usb     *usbdev = buf->usbdev;

    usbdev->bus.tx_complete(usbdev->bus.bus_priv, (struct sk_buff *)buf->data, !urb->status);
    buf->data = NULL;
    hgic_usb_enq(buf, &usbdev->tx_freeq);
    up(&usbdev->tx_sema);
}

static int hgic_usb_tx_packet(void *bus, struct sk_buff *skb)
{
    int ret = 0;
    int len = ALIGN(skb->len, 4);
    struct hgic_usb_buf *buf    = NULL;
    struct hgic_usb     *usbdev = container_of(bus, struct hgic_usb, bus);
    struct hgic_hdr *hdr = (struct hgic_hdr *)skb->data;
    bool token = false;

    if (skb_is_nonlinear(skb) && skb_linearize(skb)) {
        ret = -ENOMEM;
        goto __fail;
    }
    hdr = (struct hgic_hdr *)skb->data;

    /* Stock retail REA0x1b3c8 unconditionally appends four transport bytes
     * at the endpoint packet boundary, not just URB_ZERO_PACKET. Keep the
     * protocol hdr.length at its original value and zero only wire padding.
     */
    if (IS_ALIGNED(len, usbdev->ep_out_size))
        len += 4;

    /* Station data/control only. A boot-state check may identify recovery,
     * but all boot writes/RUN/raw firmware and OTA transport are denied.
     */
    if (skb->len < sizeof(*hdr) ||
        (hdr->type != HGIC_HDR_TYPE_FRM && hdr->type != HGIC_HDR_TYPE_FRM2 &&
         hdr->type != HGIC_HDR_TYPE_AGGFRM && hdr->type != HGIC_HDR_TYPE_CMD &&
         hdr->type != HGIC_HDR_TYPE_CMD2 && hdr->type != HGIC_HDR_TYPE_SOFTFC &&
         !(hdr->type == HGIC_HDR_TYPE_BOOTDL && skb->len >= 20 && skb->data[8] == 0))) {
        ret = -EPERM;
        goto __fail;
    }
    if (len > skb->len) {
        int padding = len - skb->len;
        if (skb_tailroom(skb) < padding &&
            pskb_expand_head(skb, 0, padding, GFP_ATOMIC)) {
            ret = -ENOMEM;
            goto __fail;
        }
        skb_put_zero(skb, padding);
    }


    if (usbdev->status & (HGIC_USB_STATUS_STOP | HGIC_USB_STATUS_ERR)) {
        ret = -EIO;
        goto __fail;
    }

    ret = down_timeout(&usbdev->tx_sema, 1000);
    if (ret) {
        ret = -EIO;
        goto __fail;
    }
    token = true;

    if (usbdev->status & (HGIC_USB_STATUS_STOP | HGIC_USB_STATUS_ERR)) {
        ret = -EIO;
        goto __fail;
    }

    buf = hgic_usb_deq(usbdev, &usbdev->tx_freeq);
    if (buf == NULL) {
        ret = -ENOMEM;
        goto __fail;
    }
    buf->data = skb;
    usb_fill_bulk_urb(buf->urb, usbdev->udev, usb_sndbulkpipe(usbdev->udev, usbdev->ep_out),
                      skb->data, len, hgic_usb_tx_complete, buf);
    buf->urb->transfer_flags |= URB_ZERO_PACKET;
    ret = usb_submit_urb(buf->urb, GFP_ATOMIC);
    if (ret) {
        hgic_err("usb_submit_urb failed, ret:%d\n", ret);
        goto __fail;
    }

    return ret;

__fail:
    if (buf) {
        buf->data = NULL;
        hgic_usb_enq(buf, &usbdev->tx_freeq);
    }
    if (token)
        up(&usbdev->tx_sema);
    usbdev->bus.tx_complete(usbdev->bus.bus_priv, skb, 0);
    return ret;
}

static int hgic_usb_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
    int i   = 0;
    int ret = 0;
    struct usb_device *udev = NULL;
    struct hgic_usb   *usbdev = NULL;
    struct usb_host_interface *iface_desc = NULL;
    struct usb_endpoint_descriptor *endpoint = NULL;

    hgic_dbg("new usb card: vendor:%x, id:%x\n", id->idVendor, id->idProduct);
    udev = interface_to_usbdev(intf);
    if (udev->bus->busnum != 1 || strcmp(udev->devpath, "1") ||
        udev->speed != USB_SPEED_HIGH || !udev->actconfig ||
        udev->actconfig->desc.bConfigurationValue != 1)
        return -ENODEV;
    iface_desc = intf->cur_altsetting;
    usbdev = kzalloc(sizeof(struct hgic_usb), GFP_KERNEL);
    if (!usbdev) {
        return -ENOMEM;
    }

    usbdev->udev = udev;
    usbdev->bus.type = HGIC_BUS_USB;
    usbdev->bus.dev_id = id->idProduct;
    usbdev->bus.drv_tx_headroom = USB_TX_HEADROM;
    usbdev->bus.tx_packet = hgic_usb_tx_packet;
    usbdev->bus.bootdl_pktlen = 2048;
    usbdev->bus.bootdl_cksum = HGIC_BUS_BOOTDL_CHECK_0XFD;
    usbdev->bus.probe = probe_hdl;
    
    //usbdev->status = HGIC_USB_STATUS_STOP;

    for (i = 0; i < iface_desc->desc.bNumEndpoints; ++i) {
        endpoint = &iface_desc->endpoint[i].desc;
        if (usb_endpoint_is_bulk_in(endpoint)) {
            usbdev->ep_in_size = le16_to_cpu(endpoint->wMaxPacketSize);
            usbdev->ep_in = usb_endpoint_num(endpoint);
            hgic_dbg("IN  BULK: ep_in_size:%x, ep_in:%x\n", usbdev->ep_in_size, usbdev->ep_in);
        } else if (usb_endpoint_is_bulk_out(endpoint)) {
            usbdev->ep_out_size = le16_to_cpu(endpoint->wMaxPacketSize);
            usbdev->ep_out = usb_endpoint_num(endpoint);
            hgic_dbg("OUT BULK: ep_out_size:%x, ep_out:%x\n", usbdev->ep_out_size, usbdev->ep_out);
        }
    }
    usbdev->bus.blk_size = usbdev->ep_out_size;

    spin_lock_init(&usbdev->qlock);
    INIT_LIST_HEAD(&usbdev->tx_freeq);
    INIT_LIST_HEAD(&usbdev->rx_freeq);
    INIT_LIST_HEAD(&usbdev->used);
    ret = hgic_usb_qinit(usbdev, &usbdev->tx_freeq, txq_cnt, 0);
    sema_init(&usbdev->tx_sema, ret);
    hgic_dbg("usb txq:%d\r\n", ret);
    ret = hgic_usb_qinit(usbdev, &usbdev->rx_freeq, rxq_cnt, 1);
    hgic_dbg("usb rxq:%d\r\n", ret);

    usbdev->bus.probe(&udev->dev, &usbdev->bus);
    if (!usbdev->bus.bus_priv) {
        ret = -ENOMEM;
        goto __failed;
    }

    usb_get_dev(udev);
    usb_set_intfdata(intf, usbdev);
    //usbdev->status &= ~HGIC_USB_STATUS_STOP;

    ret = hgic_usb_submit_rx_urbs(usbdev);
    if (ret) {
        goto __failed;
    }

    if (usbdev->bus.probe_post) {
        usbdev->bus.probe_post(usbdev->bus.bus_priv);
    }
    return ret;

__failed:
    hgic_usb_quiesce(usbdev);
    if (usbdev->bus.remove) {
        usbdev->bus.remove(usbdev->bus.bus_priv);
    }
    hgic_usb_free(usbdev);
    kfree(usbdev);
    return -1;
}

static void hgic_usb_disconnect(struct usb_interface *intf)
{
    struct hgic_usb *usbdev = usb_get_intfdata(intf);
    hgic_dbg("Enter\n");
    if (usbdev) {
        usbdev->status |= HGIC_USB_STATUS_STOP;
        up(&usbdev->tx_sema);
        /* TX callbacks still reference bus_priv. Drain them before the
         * mac80211 core tears that object down, not after free_hw().
         */
        hgic_usb_quiesce(usbdev);
        if (usbdev->bus.remove) {
            usbdev->bus.remove(usbdev->bus.bus_priv);
        }
        hgic_usb_free(usbdev);
        usb_put_dev(usbdev->udev);
        kfree(usbdev);
        usb_set_intfdata(intf, NULL);
    }
    hgic_dbg("Leave\n");
}

static struct usb_driver hgic_usb_driver = {
    .name = KBUILD_MODNAME,
    .probe = hgic_usb_probe,
    .disconnect = hgic_usb_disconnect,
    .id_table = hgic_usb_wdev_ids,
    .suspend = NULL,
    .resume = NULL,
    .supports_autosuspend = 1,
};

int __init hgic_usb_init(hgic_probe probe, u32 max_pkt)
{
    int ret = 0;

    hgic_dbg("Enter, max_pkt_len = %d\n", max_pkt_len);
    probe_hdl = probe;
    if(max_pkt > HGIC_PKT_MAX_LEN){
        max_pkt_len = max_pkt;
    }
    ret = usb_register(&hgic_usb_driver);
    if (ret) {
        hgic_err("driver register failed: %d\n", ret);
    }
    hgic_dbg("Leave\n");
    return ret;
}

void __exit hgic_usb_exit(void)
{
    hgic_dbg("Enter\n");
    usb_deregister(&hgic_usb_driver);
    hgic_dbg("Leave\n");
}

