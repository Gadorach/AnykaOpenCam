/* SPDX-License-Identifier: GPL-2.0-only */
#include "reset_input.h"
#include <string.h>

static int overlap(const void *a,size_t an,const void *b,size_t bn)
{
 uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 if(!an||!bn)return 0;
 if(!a||!b||an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return 1;
 return x<y+bn&&y<x+an;
}
static int zero(const void *p,size_t n)
{
 const unsigned char *v=p;size_t i;
 for(i=0;i<n;i++)if(v[i])return 0;
 return 1;
}
static int live(const struct reset_input *e)
{
 if(!e||e->self!=e||e->disposed)return RI_INVALID;
 return e->busy?RI_BUSY:RI_OK;
}
static int disjoint(const struct reset_input *e,const void *p,size_t n)
{
 return p&&!overlap(e,sizeof(*e),p,n)&&
  (!e->provider.context||!overlap(e->provider.context,1,p,n));
}
static int config_valid(const struct ri_config *c)
{
 return c->debounce_ns>=RI_NS_PER_MS&&c->debounce_ns<=RI_DEBOUNCE_MAX_NS&&
  c->long_press_ns>=2*c->debounce_ns&&c->long_press_ns<=RI_LONG_MAX_NS&&
  c->stuck_cap_ns>c->long_press_ns+c->debounce_ns&&c->stuck_cap_ns<=RI_STUCK_MAX_NS&&
  c->fresh_gap_ns>=c->debounce_ns&&c->fresh_gap_ns<=RI_GAP_MAX_NS&&
  c->event_ttl_ns>=RI_NS_PER_MS&&c->event_ttl_ns<=RI_EVENT_TTL_MAX_NS;
}
static void disarm(struct reset_input *e,uint32_t reason)
{
 e->view.desired_state=RI_WAIT_RELEASE;e->view.reason=reason;
 e->view.event_queued=0;e->view.long_threshold_observed=0;e->view.press_since_ns=0;
 e->release_tracking=0;e->anchor_ns=0;e->press_since_ns=0;e->last_pressed_ns=0;
}
static int invalidate(struct reset_input *e,uint32_t reason,uint32_t validity,int fatal)
{
 disarm(e,reason);e->view.logical_observed=RI_LOGICAL_UNKNOWN;
 e->view.observation_validity=validity;e->raw_held_tracking=0;
 if(fatal){e->view.fault=1;e->view.desired_state=RI_FAILED_CLOSED;return RI_FAULT;}
 return RI_UNAVAILABLE;
}
static int tick(struct reset_input *e,uint64_t now)
{
 if(now<e->last_now_ns)return invalidate(e,RI_REASON_CLOCK,RI_OBS_FAULT,1);
 e->last_now_ns=now;e->view.last_received_ns=now;
 if(e->view.fault)return RI_FAULT;
 if(e->view.event_queued&&now>=e->event.expires_ns)disarm(e,RI_REASON_EXPIRED);
 if(e->view.observation_validity==RI_OBS_VALID_UNQUALIFIED&&
    now-e->last_sample_ns>e->config.fresh_gap_ns)
  return invalidate(e,RI_REASON_GAP,RI_OBS_STALE,0);
 return RI_OK;
}
int reset_input_init(struct reset_input *e,const struct ri_provider *p,
 const struct ri_config *c,uint64_t space,uint64_t generation,uint64_t now)
{
 struct reset_input candidate;
 if(!e||!p||!c||!space||!generation||overlap(e,sizeof(*e),p,sizeof(*p))||
    overlap(e,sizeof(*e),c,sizeof(*c))||!zero(e,sizeof(*e)))return RI_INVALID;
 if(!config_valid(c)||p->abi!=RESET_INPUT_ABI||p->struct_bytes!=sizeof(*p)||
    p->presence>RI_INPUT_PRESENT_UNQUALIFIED||
    (p->source_class!=RI_INPUT_ONLY_TRUSTED&&p->source_class!=RI_PUBLICDUMMY_INPUT_ONLY))return RI_INVALID;
 if(p->presence==RI_INPUT_PRESENT_UNQUALIFIED&&(!p->context||!p->read||
    overlap(e,sizeof(*e),p->context,1)))return RI_INVALID;
 memset(&candidate,0,sizeof(candidate));candidate.self=e;candidate.provider=*p;candidate.config=*c;
 candidate.namespace_epoch=space;candidate.engine_generation=generation;candidate.next_sequence=1;
 candidate.last_now_ns=now;candidate.view.last_received_ns=now;
 candidate.view.desired_state=RI_WAIT_RELEASE;candidate.view.reason=RI_REASON_STARTUP;
 candidate.view.logical_observed=RI_LOGICAL_UNKNOWN;candidate.view.logical_unit=RI_UNIT_LOGICAL_PRESSED;
 candidate.view.presence=p->presence;candidate.view.source_class=p->source_class;
 candidate.view.observation_validity=p->presence?RI_OBS_UNKNOWN:RI_OBS_ABSENT;
 *e=candidate;return RI_OK;
}
static int publish(struct reset_input *e,uint64_t sampled,uint64_t now)
{
 struct ri_event event;
 if(e->view.event_queued)return RI_BUSY;
 if(!e->next_sequence||e->next_sequence==UINT64_MAX||UINT64_MAX-now<e->config.event_ttl_ns){
  invalidate(e,RI_REASON_EXHAUSTED,RI_OBS_FAULT,1);return RI_EXHAUSTED;
 }
 memset(&event,0,sizeof(event));event.ticket.namespace_epoch=e->namespace_epoch;
 event.ticket.engine_generation=e->engine_generation;event.ticket.source_epoch=e->highest_source_epoch;
 event.ticket.sequence=e->next_sequence++;event.issued_ns=now;event.expires_ns=now+e->config.event_ttl_ns;
 event.stable_press_ns=e->press_since_ns;event.stable_release_ns=sampled;
 event.held_ns=e->last_pressed_ns-e->press_since_ns;
 event.kind=RI_DELIBERATE_LONG_PRESS_RELEASE;event.evidence=RI_PROVIDER_DECLARATION_ONLY;
 e->event=event;disarm(e,RI_REASON_NONE);e->view.event_queued=1;e->view.desired_state=RI_EVENT_PENDING;
 return RI_OK;
}
static int apply(struct reset_input *e,const struct ri_observation *o,uint64_t now)
{
 uint32_t logical=o->logical_pressed;int new_source=0;
 if(o->source_class!=e->provider.source_class||!o->source_epoch)
  return invalidate(e,RI_REASON_METADATA,RI_OBS_UNAVAILABLE,0);
 if(o->source_epoch!=e->highest_source_epoch){
  if(o->source_epoch<e->highest_source_epoch)return invalidate(e,RI_REASON_SOURCE_REUSE,RI_OBS_FAULT,1);
  e->highest_source_epoch=o->source_epoch;e->view.source_epoch=o->source_epoch;new_source=1;
  disarm(e,RI_REASON_SOURCE_CHANGE);e->raw_held_tracking=0;
 }
 if(o->valid!=1||logical>RI_PRESSED)return invalidate(e,RI_REASON_METADATA,RI_OBS_UNAVAILABLE,0);
 if(o->sampled_ns>now)return invalidate(e,RI_REASON_METADATA,RI_OBS_UNAVAILABLE,0);
 if(now-o->sampled_ns>e->config.fresh_gap_ns)return invalidate(e,RI_REASON_GAP,RI_OBS_STALE,0);
 if(e->have_sample&&o->sampled_ns<e->last_sample_ns)return invalidate(e,RI_REASON_CLOCK,RI_OBS_FAULT,1);
 if(e->have_sample&&o->sampled_ns==e->last_sample_ns){
  if(!new_source&&e->view.observation_validity==RI_OBS_VALID_UNQUALIFIED&&logical==e->view.logical_observed)return RI_OK;
  return invalidate(e,RI_REASON_CONFLICT,RI_OBS_UNAVAILABLE,0);
 }
 if(e->have_sample&&o->sampled_ns-e->last_sample_ns>e->config.fresh_gap_ns){
  /* The current fresh sample may START a new release debounce; it cannot
   * complete the interrupted gesture or pretend a missed interval was stable. */
  disarm(e,RI_REASON_GAP);e->raw_held_tracking=0;
 }
 e->have_sample=1;e->last_sample_ns=o->sampled_ns;e->view.sampled_ns=o->sampled_ns;
 e->view.logical_observed=logical;e->view.observation_validity=RI_OBS_VALID_UNQUALIFIED;
 if(logical==RI_PRESSED){
  if(!e->raw_held_tracking){e->raw_held_tracking=1;e->raw_held_since_ns=o->sampled_ns;}
  if(o->sampled_ns-e->raw_held_since_ns>=e->config.stuck_cap_ns){disarm(e,RI_REASON_STUCK);return RI_OK;}
 }else e->raw_held_tracking=0;
 /* While the one queue is occupied, a second gesture is never admitted.
  * Consumption/expiry still requires a NEW stable release before another press. */
 if(e->view.event_queued){e->view.desired_state=RI_EVENT_PENDING;return RI_OK;}
 if((e->view.desired_state==RI_HELD||e->view.desired_state==RI_RELEASE_DEBOUNCE)&&
    o->sampled_ns-e->press_since_ns>=e->config.stuck_cap_ns){disarm(e,RI_REASON_STUCK);return RI_OK;}
 switch(e->view.desired_state){
 case RI_WAIT_RELEASE:
  if(logical==RI_PRESSED){e->release_tracking=0;break;}
  if(!e->release_tracking){e->release_tracking=1;e->anchor_ns=o->sampled_ns;}
  if(o->sampled_ns-e->anchor_ns>=e->config.debounce_ns){
   e->release_tracking=0;e->view.desired_state=RI_ARMED;e->view.reason=RI_REASON_NONE;
  }
  break;
 case RI_ARMED:
  if(logical==RI_PRESSED){e->anchor_ns=o->sampled_ns;e->view.desired_state=RI_PRESS_DEBOUNCE;}
  break;
 case RI_PRESS_DEBOUNCE:
  if(logical==RI_RELEASED){e->view.desired_state=RI_ARMED;break;}
  if(o->sampled_ns-e->anchor_ns>=e->config.debounce_ns){
   e->press_since_ns=o->sampled_ns;e->last_pressed_ns=o->sampled_ns;
   e->view.press_since_ns=o->sampled_ns;e->view.desired_state=RI_HELD;
  }
  break;
 case RI_HELD:
  if(logical==RI_RELEASED){e->anchor_ns=o->sampled_ns;e->view.desired_state=RI_RELEASE_DEBOUNCE;break;}
  e->last_pressed_ns=o->sampled_ns;
  if(o->sampled_ns-e->press_since_ns>=e->config.long_press_ns)e->view.long_threshold_observed=1;
  break;
 case RI_RELEASE_DEBOUNCE:
  if(logical==RI_PRESSED){
   /* Short release bounce does not restart the original held/stuck timer. */
   e->view.desired_state=RI_HELD;e->last_pressed_ns=o->sampled_ns;
   if(o->sampled_ns-e->press_since_ns>=e->config.long_press_ns)e->view.long_threshold_observed=1;
   break;
  }
  if(o->sampled_ns-e->anchor_ns>=e->config.debounce_ns){
   if(e->view.long_threshold_observed)return publish(e,o->sampled_ns,now);
   disarm(e,RI_REASON_NONE);e->view.desired_state=RI_ARMED;
  }
  break;
 default:return invalidate(e,RI_REASON_METADATA,RI_OBS_FAULT,1);
 }
 return RI_OK;
}
int reset_input_poll(struct reset_input *e,uint64_t now)
{
 struct ri_observation observation;int ret=live(e);
 if(ret)return ret;
 ret=tick(e,now);
 if(ret==RI_FAULT)return ret;
 if(e->provider.presence==RI_INPUT_ABSENT)return invalidate(e,RI_REASON_PROVIDER,RI_OBS_ABSENT,0);
 memset(&observation,0,sizeof(observation));observation.logical_pressed=RI_LOGICAL_UNKNOWN;
 e->busy=1;ret=e->provider.read(e->provider.context,&observation);e->busy=0;
 if(ret!=RI_READ_OK)return invalidate(e,ret==RI_READ_EOF?RI_REASON_EOF:RI_REASON_PROVIDER,RI_OBS_UNAVAILABLE,0);
 return apply(e,&observation,now);
}
static int event_equal(const struct ri_event *a,const struct ri_event *b)
{
 return a->ticket.namespace_epoch==b->ticket.namespace_epoch&&a->ticket.engine_generation==b->ticket.engine_generation&&
  a->ticket.source_epoch==b->ticket.source_epoch&&a->ticket.sequence==b->ticket.sequence&&
  a->issued_ns==b->issued_ns&&a->expires_ns==b->expires_ns&&a->stable_press_ns==b->stable_press_ns&&
  a->stable_release_ns==b->stable_release_ns&&a->held_ns==b->held_ns&&a->kind==b->kind&&
  a->evidence==b->evidence&&a->native_qualified==b->native_qualified;
}
int reset_input_peek(struct reset_input *e,uint64_t now,struct ri_event *event)
{
 int ret=live(e);
 if(ret)return ret;
 if(!disjoint(e,event,sizeof(*event)))return RI_INVALID;
 ret=tick(e,now);if(ret==RI_FAULT)return ret;
 if(!e->view.event_queued||e->view.observation_validity!=RI_OBS_VALID_UNQUALIFIED)return RI_EMPTY;
 *event=e->event;return RI_OK;
}
int reset_input_consume(struct reset_input *e,const struct ri_event *receipt,uint64_t now)
{
 int ret=live(e);
 if(ret)return ret;
 if(!disjoint(e,receipt,sizeof(*receipt)))return RI_INVALID;
 /* Stale/modified/aliased incoming receipts cannot change current intent/time. */
 if(!e->view.event_queued||!event_equal(receipt,&e->event))return RI_STALE;
 ret=tick(e,now);if(ret==RI_FAULT)return ret;
 if(!e->view.event_queued||e->view.observation_validity!=RI_OBS_VALID_UNQUALIFIED)return RI_STALE;
 e->last_consumed=receipt->ticket;disarm(e,RI_REASON_CONSUMED);return RI_OK;
}
int reset_input_view(const struct reset_input *e,struct ri_view *view)
{
 if(!e||e->self!=e||!view||overlap(e,sizeof(*e),view,sizeof(*view))||
    (e->provider.context&&overlap(e->provider.context,1,view,sizeof(*view))))return RI_INVALID;
 if(e->busy)return RI_BUSY;
 *view=e->view;return RI_OK;
}
int reset_input_dispose(struct reset_input *e)
{
 int ret=live(e);
 if(ret)return ret;
 disarm(e,RI_REASON_NONE);e->disposed=1;e->view.desired_state=RI_DISPOSED;return RI_OK;
}
