/* SPDX-License-Identifier: GPL-2.0-only
 * PRIVATE generation SOURCE for later parent PC-only invocation. Agent executes
 * only separately compiled PUBLICDUMMY mode. No product defaults/UI key reuse. */
#define _GNU_SOURCE
#include "source_abi.h"
#include "paths.h"
#include <camera/rtsp_digest.h>
#include <camera/access_policy.h>
#include <camera/rtsp_input.h>
#include <mbedtls/platform_util.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/capability.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/random.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>
static int identifier(const char *s)
{size_t n=strlen(s);if(!n||n>=64)return 0;for(size_t i=0;i<n;i++)if(!((s[i]>='a'&&s[i]<='z')||(s[i]>='A'&&s[i]<='Z')||(s[i]>='0'&&s[i]<='9')||s[i]=='-'||s[i]=='_'||s[i]=='.'))return 0;return 1;}
static int number(const char *s,uint64_t *out)
{char *end;errno=0;if(!s[0]||s[0]=='-'||s[0]=='+')return -1;unsigned long long n=strtoull(s,&end,10);if(errno||*end||!n||n==UINT64_MAX)return -1;*out=(uint64_t)n;return 0;}
#ifndef NQ_HOST_PUBLIC_ONLY
static void hex(const unsigned char *in,size_t n,char *out)
{static const char d[]="0123456789abcdef";for(size_t i=0;i<n;i++){out[2*i]=d[in[i]>>4];out[2*i+1]=d[in[i]&15];}out[2*n]=0;}
#endif
static int write_private(int dir,const char *name,const void *data,size_t n)
{
 int fd=openat(dir,name,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);if(fd<0)return -1;
 size_t done=0;for(unsigned i=0;i<64&&done<n;i++){ssize_t k=write(fd,(const char *)data+done,n-done);if(k<0&&errno==EINTR)continue;if(k<=0){close(fd);return -1;}done+=(size_t)k;}
 int result=done==n&&!fsync(fd)&&!fchmod(fd,0600)?0:-1;close(fd);return result;
}
int main(int argc,char **argv)
{
 struct rv_material m;unsigned char seed[40]={0};char password[65]={0},realm[64]={0};int dir=-1,result=20;
 memset(&m,0,sizeof(m));
 /*Only finite PUBLIC nonsecret args: class, scope path, algorithm, known graph
  * IDs and epochs. Password/realm/HA1 never argv/env/input paths/selectors.*/
 if(argc!=12)goto done;
#ifdef NQ_HOST_PUBLIC_ONLY
 if(strcmp(argv[1],"--PUBLICDUMMY"))goto done;
#else
 if(strcmp(argv[1],"--PRIVATE-PC-ONLY"))goto done;
#endif
 if(strcmp(argv[3],"SHA-256")&&strcmp(argv[3],"MD5"))goto done;
 for(unsigned i=4;i<8;i++)if(!identifier(argv[i]))goto done;
 if(number(argv[8],&m.epoch)||number(argv[9],&m.boot_generation)||number(argv[10],&m.credential_generation)||
    strlen(argv[11])>512)goto done;
 /*One trusted fixed profile origin; track derived exactly, not arbitrary path
  * selection. Existing registry validator enforces no authority userinfo.*/
 strcpy(m.live,argv[11]);size_t live_bytes=strlen(m.live);
 if(live_bytes+7>=sizeof(m.track))goto done;
 memcpy(m.track,m.live,live_bytes);memcpy(m.track+live_bytes,"/track0",8);
 struct camera_rtsp_registry registry;
 if(camera_rtsp_registry_init(&registry,m.live,m.track))goto done;
 dir=nq_open_dir(argv[2],getuid(),0700);if(dir<0)goto done;
 m.abi=RV_ABI;m.bytes=sizeof(m);m.domain=CAMERA_ACCESS_RTSP;m.algorithm=!strcmp(argv[3],"SHA-256")?CAMERA_DIGEST_SHA256:CAMERA_DIGEST_MD5_LEGACY;
 memcpy(m.device,argv[4],strlen(argv[4]));memcpy(m.account_id,argv[5],strlen(argv[5]));memcpy(m.sensor,argv[6],strlen(argv[6]));memcpy(m.profile,argv[7],strlen(argv[7]));
 memcpy(m.username,argv[5],strlen(argv[5]));m.username_bytes=(uint32_t)strlen(argv[5]);
 m.nonce_ttl_ms=5000;m.window_ms=1000;m.global_verifies=m.account_verifies=m.peer_verifies=16;m.global_challenges=m.account_challenges=m.peer_challenges=8;
#ifdef NQ_HOST_PUBLIC_ONLY
 m.account=NQ_DUMMY_ACCOUNT;strcpy(password,"PUBLICDUMMY-PC-only-NOT-product");strcpy(realm,"PUBLICDUMMY entry qualification");
#else
 /*Native material is NEVER generated/executed by this delegated agent. Source
  * is a hardened PC-only later parent tool, not target password reader.*/
 {struct rlimit core={0,0};struct __user_cap_header_struct h={_LINUX_CAPABILITY_VERSION_3,0};struct __user_cap_data_struct caps[2]={{0}};
  if(getuid()!=geteuid()||getgid()!=getegid()||!getuid()||setrlimit(RLIMIT_CORE,&core)||prctl(PR_SET_DUMPABLE,0)||
     syscall(SYS_capset,&h,caps)||prctl(PR_SET_NO_NEW_PRIVS,1,0,0,0)||mlockall(MCL_CURRENT|MCL_FUTURE))goto done;}
 if(getrandom(seed,sizeof(seed),0)!=(ssize_t)sizeof(seed))goto done;
 m.account=NQ_PRIVATE_ACCOUNT;hex(seed+8,32,password);
 memcpy(realm,"RTSP qualification ",19);hex(seed,8,realm+19); /*per-qualification realm salt, no UID/MAC/default derivation*/
#endif
 m.realm_bytes=(uint32_t)strlen(realm);memcpy(m.realm,realm,m.realm_bytes);m.ha1_bytes=m.algorithm==CAMERA_DIGEST_SHA256?32:16;
 if(camera_rtsp_digest_ha1(m.algorithm,(struct camera_digest_span){m.username,m.username_bytes},(struct camera_digest_span){m.realm,m.realm_bytes},
    (struct camera_digest_span){(unsigned char *)password,strlen(password)},m.ha1))goto done;
 /*Password retention is PC-only. NEVER stage rtsp-password.pc onto target. No
  * file inventory/hash/dump of either file, even if material were encrypted.*/
 if(write_private(dir,"rtsp-password.pc",password,strlen(password))||write_private(dir,"rtsp-material.bin",&m,sizeof(m)))goto done;
 puts("material_source_abi_bytes=1536 password_PC_only=1 material_contents_NOTlogged=1 material_payload_hash_NOTrecorded=1");
#ifdef NQ_HOST_PUBLIC_ONLY
 puts("material_class=PUBLICDUMMY_ONLY NOT_native_qualification=1");
#else
 puts("material_class=PRIVATE_PC_ONLY positive_native_auth_NOTtested=1");
#endif
 result=0;
 done:rv_wipe(&m,sizeof(m));rv_wipe(seed,sizeof(seed));rv_wipe(password,sizeof(password));rv_wipe(realm,sizeof(realm));if(dir>=0)close(dir);return result;
}
