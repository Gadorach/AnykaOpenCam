/* SPDX-License-Identifier: GPL-2.0-only */
/* PC-only qualification provisioning. Never built/staged as ARM executable.
 * Real secrets generated ONLY when the parent consciously invokes this tool.
 * This agent runs solely the separately compiled PUBLICDUMMY host fixture. */
#define _GNU_SOURCE
#include <mbedtls/version.h>
#include <mbedtls/pkcs5.h>
#include <mbedtls/base64.h>
#include <mbedtls/platform_util.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/random.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>
#if MBEDTLS_VERSION_NUMBER != 0x03060700
#error "Provisioning requires reviewed Mbed TLS3.6.7"
#endif
#ifndef MATERIAL_PUBLIC_HOST_FIXTURE
static int random_bytes(void *out,size_t size)
{
 size_t done=0;unsigned count=0;
 while(done<size&&count++<32){
  ssize_t n=getrandom((char *)out+done,size-done,GRND_NONBLOCK);
  if(n<0&&errno==EINTR)continue;
  if(n<=0)goto bad;
  done+=(size_t)n;
 }
 if(done==size)return 0;
bad:mbedtls_platform_zeroize(out,size);return -1;
}
#endif
static void u32(unsigned char *p,unsigned n)
{p[0]=(unsigned char)n;p[1]=(unsigned char)(n>>8);p[2]=(unsigned char)(n>>16);p[3]=(unsigned char)(n>>24);}
static int store(int dir,const char *name,const void *data,size_t size)
{
 int fd=openat(dir,name,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);
 size_t done=0;unsigned attempts=0;int result=-1;
 if(fd<0)return -1;
 while(done<size&&attempts++<64){
  ssize_t n=write(fd,(const char *)data+done,size-done);
  if(n<0&&errno==EINTR)continue;
  if(n<=0)goto end;
  done+=(size_t)n;
 }
 if(done==size&&!fsync(fd))result=0;
end:if(close(fd))result=-1;return result;
}
int main(int argc,char **argv)
{
 unsigned char password[128]={0},dummy[32]={0},seed[32]={0},wire[112]={0};
 struct rlimit core={0,0};struct stat st;int dir=-1,result=20;size_t length=0;
 if(argc!=2||getuid()!=geteuid()||getgid()!=getegid()||setrlimit(RLIMIT_CORE,&core)||
    prctl(PR_SET_DUMPABLE,0)||mlockall(MCL_CURRENT|MCL_FUTURE))return 20;
 dir=open(argv[1],O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);
 if(dir<0||fstat(dir,&st)||st.st_uid!=getuid()||(st.st_mode&07777)!=0700)goto end;
#ifdef MATERIAL_PUBLIC_HOST_FIXTURE
 memcpy(password,"PUBLIC-DUMMY-NOT-A-PRODUCT-PASSWORD",33);length=33;
 memcpy(dummy,"PUBLIC-DUMMY-UNKNOWN-ACCOUNT-ONLY",32);
 memcpy(wire+8,"PUBLIC-DUMMY-001",16);memcpy(wire+56+8,"PUBLIC-DUMMY-002",16);
#else
 if(random_bytes(seed,32)||random_bytes(dummy,32)||random_bytes(wire+8,16)||random_bytes(wire+56+8,16)||
    mbedtls_base64_encode(password,sizeof(password),&length,seed,32))goto end;
#endif
 u32(wire,1);u32(wire+4,600000);u32(wire+56,1);u32(wire+60,600000);
 /* Wire first record is UNKNOWN/dummy, second record actual operator. */
 if(mbedtls_pkcs5_pbkdf2_hmac_ext(MBEDTLS_MD_SHA256,dummy,32,wire+8,16,600000,32,wire+24)||
    mbedtls_pkcs5_pbkdf2_hmac_ext(MBEDTLS_MD_SHA256,password,length,wire+64,16,600000,32,wire+80))goto end;
 if(store(dir,"operator.password",password,length)||store(dir,"credentials.bin",wire,sizeof(wire))||fsync(dir))goto end;
 result=0;
end:if(dir>=0)close(dir);
 mbedtls_platform_zeroize(password,sizeof(password));mbedtls_platform_zeroize(dummy,sizeof(dummy));
 mbedtls_platform_zeroize(seed,sizeof(seed));mbedtls_platform_zeroize(wire,sizeof(wire));
 /* No password, salt, verifier, material fingerprint or crypto match output. */
#ifdef MATERIAL_PUBLIC_HOST_FIXTURE
 printf("PC_material_generation_result=%d class=PUBLICDUMMY_HOST_ONLY work_factor=600000 files_private=1\n",result);
#else
 printf("PC_material_generation_result=%d class=PRIVATE_PARENT_ONLY work_factor=600000 files_private=1\n",result);
#endif
 return result;
}
