/* SPDX-License-Identifier: GPL-2.0-only */
#define _POSIX_C_SOURCE 200809L
#include <camera/recording_index_store.h>
#include <camera/dma_integrity.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define INDEX_MAGIC 0x52495331U
_Static_assert(CAMERA_ID_BYTES==64&&CAMERA_RECORDING_MAX_SENSORS==4&&CAMERA_RECORDING_MAX_ENTRIES==64,"wire v1 fixed bounds");
static const unsigned char wire_magic[8]={'R','E','C','I','D','X','0','1'};
static int disjoint(const void *a,size_t an,const void *b,size_t bn)
{
 uintptr_t av=(uintptr_t)a,bv=(uintptr_t)b;
 if(!an||!bn)return 1;
 if(!a||!b||an-1>UINTPTR_MAX-av||bn-1>UINTPTR_MAX-bv)return 0;
 return av<=bv?bv-av>=an:av-bv>=bn;
}
static int outside(const struct camera_recording_index_store *s,const void *p,size_t n)
{return p&&disjoint(s,sizeof(*s),p,n);}
static int zero(const void *ptr,size_t n)
{const unsigned char *p=ptr;for(size_t i=0;i<n;i++)if(p[i])return 0;return 1;}
static int id_valid(const char id[CAMERA_ID_BYTES])
{
 if(!id[0])return 0;
 for(size_t i=0;i<CAMERA_ID_BYTES;i++){
  unsigned char c=(unsigned char)id[i];
  if(!c)return zero(id+i,CAMERA_ID_BYTES-i);
  if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'||c=='_'||c=='.'))return 0;
 }return 0;
}
static int same_id(const char *a,const char *b){return !memcmp(a,b,CAMERA_ID_BYTES);}
static uint32_t get32(const unsigned char *p)
{return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static uint64_t get64(const unsigned char *p){return get32(p)|((uint64_t)get32(p+4)<<32);}
static void put32(unsigned char *p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=(unsigned char)(v>>(8*i));}
static void put64(unsigned char *p,uint64_t v){put32(p,(uint32_t)v);put32(p+4,(uint32_t)(v>>32));}
static int scope_valid(const struct camera_recording_index_scope *q)
{
 if(!q||!id_valid(q->space.storage_id)||!q->space.storage_generation||!q->space.instance_generation||
    !q->sensor_count||q->sensor_count>CAMERA_RECORDING_MAX_SENSORS)return 0;
 for(uint32_t i=0;i<q->sensor_count;i++){
  if(!id_valid(q->sensors[i]))return 0;
  for(uint32_t j=0;j<i;j++)if(same_id(q->sensors[i],q->sensors[j]))return 0;
 }
 for(uint32_t i=q->sensor_count;i<CAMERA_RECORDING_MAX_SENSORS;i++)if(!zero(q->sensors[i],CAMERA_ID_BYTES))return 0;
 return 1;
}
static int metadata_valid(const struct camera_recording_metadata *m,const struct camera_recording_index_scope *q)
{
 if(m->abi!=CAMERA_RECORDING_ABI||m->struct_bytes!=sizeof(*m)||m->reserved||
    !id_valid(m->recording_id)||!id_valid(m->sensor_id)||
    m->origin<CAMERA_RECORDING_SERVICE_OWNED||m->origin>CAMERA_RECORDING_FOREIGN||
    m->state<CAMERA_RECORDING_OPEN||m->state>CAMERA_RECORDING_CORRUPT||m->closed>1||m->active>1||
    m->time_domain>CAMERA_RECORDING_TIME_QUALIFIED_UTC)return 0;
 if(m->state==CAMERA_RECORDING_OPEN&&m->closed)return 0;
 if(m->state==CAMERA_RECORDING_COMPLETE&&(!m->closed||m->active))return 0;
 if(m->time_domain==CAMERA_RECORDING_TIME_UNKNOWN){if(m->start_ns||m->end_ns||m->time_epoch)return 0;}
 else if(!m->time_epoch||m->start_ns>m->end_ns)return 0;
 for(uint32_t i=0;i<q->sensor_count;i++)if(same_id(m->sensor_id,q->sensors[i]))return 1;
 return 0;
}
static int snapshot_valid(const struct camera_recording_index_snapshot *v)
{
 uint64_t bytes=0,last=0;
 if(!v||!scope_valid(&v->scope)||!v->revision||!v->catalog_revision||
    v->entry_count>CAMERA_RECORDING_MAX_ENTRIES)return 0;
 for(uint32_t i=0;i<v->entry_count;i++){
  const struct camera_recording_index_entry *e=&v->entries[i];
  if(!metadata_valid(&e->metadata,&v->scope)||e->generation<=last||e->generation>v->generation_highwater||
     e->instance_generation>v->scope.space.instance_generation||e->metadata.bytes>UINT64_MAX-bytes)return 0;
  last=e->generation;bytes+=e->metadata.bytes;
  for(uint32_t j=0;j<i;j++)if(same_id(e->metadata.recording_id,v->entries[j].metadata.recording_id))return 0;
 }return 1;
}
static int fp_valid(const struct camera_recording_index_fingerprint *f)
{
 if(!f||f->present>1)return 0;
 if(!f->present)return !f->device&&!f->inode&&!f->bytes&&!f->revision&&!f->crc;
 return f->inode&&f->revision&&f->bytes>=CAMERA_RECORDING_INDEX_HEADER_BYTES&&f->bytes<=CAMERA_RECORDING_INDEX_MAX_BYTES&&
  (f->bytes-CAMERA_RECORDING_INDEX_HEADER_BYTES)%CAMERA_RECORDING_INDEX_ENTRY_BYTES==0;
}
static int fp_same(const struct camera_recording_index_fingerprint *a,const struct camera_recording_index_fingerprint *b)
{return a->present==b->present&&a->device==b->device&&a->inode==b->inode&&a->bytes==b->bytes&&a->revision==b->revision&&a->crc==b->crc;}
static int relation(const struct camera_recording_index_snapshot *,const struct camera_recording_index_scope *);
static int entry_same(const struct camera_recording_index_entry *e,const struct camera_recording_index_entry *f)
{
 const struct camera_recording_metadata *m=&e->metadata,*n=&f->metadata;
 return e->generation==f->generation&&e->instance_generation==f->instance_generation&&same_id(m->recording_id,n->recording_id)&&
  same_id(m->sensor_id,n->sensor_id)&&m->bytes==n->bytes&&m->start_ns==n->start_ns&&m->end_ns==n->end_ns&&m->time_epoch==n->time_epoch&&
  m->origin==n->origin&&m->state==n->state&&m->closed==n->closed&&m->active==n->active&&m->time_domain==n->time_domain;
}
static int snapshot_same(const struct camera_recording_index_snapshot *a,const struct camera_recording_index_snapshot *b)
{
 if(a->revision!=b->revision||a->catalog_revision!=b->catalog_revision||a->generation_highwater!=b->generation_highwater||
    a->serial_highwater!=b->serial_highwater||a->entry_count!=b->entry_count||relation(a,&b->scope)!=CAMERA_INDEX_OK)return 0;
 for(uint32_t i=0;i<a->entry_count;i++){
  if(!entry_same(&a->entries[i],&b->entries[i]))return 0;
 }return 1;
}
static uint32_t crc(const unsigned char *wire,size_t n)
{
 /* CRC-32/BZIP2, checksum bytes 32..35 treated as zero, input never modified. */
 uint32_t table[256],c=0xffffffffU;camera_crc_bzip2_table(table);
 for(size_t i=0;i<n;i++)c=(c<<8)^table[((c>>24)^((i>=32&&i<36)?0:wire[i]))&255];
 return c^0xffffffffU;
}
static int relation(const struct camera_recording_index_snapshot *v,const struct camera_recording_index_scope *q)
{
 int old=0;
 if(!same_id(v->scope.space.storage_id,q->space.storage_id)||v->scope.sensor_count!=q->sensor_count)return CAMERA_INDEX_SCOPE;
 for(uint32_t i=0;i<q->sensor_count;i++)if(!same_id(v->scope.sensors[i],q->sensors[i]))return CAMERA_INDEX_SCOPE;
 if(v->scope.space.instance_generation>q->space.instance_generation)return CAMERA_INDEX_SCOPE;
 if(v->scope.space.storage_generation!=q->space.storage_generation||v->scope.space.instance_generation!=q->space.instance_generation)old=1;
 for(uint32_t i=0;i<v->entry_count;i++)if(v->entries[i].instance_generation!=q->space.instance_generation)old=1;
 return old?CAMERA_INDEX_RECONCILE:CAMERA_INDEX_OK;
}
int camera_recording_index_encode(const struct camera_recording_index_snapshot *v,
 const struct camera_recording_index_fingerprint *base,uint32_t intent,unsigned char *wire,size_t capacity,size_t *used)
{
 size_t n;
 if(!v||!base||!wire||!used||!disjoint(wire,capacity,v,sizeof(*v))||!disjoint(wire,capacity,base,sizeof(*base))||
    !disjoint(used,sizeof(*used),v,sizeof(*v))||!disjoint(used,sizeof(*used),base,sizeof(*base))||
    !disjoint(used,sizeof(*used),wire,capacity)||!snapshot_valid(v)||!fp_valid(base)||
    (intent!=CAMERA_INDEX_NORMAL&&intent!=CAMERA_INDEX_RECONCILED)||(intent==CAMERA_INDEX_RECONCILED&&!base->present)||
    base->revision==UINT64_MAX||v->revision!=base->revision+1)return CAMERA_INDEX_INVALID;
 n=CAMERA_RECORDING_INDEX_HEADER_BYTES+(size_t)v->entry_count*CAMERA_RECORDING_INDEX_ENTRY_BYTES;
 if(capacity<n)return CAMERA_INDEX_INVALID;
 memset(wire,0,n);memcpy(wire,wire_magic,8);put32(wire+8,1);put32(wire+12,512);put32(wire+16,(uint32_t)n);
 put32(wire+20,200);put32(wire+24,v->entry_count);put32(wire+28,v->scope.sensor_count);put32(wire+36,intent);
 put64(wire+40,v->revision);put64(wire+48,v->catalog_revision);put64(wire+56,v->generation_highwater);put64(wire+64,v->serial_highwater);
 put64(wire+72,v->scope.space.storage_generation);put64(wire+80,v->scope.space.instance_generation);
 memcpy(wire+88,v->scope.space.storage_id,CAMERA_ID_BYTES);memcpy(wire+160,v->scope.sensors,sizeof(v->scope.sensors));
 put32(wire+416,base->present);put32(wire+420,base->crc);put64(wire+424,base->device);put64(wire+432,base->inode);
 put64(wire+440,base->bytes);put64(wire+448,base->revision);
 for(uint32_t i=0;i<v->entry_count;i++){
  const struct camera_recording_index_entry *e=&v->entries[i];const struct camera_recording_metadata *m=&e->metadata;
  unsigned char *p=wire+512+(size_t)i*200;
  put64(p,e->generation);put64(p+8,e->instance_generation);memcpy(p+16,m->recording_id,64);memcpy(p+80,m->sensor_id,64);
  put64(p+144,m->bytes);put64(p+152,m->start_ns);put64(p+160,m->end_ns);put64(p+168,m->time_epoch);
  put32(p+176,m->origin);put32(p+180,m->state);put32(p+184,m->closed);put32(p+188,m->active);put32(p+192,m->time_domain);
 }
 put32(wire+32,crc(wire,n));*used=n;return CAMERA_INDEX_OK;
}
int camera_recording_index_decode(const unsigned char *wire,size_t n,const struct camera_recording_index_scope *q,
 struct camera_recording_index_snapshot *v,struct camera_recording_index_fingerprint *base,uint32_t *intent)
{
 struct camera_recording_index_snapshot result;struct camera_recording_index_fingerprint b;uint32_t kind,count;int r;
 if(!wire||!q||!v||!base||!intent||!disjoint(wire,n,v,sizeof(*v))||!disjoint(wire,n,base,sizeof(*base))||
    !disjoint(wire,n,intent,sizeof(*intent))||!disjoint(q,sizeof(*q),v,sizeof(*v))||!disjoint(q,sizeof(*q),base,sizeof(*base))||
    !disjoint(q,sizeof(*q),intent,sizeof(*intent))||!disjoint(v,sizeof(*v),base,sizeof(*base))||
    !disjoint(v,sizeof(*v),intent,sizeof(*intent))||!disjoint(base,sizeof(*base),intent,sizeof(*intent))||!scope_valid(q))return CAMERA_INDEX_INVALID;
 if(n<512||n>CAMERA_RECORDING_INDEX_MAX_BYTES||memcmp(wire,wire_magic,8)||get32(wire+8)!=1||get32(wire+12)!=512||
    get32(wire+16)!=n||get32(wire+20)!=200||!zero(wire+152,8)||!zero(wire+456,56))return CAMERA_INDEX_CORRUPT;
 count=get32(wire+24);kind=get32(wire+36);
 if(count>CAMERA_RECORDING_MAX_ENTRIES||n!=512+(size_t)count*200||(kind!=CAMERA_INDEX_NORMAL&&kind!=CAMERA_INDEX_RECONCILED))return CAMERA_INDEX_CORRUPT;
 memset(&result,0,sizeof(result));memset(&b,0,sizeof(b));result.entry_count=count;result.scope.sensor_count=get32(wire+28);
 result.revision=get64(wire+40);result.catalog_revision=get64(wire+48);result.generation_highwater=get64(wire+56);result.serial_highwater=get64(wire+64);
 result.scope.space.storage_generation=get64(wire+72);result.scope.space.instance_generation=get64(wire+80);
 memcpy(result.scope.space.storage_id,wire+88,64);memcpy(result.scope.sensors,wire+160,sizeof(result.scope.sensors));
 b.present=get32(wire+416);b.crc=get32(wire+420);b.device=get64(wire+424);b.inode=get64(wire+432);b.bytes=get64(wire+440);b.revision=get64(wire+448);
 for(uint32_t i=0;i<count;i++){
  struct camera_recording_index_entry *e=&result.entries[i];struct camera_recording_metadata *m=&e->metadata;
  const unsigned char *p=wire+512+(size_t)i*200;
  if(get32(p+196))return CAMERA_INDEX_CORRUPT;
  e->generation=get64(p);e->instance_generation=get64(p+8);memcpy(m->recording_id,p+16,64);memcpy(m->sensor_id,p+80,64);
  m->abi=CAMERA_RECORDING_ABI;m->struct_bytes=sizeof(*m);m->bytes=get64(p+144);m->start_ns=get64(p+152);m->end_ns=get64(p+160);m->time_epoch=get64(p+168);
  m->origin=get32(p+176);m->state=get32(p+180);m->closed=get32(p+184);m->active=get32(p+188);m->time_domain=get32(p+192);
 }
 if(!snapshot_valid(&result)||!fp_valid(&b)||(kind==CAMERA_INDEX_RECONCILED&&!b.present)||
    b.revision==UINT64_MAX||result.revision!=b.revision+1||crc(wire,n)!=get32(wire+32))return CAMERA_INDEX_CORRUPT;
 r=relation(&result,q);if(r!=CAMERA_INDEX_OK&&r!=CAMERA_INDEX_RECONCILE)return r;
 *v=result;*base=b;*intent=kind;return r;
}
static int ready(const struct camera_recording_index_store *s){return s&&s->initialized==INDEX_MAGIC;}
static int fault(struct camera_recording_index_store *s,uint32_t point){return s->fault&&s->fault(s->fault_context,point);}
static int close_file(struct camera_recording_index_store *s,int fd)
{int injected=fault(s,CAMERA_INDEX_CLOSE);int r=close(fd);return r||injected?CAMERA_INDEX_IO:CAMERA_INDEX_OK;}
static int sync_fd(struct camera_recording_index_store *s,int fd,uint32_t point)
{return fault(s,point)||fsync(fd)?CAMERA_INDEX_IO:CAMERA_INDEX_OK;}
static int safe_file(const struct camera_recording_index_store *s,const struct stat *st)
{return S_ISREG(st->st_mode)&&(uint64_t)st->st_uid==s->owner_uid&&(st->st_mode&07777)==0600&&st->st_nlink==1;}
static int same_inode(const struct stat *a,const struct stat *b)
{return a->st_dev==b->st_dev&&a->st_ino==b->st_ino&&a->st_size==b->st_size&&a->st_uid==b->st_uid&&a->st_mode==b->st_mode&&a->st_nlink==b->st_nlink&&
 a->st_mtim.tv_sec==b->st_mtim.tv_sec&&a->st_mtim.tv_nsec==b->st_mtim.tv_nsec&&a->st_ctim.tv_sec==b->st_ctim.tv_sec&&a->st_ctim.tv_nsec==b->st_ctim.tv_nsec;}
static int directory(struct camera_recording_index_store *s)
{
 struct stat st;DIR *dir;struct dirent *entry;int fd,r=CAMERA_INDEX_OK;unsigned count=0;
 if(fstat(s->dirfd,&st))return CAMERA_INDEX_IO;
 if(!S_ISDIR(st.st_mode)||(uint64_t)st.st_uid!=s->owner_uid||(st.st_mode&07777)!=0700||
    (uint64_t)st.st_dev!=s->directory_device||(uint64_t)st.st_ino!=s->directory_inode)return CAMERA_INDEX_UNSAFE;
 fd=openat(s->dirfd,".",O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);if(fd<0)return CAMERA_INDEX_IO;
 dir=fdopendir(fd);if(!dir){close(fd);return CAMERA_INDEX_IO;}
 errno=0;
 while((entry=readdir(dir))){
  if(!strcmp(entry->d_name,".")||!strcmp(entry->d_name,".."))continue;
  if(++count>2||(strcmp(entry->d_name,CAMERA_RECORDING_INDEX_CURRENT)&&strcmp(entry->d_name,CAMERA_RECORDING_INDEX_PENDING))){r=CAMERA_INDEX_UNSAFE;break;}
 }
 if(!entry&&errno)r=CAMERA_INDEX_IO;
 if(closedir(dir)&&!r)r=CAMERA_INDEX_IO;
 return r;
}
int camera_recording_index_store_init(struct camera_recording_index_store *s,int dirfd,
 const struct camera_recording_index_scope *q,camera_recording_index_fault_fn hook,void *context)
{
 struct stat st;int r;
 if(!s||!q||!outside(s,q,sizeof(*q))||!scope_valid(q)||dirfd<0)return CAMERA_INDEX_INVALID;
 memset(s,0,sizeof(*s));s->dirfd=dirfd;s->scope=*q;s->owner_uid=(uint64_t)geteuid();s->fault=hook;s->fault_context=context;
 if(fstat(dirfd,&st))return CAMERA_INDEX_IO;
 s->directory_device=(uint64_t)st.st_dev;s->directory_inode=(uint64_t)st.st_ino;
 r=directory(s);if(!r)s->initialized=INDEX_MAGIC;return r;
}
static int load(struct camera_recording_index_store *s,const char *name,struct camera_recording_index_file *out)
{
 unsigned char wire[CAMERA_RECORDING_INDEX_MAX_BYTES];struct stat before,opened,after;size_t used=0;int fd,r;
 memset(out,0,sizeof(*out));out->state=CAMERA_INDEX_BAD;
 if(fstatat(s->dirfd,name,&before,AT_SYMLINK_NOFOLLOW)){
  if(errno==ENOENT){out->state=CAMERA_INDEX_ABSENT;return CAMERA_INDEX_OK;}return CAMERA_INDEX_IO;
 }
 if(!safe_file(s,&before))return CAMERA_INDEX_UNSAFE;
 fd=openat(s->dirfd,name,O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC);if(fd<0)return errno==ELOOP?CAMERA_INDEX_UNSAFE:CAMERA_INDEX_IO;
 if(fstat(fd,&opened))r=CAMERA_INDEX_IO;
 else if(!safe_file(s,&opened)||!same_inode(&before,&opened))r=CAMERA_INDEX_UNSAFE;
 else if(opened.st_size<512||opened.st_size>CAMERA_RECORDING_INDEX_MAX_BYTES)r=CAMERA_INDEX_CORRUPT;
 else{
  r=CAMERA_INDEX_OK;
  while(used<(size_t)opened.st_size){
   ssize_t n=read(fd,wire+used,(size_t)opened.st_size-used);
   if(n<0&&errno==EINTR)continue;
   if(n<=0){r=n<0?CAMERA_INDEX_IO:CAMERA_INDEX_CORRUPT;break;}used+=(size_t)n;
  }
  if(!r){unsigned char tail;ssize_t n;do{n=read(fd,&tail,1);}while(n<0&&errno==EINTR);
   if(n)r=n<0?CAMERA_INDEX_IO:CAMERA_INDEX_CORRUPT;
  }
  if(!r&&(fstat(fd,&after)||!same_inode(&opened,&after)))r=CAMERA_INDEX_CONFLICT;
  if(!r)r=camera_recording_index_decode(wire,used,&s->scope,&out->snapshot,&out->base,&out->intent);
  if(r==CAMERA_INDEX_OK||r==CAMERA_INDEX_RECONCILE){
   out->fingerprint.present=1;out->fingerprint.device=(uint64_t)opened.st_dev;out->fingerprint.inode=(uint64_t)opened.st_ino;
   out->fingerprint.bytes=used;out->fingerprint.revision=out->snapshot.revision;out->fingerprint.crc=get32(wire+32);
   out->state=r==CAMERA_INDEX_OK?CAMERA_INDEX_VALID:CAMERA_INDEX_OLD_NAMESPACE;
  }
 }
 if(close_file(s,fd))return CAMERA_INDEX_IO;
 return r;
}
int camera_recording_index_inspect(struct camera_recording_index_store *s,struct camera_recording_index_report *out)
{
 int r;
 if(!ready(s)||!out||!outside(s,out,sizeof(*out)))return CAMERA_INDEX_INVALID;
 memset(out,0,sizeof(*out));r=directory(s);if(r)return r;
 out->current.error=load(s,CAMERA_RECORDING_INDEX_CURRENT,&out->current);
 out->pending.error=load(s,CAMERA_RECORDING_INDEX_PENDING,&out->pending);
 if(out->current.error&&out->current.error!=CAMERA_INDEX_RECONCILE)r=out->current.error;
 else if(out->pending.error&&out->pending.error!=CAMERA_INDEX_RECONCILE)r=out->pending.error;
 else if(out->pending.state!=CAMERA_INDEX_ABSENT){out->outcome=CAMERA_INDEX_PENDING;r=CAMERA_INDEX_RECOVERY_REQUIRED;}
 else if(out->current.error==CAMERA_INDEX_RECONCILE)r=CAMERA_INDEX_RECONCILE;
 else{out->outcome=CAMERA_INDEX_READY;r=CAMERA_INDEX_OK;}
 return r;
}
static int transition(const struct camera_recording_index_file *old,const struct camera_recording_index_file *pending,
 const struct camera_recording_index_scope *q)
{
 const struct camera_recording_index_snapshot *a=&old->snapshot,*b=&pending->snapshot;
 if(pending->state!=CAMERA_INDEX_VALID||!fp_same(&old->fingerprint,&pending->base))return CAMERA_INDEX_CONFLICT;
 if(!old->fingerprint.present)return pending->intent==CAMERA_INDEX_NORMAL?CAMERA_INDEX_OK:CAMERA_INDEX_CONFLICT;
 if(a->revision==UINT64_MAX)return CAMERA_INDEX_EXHAUSTED;
 if(pending->intent==CAMERA_INDEX_NORMAL&&old->state!=CAMERA_INDEX_VALID)return CAMERA_INDEX_RECONCILE;
 if(b->revision!=a->revision+1||b->catalog_revision<a->catalog_revision||b->generation_highwater<a->generation_highwater||b->serial_highwater<a->serial_highwater)return CAMERA_INDEX_CONFLICT;
 for(uint32_t i=0;i<b->entry_count;i++){
  int exact=0;
  for(uint32_t j=0;j<a->entry_count;j++){
   if(same_id(b->entries[i].metadata.recording_id,a->entries[j].metadata.recording_id)&&
      (!same_id(b->entries[i].metadata.sensor_id,a->entries[j].metadata.sensor_id)||
       b->entries[i].metadata.origin!=a->entries[j].metadata.origin))return CAMERA_INDEX_CONFLICT;
   if(entry_same(&b->entries[i],&a->entries[j]))exact=1;
  }
  if(pending->intent==CAMERA_INDEX_NORMAL&&b->entries[i].generation<=a->generation_highwater&&!exact)return CAMERA_INDEX_CONFLICT;
 }
 if(pending->intent==CAMERA_INDEX_NORMAL)return old->state==CAMERA_INDEX_VALID?CAMERA_INDEX_OK:CAMERA_INDEX_RECONCILE;
 if(a->scope.space.storage_generation!=q->space.storage_generation||a->scope.space.instance_generation>=q->space.instance_generation)return CAMERA_INDEX_RECONCILE;
 for(uint32_t i=0;i<b->entry_count;i++)if(b->entries[i].generation<=a->generation_highwater)return CAMERA_INDEX_CONFLICT;
 return CAMERA_INDEX_OK;
}
static int promote(struct camera_recording_index_store *s,const struct camera_recording_index_fingerprint *current,
 const struct camera_recording_index_fingerprint *pending,struct camera_recording_index_report *out)
{
 /* Internal commit may pass a fingerprint in out; freeze both before inspect
  * overwrites caller-owned scratch. Avoid stacked full report copies. */
 struct camera_recording_index_fingerprint want_current=*current,want_pending=*pending;
 struct stat st;int fd,r;uint32_t created=out->stage_created;
 current=&want_current;pending=&want_pending;
 r=camera_recording_index_inspect(s,out);out->stage_created=created;
 if(r!=CAMERA_INDEX_RECOVERY_REQUIRED)return r;
 if(!fp_same(current,&out->current.fingerprint)||!fp_same(pending,&out->pending.fingerprint))return CAMERA_INDEX_CONFLICT;
 r=transition(&out->current,&out->pending,&s->scope);if(r)return r;
 fd=openat(s->dirfd,CAMERA_RECORDING_INDEX_PENDING,O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC);if(fd<0)return CAMERA_INDEX_UNCERTAIN;
 r=fstat(fd,&st)||!safe_file(s,&st)||(uint64_t)st.st_dev!=pending->device||(uint64_t)st.st_ino!=pending->inode||
  (uint64_t)st.st_size!=pending->bytes?CAMERA_INDEX_UNSAFE:sync_fd(s,fd,CAMERA_INDEX_FILE_SYNC);
 if(close_file(s,fd)&&!r)r=CAMERA_INDEX_IO;
 if(r)return CAMERA_INDEX_UNCERTAIN;
 if(sync_fd(s,s->dirfd,CAMERA_INDEX_STAGE_DIR_SYNC))return CAMERA_INDEX_UNCERTAIN;
 /* Re-read both exact fingerprints and namespace/ownership readiness immediately
  * before replacement, while caller holds exclusive cross-process ownership. */
 r=camera_recording_index_inspect(s,out);out->stage_created=created;
 if(r!=CAMERA_INDEX_RECOVERY_REQUIRED||!fp_same(current,&out->current.fingerprint)||
    !fp_same(pending,&out->pending.fingerprint)||transition(&out->current,&out->pending,&s->scope))return CAMERA_INDEX_CONFLICT;
 if(fault(s,CAMERA_INDEX_RENAME)||renameat(s->dirfd,CAMERA_RECORDING_INDEX_PENDING,s->dirfd,CAMERA_RECORDING_INDEX_CURRENT))return CAMERA_INDEX_UNCERTAIN;
 out->renamed=1;out->outcome=CAMERA_INDEX_RENAMED_UNCONFIRMED;
 if(sync_fd(s,s->dirfd,CAMERA_INDEX_COMMIT_DIR_SYNC))return CAMERA_INDEX_UNCERTAIN;
 r=camera_recording_index_inspect(s,out);out->renamed=1;out->stage_created=created;out->outcome=CAMERA_INDEX_RENAMED_UNCONFIRMED;
 if(r||!fp_same(pending,&out->current.fingerprint)||out->pending.state!=CAMERA_INDEX_ABSENT)return CAMERA_INDEX_UNCERTAIN;
 out->outcome=CAMERA_INDEX_COMMITTED;out->sync_confirmed=1;return CAMERA_INDEX_OK;
}
static void refresh(struct camera_recording_index_store *s,struct camera_recording_index_report *out)
{
 uint32_t created=out->stage_created,renamed=out->renamed;
 (void)camera_recording_index_inspect(s,out);out->stage_created=created;out->renamed=renamed;
 if(renamed)out->outcome=CAMERA_INDEX_RENAMED_UNCONFIRMED;
}
int camera_recording_index_commit(struct camera_recording_index_store *s,
 const struct camera_recording_index_fingerprint *expected,const struct camera_recording_index_snapshot *next,
 uint32_t intent,struct camera_recording_index_report *out)
{
 struct camera_recording_index_file candidate;
 unsigned char wire[CAMERA_RECORDING_INDEX_MAX_BYTES];struct stat st;size_t n=0,used=0;int fd,r,error=0;
 if(!ready(s)||!expected||!next||!out||!outside(s,expected,sizeof(*expected))||!outside(s,next,sizeof(*next))||
    !outside(s,out,sizeof(*out))||!disjoint(out,sizeof(*out),expected,sizeof(*expected))||!disjoint(out,sizeof(*out),next,sizeof(*next)))return CAMERA_INDEX_INVALID;
 if(fp_valid(expected)&&expected->revision==UINT64_MAX)return CAMERA_INDEX_EXHAUSTED;
 r=camera_recording_index_encode(next,expected,intent,wire,sizeof(wire),&n);if(r)return r;
 if(relation(next,&s->scope)!=CAMERA_INDEX_OK)return CAMERA_INDEX_RECONCILE;
 r=camera_recording_index_inspect(s,out);
 if(r!=CAMERA_INDEX_OK&&r!=CAMERA_INDEX_RECONCILE)return r;
 if(!fp_same(expected,&out->current.fingerprint))return CAMERA_INDEX_CONFLICT;
 memset(&candidate,0,sizeof(candidate));candidate.state=CAMERA_INDEX_VALID;candidate.snapshot=*next;candidate.base=*expected;candidate.intent=intent;
 r=transition(&out->current,&candidate,&s->scope);if(r)return r;
 if(fault(s,CAMERA_INDEX_STAGE_OPEN))return CAMERA_INDEX_IO;
 fd=openat(s->dirfd,CAMERA_RECORDING_INDEX_PENDING,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);
 if(fd<0)return errno==EEXIST?CAMERA_INDEX_RECOVERY_REQUIRED:CAMERA_INDEX_IO;
 out->stage_created=1;
 if(fstat(fd,&st)||!safe_file(s,&st))error=1;
 while(!error&&used<n){
  size_t chunk=n-used;ssize_t wrote;if(chunk>256)chunk=256;
  if(fault(s,CAMERA_INDEX_WRITE)){error=1;break;}
  wrote=write(fd,wire+used,chunk);if(wrote<0&&errno==EINTR)continue;
  if(wrote<=0){error=1;break;}used+=(size_t)wrote;
 }
 if(!error&&sync_fd(s,fd,CAMERA_INDEX_FILE_SYNC))error=1;
 if(close_file(s,fd))error=1;
 if(error){refresh(s,out);return CAMERA_INDEX_UNCERTAIN;}
 r=camera_recording_index_inspect(s,out);out->stage_created=1;
 if(r!=CAMERA_INDEX_RECOVERY_REQUIRED||out->pending.state!=CAMERA_INDEX_VALID||out->pending.fingerprint.bytes!=n||
    out->pending.fingerprint.crc!=get32(wire+32)||out->pending.fingerprint.device!=(uint64_t)st.st_dev||
    out->pending.fingerprint.inode!=(uint64_t)st.st_ino||!fp_same(expected,&out->current.fingerprint)||
    !fp_same(expected,&out->pending.base)||out->pending.intent!=intent||!snapshot_same(next,&out->pending.snapshot)){refresh(s,out);return CAMERA_INDEX_UNCERTAIN;}
 r=promote(s,expected,&out->pending.fingerprint,out);if(r)refresh(s,out);return r;
}
int camera_recording_index_resume(struct camera_recording_index_store *s,
 const struct camera_recording_index_fingerprint *current,const struct camera_recording_index_fingerprint *pending,
 struct camera_recording_index_report *out)
{
 int r;
 if(!ready(s)||!current||!pending||!out||!outside(s,current,sizeof(*current))||!outside(s,pending,sizeof(*pending))||
    !outside(s,out,sizeof(*out))||!disjoint(out,sizeof(*out),current,sizeof(*current))||!disjoint(out,sizeof(*out),pending,sizeof(*pending)))return CAMERA_INDEX_INVALID;
 r=camera_recording_index_inspect(s,out);
 if(r!=CAMERA_INDEX_RECOVERY_REQUIRED)return r==CAMERA_INDEX_OK?CAMERA_INDEX_CONFLICT:r;
 r=promote(s,current,pending,out);if(r)refresh(s,out);return r;
}
