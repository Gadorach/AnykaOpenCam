/* Actual changed functions are extracted above; no duplicated parser here. */
static unsigned int checks, sends, rejected;
#define CHECK(x) do { checks++; if (!(x)) { fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); exit(2); } } while (0)
static struct hgics_wdev hg;
static struct hgic_usb usb;
static struct hgic_usb_buf buf;
static struct urb urb;
static struct workqueue_struct wq;
static u8 packet[HGIC_PKT_MAX_LEN + 8];
static int actual_len, completion_status, transfer_len, displacement, mode;
static int null_data, wrong_buffer;
static int cookie_offset;
static unsigned int initial_status;
static bool defer_worker;
static u8 *allocation;
static size_t allocation_size;

static void make_info(void)
{
    memset(packet, 0, sizeof(packet));
    put_unaligned_le16(HGIC_HDR_RX_MAGIC, packet);
    packet[2] = HGIC_HDR_TYPE_CMD; packet[3] = 1;
    put_unaligned_le16(36, packet + 4);
    packet[8] = HGIC_CMD_GET_FW_INFO;
    put_unaligned_le16(24, packet + 10);
    put_unaligned_le32(0x02010001, packet + 12);
    put_unaligned_le32(22118, packet + 16);
    put_unaligned_le16(HGIC_WLAN_8400, packet + 20);
    put_unaligned_le16(1, packet + 22);
    memcpy(packet + 24, (u8[]){2, 0x11, 0x22, 0x33, 0x44, 0x55}, 6);
    put_unaligned_le32(0x12345678, packet + 32);
}

static void event_unqualified(struct hgic_fwctrl *c, struct sk_buff *skb)
{ (void)c; (void)skb; fputs("event consumer is NOT qualified or executed\n", stderr); exit(9); }

static void completion(void)
{
    size_t n = actual_len > 0 && actual_len <= HGIC_PKT_MAX_LEN + 1 ? actual_len : 36;
    allocation_size = n + displacement;
    allocation = malloc(allocation_size ? allocation_size : 1);
    CHECK(allocation != NULL);
    memset(allocation, 0xcc, allocation_size);
    if (n) memcpy(allocation + displacement, packet, n);
    u8 *snapshot = malloc(allocation_size ? allocation_size : 1);
    CHECK(snapshot != NULL);
    memcpy(snapshot, allocation, allocation_size);
    buf.data = null_data ? NULL : allocation + displacement;
    urb.context = &buf;
    urb.transfer_buffer = wrong_buffer ? packet : buf.data;
    urb.transfer_buffer_length = transfer_len;
    urb.actual_length = actual_len;
    urb.status = completion_status;
    buf.flag = HGIC_USB_BUF_FLAG_USED | HGIC_USB_BUF_FLAG_RX;
    INIT_LIST_HEAD(&buf.list);
    list_add_tail(&buf.list, &usb.used);
    unsigned int before = submissions;
    hgic_usb_receive(&urb);
    /* LE host normalization is identity, but all rejected inputs must also be
     * byte-for-byte unchanged, including alignment and untrimmed surplus. */
    CHECK(!memcmp(allocation, snapshot, allocation_size));
    if (!defer_worker) hgic_fwctrl_work(&hg.ctrl.work);
    CHECK(submissions <= before + 1);
    /* Fake submitted URBs remain owned until this deterministic test retires it.
     * This is not a real USB drain: no kernel backing is freed by candidate code. */
    list_del(&buf.list);
    free(snapshot); free(allocation); allocation = NULL;
}

static void schedule(struct hgic_fwctrl *c)
{
    struct sk_buff *tx = skb_dequeue(&c->txq);
    CHECK(tx != NULL); sends++;
    if (mode != 4)
        put_unaligned_le16(get_unaligned_le16(tx->data + 6), packet + 6 + cookie_offset);
    dev_kfree_skb_any(tx);
    completion();
    if (mode == 1) {
        /* A valid later callback cannot repair either USB ERR or sticky latch. */
        make_info(); completion_status = 0; actual_len = 36;
        completion();
    } else if (mode == 2) {
        /* Good completion first; error+zero arrives before locked A1 copy. */
        completion_status = -EIO; actual_len = 0; completion();
    } else if (mode == 3) {
        /* Worker sees an unknown cookie after the callback already resubmitted.
         * There is intentionally no retroactive cancellation/drain claim. */
        packet[6]++; completion();
    }
}

static void init(void)
{
    memset(&hg, 0, sizeof(hg)); memset(&usb, 0, sizeof(usb));
    memset(&buf, 0, sizeof(buf)); memset(&urb, 0, sizeof(urb));
    usb.bus.drv_tx_headroom = 3;
    usb.bus.bus_priv = &hg; usb.bus.rx_packet = hgics_rx_data;
    usb.bus.rx_error = hgics_rx_error;
    hg.bus = &usb.bus; hg.ctrl.bus = &usb.bus;
    hg.ctrl.schedule = schedule; hg.ctrl.rx_event = event_unqualified;
    hg.ctrl.wq = &wq; INIT_LIST_HEAD(&hg.ctrl.pd_list);
    INIT_LIST_HEAD(&usb.used); INIT_LIST_HEAD(&usb.rx_freeq);
    INIT_LIST_HEAD(&usb.tx_freeq);
    buf.usbdev = &usb; buf.urb = &urb;
    actual_len = 36; completion_status = 0; transfer_len = HGIC_PKT_MAX_LEN;
    displacement = mode = null_data = wrong_buffer = submit_error = cookie_offset = 0;
    initial_status = 0; defer_worker = false;
    make_info();
}

static void cleanup(void)
{
    struct sk_buff *s;
    while ((s = skb_dequeue(&hg.ctrl.rxq))) dev_kfree_skb_any(s);
    while ((s = skb_dequeue(&hg.ctrl.txq))) dev_kfree_skb_any(s);
    CHECK(hg.ctrl.pd_list.next == &hg.ctrl.pd_list);
    CHECK(!live_skb);
}

static int command(u8 *out)
{
    usb.status = initial_status;
    return hgic_fwctrl_do_cmd(&hg.ctrl, 1, HGIC_CMD_GET_FW_INFO, NULL, 0, out, 24);
}

static void refuse(bool sticky)
{
    u8 out[26]; memset(out, 0xa5, sizeof(out));
    unsigned int before = submissions, before_sends = sends;
    CHECK(command(out + 1) < 0);
    for (size_t i = 0; i < sizeof(out); i++) CHECK(out[i] == 0xa5);
    if (submissions != before)
        fprintf(stderr, "unexpected submit: actual=%d status=%d type=%u declared=%u mode=%d initial=%u\n",
                actual_len, completion_status, packet[2], get_unaligned_le16(packet + 4), mode, initial_status);
    CHECK(submissions == before);
    CHECK(sends == before_sends + 1);
    if (sticky) {
        CHECK(hg.ctrl.receipt_error == -EPROTO);
        CHECK(usb.bus.rx_failed == 1);
        CHECK(usb.status & HGIC_USB_STATUS_ERR);
        unsigned int saved = sends;
        /* Clearing the local transport ERR is not clearing A1's receipt latch. */
        usb.status = initial_status = 0; make_info(); actual_len = 36; completion_status = 0;
        CHECK(command(out + 1) == -EPROTO); CHECK(sends == saved);
    }
    rejected++; cleanup();
}

static void test_usb_and_extents(void)
{
    for (int offset = 0; offset < 8; offset++) {
        for (int n = 0; n < 36; n++) { init(); displacement = offset; actual_len = n; refuse(true); }
        init(); displacement = offset;
        u8 out[26]; memset(out, 0xa5, sizeof(out)); unsigned int before = submissions;
        CHECK(command(out + 1) == 24);
        CHECK(submissions == before + 1 && !hg.ctrl.receipt_error && !usb.bus.rx_failed);
        CHECK(out[0] == 0xa5 && out[25] == 0xa5 && !memcmp(out + 1, packet + 12, 24));
        cleanup();
    }
    for (int n = 37; n <= HGIC_PKT_MAX_LEN; n++) {
        init(); actual_len = n; refuse(true); /* zero surplus is forbidden too */
    }
    for (unsigned int declared = 0; declared <= 65535; declared++) {
        if (declared == 36) continue;
        init(); put_unaligned_le16(declared, packet + 4); refuse(true);
    }
    for (int n = 8; n < 12; n++) {
        init(); actual_len = n; put_unaligned_le16(n, packet + 4); refuse(true);
    }
    for (int type = 3; type <= 13; type += 10) {
        for (int n = 8; n <= 40; n++) {
            if (n == 36) continue;
            init(); packet[2] = type; actual_len = n; refuse(true);
        }
    }
    for (int status = 0; status < 6; status++) {
        for (int zero = 0; zero < 2; zero++) {
            init(); completion_status = (int[]){-EIO,-EPIPE,-EPROTO,-ECONNRESET,-ENOENT,1}[status];
            actual_len = zero ? 0 : 36; refuse(true);
        }
    }
    init(); actual_len = -1; refuse(true);
    init(); actual_len = HGIC_PKT_MAX_LEN + 1; refuse(true);
    init(); actual_len = HGIC_PKT_MAX_LEN + 1;
    put_unaligned_le16(actual_len, packet + 4); refuse(true);
    init(); actual_len = INT_MAX; refuse(true);
    init(); transfer_len = -1; refuse(true);
    init(); transfer_len = 35; refuse(true);
    init(); transfer_len = HGIC_PKT_MAX_LEN + 1; refuse(true);
    init(); null_data = 1; refuse(true);
    init(); wrong_buffer = 1; refuse(true);
    init(); packet[0] ^= 1; refuse(true);
    /* No control alignment-prefix wire evidence: conservatively refuse it. */
    for (int prefix = 1; prefix <= 5; prefix++) {
        init(); memmove(packet + prefix, packet, 36); memset(packet, 0xff, prefix);
        actual_len = 36 + prefix; cookie_offset = prefix; refuse(true);
    }
}

static void test_latches_and_cancellation(void)
{
    init(); completion_status = -EIO; mode = 1; refuse(true);
    init(); actual_len = 37; mode = 1; refuse(true);
    init(); initial_status = HGIC_USB_STATUS_ERR; refuse(true);
    for (int status = 0; status < 3; status++) {
        init(); initial_status = HGIC_USB_STATUS_STOP;
        completion_status = (int[]){0,-ENOENT,-ECONNRESET}[status];
        refuse(false); CHECK(!hg.ctrl.receipt_error && !usb.bus.rx_failed);
    }
    init(); mode = 2;
    u8 out[26]; memset(out, 0xa5, sizeof(out)); unsigned int before = submissions;
    CHECK(command(out + 1) == -EPROTO); CHECK(submissions == before + 1);
    for (size_t i = 0; i < sizeof(out); i++) CHECK(out[i] == 0xa5);
    cleanup();
    /* Actual association error in asynchronous worker sets bus rx_failed; next
     * callback and direct submit refuse even with USB status bits cleared. */
    init(); mode = 4; put_unaligned_le16(99, packet + 6);
    CHECK(command(out + 1) == -EPROTO); CHECK(usb.bus.rx_failed);
    usb.status = 0; before = submissions; make_info(); completion();
    CHECK(submissions == before); cleanup();
    init(); usb.bus.rx_failed = 1;
    INIT_LIST_HEAD(&buf.list); list_add_tail(&buf.list, &usb.used);
    before = submissions;
    CHECK(hgic_usb_submit_rx_urb(&buf) != 0); CHECK(submissions == before);
    list_del(&buf.list); cleanup();
    init(); submit_error = -ENOMEM;
    /* First good response exists but submission failure occurs before its worker
     * and copy; pending station waiter must be poisoned without a second TX. */
    before = cancel_calls; memset(out, 0xa5, sizeof(out));
    CHECK(command(out + 1) == -EPROTO); CHECK(cancel_calls == before + 1);
    for (size_t i = 0; i < sizeof(out); i++) CHECK(out[i] == 0xa5);
    cleanup();
    /* Real bad-receipt helper completes every pending station waiter, without
     * freeing their unknown skb backing or treating maintenance as station. */
    init();
    struct hgic_cmd_response a = { .type = HGIC_HDR_TYPE_CMD },
                             b = { .type = HGIC_HDR_TYPE_CMD2 },
                             maintenance = { .type = HGIC_HDR_TYPE_BOOTDL };
    list_add(&a.list, &hg.ctrl.pd_list); list_add(&b.list, &hg.ctrl.pd_list);
    list_add(&maintenance.list, &hg.ctrl.pd_list);
    completion_status = -EIO; actual_len = 0; before = sends;
    completion();
    CHECK(sends == before && a.done && b.done);
    CHECK(a.cmpl.done == 1 && b.cmpl.done == 1 && !maintenance.cmpl.done && !maintenance.done);
    CHECK(!a.skb && !b.skb && !maintenance.skb);
    list_del(&a.list); list_del(&b.list); list_del(&maintenance.list); cleanup();
}

static void test_real_association(void)
{
    u8 out[26]; struct hgic_fw_info info;
    for (int which = 0; which < 9; which++) {
        init();
        if (which == 0) packet[3] = 2;
        if (which == 1) packet[8]++;
        if (which == 2) packet[2] = HGIC_HDR_TYPE_CMD2;
        if (which == 3) packet[12]++;
        if (which == 4) packet[16]++;
        if (which == 5) packet[20]++;
        if (which == 6) packet[22]++;
        if (which == 7) memset(packet + 24, 0, 6);
        if (which == 8) packet[24] |= 1;
        memset(out, 0xa5, sizeof(out));
        CHECK(command(out + 1) < 0);
        for (size_t i = 0; i < sizeof(out); i++) CHECK(out[i] == 0xa5);
        cleanup();
    }
    init(); memset(&info, 0xa5, sizeof(info));
    CHECK(hgic_fwctrl_get_fwinfo(&hg.ctrl, 1, &info) == 24);
    CHECK(info.version == 0x02010001 && info.svn_version == 22118);
    CHECK(info.chip_id == 0x8400 && info.cpu_id == 1 && info.app_version == 0);
    CHECK(info.smt_dat == 0x12345678 && info.resv[0] == 0 && info.resv[1] == 0);
    cleanup();
    /* Full buffer, exact non-FWINFO control exercises original actual extent;
     * no full-buffer data/aggregate grammar or event consumer is qualified. */
    init(); packet[8] = HGIC_CMD_SET_MAC; actual_len = HGIC_PKT_MAX_LEN;
    put_unaligned_le16(actual_len, packet + 4); put_unaligned_le16(0, packet + 10);
    unsigned int before = submissions;
    CHECK(hgic_fwctrl_do_cmd(&hg.ctrl, 1, HGIC_CMD_SET_MAC, NULL, 0, NULL, 0) == 0);
    CHECK(submissions == before + 1); cleanup();
}

int main(void)
{
    u16 endian = 1; CHECK(*(u8 *)&endian == 1);
#ifdef A2A_ORIGINAL_REPRO
    init(); actual_len = 40; u8 out[24];
    CHECK(command(out) == 24); CHECK(!memcmp(out, packet + 12, 24));
    CHECK(!hg.ctrl.receipt_error); cleanup();
    puts("REPRODUCED A1 real USB->rx trim->strict copy accepts 4 surplus zero bytes");
#else
    test_usb_and_extents(); test_latches_and_cancellation(); test_real_association();
    CHECK(!live_skb); CHECK(data_calls == 0);
    printf("PASS A2a actual USB->core ingress->A1 copy: %u assertions, %u rejected cases, %u modeled sends, %u submissions\n", checks, rejected, sends, submissions);
#endif
    return 0;
}
