/* SPDX-License-Identifier: GPL-2.0-only
 * Stock QC07F ABI observations from REA/Ghidra ak_motor_ioctl and timer handler.
 * NOT the public BSP's older '/dev/ak-motorN' interface. This header does not
 * actuate motors or certify the meaning/safety of calibration/movement calls.
 */
#ifndef QC07F_STOCK_MOTOR_ABI_H
#define QC07F_STOCK_MOTOR_ABI_H
#include <stdint.h>

#define QC07F_MOTOR0_PATH "/dev/motor0"
#define QC07F_MOTOR1_PATH "/dev/motor1"

/* Exact observed wire request constants. Encoded size 4 does NOT describe
 * the driver's actual 24-byte configuration or 36-byte status copy length.
 * Never allocate a status buffer from _IOC_SIZE(request).
 */
#define QC07F_MOTOR_CONFIG_WIRE       0x40046d00UL
#define QC07F_MOTOR_RATE_HZ_WIRE      0x40046d20UL
#define QC07F_MOTOR_RATE_ANGLE_WIRE   0x40046d21UL
#define QC07F_MOTOR_MOVE_LIMITED_WIRE 0x40046d40UL
#define QC07F_MOTOR_MOVE_UNBOUNDED_WIRE 0x40046d41UL
#define QC07F_MOTOR_STOP_WIRE         0x40046d42UL
#define QC07F_MOTOR_STATUS_WIRE       0x40046d43UL
#define QC07F_MOTOR_RESET_WIRE        0x40046d60UL
#define QC07F_MOTOR_CENTER_WIRE       0x40046d61UL
#define QC07F_MOTOR_SCAN_WIRE         0x40046d62UL
#define QC07F_MOTOR_END_WIRE          0x40046d63UL

struct qc07f_motor_config_wire { int32_t words[6]; };
struct qc07f_motor_status_wire { int32_t words[9]; };
struct qc07f_motor_reset_result_wire { int32_t words[2]; };
_Static_assert(sizeof(struct qc07f_motor_config_wire) == 24, "observed config copy length");
_Static_assert(sizeof(struct qc07f_motor_status_wire) == 36, "observed status copy length");
_Static_assert(sizeof(struct qc07f_motor_reset_result_wire) == 8, "observed reset result copy length");

/* Reset/homing drives beyond the configured range by 50/100 extra steps in
 * the recovered code. MOVE_UNBOUNDED bypasses normal soft limits. Do not call
 * either during initial bring-up. Exact board-version/phase-pin data must be
 * observed before replacing the driver; version 3 routes through AW9523B.
 */
#endif
