/* SPDX-License-Identifier: GPL-2.0-only
 * HOST synthetic observation only. No material/identity/generation VALUES.
 * Real fork is delegated; counters are not fabricated child startup/exit proof.
 */
#ifndef IS_SYNTHETIC_SOURCE_ONLY
#error "HOST synthetic receipt only; no product/native adoption"
#endif
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
static unsigned stage,generation_match,domain_match,binding_match,attempts,launched;
void is_host_prefix_stage(unsigned s,unsigned g,unsigned d)
{stage=s;generation_match=g;domain_match=d;binding_match=1;}
pid_t __real_fork(void);
pid_t __wrap_fork(void)
{attempts++;pid_t p=__real_fork();if(p>0)launched++;return p;}
void is_host_prefix_report(void)
{printf("HOST_ONLY PREFIX stage=%u generation_match=%u domain_match=%u binding_match=%u launch_attempts=%u launched=%u\n",
 stage,generation_match,domain_match,binding_match,attempts,launched);}
