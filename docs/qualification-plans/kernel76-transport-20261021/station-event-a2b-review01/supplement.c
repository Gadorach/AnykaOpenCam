/* Review-only public synthetic probes, not driver changes or target code. */
#include <sys/mman.h>
#include <unistd.h>

static unsigned int review_probes;

static void guarded_payload(void)
{
    const long page = sysconf(_SC_PAGESIZE);
    CHECK(page >= 4096);
    for (int type = 4; type <= 14; type += 10) {
        for (int k = 0; k < 4; k++) {
            const unsigned int id = (unsigned int[]){18, 31, 0x0112, 65535}[k];
            if (type == 4 && id > 255) continue;
            for (int size_index = 0; size_index < 3; size_index++) {
                const unsigned int n = (unsigned int[]){12, 13, 65535}[size_index];
                const bool failure = id != 18;
                init(false); make_event(type, id, n);
                /* Place the last header byte at a readable page boundary. All
                 * payload pages exist but are PROT_NONE: any payload read traps.
                 * skb allocation/free primitives are the inspected HOST model. */
                const size_t extent = (size_t)page +
                    ((n - 12 + page - 1) / page + 1) * (size_t)page;
                u8 *mapping = mmap(NULL, extent, PROT_NONE,
                                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
                CHECK(mapping != MAP_FAILED);
                CHECK(mprotect(mapping, page, PROT_READ | PROT_WRITE) == 0);
                struct sk_buff *s = dev_alloc_skb(1);
                s->data = mapping + page - 12; s->len = n;
                memcpy(s->data, packet, 12);
                watched_skb = s; watched_guard = NULL;
                review_free_ctrl = &hg.ctrl; review_expect_deny = failure;
                review_free_latched = 0;
                event_phase = true;
                hg.ctrl.rx_event(&hg.ctrl, s);
                event_phase = false;
                CHECK(event_frees == 1);
                CHECK(review_free_latched == (failure ? 1U : 0U));
                review_expect_deny = false;
                CHECK(munmap(mapping, extent) == 0);
                finish(failure); review_probes++;
            }
        }
    }
}

static void already_failed_does_not_inspect(void)
{
    for (int flag = 0; flag < 2; flag++) {
        init(false);
        if (flag) hg.ctrl.receipt_error = -EIO;
        else usb.bus.rx_failed = 1;
        struct sk_buff *s = dev_alloc_skb(1);
        s->data = (u8 *)(uintptr_t)1; s->len = UINT_MAX;
        nonlinear_skb = watched_skb = s;
        watched_guard = NULL;
        event_phase = true;
        hg.ctrl.rx_event(&hg.ctrl, s);
        hg.ctrl.rx_event(&hg.ctrl, NULL);
        event_phase = false;
        CHECK(event_frees == 1 && !pending.done && !error_completions);
        CHECK(hg.ctrl.receipt_error == (flag ? -EIO : 0));
        CHECK(usb.bus.rx_failed == (flag ? 0 : 1));
        nonlinear_skb = NULL;
        /* Fixture reset only; production has no failure-clear operation. */
        hg.ctrl.receipt_error = 0; usb.bus.rx_failed = 0;
        finish(false); review_probes++;
    }
}

static void pre_read_bounds_and_latch_order(void)
{
    for (unsigned int n = 0; n <= 11; n++) {
        init(false);
        struct sk_buff *s = dev_alloc_skb(n);
        s->len = n;
        review_free_ctrl = &hg.ctrl; review_expect_deny = true;
        review_free_latched = 0;
        event_phase = true;
        hg.ctrl.rx_event(&hg.ctrl, s);
        event_phase = false;
        CHECK(review_free_latched == 1);
        review_expect_deny = false;
        finish(true); review_probes++;
    }
    for (int special = 0; special < 4; special++) {
        init(false);
        struct sk_buff *s = dev_alloc_skb(1);
        s->data = special == 0 ? NULL : (u8 *)(uintptr_t)1;
        s->len = special == 1 ? 65536U : special == 2 ? UINT_MAX : 12U;
        if (special == 3) nonlinear_skb = s;
        review_free_ctrl = &hg.ctrl; review_expect_deny = true;
        review_free_latched = 0;
        hg.ctrl.rx_event(&hg.ctrl, s);
        CHECK(review_free_latched == 1);
        review_expect_deny = false; nonlinear_skb = NULL;
        finish(true); review_probes++;
    }
}

static void multi_waiter_backing(void)
{
    init(false);
    struct hgic_cmd_response cmd2 = {.type = HGIC_HDR_TYPE_CMD2},
        completed = {.type = HGIC_HDR_TYPE_CMD, .done = true},
        maintenance = {.type = HGIC_HDR_TYPE_BOOTDL};
    struct sk_buff *saved = dev_alloc_skb(1);
    *skb_put(saved, 1) = 0x75;
    completed.skb = saved; completed.cmpl.done = 1;
    list_add(&cmd2.list, &hg.ctrl.pd_list);
    list_add(&completed.list, &hg.ctrl.pd_list);
    list_add(&maintenance.list, &hg.ctrl.pd_list);
    make_event(14, 0x0112, 12);
    review_free_ctrl = &hg.ctrl; review_expect_deny = true; review_free_latched = 0;
    direct(12, 0, 0, true);
    CHECK(review_free_latched == 1 && error_completions == 2);
    review_expect_deny = false;
    CHECK(cmd2.done && cmd2.cmpl.done == 1 && !cmd2.skb);
    CHECK(completed.done && completed.cmpl.done == 1 && completed.skb == saved);
    CHECK(saved->len == 1 && saved->data[0] == 0x75);
    CHECK(!maintenance.done && !maintenance.cmpl.done && !maintenance.skb);
    event_frees = 0;
    direct(12, 7, 0, true);
    CHECK(error_completions == 2 && cmd2.cmpl.done == 1);
    list_del(&cmd2.list); list_del(&completed.list); list_del(&maintenance.list);
    dev_kfree_skb_any(saved);
    finish(true); review_probes++;
}

static void wrapper_schedule(struct hgic_fwctrl *c)
{
    struct sk_buff *tx = skb_dequeue(&c->txq);
    CHECK(tx != NULL);
    const u16 cookie = get_unaligned_le16(tx->data + 6);
    dev_kfree_skb_any(tx); command_sends++;
    make_info(cookie); deliver(false, true, false);
    make_event(14, 31, 12); deliver(true, !host_immediate, false);
    dispatch();
}

static void wrapper_commit_and_prior_resubmits(void)
{
    for (int immediate = 0; immediate < 2; immediate++) {
        init(immediate); hg.ctrl.schedule = wrapper_schedule;
        struct hgic_fw_info info, before;
        memset(&info, 0xa5, sizeof(info)); before = info;
        const unsigned int old_submissions = submissions;
        CHECK(hgic_fwctrl_get_fwinfo(&hg.ctrl, 1, &info) == -EPROTO);
        CHECK(!memcmp(&info, &before, sizeof(info)));
        CHECK(submissions == old_submissions + (immediate ? 1U : 2U));
        CHECK(success_completions == 1 && formats_constant == 1 && !formats_payload);
        success_completions = formats_constant = fake_logs = 0;
        finish(true); review_probes++;
    }
}

int main(void)
{
    const u16 endian = 1; CHECK(*(const u8 *)&endian == 1);
    guarded_payload(); already_failed_does_not_inspect();
    pre_read_bounds_and_latch_order(); multi_waiter_backing();
    wrapper_commit_and_prior_resubmits();
    CHECK(!live_skb && !data_calls && !cancel_calls);
    printf("PASS independent A2b supplement: %u assertions; %u ordered probes\n", checks, review_probes);
    return 0;
}
