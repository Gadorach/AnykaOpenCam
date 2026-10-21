/* SPDX-License-Identifier: GPL-2.0-only; public synthetic host metadata ONLY. */
#define _POSIX_C_SOURCE 200809L
#include <camera/recording_index_store.h>
#include <camera/dma_integrity.h>
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static unsigned checks,suites,cases;
static int rootfd;
#define CHECK(x) do{checks++;assert(x);}while(0)
struct hook {uint32_t point,nth,seen,trace[1024],count;};
static int fail(void *context,uint32_t point)
{
 struct hook *h=context;
 if(h->count<1024)h->trace[h->count++]=point;
 return point==h->point&&++h->seen==h->nth;
}
static struct camera_recording_index_scope scope(uint64_t owner)
{
 struct camera_recording_index_scope q;memset(&q,0,sizeof(q));
 memcpy(q.space.storage_id,"public-storage",15);q.space.storage_generation=1;q.space.instance_generation=owner;
 q.sensor_count=2;memcpy(q.sensors[0],"sensor-a",9);memcpy(q.sensors[1],"sensor-b",9);return q;
}
static struct camera_recording_index_snapshot snapshot(uint64_t owner,uint32_t count)
{
 struct camera_recording_index_snapshot v;memset(&v,0,sizeof(v));v.scope=scope(owner);
 v.revision=1;v.catalog_revision=10;v.generation_highwater=count;v.serial_highwater=20;v.entry_count=count;
 for(uint32_t i=0;i<count&&i<CAMERA_RECORDING_MAX_ENTRIES;i++){
  struct camera_recording_index_entry *e=&v.entries[i];struct camera_recording_metadata *m=&e->metadata;
  m->abi=CAMERA_RECORDING_ABI;m->struct_bytes=sizeof(*m);snprintf(m->recording_id,sizeof(m->recording_id),"public-%u",i);
  memcpy(m->sensor_id,v.scope.sensors[i%2],64);m->bytes=100+i;m->origin=CAMERA_RECORDING_SERVICE_OWNED;
  m->state=CAMERA_RECORDING_COMPLETE;m->closed=1;e->generation=i+1;e->instance_generation=owner;
 }return v;
}
static int new_directory(void)
{
 char name[32];int fd;snprintf(name,sizeof(name),"case-%03u",cases++);
 CHECK(mkdirat(rootfd,name,0700)==0);fd=openat(rootfd,name,O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);
 CHECK(fd>=0);return fd;
}
static void init(struct camera_recording_index_store *s,int fd,struct hook *h,uint64_t owner)
{
 struct camera_recording_index_scope q=scope(owner);
 CHECK(camera_recording_index_store_init(s,fd,&q,h?fail:NULL,h)==CAMERA_INDEX_OK);
}
static size_t encode(const struct camera_recording_index_snapshot *v,unsigned char *wire)
{
 struct camera_recording_index_fingerprint base={0};size_t n=0;
 CHECK(camera_recording_index_encode(v,&base,CAMERA_INDEX_NORMAL,wire,CAMERA_RECORDING_INDEX_MAX_BYTES,&n)==CAMERA_INDEX_OK);return n;
}
static void store_wire(int fd,const char *name,const unsigned char *wire,size_t n)
{
 int file=openat(fd,name,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);size_t used=0;
 CHECK(file>=0);
 while(used<n){ssize_t wrote=write(file,wire+used,n-used);CHECK(wrote>0);used+=(size_t)wrote;}
 CHECK(fsync(file)==0);CHECK(close(file)==0);
}
static size_t read_wire(int fd,const char *name,unsigned char *wire)
{
 int file=openat(fd,name,O_RDONLY|O_NOFOLLOW|O_CLOEXEC);struct stat st;size_t n=0;
 CHECK(file>=0);CHECK(fstat(file,&st)==0&&S_ISREG(st.st_mode)&&st.st_size<=CAMERA_RECORDING_INDEX_MAX_BYTES);
 while(n<(size_t)st.st_size){ssize_t got=read(file,wire+n,(size_t)st.st_size-n);CHECK(got>0);n+=(size_t)got;}
 CHECK(close(file)==0);return n;
}
static void put32(unsigned char *p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=(unsigned char)(v>>(8*i));}
static void put64(unsigned char *p,uint64_t v){put32(p,(uint32_t)v);put32(p+4,(uint32_t)(v>>32));}
static void fix_crc(unsigned char *p,size_t n)
{uint32_t table[256];put32(p+32,0);camera_crc_bzip2_table(table);put32(p+32,camera_crc_bzip2_ram(p,n,table));}
static void codec_roundtrip(void)
{
 struct camera_recording_index_snapshot v=snapshot(1,64),decoded;
 struct camera_recording_index_scope q=scope(1);struct camera_recording_index_fingerprint base;
 unsigned char wire[CAMERA_RECORDING_INDEX_MAX_BYTES],again[CAMERA_RECORDING_INDEX_MAX_BYTES];size_t n,second;uint32_t intent;
 v.entries[1].metadata.origin=CAMERA_RECORDING_STOCK;
 v.entries[2].metadata.state=CAMERA_RECORDING_INCOMPLETE;v.entries[2].metadata.active=1;
 v.entries[3].metadata.state=CAMERA_RECORDING_CORRUPT;
 v.entries[4].metadata.time_domain=CAMERA_RECORDING_TIME_MONOTONIC;v.entries[4].metadata.time_epoch=44;
 v.entries[4].metadata.start_ns=1;v.entries[4].metadata.end_ns=99;
 v.entries[5].metadata.time_domain=CAMERA_RECORDING_TIME_QUALIFIED_UTC;v.entries[5].metadata.time_epoch=7;
 v.entries[5].metadata.start_ns=1;v.entries[5].metadata.end_ns=UINT64_C(0x123456789abcdef0);
 n=encode(&v,wire);CHECK(n==CAMERA_RECORDING_INDEX_MAX_BYTES&&n==13312);
 CHECK(!memcmp(wire,"RECIDX01",8)&&wire[8]==1&&wire[12]==0&&wire[13]==2&&wire[20]==200);
 CHECK(camera_recording_index_decode(wire,n,&q,&decoded,&base,&intent)==CAMERA_INDEX_OK);
 CHECK(decoded.entry_count==64&&decoded.entries[5].metadata.end_ns==UINT64_C(0x123456789abcdef0));
 CHECK(decoded.entries[1].metadata.origin==CAMERA_RECORDING_STOCK&&decoded.entries[2].metadata.active==1);
 CHECK(camera_recording_index_encode(&decoded,&base,intent,again,sizeof(again),&second)==CAMERA_INDEX_OK);
 CHECK(n==second&&!memcmp(wire,again,n));
 {const unsigned char le[8]={0xf0,0xde,0xbc,0x9a,0x78,0x56,0x34,0x12};
  CHECK(!memcmp(wire+512+5*200+160,le,8));}
 {uint32_t table[256];camera_crc_bzip2_table(table);
  CHECK(camera_crc_bzip2_ram("123456789",9,table)==0xfc891918U);}
 v=snapshot(1,0);n=encode(&v,wire);CHECK(n==512);
 CHECK(camera_recording_index_decode(wire,n,&q,&decoded,&base,&intent)==CAMERA_INDEX_OK&&decoded.entry_count==0);suites++;
}
static void codec_denials(void)
{
 struct camera_recording_index_snapshot v=snapshot(1,2),out,saved;
 struct camera_recording_index_scope q=scope(1);struct camera_recording_index_fingerprint base={0},b,saved_base;
 unsigned char wire[CAMERA_RECORDING_INDEX_MAX_BYTES],bad[CAMERA_RECORDING_INDEX_MAX_BYTES+1];size_t n=encode(&v,wire),used=777;
 uint32_t intent=999,saved_intent=999;
 memset(&out,0xa5,sizeof(out));memcpy(&saved,&out,sizeof(out));memset(&b,0xa5,sizeof(b));memcpy(&saved_base,&b,sizeof(b));
 for(size_t i=0;i<n;i++){
  CHECK(camera_recording_index_decode(wire,i,&q,&out,&b,&intent)==CAMERA_INDEX_CORRUPT);
  CHECK(!memcmp(&out,&saved,sizeof(out))&&!memcmp(&b,&saved_base,sizeof(b))&&intent==saved_intent);
 }
 for(unsigned i=0;i<32;i++){
  memcpy(bad,wire,n);
  switch(i){
  case 0:bad[0]^=1;break;case 1:put32(bad+8,2);break;case 2:put32(bad+12,511);break;
  case 3:put32(bad+16,UINT32_MAX);break;case 4:put32(bad+20,201);break;case 5:put32(bad+24,65);break;
  case 6:put32(bad+28,0);break;case 7:put32(bad+36,3);break;case 8:put64(bad+40,0);break;
  case 9:put64(bad+48,0);break;case 10:put64(bad+56,1);break;case 11:put64(bad+72,0);break;
  case 12:put64(bad+80,0);break;case 13:bad[88]='/';break;case 14:bad[103]='x';break;
  case 15:bad[152]=1;break;case 16:bad[160+2*64]=1;break;case 17:put32(bad+416,2);break;
  case 18:put64(bad+448,UINT64_MAX);break;case 19:bad[456]=1;break;
  case 20:put64(bad+512,0);break;case 21:put64(bad+512+200,1);break;
  case 22:put64(bad+512+8,2);break;case 23:memcpy(bad+512+200+16,bad+512+16,64);break;
  case 24:bad[512+80]='?';break;case 25:put64(bad+512+144,UINT64_MAX);break;
  case 26:put64(bad+512+160,1);break;case 27:put32(bad+512+176,0);break;
  case 28:put32(bad+512+180,CAMERA_RECORDING_OPEN);break;case 29:put32(bad+512+188,1);break;
  case 30:put32(bad+512+192,3);break;case 31:bad[512+196]=1;break;
  }
  fix_crc(bad,n);CHECK(camera_recording_index_decode(bad,n,&q,&out,&b,&intent)==CAMERA_INDEX_CORRUPT);
  CHECK(!memcmp(&out,&saved,sizeof(out))&&!memcmp(&b,&saved_base,sizeof(b))&&intent==saved_intent);
 }
 memcpy(bad,wire,n);bad[512+144]^=1;
 CHECK(camera_recording_index_decode(bad,n,&q,&out,&b,&intent)==CAMERA_INDEX_CORRUPT);
 memcpy(bad,wire,n);bad[n]=0;
 CHECK(camera_recording_index_decode(bad,n+1,&q,&out,&b,&intent)==CAMERA_INDEX_CORRUPT);
 q.sensors[0][0]='X';CHECK(camera_recording_index_decode(wire,n,&q,&out,&b,&intent)==CAMERA_INDEX_SCOPE);
 q=scope(1);q.space.storage_id[0]='X';CHECK(camera_recording_index_decode(wire,n,&q,&out,&b,&intent)==CAMERA_INDEX_SCOPE);
 q=scope(2);CHECK(camera_recording_index_decode(wire,n,&q,&out,&b,&intent)==CAMERA_INDEX_RECONCILE);
 q=scope(1);q.space.storage_generation=2;CHECK(camera_recording_index_decode(wire,n,&q,&out,&b,&intent)==CAMERA_INDEX_RECONCILE);
 v=snapshot(2,2);n=encode(&v,wire);q=scope(1);
 CHECK(camera_recording_index_decode(wire,n,&q,&out,&b,&intent)==CAMERA_INDEX_SCOPE);
 v=snapshot(1,2);v.entries[0].instance_generation=0;n=encode(&v,wire);
 CHECK(camera_recording_index_decode(wire,n,&q,&out,&b,&intent)==CAMERA_INDEX_RECONCILE);
 v=snapshot(1,2);v.entries[0].metadata.time_domain=CAMERA_RECORDING_TIME_MONOTONIC;v.entries[0].metadata.time_epoch=1;
 v.entries[0].metadata.start_ns=2;v.entries[0].metadata.end_ns=1;
 CHECK(camera_recording_index_encode(&v,&base,1,wire,sizeof(wire),&used)==CAMERA_INDEX_INVALID&&used==777);
 v=snapshot(1,65);CHECK(camera_recording_index_encode(&v,&base,1,wire,sizeof(wire),&used)==CAMERA_INDEX_INVALID);
 v=snapshot(1,2);CHECK(camera_recording_index_encode(&v,&base,1,wire,511,&used)==CAMERA_INDEX_INVALID);suites++;
}
static void aliases(void)
{
 struct camera_recording_index_snapshot v=snapshot(1,2),out;
 struct camera_recording_index_scope q=scope(1);struct camera_recording_index_fingerprint base={0},b;
 union {struct camera_recording_index_snapshot snapshot;unsigned char wire[sizeof(struct camera_recording_index_snapshot)];} shared,saved;
 struct camera_recording_index_store s,before;struct camera_recording_index_report report;
 unsigned char wire[CAMERA_RECORDING_INDEX_MAX_BYTES];size_t n=encode(&v,wire),used=99;uint32_t intent;int fd=new_directory();
 shared.snapshot=v;memcpy(&saved,&shared,sizeof(shared));
 CHECK(camera_recording_index_encode(&shared.snapshot,&base,1,shared.wire,sizeof(shared.wire),&used)==CAMERA_INDEX_INVALID);
 CHECK(!memcmp(&shared,&saved,sizeof(shared))&&used==99);
 memcpy(shared.wire,wire,n);memcpy(&saved,&shared,sizeof(shared));
 CHECK(camera_recording_index_decode(shared.wire,n,&q,&shared.snapshot,&b,&intent)==CAMERA_INDEX_INVALID);
 CHECK(!memcmp(&shared,&saved,sizeof(shared)));
 CHECK(camera_recording_index_decode(wire,n,&q,&out,&b,(uint32_t *)&b)==CAMERA_INDEX_INVALID);
 CHECK(camera_recording_index_encode(&v,&base,1,wire,sizeof(wire),(size_t *)wire)==CAMERA_INDEX_INVALID);
 init(&s,fd,NULL,1);memcpy(&before,&s,sizeof(s));
 CHECK(camera_recording_index_store_init(&s,fd,&s.scope,NULL,NULL)==CAMERA_INDEX_INVALID);
 CHECK(!memcmp(&s,&before,sizeof(s)));
 CHECK(camera_recording_index_inspect(&s,(struct camera_recording_index_report *)&s)==CAMERA_INDEX_INVALID);
 CHECK(camera_recording_index_commit(&s,&base,&v,1,(struct camera_recording_index_report *)&s)==CAMERA_INDEX_INVALID);
 CHECK(camera_recording_index_commit(&s,&base,&v,1,(struct camera_recording_index_report *)&v)==CAMERA_INDEX_INVALID);
 CHECK(camera_recording_index_resume(&s,&base,&base,(struct camera_recording_index_report *)&s)==CAMERA_INDEX_INVALID);
 CHECK(camera_recording_index_inspect(&s,&report)==CAMERA_INDEX_OK&&report.current.state==CAMERA_INDEX_ABSENT);
 CHECK(!memcmp(&s,&before,sizeof(s)));CHECK(close(fd)==0);suites++;
}
static void transactions(void)
{
 struct camera_recording_index_store s;struct camera_recording_index_report r,observed;
 struct camera_recording_index_snapshot v=snapshot(1,2);struct camera_recording_index_fingerprint absent={0},fp;
 struct hook h={0};int fd=new_directory();unsigned file_sync=0,stage_sync=0,rename=0,dir_sync=0;
 init(&s,fd,&h,1);
 CHECK(camera_recording_index_commit(&s,&absent,&v,1,&r)==CAMERA_INDEX_OK&&r.sync_confirmed&&r.renamed&&r.outcome==CAMERA_INDEX_COMMITTED);
 CHECK(r.current.snapshot.entry_count==2&&r.pending.state==CAMERA_INDEX_ABSENT);fp=r.current.fingerprint;
 for(unsigned i=0;i<h.count;i++){
  if(h.trace[i]==CAMERA_INDEX_FILE_SYNC)file_sync++;
  if(h.trace[i]==CAMERA_INDEX_STAGE_DIR_SYNC){CHECK(file_sync>=2);stage_sync++;}
  if(h.trace[i]==CAMERA_INDEX_RENAME){CHECK(stage_sync==1);rename++;}
  if(h.trace[i]==CAMERA_INDEX_COMMIT_DIR_SYNC){CHECK(rename==1);dir_sync++;}
 }
 CHECK(file_sync==2&&stage_sync==1&&rename==1&&dir_sync==1);
 CHECK(camera_recording_index_inspect(&s,&observed)==CAMERA_INDEX_OK&&!observed.sync_confirmed);
 CHECK(camera_recording_index_resume(&s,&absent,&fp,&r)==CAMERA_INDEX_CONFLICT&&!r.sync_confirmed);
 v.revision=2;v.catalog_revision++;
 CHECK(camera_recording_index_commit(&s,&fp,&v,1,&r)==CAMERA_INDEX_OK&&r.current.snapshot.revision==2);
 CHECK(camera_recording_index_commit(&s,&fp,&v,1,&r)==CAMERA_INDEX_CONFLICT);
 fp=observed.current.fingerprint;v.revision=3;
 CHECK(camera_recording_index_commit(&s,&fp,&v,1,&r)==CAMERA_INDEX_INVALID); /* stale base revision cannot encode skip */
 CHECK(close(fd)==0);suites++;
}
static void fault_matrix(void)
{
 const uint32_t points[]={CAMERA_INDEX_STAGE_OPEN,CAMERA_INDEX_WRITE,CAMERA_INDEX_FILE_SYNC,CAMERA_INDEX_STAGE_DIR_SYNC,CAMERA_INDEX_RENAME,CAMERA_INDEX_COMMIT_DIR_SYNC,CAMERA_INDEX_CLOSE};
 for(unsigned k=0;k<sizeof(points)/sizeof(points[0]);k++){
  int fd=new_directory();struct hook h={0};struct camera_recording_index_store s;
  struct camera_recording_index_snapshot v=snapshot(1,2);struct camera_recording_index_report r,inspection;
  struct camera_recording_index_fingerprint absent={0},fp,pending;
  unsigned char old[CAMERA_RECORDING_INDEX_MAX_BYTES],now[CAMERA_RECORDING_INDEX_MAX_BYTES];size_t old_n,new_n;int result;
  init(&s,fd,&h,1);CHECK(camera_recording_index_commit(&s,&absent,&v,1,&r)==CAMERA_INDEX_OK);fp=r.current.fingerprint;
  old_n=read_wire(fd,CAMERA_RECORDING_INDEX_CURRENT,old);v.revision=2;v.catalog_revision++;
  memset(&h,0,sizeof(h));h.point=points[k];h.nth=(h.point==CAMERA_INDEX_WRITE||h.point==CAMERA_INDEX_CLOSE)?2:1;
  result=camera_recording_index_commit(&s,&fp,&v,1,&r);
  CHECK(result==(points[k]==CAMERA_INDEX_STAGE_OPEN?CAMERA_INDEX_IO:CAMERA_INDEX_UNCERTAIN));
  CHECK(!r.sync_confirmed);h.point=0;
  new_n=read_wire(fd,CAMERA_RECORDING_INDEX_CURRENT,now);
  if(points[k]==CAMERA_INDEX_COMMIT_DIR_SYNC){
   CHECK(r.renamed&&r.outcome==CAMERA_INDEX_RENAMED_UNCONFIRMED);
   CHECK(camera_recording_index_inspect(&s,&inspection)==CAMERA_INDEX_OK&&inspection.current.snapshot.revision==2&&!inspection.sync_confirmed);
  }else{
   CHECK(old_n==new_n&&!memcmp(old,now,old_n)&&!r.renamed);
   result=camera_recording_index_inspect(&s,&inspection);
   if(points[k]==CAMERA_INDEX_STAGE_OPEN)CHECK(result==CAMERA_INDEX_OK&&inspection.pending.state==CAMERA_INDEX_ABSENT);
   else if(points[k]==CAMERA_INDEX_WRITE){
    CHECK(result==CAMERA_INDEX_CORRUPT&&inspection.pending.state==CAMERA_INDEX_BAD);
    CHECK(camera_recording_index_resume(&s,&fp,&inspection.pending.fingerprint,&r)==CAMERA_INDEX_CORRUPT);
   }else{
    CHECK(result==CAMERA_INDEX_RECOVERY_REQUIRED&&inspection.pending.state==CAMERA_INDEX_VALID);pending=inspection.pending.fingerprint;
    CHECK(camera_recording_index_commit(&s,&fp,&v,1,&r)==CAMERA_INDEX_RECOVERY_REQUIRED);
    CHECK(camera_recording_index_resume(&s,&fp,&pending,&r)==CAMERA_INDEX_OK&&r.sync_confirmed&&r.renamed&&!r.stage_created);
   }
  }
  CHECK(close(fd)==0);
 }suites++;
}
static void namespaces(void)
{
 int fd=new_directory();struct camera_recording_index_store s,fresh;
 struct camera_recording_index_snapshot old=snapshot(1,2),next=snapshot(2,2);
 struct camera_recording_index_fingerprint absent={0},fp;struct camera_recording_index_report r;
 init(&s,fd,NULL,1);CHECK(camera_recording_index_commit(&s,&absent,&old,1,&r)==CAMERA_INDEX_OK);fp=r.current.fingerprint;
 init(&fresh,fd,NULL,2);CHECK(camera_recording_index_inspect(&fresh,&r)==CAMERA_INDEX_RECONCILE);
 CHECK(r.current.state==CAMERA_INDEX_OLD_NAMESPACE&&!r.sync_confirmed);
 next.revision=2;
 CHECK(camera_recording_index_commit(&fresh,&fp,&next,1,&r)==CAMERA_INDEX_RECONCILE);
 CHECK(camera_recording_index_commit(&fresh,&fp,&next,2,&r)==CAMERA_INDEX_CONFLICT); /* no generation reissue */
 next.entries[0].generation=3;next.entries[1].generation=4;next.generation_highwater=4;
 CHECK(camera_recording_index_commit(&fresh,&fp,&next,2,&r)==CAMERA_INDEX_OK&&r.current.snapshot.scope.space.instance_generation==2);
 fp=r.current.fingerprint;next.revision=3;next.serial_highwater=19;
 CHECK(camera_recording_index_commit(&fresh,&fp,&next,1,&r)==CAMERA_INDEX_CONFLICT);
 next.serial_highwater=20;next.catalog_revision=9;
 CHECK(camera_recording_index_commit(&fresh,&fp,&next,1,&r)==CAMERA_INDEX_CONFLICT);
 next.catalog_revision=10;next.entries[0].instance_generation=0;
 CHECK(camera_recording_index_commit(&fresh,&fp,&next,1,&r)==CAMERA_INDEX_RECONCILE);
 CHECK(close(fd)==0);
 fd=new_directory();unsigned char wire[CAMERA_RECORDING_INDEX_MAX_BYTES];size_t n=encode(&old,wire);
 store_wire(fd,CAMERA_RECORDING_INDEX_CURRENT,wire,n);
 struct camera_recording_index_scope q=scope(2);q.space.storage_generation=2;
 CHECK(camera_recording_index_store_init(&fresh,fd,&q,NULL,NULL)==CAMERA_INDEX_OK);
 CHECK(camera_recording_index_inspect(&fresh,&r)==CAMERA_INDEX_RECONCILE);fp=r.current.fingerprint;
 next=snapshot(2,0);next.scope=q;next.revision=2;next.generation_highwater=old.generation_highwater;
 CHECK(camera_recording_index_commit(&fresh,&fp,&next,2,&r)==CAMERA_INDEX_RECONCILE);
 CHECK(close(fd)==0);suites++;
}
static void preserve_denials(void)
{
 unsigned char wire[CAMERA_RECORDING_INDEX_MAX_BYTES],observed[CAMERA_RECORDING_INDEX_MAX_BYTES];
 struct camera_recording_index_snapshot v=snapshot(1,2);size_t n=encode(&v,wire);
 struct camera_recording_index_store s;struct camera_recording_index_report r;
 struct camera_recording_index_fingerprint absent={0};int fd;
 for(unsigned kind=0;kind<6;kind++){
  fd=new_directory();init(&s,fd,NULL,1);
  if(kind==0){wire[32]^=1;store_wire(fd,CAMERA_RECORDING_INDEX_CURRENT,wire,n);}
  if(kind==1)store_wire(fd,CAMERA_RECORDING_INDEX_PENDING,wire,256);
  if(kind==2){store_wire(fd,CAMERA_RECORDING_INDEX_CURRENT,wire,n);CHECK(fchmodat(fd,CAMERA_RECORDING_INDEX_CURRENT,0644,0)==0);}
  if(kind==3)CHECK(symlinkat("../../public-only-missing",fd,CAMERA_RECORDING_INDEX_CURRENT)==0);
  if(kind==4)CHECK(mkfifoat(fd,CAMERA_RECORDING_INDEX_CURRENT,0600)==0);
  if(kind==5){store_wire(fd,CAMERA_RECORDING_INDEX_CURRENT,wire,n);char name[32];snprintf(name,sizeof(name),"public-link-%u",cases);CHECK(linkat(fd,CAMERA_RECORDING_INDEX_CURRENT,rootfd,name,0)==0);}
  int expected=kind<2?CAMERA_INDEX_CORRUPT:CAMERA_INDEX_UNSAFE;
  CHECK(camera_recording_index_inspect(&s,&r)==expected&&!r.sync_confirmed);
  CHECK(camera_recording_index_commit(&s,&absent,&v,1,&r)==expected);
  struct stat st;CHECK(fstatat(fd,kind==1?CAMERA_RECORDING_INDEX_PENDING:CAMERA_RECORDING_INDEX_CURRENT,&st,AT_SYMLINK_NOFOLLOW)==0);
  if(kind==0)CHECK(read_wire(fd,CAMERA_RECORDING_INDEX_CURRENT,observed)==n&&!memcmp(wire,observed,n));
  CHECK(close(fd)==0);if(kind==0)wire[32]^=1;
 }
 fd=new_directory();store_wire(fd,"foreign-public-file",wire,n);struct camera_recording_index_scope q=scope(1);
 CHECK(camera_recording_index_store_init(&s,fd,&q,NULL,NULL)==CAMERA_INDEX_UNSAFE);
 CHECK(read_wire(fd,"foreign-public-file",observed)==n&&!memcmp(wire,observed,n));CHECK(close(fd)==0);
 fd=new_directory();init(&s,fd,NULL,1);CHECK(fchmod(fd,0755)==0);
 CHECK(camera_recording_index_inspect(&s,&r)==CAMERA_INDEX_UNSAFE);CHECK(close(fd)==0);suites++;
}
static void inode_cas(void)
{
 int fd=new_directory();struct camera_recording_index_store s;struct camera_recording_index_snapshot v=snapshot(1,2);
 struct camera_recording_index_fingerprint absent={0},old,pending;struct camera_recording_index_report r;
 struct hook h={0};unsigned char wire[CAMERA_RECORDING_INDEX_MAX_BYTES];size_t n;char saved[32];
 init(&s,fd,&h,1);CHECK(camera_recording_index_commit(&s,&absent,&v,1,&r)==CAMERA_INDEX_OK);old=r.current.fingerprint;
 n=read_wire(fd,CAMERA_RECORDING_INDEX_CURRENT,wire);snprintf(saved,sizeof(saved),"preserved-index-%u",cases);
 CHECK(renameat(fd,CAMERA_RECORDING_INDEX_CURRENT,rootfd,saved)==0);store_wire(fd,CAMERA_RECORDING_INDEX_CURRENT,wire,n);
 v.revision=2;
 CHECK(camera_recording_index_commit(&s,&old,&v,1,&r)==CAMERA_INDEX_CONFLICT);
 CHECK(r.current.fingerprint.inode!=old.inode&&r.pending.state==CAMERA_INDEX_ABSENT);old=r.current.fingerprint;
 h.point=CAMERA_INDEX_RENAME;h.nth=1;
 CHECK(camera_recording_index_commit(&s,&old,&v,1,&r)==CAMERA_INDEX_UNCERTAIN);pending=r.pending.fingerprint;
 h.point=0;n=read_wire(fd,CAMERA_RECORDING_INDEX_PENDING,wire);snprintf(saved,sizeof(saved),"preserved-pending-%u",cases);
 CHECK(renameat(fd,CAMERA_RECORDING_INDEX_PENDING,rootfd,saved)==0);store_wire(fd,CAMERA_RECORDING_INDEX_PENDING,wire,n);
 CHECK(camera_recording_index_resume(&s,&old,&pending,&r)==CAMERA_INDEX_CONFLICT);
 CHECK(r.pending.fingerprint.inode!=pending.inode&&r.current.snapshot.revision==1&&!r.renamed);
 CHECK(close(fd)==0);suites++;
}
static void generation_and_exhaustion(void)
{
 int fd=new_directory();struct camera_recording_index_store s;struct camera_recording_index_snapshot v=snapshot(1,2),next;
 struct camera_recording_index_report r;struct camera_recording_index_fingerprint absent={0},fp;
 init(&s,fd,NULL,1);CHECK(camera_recording_index_commit(&s,&absent,&v,1,&r)==CAMERA_INDEX_OK);fp=r.current.fingerprint;
 next=v;next.revision=2;next.entries[0].metadata.bytes++;
 CHECK(camera_recording_index_commit(&s,&fp,&next,1,&r)==CAMERA_INDEX_CONFLICT&&r.pending.state==CAMERA_INDEX_ABSENT);
 next=v;next.revision=2;memset(next.entries[0].metadata.recording_id,0,64);memcpy(next.entries[0].metadata.recording_id,"new-id",7);
 CHECK(camera_recording_index_commit(&s,&fp,&next,1,&r)==CAMERA_INDEX_CONFLICT);
 next=v;next.revision=2;next.entries[0]=v.entries[1];next.entries[1]=v.entries[0];next.entries[1].generation=3;next.generation_highwater=3;
 next.entries[1].metadata.origin=CAMERA_RECORDING_USER;
 CHECK(camera_recording_index_commit(&s,&fp,&next,1,&r)==CAMERA_INDEX_CONFLICT);
 next.entries[1].metadata.origin=CAMERA_RECORDING_SERVICE_OWNED;memcpy(next.entries[1].metadata.sensor_id,next.scope.sensors[1],64);
 CHECK(camera_recording_index_commit(&s,&fp,&next,1,&r)==CAMERA_INDEX_CONFLICT);
 memcpy(next.entries[1].metadata.sensor_id,next.scope.sensors[0],64);next.entries[1].metadata.bytes++;
 CHECK(camera_recording_index_commit(&s,&fp,&next,1,&r)==CAMERA_INDEX_OK&&r.current.snapshot.generation_highwater==3);
 CHECK(close(fd)==0);
 fd=new_directory();init(&s,fd,NULL,1);v=snapshot(1,64);
 CHECK(camera_recording_index_commit(&s,&absent,&v,1,&r)==CAMERA_INDEX_OK&&r.current.fingerprint.bytes==CAMERA_RECORDING_INDEX_MAX_BYTES);
 CHECK(close(fd)==0);
 fd=new_directory();init(&s,fd,NULL,1);v=snapshot(1,0);v.revision=UINT64_MAX;
 struct camera_recording_index_fingerprint historical={1,2,512,UINT64_MAX-1,1,0};
 unsigned char wire[CAMERA_RECORDING_INDEX_MAX_BYTES];size_t n=0;uint32_t intent;struct camera_recording_index_snapshot parsed;
 CHECK(camera_recording_index_encode(&v,&historical,1,wire,sizeof(wire),&n)==CAMERA_INDEX_OK);
 CHECK(camera_recording_index_decode(wire,n,&v.scope,&parsed,&fp,&intent)==CAMERA_INDEX_OK&&parsed.revision==UINT64_MAX);
 store_wire(fd,CAMERA_RECORDING_INDEX_CURRENT,wire,n);
 CHECK(camera_recording_index_inspect(&s,&r)==CAMERA_INDEX_OK);fp=r.current.fingerprint;next=snapshot(1,0);
 CHECK(camera_recording_index_commit(&s,&fp,&next,1,&r)==CAMERA_INDEX_EXHAUSTED);
 put64(wire+440,513);fix_crc(wire,n);
 CHECK(camera_recording_index_decode(wire,n,&v.scope,&parsed,&fp,&intent)==CAMERA_INDEX_CORRUPT);
 CHECK(close(fd)==0);suites++;
}
static void anchor_and_no_struct_dump(void)
{
 int fd=new_directory(),other=new_directory();struct camera_recording_index_store s;
 struct camera_recording_index_snapshot v=snapshot(1,0);struct camera_recording_index_fingerprint base={0};
 struct camera_recording_index_report r;unsigned char a[CAMERA_RECORDING_INDEX_MAX_BYTES],b[CAMERA_RECORDING_INDEX_MAX_BYTES];size_t n,m;
 n=encode(&v,a);memset(&v.entries[63],0x5a,sizeof(v.entries[63])); /* Unused native storage is NOT serialized. */
 m=encode(&v,b);CHECK(n==m&&!memcmp(a,b,n));
 init(&s,fd,NULL,1);CHECK(dup2(other,fd)==fd);
 CHECK(camera_recording_index_inspect(&s,&r)==CAMERA_INDEX_UNSAFE);
 CHECK(camera_recording_index_commit(&s,&base,&v,1,&r)==CAMERA_INDEX_UNSAFE);
 struct stat st;CHECK(fstatat(other,CAMERA_RECORDING_INDEX_PENDING,&st,AT_SYMLINK_NOFOLLOW)<0);
 CHECK(close(fd)==0&&close(other)==0);suites++;
}
int main(int argc,char **argv)
{
 CHECK(argc==2);CHECK(mkdir(argv[1],0700)==0); /* REFUSE existing paths; public fresh fixtures only. */
 rootfd=open(argv[1],O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);CHECK(rootfd>=0);
 codec_roundtrip();codec_denials();aliases();transactions();fault_matrix();namespaces();preserve_denials();inode_cas();
 generation_and_exhaustion();
 anchor_and_no_struct_dump();
 CHECK(close(rootfd)==0);
 printf("recording_index_store suites=%u checks=%u fs_cases=%u PASS (public synthetic host metadata only)\n",suites,checks,cases);
 return 0;
}
