/* Review-only synthetic behavior probes. No target or kernel I/O. */
#define main author_main
#include "candidate.c"
#undef main

static int review_mode;
static void review_schedule(struct hgic_fwctrl *c)
{
    struct sk_buff *tx = skb_dequeue(&c->txq);
    CHECK(tx != NULL);
    sends++;
    put_unaligned_le16(get_unaligned_le16(tx->data + 6), reply + 6);
    dev_kfree_skb_any(tx);
    inject(c);
    if (review_mode == 1) {
        reply[8]++; /* Wrong opcode AFTER first completion, same cookie. */
        inject(c);
    } else if (review_mode == 2) {
        make_info(get_unaligned_le16(reply + 6)); /* Valid after signed error. */
        inject(c);
    } else if (review_mode == 3) {
        reply[2] = HGIC_HDR_TYPE_CMD2; /* Type conflict after first completion. */
        inject(c);
    } else if (review_mode == 4) {
        reply_len = 11; /* Malformed outer frame is NOT an ignored duplicate. */
        inject(c);
    }
}

static void review_init(struct hgic_fwctrl *c, int mode)
{
    init_ctrl(c);
    c->schedule = review_schedule;
    review_mode = mode;
}

int main(void)
{
    struct hgic_fwctrl c;
    struct hgic_fw_info info, saved;
    u8 out[32];
    unsigned int before = checks;
    int status;

    /* First-wins is deliberately not validate-every-later-packet semantics. */
    for (int mode = 1; mode <= 3; mode += 2) {
        review_init(&c, mode); make_info(0); memset(out, 0xa5, sizeof(out));
        CHECK(hgic_fwctrl_do_cmd(&c, 1, 43, NULL, 0, out, 24) == 24);
        CHECK(!c.receipt_error && out[12] == 2);
        output_unchanged(out + 24, 8); clean_ctrl(&c);
    }
    review_init(&c, 2); make_reply(43, 0, -32768, 7);
    memset(&info, 0xa5, sizeof(info)); saved = info;
    CHECK(hgic_fwctrl_get_fwinfo(&c, 1, &info) == -32768);
    CHECK(!memcmp(&info, &saved, sizeof(info)) && !c.receipt_error);
    clean_ctrl(&c);

    review_init(&c, 4); make_info(0); memset(out, 0xa5, sizeof(out));
    CHECK(hgic_fwctrl_do_cmd(&c, 1, 43, NULL, 0, out, 24) == -EPROTO);
    CHECK(c.receipt_error); output_unchanged(out, sizeof(out)); clean_ctrl(&c);

    /* A legitimate short result may fill only its declared portion, never pad. */
    review_init(&c, 0); make_reply(3, 0, 4, 4); memcpy(reply + 12, "PART", 4);
    memset(out, 0xa5, sizeof(out));
    CHECK(hgic_fwctrl_do_cmd(&c, 1, 3, NULL, 0, out, sizeof(out)) == 4);
    CHECK(!memcmp(out, "PART", 4)); output_unchanged(out + 4, 28); clean_ctrl(&c);

    /* Copy-capacity rejection is local, not a persistent malformed-association
     * latch. Signed errors/zero/scalars ignore optional bounded bytes as stated.
     */
    review_init(&c, 0); make_reply(3, 0, 5, 4); memset(out, 0xa5, sizeof(out));
    CHECK(hgic_fwctrl_do_cmd(&c, 1, 3, NULL, 0, out, 32) == -EMSGSIZE);
    CHECK(!c.receipt_error); output_unchanged(out, sizeof(out)); clean_ctrl(&c);
    const int values[] = {-32768, -71, -1, 0, 1, 32767};
    for (size_t i = 0; i < sizeof(values) / sizeof(*values); i++) {
        review_init(&c, 0); make_reply(3, 0, values[i], 5);
        CHECK(hgic_fwctrl_do_cmd(&c, 1, 3, NULL, 0, NULL, 0) == values[i]);
        CHECK(!c.receipt_error); clean_ctrl(&c);
    }
    review_init(&c, 0); make_reply(3, 0, 0, 5); memset(out, 0xa5, sizeof(out));
    CHECK(hgic_fwctrl_do_cmd(&c, 1, 3, NULL, 0, out, 32) == 0);
    output_unchanged(out, sizeof(out)); clean_ctrl(&c);

    /* High header flag nibble and CMD's padding byte9 are not invented gates. */
    review_init(&c, 0); make_info(0); reply[3] = 0xf1; reply[9] = 0xff;
    CHECK(hgic_fwctrl_get_fwinfo(&c, 1, &info) == 24);
    CHECK(!c.receipt_error); clean_ctrl(&c);

    /* Wrong-type CMD2 does not satisfy a CMD request even with same low opcode. */
    review_init(&c, 0); make_info(0); reply[2] = HGIC_HDR_TYPE_CMD2;
    memset(out, 0xa5, sizeof(out));
    CHECK(hgic_fwctrl_do_cmd(&c, 1, 43, NULL, 0, out, 24) == -EPROTO);
    output_unchanged(out, sizeof(out)); clean_ctrl(&c);

    /* Entire signed16 domain through unaligned native/LE parser, no output on
     * helper rejection; FWINFO negative statuses are firmware errors, not lengths.
     */
    for (unsigned int bits = 0; bits <= 0xffff; bits++) {
        make_reply(3, 9, (s16)bits, 0);
        status = 999999;
        CHECK(hgic_ctrl_reply_status(reply, 12, 3, 1, 3, 9, &status) == 0);
        CHECK(status == (s16)bits);
    }
    make_reply(3, 9, 0, 0); reply[8]++;
    status = 999999;
    CHECK(hgic_ctrl_reply_status(reply, 12, 3, 1, 3, 9, &status) == -EPROTO);
    CHECK(status == 999999);

    /* The final cookie is usable once, then no further modeled TX. */
    review_init(&c, 0); c.cookie = HGIC_TX_COOKIE_MASK; make_info(0);
    CHECK(hgic_fwctrl_get_fwinfo(&c, 1, &info) == 24);
    unsigned int sent = sends;
    make_info(0);
    CHECK(hgic_fwctrl_get_fwinfo(&c, 1, &info) < 0);
    CHECK(sends == sent && c.cookie == HGIC_TX_COOKIE_MASK + 1); clean_ctrl(&c);

    /* Negative/short FWINFO leaves whole destination object, not just MAC. */
    for (int n = 0; n < 3; n++) {
        review_init(&c, 0); make_info(0);
        if (n == 0) { reply_len = 35; put_unaligned_le16(35, reply + 4); }
        if (n == 1) put_unaligned_le16(0xffff, reply + 10);
        if (n == 2) memset(reply + 24, 0xff, 6);
        memset(&info, 0xa5, sizeof(info)); saved = info;
        CHECK(hgic_fwctrl_get_fwinfo(&c, 1, &info) < 0);
        CHECK(!memcmp(&info, &saved, sizeof(info))); clean_ctrl(&c);
    }
    printf("PASS independent supplements: %u assertions; first-wins/local-copy policy confirmed\n", checks - before);
    return 0;
}
