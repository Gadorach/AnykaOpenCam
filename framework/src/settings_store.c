/* SPDX-License-Identifier: GPL-2.0-only */
#define _POSIX_C_SOURCE 200809L
#include <camera/settings_store.h>
#include <camera/dma_integrity.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define STORE_MAGIC 0x53535431U
#define WIRE_MAGIC 0x31545343U
#define HEADER_BYTES 80U
#define RECORD_BYTES (HEADER_BYTES+2U*CAMERA_SETTINGS_MAX_PAYLOAD)

static uint32_t get32(const unsigned char *p)
{return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static uint64_t get64(const unsigned char *p)
{return get32(p)|((uint64_t)get32(p+4)<<32);}
static void put32(unsigned char *p,uint32_t v)
{for(unsigned i=0;i<4;i++)p[i]=(unsigned char)(v>>(8*i));}
static void put64(unsigned char *p,uint64_t v)
{put32(p,(uint32_t)v);put32(p+4,(uint32_t)(v>>32));}
static int fault(struct camera_settings_store *s,uint32_t point)
{return s->fault&&s->fault(s->fault_context,point);}
static int close_file(struct camera_settings_store *s,int fd)
{
 int injected=fault(s,CAMERA_SETTINGS_CLOSE);
 int r=close(fd);return r||injected?CAMERA_SETTINGS_IO:CAMERA_SETTINGS_OK;
}
static int sync_fd(struct camera_settings_store *s,int fd,uint32_t point)
{return fault(s,point)||fsync(fd)?CAMERA_SETTINGS_IO:CAMERA_SETTINGS_OK;}
static int ready(const struct camera_settings_store *s)
{return s&&s->initialized==STORE_MAGIC&&s->validate;}
static int safe_file(const struct stat *st)
{
 return S_ISREG(st->st_mode)&&st->st_uid==geteuid()&&
  (st->st_mode&07777)==0600&&st->st_nlink==1;
}
static int directory(struct camera_settings_store *s)
{
 struct stat st;struct dirent *entry;DIR *dir;int fd,result=CAMERA_SETTINGS_OK;
 if(fstat(s->dirfd,&st))return CAMERA_SETTINGS_IO;
 if(!S_ISDIR(st.st_mode)||st.st_uid!=geteuid()||(st.st_mode&07777)!=0700)
  return CAMERA_SETTINGS_UNSAFE;
 /* Independent open description, not dup(): don't share readdir offset. */
 fd=openat(s->dirfd,".",O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);
 if(fd<0)return CAMERA_SETTINGS_IO;
 dir=fdopendir(fd);if(!dir){close(fd);return CAMERA_SETTINGS_IO;}
 errno=0;
 while((entry=readdir(dir))){
  if(!strcmp(entry->d_name,".")||!strcmp(entry->d_name,".."))continue;
  if(strcmp(entry->d_name,CAMERA_SETTINGS_CURRENT)&&
     strcmp(entry->d_name,CAMERA_SETTINGS_PENDING)){result=CAMERA_SETTINGS_UNSAFE;break;}
 }
 if(!entry&&errno)result=CAMERA_SETTINGS_IO;
 if(closedir(dir)&&result==CAMERA_SETTINGS_OK)result=CAMERA_SETTINGS_IO;
 return result;
}
static int validate(struct camera_settings_store *s,
 const struct camera_settings_snapshot *v)
{
 int r;
 if(!v->schema||v->payload_bytes>CAMERA_SETTINGS_MAX_PAYLOAD||
    (v->kind!=CAMERA_SETTINGS_VALUES&&v->kind!=CAMERA_SETTINGS_RESET_REQUEST))
  return CAMERA_SETTINGS_INVALID;
 r=s->validate(s->validate_context,v->schema,v->kind,v->payload,v->payload_bytes);
 if(r==CAMERA_SETTINGS_OK||r==CAMERA_SETTINGS_UNSUPPORTED||
    r==CAMERA_SETTINGS_SENSITIVE)return r;
 return CAMERA_SETTINGS_INVALID;
}
static uint32_t crc(unsigned char *wire,size_t n)
{
 uint32_t table[256],saved=get32(wire+12),result;
 put32(wire+12,0);camera_crc_bzip2_table(table);
 result=camera_crc_bzip2_ram(wire,n,table);put32(wire+12,saved);return result;
}
static size_t encode(unsigned char *wire,const struct camera_settings_snapshot *v,
 const struct camera_settings_snapshot *old)
{
 size_t bytes=HEADER_BYTES+v->payload_bytes+old->payload_bytes;
 memset(wire,0,RECORD_BYTES);put32(wire,WIRE_MAGIC);put32(wire+4,1);
 put32(wire+8,(uint32_t)bytes);put32(wire+16,v->schema);
 put32(wire+20,v->payload_bytes);put32(wire+24,v->kind);
 put32(wire+28,old->schema);put32(wire+32,old->payload_bytes);
 put32(wire+36,old->kind);put64(wire+40,v->revision);
 put64(wire+48,old->revision);put64(wire+56,v->operation_id);
 put64(wire+64,old->operation_id);
 memcpy(wire+HEADER_BYTES,v->payload,v->payload_bytes);
 memcpy(wire+HEADER_BYTES+v->payload_bytes,old->payload,old->payload_bytes);
 put32(wire+12,crc(wire,bytes));return bytes;
}
static int decode(struct camera_settings_store *s,unsigned char *wire,size_t n,
 struct camera_settings_snapshot *v,struct camera_settings_snapshot *old)
{
 int r;uint64_t expected;uint32_t len,previous_len;
 if(n<HEADER_BYTES||get32(wire)!=WIRE_MAGIC||get32(wire+4)!=1||
    get32(wire+8)!=n||get64(wire+72))return CAMERA_SETTINGS_CORRUPT;
 len=get32(wire+20);previous_len=get32(wire+32);expected=get64(wire+48);
 if(len>CAMERA_SETTINGS_MAX_PAYLOAD||previous_len>CAMERA_SETTINGS_MAX_PAYLOAD||
    n!=HEADER_BYTES+len+previous_len||expected==UINT64_MAX||
    get64(wire+40)!=expected+1||!get64(wire+56))return CAMERA_SETTINGS_CORRUPT;
 memset(v,0,sizeof(*v));memset(old,0,sizeof(*old));
 v->schema=get32(wire+16);v->kind=get32(wire+24);v->payload_bytes=len;
 v->revision=get64(wire+40);v->operation_id=get64(wire+56);
 old->schema=get32(wire+28);old->kind=get32(wire+36);old->payload_bytes=previous_len;
 old->revision=expected;old->operation_id=get64(wire+64);
 if(!expected&&(old->schema||old->kind||previous_len||old->operation_id))
  return CAMERA_SETTINGS_CORRUPT;
 if(expected&&(!old->operation_id||old->operation_id>=v->operation_id))
  return CAMERA_SETTINGS_CORRUPT;
 memcpy(v->payload,wire+HEADER_BYTES,len);
 memcpy(old->payload,wire+HEADER_BYTES+len,previous_len);
 r=validate(s,v);if(r)return r==CAMERA_SETTINGS_INVALID?CAMERA_SETTINGS_CORRUPT:r;
 if(expected){r=validate(s,old);if(r)return r==CAMERA_SETTINGS_INVALID?CAMERA_SETTINGS_CORRUPT:r;}
 return crc(wire,n)==get32(wire+12)?CAMERA_SETTINGS_OK:CAMERA_SETTINGS_CORRUPT;
}
static int load(struct camera_settings_store *s,const char *name,uint32_t *state,
 struct camera_settings_snapshot *v,struct camera_settings_snapshot *old)
{
 unsigned char wire[RECORD_BYTES];struct stat st;size_t used=0;int fd,r;
 *state=CAMERA_SETTINGS_BAD;
 /* Reject special files before open as well as checking the opened inode.
  * Exclusive owner serialization still required: no same-UID hostile races. */
 if(fstatat(s->dirfd,name,&st,AT_SYMLINK_NOFOLLOW)){
  if(errno==ENOENT){*state=CAMERA_SETTINGS_ABSENT;return CAMERA_SETTINGS_OK;}
  return CAMERA_SETTINGS_IO;
 }
 if(!safe_file(&st))return CAMERA_SETTINGS_UNSAFE;
 /* Nonblock avoids hanging on a hostile FIFO before fstat rejects it. */
 fd=openat(s->dirfd,name,O_RDONLY|O_NOFOLLOW|O_CLOEXEC|O_NONBLOCK);
 if(fd<0){
  if(errno==ENOENT){*state=CAMERA_SETTINGS_ABSENT;return CAMERA_SETTINGS_OK;}
  return errno==ELOOP?CAMERA_SETTINGS_UNSAFE:CAMERA_SETTINGS_IO;
 }
 if(fstat(fd,&st))r=CAMERA_SETTINGS_IO;
 else if(!safe_file(&st))r=CAMERA_SETTINGS_UNSAFE;
 else if(st.st_size<HEADER_BYTES||st.st_size>RECORD_BYTES)r=CAMERA_SETTINGS_CORRUPT;
 else{
  r=CAMERA_SETTINGS_OK;
  while(used<(size_t)st.st_size){
   ssize_t got=read(fd,wire+used,(size_t)st.st_size-used);
   if(got<0&&errno==EINTR)continue;
   if(got<=0){r=got<0?CAMERA_SETTINGS_IO:CAMERA_SETTINGS_CORRUPT;break;}
   used+=(size_t)got;
  }
  if(!r){unsigned char tail;ssize_t got=read(fd,&tail,1);
   if(got)r=got<0?CAMERA_SETTINGS_IO:CAMERA_SETTINGS_CORRUPT;
  }
  if(!r)r=decode(s,wire,used,v,old);
 }
 if(close_file(s,fd)&&!r)r=CAMERA_SETTINGS_IO;
 if(!r)*state=CAMERA_SETTINGS_VALID;
 else{memset(v,0,sizeof(*v));memset(old,0,sizeof(*old));}
 return r;
}
static int same(const struct camera_settings_snapshot *a,
 const struct camera_settings_snapshot *b)
{
 return a->schema==b->schema&&a->kind==b->kind&&a->payload_bytes==b->payload_bytes&&
  a->revision==b->revision&&a->operation_id==b->operation_id&&
  !memcmp(a->payload,b->payload,a->payload_bytes);
}
int camera_settings_init(struct camera_settings_store *s,int dirfd,
 camera_settings_validate_fn fn,void *context,camera_settings_fault_fn hook,void *hc)
{
 int r;if(!s||dirfd<0||!fn)return CAMERA_SETTINGS_INVALID;
 memset(s,0,sizeof(*s));s->dirfd=dirfd;s->validate=fn;s->validate_context=context;
 s->fault=hook;s->fault_context=hc;
 r=directory(s);if(!r)s->initialized=STORE_MAGIC;return r;
}
int camera_settings_inspect(struct camera_settings_store *s,struct camera_settings_report *out)
{
 int r;if(!out)return CAMERA_SETTINGS_INVALID;memset(out,0,sizeof(*out));
 if(!ready(s))return CAMERA_SETTINGS_INVALID;
 out->desired_state=CAMERA_SETTINGS_BLOCKED;
 r=directory(s);if(r)return r;
 out->current_error=load(s,CAMERA_SETTINGS_CURRENT,&out->current_file,
  &out->current,&out->previous);
 out->pending_error=load(s,CAMERA_SETTINGS_PENDING,&out->pending_file,
  &out->pending,&out->pending_previous);
 if(out->current_error)return out->current_error;
 if(out->pending_error)return out->pending_error;
 if(out->pending_file==CAMERA_SETTINGS_VALID){
  if(!same(&out->current,&out->pending_previous))return CAMERA_SETTINGS_CONFLICT;
  out->desired_state=CAMERA_SETTINGS_PREPARED;return CAMERA_SETTINGS_RECOVERY_REQUIRED;
 }
 out->desired_state=out->current_file==CAMERA_SETTINGS_VALID?
  CAMERA_SETTINGS_COMMITTED:CAMERA_SETTINGS_NONE;
 return CAMERA_SETTINGS_OK;
}
static int promote(struct camera_settings_store *s,
 const struct camera_settings_snapshot *v,const struct camera_settings_snapshot *old,
 struct camera_settings_report *out)
{
 int fd,r;struct stat st;
 fd=openat(s->dirfd,CAMERA_SETTINGS_PENDING,O_RDONLY|O_NOFOLLOW|O_CLOEXEC|O_NONBLOCK);
 if(fd<0)return CAMERA_SETTINGS_UNCERTAIN;
 r=fstat(fd,&st)||!safe_file(&st)?CAMERA_SETTINGS_UNSAFE:
  sync_fd(s,fd,CAMERA_SETTINGS_FILE_SYNC);
 if(close_file(s,fd)&&!r)r=CAMERA_SETTINGS_IO;
 if(r)return CAMERA_SETTINGS_UNCERTAIN;
 if(sync_fd(s,s->dirfd,CAMERA_SETTINGS_DIR_SYNC))return CAMERA_SETTINGS_UNCERTAIN;
 if(fault(s,CAMERA_SETTINGS_RENAME)||
    renameat(s->dirfd,CAMERA_SETTINGS_PENDING,s->dirfd,CAMERA_SETTINGS_CURRENT))
  return CAMERA_SETTINGS_UNCERTAIN;
 if(sync_fd(s,s->dirfd,CAMERA_SETTINGS_DIR_SYNC))return CAMERA_SETTINGS_UNCERTAIN;
 r=camera_settings_inspect(s,out);
 if(r||!same(v,&out->current)||!same(old,&out->previous))return CAMERA_SETTINGS_UNCERTAIN;
 out->durability=CAMERA_SETTINGS_SYNC_CONFIRMED;return CAMERA_SETTINGS_OK;
}
int camera_settings_commit(struct camera_settings_store *s,uint64_t expected,uint64_t op,
 uint32_t schema,uint32_t kind,const void *payload,uint32_t bytes,
 struct camera_settings_report *out)
{
 struct camera_settings_snapshot v={0};unsigned char wire[RECORD_BYTES];
 size_t n,used=0;int fd,r,write_error=0;struct stat st;
 if(!ready(s)||!out||!op||bytes>CAMERA_SETTINGS_MAX_PAYLOAD||(!payload&&bytes))
  return CAMERA_SETTINGS_INVALID;
 v.schema=schema;v.kind=kind;v.payload_bytes=bytes;
 if(bytes)memcpy(v.payload,payload,bytes);
 memset(out,0,sizeof(*out));
 r=validate(s,&v);if(r)return r; /* No CRC/log/persistence before validation. */
 r=camera_settings_inspect(s,out);if(r)return r;
 if(out->current_file==CAMERA_SETTINGS_VALID&&op==out->current.operation_id){
  v.revision=out->current.revision;v.operation_id=op;
  return expected==out->previous.revision&&same(&v,&out->current)?
   CAMERA_SETTINGS_OK:CAMERA_SETTINGS_CONFLICT;
 }
 if(expected!=out->current.revision)
  return CAMERA_SETTINGS_CONFLICT;
 if(expected==UINT64_MAX||out->current.operation_id==UINT64_MAX)
  return CAMERA_SETTINGS_EXHAUSTED;
 if(op<=out->current.operation_id)return CAMERA_SETTINGS_CONFLICT;
 /* A second reset request must not displace the retained values snapshot.
  * Only SETUP's later supervised workflow may act on the first request. */
 if(kind==CAMERA_SETTINGS_RESET_REQUEST&&
    out->current.kind==CAMERA_SETTINGS_RESET_REQUEST)return CAMERA_SETTINGS_CONFLICT;
 v.revision=expected+1;v.operation_id=op;n=encode(wire,&v,&out->current);
 fd=openat(s->dirfd,CAMERA_SETTINGS_PENDING,
  O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);
 if(fd<0)return errno==EEXIST?CAMERA_SETTINGS_RECOVERY_REQUIRED:CAMERA_SETTINGS_IO;
 if(fstat(fd,&st)||!safe_file(&st))write_error=1;
 while(!write_error&&used<n){
  size_t chunk=n-used;ssize_t wrote;if(chunk>256)chunk=256;
  if(fault(s,CAMERA_SETTINGS_WRITE)){write_error=1;break;}
  wrote=write(fd,wire+used,chunk);
  if(wrote<0&&errno==EINTR)continue;
  if(wrote<=0){write_error=1;break;}used+=(size_t)wrote;
 }
 if(!write_error&&sync_fd(s,fd,CAMERA_SETTINGS_FILE_SYNC))write_error=1;
 if(close_file(s,fd))write_error=1;
 if(write_error){(void)camera_settings_inspect(s,out);return CAMERA_SETTINGS_UNCERTAIN;}
 /* Load validates the exact pending bytes before any replacement. */
 r=camera_settings_inspect(s,out);
 if(r!=CAMERA_SETTINGS_RECOVERY_REQUIRED||!same(&v,&out->pending))
  return CAMERA_SETTINGS_UNCERTAIN;
 {
  struct camera_settings_snapshot old=out->current;
  r=promote(s,&v,&old,out);
 }
 if(r)(void)camera_settings_inspect(s,out);
 return r;
}
int camera_settings_resume(struct camera_settings_store *s,uint64_t op,
 struct camera_settings_report *out)
{
 int r;struct camera_settings_snapshot v,old;
 if(!op||!out)return CAMERA_SETTINGS_INVALID;
 r=camera_settings_inspect(s,out);
 if(!r)return out->current_file==CAMERA_SETTINGS_VALID&&
  out->current.operation_id==op?CAMERA_SETTINGS_OK:CAMERA_SETTINGS_CONFLICT;
 if(r!=CAMERA_SETTINGS_RECOVERY_REQUIRED)return r;
 if(out->pending.operation_id!=op)return CAMERA_SETTINGS_CONFLICT;
 v=out->pending;old=out->pending_previous;r=promote(s,&v,&old,out);
 if(r)(void)camera_settings_inspect(s,out);
 return r;
}
int camera_settings_rollback(struct camera_settings_store *s,uint64_t expected,uint64_t op,
 struct camera_settings_report *out)
{
 struct camera_settings_snapshot old;int r;
 if(!out)return CAMERA_SETTINGS_INVALID;
 r=camera_settings_inspect(s,out);if(r)return r;
 if(out->current.revision!=expected)return CAMERA_SETTINGS_CONFLICT;
 if(!out->previous.revision)return CAMERA_SETTINGS_INVALID;
 old=out->previous;
 return camera_settings_commit(s,expected,op,old.schema,old.kind,
  old.payload,old.payload_bytes,out);
}
