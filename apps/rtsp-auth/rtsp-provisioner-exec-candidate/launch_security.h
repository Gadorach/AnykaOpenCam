/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef QC07F_RTSP_EXEC_LAUNCH_SECURITY_H
#define QC07F_RTSP_EXEC_LAUNCH_SECURITY_H
#include "public_codec.h"
#include <sys/types.h>
/* Caller-supplied independently reviewed WORKER policy, not verifier rv_image.
 * No path/default/role flag establishes provenance; descriptor is borrowed. */
struct pe_worker_image { int fd;uint32_t bytes;unsigned char sha256[32]; };
int pe_root_ids(void);
int pe_clock(uint64_t *);
int pe_tick(uint64_t *,uint64_t);
int pe_same_inode(const struct stat *,const struct stat *);
int pe_directory(int,struct stat *);
int pe_ingress(int,struct stat *);
int pe_control(int,pid_t,struct stat *);
int pe_image_validate(const struct pe_worker_image *,struct stat *);
int pe_image_unchanged(const struct pe_worker_image *,const struct stat *);
int pe_close_once(int *);
/* Store ALL installed rights before checking packet length/control. Returns
 * 1 pending, 0 packet (possibly bad), -1 receive error. No FD is closed here. */
int pe_receive(int,unsigned char *,size_t,ssize_t *,int [PE_RIGHTS_MAX],
 unsigned *,unsigned *);
int pe_security_verify(void);
_Noreturn void pe_child_exec(const int [4],pid_t);
#endif
