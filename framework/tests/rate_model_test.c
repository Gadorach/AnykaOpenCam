/* SPDX-License-Identifier: GPL-2.0-only; host arithmetic ONLY, no device access. */
#include <camera/rate_model.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
/* Generated independently in fresh analysis/rate-model01 by its runner. */
#include "rate_model_golden.h"

static unsigned checks;
#define CHECK(x) do {assert(x);checks++;} while(0)
static struct camera_rate_config config(void)
{
 struct camera_rate_config c={.abi_version=1,.struct_bytes=sizeof(c),.mode=CAMERA_RATE_SHADOW_ONLY,
  .context_id=11,.source_generation=12,.stream_generation=13,.target_bps=800000,.burst_bits=800000,
  .max_capture_gap_ns=CAMERA_RATE_NS_PER_SECOND,.max_completion_lag_ns=CAMERA_RATE_NS_PER_SECOND,
  .candidate_p_min=25,.candidate_p_max=27,.gop_pictures=30,
  .max_whole_au_bytes=CAMERA_RATE_ASIC_MAX_BYTES+CAMERA_RATE_HEADER_MAX_BYTES};
 return c;
}
static struct camera_rate_feedback feedback(const struct camera_rate_model *m,uint64_t ns,uint32_t bytes)
{
 struct camera_rate_feedback f={.model_generation=m->generation,.context_id=m->config.context_id,
  .source_generation=m->config.source_generation,.stream_generation=m->config.stream_generation,
  .accepted_sequence=m->accepted_sequence+1,.capture_sequence=m->last_capture_sequence+1,
  .capture_ns=ns,.encode_start_ns=ns,.complete_ns=ns,.raw_input_bytes=CAMERA_RATE_RAW_BYTES,
  .asic_bytes=bytes>CAMERA_RATE_ASIC_MAX_BYTES?CAMERA_RATE_ASIC_MAX_BYTES:bytes,.whole_au_bytes=bytes,
  .picture=m->accepted_sequence?CAMERA_RATE_P:CAMERA_RATE_IDR,
  .encode_success=1,.whole_au_complete=1,.reference_committed=1,.profile_qp=26};
 return f;
}
static void refusal(struct camera_rate_model *m,struct camera_rate_feedback f,int expected)
{
 struct camera_rate_model before;struct camera_rate_recommendation out,unchanged;
 memcpy(&before,m,sizeof(before));memset(&out,0xa5,sizeof(out));memcpy(&unchanged,&out,sizeof(out));
 CHECK(camera_rate_model_complete(m,&f,&out)==expected);
 CHECK(!memcmp(m,&before,sizeof(before))&&!memcmp(&out,&unchanged,sizeof(out)));
}
static void fixed(const struct camera_rate_recommendation *r)
{
 CHECK(r->mode==CAMERA_RATE_SHADOW_ONLY&&r->hardware_profile_qp==26&&
       r->hardware_qualified_min==26&&r->hardware_qualified_max==26&&r->candidate_next_idr_qp==26);
 CHECK(r->candidate_next_p_qp>=25&&r->candidate_next_p_qp<=27);
}
static void vectors(void)
{
 struct camera_rate_config c=config();struct camera_rate_model m;struct camera_rate_recommendation r;
 for(unsigned i=0;i<sizeof(golden)/sizeof(golden[0]);i++){
  CHECK(camera_rate_model_init(&m,&c,1,0,0)==CAMERA_OK);
  struct camera_rate_feedback f=feedback(&m,golden[i].dt,golden[i].bytes);
  CHECK(camera_rate_model_complete(&m,&f,&r)==CAMERA_OK);
  CHECK(r.debt_bits==golden[i].debt&&r.fractional_credit_remainder==golden[i].remainder&&
        r.candidate_next_p_qp==golden[i].qp);
  fixed(&r);
 }
 CHECK(camera_rate_model_init(&m,&c,1,0,0)==CAMERA_OK);
 CHECK(camera_rate_model_snapshot(&m,&r)==CAMERA_OK&&r.next_gop_idr_due&&r.gop_position==0);
 for(unsigned i=0;i<60;i++){
  struct camera_rate_feedback f=feedback(&m,(uint64_t)(i+1)*400000000,45000);
  if(i%30==0)f.picture=CAMERA_RATE_IDR;
  CHECK(camera_rate_model_complete(&m,&f,&r)==CAMERA_OK);
  CHECK(r.debt_bits==audit_trace[i].debt&&r.candidate_next_p_qp==audit_trace[i].qp);
  fixed(&r);
 }
 CHECK(r.raising_at_max&&r.debt_clamped&&r.next_gop_idr_due);
 /* Exact deadband edges: strict inequality, no extra step at +/-burst/8. */
 CHECK(camera_rate_model_init(&m,&c,1,0,0)==CAMERA_OK);
 struct camera_rate_feedback f=feedback(&m,0,12500);
 CHECK(camera_rate_model_complete(&m,&f,&r)==CAMERA_OK&&r.debt_bits==100000&&r.candidate_next_p_qp==26);
 CHECK(camera_rate_model_init(&m,&c,1,0,0)==CAMERA_OK);f=feedback(&m,250000000,12500);
 CHECK(camera_rate_model_complete(&m,&f,&r)==CAMERA_OK&&r.debt_bits== -100000&&r.candidate_next_p_qp==26);
 c.target_bps=c.burst_bits=10001;
 CHECK(camera_rate_model_init(&m,&c,1,0,0)==CAMERA_OK);
 for(unsigned i=1;i<=15;i++){
  f=feedback(&m,(uint64_t)i*(CAMERA_RATE_NS_PER_SECOND/15),1);
  CHECK(camera_rate_model_complete(&m,&f,&r)==CAMERA_OK);
 }
 CHECK(r.debt_bits==120-(int64_t)(10001*(15*(CAMERA_RATE_NS_PER_SECOND/15))/CAMERA_RATE_NS_PER_SECOND));
 CHECK(r.fractional_credit_remainder==(10001*(15*(CAMERA_RATE_NS_PER_SECOND/15)))%CAMERA_RATE_NS_PER_SECOND);
}
static void invalid_feedback(void)
{
 struct camera_rate_config c=config();struct camera_rate_model m;struct camera_rate_recommendation r;
 CHECK(camera_rate_model_init(&m,&c,7,100,4)==CAMERA_OK);
 struct camera_rate_feedback good=feedback(&m,1000,1000),f;
#define BAD(field,value,result) do {f=good;f.field=value;refusal(&m,f,result);} while(0)
 BAD(model_generation,6,CAMERA_STALE_LEASE);BAD(context_id,99,CAMERA_STALE_LEASE);
 BAD(source_generation,99,CAMERA_STALE_LEASE);BAD(stream_generation,99,CAMERA_STALE_LEASE);
 BAD(accepted_sequence,0,CAMERA_STALE_LEASE);BAD(accepted_sequence,2,CAMERA_INVALID);
 BAD(capture_sequence,4,CAMERA_STALE_LEASE);BAD(whole_au_bytes,0,CAMERA_INVALID);
 BAD(asic_bytes,0,CAMERA_INVALID);BAD(asic_bytes,CAMERA_RATE_ASIC_MAX_BYTES+1,CAMERA_INVALID);
 BAD(whole_au_bytes,CAMERA_RATE_ASIC_MAX_BYTES+257,CAMERA_INVALID);
 BAD(whole_au_bytes,999,CAMERA_INVALID);BAD(whole_au_bytes,1257,CAMERA_INVALID);
 BAD(raw_input_bytes,1728000,CAMERA_INVALID);BAD(profile_qp,27,CAMERA_INVALID);
 BAD(picture,CAMERA_RATE_P,CAMERA_INVALID);BAD(picture,(enum camera_rate_picture)3,CAMERA_INVALID);
 BAD(encode_success,0,CAMERA_IO_ERROR);BAD(encode_success,2,CAMERA_IO_ERROR);
 BAD(whole_au_complete,0,CAMERA_IO_ERROR);BAD(reference_committed,0,CAMERA_IO_ERROR);
 BAD(capture_ns,99,CAMERA_INVALID);BAD(encode_start_ns,999,CAMERA_INVALID);
 BAD(complete_ns,999,CAMERA_INVALID);BAD(complete_ns,UINT64_MAX,CAMERA_INVALID);
 f=good;f.capture_ns=f.encode_start_ns=f.complete_ns=UINT64_MAX;refusal(&m,f,CAMERA_INVALID);
 CHECK(camera_rate_model_complete(&m,&good,&r)==CAMERA_OK);refusal(&m,good,CAMERA_STALE_LEASE);
 good=feedback(&m,2000,1000);good.complete_ns=1000000;
 CHECK(camera_rate_model_complete(&m,&good,&r)==CAMERA_OK);
 f=feedback(&m,3000,1000);refusal(&m,f,CAMERA_INVALID); /* completion regressed */
 f.complete_ns=1000000;CHECK(camera_rate_model_complete(&m,&f,&r)==CAMERA_OK);
 CHECK(camera_rate_model_complete(NULL,&f,&r)==CAMERA_INVALID);
 CHECK(camera_rate_model_complete(&m,NULL,&r)==CAMERA_INVALID);
 CHECK(camera_rate_model_complete(&m,&f,NULL)==CAMERA_INVALID);
#undef BAD
}
static void configuration(void)
{
 struct camera_rate_config c=config(),bad;struct camera_rate_model m,before;
 CHECK(camera_rate_model_init(&m,&c,1,0,0)==CAMERA_OK);
 memcpy(&before,&m,sizeof(m));
#define BAD(field,value) do {bad=c;bad.field=value;CHECK(camera_rate_model_reconfigure(&m,&bad,1,0,0)==CAMERA_INVALID);CHECK(!memcmp(&m,&before,sizeof(m)));} while(0)
 BAD(abi_version,2);BAD(struct_bytes,sizeof(c)-1);BAD(mode,(enum camera_rate_mode)2);
 BAD(context_id,0);BAD(source_generation,0);BAD(stream_generation,0);
 BAD(target_bps,0);BAD(target_bps,9999);BAD(target_bps,UINT64_MAX);
 BAD(burst_bits,UINT64_MAX);BAD(max_capture_gap_ns,0);BAD(max_capture_gap_ns,UINT64_MAX);
 BAD(max_completion_lag_ns,0);BAD(max_completion_lag_ns,UINT64_MAX);
 BAD(candidate_p_min,24);BAD(candidate_p_min,27);BAD(candidate_p_max,25);BAD(candidate_p_max,28);
 BAD(gop_pictures,0);BAD(gop_pictures,151);BAD(max_whole_au_bytes,1048575);
 BAD(max_whole_au_bytes,1048833);
 CHECK(camera_rate_model_reconfigure(&m,&c,2,0,0)==CAMERA_STALE_LEASE&&!memcmp(&m,&before,sizeof(m)));
 CHECK(camera_rate_model_reconfigure(&m,&c,1,0,UINT64_MAX)==CAMERA_INVALID);
 CHECK(camera_rate_model_init(NULL,&c,1,0,0)==CAMERA_INVALID);
 CHECK(camera_rate_model_init(&m,NULL,1,0,0)==CAMERA_INVALID);
 CHECK(camera_rate_model_init(&m,&c,0,0,0)==CAMERA_INVALID);
 struct camera_rate_feedback old=feedback(&m,400000000,45000);
 struct camera_rate_recommendation r;
 CHECK(camera_rate_model_complete(&m,&old,&r)==CAMERA_OK);
 /* Valid alias reconfigure reads old config before atomic reset. */
 CHECK(camera_rate_model_reconfigure(&m,&m.config,1,500000000,10)==CAMERA_OK);
 CHECK(camera_rate_model_snapshot(&m,&r)==CAMERA_OK&&r.model_generation==2&&
       r.accepted_sequence==0&&r.debt_bits==0&&r.fractional_credit_remainder==0&&r.candidate_next_p_qp==26);
 refusal(&m,old,CAMERA_STALE_LEASE);
 old=feedback(&m,600000000,45000);CHECK(camera_rate_model_complete(&m,&old,&r)==CAMERA_OK);
 c.source_generation++;c.stream_generation++;
 CHECK(camera_rate_model_reconfigure(&m,&c,2,0,0)==CAMERA_OK);
 refusal(&m,old,CAMERA_STALE_LEASE);
 CHECK(camera_rate_model_init(&m,&c,UINT64_MAX,0,0)==CAMERA_OK);
 CHECK(camera_rate_model_reconfigure(&m,&c,UINT64_MAX,0,0)==CAMERA_NOT_READY);
 c.target_bps=c.burst_bits=40000000;c.candidate_p_min=c.candidate_p_max=26;c.gop_pictures=1;
 CHECK(camera_rate_model_init(&m,&c,1,0,0)==CAMERA_OK);
 old=feedback(&m,0,1048832);CHECK(camera_rate_model_complete(&m,&old,&r)==CAMERA_OK);
 CHECK(r.candidate_next_p_qp==26&&r.raising_at_max&&r.next_gop_idr_due);
#undef BAD
}
static void timing_and_rejection(void)
{
 struct camera_rate_config c=config();c.gop_pictures=3;
 struct camera_rate_model m,a,b;struct camera_rate_recommendation r,ra,rb;
 CHECK(camera_rate_model_init(&m,&c,1,0,0)==CAMERA_OK);
 struct camera_rate_feedback f=feedback(&m,400000000,45000);
 CHECK(camera_rate_model_complete(&m,&f,&r)==CAMERA_OK&&r.gop_position==1);
 f=feedback(&m,600000000,45000);f.encode_success=0;refusal(&m,f,CAMERA_IO_ERROR);
 /* Rejected uncoded captures did not consume budget/sequence; real elapsed
  * gap800-400ms is credited once, capture sequence jumps1->5. */
 f=feedback(&m,800000000,45000);f.capture_sequence=5;
 CHECK(camera_rate_model_complete(&m,&f,&r)==CAMERA_OK&&r.debt_bits==80000&&
       r.accepted_sequence==2&&r.last_capture_sequence==5&&r.gop_position==2);
 f=feedback(&m,1800000000,45000);
 CHECK(camera_rate_model_complete(&m,&f,&r)==CAMERA_OK&&r.next_gop_idr_due);
 f=feedback(&m,1800000001,45000);f.picture=CAMERA_RATE_IDR;
 CHECK(camera_rate_model_complete(&m,&f,&r)==CAMERA_OK&&!r.next_gop_idr_due&&r.gop_position==1);
 f=feedback(&m,2800000002,45000);refusal(&m,f,CAMERA_INVALID); /* >1s, no fakeFPS */
 CHECK(camera_rate_model_init(&a,&c,1,0,0)==CAMERA_OK&&camera_rate_model_init(&b,&c,1,0,0)==CAMERA_OK);
 for(unsigned i=1;i<=3;i++){
  f=feedback(&a,(uint64_t)i*100000000,45000);
  CHECK(camera_rate_model_complete(&a,&f,&ra)==CAMERA_OK);
  f=feedback(&b,(uint64_t)i*100000000,45000);f.encode_start_ns+=200000000;f.complete_ns+=500000000;
  CHECK(camera_rate_model_complete(&b,&f,&rb)==CAMERA_OK);
  CHECK(ra.debt_bits==rb.debt_bits&&ra.candidate_next_p_qp==rb.candidate_next_p_qp&&ra.gop_position==rb.gop_position);
 }
 /* Safe timestamp subtraction near UINT64_MAX, no anchor+dt overflow. */
 CHECK(camera_rate_model_init(&m,&c,1,UINT64_MAX-100,UINT64_MAX-1)==CAMERA_OK);
 f=feedback(&m,UINT64_MAX,1);CHECK(camera_rate_model_complete(&m,&f,&r)==CAMERA_OK);
 f.capture_sequence=0;refusal(&m,f,CAMERA_STALE_LEASE);
 c.max_capture_gap_ns=1;c.max_completion_lag_ns=1;
 CHECK(camera_rate_model_init(&m,&c,1,0,0)==CAMERA_OK);
 f=feedback(&m,1,1);f.complete_ns=2;CHECK(camera_rate_model_complete(&m,&f,&r)==CAMERA_OK);
 f=feedback(&m,3,1);refusal(&m,f,CAMERA_INVALID);
}
int main(void)
{
 struct camera_rate_model uninitialized={0};struct camera_rate_recommendation r;
 CHECK(camera_rate_model_snapshot(&uninitialized,&r)==CAMERA_INVALID);
 vectors();invalid_feedback();configuration();timing_and_rejection();
 printf("Shadow-only rate model PASS: %u assertions; context=%zu bytes; no hardware control\n",checks,sizeof(struct camera_rate_model));
 return 0;
}
