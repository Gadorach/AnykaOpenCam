/* SPDX-License-Identifier: GPL-2.0-only
 * SEPARATE HOST PUBLICDUMMY bootstrap creator, never linked into server.
 * No PRIVATE/native generation/class support, file/payload hash or stdout bytes.*/
#define _GNU_SOURCE
#ifndef IS_PUBLICDUMMY_GENERATOR
#error "This fixture creator MUST be explicitly PUBLICDUMMY only"
#endif
#include "verifier.h"
#include <camera/access_policy.h>
#include <camera/rtsp_digest.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
typedef char abi_bytes[sizeof(struct rv_material)==1536?1:-1];
int main(int argc,char **argv)
{
 struct rv_material m={0};struct stat st;int dir=-1,fd=-1,result=20;char *end;
 if(argc!=7||strcmp(argv[1],"--PUBLICDUMMY")||!getuid())goto done;
 if(!strcmp(argv[3],"SHA-256")||!strcmp(argv[3],"BAD-DOMAIN"))m.algorithm=CAMERA_DIGEST_SHA256;else if(!strcmp(argv[3],"MD5"))m.algorithm=CAMERA_DIGEST_MD5_LEGACY;else goto done;
 unsigned long port=strtoul(argv[4],&end,10);if(*end||port<1024||port>65535)goto done;
 unsigned long ttl=strtoul(argv[5],&end,10);if(*end||ttl<100||ttl>300000)goto done;
 unsigned long rate=strtoul(argv[6],&end,10);if(*end||!rate||rate>1024)goto done;
 dir=open(argv[2],O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);if(dir<0||fstat(dir,&st)||st.st_uid!=getuid()||(st.st_mode&07777)!=0700)goto done;
 m.abi=RV_ABI;m.bytes=sizeof(m);m.domain=!strcmp(argv[3],"BAD-DOMAIN")?CAMERA_ACCESS_UI:CAMERA_ACCESS_RTSP;m.epoch=444;m.boot_generation=42;m.credential_generation=1;m.account=UINT64_C(0x505542444d59);
 strcpy(m.device,"PUBLICDUMMY-device");strcpy(m.account_id,"PUBLICDUMMY-rtsp");strcpy(m.sensor,"camera0");strcpy(m.profile,"camera0-main");
 memcpy(m.username,"PUBLICDUMMY",11);m.username_bytes=11;
 const char *realm="PUBLICDUMMY RTSP candidate",*password="PUBLICDUMMY-not-product";
 m.realm_bytes=(uint32_t)strlen(realm);memcpy(m.realm,realm,m.realm_bytes);m.ha1_bytes=m.algorithm==CAMERA_DIGEST_SHA256?32:16;
 if(camera_rtsp_digest_ha1(m.algorithm,(struct camera_digest_span){m.username,m.username_bytes},(struct camera_digest_span){m.realm,m.realm_bytes},
    (struct camera_digest_span){(const unsigned char *)password,strlen(password)},m.ha1))goto done;
 snprintf(m.live,sizeof(m.live),"rtsp://127.0.0.1:%lu/live",port);snprintf(m.track,sizeof(m.track),"rtsp://127.0.0.1:%lu/live/track0",port);
 m.nonce_ttl_ms=(uint32_t)ttl;m.window_ms=1000;
 m.global_verifies=m.account_verifies=m.peer_verifies=(uint32_t)rate;m.global_challenges=m.account_challenges=m.peer_challenges=(uint32_t)rate;
 fd=openat(dir,"rtsp-material.bin",O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);if(fd<0||write(fd,&m,sizeof(m))!=(ssize_t)sizeof(m)||fsync(fd))goto done;
 result=0;
 done:rv_wipe(&m,sizeof(m));if(fd>=0)close(fd);if(dir>=0)close(dir);return result;
}
