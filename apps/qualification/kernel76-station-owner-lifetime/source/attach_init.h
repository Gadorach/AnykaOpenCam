/* SPDX-License-Identifier: GPL-2.0-only
 * P2 local attach-only lifecycle. No protocol or platform ABI is defined here.
 */
#ifndef HGIC_ATTACH_INIT_H
#define HGIC_ATTACH_INIT_H

enum hgic_attach_init_state {
    HGIC_ATTACH_NOT_STARTED,
    HGIC_ATTACH_RUNNING,
    HGIC_ATTACH_SUCCEEDED,
    HGIC_ATTACH_FAILED,
};

enum hgic_attach_phase {
    HGIC_ATTACH_PARAMETERS = 1,
    HGIC_ATTACH_PROC,
    HGIC_ATTACH_NOTIFIER,
    HGIC_ATTACH_USB_REGISTER,
    HGIC_ATTACH_PROBE,
    HGIC_ATTACH_CORE_PROBE,
    HGIC_ATTACH_INIT,
    HGIC_ATTACH_COMMAND,
    HGIC_ATTACH_COPY,
    HGIC_ATTACH_RX,
    HGIC_ATTACH_RX_SUBMIT,
    HGIC_ATTACH_RX_COMPLETE,
    HGIC_ATTACH_TX_ADMIT,
    HGIC_ATTACH_TX_SUBMIT,
    HGIC_ATTACH_TX_COMPLETE,
    HGIC_ATTACH_HW_REGISTER,
    HGIC_ATTACH_EFFECT_REFUSED,
    /* Local absence observation; original REGISTER-chain errno is unknown. */
    HGIC_ATTACH_NETDEV_LOST,
};

/* Fixed module-owned cached scalars; no hg/netdev/request/URB pointer escapes.
 * This is NOT the P4 drain/refcount/retirement receipt or a progress feed API.
 */
struct hgic_attach_receipt {
    unsigned int generation;
    unsigned int init_state;
    unsigned int init_queued;
    unsigned int init_starts;
    unsigned int init_done;
    unsigned int fwinfo_commands;
    unsigned int hw_registered;
    unsigned int netdev_registered;
    unsigned int netdev_up_seen;
    unsigned int proc_registered;
    unsigned int usb_registered;
    int first_error;
    unsigned int first_phase;
    /* Schema2 additions; netdev_registered above stays historical. No pointer
     * or held netdev reference: these are coherent lifecycle observations.
     */
    unsigned int netdev_present;
    unsigned int netdev_removed;
    unsigned int netdev_teardown;
    unsigned int netdev_removal_expected;
};

void hgic_attach_error(int error, unsigned int phase);
int hgic_attach_failed(void);
int hgic_attach_command_begin(void);
void hgic_attach_snapshot(struct hgic_attach_receipt *out);

#endif
