/* SPDX-License-Identifier: GPL-2.0-only */
#include <camera/recording_catalog.h>
#include <string.h>

#define CATALOG_MAGIC 0x52434331U
/* Compare integer addresses, not unrelated-object pointer order. Validate spans
 * without overflowing an endpoint; zero-byte spans may have a NULL pointer. */
static int span_disjoint(const void *a,size_t an,const void *b,size_t bn)
{
 uintptr_t av=(uintptr_t)a,bv=(uintptr_t)b;
 if(!an||!bn)return 1;
 if(!a||!b||an-1>UINTPTR_MAX-av||bn-1>UINTPTR_MAX-bv)return 0;
 return av<=bv?bv-av>=an:av-bv>=bn;
}
static int array_bytes(size_t count,size_t element,size_t *bytes)
{
 if(count>SIZE_MAX/element)return 0;
 *bytes=count*element;return 1;
}
static int outside(const struct camera_recording_catalog *c,const void *p,size_t bytes)
{return !p||span_disjoint(c,sizeof(*c),p,bytes);}
static int zero(const void *ptr,size_t bytes)
{
 const unsigned char *p=ptr;size_t i;
 for(i=0;i<bytes;i++)if(p[i])return 0;
 return 1;
}
/* Same fixed64-byte canonical convention as access_policy.c; no string/path API. */
static int id_valid(const char id[CAMERA_ID_BYTES])
{
 size_t i;
 if(!id||!id[0])return 0;
 for(i=0;i<CAMERA_ID_BYTES;i++){
  unsigned char c=(unsigned char)id[i];
  if(!c)return zero(id+i,CAMERA_ID_BYTES-i);
  if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||
       c=='-'||c=='_'||c=='.'))return 0;
 }
 return 0;
}
static int same_id(const char *a,const char *b){return !memcmp(a,b,CAMERA_ID_BYTES);}
static int ready(const struct camera_recording_catalog *c)
{return c&&c->initialized==CATALOG_MAGIC;}
static int space_valid(const struct camera_recording_namespace *s)
{return s&&id_valid(s->storage_id)&&s->storage_generation&&s->instance_generation;}
static int space_same(const struct camera_recording_namespace *a,
 const struct camera_recording_namespace *b)
{
 return same_id(a->storage_id,b->storage_id)&&a->storage_generation==b->storage_generation&&
  a->instance_generation==b->instance_generation;
}
static int scope_same(const struct camera_recording_scope *a,const struct camera_recording_scope *b)
{return space_same(&a->space,&b->space)&&same_id(a->sensor_id,b->sensor_id);}
static int scope_valid(const struct camera_recording_catalog *c,const struct camera_recording_scope *s)
{
 uint32_t i;
 if(!s||!space_valid(&s->space)||!id_valid(s->sensor_id)||!space_same(&c->space,&s->space))return 0;
 for(i=0;i<c->sensor_count;i++)if(same_id(s->sensor_id,c->sensors[i]))return 1;
 return 0;
}
static int metadata_valid(const struct camera_recording_catalog *c,const struct camera_recording_metadata *m)
{
 uint32_t i;
 if(!m||m->abi!=CAMERA_RECORDING_ABI||m->struct_bytes!=sizeof(*m)||m->reserved||
    !id_valid(m->recording_id)||!id_valid(m->sensor_id)||
    m->origin<CAMERA_RECORDING_SERVICE_OWNED||m->origin>CAMERA_RECORDING_FOREIGN||
    m->state<CAMERA_RECORDING_OPEN||m->state>CAMERA_RECORDING_CORRUPT||
    m->closed>1||m->active>1||m->time_domain>CAMERA_RECORDING_TIME_QUALIFIED_UTC)return 0;
 if(m->state==CAMERA_RECORDING_OPEN&&m->closed)return 0;
 if(m->state==CAMERA_RECORDING_COMPLETE&&(!m->closed||m->active))return 0;
 if(m->time_domain==CAMERA_RECORDING_TIME_UNKNOWN){
  if(m->start_ns||m->end_ns||m->time_epoch)return 0;
 }else if(!m->time_epoch||m->start_ns>m->end_ns)return 0;
 for(i=0;i<c->sensor_count;i++)if(same_id(m->sensor_id,c->sensors[i]))return 1;
 return 0;
}
static int find_id(const struct camera_recording_catalog *c,const char *id)
{
 uint32_t i;
 for(i=0;i<CAMERA_RECORDING_MAX_ENTRIES;i++)
  if(c->slots[i].used&&same_id(id,c->slots[i].metadata.recording_id))return (int)i;
 return -1;
}
static int find_ref(const struct camera_recording_catalog *c,const struct camera_recording_ref *r)
{
 int i;
 if(!r||!scope_valid(c,&r->scope)||!id_valid(r->recording_id)||!r->generation)return -1;
 i=find_id(c,r->recording_id);
 if(i<0||!same_id(r->scope.sensor_id,c->slots[i].metadata.sensor_id)||
    r->generation!=c->slots[i].generation)return -1;
 return i;
}
static void make_ref(const struct camera_recording_catalog *c,uint32_t i,struct camera_recording_ref *r)
{
 memset(r,0,sizeof(*r));r->scope.space=c->space;
 memcpy(r->scope.sensor_id,c->slots[i].metadata.sensor_id,CAMERA_ID_BYTES);
 memcpy(r->recording_id,c->slots[i].metadata.recording_id,CAMERA_ID_BYTES);
 r->generation=c->slots[i].generation;
}
static int ref_same(const struct camera_recording_ref *a,const struct camera_recording_ref *b)
{return scope_same(&a->scope,&b->scope)&&same_id(a->recording_id,b->recording_id)&&a->generation==b->generation;}
static int can_change(const struct camera_recording_catalog *c)
{return c->revision!=UINT64_MAX;}
static void changed(struct camera_recording_catalog *c){c->revision++;c->plan_valid=0;}
static int observe(struct camera_recording_catalog *c,uint64_t now)
{
 if(c->clock_failed)return CAMERA_NOT_READY;
 if(now<c->last_ns){c->clock_failed=1;c->plan_valid=0;return CAMERA_NOT_READY;}
 c->last_ns=now;return CAMERA_OK;
}
static int eligible(const struct camera_recording_slot *s)
{
 return s->used&&s->metadata.origin==CAMERA_RECORDING_SERVICE_OWNED&&
  s->metadata.state==CAMERA_RECORDING_COMPLETE&&s->metadata.closed&&
  !s->metadata.active&&!s->pins&&!s->reservation;
}
int camera_recording_catalog_init(struct camera_recording_catalog *c,
 const struct camera_recording_namespace *space,const char sensors[][CAMERA_ID_BYTES],
 uint32_t count,uint64_t now)
{
 uint32_t i,j;size_t bytes;
 if(!c)return CAMERA_INVALID;
 /* Alias rejection precedes even failed-init clearing. */
 if(!array_bytes(count,CAMERA_ID_BYTES,&bytes)||!outside(c,space,sizeof(*space))||
    !outside(c,sensors,bytes))return CAMERA_INVALID;
 memset(c,0,sizeof(*c));
 if(!space_valid(space)||!sensors||!count||count>CAMERA_RECORDING_MAX_SENSORS)return CAMERA_INVALID;
 for(i=0;i<count;i++){
  if(!id_valid(sensors[i]))return CAMERA_INVALID;
  for(j=0;j<i;j++)if(same_id(sensors[i],sensors[j]))return CAMERA_INVALID;
 }
 c->space=*space;c->sensor_count=count;c->last_ns=now;
 memcpy(c->sensors,sensors,bytes);
 c->revision=1;c->initialized=CATALOG_MAGIC;return CAMERA_OK;
}
static int put(struct camera_recording_catalog *c,const struct camera_recording_metadata *m,
 int i,struct camera_recording_ref *r)
{
 struct camera_recording_metadata snapshot=*m;
 uint64_t base=c->total_bytes-(c->slots[i].used?c->slots[i].metadata.bytes:0);
 if(m->bytes>UINT64_MAX-base)return CAMERA_INVALID;
 if(!can_change(c)||c->next_generation==UINT64_MAX)return CAMERA_NOT_READY;
 memset(&c->slots[i],0,sizeof(c->slots[i]));c->slots[i].metadata=snapshot;
 c->slots[i].used=1;c->slots[i].generation=++c->next_generation;
 c->total_bytes=base+snapshot.bytes;changed(c);make_ref(c,(uint32_t)i,r);return CAMERA_OK;
}
int camera_recording_catalog_add(struct camera_recording_catalog *c,
 const struct camera_recording_metadata *m,struct camera_recording_ref *r)
{
 uint32_t i;
 if(!ready(c)||!r||!outside(c,m,sizeof(*m))||!outside(c,r,sizeof(*r))||
    !metadata_valid(c,m))return CAMERA_INVALID;
 if(find_id(c,m->recording_id)>=0)return CAMERA_INVALID;
 for(i=0;i<CAMERA_RECORDING_MAX_ENTRIES;i++)if(!c->slots[i].used)return put(c,m,(int)i,r);
 return CAMERA_BUSY;
}
int camera_recording_catalog_replace(struct camera_recording_catalog *c,
 const struct camera_recording_ref *expected,const struct camera_recording_metadata *m,
 struct camera_recording_ref *r)
{
 int i;
 if(!ready(c)||!r||!outside(c,expected,sizeof(*expected))||!outside(c,m,sizeof(*m))||
    !outside(c,r,sizeof(*r))||!metadata_valid(c,m))return CAMERA_INVALID;
 i=find_ref(c,expected);if(i<0)return CAMERA_STALE_LEASE;
 if(!same_id(m->recording_id,expected->recording_id)||!same_id(m->sensor_id,expected->scope.sensor_id)||
    m->origin!=c->slots[i].metadata.origin)return CAMERA_INVALID;
 if(c->slots[i].pins||c->slots[i].reservation)return CAMERA_BUSY;
 return put(c,m,i,r);
}
int camera_recording_catalog_get(const struct camera_recording_catalog *c,
 const struct camera_recording_ref *r,struct camera_recording_metadata *m)
{
 int i;
 if(!ready(c)||!m||!outside(c,r,sizeof(*r))||!outside(c,m,sizeof(*m)))return CAMERA_INVALID;
 i=find_ref(c,r);if(i<0)return CAMERA_STALE_LEASE;
 *m=c->slots[i].metadata;return CAMERA_OK;
}
int camera_recording_pin_acquire(struct camera_recording_catalog *c,
 const struct camera_recording_ref *r,struct camera_recording_pin *p)
{
 int i;uint32_t j;struct camera_recording_slot *s;
 if(!ready(c)||!p||!outside(c,r,sizeof(*r))||!outside(c,p,sizeof(*p)))return CAMERA_INVALID;
 i=find_ref(c,r);if(i<0)return CAMERA_STALE_LEASE;s=&c->slots[i];
 if(s->reservation||s->metadata.state!=CAMERA_RECORDING_COMPLETE||
    !s->metadata.closed||s->metadata.active)return CAMERA_BUSY;
 for(j=0;j<CAMERA_RECORDING_MAX_PINS;j++)if(!c->pins[j].used)break;
 if(j==CAMERA_RECORDING_MAX_PINS)return CAMERA_BUSY;
 if(!can_change(c)||c->next_serial==UINT64_MAX)return CAMERA_NOT_READY;
 c->pins[j].pin.ref=*r;c->pins[j].pin.serial=++c->next_serial;c->pins[j].used=1;
 s->pins++;changed(c);*p=c->pins[j].pin;return CAMERA_OK;
}
static int find_pin(const struct camera_recording_catalog *c,const struct camera_recording_pin *p)
{
 uint32_t i;
 if(!p||!p->serial||find_ref(c,&p->ref)<0)return -1;
 for(i=0;i<CAMERA_RECORDING_MAX_PINS;i++)if(c->pins[i].used&&
  c->pins[i].pin.serial==p->serial&&ref_same(&c->pins[i].pin.ref,&p->ref))return (int)i;
 return -1;
}
int camera_recording_pin_check(const struct camera_recording_catalog *c,const struct camera_recording_pin *p)
{
 if(!ready(c)||!outside(c,p,sizeof(*p)))return CAMERA_INVALID;
 return find_pin(c,p)>=0?CAMERA_OK:CAMERA_STALE_LEASE;
}
int camera_recording_pin_release(struct camera_recording_catalog *c,const struct camera_recording_pin *p)
{
 int i,j;
 if(!ready(c)||!outside(c,p,sizeof(*p)))return CAMERA_INVALID;
 i=find_pin(c,p);if(i<0)return CAMERA_STALE_LEASE;
 if(!can_change(c))return CAMERA_NOT_READY;
 j=find_ref(c,&p->ref);c->slots[j].pins--;
 memset(&c->pins[i],0,sizeof(c->pins[i]));changed(c);return CAMERA_OK;
}
static int age_known(const struct camera_recording_metadata *m,const struct camera_recording_budget *b)
{
 return m->time_domain==CAMERA_RECORDING_TIME_QUALIFIED_UTC&&
  m->time_epoch==b->utc_epoch&&m->end_ns<=b->utc_now_ns;
}
static int aged(const struct camera_recording_metadata *m,const struct camera_recording_budget *b)
{return age_known(m,b)&&b->utc_now_ns-m->end_ns>b->max_age_ns;}
int camera_recording_retention_plan(struct camera_recording_catalog *c,
 const struct camera_recording_scope *scope,const struct camera_recording_budget *b,
 uint64_t now,struct camera_recording_plan *p)
{
 uint32_t i,n=0,order[CAMERA_RECORDING_MAX_ENTRIES],j,k;uint64_t bytes=0;
 struct camera_recording_plan result;
 if(!ready(c)||!p||!b||!outside(c,scope,sizeof(*scope))||!outside(c,b,sizeof(*b))||
    !outside(c,p,sizeof(*p))||b->max_count>CAMERA_RECORDING_MAX_ENTRIES||b->age_enabled>1||
    (!b->age_enabled&&(b->max_age_ns||b->utc_now_ns||b->utc_epoch))||
    (b->age_enabled&&!b->utc_epoch))return CAMERA_INVALID;
 if(!scope_valid(c,scope))return CAMERA_STALE_LEASE;
 if(observe(c,now)!=CAMERA_OK)return CAMERA_NOT_READY;
 if(b->age_enabled){
  if(b->utc_epoch<c->last_utc_epoch||(b->utc_epoch==c->last_utc_epoch&&b->utc_now_ns<c->last_utc_ns)){
   c->clock_failed=1;c->plan_valid=0;return CAMERA_NOT_READY;
  }
  c->last_utc_epoch=b->utc_epoch;c->last_utc_ns=b->utc_now_ns;
 }
 if(c->next_serial==UINT64_MAX)return CAMERA_NOT_READY;
 memset(&result,0,sizeof(result));result.scope=*scope;result.revision=c->revision;
 for(i=0;i<CAMERA_RECORDING_MAX_ENTRIES;i++)if(c->slots[i].used&&
  same_id(scope->sensor_id,c->slots[i].metadata.sensor_id)){
  /* Insertion sort on nonwrapping unique admission generation. */
  j=n;while(j&&c->slots[order[j-1]].generation>c->slots[i].generation){order[j]=order[j-1];j--;}
  order[j]=i;n++;bytes+=c->slots[i].metadata.bytes;
 }
 result.remaining_count=n;result.remaining_bytes=bytes;
 for(k=0;k<n;k++){
  struct camera_recording_slot *s=&c->slots[order[k]];
  int old=b->age_enabled&&aged(&s->metadata,b);
  if(eligible(s)&&(old||result.remaining_count>b->max_count||result.remaining_bytes>b->max_bytes)){
   make_ref(c,order[k],&result.entries[result.count++]);
   result.remaining_count--;result.remaining_bytes-=s->metadata.bytes;
  }else if(b->age_enabled){
   if(!age_known(&s->metadata,b))result.pressure|=CAMERA_RECORDING_PRESSURE_TIME_UNKNOWN;
   else if(old)result.pressure|=CAMERA_RECORDING_PRESSURE_AGE;
  }
 }
 if(result.remaining_count>b->max_count)result.pressure|=CAMERA_RECORDING_PRESSURE_COUNT;
 if(result.remaining_bytes>b->max_bytes)result.pressure|=CAMERA_RECORDING_PRESSURE_BYTES;
 result.serial=++c->next_serial;c->pending=result;c->plan_valid=1;*p=result;
 return result.pressure?CAMERA_BUSY:CAMERA_OK;
}
static int plan_same(const struct camera_recording_plan *a,const struct camera_recording_plan *b)
{
 uint32_t i;
 if(!scope_same(&a->scope,&b->scope)||a->revision!=b->revision||a->serial!=b->serial||
    a->remaining_bytes!=b->remaining_bytes||a->count!=b->count||a->remaining_count!=b->remaining_count||
    a->pressure!=b->pressure||a->reserved!=b->reserved)return 0;
 for(i=0;i<CAMERA_RECORDING_MAX_ENTRIES;i++)if(!ref_same(&a->entries[i],&b->entries[i]))return 0;
 return 1;
}
int camera_recording_retention_reserve(struct camera_recording_catalog *c,
 const struct camera_recording_plan *p,uint64_t now,struct camera_recording_reservation *r,uint32_t capacity)
{
 uint32_t i;int indices[CAMERA_RECORDING_MAX_ENTRIES];size_t bytes;
 if(!ready(c)||!p||!outside(c,p,sizeof(*p))||
    !array_bytes(capacity,sizeof(*r),&bytes)||!outside(c,r,bytes)||
    (r&&!span_disjoint(p,sizeof(*p),r,bytes))||
    p->count>CAMERA_RECORDING_MAX_ENTRIES||capacity<p->count||
    (p->count&&!r))return CAMERA_INVALID;
 if(observe(c,now)!=CAMERA_OK)return CAMERA_NOT_READY;
 if(!c->plan_valid||p->revision!=c->revision||!plan_same(p,&c->pending))return CAMERA_STALE_LEASE;
 for(i=0;i<p->count;i++){
  indices[i]=find_ref(c,&p->entries[i]);
  if(indices[i]<0||!eligible(&c->slots[indices[i]]))return CAMERA_STALE_LEASE;
 }
 if(!p->count){c->plan_valid=0;return CAMERA_OK;}
 if(!can_change(c)||c->next_serial==UINT64_MAX)return CAMERA_NOT_READY;
 c->next_serial++;
 for(i=0;i<p->count;i++){
  c->slots[indices[i]].reservation=c->next_serial;
  r[i].ref=p->entries[i];r[i].serial=c->next_serial;
 }
 changed(c);return CAMERA_OK;
}
static int find_reservation(const struct camera_recording_catalog *c,const struct camera_recording_reservation *r)
{
 int i;
 if(!r||!r->serial)return -1;
 i=find_ref(c,&r->ref);
 if(i<0||c->slots[i].reservation!=r->serial)return -1;
 return i;
}
int camera_recording_retention_check(struct camera_recording_catalog *c,
 const struct camera_recording_reservation *r,uint64_t now)
{
 if(!ready(c)||!outside(c,r,sizeof(*r)))return CAMERA_INVALID;
 if(observe(c,now)!=CAMERA_OK)return CAMERA_NOT_READY;
 /* Do not authorize a native effect if its acknowledgement cannot advance. */
 if(!can_change(c))return CAMERA_NOT_READY;
 return find_reservation(c,r)>=0?CAMERA_OK:CAMERA_STALE_LEASE;
}
int camera_recording_retention_ack(struct camera_recording_catalog *c,
 const struct camera_recording_reservation *r,uint32_t removed)
{
 int i;
 if(!ready(c)||removed>1||!outside(c,r,sizeof(*r)))return CAMERA_INVALID;
 i=find_reservation(c,r);if(i<0)return CAMERA_STALE_LEASE;
 if(!can_change(c))return CAMERA_NOT_READY;
 if(removed){c->total_bytes-=c->slots[i].metadata.bytes;memset(&c->slots[i],0,sizeof(c->slots[i]));}
 else c->slots[i].reservation=0;
 changed(c);return CAMERA_OK;
}
