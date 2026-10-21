// SPDX-License-Identifier: GPL-2.0-only
#include <camera/backend.h>
#include <string.h>
static int id_valid(const char *id)
{return id&&id[0]&&memchr(id,0,CAMERA_ID_BYTES)!=NULL;}
static const struct camera_device *find_device(const struct camera_graph *g,const char *id)
{
 for(size_t i=0;i<g->device_count;i++)if(!strcmp(g->devices[i].id,id))return &g->devices[i];
 return NULL;
}
static int has_group(const struct camera_graph *g,const char *id)
{
 for(size_t i=0;i<g->group_count;i++)if(!strcmp(g->groups[i].id,id))return 1;
 return 0;
}
int camera_backend_validate(const struct camera_backend *b)
{
 if(!b||b->abi!=CAMERA_BACKEND_ABI||b->struct_bytes!=sizeof(*b)||!b->name||!b->name[0]||!b->enumerate)return CAMERA_INVALID;
 if(b->capture&&(b->capture->abi!=CAMERA_BACKEND_ABI||b->capture->struct_bytes!=sizeof(*b->capture)||!b->capture->open||!b->capture->next||!b->capture->release||!b->capture->close))return CAMERA_INVALID;
 if(b->encoder&&(b->encoder->abi!=CAMERA_BACKEND_ABI||b->encoder->struct_bytes!=sizeof(*b->encoder)||!b->encoder->open||!b->encoder->encode||!b->encoder->release||!b->encoder->close))return CAMERA_INVALID;
 if(b->control&&(b->control->abi!=CAMERA_BACKEND_ABI||b->control->struct_bytes!=sizeof(*b->control)))return CAMERA_INVALID;
 if(b->radio&&(b->radio->abi!=CAMERA_BACKEND_ABI||b->radio->struct_bytes!=sizeof(*b->radio)||!b->radio->status))return CAMERA_INVALID;
 if(b->storage&&(b->storage->abi!=CAMERA_BACKEND_ABI||b->storage->struct_bytes!=sizeof(*b->storage)||!b->storage->status||!b->storage->recording_directory))return CAMERA_INVALID;
 return CAMERA_OK;
}
int camera_graph_validate(const struct camera_graph *g)
{
 if(!g||(g->device_count&&!g->devices)||(g->association_count&&!g->associations)||
    (g->profile_count&&!g->profiles)||(g->group_count&&!g->groups))return CAMERA_INVALID;
 for(size_t i=0;i<g->group_count;i++){
  const struct camera_resource_group *r=&g->groups[i];
  if(!id_valid(r->id)||r->state>CAMERA_QUALIFIED||r->state<0||
     (r->state!=CAMERA_QUALIFIED&&r->qualified_concurrent_sessions))return CAMERA_INVALID;
  for(size_t j=0;j<i;j++)if(!strcmp(r->id,g->groups[j].id))return CAMERA_INVALID;
 }
 for(size_t i=0;i<g->device_count;i++){
  const struct camera_device *d=&g->devices[i];
  if(!id_valid(d->id)||!memchr(d->resource_group,0,CAMERA_ID_BYTES)||d->kind<CAMERA_DEVICE_SENSOR||d->kind>CAMERA_DEVICE_STORAGE||d->state<0||d->state>CAMERA_QUALIFIED)return CAMERA_INVALID;
  if(d->resource_group[0]&&!has_group(g,d->resource_group))return CAMERA_INVALID;
  for(size_t j=0;j<i;j++)if(!strcmp(d->id,g->devices[j].id))return CAMERA_INVALID;
 }
 for(size_t i=0;i<g->association_count;i++){
  const struct camera_association *a=&g->associations[i];
  if(!id_valid(a->camera_id)||!id_valid(a->device_id))return CAMERA_INVALID;
  const struct camera_device *camera=find_device(g,a->camera_id),*device=find_device(g,a->device_id);
  if(!camera||camera->kind!=CAMERA_DEVICE_SENSOR||!device)return CAMERA_INVALID;
  for(size_t j=0;j<i;j++)if(!strcmp(a->camera_id,g->associations[j].camera_id)&&!strcmp(a->device_id,g->associations[j].device_id))return CAMERA_INVALID;
 }
 for(size_t i=0;i<g->profile_count;i++){
  const struct camera_profile *p=&g->profiles[i];
  if(!id_valid(p->id)||!id_valid(p->camera_id)||!memchr(p->encoder_id,0,CAMERA_ID_BYTES)||!p->width||!p->height||p->codec<CAMERA_PIXELS||p->codec>CAMERA_H264)return CAMERA_INVALID;
  const struct camera_device *camera=find_device(g,p->camera_id);
  if(!camera||camera->kind!=CAMERA_DEVICE_SENSOR)return CAMERA_INVALID;
  if(p->codec!=CAMERA_PIXELS){const struct camera_device *encoder=find_device(g,p->encoder_id);if(!encoder||encoder->kind!=CAMERA_DEVICE_ENCODER)return CAMERA_INVALID;}
  for(size_t j=0;j<i;j++)if(!strcmp(p->id,g->profiles[j].id))return CAMERA_INVALID;
 }
 return CAMERA_OK;
}
size_t camera_graph_sensor_count(const struct camera_graph *g)
{
 if(camera_graph_validate(g)!=CAMERA_OK)return 0;
 size_t n=0;for(size_t i=0;i<g->device_count;i++)n+=g->devices[i].kind==CAMERA_DEVICE_SENSOR&&g->devices[i].state!=CAMERA_UNAVAILABLE;
 return n;
}
int camera_control_stop(const struct camera_backend *b,const char *id)
{
 if(camera_backend_validate(b)!=CAMERA_OK||!id||!id[0])return CAMERA_INVALID;
 if(!b->control||!b->control->ptz_stop)return CAMERA_UNSUPPORTED;
 return b->control->ptz_stop(b->context,id);
}
