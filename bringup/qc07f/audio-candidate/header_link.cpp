/* SPDX-License-Identifier: GPL-2.0-only */
#include "capture_core.h"
static int refuse(void *,const ac_request *){return AC_NO_EFFECT;}
int main(){ac_core core{};int context=0;ac_provider provider{AC_ABI,sizeof(ac_provider),&context,refuse};
 return ac_init(&core,&provider,1,0)==AC_OK&&ac_can_free(&core)?0:1;}
