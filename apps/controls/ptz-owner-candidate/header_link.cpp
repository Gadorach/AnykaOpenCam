/* SPDX-License-Identifier: GPL-2.0-only */
#include "ptz_owner.h"
int main()
{
 ptz_registry registry{};
 ptz_owner owner{};
 ptz_view view{};
 if(ptz_registry_init(&registry,1)!=PTZ_OK)return 1;
 return ptz_owner_view(&owner,&view)==PTZ_INVALID?0:2;
}
