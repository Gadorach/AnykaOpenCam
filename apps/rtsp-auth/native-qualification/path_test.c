/* SPDX-License-Identifier: GPL-2.0-only */
#include "source_abi.h"
#include "paths.h"
#include <stdio.h>
#include <unistd.h>
int main(void)
{
 unsigned checks=0;
#define CHECK(x) do{checks++;if(!(x)){printf("HOST_ONLY FAIL check=%u\n",checks);return 1;}}while(0)
 CHECK(nq_open_dir(NULL,getuid(),0700)<0);CHECK(nq_open_dir("relative",getuid(),0700)<0);
 CHECK(nq_open_dir("/",getuid(),0700)<0);CHECK(nq_open_dir("/../",getuid(),0700)<0);
 CHECK(nq_open_file(-1,"../secret",getuid(),0600,1536)<0);CHECK(nq_open_file(-1,".",getuid(),0600,1536)<0);
 printf("HOST_ONLY PASS path_ABI_checks=%u SOURCE_material_bytes=%zu source_align=8 helper_prefix_bytes=%zu\n",checks,sizeof(struct rv_material),sizeof(struct nq_public_prefix));return 0;
}
