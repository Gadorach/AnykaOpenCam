/* SPDX-License-Identifier: GPL-2.0-only
 * Attach-only P2 derivative of the imported Huge-IC core by Dongyun.
 * Imported core.c and its notices remain unchanged, but are not selected.
 * No firmware loader, boot query, recovery, UP or normal traffic is implemented.
 */
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/rtnetlink.h>
#include <linux/slab.h>
#include "hgics.h"
#include "event.h"
#include "../utils/owned_control.h"

#if !defined(CONFIG_HGIC_USB) || !defined(CONFIG_HGIC_2G) || \
    defined(CONFIG_HGIC_SDIO) || defined(CONFIG_HGIC_AH) || \
    defined(CONFIG_HGIC_STABR) || defined(__RTOS__)
#error "P2 selects Linux 2G USB attach-only, not another bus/platform/profile"
#endif

static int if_test;
static int if_agg;
static int hgics_dack;
static int bt_en;
static int no_bootdl = 1;
static int txq_size = 1024;
DEFINE_SPINLOCK(attach_lock); /* private receipt + owned-request error domain */
static struct hgic_attach_receipt attach_receipt;
static struct wiphy *attach_wiphy;
static bool notifier_registered;
static bool usb_registered;

/* Caller holds attach_lock. Failure is sticky even following init success.
 * A later error does not retract an earlier completed init/copy receipt.
 */
static void attach_error_locked(int error, unsigned int phase)
{
    if (!attach_receipt.first_error) {
        attach_receipt.first_error = error < 0 ? error : -EIO;
        attach_receipt.first_phase = phase;
    }
    if (attach_receipt.init_state == HGIC_ATTACH_NOT_STARTED ||
        attach_receipt.init_state == HGIC_ATTACH_RUNNING) {
        attach_receipt.init_state = HGIC_ATTACH_FAILED;
        attach_receipt.init_done = 1;
    }
    hgic_control_error_locked(attach_receipt.first_error);
}

int hgic_attach_cause_locked(void)
{
    lockdep_assert_held(&attach_lock);
    return attach_receipt.first_error;
}

u32 hgic_attach_generation_locked(void)
{
    lockdep_assert_held(&attach_lock);
    return attach_receipt.generation;
}

void hgic_attach_fail_locked(int error, unsigned int phase)
{
    lockdep_assert_held(&attach_lock);
    attach_error_locked(error, phase);
}

void hgic_attach_error(int error, unsigned int phase)
{
    unsigned long flags;
    spin_lock_irqsave(&attach_lock, flags);
    attach_error_locked(error, phase);
    spin_unlock_irqrestore(&attach_lock, flags);
}

int hgic_attach_failed(void)
{
    unsigned long flags;
    int error;
    spin_lock_irqsave(&attach_lock, flags);
    error = attach_receipt.first_error;
    spin_unlock_irqrestore(&attach_lock, flags);
    return error;
}

void hgic_attach_snapshot(struct hgic_attach_receipt *out)
{
    unsigned long flags;
    spin_lock_irqsave(&attach_lock, flags);
    *out = attach_receipt;
    spin_unlock_irqrestore(&attach_lock, flags);
}

int hgic_attach_command_begin(void)
{
    unsigned long flags;
    int error = 0;
    spin_lock_irqsave(&attach_lock, flags);
    if (attach_receipt.first_error)
        error = attach_receipt.first_error;
    else if (attach_receipt.init_state != HGIC_ATTACH_RUNNING ||
             attach_receipt.fwinfo_commands)
        error = -EPERM;
    else
        attach_receipt.fwinfo_commands = 1;
    if (error)
        attach_error_locked(error, HGIC_ATTACH_COMMAND);
    spin_unlock_irqrestore(&attach_lock, flags);
    return error;
}

static int attach_refuse(void)
{
    hgic_attach_error(-EPERM, HGIC_ATTACH_EFFECT_REFUSED);
    return -EPERM;
}

/* These are refusals, not success stubs around disabled hardware callbacks. */
static int attach_start(struct ieee80211_hw *hw)
{
    return attach_refuse();
}

static void attach_stop(struct ieee80211_hw *hw, bool suspend)
{
    /* No successful start exists. DOWN cleanup needs no DEV_CLOSE/hw stop. */
    struct hgics_wdev *hg = hw->priv;
    clear_bit(HGICS_STATE_START, &hg->state);
    hg->radio_off = 1;
}

static int attach_start_ap(struct ieee80211_hw *hw, struct ieee80211_vif *vif,
                           struct ieee80211_bss_conf *conf)
{
    return attach_refuse();
}

static void attach_stop_ap(struct ieee80211_hw *hw, struct ieee80211_vif *vif,
                          struct ieee80211_bss_conf *conf)
{
    attach_refuse();
}

static int attach_add_interface(struct ieee80211_hw *hw, struct ieee80211_vif *vif)
{
    return attach_refuse();
}

static void attach_remove_interface(struct ieee80211_hw *hw, struct ieee80211_vif *vif)
{
    /* add_interface never succeeds: no firmware interface exists to remove. */
}

static int attach_change_interface(struct ieee80211_hw *hw, struct ieee80211_vif *vif,
                                    enum nl80211_iftype type, bool p2p)
{
    return attach_refuse();
}

static int attach_config(struct ieee80211_hw *hw, int radio_idx, u32 changed)
{
    return attach_refuse();
}

static void attach_bss_changed(struct ieee80211_hw *hw, struct ieee80211_vif *vif,
                               struct ieee80211_bss_conf *conf, u64 changed)
{
    if (changed)
        attach_refuse();
}

static void attach_vif_changed(struct ieee80211_hw *hw, struct ieee80211_vif *vif,
                               u64 changed)
{
    if (changed)
        attach_refuse();
}

static void attach_filter(struct ieee80211_hw *hw, unsigned int changed,
                           unsigned int *total, u64 multicast)
{
    *total = 0; /* local capabilities only; no firmware filter operation */
}

static int attach_sta(struct ieee80211_hw *hw, struct ieee80211_vif *vif,
                       struct ieee80211_sta *sta)
{
    return attach_refuse();
}

static int attach_rts(struct ieee80211_hw *hw, int radio_idx, u32 value)
{
    return attach_refuse();
}

static int attach_key(struct ieee80211_hw *hw, enum set_key_cmd cmd,
                       struct ieee80211_vif *vif, struct ieee80211_sta *sta,
                       struct ieee80211_key_conf *key)
{
    return attach_refuse(); /* no key allocation/copy/logging */
}

static int attach_conf_tx(struct ieee80211_hw *hw, struct ieee80211_vif *vif,
                           unsigned int link_id, u16 ac,
                           const struct ieee80211_tx_queue_params *params)
{
    return attach_refuse();
}

static int attach_ampdu(struct ieee80211_hw *hw, struct ieee80211_vif *vif,
                         struct ieee80211_ampdu_params *params)
{
    return attach_refuse();
}

static void attach_tx(struct ieee80211_hw *hw, struct ieee80211_tx_control *control,
                       struct sk_buff *skb)
{
    attach_refuse();
    ieee80211_free_txskb(hw, skb); /* actual refusal, never a synthetic TX ACK */
}

static void attach_wake_tx_queue(struct ieee80211_hw *hw, struct ieee80211_txq *txq)
{
    attach_refuse(); /* no dequeue, queue wake or data submission */
}

static int attach_add_chanctx(struct ieee80211_hw *hw,
                               struct ieee80211_chanctx_conf *ctx)
{
    return attach_refuse();
}

static void attach_remove_chanctx(struct ieee80211_hw *hw,
                                  struct ieee80211_chanctx_conf *ctx)
{
    /* No channel context was successfully added. */
}

static void attach_change_chanctx(struct ieee80211_hw *hw,
                                  struct ieee80211_chanctx_conf *ctx, u32 changed)
{
    attach_refuse();
}

static int attach_assign_vif_chanctx(struct ieee80211_hw *hw,
                                      struct ieee80211_vif *vif,
                                      struct ieee80211_bss_conf *link_conf,
                                      struct ieee80211_chanctx_conf *ctx)
{
    return attach_refuse();
}

static void attach_unassign_vif_chanctx(struct ieee80211_hw *hw,
                                        struct ieee80211_vif *vif,
                                        struct ieee80211_bss_conf *link_conf,
                                        struct ieee80211_chanctx_conf *ctx)
{
    /* Assignment never succeeds: no device/channel action exists to undo. */
}

static const struct ieee80211_ops attach_ops = {
    .tx = attach_tx,
    .start = attach_start,
    .stop = attach_stop,
    .start_ap = attach_start_ap,
    .stop_ap = attach_stop_ap,
    .add_interface = attach_add_interface,
    .remove_interface = attach_remove_interface,
    .change_interface = attach_change_interface,
    .config = attach_config,
    .link_info_changed = attach_bss_changed,
    .vif_cfg_changed = attach_vif_changed,
    .wake_tx_queue = attach_wake_tx_queue,
    .add_chanctx = attach_add_chanctx,
    .remove_chanctx = attach_remove_chanctx,
    .change_chanctx = attach_change_chanctx,
    .assign_vif_chanctx = attach_assign_vif_chanctx,
    .unassign_vif_chanctx = attach_unassign_vif_chanctx,
    .configure_filter = attach_filter,
    .sta_add = attach_sta,
    .sta_remove = attach_sta,
    .set_rts_threshold = attach_rts,
    .set_key = attach_key,
    .conf_tx = attach_conf_tx,
    .ampdu_action = attach_ampdu,
};

static void attach_tx_complete(void *priv, struct sk_buff *skb, int success)
{
    struct hgics_wdev *hg = priv;
    if (success) {
        hg->status.tx_ok++;
        /* Never clear BUS_ERROR or the first-error receipt on later success. */
    } else {
        hg->status.tx_fail++;
        set_bit(HGIC_BUS_FLAGS_ERROR, &hg->bus->flags);
        hgic_attach_error(-EIO, HGIC_ATTACH_TX_COMPLETE);
    }
    dev_kfree_skb_any(skb);
}

static int attach_tx_admit(void *priv, struct sk_buff *skb)
{
    struct hgics_wdev *hg = priv;
    if (test_bit(HGICS_STATE_REMOVE, &hg->state)) {
        hgic_attach_error(-ESHUTDOWN, HGIC_ATTACH_TX_ADMIT);
        return -ESHUTDOWN;
    }
    return hgic_fwctrl_attach_tx_admit(&hg->ctrl, skb);
}

static void attach_tx_work(struct work_struct *work)
{
    struct hgics_wdev *hg = container_of(work, struct hgics_wdev, tx_work);
    struct hgic_control_request *request;
    struct sk_buff *skb;
    /* One command only. No credit query, poll, sleep, retry or data loop. */
    request = hgic_control_take(&hg->ctrl);
    if (!request)
        return;
    skb = hgic_control_command(request);
    if (!skb || test_bit(HGICS_STATE_REMOVE, &hg->state) || hgic_attach_failed())
        hgic_control_worker_abort(request, -ESHUTDOWN);
    else
        hg->bus->tx_packet(hg->bus, skb); /* consumes worker/transport skb once */
    hgic_control_worker_done(request);
}

static void attach_schedule(struct hgic_fwctrl *ctrl)
{
    struct hgics_wdev *hg = container_of(ctrl, struct hgics_wdev, ctrl);
    /* Sole selected caller holds owner_lock across queue + work admission. */
    lockdep_assert_held(&hg->bus->owner_lock);
    if (!hg->tx_wq || hg->bus->stopping || test_bit(HGICS_STATE_REMOVE, &hg->state) || hgic_attach_failed()) {
        hgic_attach_error(-ESHUTDOWN, HGIC_ATTACH_TX_ADMIT);
        return;
    }
    queue_work(hg->tx_wq, &hg->tx_work);
}

static void attach_disabled_work(struct work_struct *work)
{
    attach_refuse(); /* detect/test cannot query, reinit, reopen or schedule init */
}

static void attach_disabled_timer(struct timer_list *timer)
{
    attach_refuse(); /* never scheduled/rearmed; no beacon or detect callback */
}

static int attach_register_hw(struct hgics_wdev *hg)
{
    int error;
    memcpy(hg->hw->wiphy->perm_addr, hg->fwinfo.mac, ETH_ALEN);
    error = hg->hghw->ops->init(hg); /* selected 2G init describes local capabilities */
    if (error)
        return error;
    hg->hw_initialized = true;
    if (hgic_attach_failed())
        return hgic_attach_failed();
    error = ieee80211_register_hw(hg->hw);
    if (error)
        return error;
    hg->hw_registered = true; /* separate from terminal init; unwind even on late failure */
    set_bit(HGICS_STATE_INITED, &hg->state);
    return 0;
}

static void attach_delay_init(struct work_struct *work)
{
    struct hgics_wdev *hg = container_of(work, struct hgics_wdev, delay_init);
    unsigned long flags;
    int error;
    spin_lock_irqsave(&attach_lock, flags);
    if (attach_receipt.first_error || !attach_receipt.init_queued ||
        attach_receipt.init_state != HGIC_ATTACH_NOT_STARTED || attach_receipt.init_starts) {
        attach_error_locked(attach_receipt.first_error ? attach_receipt.first_error : -EALREADY,
                            HGIC_ATTACH_INIT);
        spin_unlock_irqrestore(&attach_lock, flags);
        return;
    }
    attach_receipt.init_state = HGIC_ATTACH_RUNNING;
    attach_receipt.init_starts = 1;
    spin_unlock_irqrestore(&attach_lock, flags);

    /* No download/RUN/boot query, soft-credit mode, MAC fallback or restart. */
    error = hgic_fwctrl_attach_fwinfo(&hg->ctrl, &hg->fwinfo);
    if (error != 24) {
        hgic_attach_error(error < 0 ? error : -EPROTO, HGIC_ATTACH_INIT);
        return;
    }
    if (hgic_attach_failed())
        return;
    hg->ctrl.fwinfo = &hg->fwinfo;
    error = attach_register_hw(hg);
    if (error) {
        hgic_attach_error(error, HGIC_ATTACH_HW_REGISTER);
        return; /* no failed-init automatic unregister/unload/retry */
    }
    /* Netdev registration/rollback/removal notifiers are RTNL serialized.
     * Reacquire it after register_hw so removal cannot pass its unlist stage
     * while the final publication still observes an older REGISTER snapshot.
     * No netdev pointer/reference or device action is needed here.
     */
    rtnl_lock();
    spin_lock_irqsave(&attach_lock, flags);
    attach_receipt.hw_registered = 1;
    if (!attach_receipt.first_error && attach_receipt.init_state == HGIC_ATTACH_RUNNING &&
        attach_receipt.proc_registered && attach_receipt.netdev_registered == 1 &&
        attach_receipt.netdev_present && !attach_receipt.netdev_removed &&
        !attach_receipt.netdev_teardown && !test_bit(HGICS_STATE_REMOVE, &hg->state) &&
        !attach_receipt.netdev_up_seen && !test_bit(HGICS_STATE_START, &hg->state) &&
        hg->radio_off) {
        attach_receipt.init_state = HGIC_ATTACH_SUCCEEDED;
        attach_receipt.init_done = 1;
    } else if (attach_receipt.netdev_teardown || test_bit(HGICS_STATE_REMOVE, &hg->state)) {
        attach_error_locked(-ECANCELED, HGIC_ATTACH_INIT);
    } else if (!attach_receipt.netdev_present || attach_receipt.netdev_removed) {
        /* Local missing-device condition, NOT the unavailable errno from a
         * later REGISTER notifier or automatic-interface creation failure.
         */
        attach_error_locked(-ENODEV, HGIC_ATTACH_NETDEV_LOST);
    } else {
        attach_error_locked(-EPROTO, HGIC_ATTACH_INIT);
    }
    spin_unlock_irqrestore(&attach_lock, flags);
    rtnl_unlock();
}

static void attach_probe_post(void *priv)
{
    struct hgics_wdev *hg = priv;
    unsigned long flags;
    unsigned long owner_flags;
    bool schedule = false;
    spin_lock_irqsave(&hg->bus->owner_lock, owner_flags);
    if (hg->bus->stopping) {
        spin_unlock_irqrestore(&hg->bus->owner_lock, owner_flags);
        return;
    }
    spin_lock_irqsave(&attach_lock, flags);
    if (!attach_receipt.first_error && !attach_receipt.init_queued &&
        attach_receipt.init_state == HGIC_ATTACH_NOT_STARTED) {
        attach_receipt.init_queued = 1;
        schedule = true;
    } else {
        attach_error_locked(attach_receipt.first_error ? attach_receipt.first_error : -EALREADY,
                            HGIC_ATTACH_INIT);
    }
    spin_unlock_irqrestore(&attach_lock, flags);
    if (schedule && !schedule_work(&hg->delay_init))
        hgic_attach_error(-EALREADY, HGIC_ATTACH_INIT);
    spin_unlock_irqrestore(&hg->bus->owner_lock, owner_flags);
}

static void attach_owner_stop(void *priv)
{
    struct hgics_wdev *hg = priv;
    unsigned long flags;
    if (!hg)
        return;
    spin_lock_irqsave(&hg->bus->owner_lock, flags);
    hg->bus->stopping = true;
    spin_unlock_irqrestore(&hg->bus->owner_lock, flags);
    set_bit(HGICS_STATE_REMOVE, &hg->state);
    clear_bit(HGICS_STATE_START, &hg->state);
    spin_lock_irqsave(&attach_lock, flags);
    /* Identifies expected core-removal notifications only; not P4 retirement. */
    attach_receipt.netdev_teardown = 1;
    if (attach_receipt.init_state == HGIC_ATTACH_NOT_STARTED ||
        attach_receipt.init_state == HGIC_ATTACH_RUNNING)
        attach_error_locked(-ECANCELED, HGIC_ATTACH_INIT);
    spin_unlock_irqrestore(&attach_lock, flags);
    if (hg->ctrl_ready)
        hgic_fwctrl_owner_stop(&hg->ctrl);
    if (hg->primitives_ready) {
        cancel_work_sync(&hg->delay_init);
        cancel_work_sync(&hg->detect_work);
        cancel_work_sync(&hg->test_work);
        del_timer_sync(&hg->detect_tmr);
        del_timer_sync(&hg->beacon_timer);
        cancel_work_sync(&hg->tx_work);
    }
    /* Core/FWCTRL/queues remain allocated until transport poison has returned
     * for every fixed slot, including URBs already moved off used lists.
     */
}

static void attach_core_remove(void *priv)
{
    struct hgics_wdev *hg = priv;
    unsigned int i;
    if (!hg)
        return;
    if (hg->hw_registered)
        ieee80211_unregister_hw(hg->hw); /* DOWN: no successful start/interface */
    if (hg->ctrl_ready)
        hgic_fwctrl_release(&hg->ctrl);
    if (hg->tx_wq)
        destroy_workqueue(hg->tx_wq);
    if (hg->hw_initialized)
        hg->hghw->ops->free(hg);
    if (hg->primitives_ready) {
        for (i = 0; i < IEEE80211_NUM_ACS; i++)
            hgic_clear_queue(&hg->data_txq[i]);
        hgic_clear_queue(&hg->trans_q);
        hgic_clear_queue(&hg->ack_q);
        hgic_clear_queue(&hg->evt_list);
    }
    /* Published core: transport has joined producers and poisoned all slots.
     * Early probe failure: no URB has yet been submitted or published.
     */
    WRITE_ONCE(attach_wiphy, NULL);
    ieee80211_free_hw(hg->hw);
}

static int attach_core_probe(void *dev, struct hgic_bus *bus)
{
    struct ieee80211_hw *hw;
    struct hgics_wdev *hg;
    unsigned long flags;
    unsigned int i;
    int error = 0;
    spin_lock_irqsave(&attach_lock, flags);
    if (attach_receipt.first_error)
        error = attach_receipt.first_error;
    else if (attach_receipt.generation)
        error = -EALREADY; /* single owned device lifetime; no rebind/recovery */
    else
        attach_receipt.generation = 1;
    spin_unlock_irqrestore(&attach_lock, flags);
    if (error)
        goto fail_receipt;
    if (bus->type != HGIC_BUS_USB || bus->dev_id != HGIC_WLAN_8400) {
        error = -ENODEV;
        goto fail_receipt;
    }
    hw = ieee80211_alloc_hw(sizeof(*hg), &attach_ops);
    if (!hw) {
        error = -ENOMEM;
        goto fail_receipt;
    }
    hg = hw->priv;
    memset(hg, 0, sizeof(*hg));
    put_unaligned_le32(0xD8833253, hg->magic);
    hg->dev = dev;
    hg->hw = hw;
    hg->bus = bus;
    hg->dev_id = bus->dev_id;
    hg->txq_size = txq_size;
    hg->radio_off = 1;
    spin_lock_init(&hg->lock);
    skb_queue_head_init(&hg->trans_q);
    skb_queue_head_init(&hg->ack_q);
    skb_queue_head_init(&hg->evt_list);
    sema_init(&hg->evt_sema, 0);
    init_completion(&hg->txwnd_cp);
    atomic_set(&hg->txwnd, 2);
    for (i = 0; i < IEEE80211_NUM_ACS; i++)
        skb_queue_head_init(&hg->data_txq[i]);
    INIT_WORK(&hg->tx_work, attach_tx_work);
    INIT_WORK(&hg->delay_init, attach_delay_init);
    INIT_WORK(&hg->detect_work, attach_disabled_work);
    INIT_WORK(&hg->test_work, attach_disabled_work);
    timer_setup(&hg->detect_tmr, attach_disabled_timer, 0);
    timer_setup(&hg->beacon_timer, attach_disabled_timer, 0);
    hg->primitives_ready = true;
    hg->hghw = hgics_hw_match(hg->dev_id);
    if (!hg->hghw) {
        error = -ENODEV;
        goto fail_hw;
    }
    hgic_fwctrl_init(&hg->ctrl, dev, bus);
    hg->ctrl_ready = true;
    if (!hg->ctrl.wq) {
        error = -ENOMEM;
        goto fail_hw;
    }
    hg->ctrl.schedule = attach_schedule;
    hg->ctrl.rx_event = hgics_rx_fw_event;
    hg->tx_wq = alloc_ordered_workqueue("hgics_attach", 0);
    if (!hg->tx_wq) {
        error = -ENOMEM;
        goto fail_hw;
    }
    hw->extra_tx_headroom = sizeof(struct hgic_frm_hdr) + bus->drv_tx_headroom + 4;
    hw->wiphy->signal_type = CFG80211_SIGNAL_TYPE_MBM;
    hw->wiphy->addresses = hg->macaddr;
    WRITE_ONCE(attach_wiphy, hw->wiphy);
    bus->tx_complete = attach_tx_complete;
    bus->attach_tx_admit = attach_tx_admit;
    bus->rx_packet = hgics_rx_data;
    bus->rx_error = hgics_rx_error;
    bus->probe_post = attach_probe_post;
    bus->owner_stop = attach_owner_stop;
    bus->remove = attach_core_remove;
    bus->bus_priv = hg; /* publication only after initialized stages */
    return 0;
fail_hw:
    hgic_attach_error(error, HGIC_ATTACH_CORE_PROBE);
    attach_owner_stop(hg);
    attach_core_remove(hg);
fail_receipt:
    hgic_attach_error(error, HGIC_ATTACH_CORE_PROBE);
    return error;
}

static int attach_netdev_event(struct notifier_block *nb, unsigned long event, void *arg)
{
    struct net_device *dev = netdev_notifier_info_to_dev(arg);
    unsigned long flags;
    int error = 0;
    if (!dev || !dev->ieee80211_ptr ||
        dev->ieee80211_ptr->wiphy != READ_ONCE(attach_wiphy))
        return NOTIFY_DONE;
    spin_lock_irqsave(&attach_lock, flags);
    if (event == NETDEV_PRE_UP) {
        error = -EPERM; /* administratively DOWN, not merely operstate down */
        attach_error_locked(error, HGIC_ATTACH_EFFECT_REFUSED);
    } else if (event == NETDEV_REGISTER) {
        if (attach_receipt.netdev_registered || (dev->flags & IFF_UP)) {
            error = -EPROTO;
            attach_error_locked(error, HGIC_ATTACH_HW_REGISTER);
        } else {
            attach_receipt.netdev_registered = 1;
            attach_receipt.netdev_present = 1;
        }
    } else if (event == NETDEV_UNREGISTER || event == NETDEV_PRE_UNINIT) {
        /* Both rollback and ordinary removal can follow REGISTER. Keep the
         * historical milestone, but never infer surviving presence from it.
         * Repeated UNREGISTER/PRE_UNINIT notifications remain idempotent.
        */
        attach_receipt.netdev_present = 0;
        if (!attach_receipt.netdev_removed) {
            attach_receipt.netdev_removed = 1;
            attach_receipt.netdev_removal_expected = attach_receipt.netdev_teardown;
        }
        if (!attach_receipt.netdev_removal_expected) {
            /* The notifier does not expose the original registration errno.
             * Record only the observed local loss, not a guessed cause.
             * Returning NOTIFY_DONE does not obstruct the ongoing removal.
             * Later teardown cannot reclassify the first unexpected removal.
             */
            attach_error_locked(-ENODEV, HGIC_ATTACH_NETDEV_LOST);
        }
    } else if (event == NETDEV_UP) {
        attach_receipt.netdev_up_seen = 1;
        error = -EPROTO;
        attach_error_locked(error, HGIC_ATTACH_EFFECT_REFUSED);
    }
    spin_unlock_irqrestore(&attach_lock, flags);
    return error ? notifier_from_errno(error) : NOTIFY_DONE;
}

static struct notifier_block attach_netdev_notifier = {
    .notifier_call = attach_netdev_event,
};

static int __init attach_module_init(void)
{
    unsigned long flags;
    int error;
    if (if_test || if_agg || hgics_dack || bt_en || no_bootdl != 1 || txq_size != 1024) {
        hgic_attach_error(-EINVAL, HGIC_ATTACH_PARAMETERS);
        return -EINVAL; /* refuse overrides, do not silently coerce them */
    }
    error = hgics_attach_status_init();
    if (error) {
        hgic_attach_error(error, HGIC_ATTACH_PROC);
        return error;
    }
    spin_lock_irqsave(&attach_lock, flags);
    attach_receipt.proc_registered = 1;
    spin_unlock_irqrestore(&attach_lock, flags);
    error = register_netdevice_notifier(&attach_netdev_notifier);
    if (error) {
        hgic_attach_error(error, HGIC_ATTACH_NOTIFIER);
        goto fail_status;
    }
    notifier_registered = true;
    error = hgic_usb_init(attach_core_probe, 0);
    if (error) {
        hgic_attach_error(error, HGIC_ATTACH_USB_REGISTER);
        goto fail_notifier;
    }
    usb_registered = true;
    spin_lock_irqsave(&attach_lock, flags);
    attach_receipt.usb_registered = 1;
    spin_unlock_irqrestore(&attach_lock, flags);
    /* Registration/no bound probe remains NOT_STARTED, never init success. */
    return 0;
fail_notifier:
    unregister_netdevice_notifier(&attach_netdev_notifier);
    notifier_registered = false;
fail_status:
    hgics_attach_status_exit();
    return error;
}

static void __exit attach_module_exit(void)
{
    if (usb_registered)
        hgic_usb_exit();
    if (notifier_registered)
        unregister_netdevice_notifier(&attach_netdev_notifier);
    hgics_attach_status_exit();
    /* No P4 final-retirement summary or safe normal-unload claim in P2. */
}

module_init(attach_module_init);
module_exit(attach_module_exit);
module_param(if_test, int, 0);
module_param(if_agg, int, 0);
module_param(hgics_dack, int, 0);
module_param(bt_en, int, 0);
module_param(no_bootdl, int, 0);
module_param(txq_size, int, 0);
MODULE_DESCRIPTION("HUGE-IC P2 attach-only source candidate; P1/P3-P5 unmet");
MODULE_AUTHOR("Dongyun; AnykaOpenCam attach-only derivative");
MODULE_LICENSE("GPL");
