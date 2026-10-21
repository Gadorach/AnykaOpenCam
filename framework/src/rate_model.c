// SPDX-License-Identifier: GPL-2.0-only
#include <camera/rate_model.h>
#include <stdint.h>

static int config_valid(const struct camera_rate_config *c)
{
 return c&&c->abi_version==CAMERA_RATE_MODEL_ABI&&c->struct_bytes==sizeof(*c)&&
  c->mode==CAMERA_RATE_SHADOW_ONLY&&c->context_id&&c->source_generation&&c->stream_generation&&
  c->target_bps>=10000&&c->target_bps<=40000000&&c->burst_bits==c->target_bps&&
  c->max_capture_gap_ns&&c->max_capture_gap_ns<=CAMERA_RATE_NS_PER_SECOND&&
  c->max_completion_lag_ns&&c->max_completion_lag_ns<=CAMERA_RATE_NS_PER_SECOND&&
  c->candidate_p_min>=25&&c->candidate_p_min<=26&&
  c->candidate_p_max>=26&&c->candidate_p_max<=27&&
  c->gop_pictures&&c->gop_pictures<=150&&
  c->max_whole_au_bytes>=CAMERA_RATE_ASIC_MAX_BYTES&&
  c->max_whole_au_bytes<=CAMERA_RATE_ASIC_MAX_BYTES+CAMERA_RATE_HEADER_MAX_BYTES;
}
static int valid(const struct camera_rate_model *m)
{return m&&m->initialized;}
static int add_signed(int64_t a,int64_t b,int64_t *out)
{
 if((b>0&&a>INT64_MAX-b)||(b<0&&a<INT64_MIN-b))return CAMERA_INVALID;
 *out=a+b;return CAMERA_OK;
}
static void fresh(struct camera_rate_model *m,const struct camera_rate_config *c,
                  uint64_t generation,uint64_t ns,uint64_t sequence)
{
 struct camera_rate_model next={0};
 next.config=*c;next.generation=generation;next.last_capture_ns=ns;
 next.last_capture_sequence=sequence;next.last_complete_ns=ns;
 next.initialized=1;next.candidate_p_qp=CAMERA_RATE_PROFILE_QP;*m=next;
}
int camera_rate_model_init(struct camera_rate_model *m,const struct camera_rate_config *c,
                          uint64_t generation,uint64_t ns,uint64_t sequence)
{
 if(!m||!config_valid(c)||!generation||sequence==UINT64_MAX)return CAMERA_INVALID;
 fresh(m,c,generation,ns,sequence);return CAMERA_OK;
}
int camera_rate_model_reconfigure(struct camera_rate_model *m,const struct camera_rate_config *c,
                                 uint64_t expected,uint64_t ns,uint64_t sequence)
{
 if(!valid(m)||!config_valid(c)||sequence==UINT64_MAX)return CAMERA_INVALID;
 if(expected!=m->generation)return CAMERA_STALE_LEASE;
 if(m->generation==UINT64_MAX)return CAMERA_NOT_READY;
 fresh(m,c,m->generation+1,ns,sequence);return CAMERA_OK;
}
int camera_rate_model_snapshot(const struct camera_rate_model *m,struct camera_rate_recommendation *out)
{
 if(!valid(m)||!out)return CAMERA_INVALID;
 struct camera_rate_recommendation r={0};
 r.mode=CAMERA_RATE_SHADOW_ONLY;r.model_generation=m->generation;
 r.context_id=m->config.context_id;r.source_generation=m->config.source_generation;
 r.stream_generation=m->config.stream_generation;r.accepted_sequence=m->accepted_sequence;
 r.last_capture_sequence=m->last_capture_sequence;r.last_capture_ns=m->last_capture_ns;
 r.debt_bits=m->debt;r.fractional_credit_remainder=m->remainder;
 r.candidate_next_p_qp=m->candidate_p_qp;r.candidate_next_idr_qp=CAMERA_RATE_PROFILE_QP;
 r.hardware_profile_qp=r.hardware_qualified_min=r.hardware_qualified_max=CAMERA_RATE_PROFILE_QP;
 r.gop_position=m->gop_position;
 r.next_gop_idr_due=!m->accepted_sequence||m->gop_position>=m->config.gop_pictures;
 r.candidate_at_bound=m->candidate_p_qp==m->config.candidate_p_min||
                      m->candidate_p_qp==m->config.candidate_p_max;
 r.raising_at_max=m->debt>(int64_t)(m->config.burst_bits/8)&&
                  m->candidate_p_qp==m->config.candidate_p_max;
 r.debt_clamped=m->debt_clamped;*out=r;return CAMERA_OK;
}
int camera_rate_model_complete(struct camera_rate_model *m,const struct camera_rate_feedback *f,
                               struct camera_rate_recommendation *out)
{
 uint64_t dt,numerator,credit;
 int64_t debt,limit;
 if(!valid(m)||!f||!out)return CAMERA_INVALID;
 if(f->model_generation!=m->generation||f->context_id!=m->config.context_id||
    f->source_generation!=m->config.source_generation||f->stream_generation!=m->config.stream_generation||
    f->accepted_sequence<=m->accepted_sequence||f->capture_sequence<=m->last_capture_sequence)
  return CAMERA_STALE_LEASE;
 if(m->accepted_sequence==UINT64_MAX)return CAMERA_NOT_READY;
 if(f->accepted_sequence!=m->accepted_sequence+1)return CAMERA_INVALID;
 if(f->encode_success!=1||f->whole_au_complete!=1||f->reference_committed!=1)
  return CAMERA_IO_ERROR;
 if(f->profile_qp!=CAMERA_RATE_PROFILE_QP||f->raw_input_bytes!=CAMERA_RATE_RAW_BYTES||
    !f->asic_bytes||f->asic_bytes>CAMERA_RATE_ASIC_MAX_BYTES||!f->whole_au_bytes||
    f->whole_au_bytes>m->config.max_whole_au_bytes||f->whole_au_bytes<f->asic_bytes||
    f->whole_au_bytes-f->asic_bytes>CAMERA_RATE_HEADER_MAX_BYTES||
    (f->picture!=CAMERA_RATE_IDR&&f->picture!=CAMERA_RATE_P)||
    (!m->accepted_sequence&&f->picture!=CAMERA_RATE_IDR))return CAMERA_INVALID;
 if(f->capture_ns<m->last_capture_ns||f->encode_start_ns<f->capture_ns||
    f->complete_ns<f->encode_start_ns||f->complete_ns<m->last_complete_ns||
    f->complete_ns-f->capture_ns>m->config.max_completion_lag_ns)return CAMERA_INVALID;
 dt=f->capture_ns-m->last_capture_ns;
 if(dt>m->config.max_capture_gap_ns||m->remainder>=CAMERA_RATE_NS_PER_SECOND)return CAMERA_INVALID;
 /* Bounded config fits uint64 on ARM32. Check BEFORE multiply/add, independent
  * of that bound. No __int128, floating point or absolute timestamp addition.
  */
 if(dt&&m->config.target_bps>(UINT64_MAX-m->remainder)/dt)return CAMERA_INVALID;
 numerator=m->config.target_bps*dt+m->remainder;
 credit=numerator/CAMERA_RATE_NS_PER_SECOND;
 if(credit>INT64_MAX||add_signed(m->debt,(int64_t)((uint64_t)f->whole_au_bytes*8),&debt)||
    add_signed(debt,-(int64_t)credit,&debt))return CAMERA_INVALID;
 struct camera_rate_model next=*m;
 limit=(int64_t)m->config.burst_bits;next.debt_clamped=debt>limit||debt< -limit;
 next.debt=debt>limit?limit:debt< -limit? -limit:debt;
 next.remainder=numerator%CAMERA_RATE_NS_PER_SECOND;
 if(next.debt>limit/8&&next.candidate_p_qp<next.config.candidate_p_max)next.candidate_p_qp++;
 else if(next.debt< -(limit/8)&&next.candidate_p_qp>next.config.candidate_p_min)next.candidate_p_qp--;
 next.accepted_sequence=f->accepted_sequence;next.last_capture_sequence=f->capture_sequence;
 next.last_capture_ns=f->capture_ns;next.last_complete_ns=f->complete_ns;
 if(f->picture==CAMERA_RATE_IDR)next.gop_position=1;
 else if(next.gop_position<next.config.gop_pictures)next.gop_position++;
 /* No mutating operation can fail after validation; commit state + output only
  * now. Snapshot has fixed profile metadata, never an applied candidate QP.
  */
 camera_rate_model_snapshot(&next,out);*m=next;return CAMERA_OK;
}
