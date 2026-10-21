/* SPDX-License-Identifier: GPL-2.0-only
 * Link-time fence for platform CLI parsing fixtures. A call to open exits
 * before libc or any host device access. This is not a hardware simulator.
 */
#include <stdlib.h>
int __wrap_open(const char *path,int flags,...)
{(void)path;(void)flags;_Exit(99);}
