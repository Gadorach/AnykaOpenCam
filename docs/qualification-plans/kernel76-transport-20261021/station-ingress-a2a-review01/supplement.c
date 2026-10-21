/* Independent ordered behavior probes; includes actual frozen functions above. */
_Static_assert(sizeof(struct hgic_hdr) == 8, "actual outer ABI");
_Static_assert(sizeof(struct hgic_ctrl_hdr) == 12, "actual control ABI");
_Static_assert(offsetof(struct hgic_ctrl_hdr, cmd2.status) == 10, "CMD2 status ABI");
static unsigned int scenarios;
static int review_mode;

static void review_schedule(struct hgic_fwctrl *c)
{
    struct sk_buff *tx = skb_dequeue(&c->txq);
    CHECK(tx != NULL); sends++;
    put_unaligned_le16(get_unaligned_le16(tx->data + 6), packet + 6);
    dev_kfree_skb_any(tx);
    defer_worker = true;
    completion();
    if (review_mode) {
        if (review_mode == 1) packet[6]++; /* late unknown association */
        if (review_mode == 2) packet[8]++; /* same-cookie conflicting duplicate */
        if (review_mode == 3) packet[2] = HGIC_HDR_TYPE_CMD2;
        completion();
    }
    CHECK(!usb.bus.rx_failed && !hg.ctrl.receipt_error);
    CHECK(skb_queue_len(&c->rxq) == (review_mode ? 2 : 1));
    hgic_fwctrl_work(&c->work);
}

static void unchanged(const u8 *b, size_t n)
{ for (size_t i = 0; i < n; i++) CHECK(b[i] == 0xa5); }

static void async_receipts(void)
{
    u8 out[26];
    for (int m = 1; m <= 3; m++) {
        init(); hg.ctrl.schedule = review_schedule; review_mode = m;
        memset(out, 0xa5, sizeof(out));
        unsigned int before = submissions;
        int ret = command(out + 1);
        CHECK(submissions == before + 2); /* worker did not run at submit checks */
        if (m == 1) {
            CHECK(ret == -EPROTO && usb.bus.rx_failed && hg.ctrl.receipt_error == -EPROTO);
            unchanged(out, sizeof(out));
            /* Worker alone sets sticky bus flag, not local USB ERR. */
            CHECK(!usb.status);
            hg.ctrl.schedule = schedule; make_info(); defer_worker = false;
            unsigned int sent = sends;
            CHECK(command(out + 1) == -EPROTO && sends == sent);
            CHECK(submissions == before + 2);
        } else {
            CHECK(ret == 24 && !hg.ctrl.receipt_error && !usb.bus.rx_failed);
            CHECK(out[0] == 0xa5 && out[25] == 0xa5 && out[13] == 2);
        }
        cleanup(); scenarios++;
    }
    /* Error after copy cannot undo that copy. It prevents future admission. */
    init(); memset(out, 0xa5, sizeof(out)); CHECK(command(out + 1) == 24);
    u8 saved[26]; memcpy(saved, out, sizeof(saved));
    put_unaligned_le16(0x7fff, packet + 6); completion();
    CHECK(hg.ctrl.receipt_error == -EPROTO && usb.bus.rx_failed);
    CHECK(!memcmp(out, saved, sizeof(out)));
    unsigned int before = submissions, sent = sends;
    usb.status = 0; make_info();
    CHECK(command(out + 1) == -EPROTO && sends == sent && submissions == before);
    cleanup(); scenarios++;
}

static void strict_extremes(void)
{
    const int lens[] = {INT_MIN, -65536, -12, -1, 0, 1, 7, 8, 11, 12, 35, 37,
                       HGIC_PKT_MAX_LEN+1, 65535, 65536, INT_MAX};
    for (size_t i = 0; i < sizeof(lens)/sizeof(*lens); i++) {
        init(); actual_len = lens[i]; refuse(true); scenarios++;
    }
    const int sizes[] = {INT_MIN, -1, 0, 35, HGIC_PKT_MAX_LEN-1,
                         HGIC_PKT_MAX_LEN+1, INT_MAX};
    for (size_t i = 0; i < sizeof(sizes)/sizeof(*sizes); i++) {
        init(); transfer_len = sizes[i]; refuse(true); scenarios++;
    }
    const int statuses[] = {INT_MIN, -ESHUTDOWN, -EOVERFLOW, -EREMOTEIO, 1, INT_MAX};
    for (size_t i = 0; i < sizeof(statuses)/sizeof(*statuses); i++) {
        for (int zero = 0; zero < 2; zero++) {
            init(); completion_status = statuses[i]; actual_len = zero ? 0 : 36;
            refuse(true); scenarios++;
        }
    }
    /* Original CMD2 length equality/minimum applies independently of association. */
    for (unsigned int d = 0; d <= 0xffff; d++) {
        if (d == 36) continue;
        init(); packet[2] = HGIC_HDR_TYPE_CMD2; put_unaligned_le16(d, packet+4);
        refuse(true); scenarios++;
    }
    for (int kind = 0; kind < 2; kind++) {
        for (int n = 8; n <= 12; n++) {
            init(); packet[2] = kind ? HGIC_HDR_TYPE_CMD2 : HGIC_HDR_TYPE_CMD;
            actual_len = n; put_unaligned_le16(n, packet+4);
            if (n < 12) {
                refuse(true);
            } else {
                /* Exact minimum is structurally admitted; A1 rejects missing
                 * FWINFO bytes later, after this callback can resubmit. */
                u8 out[26]; memset(out, 0xa5, sizeof(out));
                unsigned int before = submissions;
                CHECK(command(out+1) == -EPROTO && submissions == before+1);
                unchanged(out, sizeof(out)); CHECK(usb.bus.rx_failed);
                cleanup();
            }
            scenarios++;
        }
    }
}

static void stop_ownership(void)
{
    /* STOP wins even when ERR and sticky failure are also known: no new error hook.
     * Invalid payload pointer must not be read on this normal-cancellation branch. */
    for (unsigned int flags = 0; flags < 4; flags++) {
        init();
        usb.status = HGIC_USB_STATUS_STOP | ((flags & 1) ? HGIC_USB_STATUS_ERR : 0);
        usb.bus.rx_failed = !!(flags & 2);
        buf.data = (void *)(uintptr_t)1;
        urb.context = &buf; urb.status = -ENOENT; urb.actual_length = INT_MAX;
        urb.transfer_buffer = NULL; urb.transfer_buffer_length = -1;
        INIT_LIST_HEAD(&buf.list); list_add_tail(&buf.list, &usb.used);
        unsigned int before = submissions;
        hgic_usb_receive(&urb);
        CHECK(submissions == before && !hg.ctrl.receipt_error);
        CHECK(usb.bus.rx_failed == !!(flags & 2));
        CHECK(usb.used.next == &usb.used && usb.used.prev == &usb.used);
        CHECK(usb.rx_freeq.next == &buf.list && usb.rx_freeq.prev == &buf.list);
        CHECK(!(buf.flag & HGIC_USB_BUF_FLAG_USED));
        list_del(&buf.list); cleanup(); scenarios++;
    }
    /* Poisoning cannot steal/free a completed waiter's skb or complete it twice. */
    init();
    struct hgic_cmd_response done = {.type = HGIC_HDR_TYPE_CMD, .done = true},
                             pending = {.type = HGIC_HDR_TYPE_CMD2};
    done.skb = dev_alloc_skb(12); struct sk_buff *owned = done.skb;
    list_add(&done.list, &hg.ctrl.pd_list); list_add(&pending.list, &hg.ctrl.pd_list);
    hgics_rx_error(&hg); hgics_rx_error(&hg);
    CHECK(done.skb == owned && !done.cmpl.done);
    CHECK(pending.done && pending.cmpl.done == 1 && !pending.skb);
    CHECK(live_skb == 1 && hg.ctrl.receipt_error == -EPROTO && usb.bus.rx_failed);
    list_del(&done.list); list_del(&pending.list); dev_kfree_skb_any(owned);
    cleanup(); scenarios++;
}

static void cmd2_and_wrappers(void)
{
    /* Selected do_cmd allowlist emits CMD, not CMD2. Exercise the real internal
     * send_data/worker ownership and CMD2 copy helper without inventing an opcode. */
    init(); packet[2] = HGIC_HDR_TYPE_CMD2; packet[3] = 1;
    actual_len = 16; put_unaligned_le16(16, packet+4);
    put_unaligned_le16(0x123, packet+8); put_unaligned_le16(4, packet+10);
    memcpy(packet+12, "WXYZ", 4);
    struct sk_buff *tx = dev_alloc_skb(12);
    struct hgic_ctrl_hdr *hdr = (void *)skb_put(tx, 12); memset(hdr, 0, 12);
    hdr->hdr.type = HGIC_HDR_TYPE_CMD2; hdr->hdr.ifidx = 1;
    put_unaligned_le16(11, (u8 *)hdr+6);
    put_unaligned_le16(0x123, (u8 *)hdr+8);
    struct hgic_cmd_response receipt = {.cookie = 11};
    struct sk_buff *response = hgic_fwctrl_send_data(&hg.ctrl, tx, &receipt, 500);
    CHECK(response && receipt.done && receipt.type == HGIC_HDR_TYPE_CMD2);
    u8 out[6]; memset(out, 0xa5, sizeof(out));
    unsigned long irq;
    spin_lock_irqsave(&hg.ctrl.lock, irq);
    CHECK(hgic_ctrl_reply_copy(response->data, response->len, receipt.type, receipt.ifidx,
                             receipt.opcode, receipt.cookie, out+1, 4) == 4);
    spin_unlock_irqrestore(&hg.ctrl.lock, irq);
    CHECK(out[0] == 0xa5 && out[5] == 0xa5 && !memcmp(out+1, "WXYZ", 4));
    dev_kfree_skb_any(response); cleanup(); scenarios++;
    for (int n = 0; n < 7; n++) {
        init(); struct hgic_fw_info info, saved;
        memset(&info, 0xa5, sizeof(info)); saved = info;
        if (n == 0) actual_len = 40;
        if (n == 1) completion_status = -EIO;
        if (n == 2) actual_len = 0;
        if (n == 3) packet[12]++;
        if (n == 4) packet[24] |= 1;
        if (n == 5) { hg.ctrl.schedule = review_schedule; review_mode = 1; }
        if (n == 6) put_unaligned_le16(23, packet+10);
        CHECK(hgic_fwctrl_get_fwinfo(&hg.ctrl, 1, &info) < 0);
        CHECK(!memcmp(&info, &saved, sizeof(info)));
        cleanup(); scenarios++;
    }
}

int main(void)
{
    async_receipts(); strict_extremes(); stop_ownership(); cmd2_and_wrappers();
    CHECK(!live_skb && !data_calls);
    printf("PASS independent A2a supplement: %u assertions; %u ordered fixtures; %u modeled sends; %u fake submissions\n",
           checks, scenarios, sends, submissions);
    return 0;
}
