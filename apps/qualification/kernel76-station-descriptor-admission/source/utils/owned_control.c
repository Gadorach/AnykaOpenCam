/* SPDX-License-Identifier: GPL-2.0-only
 * One FWINFO request, actual sole P4a TX producer and owner-return proofs only.
 */
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/jiffies.h>
#include <linux/netdevice.h>
#include "../hgic_def.h"
#include "owned_control.h"

static struct hgic_control_request *active_request; /* registry ref, attach_lock */

static void notify_locked(struct hgic_control_request *r)
{
    lockdep_assert_held(&attach_lock);
    if (!r->notified && (r->error || (r->reply_ready && r->accepted))) {
        r->notified = true;
        complete(&r->response.cmpl);
    }
}

static void role_get(struct hgic_control_request *r, enum hgic_cr_role role)
{
    lockdep_assert_held(&attach_lock);
    if (r->role[role]) {
        r->inconsistent = true;
        return;
    }
    r->role[role] = true;
    refcount_inc(&r->refs);
}

static void role_put(struct hgic_control_request *r, enum hgic_cr_role role)
{
    lockdep_assert_held(&attach_lock);
    if (!r->role[role]) {
        r->inconsistent = true;
        return;
    }
    r->role[role] = false;
    /* Registry is deliberately retained until P4a joins/poison have returned. */
    if (refcount_dec_and_test(&r->refs))
        r->inconsistent = true;
}

static void cancel_locked(struct hgic_control_request *r, int error)
{
    lockdep_assert_held(&attach_lock);
    if (!r->error)
        r->error = error;
    if (r->phase == HGIC_CR_QUEUED_UNSENT) {
        struct sk_buff *skb = r->command;
        r->command = NULL;
        r->phase = HGIC_CR_CANCELLED_UNSENT;
        if (!skb)
            r->inconsistent = true;
        role_put(r, HGIC_CR_QUEUE);
        /* Only the exclusively owned queued UNSENT object; never worker/USB. */
        dev_kfree_skb_any(skb);
    }
    notify_locked(r);
}

void hgic_control_error_locked(int error)
{
    lockdep_assert_held(&attach_lock);
    if (active_request)
        cancel_locked(active_request, error);
}

static int gate_locked(struct hgic_control_request *r)
{
    int error;
    lockdep_assert_held(&attach_lock);
    error = hgic_attach_cause_locked();
    if (!error && r->error)
        error = r->error;
    if (!error && (r != active_request || r->generation != hgic_attach_generation_locked()))
        error = -ESHUTDOWN;
    if (!error && r->inconsistent)
        error = -EUCLEAN;
    if (!error && ktime_get_ns() >= r->deadline_ns)
        error = -ETIMEDOUT;
    if (error)
        hgic_attach_fail_locked(error, HGIC_ATTACH_COMMAND);
    return error;
}

struct hgic_control_request *hgic_control_create(struct hgic_fwctrl *ctrl)
{
    u64 deadline = ctrl->attach_deadline_ns;
    struct hgic_control_request *r;
    unsigned long flags;
    int error;
    if (!deadline) {
        hgic_attach_error(-EINVAL, HGIC_ATTACH_COMMAND);
        return ERR_PTR(-EINVAL);
    }
    r = kzalloc(sizeof(*r), GFP_KERNEL);
    if (!r) {
        hgic_attach_error(-ENOMEM, HGIC_ATTACH_COMMAND);
        return ERR_PTR(-ENOMEM);
    }
    refcount_set(&r->refs, 1);
    r->role[HGIC_CR_WAITER] = true;
    r->ctrl = ctrl;
    r->deadline_ns = deadline;
    init_completion(&r->response.cmpl);
    init_completion(&r->quarantine);
    INIT_LIST_HEAD(&r->response.list);
    spin_lock_irqsave(&ctrl->bus->owner_lock, flags);
    spin_lock(&attach_lock);
    error = hgic_attach_cause_locked();
    if (!error && (ctrl->bus->stopping || active_request || ctrl->owned))
        error = -ESHUTDOWN;
    if (!error && ktime_get_ns() >= deadline)
        error = -ETIMEDOUT;
    if (error) {
        hgic_attach_fail_locked(error, HGIC_ATTACH_COMMAND);
    } else {
        r->generation = hgic_attach_generation_locked();
        role_get(r, HGIC_CR_REGISTRY);
        active_request = r;
        ctrl->owned = r;
    }
    spin_unlock(&attach_lock);
    spin_unlock_irqrestore(&ctrl->bus->owner_lock, flags);
    if (error) {
        kfree(r); /* never published; constructor's sole ref */
        return ERR_PTR(error);
    }
    return r;
}

struct hgic_control_request *hgic_control_from_skb(const struct sk_buff *skb)
{
    struct hgic_control_cb cb;
    struct hgic_control_request *r;
    unsigned long flags;
    memcpy(&cb, skb->cb, sizeof(cb));
    spin_lock_irqsave(&attach_lock, flags);
    r = cb.request == active_request ? active_request : NULL;
    spin_unlock_irqrestore(&attach_lock, flags);
    return r; /* caller already holds worker or actual-callback role */
}

int hgic_control_publish(struct hgic_control_request *r, struct sk_buff *skb)
{
    struct hgic_control_cb cb = { .request = r };
    int error;
    lockdep_assert_held(&r->ctrl->bus->owner_lock);
    lockdep_assert_held(&r->ctrl->lock);
    spin_lock(&attach_lock);
    error = gate_locked(r);
    if (!error && (r->ctrl->bus->stopping || r->phase != HGIC_CR_NEW || r->command))
        error = -ESHUTDOWN;
    if (error) {
        hgic_attach_fail_locked(error, HGIC_ATTACH_COMMAND);
    } else {
        memcpy(skb->cb, &cb, sizeof(cb));
        role_get(r, HGIC_CR_PENDING);
        list_add(&r->response.list, &r->ctrl->pd_list);
        role_get(r, HGIC_CR_QUEUE);
        r->command = skb;
        r->phase = HGIC_CR_QUEUED_UNSENT;
    }
    spin_unlock(&attach_lock);
    return error;
}

struct hgic_control_request *hgic_control_take(struct hgic_fwctrl *ctrl)
{
    struct hgic_control_request *r;
    unsigned long flags;
    spin_lock_irqsave(&ctrl->bus->owner_lock, flags);
    spin_lock(&attach_lock);
    r = ctrl->owned;
    if (!r || ctrl->bus->stopping || gate_locked(r) || r->phase != HGIC_CR_QUEUED_UNSENT) {
        if (r && ctrl->bus->stopping)
            cancel_locked(r, -ESHUTDOWN);
        r = NULL;
    } else {
        /* Queue -> worker is one ref transfer, no unowned intermediate. */
        r->role[HGIC_CR_QUEUE] = false;
        r->role[HGIC_CR_WORKER] = true;
        r->phase = HGIC_CR_WORKER_PREPARING;
    }
    spin_unlock(&attach_lock);
    spin_unlock_irqrestore(&ctrl->bus->owner_lock, flags);
    return r;
}

struct sk_buff *hgic_control_command(struct hgic_control_request *r)
{
    unsigned long flags;
    struct sk_buff *skb;
    spin_lock_irqsave(&attach_lock, flags);
    skb = r->command; /* exclusively worker-owned; error hook cannot free it */
    spin_unlock_irqrestore(&attach_lock, flags);
    return skb;
}

void hgic_control_worker_abort(struct hgic_control_request *r, int error)
{
    unsigned long flags;
    struct sk_buff *skb;
    spin_lock_irqsave(&attach_lock, flags);
    hgic_attach_fail_locked(error, HGIC_ATTACH_COMMAND);
    skb = r->command;
    r->command = NULL;
    r->phase = HGIC_CR_LOCAL_FAILED;
    spin_unlock_irqrestore(&attach_lock, flags);
    dev_kfree_skb_any(skb);
}

void hgic_control_worker_done(struct hgic_control_request *r)
{
    unsigned long flags;
    spin_lock_irqsave(&attach_lock, flags);
    if (r->command || r->role[HGIC_CR_SUBMITTER])
        r->inconsistent = true;
    role_put(r, HGIC_CR_WORKER);
    spin_unlock_irqrestore(&attach_lock, flags);
}

int hgic_control_prepare(struct hgic_control_request *r, struct sk_buff *skb)
{
    unsigned long flags;
    int error;
    spin_lock_irqsave(&attach_lock, flags);
    error = gate_locked(r);
    if (!error && (r->phase != HGIC_CR_WORKER_PREPARING || r->command != skb ||
                   !r->role[HGIC_CR_WORKER])) {
        error = -EPERM;
        hgic_attach_fail_locked(error, HGIC_ATTACH_COMMAND);
    }
    spin_unlock_irqrestore(&attach_lock, flags);
    return error;
}

int hgic_control_submit_claim(struct hgic_control_request *r, struct sk_buff *skb)
{
    int error;
    lockdep_assert_held(&r->ctrl->bus->owner_lock);
    spin_lock(&attach_lock);
    error = gate_locked(r);
    if (!error && (r->ctrl->bus->stopping || r->phase != HGIC_CR_WORKER_PREPARING ||
                   r->command != skb || !r->role[HGIC_CR_WORKER]))
        error = -ESHUTDOWN;
    if (error) {
        hgic_attach_fail_locked(error, HGIC_ATTACH_COMMAND);
    } else {
        role_get(r, HGIC_CR_SUBMITTER);
        role_get(r, HGIC_CR_CALLBACK);
        r->potentially_submitted = true;
        r->token_owned = true;
        r->command = NULL; /* transport/local-failure owner, not error hook */
        r->phase = HGIC_CR_SUBMITTING;
    }
    spin_unlock(&attach_lock);
    return error;
}

void hgic_control_submit_result(struct hgic_control_request *r, int error)
{
    unsigned long flags;
    spin_lock_irqsave(&attach_lock, flags);
    if (error) {
        hgic_attach_fail_locked(error, HGIC_ATTACH_TX_SUBMIT);
        if (r->callback_seen)
            r->inconsistent = true; /* kernel contract forbids error+callback */
        role_put(r, HGIC_CR_CALLBACK); /* no callback; local owner reconciles */
        r->phase = HGIC_CR_LOCAL_FAILED;
    } else {
        gate_locked(r); /* accepted return may be late; never repairs failure */
        r->accepted = true;
        r->phase = r->callback_seen ? HGIC_CR_CALLBACK_RECONCILED : HGIC_CR_SUBMITTED;
        notify_locked(r);
    }
    role_put(r, HGIC_CR_SUBMITTER);
    spin_unlock_irqrestore(&attach_lock, flags);
}

void hgic_control_local_failure(struct hgic_control_request *r, struct sk_buff *skb, int error)
{
    unsigned long flags;
    spin_lock_irqsave(&attach_lock, flags);
    hgic_attach_fail_locked(error, HGIC_ATTACH_TX_SUBMIT);
    if (r->phase == HGIC_CR_WORKER_PREPARING && r->command == skb) {
        r->command = NULL;
        r->phase = HGIC_CR_LOCAL_FAILED;
    } else if (r->phase != HGIC_CR_LOCAL_FAILED) {
        r->inconsistent = true;
    }
    spin_unlock_irqrestore(&attach_lock, flags);
}

void hgic_control_local_token_returned(struct hgic_control_request *r)
{
    unsigned long flags;
    spin_lock_irqsave(&attach_lock, flags);
    r->token_owned = false;
    spin_unlock_irqrestore(&attach_lock, flags);
}

void hgic_control_callback(struct hgic_control_request *r, int error)
{
    unsigned long flags;
    spin_lock_irqsave(&attach_lock, flags);
    if (error)
        hgic_attach_fail_locked(error, HGIC_ATTACH_TX_COMPLETE);
    if (r->callback_seen || !r->role[HGIC_CR_CALLBACK] || !r->token_owned)
        r->inconsistent = true;
    r->callback_seen = true;
    r->token_owned = false; /* real callback has already returned the token */
    if (!r->role[HGIC_CR_SUBMITTER])
        r->phase = HGIC_CR_CALLBACK_RECONCILED;
    role_put(r, HGIC_CR_CALLBACK);
    spin_unlock_irqrestore(&attach_lock, flags);
}

int hgic_control_reply_gate(struct hgic_control_request *r)
{
    unsigned long flags;
    int error;
    spin_lock_irqsave(&attach_lock, flags);
    error = gate_locked(r);
    if (!error && !r->potentially_submitted) {
        error = -EPROTO;
        hgic_attach_fail_locked(error, HGIC_ATTACH_RX);
    }
    spin_unlock_irqrestore(&attach_lock, flags);
    return error;
}

int hgic_control_check_locked(struct hgic_control_request *r)
{
    int error = gate_locked(r);
    if (!error && (!r->accepted || !r->reply_ready)) {
        error = -EPROTO;
        hgic_attach_fail_locked(error, HGIC_ATTACH_COMMAND);
    }
    return error;
}

void hgic_control_response_published(struct hgic_control_request *r)
{
    lockdep_assert_held(&r->ctrl->lock);
    spin_lock(&attach_lock);
    r->reply_ready = true;
    notify_locked(r); /* positive reply alone cannot prove submit was accepted */
    spin_unlock(&attach_lock);
}

unsigned long hgic_control_wait_budget(struct hgic_control_request *r)
{
    unsigned long flags, ticks = 0;
    u64 now;
    spin_lock_irqsave(&attach_lock, flags);
    if (!gate_locked(r) && !r->notified) {
        now = ktime_get_ns();
        if (now < r->deadline_ns)
            ticks = nsecs_to_jiffies64(r->deadline_ns - now);
        if (!ticks)
            hgic_attach_fail_locked(-ETIMEDOUT, HGIC_ATTACH_COMMAND);
    }
    spin_unlock_irqrestore(&attach_lock, flags);
    return ticks;
}

void hgic_control_pending_done(struct hgic_control_request *r)
{
    lockdep_assert_held(&r->ctrl->lock);
    spin_lock(&attach_lock);
    list_del_init(&r->response.list);
    role_put(r, HGIC_CR_PENDING);
    spin_unlock(&attach_lock);
}

void hgic_control_waiter_done(struct hgic_control_request *r)
{
    unsigned long flags;
    spin_lock_irqsave(&attach_lock, flags);
    role_put(r, HGIC_CR_WAITER);
    spin_unlock_irqrestore(&attach_lock, flags);
}

void hgic_control_stop_locked(struct hgic_fwctrl *ctrl)
{
    lockdep_assert_held(&ctrl->lock);
    spin_lock(&attach_lock);
    if (ctrl->owned && (ctrl->owned->role[HGIC_CR_PENDING] || ctrl->owned->role[HGIC_CR_QUEUE]))
        cancel_locked(ctrl->owned, hgic_attach_cause_locked() ?: -ESHUTDOWN);
    spin_unlock(&attach_lock);
}

void hgic_control_owner_release(struct hgic_fwctrl *ctrl)
{
    struct hgic_control_request *r;
    unsigned long flags;
    unsigned int i, roles = 0;
    bool bad = false;
    /* Only P4a final release, after full producer join AND all-slot poison. */
    spin_lock_irqsave(&ctrl->lock, flags);
    spin_lock(&attach_lock);
    r = ctrl->owned;
    if (r) {
        for (i = 0; i < HGIC_CR_ROLES; i++)
            roles += r->role[i];
        bad = r != active_request || r->inconsistent || roles != 1 ||
              !r->role[HGIC_CR_REGISTRY] || refcount_read(&r->refs) != 1 ||
              r->command || r->response.skb || r->token_owned ||
              (r->accepted && !r->callback_seen) || !list_empty(&ctrl->pd_list);
        if (!bad) {
            r->phase = HGIC_CR_RETIRED;
            ctrl->owned = NULL;
            active_request = NULL;
            r->role[HGIC_CR_REGISTRY] = false;
            if (!refcount_dec_and_test(&r->refs))
                bad = true;
        }
    }
    spin_unlock(&attach_lock);
    spin_unlock_irqrestore(&ctrl->lock, flags);
    if (bad) {
        hgic_attach_error(-EUCLEAN, HGIC_ATTACH_COMMAND);
        for (;;)
            wait_for_completion(&r->quarantine); /* do not return/free owners */
    }
    kfree(r); /* NULL or sole registry ref reconciled after actual return proofs */
}
