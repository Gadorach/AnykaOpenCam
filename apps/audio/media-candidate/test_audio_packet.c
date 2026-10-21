/* SPDX-License-Identifier: GPL-2.0-only */
#define _POSIX_C_SOURCE 200809L
#include "audio_packet.h"
#include <camera/access_policy.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
extern unsigned char reference_mu_encode(int);
extern int reference_mu_decode(unsigned char);
static unsigned checks,suites;
#define CHECK(x) do {checks++;if(!(x)){fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x);exit(1);}} while(0)
#define BASE 1000000000ULL
struct fake {struct ac_request request[64];unsigned count;};
struct fixture {
 struct ac_core capture;struct ac_provider provider;struct ac_params params;struct ac_clock clock;struct fake fake;
 _Alignas(64) uint8_t ring[AC_RING_BYTES];struct am_packet packet;
 struct camera_access_policy policy;struct camera_access_principal principal;
 struct camera_access_request permission;uint64_t now;
 unsigned admission_calls,effects[5],deny_at,source_mutate_at,reenter_at;int reentrant_result;
 unsigned pre_calls,pre_deny_at,pre_mute_at,pre_generation_at,pre_revoke_after,
  revoke_at,wrong_domain_at,pre_delay_at,delay_at;
 uint64_t delay_ns,pre_delay_ns,admission_until;
 struct am_packet *active_consumer;
 unsigned pre_always_deny,revoke_after_at;
};
static void id(char *out,const char *text){memset(out,0,CAMERA_ID_BYTES);memcpy(out,text,strlen(text));}
static int provider_submit(void *context,const struct ac_request *q)
{struct fake *f=context;CHECK(f->count<64);f->request[f->count++]=*q;return AC_ACCEPTED;}
static uint64_t actual_now(void *context){return ((struct fixture *)context)->now;}
static uint64_t actual_until(const struct fixture *f)
{return f->admission_until&&f->admission_until<f->principal.fact.expires_ns?f->admission_until:f->principal.fact.expires_ns;}
static int preborrow(void *context,const struct am_borrow_identity *source,uint64_t now,uint64_t *until)
{
 struct fixture *f=context;int result;
 CHECK(source->stream_namespace==f->active_consumer->key.stream_namespace&&source->stream_generation==f->active_consumer->key.stream_generation);
 CHECK(source->capture_namespace==f->capture.namespace_epoch&&source->capture_generation==f->capture.generation);
 CHECK(source->resource_epoch==f->capture.clock.epoch&&source->track_generation==f->capture.track_generation);
 CHECK(source->rate_hz==8000&&source->channels==1&&source->sample_bits==16);
 f->pre_calls++;result=camera_access_check(&f->policy,&f->principal,&f->permission,now)==CAMERA_ACCESS_OK;
 *until=actual_until(f); /* actual issued-principal/admission window, not wire verified=1 */
 if(f->pre_delay_at==f->pre_calls)f->now+=f->pre_delay_ns;
 if(f->pre_mute_at==f->pre_calls)CHECK(ac_mute(&f->capture,1,f->now)==AC_RETAINED);
 if(f->pre_generation_at==f->pre_calls)f->capture.generation++;
 if(f->pre_revoke_after==f->pre_calls)CHECK(camera_access_revoke_session(&f->policy,&f->principal)==CAMERA_ACCESS_OK);
 return f->pre_always_deny||f->pre_deny_at==f->pre_calls?0:result;
}
static int admit(void *context,const struct am_key *key,uint64_t now,uint32_t effect,uint32_t offset,uint32_t bytes,uint64_t *until)
{
 struct fixture *f=context;int result;
 CHECK(effect>=AM_ENCODE&&effect<=AM_QUEUED_SEND);CHECK(offset<=AM_WIRE_BYTES&&bytes<=512);
 CHECK(ac_ticket_equal(&key->capture_ticket,&f->active_consumer->span.ticket));
 f->admission_calls++;f->effects[effect]++;
 if(f->revoke_at==f->admission_calls)CHECK(camera_access_revoke_session(&f->policy,&f->principal)==CAMERA_ACCESS_OK);
 if(f->wrong_domain_at==f->admission_calls)f->permission.domain=CAMERA_ACCESS_UI;
 result=camera_access_check(&f->policy,&f->principal,&f->permission,now)==CAMERA_ACCESS_OK;
 *until=actual_until(f);
 if(f->delay_at==f->admission_calls)f->now+=f->delay_ns;
 if(f->revoke_after_at==f->admission_calls)CHECK(camera_access_revoke_session(&f->policy,&f->principal)==CAMERA_ACCESS_OK);
 if(f->reenter_at==f->admission_calls)f->reentrant_result=am_cancel(f->active_consumer,now);
 if(f->source_mutate_at==f->admission_calls)CHECK(ac_mute(&f->capture,1,now)==AC_RETAINED);
 if(f->deny_at==f->admission_calls)return 0;
 return result;
}
static int ack(struct fixture *f)
{
 struct ac_request q=f->fake.request[f->fake.count-1];struct ac_completion r;unsigned i;
 memset(&r,0,sizeof(r));r.ticket=q.ticket;r.operation=q.operation;r.status=AC_ACK;f->now++;
 if(q.operation==AC_DMA_PERIOD){
  r.timestamp_valid=1;r.bytes=512;r.sample_index=f->capture.next_sample_index;
  r.first_sample_ns=BASE+r.sample_index*125000ULL;r.irq_ns=r.first_sample_ns+256*125000ULL;
  if(f->now<=r.irq_ns)f->now=r.irq_ns+1;
  for(i=0;i<256;i++){
   int32_t sample=(int32_t)((r.sample_index+i)*257%65536)-32768;unsigned word=(unsigned)(sample<0?sample+65536:sample);
   f->ring[q.offset+2*i]=(uint8_t)word;f->ring[q.offset+2*i+1]=(uint8_t)(word>>8);
  }
 }
 return ac_complete(&f->capture,&r,f->now);
}
static void capture_period(struct fixture *f,int first)
{
 CHECK((first?ac_start(&f->capture,++f->now):ac_pump(&f->capture,++f->now))==AC_OK);
 CHECK(ack(f)==AC_OK);CHECK(ack(f)==AC_OK);CHECK(ack(f)==AC_OK);
}
static void setup(struct fixture *f,uint32_t law,uint32_t timestamp,uint16_t sequence)
{
 struct camera_access_resource resources[3];struct camera_access_account account;struct camera_access_verified fact;
 struct am_seed seed={0x5055424c4943ULL,1,0x50554244U,timestamp,sequence};char device[CAMERA_ID_BYTES];
 memset(f,0,sizeof(*f));f->now=BASE;f->active_consumer=&f->packet;
 f->provider.abi=AC_ABI;f->provider.struct_bytes=sizeof(f->provider);f->provider.context=&f->fake;f->provider.submit=provider_submit;
 f->params.rate=8000;f->params.channels=1;f->params.sample_bits=16;f->params.period_bytes=512;f->params.periods=4;
 f->clock.parent_hz=98304000;f->clock.epoch=1;f->clock.ready_mask=AC_RESOURCE_REQUIRED;
 CHECK(ac_init(&f->capture,&f->provider,0x43415054555245ULL,f->now)==AC_OK);
 CHECK(ac_prepare(&f->capture,&f->params,&f->clock,f->ring,AC_RING_BYTES,1,f->now)==AC_OK);
 CHECK(ack(f)==AC_OK);CHECK(ack(f)==AC_OK);capture_period(f,1);
 memset(resources,0,sizeof(resources));resources[0].kind=CAMERA_ACCESS_DEVICE;id(resources[0].id,"PUBLICDUMMY-device");
 resources[1].kind=CAMERA_ACCESS_SENSOR;id(resources[1].id,"PUBLICDUMMY-camera0");
 resources[2].kind=CAMERA_ACCESS_PROFILE;id(resources[2].id,"PUBLICDUMMY-live-profile");id(resources[2].sensor_id,"PUBLICDUMMY-camera0");
 id(device,"PUBLICDUMMY-device");CHECK(camera_access_init(&f->policy,device,77,f->now,resources,3)==CAMERA_ACCESS_OK);
 memset(&account,0,sizeof(account));account.abi=CAMERA_ACCESS_ABI;account.struct_bytes=sizeof(account);account.domain=CAMERA_ACCESS_RTSP;
 account.roles=CAMERA_ACCESS_VIEWER;account.credential_generation=1;account.grant_count=1;id(account.account_id,"PUBLICDUMMY-viewer");
 account.grants[0].kind=CAMERA_ACCESS_PROFILE;account.grants[0].operations=CAMERA_ACCESS_LIVE;id(account.grants[0].resource_id,"PUBLICDUMMY-live-profile");
 CHECK(camera_access_put_account(&f->policy,&account)==CAMERA_ACCESS_OK);
 memset(&fact,0,sizeof(fact));fact.abi=CAMERA_ACCESS_ABI;fact.struct_bytes=sizeof(fact);fact.verified=1;fact.domain=CAMERA_ACCESS_RTSP;
 fact.boot_generation=77;fact.credential_generation=1;fact.session_generation=1;fact.issued_ns=f->now;fact.expires_ns=f->now+10000000000ULL;
 id(fact.device_id,"PUBLICDUMMY-device");id(fact.account_id,"PUBLICDUMMY-viewer");
 CHECK(camera_access_issue(&f->policy,&fact,f->now,&f->principal)==CAMERA_ACCESS_OK);
 memset(&f->permission,0,sizeof(f->permission));f->permission.abi=CAMERA_ACCESS_ABI;f->permission.struct_bytes=sizeof(f->permission);
 f->permission.domain=CAMERA_ACCESS_RTSP;f->permission.kind=CAMERA_ACCESS_PROFILE;f->permission.operation=CAMERA_ACCESS_LIVE;
 id(f->permission.device_id,"PUBLICDUMMY-device");id(f->permission.sensor_id,"PUBLICDUMMY-camera0");id(f->permission.profile_id,"PUBLICDUMMY-live-profile");
 CHECK(am_init(&f->packet,&f->capture,preborrow,admit,actual_now,f,&seed,law,f->now)==AM_OK);
}
static uint32_t be32(const uint8_t *b){return ((uint32_t)b[0]<<24)|((uint32_t)b[1]<<16)|((uint32_t)b[2]<<8)|b[3];}
static void drain(struct fixture *f,const struct am_key *key,uint8_t wire[AM_WIRE_BYTES],size_t cap,unsigned partial)
{
 struct am_chunk chunk;uint8_t bytes[AM_EFFECT_MAX];int ret;
 while(f->packet.cursor<AM_WIRE_BYTES){
  CHECK(am_offer(&f->packet,key,++f->now,bytes,cap,&chunk)==AM_OK);
  CHECK(am_recheck(&f->packet,&chunk,f->now)==AM_OK);
  if(partial&&chunk.bytes>1){
   memcpy(wire+chunk.offset,bytes,1);CHECK(am_advance(&f->packet,&chunk,1,f->now)==AM_OK);
  }else{
   memcpy(wire+chunk.offset,bytes,chunk.bytes);ret=am_advance(&f->packet,&chunk,chunk.bytes,f->now);
   CHECK(ret==(f->packet.cursor==AM_WIRE_BYTES?AM_COMPLETE:AM_OK));
  }
 }
}
static void codec_vectors(const char *directory)
{
 char path[2048];uint8_t oracle[65536],decode[512];uint32_t law;int sample;unsigned i;FILE *file;
 suites++;
 for(law=G711_MULAW;law<=G711_ALAW;law++){
  snprintf(path,sizeof(path),"%s/%s-encode-oracle.bin",directory,law==G711_MULAW?"mu":"a");file=fopen(path,"rb");CHECK(file!=NULL);
  CHECK(fread(oracle,1,sizeof(oracle),file)==sizeof(oracle));CHECK(fclose(file)==0);
  snprintf(path,sizeof(path),"%s/%s-decode-oracle.s16le",directory,law==G711_MULAW?"mu":"a");file=fopen(path,"rb");CHECK(file!=NULL);
  CHECK(fread(decode,1,sizeof(decode),file)==sizeof(decode));CHECK(fclose(file)==0);
  for(sample=-32768;sample<32768;sample++){
   uint8_t code=law==G711_MULAW?g711_encode_mu((int16_t)sample):g711_encode_a((int16_t)sample);
   CHECK(code==oracle[sample+32768]);
   if(law==G711_MULAW)CHECK(code==reference_mu_encode(sample));
  }
  for(i=0;i<256;i++){
   int16_t v=law==G711_MULAW?g711_decode_mu((uint8_t)i):g711_decode_a((uint8_t)i);
   uint16_t bits=(uint16_t)v;CHECK(decode[2*i]==(uint8_t)bits&&decode[2*i+1]==(uint8_t)(bits>>8));
   if(law==G711_MULAW)CHECK(v==reference_mu_decode((unsigned char)i));
  }
 }
 CHECK(g711_encode_mu(0)==0xff&&g711_encode_mu(-32768)==0&&g711_encode_mu(32767)==0x80);
 CHECK(g711_decode_mu(0)==-32124&&g711_decode_mu(0x80)==32124&&g711_decode_mu(0x7f)==0);
 CHECK(g711_encode_a(0)==0xd5&&g711_encode_a(-1)==0x55&&g711_decode_a(0xd5)==8);
 {uint8_t pcm[512]={0},codes[256];memset(codes,0xa7,sizeof(codes));
  CHECK(g711_encode_s16le(G711_MULAW,pcm,511,codes,sizeof(codes))==-1);
  CHECK(g711_encode_s16le(99,pcm,512,codes,sizeof(codes))==-1);CHECK(codes[0]==0xa7);
  CHECK(g711_encode_s16le(G711_MULAW,pcm,512,pcm,256)==-1);}
}
static void packet_laws_chunks_wrap(void)
{
 struct fixture f;struct am_key key,old;uint8_t wire[AM_WIRE_BYTES];unsigned law,packet,cap,partial,i;uint64_t ts;
 suites++;
 for(law=G711_MULAW;law<=G711_ALAW;law++)for(cap=1;cap<=64;cap*=4)for(partial=0;partial<2;partial++){
  setup(&f,law,UINT32_MAX-100,65535);
  for(packet=0;packet<3;packet++){
   CHECK(am_begin(&f.packet,&key,++f.now)==AM_OK);old=key;
   CHECK(f.capture.borrowed==1&&f.packet.owned);
   CHECK(am_done(&f.packet,&key,f.now)==AM_BUSY);
   drain(&f,&key,wire,cap,partial);ts=(uint64_t)UINT32_MAX-100+(uint64_t)packet*256;
   CHECK(wire[0]==0x80&&wire[1]==(law==G711_MULAW?0:8));
   CHECK((((unsigned)wire[2]<<8)|wire[3])==(uint16_t)(65535+packet));
   CHECK(be32(wire+4)==(uint32_t)ts&&f.packet.rtp_ext==ts&&be32(wire+8)==0x50554244U);
   CHECK(f.packet.span.sample_index==(uint64_t)packet*256&&f.packet.span.monotonic_ns==BASE+(uint64_t)packet*32000000);
   for(i=0;i<256;i++){
    const uint8_t *b=f.packet.span.data+2*i;unsigned word=b[0]|((unsigned)b[1]<<8);int32_t value=word>=32768?(int32_t)word-65536:(int32_t)word;
    CHECK(wire[12+i]==(law==G711_MULAW?g711_encode_mu((int16_t)value):g711_encode_a((int16_t)value)));
   }
   CHECK(am_done(&f.packet,&key,++f.now)==AM_OK);CHECK(!f.packet.owned&&f.capture.borrowed==0);
   CHECK(am_done(&f.packet,&old,f.now)==AM_STALE);CHECK(f.capture.state==AC_CAPTURING&&!f.capture.closing);
   if(packet<2)capture_period(&f,0);
  }
 }
}
static void effects_revoke_mute_expire(void)
{
 struct fixture f;struct am_key key;struct am_chunk chunk;uint8_t bytes[64];unsigned effect,mode,count;
 suites++;
 for(effect=AM_ENCODE;effect<=AM_QUEUED_SEND;effect++)for(mode=0;mode<4;mode++){
  setup(&f,G711_MULAW,0,1);
  if(effect==AM_ENCODE){
   if(mode==0)f.deny_at=1;
    if(mode==1)f.revoke_at=1; /* genuine race AFTER successful pre-borrow checks */
   if(mode==2)f.source_mutate_at=1;
    if(mode==3)f.wrong_domain_at=1;
   CHECK(am_begin(&f.packet,&key,++f.now)==AM_DENIED);
  }else{
   CHECK(am_begin(&f.packet,&key,++f.now)==AM_OK);
   if(effect==AM_PAYLOAD){
    CHECK(am_offer(&f.packet,&key,++f.now,bytes,64,&chunk)==AM_OK);
    CHECK(am_recheck(&f.packet,&chunk,f.now)==AM_OK);CHECK(am_advance(&f.packet,&chunk,chunk.bytes,f.now)==AM_OK);
   }
   if(effect==AM_QUEUED_SEND)CHECK(am_offer(&f.packet,&key,++f.now,bytes,64,&chunk)==AM_OK);
   if(mode==0)f.deny_at=f.admission_calls+1;
   if(mode==1)CHECK(camera_access_revoke_session(&f.policy,&f.principal)==CAMERA_ACCESS_OK);
   if(mode==2)f.source_mutate_at=f.admission_calls+1;
   if(mode==3)f.now=f.principal.fact.expires_ns;
   memset(bytes,0xa7,sizeof(bytes));
   CHECK((effect==AM_QUEUED_SEND?am_recheck(&f.packet,&chunk,++f.now):am_offer(&f.packet,&key,++f.now,bytes,64,&chunk))==AM_DENIED);
   CHECK(bytes[0]==0xa7);
  }
  CHECK(f.packet.invalid&&f.packet.owned&&f.capture.borrowed==1);
  for(count=0;count<256;count++)CHECK(f.packet.codes[count]==0);
  count=f.fake.count;CHECK(am_cancel(&f.packet,++f.now)==AM_OK);
  CHECK(!f.packet.owned&&f.capture.borrowed==0);
  CHECK(am_cancel(&f.packet,f.now)==AM_STALE);CHECK(f.fake.count==count); /* no EOF-driven source STOP */
 }
 setup(&f,G711_ALAW,0,1);f.reenter_at=1;
 CHECK(am_begin(&f.packet,&key,++f.now)==AM_OK);CHECK(f.reentrant_result==AM_INVALID);
 CHECK(am_cancel(&f.packet,++f.now)==AM_OK);
}
static void invalid_formats_aliases_tokens_and_clock(void)
{
 struct fixture f;struct am_key key,wrong;struct am_chunk chunk,bad;struct am_packet before,copied;struct am_format fmt;
 uint8_t bytes[64];unsigned i,count;
 suites++;CHECK(am_format_select(0x5350440dU,&fmt)==AM_FORMAT); /* no string/SDP injection API */
 for(i=0;i<5;i++){
  setup(&f,G711_MULAW,0,1);
  if(i==0)f.capture.params.rate=16000;
  if(i==1)f.capture.params.rate=48000;
  if(i==2)f.capture.params.channels=2;
  if(i==3)f.capture.params.sample_bits=8;
  if(i==4)f.capture.params.rate=256000;
  CHECK(am_begin(&f.packet,&key,++f.now)==AM_FORMAT);CHECK(f.capture.borrowed==0);
 }
 setup(&f,G711_MULAW,0,1);CHECK(am_begin(&f.packet,&key,++f.now)==AM_OK);
 for(i=0;i<4;i++){
  wrong=key;
  if(i==0)wrong.stream_namespace++;
  if(i==1)wrong.stream_generation++;
  if(i==2)wrong.capture_ticket.resource_epoch++;
  if(i==3)wrong.packet_serial++;
  memcpy(&before,&f.packet,sizeof(before));
  CHECK(am_offer(&f.packet,&wrong,f.now,bytes,64,&chunk)==AM_STALE);CHECK(!memcmp(&before,&f.packet,sizeof(before)));
 }
 CHECK(am_offer(&f.packet,&key,++f.now,bytes,64,&chunk)==AM_OK);
 bad=chunk;bad.chunk_serial++;CHECK(am_recheck(&f.packet,&bad,f.now)==AM_STALE);
 CHECK(am_advance(&f.packet,&chunk,chunk.bytes+1,f.now)==AM_INVALID);
 CHECK(am_offer(&f.packet,&key,f.now,bytes,64,&bad)==AM_BUSY);
 CHECK(am_recheck(&f.packet,&f.packet.offered,f.now)==AM_INVALID);
 CHECK(am_advance(&f.packet,&chunk,0,f.now)==AM_OK);CHECK(f.packet.offered_valid);
 CHECK(am_advance(&f.packet,&chunk,1,f.now)==AM_OK);CHECK(am_recheck(&f.packet,&chunk,f.now)==AM_STALE);
 copied=f.packet;CHECK(am_cancel(&copied,f.now)==AM_INVALID);
 CHECK(am_offer(&f.packet,&key,f.now,f.packet.codes,64,&chunk)==AM_INVALID);
 count=f.fake.count;CHECK(am_cancel(&f.packet,f.capture.last_ns-1)==AM_CLOCK);CHECK(f.fake.count==count&&f.packet.owned);
 CHECK(am_cancel(&f.packet,++f.now)==AM_OK&&f.fake.count==count);
 setup(&f,G711_MULAW,0,1);CHECK(am_begin(&f.packet,&key,++f.now)==AM_OK);
 CHECK(am_offer(&f.packet,&key,f.now-1,bytes,64,&chunk)==AM_CLOCK);
 CHECK(f.packet.clock_failed&&f.capture.borrowed==1);CHECK(am_cancel(&f.packet,++f.now)==AM_OK);
}
static void source_generation_timestamps_and_returns(void)
{
 struct fixture f;struct am_key key;struct am_chunk chunk;uint8_t bytes[64],wire[AM_WIRE_BYTES];unsigned i,count;
 suites++;
 for(i=0;i<4;i++){
  setup(&f,G711_MULAW,UINT32_MAX,1);CHECK(am_begin(&f.packet,&key,++f.now)==AM_OK);
  drain(&f,&key,wire,64,0);CHECK(am_done(&f.packet,&key,++f.now)==AM_OK);capture_period(&f,0);
  if(i==0)f.packet.expected_index++;
  if(i==1)f.packet.resource_epoch++;
  if(i==2)f.packet.track_generation++;
  if(i==3)f.packet.capture_generation++;
  CHECK(am_begin(&f.packet,&key,++f.now)==AM_STALE);CHECK(am_cancel(&f.packet,++f.now)==AM_OK);
 }
 setup(&f,G711_MULAW,0,1);f.capture.slots[0].pts_ns=UINT64_MAX;
 CHECK(am_begin(&f.packet,&key,++f.now)==AM_DENIED);CHECK(am_cancel(&f.packet,++f.now)==AM_OK);
 setup(&f,G711_MULAW,0,1);CHECK(am_begin(&f.packet,&key,++f.now)==AM_OK);
 f.packet.span.bytes=511;memset(bytes,0xa7,sizeof(bytes));
 CHECK(am_offer(&f.packet,&key,++f.now,bytes,64,&chunk)==AM_DENIED);CHECK(bytes[0]==0xa7);
 CHECK(am_cancel(&f.packet,++f.now)==AM_OK);
 setup(&f,G711_MULAW,0,1);f.packet.packet_serial=UINT64_MAX;
 CHECK(am_begin(&f.packet,&key,++f.now)==AM_EXHAUSTED&&f.capture.borrowed==0);
 setup(&f,G711_MULAW,0,1);CHECK(am_begin(&f.packet,&key,++f.now)==AM_OK);
 f.packet.next_chunk_serial=UINT64_MAX;CHECK(am_offer(&f.packet,&key,++f.now,bytes,64,&chunk)==AM_EXHAUSTED);
 CHECK(am_cancel(&f.packet,++f.now)==AM_OK);
 setup(&f,G711_MULAW,0,1);CHECK(am_begin(&f.packet,&key,++f.now)==AM_OK);
 CHECK(ac_close(&f.capture,++f.now)==AC_RETAINED);CHECK(ack(&f)==AC_RETAINED);
 count=f.fake.count;CHECK(f.capture.stop_acked&&f.capture.borrowed==1&&!ac_can_free(&f.capture));
 CHECK(am_cancel(&f.packet,++f.now)==AM_OK);CHECK(f.fake.count==count+1);
 CHECK(f.fake.request[count].operation==AC_RELEASE);CHECK(ack(&f)==AC_OK);
 CHECK(ac_can_free(&f.capture));CHECK(am_cancel(&f.packet,++f.now)==AM_STALE);
}
static void near_max_time_and_unknown_return(void)
{
 struct fixture f;struct am_key key;struct camera_access_verified fact;uint8_t wire[AM_WIRE_BYTES];unsigned packet,count;
 suites++;setup(&f,G711_MULAW,UINT32_MAX,65535);
 CHECK(camera_access_revoke_session(&f.policy,&f.principal)==CAMERA_ACCESS_OK);
 fact=f.principal.fact;fact.session_generation=2;fact.expires_ns=UINT64_MAX;
 CHECK(camera_access_issue(&f.policy,&fact,f.now,&f.principal)==CAMERA_ACCESS_OK);
 f.now=UINT64_MAX-1000;f.capture.last_ns=f.now;
 f.capture.epoch_ns=UINT64_MAX-40000000ULL;
 f.capture.slots[0].pts_ns=UINT64_MAX-40000000ULL;
 for(packet=0;packet<2;packet++){
  CHECK(am_begin(&f.packet,&key,++f.now)==AM_OK);drain(&f,&key,wire,64,0);
  CHECK(am_done(&f.packet,&key,++f.now)==AM_OK);
  /* Deliberate white-box source timeline fault fixture; no physical timestamp
   * or capture operation is inferred from this boundary construction. */
  f.capture.slots[packet+1].ticket=f.capture.slots[0].ticket;
  f.capture.slots[packet+1].ticket.sequence+=packet+1;
  f.capture.slots[packet+1].state=AC_SLOT_READY;
  f.capture.slots[packet+1].sample_index=(uint64_t)(packet+1)*256;
  f.capture.slots[packet+1].pts_ns=UINT64_MAX-40000000ULL+(uint64_t)(packet+1)*32000000ULL;
 }
 CHECK(am_begin(&f.packet,&key,++f.now)==AM_DENIED);CHECK(am_cancel(&f.packet,++f.now)==AM_OK);
 setup(&f,G711_MULAW,0,1);CHECK(am_begin(&f.packet,&key,++f.now)==AM_OK);
 /* Contract-violating external borrower return: helper must not silently
  * declare/duplicate its own return or infer source retirement. */
 CHECK(ac_return(&f.capture,&f.packet.span.ticket,++f.now)==AC_OK);count=f.fake.count;
 CHECK(am_cancel(&f.packet,++f.now)==AM_RETAINED);CHECK(f.packet.owned&&f.packet.return_attempted);
 CHECK(am_cancel(&f.packet,++f.now)==AM_RETAINED&&f.fake.count==count);
 CHECK(!f.capture.closing&&f.capture.state==AC_CAPTURING);
}
static void current_source_epoch_continuations(void)
{
 struct fixture f;struct am_key key;struct am_chunk chunk;uint8_t out[64];unsigned mutation,count;
 suites++;
 for(mutation=0;mutation<7;mutation++){
  setup(&f,G711_MULAW,0,1);CHECK(am_begin(&f.packet,&key,++f.now)==AM_OK);
  CHECK(am_offer(&f.packet,&key,++f.now,out,64,&chunk)==AM_OK);
  if(mutation==0)f.capture.clock.epoch++;
  if(mutation==1)f.capture.generation++;
  if(mutation==2)f.capture.track_generation++;
  if(mutation==3)f.capture.namespace_epoch++;
  if(mutation==4)f.capture.params.rate=16000;
  if(mutation==5)f.capture.clock.parent_hz++;
  if(mutation==6)f.capture.epoch_ns++;
  CHECK(am_recheck(&f.packet,&chunk,++f.now)==AM_DENIED);CHECK(f.packet.owned&&f.capture.borrowed==1);
  count=f.fake.count;CHECK(am_cancel(&f.packet,++f.now)==AM_OK);CHECK(f.fake.count==count&&!f.capture.closing);
 }
}
static void fresh_viewer(struct fixture *f)
{
 struct camera_access_account a;struct camera_access_verified v;
 memset(&a,0,sizeof(a));a.abi=CAMERA_ACCESS_ABI;a.struct_bytes=sizeof(a);a.domain=CAMERA_ACCESS_RTSP;
 a.roles=CAMERA_ACCESS_VIEWER;a.credential_generation=1;a.grant_count=1;id(a.account_id,"PUBLICDUMMY-viewer");
 a.grants[0].kind=CAMERA_ACCESS_PROFILE;a.grants[0].operations=CAMERA_ACCESS_LIVE;id(a.grants[0].resource_id,"PUBLICDUMMY-live-profile");
 CHECK(camera_access_put_account(&f->policy,&a)==CAMERA_ACCESS_OK);
 memset(&v,0,sizeof(v));v.abi=CAMERA_ACCESS_ABI;v.struct_bytes=sizeof(v);v.verified=1;v.domain=CAMERA_ACCESS_RTSP;
 v.boot_generation=77;v.credential_generation=1;v.session_generation=2;v.issued_ns=f->now;v.expires_ns=f->now+1000000000ULL;
 id(v.device_id,"PUBLICDUMMY-device");id(v.account_id,"PUBLICDUMMY-viewer");
 CHECK(camera_access_issue(&f->policy,&v,f->now,&f->principal)==CAMERA_ACCESS_OK);
}
static void wrong_role_principal(struct fixture *f)
{
 struct camera_access_account a;struct camera_access_verified v;
 memset(&a,0,sizeof(a));a.abi=CAMERA_ACCESS_ABI;a.struct_bytes=sizeof(a);a.domain=CAMERA_ACCESS_UI;
 a.roles=CAMERA_ACCESS_OPERATOR;a.credential_generation=1;a.grant_count=1;id(a.account_id,"PUBLICDUMMY-operator-no-live");
 a.grants[0].kind=CAMERA_ACCESS_SENSOR;a.grants[0].operations=CAMERA_ACCESS_IMAGING_CONFIG;id(a.grants[0].resource_id,"PUBLICDUMMY-camera0");
 CHECK(camera_access_put_account(&f->policy,&a)==CAMERA_ACCESS_OK);
 memset(&v,0,sizeof(v));v.abi=CAMERA_ACCESS_ABI;v.struct_bytes=sizeof(v);v.verified=1;v.domain=CAMERA_ACCESS_UI;
 v.boot_generation=77;v.credential_generation=1;v.session_generation=1;v.issued_ns=f->now;v.expires_ns=f->now+1000000000ULL;
 id(v.device_id,"PUBLICDUMMY-device");id(v.account_id,"PUBLICDUMMY-operator-no-live");
 CHECK(camera_access_issue(&f->policy,&v,f->now,&f->principal)==CAMERA_ACCESS_OK);f->permission.domain=CAMERA_ACCESS_UI;
}
static void preborrow_authority_and_legitimate_peer(void)
{
 struct fixture f;struct am_packet helper_before,peer;struct ac_core capture_before;
 struct am_key key,sentinel;struct camera_access_request good_permission;unsigned mode,attempt,slot,count;
 suites++;
 for(mode=0;mode<7;mode++){
  setup(&f,G711_MULAW,0,1);for(slot=1;slot<AC_SLOTS;slot++)capture_period(&f,0);
  good_permission=f.permission;
  if(mode==0)memset(&f.principal,0,sizeof(f.principal)); /* unauthenticated guest */
  if(mode==1)CHECK(camera_access_revoke_session(&f.policy,&f.principal)==CAMERA_ACCESS_OK);
  if(mode==2)f.now=f.principal.fact.expires_ns; /* exact expiry boundary */
  if(mode==3)wrong_role_principal(&f);
  if(mode==4)id(f.permission.profile_id,"PUBLICDUMMY-stale-profile");
  if(mode==5)f.permission.domain=CAMERA_ACCESS_UI;
  if(mode==6)f.pre_always_deny=1; /* valid real policy, resource admission denies */
  memcpy(&capture_before,&f.capture,sizeof(capture_before));memcpy(&helper_before,&f.packet,sizeof(helper_before));
  memset(&sentinel,0xa7,sizeof(sentinel));count=f.fake.count;
  for(attempt=0;attempt<128;attempt++){
   key=sentinel;
   CHECK(am_begin(&f.packet,&key,f.now)==AM_DENIED);
   if(f.capture.borrowed)fprintf(stderr,"pre-borrow authority regression mode=%u borrowed=%u packet_serial=%llu\n",mode,f.capture.borrowed,(unsigned long long)f.packet.packet_serial);
   CHECK(!memcmp(&key,&sentinel,sizeof(key)));
   CHECK(!memcmp(&f.capture,&capture_before,sizeof(capture_before)));
   CHECK(!memcmp(&f.packet,&helper_before,sizeof(helper_before)));
   CHECK(f.fake.count==count&&f.capture.borrowed==0&&f.admission_calls==0);
   for(slot=0;slot<AC_SLOTS;slot++)CHECK(f.capture.slots[slot].state==AC_SLOT_READY);
  }
  /* No starvation: all four READY slots are still available to an independently
   * authorized peer on the same actual capture owner. No old helper reset. */
  fresh_viewer(&f);f.permission=good_permission;f.pre_always_deny=0;memset(&peer,0,sizeof(peer));f.active_consumer=&peer;
  {struct am_seed seed={0x50554250454552ULL,1,0x50554250U,0,1};
   CHECK(am_init(&peer,&f.capture,preborrow,admit,actual_now,&f,&seed,G711_MULAW,f.now)==AM_OK);}
  CHECK(am_begin(&peer,&key,f.now)==AM_OK);CHECK(f.capture.borrowed==1);
  CHECK(am_cancel(&peer,f.now)==AM_OK&&f.capture.borrowed==0&&f.fake.count==count);
  CHECK(!memcmp(&f.packet,&helper_before,sizeof(helper_before)));
 }
 setup(&f,G711_MULAW,0,1);memcpy(&capture_before,&f.capture,sizeof(capture_before));memcpy(&helper_before,&f.packet,sizeof(helper_before));
 CHECK(am_begin(&f.packet,NULL,f.now)==AM_INVALID);CHECK(am_begin(&f.packet,&f.packet.key,f.now)==AM_INVALID);
 CHECK(f.pre_calls==0&&f.admission_calls==0&&!memcmp(&capture_before,&f.capture,sizeof(capture_before)));
 CHECK(!memcmp(&helper_before,&f.packet,sizeof(helper_before)));
 printf("preborrow=PASS real guest/revoked/expired/wrong-role/stale-profile/resource-denial no loan/key/counter mutation;4 READY slots and legit peer preserved\n");
}
static void callback_freshness_and_preborrow_races(void)
{
 struct fixture f;struct am_packet helper_before;struct ac_core capture_before;
 struct am_key key;struct am_chunk chunk;uint8_t bytes[64];unsigned mode,count;
 suites++;
 for(mode=0;mode<7;mode++){
  setup(&f,G711_MULAW,0,1);memcpy(&helper_before,&f.packet,sizeof(helper_before));
  if(mode==0)f.pre_generation_at=1;
  if(mode==1)f.pre_mute_at=1;
  if(mode==2)f.pre_revoke_after=1; /* first cached true is not final authority */
  if(mode==3){f.admission_until=f.now+5;f.pre_delay_at=1;f.pre_delay_ns=5;}
  if(mode==4){f.pre_delay_at=1;f.pre_delay_ns=AM_CALLBACK_BOUND_NS+1;}
  if(mode==5)f.pre_generation_at=2;
  if(mode==6)f.pre_mute_at=2;
  CHECK(am_begin(&f.packet,&key,f.now)==AM_DENIED);
  CHECK(f.capture.borrowed==0&&!f.packet.owned&&f.admission_calls==0);
  CHECK(!memcmp(&f.packet,&helper_before,sizeof(helper_before)));
  if(mode!=1&&mode!=6)CHECK(f.fake.count==5); /* source mute is its OWNER's action only */
 }
 setup(&f,G711_MULAW,0,1);f.pre_delay_at=1;f.pre_delay_ns=5;
 CHECK(am_begin(&f.packet,&key,f.now)==AM_OK);
 CHECK(f.policy.last_ns==f.now&&f.packet.last_ns==f.now&&f.pre_calls==2);
 CHECK(am_cancel(&f.packet,f.now)==AM_OK);
 for(mode=0;mode<5;mode++){
  setup(&f,G711_MULAW,0,1);CHECK(am_begin(&f.packet,&key,f.now)==AM_OK);
  f.delay_at=f.admission_calls+1;
  if(mode==0){f.admission_until=f.now+5;f.delay_ns=5;}
  if(mode==1)f.delay_ns=AM_CALLBACK_BOUND_NS+1;
  if(mode==2)f.delay_ns=5;
  if(mode==3)f.delay_ns=0;
  if(mode==4){f.delay_ns=0;f.revoke_after_at=f.admission_calls+1;}
  memset(bytes,0xa7,sizeof(bytes));count=f.fake.count;
  if(mode<2||mode==4){
   CHECK(am_offer(&f.packet,&key,f.now,bytes,64,&chunk)==AM_DENIED);
   CHECK(bytes[0]==0xa7&&f.packet.owned&&f.capture.borrowed==1);
  }else{
   CHECK(am_offer(&f.packet,&key,f.now,bytes,64,&chunk)==AM_OK);
   CHECK(f.policy.last_ns==f.now&&f.packet.last_ns==f.now);
   CHECK(am_recheck(&f.packet,&chunk,f.now)==AM_OK);
  }
  CHECK(am_cancel(&f.packet,f.now)==AM_OK&&f.fake.count==count&&!f.capture.closing);
 }
 setup(&f,G711_MULAW,0,1);memcpy(&helper_before,&f.packet,sizeof(helper_before));memcpy(&capture_before,&f.capture,sizeof(capture_before));
 f.now--;CHECK(am_begin(&f.packet,&key,f.now)==AM_CLOCK);
 CHECK(!memcmp(&f.packet,&helper_before,sizeof(helper_before))&&!memcmp(&f.capture,&capture_before,sizeof(capture_before)));
}
static void vector_output(const char *directory)
{
 char path[2048];FILE *f;uint8_t codes[256],decoded[512];unsigned law,i;
 for(i=0;i<256;i++)codes[i]=(uint8_t)i;
 for(law=G711_MULAW;law<=G711_ALAW;law++){
  const char *name=law==G711_MULAW?"mu":"a";
  snprintf(path,sizeof(path),"%s/%s-all-codes.g711",directory,name);f=fopen(path,"wbx");CHECK(f!=NULL);
  CHECK(fwrite(codes,1,256,f)==256);CHECK(fclose(f)==0);
  for(i=0;i<256;i++){int16_t v=law==G711_MULAW?g711_decode_mu((uint8_t)i):g711_decode_a((uint8_t)i);decoded[2*i]=(uint8_t)v;decoded[2*i+1]=(uint8_t)((uint16_t)v>>8);}
  snprintf(path,sizeof(path),"%s/%s-c-decoded.s16le",directory,name);f=fopen(path,"wbx");CHECK(f!=NULL);
  CHECK(fwrite(decoded,1,512,f)==512);CHECK(fclose(f)==0);
 }
}
static void host_cpu_cost(void)
{
 struct timespec start,end;uint64_t ns;unsigned law,round;int s;volatile uint32_t sum=0;
 for(law=G711_MULAW;law<=G711_ALAW;law++){
  CHECK(clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&start)==0);
  for(round=0;round<32;round++)for(s=-32768;s<32768;s++)sum+=law==G711_MULAW?g711_encode_mu((int16_t)s):g711_encode_a((int16_t)s);
  CHECK(clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&end)==0);
  ns=(uint64_t)(end.tv_sec-start.tv_sec)*1000000000ULL;
  if(end.tv_nsec>=start.tv_nsec)ns+=(uint64_t)(end.tv_nsec-start.tv_nsec);else ns-=(uint64_t)(start.tv_nsec-end.tv_nsec);
  printf("HOST CPU law=%u samples=2097152 cpu_ns=%llu checksum=%u NOT_ARM_PERFORMANCE\n",law,(unsigned long long)ns,(unsigned)sum);
 }
}
int main(int argc,char **argv)
{
 if(argc==2&&!strcmp(argv[1],"--preborrow-only")){
  preborrow_authority_and_legitimate_peer();callback_freshness_and_preborrow_races();
  printf("HOST pre-borrow boundary PASS suites=%u checks=%u\n",suites,checks);return 0;
 }
 CHECK(argc>=2);codec_vectors(argv[1]);packet_laws_chunks_wrap();effects_revoke_mute_expire();
 invalid_formats_aliases_tokens_and_clock();source_generation_timestamps_and_returns();
 near_max_time_and_unknown_return();
 current_source_epoch_continuations();
 preborrow_authority_and_legitimate_peer();callback_freshness_and_preborrow_races();
 if(argc>=3)vector_output(argv[2]);
 if(argc>=4&&!strcmp(argv[3],"--host-cost"))host_cpu_cost();
 printf("HOST SYNTHETIC G711/RTP PASS suites=%u checks=%u packet_storage=%zu fixed_codes=256 native_qualified=0\n",suites,checks,sizeof(struct am_packet));return 0;
}
