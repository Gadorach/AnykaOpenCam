/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef NQ_ENTRY_H
#define NQ_ENTRY_H
#include "verifier.h"
/*Run entry-only ops on actual child. No VERIFY/password/material read/listener.
 *0=qualified entry/lifecycle;1=bounded denial with observed retirement;
 *2=UNCERTAIN ownership retained by helper for parent recovery.*/
int nq_entry(const struct rv_image *,int material_fd);
#endif
