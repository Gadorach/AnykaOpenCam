/* SPDX-License-Identifier: GPL-2.0-only */
#include "public_codec.h"
#include <camera/access_policy.h>
#include <string.h>
_Static_assert(sizeof(struct rv_material)==1536,"actual material ABI");
_Static_assert(PE_PREFIX+16*144+16*128+8*204+8*72==PE_MAX_REQUEST,"wire maximum");
_Static_assert(48+14*4+4*64+2*513+6==PE_REPLY,"explicit reply bytes");
_Static_assert(sizeof(struct pe_public)<131072,"bounded local reconstruction");
uint32_t pe_get32(const unsigned char *p)
{return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
uint64_t pe_get64(const unsigned char *p)
{return (uint64_t)pe_get32(p)|(uint64_t)pe_get32(p+4)<<32;}
static void put32(unsigned char *p,uint32_t v)
{for(unsigned i=0;i<4;i++)p[i]=(unsigned char)(v>>(8*i));}
static void put64(unsigned char *p,uint64_t v)
{for(unsigned i=0;i<8;i++)p[i]=(unsigned char)(v>>(8*i));}
static int text(const char *p,size_t n,int required,int identifier)
{
 if(required&&!p[0])return 0;
 size_t i=0;
 for(;i<n&&p[i];i++){
  unsigned char c=(unsigned char)p[i];
  if(identifier&&!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||
      (c>='0'&&c<='9')||c=='_'||c=='-'||c=='.'))return 0;
 }
 if(i==n)return 0;
 for(;i<n;i++)if(p[i])return 0;
 return 1;
}
int pe_config_class(const struct candidate_config *c)
{
 return c&&c->abi==CANDIDATE_AUTH_ABI&&c->struct_bytes==sizeof(*c)&&
  !c->host_fixture_only&&c->material_fd==-1&&c->image.abi==RV_ABI&&
  c->image.profile==RV_NATIVE&&c->begin_contract==CANDIDATE_BEGIN_LOGICAL_ONLY;
}
int pe_public_validate(const struct candidate_config *c,const struct camera_rtsp_registry *r)
{
 struct camera_rtsp_registry checked;struct camera_digest_wire_limit limit;
 if(!pe_config_class(c)||!c->boot_generation||!c->epoch||!c->credential_generation||
    !text(c->device_id,64,1,1)||!text(c->account_id,64,1,1)||!text(c->profile_id,64,1,1)||
    (c->algorithm!=CAMERA_DIGEST_SHA256&&c->algorithm!=CAMERA_DIGEST_MD5_LEGACY)||
    !c->nonce_ttl_ms||c->nonce_ttl_ms>CAMERA_DIGEST_TTL_MAX_MS||!c->graph||!r||
    c->graph->device_count>16||c->graph->association_count>16||
    c->graph->profile_count>8||c->graph->group_count>8||camera_graph_validate(c->graph)||
    !text(r->live_uri,513,1,0)||!text(r->track_uri,513,1,0)||
    camera_rtsp_registry_init(&checked,r->live_uri,r->track_uri)||
    memcmp(checked.live_uri,r->live_uri,513)||memcmp(checked.track_uri,r->track_uri,513)||
    camera_digest_wire_limit_init(&limit,0,c->rate_window_ms,c->global_verifies,c->global_challenges)||
    camera_digest_wire_limit_init(&limit,0,c->rate_window_ms,c->account_verifies,c->account_challenges)||
    camera_digest_wire_limit_init(&limit,0,c->rate_window_ms,c->peer_verifies,c->peer_challenges))return -1;
 const struct camera_graph *g=c->graph;int selected=0;
 for(size_t i=0;i<g->device_count;i++)
  if(!text(g->devices[i].id,64,1,0)||!text(g->devices[i].resource_group,64,0,0))return -1;
 for(size_t i=0;i<g->association_count;i++)
  if(!text(g->associations[i].camera_id,64,1,0)||!text(g->associations[i].device_id,64,1,0))return -1;
 for(size_t i=0;i<g->profile_count;i++){
  const struct camera_profile *p=&g->profiles[i];
  if(!text(p->id,64,1,0)||!text(p->camera_id,64,1,0)||!text(p->encoder_id,64,0,0))return -1;
  if(!memcmp(p->id,c->profile_id,64)){
   if(!text(p->camera_id,64,1,1)||p->codec!=CAMERA_H264||p->width!=1280||p->height!=720)return -1;
   selected=1;
  }
 }
 for(size_t i=0;i<g->group_count;i++)if(!text(g->groups[i].id,64,1,0))return -1;
 return selected?0:-1;
}
static void limits_encode(unsigned char *p,const struct candidate_config *c)
{
 put32(p,c->algorithm);put32(p+4,c->nonce_ttl_ms);put32(p+8,c->rate_window_ms);
 put32(p+12,c->global_verifies);put32(p+16,c->global_challenges);
 put32(p+20,c->account_verifies);put32(p+24,c->account_challenges);
 put32(p+28,c->peer_verifies);put32(p+32,c->peer_challenges);
}
static void limits_decode(struct candidate_config *c,const unsigned char *p)
{
 c->algorithm=pe_get32(p);c->nonce_ttl_ms=pe_get32(p+4);c->rate_window_ms=pe_get32(p+8);
 c->global_verifies=pe_get32(p+12);c->global_challenges=pe_get32(p+16);
 c->account_verifies=pe_get32(p+20);c->account_challenges=pe_get32(p+24);
 c->peer_verifies=pe_get32(p+28);c->peer_challenges=pe_get32(p+32);
}
static int budget(const struct pe_bindings *b)
{return b&&b->deadline>b->started&&b->deadline-b->started<=10000;}
int pe_encode_request(unsigned char out[PE_PACKET],size_t *bytes,
 const struct candidate_config *c,const struct camera_rtsp_registry *r,const struct pe_bindings *b)
{
 if(!out||!bytes||!budget(b)||pe_public_validate(c,r))return -1;
 const struct camera_graph *g=c->graph;
 /* Counts are bounded BEFORE arithmetic. Every record size is a wire constant. */
 size_t n=PE_PREFIX+144*g->device_count+128*g->association_count+204*g->profile_count+72*g->group_count;
 if(n>PE_MAX_REQUEST)return -1;
 memset(out,0,PE_PACKET);put32(out,PE_VERSION);put32(out+4,1);put32(out+8,(uint32_t)n);
 put64(out+16,b->started);put64(out+24,b->deadline);
 for(unsigned i=0;i<3;i++){put64(out+32+16*i,b->device[i]);put64(out+40+16*i,b->inode[i]);}
 put64(out+80,c->boot_generation);put64(out+88,c->epoch);put64(out+96,c->credential_generation);
 limits_encode(out+104,c);
 put32(out+140,(uint32_t)g->device_count);put32(out+144,(uint32_t)g->association_count);
 put32(out+148,(uint32_t)g->profile_count);put32(out+152,(uint32_t)g->group_count);
 memcpy(out+156,c->device_id,64);memcpy(out+220,c->account_id,64);memcpy(out+284,c->profile_id,64);
 memcpy(out+348,r->live_uri,513);memcpy(out+861,r->track_uri,513);
 unsigned char *p=out+PE_PREFIX;
 for(size_t i=0;i<g->device_count;i++,p+=144){
  const struct camera_device *d=&g->devices[i];memcpy(p,d->id,64);memcpy(p+64,d->resource_group,64);
  put32(p+128,(uint32_t)d->kind);put32(p+132,(uint32_t)d->state);put64(p+136,d->capabilities);
 }
 for(size_t i=0;i<g->association_count;i++,p+=128){memcpy(p,g->associations[i].camera_id,64);memcpy(p+64,g->associations[i].device_id,64);}
 for(size_t i=0;i<g->profile_count;i++,p+=204){
  const struct camera_profile *v=&g->profiles[i];memcpy(p,v->id,64);memcpy(p+64,v->camera_id,64);memcpy(p+128,v->encoder_id,64);
  put32(p+192,v->width);put32(p+196,v->height);put32(p+200,(uint32_t)v->codec);
 }
 for(size_t i=0;i<g->group_count;i++,p+=72){memcpy(p,g->groups[i].id,64);put32(p+64,g->groups[i].qualified_concurrent_sessions);put32(p+68,(uint32_t)g->groups[i].state);}
 *bytes=n;return 0;
}
int pe_decode_request(struct pe_public *v,const unsigned char *in,size_t n)
{
 if(!v||!in||n<PE_PREFIX||n>PE_MAX_REQUEST||pe_get32(in)!=PE_VERSION||
    pe_get32(in+4)!=1||pe_get32(in+8)!=n||pe_get32(in+12))return -1;
 uint32_t d=pe_get32(in+140),a=pe_get32(in+144),pcount=pe_get32(in+148),g=pe_get32(in+152);
 if(d>16||a>16||pcount>8||g>8||n!=PE_PREFIX+(size_t)d*144+(size_t)a*128+(size_t)pcount*204+(size_t)g*72)return -1;
 memset(v,0,sizeof(*v));struct candidate_config *c=&v->config;
 v->binding.started=pe_get64(in+16);v->binding.deadline=pe_get64(in+24);
 if(!budget(&v->binding))return -1;
 for(unsigned i=0;i<3;i++){v->binding.device[i]=pe_get64(in+32+16*i);v->binding.inode[i]=pe_get64(in+40+16*i);}
 c->abi=CANDIDATE_AUTH_ABI;c->struct_bytes=sizeof(*c);c->material_fd=-1;
 c->image.abi=RV_ABI;c->image.profile=RV_NATIVE;c->image.fd=-1;
 c->begin_contract=CANDIDATE_BEGIN_LOGICAL_ONLY; /* constructor projection, NOT verified executable */
 c->boot_generation=pe_get64(in+80);c->epoch=pe_get64(in+88);c->credential_generation=pe_get64(in+96);
 limits_decode(c,in+104);memcpy(c->device_id,in+156,64);memcpy(c->account_id,in+220,64);memcpy(c->profile_id,in+284,64);
 memcpy(v->routes.live_uri,in+348,513);memcpy(v->routes.track_uri,in+861,513);
 v->graph=(struct camera_graph){v->devices,d,v->associations,a,v->profiles,pcount,v->groups,g};c->graph=&v->graph;
 const unsigned char *p=in+PE_PREFIX;
 for(size_t i=0;i<d;i++,p+=144){
  uint32_t kind=pe_get32(p+128),state=pe_get32(p+132);
  if(kind<CAMERA_DEVICE_SENSOR||kind>CAMERA_DEVICE_STORAGE||state>CAMERA_QUALIFIED)return -1;
  memcpy(v->devices[i].id,p,64);memcpy(v->devices[i].resource_group,p+64,64);
  v->devices[i].kind=(enum camera_device_kind)kind;v->devices[i].state=(enum camera_evidence_state)state;v->devices[i].capabilities=pe_get64(p+136);
 }
 for(size_t i=0;i<a;i++,p+=128){memcpy(v->associations[i].camera_id,p,64);memcpy(v->associations[i].device_id,p+64,64);}
 for(size_t i=0;i<pcount;i++,p+=204){
  uint32_t codec=pe_get32(p+200);if(codec>CAMERA_H264)return -1;
  memcpy(v->profiles[i].id,p,64);memcpy(v->profiles[i].camera_id,p+64,64);memcpy(v->profiles[i].encoder_id,p+128,64);
  v->profiles[i].width=pe_get32(p+192);v->profiles[i].height=pe_get32(p+196);v->profiles[i].codec=(enum camera_codec)codec;
 }
 for(size_t i=0;i<g;i++,p+=72){
  uint32_t state=pe_get32(p+68);if(state>CAMERA_QUALIFIED)return -1;
  memcpy(v->groups[i].id,p,64);v->groups[i].qualified_concurrent_sessions=pe_get32(p+64);v->groups[i].state=(enum camera_evidence_state)state;
 }
 return pe_public_validate(c,&v->routes);
}
int pe_encode_reply(unsigned char out[PE_REPLY],const struct candidate_config *c,
 const struct camera_rtsp_registry *r,uint64_t device,uint64_t inode)
{
 if(!out||pe_public_validate(c,r))return -1;
 const struct camera_profile *selected=NULL;
 for(size_t i=0;i<c->graph->profile_count;i++)if(!memcmp(c->graph->profiles[i].id,c->profile_id,64))selected=&c->graph->profiles[i];
 if(!selected)return -1;
 memset(out,0,PE_REPLY);put64(out,c->boot_generation);put64(out+8,c->epoch);
 put64(out+16,c->credential_generation);put64(out+24,1);put64(out+32,device);put64(out+40,inode);
 put32(out+48,PE_VERSION);put32(out+52,RV_NATIVE);put32(out+56,RV_ABI);put32(out+60,1536);put32(out+64,CAMERA_ACCESS_RTSP);
 limits_encode(out+68,c);memcpy(out+104,c->device_id,64);memcpy(out+168,c->account_id,64);
 memcpy(out+232,selected->camera_id,64);memcpy(out+296,c->profile_id,64);memcpy(out+360,r->live_uri,513);memcpy(out+873,r->track_uri,513);
 return 0;
}
