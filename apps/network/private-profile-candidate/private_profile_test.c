/* SPDX-License-Identifier: GPL-2.0-only
 * EXPLICIT PUBLICDUMMY / published NIST+RFC vectors ONLY. No operator secrets.
 * All keys/plaintext/ciphertext/nonces/tags/fixture files are SOURCE_PRIVATE
 * artifacts even here: NEVER hash/inventory/print their contents. */
#define _GNU_SOURCE
#ifndef PP_PUBLIC_DUMMY_TEST
#error "This source is not product defaults or provisioning"
#endif
#include "private_profile.h"
#include <mbedtls/platform.h>
#include <mbedtls/platform_util.h>
#include <mbedtls/hkdf.h>
#include <mbedtls/md.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <grp.h>
#include <linux/capability.h>
#include <sys/syscall.h>
static unsigned checks,cases,serial,key_reads;
static int root,key_watch=-1;
ssize_t __real_pread(int,void *,size_t,off_t);
ssize_t __wrap_pread(int fd,void *p,size_t n,off_t o)
{if(fd==key_watch)key_reads++;return __real_pread(fd,p,n,o);}
#define CHECK(x) do {checks++;if(!(x)){fprintf(stderr,"check=%u line=%d\n",checks,__LINE__);abort();}} while(0)
#define OK(x) CHECK((x)==PP_OK)
static const unsigned char dummy_key[32]={
 0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,0x30,
 0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,0x40};
static int zero(const void *p,size_t n){const unsigned char *q=p;unsigned x=0;for(size_t i=0;i<n;i++)x|=q[i];return !x;}
static void wipe(void *p,size_t n){mbedtls_platform_zeroize(p,n);}
static void p64(unsigned char *p,uint64_t v){for(unsigned i=0;i<8;i++)p[i]=(unsigned char)(v>>(8*i));}
static uint64_t g64(const unsigned char *p){uint64_t v=0;for(unsigned i=0;i<8;i++)v|=(uint64_t)p[i]<<(8*i);return v;}
static int id(void *ctx,uint64_t n){(void)ctx;return n==100||n==101?0:-1;}
struct owner {
 int fd;uint64_t floor,commit,other_commit,key_epoch,owner_epoch,other_owner_epoch;
 int deny,deny_commit,getter_fail,wrong_scope;
 unsigned point,nth,seen,calls[8],get_calls,change_get_nth,change_fault_point,change_fault_nth,change_fault_seen;
};
static int checkpoint(struct owner *o)
{
 unsigned char wire[48]={0},seen[48]={0};int r;
 p64(wire,o->key_epoch);p64(wire+8,o->floor);p64(wire+16,o->commit);p64(wire+24,o->other_commit);
 p64(wire+32,o->owner_epoch);p64(wire+40,o->other_owner_epoch);
 r=pwrite(o->fd,wire,sizeof(wire),0)==sizeof(wire)&&!fsync(o->fd)?0:-1;
 if(!r&&(pread(o->fd,seen,48,0)!=48||memcmp(wire,seen,48)))r=-1;
 wipe(wire,sizeof(wire));wipe(seen,sizeof(seen));return r;
}
static int reserve(void *ctx,uint64_t epoch,uint64_t old,uint64_t next)
{
 struct owner *o=ctx;if(o->deny||epoch!=o->key_epoch||old!=o->floor||old==UINT64_MAX||next!=old+1)return -1;
 o->floor=next;return checkpoint(o); /* actual PUBLICDUMMY durable external CAS */
}
static int commit(void *ctx,uint64_t scope,uint64_t owner_epoch,uint64_t epoch,uint64_t old,uint64_t next)
{
 struct owner *o=ctx;uint64_t *current;
 if(scope==42&&owner_epoch==o->owner_epoch)current=&o->commit;
 else if(scope==43&&owner_epoch==o->other_owner_epoch)current=&o->other_commit;
 else return -1;
 if(o->deny_commit||epoch!=o->key_epoch||old!=*current||next!=o->floor||next<=old)return -1;
 *current=next;return checkpoint(o);
}
static int authority(void *ctx,uint64_t scope,uint64_t owner_epoch,struct pp_authority *out)
{
 struct owner *o=ctx;unsigned char state[48]={0};int r=0;(void)owner_epoch;
 o->get_calls++;
 if(o->change_get_nth==o->get_calls){o->floor++;CHECK(!checkpoint(o));}
 if(o->getter_fail||pread(o->fd,state,48,0)!=48){wipe(state,48);return -1;}
 memset(out,0,sizeof(*out));
 /* Read actual current source checkpoint; no options/expected-field echo.
  * One owner object/fd identifies the shared master-key ledger across scopes. */
 if(scope==42){out->scope_id=42;out->owner_epoch=g64(state+32);out->committed_revision=g64(state+16);}
 else if(scope==43){out->scope_id=43;out->owner_epoch=g64(state+40);out->committed_revision=g64(state+24);}
 else r=-1;
 if(o->wrong_scope)out->scope_id++;
 out->key_epoch=g64(state);out->nonce_floor=g64(state+8);wipe(state,48);return r;
}
static int fault(void *ctx,uint32_t point)
{
 struct owner *o=ctx;o->calls[point]++;
 if(point==o->change_fault_point&&++o->change_fault_seen==o->change_fault_nth){o->floor++;CHECK(!checkpoint(o));}
 return point==o->point&&++o->seen==o->nth;
}
static struct pp_options options(struct owner *o)
{
 struct pp_options c={0};c.mode=PP_HOST_PUBLICDUMMY_4M;c.scope_id=42;c.owner_epoch=o->owner_epoch;c.key_epoch=o->key_epoch;
 c.trusted_nonce_floor=o->floor;c.trusted_commit_revision=o->commit;c.public_id=id;
 c.reserve=reserve;c.reserve_context=o;c.commit=commit;c.commit_context=o;c.fault=fault;c.fault_context=o;
 c.authority=authority;c.authority_context=o;return c;
}
static int fresh(struct owner *o)
{
 char name[40];int d,fd;
 memset(o,0,sizeof(*o));o->key_epoch=5;o->owner_epoch=7;o->other_owner_epoch=8;
 CHECK(snprintf(name,sizeof(name),"private-fixture-%04u",++serial)>0);
 CHECK(!mkdirat(root,name,0700));d=openat(root,name,O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);CHECK(d>=0);
 CHECK(snprintf(name,sizeof(name),"owner-checkpoint-%04u",serial)>0);
 fd=openat(root,name,O_RDWR|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);CHECK(fd>=0);o->fd=fd;
 CHECK(!checkpoint(o));cases++;return d;
}
static int key_fd(void)
{
 char name[40];int fd;
 CHECK(snprintf(name,sizeof(name),"publicdummy-key-%04u",serial)>0);
 fd=openat(root,name,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);CHECK(fd>=0);
 CHECK(write(fd,dummy_key,32)==32&&!close(fd));
 fd=openat(root,name,O_RDONLY|O_NOFOLLOW|O_CLOEXEC);CHECK(fd>=0);key_watch=fd;return fd;
}
static int reuse_key_fd(void)
{
 char name[40];int fd;CHECK(snprintf(name,sizeof(name),"publicdummy-key-%04u",serial)>0);
 fd=openat(root,name,O_RDONLY|O_NOFOLLOW|O_CLOEXEC);CHECK(fd>=0);key_watch=fd;return fd;
}
static void reopen(struct pp_session *s,struct owner *o,int d,int key)
{struct pp_options c=options(o);OK(pp_session_init(s,&c));OK(pp_import_key_fd(s,key));OK(pp_store_open(s,d));}
static void seed(struct pp_session *s,struct owner *o,int d,int key)
{
 struct pp_options c=options(o);
 {int r=pp_session_init(s,&c);if(r){
   struct rlimit core,lock;struct __user_cap_header_struct hdr={_LINUX_CAPABILITY_VERSION_3,0};
   struct __user_cap_data_struct caps[2]={{0}};unsigned empty=1;
   int cr=getrlimit(RLIMIT_CORE,&core),lr=getrlimit(RLIMIT_MEMLOCK,&lock),cap=(int)syscall(SYS_capget,&hdr,caps);
   for(unsigned i=0;i<2;i++)if(caps[i].effective||caps[i].permitted||caps[i].inheritable)empty=0;
   fprintf(stderr,"init_result=%d errno=%d uid_equal=%d nonroot=%d gid_equal=%d groups_bounded=%d dump_zero=%d nnp=%d core_zero=%d lock_fit=%d caps_empty=%d\n",
    r,errno,getuid()==geteuid(),geteuid()!=0,getgid()==getegid(),getgroups(0,NULL)<=64,
    prctl(PR_GET_DUMPABLE)==0,prctl(PR_GET_NO_NEW_PRIVS,0,0,0,0)==1,!cr&&!core.rlim_cur&&!core.rlim_max,
    !lr&&lock.rlim_cur>=sizeof(struct pp_private)&&lock.rlim_cur<=PP_HOST_LOCK_LIMIT,!cap&&empty);
  }OK(r);}
 OK(pp_import_key_fd(s,key));
 CHECK(pp_store_open(s,d)==PP_EMPTY);OK(pp_seed_ledger(s));
}
static struct pp_profile profile(void)
{
 struct pp_profile p={0};p.ssid_bytes=16;p.psk_bytes=16;p.hostname_bytes=11;p.ipv4_mode=1;
 memcpy(p.ssid,"PUBLICDUMMY-SSID!",16);memcpy(p.psk,"PUBLICDUMMY-PSK!!",16);memcpy(p.hostname,"dummycamera",11);return p;
}
static int use(void *ctx,const struct pp_profile *p)
{struct pp_profile *expected=ctx;CHECK(!memcmp(p,expected,sizeof(*p)));return 0;}
static void close_case(struct pp_session *s,struct owner *o,int d,int key)
{pp_session_close(s);CHECK(zero(&s->secret,sizeof(s->secret)));CHECK(!close(key)&&!close(d)&&!close(o->fd));key_watch=-1;}
static void vectors(void)
{
 /* NIST AES-256-GCM, zero256 key / zero96 IV / zero128 plaintext (published).
  * RFC5869 SHA256 test1 (published), NOT product key/nonce defaults. */
 const unsigned char ct[16]={0xce,0xa7,0x40,0x3d,0x4d,0x60,0x6b,0x6e,0x07,0x4e,0xc5,0xd3,0xba,0xf3,0x9d,0x18};
 const unsigned char tag[16]={0xd0,0xd1,0xc8,0xa7,0x99,0x99,0x6b,0xf0,0x26,0x5b,0x98,0xb5,0xd4,0x8a,0xb9,0x19};
 const unsigned char okm[42]={0x3c,0xb2,0x5f,0x25,0xfa,0xac,0xd5,0x7a,0x90,0x43,0x4f,0x64,0xd0,0x36,0x2f,0x2a,0x2d,0x2d,0x0a,0x90,0xcf,0x1a,0x5a,0x4c,0x5d,0xb0,0x2d,0x56,0xec,0xc4,0xc5,0xbf,0x34,0x00,0x72,0x08,0xd5,0xb8,0x87,0x18,0x58,0x65};
 mbedtls_gcm_context g;unsigned char key[32]={0},iv[12]={0},plain[16]={0},out[42]={0},t[16]={0},decoded[16];
 unsigned char ikm[22],salt[13],info[10];OK(mbedtls_platform_set_calloc_free(calloc,free));
 mbedtls_gcm_init(&g);OK(mbedtls_gcm_setkey(&g,MBEDTLS_CIPHER_ID_AES,key,256));
 OK(mbedtls_gcm_crypt_and_tag(&g,MBEDTLS_GCM_ENCRYPT,16,iv,12,NULL,0,plain,out,16,t));
 CHECK(!memcmp(out,ct,16)&&!memcmp(t,tag,16));
 OK(mbedtls_gcm_auth_decrypt(&g,16,iv,12,NULL,0,t,16,out,decoded));CHECK(zero(decoded,16));
 t[0]^=1;CHECK(mbedtls_gcm_auth_decrypt(&g,16,iv,12,NULL,0,t,16,out,decoded)!=0);mbedtls_gcm_free(&g);
 memset(ikm,0x0b,22);for(unsigned i=0;i<13;i++)salt[i]=(unsigned char)i;for(unsigned i=0;i<10;i++)info[i]=(unsigned char)(0xf0+i);
 OK(mbedtls_hkdf(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256),salt,13,ikm,22,info,10,out,42));CHECK(!memcmp(out,okm,42));
 wipe(key,32);wipe(iv,12);wipe(plain,16);wipe(out,42);wipe(t,16);wipe(decoded,16);wipe(ikm,22);wipe(salt,13);wipe(info,10);cases++;
}
static void codec(void)
{
 struct pp_profile p=profile(),got;unsigned char wire[256];OK(pp_private_encode(&p,wire));OK(pp_private_decode(wire,&got));
 CHECK(!memcmp(&p,&got,sizeof(p)));
 for(unsigned field=0;field<8;field++){
  OK(pp_private_encode(&p,wire));
  if(field==0)wire[4]=2;
  if(field==1)wire[8]=0; /* no open network */
  if(field==2)wire[16]=33;
  if(field==3)wire[20]=7;
  if(field==4)wire[28]=1;
  if(field==5)wire[52]=1;
  if(field==6)wire[224]=1;
  if(field==7)wire[223]=1; /* nonzero hostname tail */
  CHECK(pp_private_decode(wire,&got)==PP_INVALID&&zero(&got,sizeof(got)));
 }
 p.ipv4_mode=2;p.ipv4=0xc0a80102U;p.prefix=24;p.gateway=0xc0a80101U;p.dns1=0xc0a80101U;
 OK(pp_private_encode(&p,wire));p.gateway=0xc0a80201U;CHECK(pp_private_encode(&p,wire)==PP_INVALID&&zero(wire,256));
 wipe(&p,sizeof(p));wipe(&got,sizeof(got));wipe(wire,256);cases++;
}
static void guards_before_key(int hardened_now)
{
 struct owner o;struct pp_session s;struct pp_options c;int d=fresh(&o),key=key_fd();unsigned before=key_reads;
 c=options(&o);c.mode=PP_NATIVE;CHECK(pp_session_init(&s,&c)==PP_HOLD);
 CHECK(pp_import_key_fd(&s,key)==PP_HOLD&&key_reads==before&&zero(&s.secret,sizeof(s.secret)));
 if(!hardened_now){c.mode=PP_HOST_PUBLICDUMMY_4M;CHECK(pp_session_init(&s,&c)==PP_HOLD);
  CHECK(pp_import_key_fd(&s,key)==PP_HOLD&&key_reads==before);}
 close_case(&s,&o,d,key);
}
static void positive(void)
{
 struct owner o;struct pp_session s;struct pp_profile p=profile();struct pp_handle h;int d=fresh(&o),key=key_fd();
 seed(&s,&o,d,key);OK(pp_store_put(&s,0,100,&p));CHECK(o.floor==1&&o.commit==1);
 OK(pp_store_handle(&s,100,1,&h));CHECK(h.profile_id==100&&h.revision==1);
 CHECK(zero(s.secret.clear,256)&&zero(s.secret.envelope,384)&&zero(s.secret.ledger,80));
 OK(pp_store_use_private(&s,100,1,use,&p));OK(pp_store_put(&s,1,101,&p));
 {unsigned char private_wire[848];int file=openat(d,"profile.current",O_RDONLY|O_NOFOLLOW);
  CHECK(file>=0&&read(file,private_wire,848)==848&&!close(file));
  CHECK(memcmp(private_wire+128,private_wire+512,12)!=0);
  CHECK(g64(private_wire+132)==1&&g64(private_wire+516)==2); /* nonce suffixes, never printed/hashed */
  wipe(private_wire,848);}
 OK(pp_store_handle(&s,100,1,&h));OK(pp_store_handle(&s,101,2,&h));
 pp_session_close(&s);CHECK(zero(&s.secret,sizeof(s.secret)));CHECK(!close(key));key=reuse_key_fd();reopen(&s,&o,d,key);
 OK(pp_store_handle(&s,101,2,&h));CHECK(pp_store_put(&s,2,100,&p)==PP_CAPACITY);
 CHECK(zero(&s.secret,sizeof(s.secret)));wipe(&p,sizeof(p));close_case(&s,&o,d,key);
}
static void faults(void)
{
 const unsigned counts[6]={0,2,2,2,4,2};
 for(unsigned point=1;point<=5;point++)for(unsigned nth=1;nth<=counts[point];nth++){
  struct owner o;struct pp_session s;struct pp_profile p=profile();int d=fresh(&o),key=key_fd(),r;
  seed(&s,&o,d,key);memset(o.calls,0,sizeof(o.calls));o.point=point;o.nth=nth;o.seen=0;
  r=pp_store_put(&s,0,100,&p);CHECK(r!=PP_OK&&o.calls[point]>=nth&&zero(&s.secret,sizeof(s.secret)));
  CHECK(o.floor==1&&o.commit==0);o.point=0;
  {struct pp_options c=options(&o);OK(pp_session_init(&s,&c));OK(pp_import_key_fd(&s,key));r=pp_store_open(&s,d);
   CHECK(r==PP_OK||r==PP_HOLD);if(r==PP_OK)CHECK(s.current_revision==0);}
  wipe(&p,sizeof(p));close_case(&s,&o,d,key);
 }
}
static void read_cipher(int d,unsigned char *out,size_t n)
{int f=openat(d,"profile.current",O_RDONLY|O_NOFOLLOW);CHECK(f>=0&&read(f,out,n)==(ssize_t)n&&!close(f));}
static void write_cipher(int d,const unsigned char *p,size_t n)
{int f=openat(d,"profile.current",O_WRONLY|O_TRUNC|O_NOFOLLOW);CHECK(f>=0&&write(f,p,n)==(ssize_t)n&&!close(f));}
static void reauthenticate_publicdummy_bundle(struct pp_session *s,unsigned char *b)
{
 unsigned char info[48]={0},key[32];memcpy(info,"profile-ledger-v1",17);
 p64(info+24,42);p64(info+32,7);p64(info+40,5);
 OK(mbedtls_hkdf(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256),(const unsigned char *)"profile-ledger-domain-v1",24,dummy_key,32,info,48,key,32));
 OK(mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256),key,32,b,816,b+816));
 (void)s;wipe(info,48);wipe(key,32);
}
static void corruption(void)
{
 for(unsigned kind=0;kind<9;kind++){
  struct owner o;struct pp_session s;struct pp_profile p=profile();struct pp_handle h;
  unsigned char wire[848];int d=fresh(&o),key=key_fd();seed(&s,&o,d,key);OK(pp_store_put(&s,0,100,&p));read_cipher(d,wire,848);
  if(kind==0)wire[4]=2;
  if(kind==1)wire[16]=1;
  if(kind==2)wire[48+96]^=1; /* authentic outer bundle, bad actual GCM tag */
  if(kind==3)wire[48+128]^=1; /* authentic outer bundle, changed ciphertext */
  if(kind==4)wire[48+8]=2;
  if(kind==5)wire[48+24]^=1;
  if(kind==6)p64(wire+48+48,999);
  if(kind==7)wire[48+112]=1;
  if(kind<8)reauthenticate_publicdummy_bundle(&s,wire);
  write_cipher(d,wire,kind==8?847:848);
  CHECK(pp_store_handle(&s,100,1,&h)!=PP_OK&&zero(&h,sizeof(h))&&zero(&s.secret,sizeof(s.secret)));
  wipe(wire,848);wipe(&p,sizeof(p));close_case(&s,&o,d,key);
 }
}
static void replay_binding_and_foreign(void)
{
 struct owner o;struct pp_session s;struct pp_profile p=profile();struct pp_handle h;unsigned char old[848],newer[848];
 int d=fresh(&o),key=key_fd();seed(&s,&o,d,key);OK(pp_store_put(&s,0,100,&p));read_cipher(d,old,848);
 OK(pp_store_put(&s,1,101,&p));read_cipher(d,newer,848);write_cipher(d,old,848);
 CHECK(pp_store_handle(&s,100,1,&h)==PP_HOLD&&zero(&h,sizeof(h))&&zero(&s.secret,sizeof(s.secret)));
 write_cipher(d,newer,848);reopen(&s,&o,d,key);OK(pp_store_handle(&s,100,1,&h));
 CHECK(pp_store_handle(&s,101,1,&h)==PP_CONFLICT&&zero(&h,sizeof(h)));pp_session_close(&s);
 {struct pp_options c=options(&o);unsigned before=key_reads;c.owner_epoch++;OK(pp_session_init(&s,&c));
  CHECK(pp_import_key_fd(&s,key)==PP_HOLD&&key_reads==before&&zero(&s.secret,sizeof(s.secret)));}
 {int f=openat(d,"foreign-publicdummy",O_WRONLY|O_CREAT|O_EXCL,0600);CHECK(f>=0&&!close(f));}
 {struct pp_options c=options(&o);OK(pp_session_init(&s,&c));OK(pp_import_key_fd(&s,key));CHECK(pp_store_open(&s,d)==PP_HOLD);}
 wipe(old,848);wipe(newer,848);wipe(&p,sizeof(p));close_case(&s,&o,d,key);
 d=fresh(&o);key=key_fd();seed(&s,&o,d,key);o.deny=1;
 CHECK(pp_store_put(&s,0,100,&p)==PP_INVALID); /* p wiped, strict private input */
 p=profile();reopen(&s,&o,d,key);CHECK(pp_store_put(&s,0,100,&p)==PP_HOLD&&o.floor==0);
 wipe(&p,sizeof(p));close_case(&s,&o,d,key);
}
static void commit_uncertainty(void)
{
 struct owner o;struct pp_session s;struct pp_profile p=profile();int d=fresh(&o),key=key_fd();
 seed(&s,&o,d,key);o.deny_commit=1;CHECK(pp_store_put(&s,0,100,&p)==PP_HOLD);
 CHECK(o.floor==1&&o.commit==0&&zero(&s.secret,sizeof(s.secret)));
 {struct pp_options c=options(&o);OK(pp_session_init(&s,&c));OK(pp_import_key_fd(&s,key));CHECK(pp_store_open(&s,d)==PP_HOLD);}
 wipe(&p,sizeof(p));close_case(&s,&o,d,key);
}
static void readiness_lifetime(void)
{
 struct owner o;struct pp_session s;struct pp_handle h;struct pp_options c;
 struct pp_profile p=profile();int d=fresh(&o),key=key_fd();unsigned before;
 seed(&s,&o,d,key);OK(pp_store_put(&s,0,100,&p));before=key_reads;
 CHECK(!prctl(PR_SET_DUMPABLE,1));
 CHECK(pp_store_handle(&s,100,1,&h)==PP_HOLD&&zero(&h,sizeof(h))&&zero(&s.secret,sizeof(s.secret)));
 CHECK(key_reads==before&&!prctl(PR_SET_DUMPABLE,0));wipe(&p,sizeof(p));close_case(&s,&o,d,key);
 d=fresh(&o);key=key_fd();c=options(&o);
 {int status;pid_t pid=fork();CHECK(pid>=0);
  if(!pid){struct rlimit zero_limit={0,0};if(setrlimit(RLIMIT_MEMLOCK,&zero_limit))_exit(3);
   if(pp_session_init(&s,&c)!=PP_HOLD||pp_import_key_fd(&s,key)!=PP_HOLD||!zero(&s.secret,sizeof(s.secret)))_exit(4);
   _exit(0);}
  CHECK(waitpid(pid,&status,0)==pid&&WIFEXITED(status)&&WEXITSTATUS(status)==0);}
 OK(pp_session_init(&s,&c));
 {char name[40];int wrong;CHECK(snprintf(name,sizeof(name),"publicdummy-key-%04u",serial)>0);
  wrong=openat(root,name,O_RDWR|O_NOFOLLOW|O_CLOEXEC);CHECK(wrong>=0);key_watch=wrong;before=key_reads;
  CHECK(pp_import_key_fd(&s,wrong)==PP_INVALID&&key_reads==before&&zero(&s.secret,sizeof(s.secret)));
  CHECK(!close(wrong));key_watch=key;}
 close_case(&s,&o,d,key);
}
static void entropy_hold(void)
{
 struct owner o;struct pp_session s;struct pp_profile p=profile();int d=fresh(&o),key=key_fd();
 seed(&s,&o,d,key);o.point=PP_ENTROPY;o.nth=1;
 CHECK(pp_store_put(&s,0,100,&p)==PP_HOLD&&o.floor==1&&o.commit==0);
 CHECK(zero(&s.secret,sizeof(s.secret)));wipe(&p,sizeof(p));close_case(&s,&o,d,key);
}
static int count_use(void *ctx,const struct pp_profile *p)
{unsigned *n=ctx;(void)p;(*n)++;return 0;}
static void write_ledger_fixture(int d,const unsigned char *p)
{int fd=openat(d,"nonce.ledger",O_WRONLY|O_TRUNC|O_NOFOLLOW);CHECK(fd>=0&&write(fd,p,80)==80&&!close(fd));}
static void read_ledger_fixture(int d,unsigned char *out)
{int fd=openat(d,"nonce.ledger",O_RDONLY|O_NOFOLLOW);CHECK(fd>=0&&read(fd,out,80)==80&&!close(fd));}
static void live_replayed_already_open(void)
{
 for(unsigned operation=0;operation<3;operation++){
  struct owner o;struct pp_session a;struct pp_profile p=profile();struct pp_handle h;
  unsigned char prior[848],nonce[80];unsigned calls=0;int d=fresh(&o),key=key_fd(),status;pid_t pid;
  seed(&a,&o,d,key);OK(pp_store_put(&a,0,100,&p));read_cipher(d,prior,848);read_ledger_fixture(d,nonce);
  /* PUBLICDUMMY process-only fork: retire the inherited child copy, then a
   * separate session owns serialized update. Parent performs no concurrent
   * operation and keeps its already-open session/admission snapshot unchanged. */
  pid=fork();CHECK(pid>=0);
  if(!pid){struct pp_session b;pp_session_close(&a);reopen(&b,&o,d,key);
   if(pp_store_put(&b,1,101,&p)!=PP_OK)_exit(3);
   pp_session_close(&b);wipe(&p,sizeof(p));_exit(0);}
  CHECK(waitpid(pid,&status,0)==pid&&WIFEXITED(status)&&WEXITSTATUS(status)==0);
  CHECK(o.floor==1&&o.commit==1); /* parent cache deliberately NOT refreshed */
  write_cipher(d,prior,848);write_ledger_fixture(d,nonce);memset(&h,0xa5,sizeof(h));
  if(operation==0)CHECK(pp_store_handle(&a,100,1,&h)==PP_HOLD&&zero(&h,sizeof(h)));
  if(operation==1)CHECK(pp_store_use_private(&a,100,1,count_use,&calls)==PP_HOLD&&calls==0);
  if(operation==2)CHECK(pp_store_put(&a,1,101,&p)==PP_HOLD);
  CHECK(zero(&a.secret,sizeof(a.secret))&&!a.key_loaded);
  wipe(prior,848);wipe(nonce,80);wipe(&p,sizeof(p));close_case(&a,&o,d,key);
 }
}
static void live_authority_mutations(void)
{
 for(unsigned op=0;op<2;op++)for(unsigned kind=0;kind<7;kind++){
  struct owner o;struct pp_session s;struct pp_profile p=profile();struct pp_handle h;
  unsigned calls=0;int d=fresh(&o),key=key_fd();seed(&s,&o,d,key);OK(pp_store_put(&s,0,100,&p));
  if(kind==0)o.floor++;
  if(kind==1)o.floor--;
  if(kind==2)o.commit++;
  if(kind==3)o.commit--;
  if(kind==4)o.key_epoch++; /* never relabel loaded old key */
  if(kind==5)o.owner_epoch++;
  if(kind==6)o.getter_fail=1;
  CHECK(!checkpoint(&o));memset(&h,0xa5,sizeof(h));
  if(op==0)CHECK(pp_store_handle(&s,100,1,&h)==PP_HOLD&&zero(&h,sizeof(h)));
  else CHECK(pp_store_use_private(&s,100,1,count_use,&calls)==PP_HOLD&&calls==0);
  CHECK(zero(&s.secret,sizeof(s.secret)));wipe(&p,sizeof(p));close_case(&s,&o,d,key);
 }
 {struct owner o;struct pp_session s;struct pp_profile p=profile();struct pp_handle h;
  int d=fresh(&o),key=key_fd();seed(&s,&o,d,key);OK(pp_store_put(&s,0,100,&p));o.wrong_scope=1;
  CHECK(pp_store_handle(&s,100,1,&h)==PP_HOLD&&zero(&h,sizeof(h))&&zero(&s.secret,sizeof(s.secret)));
  wipe(&p,sizeof(p));close_case(&s,&o,d,key);}
}
static void live_changes_during_work(void)
{
 for(unsigned op=0;op<2;op++){
  struct owner o;struct pp_session s;struct pp_profile p=profile();struct pp_handle h;
  unsigned calls=0,last;int d=fresh(&o),key=key_fd();seed(&s,&o,d,key);OK(pp_store_put(&s,0,100,&p));
  /* Measure same implementation path's final PRE-publication check, not a
   * hardcoded internal count. Actual authority source advances on that read. */
  o.get_calls=0;
  if(op==0)OK(pp_store_handle(&s,100,1,&h));else OK(pp_store_use_private(&s,100,1,count_use,&calls));
  last=o.get_calls-(op==1?1U:0U);CHECK(last>1);o.get_calls=0;o.change_get_nth=last;calls=0;
  memset(&h,0xa5,sizeof(h));
  if(op==0)CHECK(pp_store_handle(&s,100,1,&h)==PP_HOLD&&zero(&h,sizeof(h)));
  else CHECK(pp_store_use_private(&s,100,1,count_use,&calls)==PP_HOLD&&calls==0);
  CHECK(zero(&s.secret,sizeof(s.secret)));wipe(&p,sizeof(p));close_case(&s,&o,d,key);
 }
 for(unsigned op=0;op<2;op++){
  struct owner o;struct pp_session s;struct pp_profile p=profile();struct pp_handle h;unsigned calls=0;
  int d=fresh(&o),key=key_fd();seed(&s,&o,d,key);OK(pp_store_put(&s,0,100,&p));
  o.change_fault_point=PP_AEAD;o.change_fault_nth=1;o.change_fault_seen=0;
  if(op==0)CHECK(pp_store_handle(&s,100,1,&h)==PP_HOLD&&zero(&h,sizeof(h)));
  else CHECK(pp_store_use_private(&s,100,1,count_use,&calls)==PP_HOLD&&calls==0);
  CHECK(zero(&s.secret,sizeof(s.secret)));wipe(&p,sizeof(p));close_case(&s,&o,d,key);
 }
 {struct owner o;struct pp_session s;struct pp_profile p=profile();int d=fresh(&o),key=key_fd();
  seed(&s,&o,d,key);o.change_fault_point=PP_READBACK;o.change_fault_nth=1;
  CHECK(pp_store_put(&s,0,100,&p)==PP_HOLD&&zero(&s.secret,sizeof(s.secret)));
  CHECK(o.floor==2&&o.commit==0);wipe(&p,sizeof(p));close_case(&s,&o,d,key);}
}
static void shared_master_scoped_commits(void)
{
 struct owner o;struct pp_session s;struct pp_profile p=profile();struct pp_handle h;
 unsigned char a_wire[848],b_wire[848];int a=fresh(&o),key=key_fd(),b;
 CHECK(!mkdirat(root,"shared-master-other-scope",0700));
 b=openat(root,"shared-master-other-scope",O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);CHECK(b>=0);
 seed(&s,&o,a,key);OK(pp_store_put(&s,0,100,&p));read_cipher(a,a_wire,848);pp_session_close(&s);
 {struct pp_options c=options(&o);c.scope_id=43;c.owner_epoch=8;c.trusted_commit_revision=o.other_commit;
  OK(pp_session_init(&s,&c));OK(pp_import_key_fd(&s,key));CHECK(pp_store_open(&s,b)==PP_EMPTY);OK(pp_seed_ledger(&s));
  OK(pp_store_put(&s,0,100,&p));read_cipher(b,b_wire,848);CHECK(o.floor==2&&o.commit==1&&o.other_commit==2);
  CHECK(memcmp(a_wire+128,b_wire+128,12)!=0);pp_session_close(&s);}
 /* Explicit fresh source admission uses GLOBAL2 but scoped commit1. Per-store
  * nonce ledger1 is valid history, not a demand to reset/adopt global authority. */
 reopen(&s,&o,a,key);OK(pp_store_handle(&s,100,1,&h));OK(pp_store_put(&s,1,101,&p));
 CHECK(o.floor==3&&o.commit==3&&o.other_commit==2);OK(pp_store_handle(&s,101,3,&h));
 wipe(a_wire,848);wipe(b_wire,848);wipe(&p,sizeof(p));CHECK(!close(b));close_case(&s,&o,a,key);
}
static void stricter_basic_syntax(void)
{
 static const char *const invalid[]={"-",".","a..b","-a","a-","a.-b","a-.b","a.",".a","a_b"};
 struct pp_profile p=profile();unsigned char wire[256];
 for(unsigned i=0;i<sizeof(invalid)/sizeof(invalid[0]);i++){
  memset(p.hostname,0,64);p.hostname_bytes=(uint32_t)strlen(invalid[i]);memcpy(p.hostname,invalid[i],p.hostname_bytes);
  CHECK(pp_private_encode(&p,wire)==PP_INVALID&&zero(wire,256));}
 memset(p.hostname,0,64);memcpy(p.hostname,"node-1.test",11);p.hostname_bytes=11;OK(pp_private_encode(&p,wire));
 p.ipv4_mode=2;p.ipv4=0xc6336402U;p.prefix=24;p.gateway=0;OK(pp_private_encode(&p,wire));
 for(unsigned i=0;i<3;i++){p.gateway=i==0?p.ipv4:i==1?0xc6336400U:0xc63364ffU;
  CHECK(pp_private_encode(&p,wire)==PP_INVALID&&zero(wire,256));}
 p.prefix=31;p.gateway=0xc6336403U;OK(pp_private_encode(&p,wire));
 p.gateway=p.ipv4;CHECK(pp_private_encode(&p,wire)==PP_INVALID);p.gateway=0;OK(pp_private_encode(&p,wire));
 p.prefix=32;OK(pp_private_encode(&p,wire));p.gateway=0xc6336403U;CHECK(pp_private_encode(&p,wire)==PP_INVALID);
 wipe(wire,256);wipe(&p,sizeof(p));cases++;
}
static int harden_host(void)
{
 struct rlimit core={0,0},lock;int n;gid_t groups[64];
 if(!geteuid()){
  if(fchown(root,65534,65534))return -10;
  if(setgroups(0,NULL))return -11;
  if(setgid(65534))return -12;
  if(setuid(65534))return -13;
 }
 n=getgroups(64,groups);if(n<0)return -14;
 (void)groups; /* bounded actual inherited groups; HOST model, NOT native closure */
 if(setrlimit(RLIMIT_CORE,&core))return -15;
 if(prctl(PR_SET_DUMPABLE,0))return -16;
 if(prctl(PR_SET_NO_NEW_PRIVS,1,0,0,0))return -17;
 {struct __user_cap_header_struct hdr={_LINUX_CAPABILITY_VERSION_3,0};struct __user_cap_data_struct caps[2]={{0}};
  if(syscall(SYS_capset,&hdr,caps))return -20; /* drop only, never add privileges */}
 if(getrlimit(RLIMIT_MEMLOCK,&lock))return -18;
 if(lock.rlim_cur>PP_HOST_LOCK_LIMIT)lock.rlim_cur=PP_HOST_LOCK_LIMIT;
 lock.rlim_max=lock.rlim_cur;
 return setrlimit(RLIMIT_MEMLOCK,&lock)?-19:0;
}
int main(int argc,char **argv)
{
 CHECK(argc==2&&!mkdir(argv[1],0700));root=open(argv[1],O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);CHECK(root>=0);
 vectors();codec();guards_before_key(0);
 {int hardening=harden_host();if(hardening){printf("synthetic=1 host_guard_HOLD=1 native_HOLD=1 stage=%d checks=%u\n",hardening,checks);return 2;}}
 guards_before_key(1);positive();faults();corruption();replay_binding_and_foreign();commit_uncertainty();readiness_lifetime();entropy_hold();
 live_replayed_already_open();live_authority_mutations();live_changes_during_work();shared_master_scoped_commits();stricter_basic_syntax();
 CHECK(!close(root));printf("synthetic=1 class=HOST_PUBLICDUMMY_4M native_HOLD=1 cases=%u checks=%u retained_dirs=%u passed=1\n",cases,checks,serial);
 return 0;
}
