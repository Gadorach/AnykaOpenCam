/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef NH_SENSOR_BASELINE_H
#define NH_SENSOR_BASELINE_H
#include <camera/rtsp_clock.h>
#include <signal.h>
#include <stdint.h>
#define NH_SENSOR_PAIRS 139U
struct nh_sensor_report {
 unsigned attempted,guard_verified,written,read_count,mismatches,ready,failed_index;
 uint64_t begin_ns,end_ns,reset_delay_ns;
 struct {uint16_t reg;uint8_t programmed,expected,actual,valid;} rows[NH_SENSOR_PAIRS];
};
/* Private typed counterparts of the maintained monitor's STOP-separated I2C
 * helpers. Fixed address0x30; stream writes only0100=0/1. No CLI register API. */
int nh_sensor_read_byte(int fd,uint16_t reg);
int nh_sensor_stream(int fd,uint8_t on);
/* Exact fixed table only, once. Caller already holds watchdog5 + ARMED source
 * reservation. begin is the SAME producer start, not a renewed budget. */
int nh_sensor_exact139(int fd,int watchdog,uint64_t begin,
 struct camera_rtsp_clock *,const volatile sig_atomic_t *,struct nh_sensor_report *);
void nh_sensor_log(const struct nh_sensor_report *);
int nh_sensor_cli_optin(int argc,char **argv);
#endif
