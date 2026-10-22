/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef HGIC_OWNED_CONTROL_H
#define HGIC_OWNED_CONTROL_H
#include <linux/refcount.h>
#include <linux/ktime.h>
#include <linux/skbuff.h>
#include <linux/build_bug.h>
#include "fwctrl.h"

enum hgic_cr_phase {
    HGIC_CR_NEW, HGIC_CR_QUEUED_UNSENT, HGIC_CR_WORKER_PREPARING,
    HGIC_CR_SUBMITTING, HGIC_CR_SUBMITTED, HGIC_CR_CALLBACK_RECONCILED,
    HGIC_CR_CANCELLED_UNSENT, HGIC_CR_LOCAL_FAILED, HGIC_CR_RETIRED,
};
enum hgic_cr_role {
    HGIC_CR_WAITER, HGIC_CR_REGISTRY, HGIC_CR_PENDING, HGIC_CR_QUEUE,
    HGIC_CR_WORKER, HGIC_CR_SUBMITTER, HGIC_CR_CALLBACK, HGIC_CR_ROLES,
};
struct hgic_control_request {
    refcount_t refs;
    bool role[HGIC_CR_ROLES];
    bool inconsistent, potentially_submitted, accepted, callback_seen, token_owned;
    bool reply_ready, notified;
    enum hgic_cr_phase phase;
    int error;
    u32 generation;
    u64 deadline_ns; /* computed before allocation, assigned once before publish */
    struct hgic_fwctrl *ctrl; /* usable only inside the P4a joined owner lifetime */
    struct sk_buff *command;
    struct hgic_cmd_response response;
    struct completion quarantine;
};
struct hgic_control_cb { struct hgic_control_request *request; };
static_assert(sizeof(struct hgic_control_cb) <= sizeof(((struct sk_buff *)0)->cb));
static_assert(sizeof(struct hgic_control_request) <= 1024);

/* Actual immutable first-error/receipt lock; not an exported kernel ABI. */
extern spinlock_t attach_lock;
int hgic_attach_cause_locked(void);
u32 hgic_attach_generation_locked(void);
void hgic_attach_fail_locked(int error, unsigned int phase);
void hgic_control_error_locked(int error);

struct hgic_control_request *hgic_control_create(struct hgic_fwctrl *ctrl);
struct hgic_control_request *hgic_control_from_skb(const struct sk_buff *skb);
int hgic_control_publish(struct hgic_control_request *r, struct sk_buff *skb);
struct hgic_control_request *hgic_control_take(struct hgic_fwctrl *ctrl);
struct sk_buff *hgic_control_command(struct hgic_control_request *r);
void hgic_control_worker_abort(struct hgic_control_request *r, int error);
void hgic_control_worker_done(struct hgic_control_request *r);
int hgic_control_prepare(struct hgic_control_request *r, struct sk_buff *skb);
int hgic_control_submit_claim(struct hgic_control_request *r, struct sk_buff *skb);
void hgic_control_submit_result(struct hgic_control_request *r, int error);
void hgic_control_local_failure(struct hgic_control_request *r, struct sk_buff *skb, int error);
void hgic_control_local_token_returned(struct hgic_control_request *r);
void hgic_control_callback(struct hgic_control_request *r, int error);
int hgic_control_reply_gate(struct hgic_control_request *r);
void hgic_control_response_published(struct hgic_control_request *r);
int hgic_control_check_locked(struct hgic_control_request *r);
unsigned long hgic_control_wait_budget(struct hgic_control_request *r);
void hgic_control_pending_done(struct hgic_control_request *r);
void hgic_control_waiter_done(struct hgic_control_request *r);
void hgic_control_stop_locked(struct hgic_fwctrl *ctrl);
void hgic_control_owner_release(struct hgic_fwctrl *ctrl);
#endif
