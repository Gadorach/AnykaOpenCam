/* Included after sealed extraction of the real fwctrl.c functions. */
_Static_assert(sizeof(struct hgic_hdr) == 8, "selected outer header");
_Static_assert(sizeof(struct hgic_ctrl_hdr) == 12, "selected full control header");
_Static_assert(offsetof(struct hgic_ctrl_hdr, cmd.status) == 10, "CMD status includes padding byte9");
_Static_assert(offsetof(struct hgic_ctrl_hdr, cmd2.status) == 10, "CMD2 status");
_Static_assert(HGIC_CMD_START == 0 && HGIC_CMD_GET_FW_INFO == 43, "selected Linux opcode");
static unsigned int checks, sends, events;
#define CHECK(x) do { checks++; if (!(x)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); exit(2); } } while (0)
static u8 reply[128];
static size_t reply_len;
static int scenario;
static struct hgic_bus bus = { .drv_tx_headroom = 3 };
static struct workqueue_struct wq;

static void make_reply(u16 opcode, u16 cookie, int status, size_t payload)
{
    memset(reply, 0, sizeof(reply));
    put_unaligned_le16(HGIC_HDR_RX_MAGIC, reply);
    reply[2] = HGIC_HDR_TYPE_CMD;
    reply[3] = HGIC_WDEV_ID_STA;
    reply_len = 12 + payload;
    put_unaligned_le16(reply_len, reply + 4);
    put_unaligned_le16(cookie, reply + 6);
    reply[8] = opcode;
    put_unaligned_le16((u16)status, reply + 10);
}

static void make_info(u16 cookie)
{
    make_reply(HGIC_CMD_GET_FW_INFO, cookie, 24, 24);
    put_unaligned_le32(0x02010001, reply + 12);
    put_unaligned_le32(22118, reply + 16);
    put_unaligned_le16(HGIC_WLAN_8400, reply + 20);
    put_unaligned_le16(1, reply + 22);
    /* Public synthetic locally administered unicast; never real device bytes. */
    memcpy(reply + 24, (const u8[]){2, 0x11, 0x22, 0x33, 0x44, 0x55}, 6);
    put_unaligned_le32(0x12345678, reply + 32);
}

static void inject(struct hgic_fwctrl *c)
{
    hgic_fwctrl_rx(c, reply, (int)reply_len);
    hgic_fwctrl_work(&c->work);
}

static void event_stub(struct hgic_fwctrl *c, struct sk_buff *skb)
{ (void)c; events++; dev_kfree_skb_any(skb); }

static void schedule_stub(struct hgic_fwctrl *c)
{
    struct sk_buff *tx = skb_dequeue(&c->txq);
    CHECK(tx != NULL);
    sends++;
    /* Model the selected outer header's RX CPU-endian normalization. Host and
     * retained target are little-endian, checked in main, not a portability claim.
     */
    if (scenario != 3)
        put_unaligned_le16(get_unaligned_le16(tx->data + 6), reply + 6);
    dev_kfree_skb_any(tx);
    if (scenario == 1)
        return; /* genuine timeout in the deterministic completion model */
    if (scenario == 2) {
        u8 saved_type = reply[2];
        reply[2] = HGIC_HDR_TYPE_EVENT;
        inject(c);
        reply[2] = saved_type;
    }
    inject(c);
    if (scenario == 4) {
        /* Same-cookie duplicate, made observably different. First must win. */
        reply[24] ^= 4;
        inject(c);
    }
    if (scenario == 5) {
        /* Bad first receipt followed by a valid packet cannot repair failure. */
        make_info(get_unaligned_le16(reply + 6));
        inject(c);
    }
    if (scenario == 6) {
        /* A subsequent unknown association before copy must revoke acceptance,
         * even if the good reply had already completed its pending record.
         */
        reply[6]++;
        inject(c);
    }
}

static void init_ctrl(struct hgic_fwctrl *c)
{
    memset(c, 0, sizeof(*c));
    INIT_LIST_HEAD(&c->pd_list);
    c->bus = &bus;
    c->wq = &wq;
    c->schedule = schedule_stub;
    c->rx_event = event_stub;
    scenario = 0;
}

static void clean_ctrl(struct hgic_fwctrl *c)
{
    struct sk_buff *s;
    while ((s = skb_dequeue(&c->txq))) dev_kfree_skb_any(s);
    while ((s = skb_dequeue(&c->rxq))) dev_kfree_skb_any(s);
    CHECK(c->pd_list.next == &c->pd_list && c->pd_list.prev == &c->pd_list);
    CHECK(!live_skb);
}

#ifdef A1_LEGACY
int main(int argc, char **argv)
{
    struct hgic_fwctrl c;
    struct hgic_fw_info info;
    u8 out[24];
    (void)argv;
    init_ctrl(&c);
    make_info(0);
    if (argc == 1) {
        put_unaligned_le32(0, reply + 12);
        memset(reply + 24, 0, 6);
        CHECK(hgic_fwctrl_get_fwinfo(&c, 1, &info) == 24);
        CHECK(info.version == 0 && info.mac[0] == 0);
        puts("REPRODUCED original accepts zero version and zero MAC");
    } else {
        reply_len = 12;
        put_unaligned_le16(12, reply + 4);
        (void)hgic_fwctrl_do_cmd(&c, 1, 43, NULL, 0, out, sizeof(out));
        fputs("unexpected: legacy short-payload did not trip ASan\n", stderr);
        return 3;
    }
    clean_ctrl(&c);
    return 0;
}
#else
static void output_unchanged(const u8 *out, size_t n)
{ for (size_t i = 0; i < n; i++) CHECK(out[i] == 0xa5); }

static void expect_reject_current(void)
{
    struct hgic_fwctrl c;
    u8 guarded[26];
    unsigned int before = sends;
    init_ctrl(&c);
    memset(guarded, 0xa5, sizeof(guarded));
    CHECK(hgic_fwctrl_do_cmd(&c, 1, 43, NULL, 0, guarded + 1, 24) < 0);
    output_unchanged(guarded, sizeof(guarded));
    CHECK(sends == before + 1);
    clean_ctrl(&c);
}

static void test_real_copy_path(void)
{
    struct hgic_fwctrl c;
    u8 guarded[26];
    for (size_t n = 1; n < 36; n++) {
        make_info(0);
        reply_len = n;
        expect_reject_current();
    }
    /* Zero length is refused before allocation, still no output write. */
    make_info(0); reply_len = 0; expect_reject_current();
    for (unsigned int n = 0; n < 80; n++) {
        if (n == 36) continue;
        make_info(0); reply_len = n; put_unaligned_le16(n, reply + 4);
        expect_reject_current();
    }
    for (unsigned int n = 0; n <= 0xffff; n += 257) {
        if (n == 36) continue;
        make_info(0); put_unaligned_le16(n, reply + 4); expect_reject_current();
    }
    for (unsigned int value = 0; value < 256; value++) {
        if (value != 3 && value != 4 && value != 14) {
            make_info(0); reply[2] = value; expect_reject_current();
        }
        if ((value & 15) != 1) {
            make_info(0); reply[3] = value; expect_reject_current();
        }
        if (value != 43) {
            make_info(0); reply[8] = value; expect_reject_current();
        }
    }
    for (size_t i = 12; i < 24; i++) {
        for (unsigned int bit = 0; bit < 8; bit++) {
            make_info(0); reply[i] ^= (1u << bit); expect_reject_current();
        }
    }
    for (unsigned int n = 0; n < 6; n++) {
        make_info(0);
        if (n == 0) memset(reply + 24, 0, 6);
        if (n == 1) memset(reply + 24, 0xff, 6);
        if (n == 2) reply[24] = 1;
        if (n == 3) reply[24] = 0x33;
        if (n == 4) reply[24] = 0xff;
        if (n == 5) reply[24] = 3;
        expect_reject_current();
    }
    make_info(0); put_unaligned_le16(23, reply + 10); expect_reject_current();
    make_info(0); put_unaligned_le16(25, reply + 10); expect_reject_current();
    make_info(0); put_unaligned_le16(0x7fff, reply + 10); expect_reject_current();
    make_info(0); reply[0] ^= 1; expect_reject_current();
    for (unsigned int cap = 0; cap < 24; cap++) {
        init_ctrl(&c); make_info(0); memset(guarded, 0xa5, sizeof(guarded));
        CHECK(hgic_fwctrl_do_cmd(&c, 1, 43, NULL, 0, guarded + 1, cap) == -EMSGSIZE);
        output_unchanged(guarded, sizeof(guarded)); clean_ctrl(&c);
    }
    for (unsigned int first = 0; first < 256; first += 2) {
        init_ctrl(&c); make_info(0); reply[24] = first;
        memset(guarded, 0xa5, sizeof(guarded));
        CHECK(hgic_fwctrl_do_cmd(&c, 1, 43, NULL, 0, guarded + 1, 24) == 24);
        CHECK(guarded[0] == 0xa5 && guarded[25] == 0xa5);
        CHECK(!memcmp(guarded + 1, reply + 12, 24)); clean_ctrl(&c);
    }
}

static void test_signed_setter_semantics(void)
{
    static const u16 allowed[] = {1, 2, 3, 8, 9, 13, 21, 22, 24, 25, 26, 27, 36, 38};
    static const int values[] = {-32768, -4095, -257, -71, -22, -1, 0, 1, 24, 32767};
    struct hgic_fwctrl c;
    u8 out[24];
    for (size_t i = 0; i < sizeof(allowed)/sizeof(*allowed); i++) {
        for (size_t j = 0; j < sizeof(values)/sizeof(*values); j++) {
            init_ctrl(&c); make_reply(allowed[i], 0, values[j], 0);
            CHECK(hgic_fwctrl_do_cmd(&c, 1, allowed[i], NULL, 0, NULL, 0) == values[j]);
            CHECK(!c.receipt_error); clean_ctrl(&c);
        }
    }
    /* Legitimate negative FWINFO error is returned intact, with no output copy,
     * including bounded optional error bytes: no guessed error-payload ABI.
     */
    for (size_t j = 0; j < 6; j++) {
        for (size_t payload = 0; payload <= 24; payload++) {
            init_ctrl(&c); make_reply(43, 0, values[j], payload);
            memset(out, 0xa5, sizeof(out));
            CHECK(hgic_fwctrl_do_cmd(&c, 1, 43, NULL, 0, out, sizeof(out)) == values[j]);
            output_unchanged(out, sizeof(out)); CHECK(!c.receipt_error); clean_ctrl(&c);
        }
    }
    init_ctrl(&c); make_reply(3, 0, 4, 4); memcpy(reply + 12, "ABCD", 4);
    memset(out, 0xa5, sizeof(out));
    CHECK(hgic_fwctrl_do_cmd(&c, 1, 3, NULL, 0, out, 4) == 4);
    CHECK(!memcmp(out, "ABCD", 4)); output_unchanged(out + 4, 20); clean_ctrl(&c);
    init_ctrl(&c); make_reply(3, 0, 5, 4); memset(out, 0xa5, sizeof(out));
    CHECK(hgic_fwctrl_do_cmd(&c, 1, 3, NULL, 0, out, 24) < 0);
    output_unchanged(out, 24); clean_ctrl(&c);
    /* A setter/scalar caller never interprets bounded optional reply bytes as
     * a length. Do not invent an error/scalar payload ABI absent source proof.
     */
    init_ctrl(&c); make_reply(3, 0, 5, 4);
    CHECK(hgic_fwctrl_do_cmd(&c, 1, 3, NULL, 0, NULL, 0) == 5);
    CHECK(!c.receipt_error); clean_ctrl(&c);
}

static void test_correlation_and_replay(void)
{
    struct hgic_fwctrl c;
    u8 out[24];
    unsigned int before;
    init_ctrl(&c); make_info(0); reply[8]++; scenario = 5;
    memset(out, 0xa5, sizeof(out));
    CHECK(hgic_fwctrl_do_cmd(&c, 1, 43, NULL, 0, out, 24) == -EPROTO);
    output_unchanged(out, 24); CHECK(c.receipt_error); before = sends;
    make_info(0);
    CHECK(hgic_fwctrl_do_cmd(&c, 1, 43, NULL, 0, out, 24) == -EPROTO);
    CHECK(sends == before); clean_ctrl(&c);

    /* Replayed completed receipt, not just a late timed-out one. */
    init_ctrl(&c); make_info(0);
    CHECK(hgic_fwctrl_do_cmd(&c, 1, 43, NULL, 0, out, 24) == 24);
    inject(&c); CHECK(c.receipt_error); before = sends;
    memset(out, 0xa5, 24); make_info(1);
    CHECK(hgic_fwctrl_do_cmd(&c, 1, 43, NULL, 0, out, 24) == -EPROTO);
    output_unchanged(out, 24); CHECK(sends == before); clean_ctrl(&c);

    init_ctrl(&c); make_info(0); scenario = 4;
    CHECK(hgic_fwctrl_do_cmd(&c, 1, 43, NULL, 0, out, 24) == 24);
    CHECK(out[12] == 2); CHECK(!c.receipt_error); clean_ctrl(&c);

    init_ctrl(&c); make_info(0); scenario = 6; memset(out, 0xa5, 24);
    CHECK(hgic_fwctrl_do_cmd(&c, 1, 43, NULL, 0, out, 24) == -EPROTO);
    output_unchanged(out, 24); CHECK(c.receipt_error); clean_ctrl(&c);

    init_ctrl(&c); make_info(0); scenario = 2; before = events;
    CHECK(hgic_fwctrl_do_cmd(&c, 1, 43, NULL, 0, out, 24) == 24);
    CHECK(events == before + 1 && !c.receipt_error); clean_ctrl(&c);
    for (unsigned int type = 4; type <= 14; type += 10) {
        init_ctrl(&c); make_info(0); reply[2] = type;
        memset(out, 0xa5, sizeof(out));
        CHECK(hgic_fwctrl_do_cmd(&c, 1, 43, NULL, 0, out, 24) < 0);
        output_unchanged(out, 24); CHECK(!c.receipt_error); clean_ctrl(&c);
    }
    init_ctrl(&c); make_info(1); scenario = 3; memset(out, 0xa5, 24);
    CHECK(hgic_fwctrl_do_cmd(&c, 1, 43, NULL, 0, out, 24) == -EPROTO);
    output_unchanged(out, 24); CHECK(c.receipt_error); clean_ctrl(&c);

    /* Timeout removes the pending association. A delayed packet poisons future
     * control use; no cookie reuse/rebase turns it into the next command reply.
     */
    init_ctrl(&c); make_info(0); scenario = 1;
    CHECK(hgic_fwctrl_do_cmd(&c, 1, 43, NULL, 0, out, 24) < 0);
    scenario = 0; inject(&c); CHECK(c.receipt_error); before = sends;
    CHECK(hgic_fwctrl_do_cmd(&c, 1, 43, NULL, 0, out, 24) == -EPROTO);
    CHECK(sends == before); clean_ctrl(&c);

    init_ctrl(&c);
    for (u32 n = 0; n <= HGIC_TX_COOKIE_MASK; n++) CHECK(hgic_ctrl_cookie(&c) == n);
    for (u32 n = 0; n < 65540; n++) CHECK(hgic_ctrl_cookie(&c) == 0xffff);
    make_info(0); before = sends;
    CHECK(hgic_fwctrl_do_cmd(&c, 1, 43, NULL, 0, out, 24) < 0);
    CHECK(sends == before); clean_ctrl(&c);
}

static void test_fwinfo_wrapper_and_admission(void)
{
    struct hgic_fwctrl c;
    struct hgic_fw_info info, before_info;
    unsigned int before;
    u8 one = 0;
    init_ctrl(&c); make_info(0); memset(&info, 0xa5, sizeof(info));
    CHECK(hgic_fwctrl_get_fwinfo(&c, 1, &info) == 24);
    CHECK(info.version == 0x02010001 && info.svn_version == 22118);
    CHECK(info.chip_id == HGIC_WLAN_8400 && info.cpu_id == 1);
    CHECK(info.app_version == 0 && info.smt_dat == 0x12345678);
    CHECK(info.resv[0] == 0 && info.resv[1] == 0);
    CHECK(!memcmp(info.mac, reply + 24, 6)); clean_ctrl(&c);
    for (int n = 0; n < 5; n++) {
        init_ctrl(&c); make_info(0); memset(&info, 0xa5, sizeof(info)); before_info = info;
        if (n == 0) memset(reply + 24, 0, 6);
        if (n == 1) reply[12]++;
        if (n == 2) reply[16]++;
        if (n == 3) reply[20]++;
        if (n == 4) reply[22]++;
        CHECK(hgic_fwctrl_get_fwinfo(&c, 1, &info) < 0);
        CHECK(!memcmp(&info, &before_info, sizeof(info))); clean_ctrl(&c);
    }
    init_ctrl(&c); make_info(0); before = sends;
    CHECK(hgic_fwctrl_get_fwinfo(&c, 1, NULL) == -EINVAL);
    CHECK(hgic_fwctrl_do_cmd(&c, 1, 43, NULL, 1, &one, 1) == -EINVAL);
    CHECK(hgic_fwctrl_do_cmd(&c, 1, 43, NULL, 0, NULL, 1) == -EINVAL);
    CHECK(hgic_fwctrl_do_cmd(&c, 1, 43, &one, 2001, NULL, 0) == -EINVAL);
    CHECK(hgic_fwctrl_do_cmd(&c, 2, 43, NULL, 0, NULL, 0) < 0);
    CHECK(hgic_fwctrl_do_cmd(&c, 1, HGIC_CMD_SAVE_CFG, NULL, 0, NULL, 0) < 0);
    CHECK(sends == before); clean_ctrl(&c);

    for (int n = 0; n < 4; n++) {
        init_ctrl(&c); make_info(0); before = sends;
        if (n == 0) hgic_fwctrl_rx(&c, NULL, 36);
        if (n == 1) hgic_fwctrl_rx(&c, reply, -1);
        if (n == 2) hgic_fwctrl_rx(&c, reply, 0);
        if (n == 3) hgic_fwctrl_rx(&c, reply, 65536);
        CHECK(c.receipt_error);
        CHECK(hgic_fwctrl_get_fwinfo(&c, 1, &info) == -EPROTO);
        CHECK(sends == before); clean_ctrl(&c);
    }
}

static void test_pure_bounds_and_cmd2(void)
{
    u8 out[24], *p;
    const u8 *volatile null_frame = NULL;
    /* Exact-size allocations, every unaligned displacement. The real copy gate
     * runs under ASan, not just a padded synthetic frame buffer.
     */
    for (size_t offset = 0; offset < 8; offset++) {
        for (size_t n = 0; n < 36; n++) {
            make_info(7); p = malloc(n + offset + 1); CHECK(p != NULL);
            memcpy(p + offset + 1, reply, n); memset(out, 0xa5, 24);
            CHECK(hgic_ctrl_reply_copy(p + offset + 1, n, 3, 1, 43, 7, out, 24) < 0);
            output_unchanged(out, 24); free(p);
        }
        make_info(7); p = malloc(36 + offset + 1); CHECK(p != NULL);
        memcpy(p + offset + 1, reply, 36);
        CHECK(hgic_ctrl_reply_copy(p + offset + 1, 36, 3, 1, 43, 7, out, 24) == 24);
        free(p);
    }
    make_info(7); memset(out, 0xa5, 24);
    CHECK(hgic_ctrl_reply_copy(null_frame, 36, 3, 1, 43, 7, out, 24) < 0);
    CHECK(hgic_ctrl_reply_copy(reply, SIZE_MAX, 3, 1, 43, 7, out, 24) < 0);
    CHECK(hgic_ctrl_reply_copy(reply, 36, 3, 1, 43, 8, out, 24) < 0);
    CHECK(hgic_ctrl_reply_copy(reply, 36, 3, 2, 43, 7, out, 24) < 0);
    CHECK(hgic_ctrl_reply_copy(reply, 36, 13, 1, 43, 7, out, 24) < 0);
    output_unchanged(out, 24);
    CHECK(hgic_ctrl_reply_copy(reply, 36, 3, 1, 43, 7, NULL, 24) == -EINVAL);
    /* Helper's CMD2 branch uses u16 opcode/status at actual selected offsets;
     * the station allowlist itself does not add any CMD2 commands.
     */
    make_reply(3, 7, 4, 4); reply[2] = 13; put_unaligned_le16(0x123, reply + 8);
    memcpy(reply + 12, "WXYZ", 4);
    CHECK(hgic_ctrl_reply_copy(reply, 16, 13, 1, 0x123, 7, out, 24) == 4);
    CHECK(!memcmp(out, "WXYZ", 4));
    memset(out, 0xa5, 24);
    CHECK(hgic_ctrl_reply_copy(reply, 16, 13, 1, 0x223, 7, out, 24) < 0);
    output_unchanged(out, 24);
}

int main(void)
{
    const u16 endian = 1;
    CHECK(*(const u8 *)&endian == 1);
    test_real_copy_path();
    test_signed_setter_semantics();
    test_correlation_and_replay();
    test_fwinfo_wrapper_and_admission();
    test_pure_bounds_and_cmd2();
    CHECK(!live_skb);
    printf("PASS A1 real control paths: %u checks; %u modeled sends; %u event dispatches\n", checks, sends, events);
    return 0;
}
#endif
