/* SPDX-License-Identifier: GPL-2.0-only
 * A1: selected station76 control receipt checks, not an attach controller.
 */
#ifndef HGIC_CONTROL_RECEIPT_H
#define HGIC_CONTROL_RECEIPT_H

#include <linux/errno.h>
#include <linux/unaligned.h>
#include <linux/etherdevice.h>
#include <linux/build_bug.h>

static_assert(sizeof(struct hgic_ctrl_hdr) == 12, "selected control ABI");
static_assert(offsetof(struct hgic_ctrl_hdr, cmd.status) == 10, "selected CMD status");
static_assert(offsetof(struct hgic_ctrl_hdr, cmd2.status) == 10, "selected CMD2 status");

/* rx.c normalizes these three outer u16 fields to CPU endian before fwctrl_rx.
 * The remaining command/status/payload fields are still little endian.
 * Do not reinterpret the mirror's 28-byte fw_info as a retail wire object.
 */
static inline bool hgic_ctrl_frame_valid(const u8 *data, size_t len)
{
    return data && len >= sizeof(struct hgic_ctrl_hdr) && len <= 0xffff &&
           get_unaligned((const u16 *)(data + 0)) == HGIC_HDR_RX_MAGIC &&
           get_unaligned((const u16 *)(data + 4)) == len;
}

static inline bool hgic_ctrl_retail_info_valid(const u8 *data, size_t len)
{
    u8 mac[ETH_ALEN] __aligned(2);

    if (!data || len != 24 ||
        get_unaligned_le32(data) != 0x02010001 ||
        get_unaligned_le32(data + 4) != 22118 ||
        get_unaligned_le16(data + 8) != HGIC_WLAN_8400 ||
        get_unaligned_le16(data + 10) != 1)
        return false;
    /* is_valid_ether_addr requires u16 alignment on this ARM. The wire address
     * need not have it; use an aligned local copy rather than unaligned loads.
     */
    memcpy(mac, data + 12, ETH_ALEN);
    return is_valid_ether_addr(mac);
}

/* Return parser error separately from signed firmware status. In particular a
 * legitimate negative firmware status is not an unsigned payload byte count.
 * CMD.status and CMD2.status both start at offset10 in the selected source ABI;
 * offsetof keeps that fact checked by the actual C layout, not a packed guess.
 */
static inline int hgic_ctrl_reply_status(const u8 *data, size_t len,
                                       u8 type, u8 ifidx, u16 opcode,
                                       u16 cookie, int *status)
{
    const size_t payload = sizeof(struct hgic_ctrl_hdr);
    u16 actual_opcode;
    int value;

    if (!status || !hgic_ctrl_frame_valid(data, len) ||
        (type != HGIC_HDR_TYPE_CMD && type != HGIC_HDR_TYPE_CMD2) ||
        data[2] != type || (data[3] & 0x0f) != ifidx ||
        ifidx != HGIC_WDEV_ID_STA ||
        get_unaligned((const u16 *)(data + 6)) != cookie)
        return -EPROTO;
    actual_opcode = type == HGIC_HDR_TYPE_CMD ? data[8] :
                    get_unaligned_le16(data + 8);
    if (actual_opcode != opcode)
        return -EPROTO;
    value = (s16)get_unaligned_le16(data + offsetof(struct hgic_ctrl_hdr, cmd.status));
    if (opcode == HGIC_CMD_GET_FW_INFO - HGIC_CMD_START && value >= 0 &&
        (value != 24 || !hgic_ctrl_retail_info_valid(data + payload, len - payload)))
        return -EPROTO;
    *status = value;
    return 0;
}

static inline int hgic_ctrl_reply_copy(const u8 *data, size_t len,
                                     u8 type, u8 ifidx, u16 opcode, u16 cookie,
                                     u8 *out, size_t capacity)
{
    int value, error;

    if (!out && capacity)
        return -EINVAL;
    error = hgic_ctrl_reply_status(data, len, type, ifidx, opcode, cookie, &value);
    if (error)
        return error;
    /* Setters/scalar replies may return a positive scalar without payload.
     * Only a caller requesting bytes treats positive status as a byte count.
     * Negative status is returned intact; never copy even bounded error data.
     */
    if (value > 0 && out) {
        if ((size_t)value != len - sizeof(struct hgic_ctrl_hdr) ||
            (size_t)value > capacity)
            return -EMSGSIZE;
        memcpy(out, data + sizeof(struct hgic_ctrl_hdr), value);
    }
    return value;
}

#endif
