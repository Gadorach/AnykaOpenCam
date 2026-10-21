/* SPDX-License-Identifier: GPL-2.0-only */
#include "web_service.h"
int web_media_admit(struct camera_access_policy *p,
 const struct camera_access_principal *s,const struct camera_access_request *q,
 uint64_t now,int healthy,int qualified,web_media_effect effect,void *context)
{
 if(!healthy||!s||!q||q->kind!=CAMERA_ACCESS_PROFILE||
    (q->operation!=CAMERA_ACCESS_LIVE&&q->operation!=CAMERA_ACCESS_SNAPSHOT)||
    camera_access_check(p,s,q,now)!=CAMERA_ACCESS_OK)return 401;
 if(!qualified||!effect)return 501;
 return effect(context);
}
