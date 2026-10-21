/* SPDX-License-Identifier: GPL-2.0-only */
#include "reset_input.h"
int main()
{
 reset_input engine{};
 ri_provider provider{RESET_INPUT_ABI,sizeof(ri_provider),RI_INPUT_ABSENT,RI_PUBLICDUMMY_INPUT_ONLY,nullptr,nullptr};
 ri_config config{20000000,5000000000ULL,15000000000ULL,100000000,1000000000};
 ri_view view{};
 if(reset_input_init(&engine,&provider,&config,1,1,0)!=RI_OK)return 1;
 if(reset_input_poll(&engine,0)!=RI_UNAVAILABLE)return 2;
 if(reset_input_view(&engine,&view)!=RI_OK||view.native_qualified)return 3;
 return reset_input_dispose(&engine)==RI_OK?0:4;
}
