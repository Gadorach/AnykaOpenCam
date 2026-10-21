/* SPDX-License-Identifier: GPL-2.0-only
 * Public synthetic fixtures only. Retains ALL disposable directories/records.
 * argv[1] must be a NEW directory beneath the authorized analysis workspace.
 */
#define _POSIX_C_SOURCE 200809L
#include <camera/settings_store.h>
#include <camera/dma_integrity.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

static unsigned checks,serial;
static int root;
#ifdef CAMERA_SETTINGS_WRAP_UID
static unsigned uid_fault_nth,uid_calls;
uid_t __real_geteuid(void);
uid_t __wrap_geteuid(void)
{
 uid_t real=__real_geteuid();
 if(uid_fault_nth&&++uid_calls==uid_fault_nth)return real==0?1:0;
 return real;
}
#endif
#define CHECK(x) do {checks++;if(!(x)){fprintf(stderr,"check %u line %d: %s\n",checks,__LINE__,#x);abort();}} while(0)
#define OK(x) CHECK((x)==CAMERA_SETTINGS_OK)
struct injection {unsigned point,nth,seen,counts[6],events[64],event_count;int fired,crash;};
static int inject(void *ctx,uint32_t point)
{
 struct injection *i=ctx;i->counts[point]++;
 if(i->event_count<64)i->events[i->event_count++]=point;
 if(point==i->point&&++i->seen==i->nth){
  i->fired=1;if(i->crash)_exit(77);return 1;
 }
 return 0;
}
/* Fixture schema 1: 512 or 1024 bytes with public marker, bounded integer byte,
 * and zero tail; no strings/commands/credential contents accepted. Marker 9
 * stands for a synthetic forbidden field, NOT an actual credential. */
static int public_validate(void *ctx,uint32_t schema,uint32_t kind,
 const unsigned char *p,uint32_t n)
{
 (void)ctx;
 if(schema!=1)return CAMERA_SETTINGS_UNSUPPORTED;
 if(kind==CAMERA_SETTINGS_RESET_REQUEST)return n?CAMERA_SETTINGS_INVALID:CAMERA_SETTINGS_OK;
 if(n!=512&&n!=1024)return CAMERA_SETTINGS_INVALID;
 if(p[0]==9)return CAMERA_SETTINGS_SENSITIVE;
 if(p[0]==8)return CAMERA_SETTINGS_UNSUPPORTED; /* unsupported capability */
 if(p[0]!=1||p[1]>20)return CAMERA_SETTINGS_INVALID;
 for(unsigned j=2;j<n;j++)if(p[j])return CAMERA_SETTINGS_INVALID;
 return CAMERA_SETTINGS_OK;
}
static int new_dir(const char *label)
{
 char name[100];int fd;
 CHECK(snprintf(name,sizeof(name),"%03u-%s",++serial,label)>0);
 CHECK(!mkdirat(root,name,0700));
 fd=openat(root,name,O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);CHECK(fd>=0);return fd;
}
static struct camera_settings_store store(int fd,struct injection *i)
{
 struct camera_settings_store s;
 OK(camera_settings_init(&s,fd,public_validate,NULL,i?inject:NULL,i));return s;
}
static int put(struct camera_settings_store *s,uint64_t rev,uint64_t op,unsigned value,
 struct camera_settings_report *r)
{
 unsigned char p[512]={1};p[1]=(unsigned char)value;
 return camera_settings_commit(s,rev,op,1,CAMERA_SETTINGS_VALUES,p,sizeof(p),r);
}
static void seed(struct camera_settings_store *s)
{struct camera_settings_report r;OK(put(s,0,1,3,&r));CHECK(r.durability==CAMERA_SETTINGS_SYNC_CONFIRMED);}
static void write_fixture(int fd,const char *name,const void *bytes,size_t n)
{
 int f=openat(fd,name,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);CHECK(f>=0);
 CHECK(write(f,bytes,n)==(ssize_t)n);CHECK(!close(f));
}
static size_t read_wire(int fd,const char *name,unsigned char *wire,size_t cap)
{
 int f=openat(fd,name,O_RDONLY|O_NOFOLLOW);ssize_t n;CHECK(f>=0);
 n=read(f,wire,cap);CHECK(n>=0);CHECK(!close(f));return (size_t)n;
}
static void p32(unsigned char *p,uint32_t v)
{for(unsigned j=0;j<4;j++)p[j]=(unsigned char)(v>>(8*j));}
static void p64(unsigned char *p,uint64_t v)
{p32(p,(uint32_t)v);p32(p+4,(uint32_t)(v>>32));}
static void re_crc(unsigned char *wire,size_t n)
{
 uint32_t table[256];p32(wire+12,0);camera_crc_bzip2_table(table);
 p32(wire+12,camera_crc_bzip2_ram(wire,n,table));
}
static void basic(void)
{
 int fd=new_dir("basic");struct camera_settings_store s=store(fd,NULL);
 struct camera_settings_report r;unsigned char p[512]={1};
 OK(camera_settings_inspect(&s,&r));CHECK(r.desired_state==CAMERA_SETTINGS_NONE);
 CHECK(!r.hardware_applied_known);seed(&s);
 OK(camera_settings_inspect(&s,&r));CHECK(r.current.revision==1);
 CHECK(r.durability==CAMERA_SETTINGS_NOT_CONFIRMED);
 OK(put(&s,0,1,3,&r));CHECK(r.durability==CAMERA_SETTINGS_NOT_CONFIRMED);
 CHECK(put(&s,0,1,4,&r)==CAMERA_SETTINGS_CONFLICT);
 CHECK(put(&s,0,2,4,&r)==CAMERA_SETTINGS_CONFLICT);
 {
  struct camera_settings_store second=store(fd,NULL);struct camera_settings_report old;
  OK(camera_settings_inspect(&second,&old));CHECK(old.current.revision==1);
  OK(put(&s,1,2,4,&r));CHECK(r.previous.payload[1]==3);
  CHECK(put(&second,old.current.revision,3,5,&r)==CAMERA_SETTINGS_CONFLICT);
 }
 CHECK(put(&s,2,1,5,&r)==CAMERA_SETTINGS_CONFLICT); /* retired receipt */
 OK(camera_settings_rollback(&s,2,3,&r));CHECK(r.current.revision==3);
 CHECK(r.current.payload[1]==3&&r.previous.payload[1]==4);
 OK(camera_settings_resume(&s,3,&r));CHECK(!r.durability);
 p[0]=9;CHECK(camera_settings_commit(&s,3,4,1,1,p,512,&r)==CAMERA_SETTINGS_SENSITIVE);
 p[0]=8;CHECK(camera_settings_commit(&s,3,4,1,1,p,512,&r)==CAMERA_SETTINGS_UNSUPPORTED);
 p[0]=1;CHECK(camera_settings_commit(&s,3,4,2,1,p,512,&r)==CAMERA_SETTINGS_UNSUPPORTED);
 CHECK(camera_settings_commit(&s,3,4,1,1,p,1025,&r)==CAMERA_SETTINGS_INVALID);
 CHECK(camera_settings_commit(&s,3,4,1,1,NULL,1,&r)==CAMERA_SETTINGS_INVALID);
 OK(camera_settings_inspect(&s,&r));CHECK(r.current.revision==3&&r.pending_file==0);
 OK(camera_settings_commit(&s,3,4,1,CAMERA_SETTINGS_RESET_REQUEST,NULL,0,&r));
 CHECK(r.current.kind==CAMERA_SETTINGS_RESET_REQUEST&&r.previous.payload[1]==3);
 CHECK(!r.hardware_applied_known);
 CHECK(camera_settings_commit(&s,4,5,1,CAMERA_SETTINGS_RESET_REQUEST,NULL,0,&r)==CAMERA_SETTINGS_CONFLICT);
 CHECK(r.previous.payload[1]==3);CHECK(!close(fd));
}
static void faults(void)
{
 unsigned counts[6];int fd=new_dir("fault-reference");struct injection ref={0};
 struct camera_settings_store s=store(fd,NULL);struct camera_settings_report r;
 seed(&s);s.fault=inject;s.fault_context=&ref;OK(put(&s,1,2,4,&r));
 {
  const unsigned order[]={5,1,1,1,1,1,2,5,5,5,2,5,3,4,3,5};
  CHECK(ref.event_count==sizeof(order)/sizeof(order[0]));
  CHECK(!memcmp(order,ref.events,sizeof(order))); /* sync/rename/readback order */
 }
 memcpy(counts,ref.counts,sizeof(counts));CHECK(!close(fd));
 for(unsigned point=1;point<=5;point++)for(unsigned nth=1;nth<=counts[point];nth++){
  struct injection i={0};int result;char label[80];
  snprintf(label,sizeof(label),"fault-%u-%u",point,nth);fd=new_dir(label);s=store(fd,NULL);seed(&s);
  i.point=point;i.nth=nth;s.fault=inject;s.fault_context=&i;
  result=put(&s,1,2,4,&r);int initial=result;
  CHECK(i.fired);CHECK(result!=CAMERA_SETTINGS_OK);
  s.fault=NULL;result=camera_settings_inspect(&s,&r);
  CHECK(!r.durability&&!r.hardware_applied_known);
  if(result==CAMERA_SETTINGS_RECOVERY_REQUIRED){
   CHECK(r.current.revision==1&&r.pending.revision==2);
   CHECK(camera_settings_resume(&s,99,&r)==CAMERA_SETTINGS_CONFLICT);
   CHECK(put(&s,1,3,6,&r)==CAMERA_SETTINGS_RECOVERY_REQUIRED);
   OK(camera_settings_resume(&s,2,&r));CHECK(r.durability==CAMERA_SETTINGS_SYNC_CONFIRMED);
   CHECK(r.current.revision==2&&r.previous.revision==1);
  }else if(result==CAMERA_SETTINGS_CORRUPT){
   unsigned char before[2128],after[2128];size_t n,m;
   CHECK(r.current.revision==1&&r.pending_file==CAMERA_SETTINGS_BAD);
   n=read_wire(fd,CAMERA_SETTINGS_PENDING,before,sizeof(before));
   CHECK(camera_settings_resume(&s,2,&r)==CAMERA_SETTINGS_CORRUPT);
   CHECK(put(&s,1,3,6,&r)==CAMERA_SETTINGS_CORRUPT);
   m=read_wire(fd,CAMERA_SETTINGS_PENDING,after,sizeof(after));
   CHECK(n==m&&!memcmp(before,after,n));
  }else{
   CHECK(result==CAMERA_SETTINGS_OK);
   CHECK(r.current.revision==1||r.current.revision==2);
   if(r.current.revision==2){OK(camera_settings_resume(&s,2,&r));CHECK(!r.durability);}
  }
  printf("fault point=%u nth=%u initial=%d observed=%d final_state=%u current=%llu\n",
   point,nth,initial,result,r.desired_state,(unsigned long long)r.current.revision);
  CHECK(!close(fd));
 }
 /* Process exits at each normal I/O boundary. This models interrupted process,
  * NOT actual filesystem power-loss cache behavior. */
 for(unsigned point=1;point<=5;point++)for(unsigned nth=1;nth<=counts[point];nth++){
  pid_t pid;int status,result;char label[80];
  snprintf(label,sizeof(label),"crash-%u-%u",point,nth);fd=new_dir(label);s=store(fd,NULL);seed(&s);
  pid=fork();CHECK(pid>=0);
  if(!pid){struct injection i={0};i.point=point;i.nth=nth;i.crash=1;
   s.fault=inject;s.fault_context=&i;(void)put(&s,1,2,4,&r);_exit(78);}
  CHECK(waitpid(pid,&status,0)==pid);CHECK(WIFEXITED(status)&&WEXITSTATUS(status)==77);
  result=camera_settings_inspect(&s,&r);CHECK(!r.durability);
  CHECK(result==CAMERA_SETTINGS_OK||result==CAMERA_SETTINGS_RECOVERY_REQUIRED||
        result==CAMERA_SETTINGS_CORRUPT);
  CHECK(r.current.revision==1||r.current.revision==2);
  printf("crash point=%u nth=%u observed=%d current=%llu pending=%u\n",
   point,nth,result,(unsigned long long)r.current.revision,r.pending_file);
  CHECK(!close(fd));
 }
}
static void unsafe(void)
{
 struct camera_settings_store s;struct camera_settings_report r;int fd,f;
 fd=new_dir("unknown-footage");write_fixture(fd,"recording.mp4","public fixture",14);
 CHECK(camera_settings_init(&s,fd,public_validate,NULL,NULL,NULL)==CAMERA_SETTINGS_UNSAFE);
 CHECK(!close(fd));
 fd=new_dir("symlink");CHECK(!symlinkat("../not-a-target",fd,CAMERA_SETTINGS_CURRENT));s=store(fd,NULL);
 CHECK(camera_settings_inspect(&s,&r)==CAMERA_SETTINGS_UNSAFE);CHECK(!close(fd));
 fd=new_dir("fifo");CHECK(!mkfifoat(fd,CAMERA_SETTINGS_CURRENT,0600));s=store(fd,NULL);
 CHECK(camera_settings_inspect(&s,&r)==CAMERA_SETTINGS_UNSAFE);CHECK(!close(fd));
 fd=new_dir("bad-file-mode");s=store(fd,NULL);seed(&s);
 CHECK(!fchmodat(fd,CAMERA_SETTINGS_CURRENT,0640,0));
 CHECK(camera_settings_inspect(&s,&r)==CAMERA_SETTINGS_UNSAFE);CHECK(!close(fd));
 fd=new_dir("bad-directory-mode");CHECK(!fchmod(fd,0750));
 CHECK(camera_settings_init(&s,fd,public_validate,NULL,NULL,NULL)==CAMERA_SETTINGS_UNSAFE);
 CHECK(!close(fd));
 fd=new_dir("hardlink");s=store(fd,NULL);seed(&s);
 CHECK(!linkat(fd,CAMERA_SETTINGS_CURRENT,root,"owned-hardlink-evidence",0));
 CHECK(camera_settings_inspect(&s,&r)==CAMERA_SETTINGS_UNSAFE);CHECK(!close(fd));
 fd=new_dir("directory-as-file");CHECK(!mkdirat(fd,CAMERA_SETTINGS_CURRENT,0700));s=store(fd,NULL);
 CHECK(camera_settings_inspect(&s,&r)==CAMERA_SETTINGS_UNSAFE);CHECK(!close(fd));
 /* Actual uid fault only when this disposable fixture has permission. */
 fd=new_dir("wrong-owner");s=store(fd,NULL);seed(&s);
 f=openat(fd,CAMERA_SETTINGS_CURRENT,O_RDONLY|O_NOFOLLOW);CHECK(f>=0);
 if(!fchown(f,geteuid()==0?1:0,(gid_t)-1)){
  CHECK(camera_settings_inspect(&s,&r)==CAMERA_SETTINGS_UNSAFE);
  puts("file uid mismatch exercised");
 }else{CHECK(errno==EPERM);puts("file uid mismatch SKIP: unprivileged host");}
 CHECK(!close(f));CHECK(!close(fd));
 fd=new_dir("wrong-directory-owner");
 if(!fchown(fd,geteuid()==0?1:0,(gid_t)-1)){
  CHECK(camera_settings_init(&s,fd,public_validate,NULL,NULL,NULL)==CAMERA_SETTINGS_UNSAFE);
  puts("directory uid mismatch exercised");
 }else{CHECK(errno==EPERM);puts("directory uid mismatch SKIP: unprivileged host");}
 CHECK(!close(fd));
 #ifdef CAMERA_SETTINGS_WRAP_UID
 fd=new_dir("injected-directory-uid");uid_calls=0;uid_fault_nth=1;
 CHECK(camera_settings_init(&s,fd,public_validate,NULL,NULL,NULL)==CAMERA_SETTINGS_UNSAFE);
 uid_fault_nth=0;CHECK(!close(fd));
 fd=new_dir("injected-file-uid");s=store(fd,NULL);seed(&s);uid_calls=0;uid_fault_nth=2;
 CHECK(camera_settings_inspect(&s,&r)==CAMERA_SETTINGS_UNSAFE);
 CHECK(r.current_file==CAMERA_SETTINGS_BAD&&r.current_error==CAMERA_SETTINGS_UNSAFE);
 uid_fault_nth=0;CHECK(!close(fd));puts("uid mismatch branches exercised via linker injection");
 #endif
 fd=new_dir("no-validator");CHECK(camera_settings_init(&s,fd,NULL,NULL,NULL,NULL)==CAMERA_SETTINGS_INVALID);
 CHECK(!close(fd));
}
static void malformed(void)
{
 unsigned char original[2128],wire[2128];size_t n;int fd=new_dir("wire-source");
 struct camera_settings_store s=store(fd,NULL);struct camera_settings_report r;
 seed(&s);OK(put(&s,1,2,4,&r));n=read_wire(fd,CAMERA_SETTINGS_CURRENT,original,sizeof(original));
 CHECK(!close(fd));
 for(unsigned variant=0;variant<13;variant++){
  size_t bytes=n;char label[80];snprintf(label,sizeof(label),"malformed-%u",variant);
  fd=new_dir(label);s=store(fd,NULL);memcpy(wire,original,n);
  switch(variant){
  case 0:wire[0]^=1;break;
  case 1:wire[4]=2;break;
  case 2:p32(wire+20,1025);break;
  case 3:p32(wire+32,UINT32_MAX);break;
  case 4:wire[72]=1;break;
  case 5:p64(wire+40,0);break;
  case 6:p64(wire+56,0);break;
  case 7:bytes=40;break;
  case 8:bytes=n-1;break;
  case 9:wire[12]^=1;break;
  case 10:wire[81]=5;break; /* valid schema, bad CRC */
  case 11:p32(wire+16,2);re_crc(wire,n);break;
  case 12:wire[80]=8;re_crc(wire,n);break;
  }
  write_fixture(fd,CAMERA_SETTINGS_CURRENT,wire,bytes);
  int result=camera_settings_inspect(&s,&r);
  CHECK(result==(variant>=11?CAMERA_SETTINGS_UNSUPPORTED:CAMERA_SETTINGS_CORRUPT));
  CHECK(r.current_file==CAMERA_SETTINGS_BAD&&!r.current.revision);
  CHECK(put(&s,0,3,6,&r)==result);CHECK(!close(fd));
 }
 /* A valid pending record with wrong expected snapshot must not replace current. */
 fd=new_dir("mismatched-pending");s=store(fd,NULL);seed(&s);
 memcpy(wire,original,n);wire[80+512+1]=7;re_crc(wire,n);
 write_fixture(fd,CAMERA_SETTINGS_PENDING,wire,n);
 CHECK(camera_settings_inspect(&s,&r)==CAMERA_SETTINGS_CONFLICT);
 CHECK(r.current.revision==1&&r.pending.revision==2);
 CHECK(camera_settings_resume(&s,2,&r)==CAMERA_SETTINGS_CONFLICT);CHECK(!close(fd));
 /* Intact pending cannot justify overwriting torn current. */
 fd=new_dir("torn-current-intact-pending");s=store(fd,NULL);
 write_fixture(fd,CAMERA_SETTINGS_CURRENT,original,40);
 write_fixture(fd,CAMERA_SETTINGS_PENDING,original,n);
 CHECK(camera_settings_inspect(&s,&r)==CAMERA_SETTINGS_CORRUPT);
 CHECK(r.pending_file==CAMERA_SETTINGS_VALID&&r.current_file==CAMERA_SETTINGS_BAD);
 CHECK(camera_settings_resume(&s,2,&r)==CAMERA_SETTINGS_CORRUPT);CHECK(!close(fd));
 /* Max revisions/IDs remain readable but cannot wrap. */
 fd=new_dir("revision-exhausted");s=store(fd,NULL);memcpy(wire,original,n);
 p64(wire+40,UINT64_MAX);p64(wire+48,UINT64_MAX-1);re_crc(wire,n);
 write_fixture(fd,CAMERA_SETTINGS_CURRENT,wire,n);OK(camera_settings_inspect(&s,&r));
 CHECK(put(&s,UINT64_MAX,3,6,&r)==CAMERA_SETTINGS_EXHAUSTED);CHECK(!close(fd));
 fd=new_dir("operation-exhausted");s=store(fd,NULL);memcpy(wire,original,n);
 p64(wire+56,UINT64_MAX);re_crc(wire,n);write_fixture(fd,CAMERA_SETTINGS_CURRENT,wire,n);
 OK(camera_settings_inspect(&s,&r));CHECK(put(&s,2,3,6,&r)==CAMERA_SETTINGS_EXHAUSTED);
 CHECK(!close(fd));
}
static void initial_recovery(void)
{
 int fd=new_dir("initial-pending");struct injection i={0};struct camera_settings_store s=store(fd,&i);
 struct camera_settings_report r;i.point=CAMERA_SETTINGS_RENAME;i.nth=1;
 CHECK(put(&s,0,1,3,&r)==CAMERA_SETTINGS_UNCERTAIN);CHECK(i.fired);s.fault=NULL;
 CHECK(camera_settings_inspect(&s,&r)==CAMERA_SETTINGS_RECOVERY_REQUIRED);
 CHECK(r.current_file==CAMERA_SETTINGS_ABSENT&&r.pending.revision==1);
 OK(camera_settings_resume(&s,1,&r));CHECK(r.current.revision==1&&!r.previous.revision);
 CHECK(camera_settings_rollback(&s,1,2,&r)==CAMERA_SETTINGS_INVALID);CHECK(!close(fd));
 fd=new_dir("aliased-input-output");s=store(fd,NULL);seed(&s);
 OK(camera_settings_inspect(&s,&r));
 OK(camera_settings_commit(&s,1,2,1,CAMERA_SETTINGS_VALUES,r.current.payload,512,&r));
 CHECK(r.current.payload[1]==3&&r.current.revision==2);CHECK(!close(fd));
 fd=new_dir("maximum-record");s=store(fd,NULL);
 {
  unsigned char p[1024]={1};struct stat st;
  OK(camera_settings_commit(&s,0,1,1,CAMERA_SETTINGS_VALUES,p,sizeof(p),&r));
  p[1]=20;OK(camera_settings_commit(&s,1,2,1,CAMERA_SETTINGS_VALUES,p,sizeof(p),&r));
  CHECK(!fstatat(fd,CAMERA_SETTINGS_CURRENT,&st,AT_SYMLINK_NOFOLLOW));CHECK(st.st_size==2128);
  OK(camera_settings_inspect(&s,&r));
  CHECK(r.current.payload_bytes==1024&&r.previous.payload_bytes==1024&&r.current.payload[1]==20);
  OK(camera_settings_rollback(&s,2,3,&r));CHECK(r.current.payload[1]==0);
 }
 CHECK(!close(fd));
}
int main(int argc,char **argv)
{
 uint32_t table[256];CHECK(argc==2);CHECK(!mkdir(argv[1],0700));
 root=open(argv[1],O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);CHECK(root>=0);
 camera_crc_bzip2_table(table);
 CHECK(camera_crc_bzip2_ram("123456789",9,table)==0xfc891918U);
 basic();faults();unsafe();malformed();initial_recovery();CHECK(!close(root));
 printf("settings-store: %u checks passed; %u fixture directories retained\n",checks,serial);
 return 0;
}
