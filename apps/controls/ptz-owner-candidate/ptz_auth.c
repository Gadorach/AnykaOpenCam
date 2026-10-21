/* SPDX-License-Identifier: GPL-2.0-only */
#include "ptz_auth.h"
#include <string.h>

int ptz_overlap(const void *a,size_t an,const void *b,size_t bn)
{
 uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 if(!an||!bn)return 0;
 if(!a||!b||an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return 1;
 return x<y+bn&&y<x+an;
}
static int zero(const void *v,size_t n)
{
 const unsigned char *p=v;size_t i;
 for(i=0;i<n;i++)if(p[i])return 0;
 return 1;
}
int ptz_id_valid(const char id[CAMERA_ID_BYTES])
{
 size_t i;
 if(!id||!id[0])return 0;
 for(i=0;i<CAMERA_ID_BYTES;i++){
  unsigned char c=(unsigned char)id[i];
  if(!c)return zero(id+i,CAMERA_ID_BYTES-i);
  if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'||c=='.'))return 0;
 }
 return 0;
}
static int same(const char *a,const char *b){return !memcmp(a,b,CAMERA_ID_BYTES);}
int ptz_same_principal(const struct camera_access_principal *a,const struct camera_access_principal *b)
{
 const struct camera_access_verified *x=&a->fact,*y=&b->fact;
 return a->snapshot_serial==b->snapshot_serial&&a->policy_revision==b->policy_revision&&
  x->domain==y->domain&&x->boot_generation==y->boot_generation&&
  x->credential_generation==y->credential_generation&&x->session_generation==y->session_generation&&
  x->issued_ns==y->issued_ns&&x->expires_ns==y->expires_ns&&x->abi==y->abi&&
  x->struct_bytes==y->struct_bytes&&x->verified==y->verified&&x->reserved==y->reserved&&
  same(x->device_id,y->device_id)&&same(x->account_id,y->account_id);
}
int ptz_auth_init(struct ptz_auth *a,struct camera_access_policy *p,
 const struct camera_graph *g,const char head[CAMERA_ID_BYTES])
{
 struct ptz_auth candidate;size_t i,j;int found=0,policy_head=0;
 if(!a||!p||!g||!head||ptz_overlap(a,sizeof(*a),p,sizeof(*p))||
    ptz_overlap(a,sizeof(*a),g,sizeof(*g))||ptz_overlap(a,sizeof(*a),head,CAMERA_ID_BYTES)||
    !zero(a,sizeof(*a))||!ptz_id_valid(head)||!ptz_id_valid(p->device_id)||
    p->resource_count>CAMERA_ACCESS_MAX_RESOURCES||!p->boot_generation)return CAMERA_INVALID;
 /* Bound and canonicalize before the shared validator's string comparisons. */
 if(g->device_count>16||g->association_count>16||g->profile_count>8||g->group_count>8||
    (g->device_count&&!g->devices)||(g->association_count&&!g->associations)||
    (g->profile_count&&!g->profiles)||(g->group_count&&!g->groups))return CAMERA_INVALID;
 if(ptz_overlap(a,sizeof(*a),g->devices,g->device_count*sizeof(*g->devices))||
    ptz_overlap(a,sizeof(*a),g->associations,g->association_count*sizeof(*g->associations))||
    ptz_overlap(a,sizeof(*a),g->profiles,g->profile_count*sizeof(*g->profiles))||
    ptz_overlap(a,sizeof(*a),g->groups,g->group_count*sizeof(*g->groups)))return CAMERA_INVALID;
 for(i=0;i<g->device_count;i++){
  if(!ptz_id_valid(g->devices[i].id))return CAMERA_INVALID;
  if(same(g->devices[i].id,head)&&g->devices[i].kind==CAMERA_DEVICE_PTZ&&
     g->devices[i].state!=CAMERA_UNAVAILABLE)found=1;
 }
 for(i=0;i<g->association_count;i++)if(!ptz_id_valid(g->associations[i].camera_id)||
    !ptz_id_valid(g->associations[i].device_id))return CAMERA_INVALID;
 for(i=0;i<g->profile_count;i++)if(!ptz_id_valid(g->profiles[i].id)||
    !ptz_id_valid(g->profiles[i].camera_id))return CAMERA_INVALID;
 for(i=0;i<g->group_count;i++)if(!ptz_id_valid(g->groups[i].id))return CAMERA_INVALID;
 if(!found||camera_graph_validate(g)!=CAMERA_OK)return CAMERA_INVALID;
 for(i=0;i<p->resource_count;i++)if(p->resources[i].kind==CAMERA_ACCESS_PTZ&&
    same(p->resources[i].id,head))policy_head=1;
 if(!policy_head)return CAMERA_INVALID;
 memset(&candidate,0,sizeof(candidate));candidate.self=a;candidate.policy=p;
 memcpy(candidate.head,head,CAMERA_ID_BYTES);
 for(i=0;i<g->association_count;i++)if(same(g->associations[i].device_id,head)){
  for(j=0;j<g->device_count;j++)if(same(g->devices[j].id,g->associations[i].camera_id)&&
     g->devices[j].kind==CAMERA_DEVICE_SENSOR&&g->devices[j].state!=CAMERA_UNAVAILABLE)break;
  if(j==g->device_count||candidate.sensor_count==PTZ_ASSOC_MAX)return CAMERA_INVALID;
  for(j=0;j<p->resource_count;j++)if(p->resources[j].kind==CAMERA_ACCESS_SENSOR&&
     same(p->resources[j].id,g->associations[i].camera_id))break;
  if(j==p->resource_count)return CAMERA_INVALID;
  memcpy(candidate.sensors[candidate.sensor_count++],g->associations[i].camera_id,CAMERA_ID_BYTES);
 }
 if(!candidate.sensor_count)return CAMERA_INVALID;
 *a=candidate;return CAMERA_OK;
}
int ptz_auth_check(struct ptz_auth *a,const struct camera_access_principal *p,
 const char sensor[CAMERA_ID_BYTES],uint64_t now)
{
 struct camera_access_request q;uint32_t i;
 if(!a||a->self!=a||!a->policy||!p||!sensor||a->sensor_count>PTZ_ASSOC_MAX||
    ptz_overlap(p,sizeof(*p),a,sizeof(*a))||ptz_overlap(sensor,CAMERA_ID_BYTES,a,sizeof(*a))||
    ptz_overlap(p,sizeof(*p),a->policy,sizeof(*a->policy))||
    ptz_overlap(sensor,CAMERA_ID_BYTES,a->policy,sizeof(*a->policy))||!ptz_id_valid(sensor)||
    (p->fact.domain!=CAMERA_ACCESS_UI&&p->fact.domain!=CAMERA_ACCESS_ONVIF))return CAMERA_ACCESS_DENIED;
 for(i=0;i<a->sensor_count;i++)if(same(a->sensors[i],sensor))break;
 if(i==a->sensor_count)return CAMERA_ACCESS_DENIED;
 memset(&q,0,sizeof(q));q.abi=CAMERA_ACCESS_ABI;q.struct_bytes=sizeof(q);
 q.domain=p->fact.domain;q.kind=CAMERA_ACCESS_PTZ;q.operation=CAMERA_ACCESS_PTZ_CONTROL;
 memcpy(q.device_id,a->policy->device_id,CAMERA_ID_BYTES);memcpy(q.resource_id,a->head,CAMERA_ID_BYTES);
 return camera_access_check(a->policy,p,&q,now);
}
