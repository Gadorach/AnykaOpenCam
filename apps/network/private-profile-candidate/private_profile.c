/* SPDX-License-Identifier: GPL-2.0-only */
#define _GNU_SOURCE
#include "private_profile.h"
#include <mbedtls/version.h>
#include <mbedtls/hkdf.h>
#include <mbedtls/md.h>
#include <mbedtls/platform.h>
#include <mbedtls/platform_util.h>
#include <mbedtls/constant_time.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/capability.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>
#if MBEDTLS_VERSION_NUMBER != 0x03060700
#error "Requalify before changing pinned Mbed TLS 3.6.7"
#endif
_Static_assert(PP_BUNDLE_BYTES<4096,"profile store bound");
static struct pp_session *pool_owner;
static uint32_t g32(const unsigned char *p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static uint64_t g64(const unsigned char *p){return g32(p)|((uint64_t)g32(p+4)<<32);}
static void p32(unsigned char *p,uint32_t x){for(unsigned i=0;i<4;i++)p[i]=(unsigned char)(x>>(8*i));}
static void p64(unsigned char *p,uint64_t x){p32(p,(uint32_t)x);p32(p+4,(uint32_t)(x>>32));}
static int nonzero(const unsigned char *p,size_t n){unsigned v=0;for(size_t i=0;i<n;i++)v|=p[i];return v!=0;}
static void wipe(void *p,size_t n){mbedtls_platform_zeroize(p,n);}
static void *secure_calloc(size_t n,size_t z)
{
 if(!pool_owner||!n||!z||n>512/z)return NULL;
 for(unsigned i=0;i<8;i++)if(!pool_owner->secret.used[i]){
  pool_owner->secret.used[i]=1;wipe(pool_owner->secret.cells[i].bytes,512);return pool_owner->secret.cells[i].bytes;
 }
 return NULL;
}
static void secure_free(void *p)
{
 if(!p||!pool_owner)return;
 for(unsigned i=0;i<8;i++)if(p==pool_owner->secret.cells[i].bytes){wipe(p,512);pool_owner->secret.used[i]=0;return;}
}
static int hardened(void)
{
 struct rlimit core,lock;struct __user_cap_header_struct h={_LINUX_CAPABILITY_VERSION_3,0};
 struct __user_cap_data_struct caps[2]={{0}};
 gid_t groups[64];int n=getgroups(64,groups);
 if(!geteuid()||getuid()!=geteuid()||getgid()!=getegid()||n<0||
    prctl(PR_GET_DUMPABLE)!=0||prctl(PR_GET_NO_NEW_PRIVS,0,0,0,0)!=1||
    getrlimit(RLIMIT_CORE,&core)||core.rlim_cur||core.rlim_max||
    getrlimit(RLIMIT_MEMLOCK,&lock)||lock.rlim_cur<sizeof(struct pp_private)||lock.rlim_cur>PP_HOST_LOCK_LIMIT||
    syscall(SYS_capget,&h,caps))return 0;
 for(unsigned i=0;i<2;i++)if(caps[i].effective||caps[i].permitted||caps[i].inheritable)return 0;
 /* HOST_PUBLICDUMMY only: bounded actual group set is queried, not claimed
  * empty. Unprivileged host cannot drop inherited groups. Native remains HOLD. */
 (void)groups;
 return 1;
}
void pp_session_close(struct pp_session *s)
{
 if(!s)return;
 if(s->key_loaded)mbedtls_gcm_free(&s->secret.gcm);
 wipe(&s->secret,sizeof(s->secret));
 if(s->locked)(void)syscall(SYS_munlock,&s->secret,sizeof(s->secret));
 if(pool_owner==s)pool_owner=NULL;
 s->locked=0;s->key_loaded=0;s->opened=0;s->initialized=0;s->uncertain=1;
}
static int fail(struct pp_session *s,int r){pp_session_close(s);return r;}
static int ready(struct pp_session *s)
{return s&&s->initialized&&s->key_loaded&&s->locked&&pool_owner==s&&hardened();}
static int authority_at(struct pp_session *s,uint64_t nonce,uint64_t commit)
{
 struct pp_authority a={0};int r;
 if(!s||!s->options.authority)return PP_HOLD;
 r=s->options.authority(s->options.authority_context,s->options.scope_id,s->options.owner_epoch,&a);
 if(r||a.scope_id!=s->options.scope_id||a.owner_epoch!=s->options.owner_epoch||
    a.key_epoch!=s->options.key_epoch||a.nonce_floor!=nonce||a.committed_revision!=commit)r=PP_HOLD;
 wipe(&a,sizeof(a));return r?PP_HOLD:PP_OK;
}
static int authority_now(struct pp_session *s)
{return authority_at(s,s->options.trusted_nonce_floor,s->options.trusted_commit_revision);}
int pp_session_init(struct pp_session *s,const struct pp_options *o)
{
 if(!s||!o)return PP_INVALID;
 memset(s,0,sizeof(*s));s->dirfd=-1;
 /* Compile-time AND runtime separation; no native fake readiness/unseal. */
 if(o->mode==PP_NATIVE)return PP_HOLD;
#ifndef PP_PUBLIC_DUMMY_BUILD
 return PP_HOLD;
#else
 if(o->mode!=PP_HOST_PUBLICDUMMY_4M||!o->scope_id||!o->owner_epoch||!o->key_epoch||
    !o->public_id||!o->reserve||!o->commit||!o->authority||pool_owner||!hardened())return PP_HOLD;
 s->options=*o;
 /* Direct kernel call: sanitizer libc mlock interceptors are not evidence. */
 if(syscall(SYS_mlock,&s->secret,sizeof(s->secret)))return fail(s,PP_HOLD);
 s->locked=1;pool_owner=s;
 if(mbedtls_platform_set_calloc_free(secure_calloc,secure_free))return fail(s,PP_HOLD);
 s->initialized=1;s->nonce_floor=o->trusted_nonce_floor;return PP_OK;
#endif
}
int pp_import_key_fd(struct pp_session *s,int fd)
{
 struct stat st;int flags;
 if(!s||!s->initialized||s->options.mode!=PP_HOST_PUBLICDUMMY_4M||!s->locked||pool_owner!=s||!hardened())
  return s?fail(s,PP_HOLD):PP_HOLD;
 if(authority_now(s))return fail(s,PP_HOLD);
 flags=fcntl(fd,F_GETFL);
 if(s->key_loaded||fd<0||flags<0||(flags&O_ACCMODE)!=O_RDONLY||fstat(fd,&st)||
    !S_ISREG(st.st_mode)||st.st_uid!=geteuid()||(st.st_mode&07777)!=0600||st.st_nlink!=1||st.st_size!=32)
  return fail(s,PP_INVALID);
 if(pread(fd,s->secret.master,32,0)!=32)return fail(s,PP_IO);
 if(authority_now(s)||!hardened())return fail(s,PP_HOLD);
 s->key_loaded=1;mbedtls_gcm_init(&s->secret.gcm);return PP_OK;
}
static int unicast(uint32_t ip){unsigned a=ip>>24;return ip&&a&&a!=127&&a<224&&ip!=UINT32_MAX;}
static int hostname_valid(const unsigned char *p,unsigned n)
{
 unsigned label=0;if(!n||n>63)return 0;
 for(unsigned i=0;i<n;i++){
  unsigned c=p[i];
  if(c=='.'){if(!label||p[i-1]=='-')return 0;label=0;continue;}
  if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-')||(!label&&c=='-'))return 0;
  if(++label>63)return 0;
 }
 return label&&p[n-1]!='-';
}
static int profile_valid(const struct pp_profile *p)
{
 if(!p||!p->ssid_bytes||p->ssid_bytes>32||p->psk_bytes<8||p->psk_bytes>63||
    !p->hostname_bytes||p->hostname_bytes>63||p->ipv6_mode||
    (p->ipv4_mode!=1&&p->ipv4_mode!=2)||!hostname_valid(p->hostname,p->hostname_bytes))return 0;
 for(unsigned i=0;i<64;i++){
  if(i<p->psk_bytes){if(p->psk[i]<32||p->psk[i]>126)return 0;}else if(p->psk[i])return 0;
  if(i<p->hostname_bytes){unsigned c=p->hostname[i];if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'||c=='.'))return 0;}
  else if(p->hostname[i])return 0;
 }
 for(unsigned i=p->ssid_bytes;i<32;i++)if(p->ssid[i])return 0;
 if(p->ipv4_mode==1)return !(p->ipv4||p->prefix||p->gateway||p->dns1||p->dns2);
 if(!unicast(p->ipv4)||!p->prefix||p->prefix>32||(p->gateway&&!unicast(p->gateway))||
    (p->dns1&&!unicast(p->dns1))||(p->dns2&&!unicast(p->dns2)))return 0;
 {uint32_t mask=p->prefix==32?UINT32_MAX:UINT32_MAX<<(32-p->prefix);
  if(p->gateway&&((p->gateway&mask)!=(p->ipv4&mask)||p->gateway==p->ipv4))return 0;
  if(p->prefix<31&&((p->ipv4&~mask)==0||(p->ipv4&~mask)==~mask||
     (p->gateway&&((p->gateway&~mask)==0||(p->gateway&~mask)==~mask))))return 0;
  /* /31 permits zero gateway or the other endpoint; /32 permits zero only. */
  if(p->prefix==32&&p->gateway)return 0;}
 return 1;
}
int pp_private_encode(const struct pp_profile *p,unsigned char out[256])
{
 if(!out)return PP_INVALID;
 wipe(out,256);
 if(!profile_valid(p))return PP_INVALID;
 memcpy(out,"PPC1",4);p32(out+4,1);p32(out+8,1);p32(out+12,p->ipv4_mode);
 p32(out+16,p->ssid_bytes);p32(out+20,p->psk_bytes);p32(out+24,p->hostname_bytes);
 p32(out+28,p->ipv6_mode);p32(out+32,p->ipv4);p32(out+36,p->prefix);p32(out+40,p->gateway);
 p32(out+44,p->dns1);p32(out+48,p->dns2);memcpy(out+64,p->ssid,32);
 memcpy(out+96,p->psk,64);memcpy(out+160,p->hostname,64);return PP_OK;
}
int pp_private_decode(const unsigned char in[256],struct pp_profile *p)
{
 if(!p||!in)return PP_INVALID;
 wipe(p,sizeof(*p));
 if(memcmp(in,"PPC1",4)||g32(in+4)!=1||g32(in+8)!=1)return PP_INVALID;
 for(unsigned i=52;i<64;i++)if(in[i])return PP_INVALID;
 for(unsigned i=224;i<256;i++)if(in[i])return PP_INVALID;
 p->ipv4_mode=g32(in+12);p->ssid_bytes=g32(in+16);p->psk_bytes=g32(in+20);p->hostname_bytes=g32(in+24);
 p->ipv6_mode=g32(in+28);p->ipv4=g32(in+32);p->prefix=g32(in+36);p->gateway=g32(in+40);
 p->dns1=g32(in+44);p->dns2=g32(in+48);memcpy(p->ssid,in+64,32);memcpy(p->psk,in+96,64);memcpy(p->hostname,in+160,64);
 if(!profile_valid(p)){wipe(p,sizeof(*p));return PP_INVALID;}return PP_OK;
}
static int derive(struct pp_session *s,const unsigned char *salt,size_t salt_n,const unsigned char *info,size_t n,unsigned char *out)
{return mbedtls_hkdf(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256),salt,salt_n,s->secret.master,32,info,n,out,32);}
static int ledger_key(struct pp_session *s)
{
 wipe(s->secret.info,96);memcpy(s->secret.info,"profile-ledger-v1",17);
 p64(s->secret.info+24,s->options.scope_id);p64(s->secret.info+32,s->options.owner_epoch);p64(s->secret.info+40,s->options.key_epoch);
 return derive(s,(const unsigned char *)"profile-ledger-domain-v1",24,s->secret.info,48,s->secret.ledger_key);
}
static int mac(struct pp_session *s,const unsigned char *p,size_t n,unsigned char *out)
{return mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256),s->secret.ledger_key,32,p,n,out);}
static int binding(struct pp_session *s,const unsigned char *e,uint64_t id,uint64_t rev)
{
 if(memcmp(e,"PPR1",4)||g32(e+4)!=1||g32(e+8)!=1||g32(e+12)!=128||g32(e+16)!=256||g32(e+20)!=256||
    g64(e+24)!=s->options.scope_id||g64(e+32)!=s->options.owner_epoch||g64(e+40)!=s->options.key_epoch||
     g64(e+48)!=id||g64(e+56)!=rev||!rev||rev>s->store_nonce_revision||
    !id||s->options.public_id(s->options.id_context,id))return 0;
 if(g32(e+92)!=PP_HOST_PUBLICDUMMY_4M)return 0;
 for(unsigned i=112;i<128;i++)if(e[i])return 0;
 return nonzero(e+64,16)&&nonzero(e+80,4)&&g64(e+84)==rev;
}
static int record_key(struct pp_session *s,const unsigned char *e)
{
 memcpy(s->secret.aad,e,128);wipe(s->secret.aad+96,16);
 wipe(s->secret.info,96);memcpy(s->secret.info,"private-profile-v1",18);
 memcpy(s->secret.info+24,e,64);
 return derive(s,e+64,16,s->secret.info,88,s->secret.derived)||
  mbedtls_gcm_setkey(&s->secret.gcm,MBEDTLS_CIPHER_ID_AES,s->secret.derived,256);
}
static int decrypt(struct pp_session *s,const unsigned char *e,uint64_t id,uint64_t rev)
{
 if(authority_now(s))return PP_HOLD;
 if(!binding(s,e,id,rev)||record_key(s,e))return PP_AUTH;
 if(authority_now(s))return PP_HOLD;
 if(mbedtls_gcm_auth_decrypt(&s->secret.gcm,256,e+80,12,s->secret.aad,128,e+96,16,e+128,s->secret.clear))return PP_AUTH;
 if(s->options.fault&&s->options.fault(s->options.fault_context,PP_AEAD))return PP_HOLD;
 if(authority_now(s))return PP_HOLD;
 return pp_private_decode(s->secret.clear,&s->secret.profile);
}
static void work_wipe(struct pp_session *s)
{
 mbedtls_gcm_free(&s->secret.gcm);mbedtls_gcm_init(&s->secret.gcm);
 wipe(s->secret.derived,32);wipe(s->secret.ledger_key,32);wipe(s->secret.clear,256);
 wipe(s->secret.envelope,384);wipe(s->secret.aad,128);wipe(s->secret.info,96);
 wipe(s->secret.bundle,848);wipe(s->secret.readback,848);wipe(s->secret.ledger,80);
 wipe(&s->secret.profile,sizeof(s->secret.profile));
 for(unsigned i=0;i<8;i++){wipe(s->secret.cells[i].bytes,512);s->secret.used[i]=0;}
}
static int dir_safe(struct pp_session *s)
{
 struct stat st;DIR *d;struct dirent *e;int fd,ok=1;
 if(fstat(s->dirfd,&st)||!S_ISDIR(st.st_mode)||st.st_uid!=geteuid()||(st.st_mode&07777)!=0700)return 0;
 fd=openat(s->dirfd,".",O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);if(fd<0)return 0;
 d=fdopendir(fd);if(!d){close(fd);return 0;}errno=0;
 while((e=readdir(d))){
  if(!strcmp(e->d_name,".")||!strcmp(e->d_name,".."))continue;
  if(strcmp(e->d_name,"profile.current")&&strcmp(e->d_name,"nonce.ledger")){ok=0;break;}
 }
 if(!e&&errno)ok=0;
 if(closedir(d))ok=0;
 return ok;
}
static int read_file(struct pp_session *s,const char *name,unsigned char *out,size_t n)
{
 struct stat st;int fd;ssize_t got;
 if(authority_now(s))return PP_HOLD;
 if(fstatat(s->dirfd,name,&st,AT_SYMLINK_NOFOLLOW))return errno==ENOENT?PP_EMPTY:PP_IO;
 if(!S_ISREG(st.st_mode)||st.st_uid!=geteuid()||(st.st_mode&07777)!=0600||st.st_nlink!=1||st.st_size!=(off_t)n)return PP_INVALID;
 fd=openat(s->dirfd,name,O_RDONLY|O_NOFOLLOW|O_CLOEXEC|O_NONBLOCK);if(fd<0)return PP_IO;
 if(fstat(fd,&st)||!S_ISREG(st.st_mode)||st.st_uid!=geteuid()||(st.st_mode&07777)!=0600||st.st_nlink!=1||st.st_size!=(off_t)n){close(fd);return PP_INVALID;}
 got=read(fd,out,n);if(close(fd)||got!=(ssize_t)n)return PP_IO;
 return authority_now(s);
}
static int fault(struct pp_session *s,uint32_t point)
{return s->options.fault&&s->options.fault(s->options.fault_context,point);}
static int replace(struct pp_session *s,const char *pending,const char *current,const unsigned char *data,size_t n)
{
 int fd,err=0;if(authority_now(s))return PP_HOLD;
 fd=openat(s->dirfd,pending,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);
 if(fd<0)return PP_UNCERTAIN;
 if(fault(s,PP_WRITE)||write(fd,data,n)!=(ssize_t)n)err=1;
 if(!err&&(fault(s,PP_FILE_SYNC)||fsync(fd)))err=1;
 if(close(fd))err=1;
 if(err||fault(s,PP_DIR_SYNC)||fsync(s->dirfd)||fault(s,PP_RENAME)||
    renameat(s->dirfd,pending,s->dirfd,current)||fault(s,PP_DIR_SYNC)||fsync(s->dirfd))return PP_UNCERTAIN;
 if(fault(s,PP_READBACK))return PP_UNCERTAIN;
 if(authority_now(s))return PP_HOLD;
 if(read_file(s,current,s->secret.readback,n)||mbedtls_ct_memcmp(data,s->secret.readback,n))return PP_UNCERTAIN;
 wipe(s->secret.readback,sizeof(s->secret.readback));return authority_now(s);
}
static int read_ledger(struct pp_session *s)
{
 int r=read_file(s,"nonce.ledger",s->secret.ledger,80);if(r)return r;
 if(memcmp(s->secret.ledger,"PPL1",4)||g32(s->secret.ledger+4)!=1||g64(s->secret.ledger+8)!=s->options.scope_id||
    g64(s->secret.ledger+16)!=s->options.owner_epoch||g64(s->secret.ledger+24)!=s->options.key_epoch||
    g64(s->secret.ledger+32)>s->options.trusted_nonce_floor||
    g64(s->secret.ledger+32)<s->options.trusted_commit_revision)return PP_AUTH;
 for(unsigned i=40;i<48;i++)if(s->secret.ledger[i])return PP_AUTH;
 if(ledger_key(s)||mac(s,s->secret.ledger,48,s->secret.derived)||
    mbedtls_ct_memcmp(s->secret.derived,s->secret.ledger+48,32))return PP_AUTH;
 s->store_nonce_revision=g64(s->secret.ledger+32);
 /* Per-store last reservation can lag GLOBAL authority after other scopes
  * sharing this master key reserve. Never use disk values as live authority. */
 s->nonce_floor=s->options.trusted_nonce_floor;return authority_now(s);
}
static int write_ledger(struct pp_session *s,uint64_t floor)
{
 wipe(s->secret.ledger,80);memcpy(s->secret.ledger,"PPL1",4);p32(s->secret.ledger+4,1);
 p64(s->secret.ledger+8,s->options.scope_id);p64(s->secret.ledger+16,s->options.owner_epoch);
 p64(s->secret.ledger+24,s->options.key_epoch);p64(s->secret.ledger+32,floor);
 if(ledger_key(s)||mac(s,s->secret.ledger,48,s->secret.ledger+48))return PP_AUTH;
 return replace(s,"nonce.pending","nonce.ledger",s->secret.ledger,80);
}
static int bundle(struct pp_session *s)
{
 int r=read_file(s,"profile.current",s->secret.bundle,848);uint64_t last=0;if(r)return r;
 if(memcmp(s->secret.bundle,"PPB1",4)||g32(s->secret.bundle+4)!=1||
    !(s->count=g32(s->secret.bundle+8))||s->count>2||g32(s->secret.bundle+12)!=384)return PP_AUTH;
 for(unsigned i=16;i<48;i++)if(s->secret.bundle[i])return PP_AUTH;
 if(ledger_key(s)||mac(s,s->secret.bundle,816,s->secret.derived)||
    mbedtls_ct_memcmp(s->secret.derived,s->secret.bundle+816,32))return PP_AUTH;
 for(unsigned i=0;i<2;i++){
  const unsigned char *e=s->secret.bundle+48+i*384;
  if(i>=s->count){if(nonzero(e,384))return PP_AUTH;continue;}
  if(g64(e+56)<=last)return PP_AUTH;
  r=decrypt(s,e,g64(e+48),g64(e+56));if(r)return r;
  last=g64(e+56);
 }
 s->current_revision=last;return authority_now(s);
}
int pp_store_open(struct pp_session *s,int fd)
{
 int r;if(!ready(s))return s?fail(s,PP_HOLD):PP_HOLD;
 if(authority_now(s))return fail(s,PP_HOLD);
 s->dirfd=fd;
 if(!dir_safe(s))return fail(s,PP_HOLD);
 r=read_ledger(s);
 if(r==PP_EMPTY){
  r=read_file(s,"profile.current",s->secret.bundle,848);
  if(r!=PP_EMPTY)return fail(s,PP_HOLD);
   if(authority_now(s))return fail(s,PP_HOLD);
   work_wipe(s);return PP_EMPTY;
 }
 if(r)return fail(s,PP_HOLD);
 r=bundle(s);if(r==PP_EMPTY){s->count=0;s->current_revision=0;r=PP_OK;}
 if(r)return fail(s,r);
 if(s->current_revision!=s->options.trusted_commit_revision)return fail(s,PP_HOLD);
 if(authority_now(s))return fail(s,PP_HOLD);
 s->opened=1;work_wipe(s);return PP_OK;
}
int pp_seed_ledger(struct pp_session *s)
{
 int r;if(!ready(s)||s->opened||s->dirfd<0||s->options.trusted_commit_revision)return s?fail(s,PP_HOLD):PP_HOLD;
 if(authority_now(s)||!dir_safe(s))return fail(s,PP_HOLD);
 r=read_ledger(s);if(r!=PP_EMPTY)return fail(s,PP_HOLD);
 r=write_ledger(s,s->options.trusted_nonce_floor);if(r)return fail(s,r);
 s->nonce_floor=s->options.trusted_nonce_floor;s->store_nonce_revision=s->nonce_floor;
 if(authority_now(s))return fail(s,PP_HOLD);
 s->opened=1;work_wipe(s);return PP_OK;
}
int pp_store_put(struct pp_session *s,uint64_t expected,uint64_t id,const struct pp_profile *p)
{
 int r;uint64_t next;unsigned char *e;
 if(!ready(s)||!s->opened)return s?fail(s,PP_HOLD):PP_HOLD;
 if(authority_now(s)||!dir_safe(s))return fail(s,PP_HOLD);
 if(!id||s->options.public_id(s->options.id_context,id)||!profile_valid(p))return fail(s,PP_INVALID);
 if(authority_now(s))return fail(s,PP_HOLD);
 r=read_ledger(s);if(r)return fail(s,r);
 r=bundle(s);if(r==PP_EMPTY){s->count=0;s->current_revision=0;wipe(s->secret.bundle,848);}
 else if(r)return fail(s,r);
 if(expected!=s->current_revision)return fail(s,PP_CONFLICT);
 if(s->current_revision!=s->options.trusted_commit_revision)return fail(s,PP_HOLD);
 if(s->count>=2)return fail(s,PP_CAPACITY);
 for(unsigned i=0;i<s->count;i++)if(g64(s->secret.bundle+48+i*384+48)==id)return fail(s,PP_CONFLICT);
 if(s->nonce_floor==UINT64_MAX)return fail(s,PP_EXHAUSTED);
 next=s->nonce_floor+1;
 if(s->options.reserve(s->options.reserve_context,s->options.key_epoch,s->nonce_floor,next))return fail(s,PP_HOLD);
 if(authority_at(s,next,s->options.trusted_commit_revision))return fail(s,PP_HOLD);
 /* Advance only after this confirmed OWN CAS, never by copying getter/disk
  * state to repair a mismatch or refresh stale key/epoch expectations. */
 s->nonce_floor=next;s->options.trusted_nonce_floor=next;
 r=write_ledger(s,next);if(r)return fail(s,r);s->store_nonce_revision=next;
 e=s->secret.envelope;wipe(e,384);memcpy(e,"PPR1",4);p32(e+4,1);p32(e+8,1);p32(e+12,128);p32(e+16,256);p32(e+20,256);
 p64(e+24,s->options.scope_id);p64(e+32,s->options.owner_epoch);p64(e+40,s->options.key_epoch);p64(e+48,id);p64(e+56,next);
 if(fault(s,PP_ENTROPY)||syscall(SYS_getrandom,e+64,20,0)!=20||!nonzero(e+64,16)||!nonzero(e+80,4))return fail(s,PP_HOLD);
 p64(e+84,next);
 p32(e+92,PP_HOST_PUBLICDUMMY_4M);
 if(pp_private_encode(p,s->secret.clear)||record_key(s,e)||
    mbedtls_gcm_crypt_and_tag(&s->secret.gcm,MBEDTLS_GCM_ENCRYPT,256,e+80,12,s->secret.aad,128,s->secret.clear,e+128,16,e+96))return fail(s,PP_AUTH);
 if(fault(s,PP_AEAD)||authority_now(s))return fail(s,PP_HOLD);
 memcpy(s->secret.bundle+48+s->count*384,e,384);s->count++;
 memcpy(s->secret.bundle,"PPB1",4);p32(s->secret.bundle+4,1);p32(s->secret.bundle+8,s->count);p32(s->secret.bundle+12,384);
 if(ledger_key(s)||mac(s,s->secret.bundle,816,s->secret.bundle+816))return fail(s,PP_AUTH);
 r=replace(s,"profile.pending","profile.current",s->secret.bundle,848);if(r)return fail(s,r);
 if(s->options.commit(s->options.commit_context,s->options.scope_id,s->options.owner_epoch,
                      s->options.key_epoch,s->current_revision,next))return fail(s,PP_HOLD);
 if(authority_at(s,next,next))return fail(s,PP_HOLD);
 s->current_revision=next;s->options.trusted_commit_revision=next;
 if(authority_now(s))return fail(s,PP_HOLD);
 work_wipe(s);return PP_OK;
}
static int select_profile(struct pp_session *s,uint64_t id,uint64_t rev)
{
 int r;if(!ready(s)||!s->opened)return PP_HOLD;
 if(authority_now(s)||!dir_safe(s))return PP_HOLD;
 r=read_ledger(s);if(r)return r;r=bundle(s);if(r)return r;
 if(s->current_revision!=s->options.trusted_commit_revision)return PP_HOLD;
 for(unsigned i=0;i<s->count;i++){
  const unsigned char *e=s->secret.bundle+48+i*384;
  if(g64(e+48)==id&&g64(e+56)==rev)return decrypt(s,e,id,rev);
 }
 return PP_CONFLICT;
}
int pp_store_handle(struct pp_session *s,uint64_t id,uint64_t rev,struct pp_handle *out)
{
 int r;if(!out)return s?fail(s,PP_INVALID):PP_INVALID;memset(out,0,sizeof(*out));
 r=select_profile(s,id,rev);if(r)return s?fail(s,r):r;
 if(!ready(s)||authority_now(s))return fail(s,PP_HOLD);
 out->scope_id=s->options.scope_id;out->owner_epoch=s->options.owner_epoch;out->key_epoch=s->options.key_epoch;
 out->profile_id=id;out->revision=rev;work_wipe(s);return PP_OK;
}
int pp_store_use_private(struct pp_session *s,uint64_t id,uint64_t rev,pp_private_use_fn fn,void *ctx)
{
 int r;if(!fn)return s?fail(s,PP_INVALID):PP_INVALID;r=select_profile(s,id,rev);if(r)return s?fail(s,r):r;
 if(!ready(s)||authority_now(s))return fail(s,PP_HOLD);
 r=fn(ctx,&s->secret.profile);if(r||authority_now(s))return fail(s,PP_HOLD);work_wipe(s);return PP_OK;
}
