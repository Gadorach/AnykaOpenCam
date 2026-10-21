/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef NQ_PATHS_H
#define NQ_PATHS_H
#include <stddef.h>
#include <sys/types.h>
int nq_open_dir(const char *absolute_path,uid_t owner,mode_t final_mode);
int nq_open_file(int directory,const char *fixed_name,uid_t owner,mode_t mode,size_t bytes);
#endif
