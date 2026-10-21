/* SPDX-License-Identifier: GPL-2.0-only */
#include <camera/fmp4.h>
#include <string.h>

struct bits {uint8_t data[CAMERA_FMP4_PARAMETER_BYTES];size_t bytes,pos;int bad;};
static int overlaps(const void *a,size_t na,const void *b,size_t nb)
{uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return x<=y?y-x<na:x-y<nb;}
static int rbsp(struct bits *b,const uint8_t *nal,size_t n,unsigned type)
{
 unsigned zeros=0;memset(b,0,sizeof(*b));
 if(!nal||n<2||n>CAMERA_FMP4_PARAMETER_BYTES||nal[0]&0x80||!(nal[0]&0x60)||(nal[0]&31)!=type)return CAMERA_INVALID;
 for(size_t i=1;i<n;i++){
  unsigned v=nal[i];
  if(zeros==2){
   if(v<=2)return CAMERA_INVALID;
   if(v==3){if(i+1==n||nal[i+1]>3)return CAMERA_INVALID;zeros=0;continue;}
  }
  b->data[b->bytes++]=(uint8_t)v;zeros=v==0?zeros+1:0;
 }
 return CAMERA_OK;
}
static uint32_t get(struct bits *b,unsigned n)
{
 uint32_t value=0;
 if(n>32||b->pos>b->bytes*8||n>b->bytes*8-b->pos){b->bad=1;return 0;}
 while(n--){value=value<<1|((b->data[b->pos/8]>>(7-b->pos%8))&1);b->pos++;}
 return value;
}
static uint32_t ue(struct bits *b)
{
 unsigned zeros=0;
 while(!b->bad&&!get(b,1)){if(++zeros>31){b->bad=1;return 0;}}
 if(b->bad)return 0;
 return ((UINT32_C(1)<<zeros)-1)+get(b,zeros);
}
static int64_t se(struct bits *b)
{uint32_t v=ue(b);return v&1?(int64_t)(v/2)+1:-(int64_t)(v/2);}
static int trailing(struct bits *b)
{
 if(b->bad||!get(b,1)||b->bad||b->bytes*8-b->pos>7)return CAMERA_INVALID;
 while(b->pos<b->bytes*8)if(get(b,1))return CAMERA_INVALID;
 return b->bad?CAMERA_INVALID:CAMERA_OK;
}
int camera_fmp4_profile_read(const uint8_t *sps,size_t ns,const uint8_t *pps,size_t np,
                             struct camera_fmp4_profile *out)
{
 struct bits b;struct camera_fmp4_profile p={0};uint32_t w,h,refs,crop[4]={0};int r;
 if(!out||!sps||!pps||!ns||!np||ns>CAMERA_FMP4_PARAMETER_BYTES||np>CAMERA_FMP4_PARAMETER_BYTES||
    overlaps(out,sizeof(*out),sps,ns)||overlaps(out,sizeof(*out),pps,np))return CAMERA_INVALID;
 if((r=rbsp(&b,sps,ns,7))!=CAMERA_OK)return r;
 p.profile=(uint8_t)get(&b,8);p.compatibility=(uint8_t)get(&b,8);p.level=(uint8_t)get(&b,8);
 if(b.bad)return CAMERA_INVALID;
 if(p.profile!=66||p.compatibility!=0xe0||p.level!=31)return CAMERA_UNSUPPORTED;
 p.sps_id=ue(&b);if(p.sps_id>31||ue(&b)>12)return CAMERA_INVALID;
 if(ue(&b)!=2)return b.bad?CAMERA_INVALID:CAMERA_UNSUPPORTED;
 refs=ue(&b);if(refs>16)return CAMERA_INVALID;
 if(get(&b,1))return CAMERA_UNSUPPORTED;
 w=ue(&b);h=ue(&b);if(b.bad||w>4094||h>4094)return CAMERA_INVALID;
 /* Level3.1 geometry/DPB syntax constraints; no FPS/bitrate/hardware inference.
  * Bounds above make the products fit32 bits before these comparisons. */
 if((w+1)*(h+1)>3600||(w+1)*(w+1)>28800||(h+1)*(h+1)>28800||
    (w+1)*(h+1)*refs>18000)return CAMERA_INVALID;
 if(!get(&b,1))return b.bad?CAMERA_INVALID:CAMERA_UNSUPPORTED;
 (void)get(&b,1); /* direct_8x8_inference_flag */
 if(get(&b,1))for(unsigned i=0;i<4;i++)crop[i]=ue(&b);
 if(get(&b,1))return CAMERA_UNSUPPORTED; /* VUI not parsed in this narrow subset */
 if((r=trailing(&b))!=CAMERA_OK)return r;
 w=(w+1)*16;h=(h+1)*16;
 if(crop[0]>=w/2||crop[1]>=w/2-crop[0]||crop[2]>=h/2||crop[3]>=h/2-crop[2])return CAMERA_INVALID;
 p.width=w-2*(crop[0]+crop[1]);p.height=h-2*(crop[2]+crop[3]);
 if((r=rbsp(&b,pps,np,8))!=CAMERA_OK)return r;
 p.pps_id=ue(&b);uint32_t id=ue(&b);
 if(b.bad||p.pps_id>255||id!=p.sps_id)return CAMERA_INVALID;
 if(get(&b,1)||get(&b,1)||ue(&b)!=0)return b.bad?CAMERA_INVALID:CAMERA_UNSUPPORTED;
 if(ue(&b)>31||ue(&b)>31)return CAMERA_INVALID;
 if(get(&b,1)||get(&b,2))return b.bad?CAMERA_INVALID:CAMERA_UNSUPPORTED;
 int64_t qp=se(&b),qs=se(&b),chroma=se(&b);
 if(b.bad||qp < -26||qp>25||qs < -26||qs>25||chroma < -12||chroma>12)return CAMERA_INVALID;
 (void)get(&b,1);(void)get(&b,1);if(get(&b,1))return CAMERA_UNSUPPORTED;
 if((r=trailing(&b))!=CAMERA_OK)return r;
 *out=p;return CAMERA_OK;
}

struct writer {uint8_t *p;size_t n;int bad;};
static void bytes(struct writer *w,const void *p,size_t n)
{
 if(w->bad||n>CAMERA_FMP4_INIT_BYTES-w->n){w->bad=1;return;}
 if(n)memcpy(w->p+w->n,p,n);
 w->n+=n;
}
static void u32(struct writer *w,uint32_t v)
{uint8_t p[4]={(uint8_t)(v>>24),(uint8_t)(v>>16),(uint8_t)(v>>8),(uint8_t)v};bytes(w,p,4);}
static void u16(struct writer *w,uint16_t v)
{uint8_t p[2]={(uint8_t)(v>>8),(uint8_t)v};bytes(w,p,2);}
static void zero(struct writer *w,unsigned n)
{while(n--)bytes(w,"\0",1);}
static size_t begin(struct writer *w,const char type[4])
{size_t at=w->n;u32(w,0);bytes(w,type,4);return at;}
static size_t full(struct writer *w,const char type[4],uint32_t vf)
{size_t at=begin(w,type);u32(w,vf);return at;}
static void end(struct writer *w,size_t at)
{
 if(w->bad||at>w->n||w->n-at<8){w->bad=1;return;}
 uint32_t n=(uint32_t)(w->n-at);
 w->p[at]=(uint8_t)(n>>24);w->p[at+1]=(uint8_t)(n>>16);w->p[at+2]=(uint8_t)(n>>8);w->p[at+3]=(uint8_t)n;
}
static void matrix(struct writer *w)
{const uint32_t values[9]={0x10000,0,0,0,0x10000,0,0,0,0x40000000};for(unsigned i=0;i<9;i++)u32(w,values[i]);}
int camera_fmp4_init_prepare(const struct camera_fmp4_config *c,struct camera_fmp4_init *out)
{
 struct camera_fmp4_init result={0};int r;
 if(!c||!out||!c->generation||!c->sps||!c->pps||!c->sps_bytes||!c->pps_bytes||
    c->sps_bytes>CAMERA_FMP4_PARAMETER_BYTES||c->pps_bytes>CAMERA_FMP4_PARAMETER_BYTES||
    overlaps(out,sizeof(*out),c,sizeof(*c))||overlaps(out,sizeof(*out),c->sps,c->sps_bytes)||
    overlaps(out,sizeof(*out),c->pps,c->pps_bytes))return CAMERA_INVALID;
 if((r=camera_fmp4_profile_read(c->sps,c->sps_bytes,c->pps,c->pps_bytes,&result.profile))!=CAMERA_OK)return r;
 struct writer w={result.data,0,0};size_t b,mv,tk,md,mi,st,a;
 b=begin(&w,"ftyp");bytes(&w,"iso6",4);u32(&w,1);bytes(&w,"iso6avc1mp41",12);end(&w,b);
 size_t moov=begin(&w,"moov");b=full(&w,"mvhd",0);zero(&w,8);u32(&w,90000);u32(&w,c->duration_ticks);
 u32(&w,0x10000);u16(&w,0x100);zero(&w,10);matrix(&w);zero(&w,24);u32(&w,2);end(&w,b);
 tk=begin(&w,"trak");b=full(&w,"tkhd",3);zero(&w,8);u32(&w,1);zero(&w,4);u32(&w,c->duration_ticks);
 zero(&w,16);matrix(&w);u32(&w,result.profile.width<<16);u32(&w,result.profile.height<<16);end(&w,b);
 md=begin(&w,"mdia");b=full(&w,"mdhd",0);zero(&w,8);u32(&w,90000);u32(&w,c->duration_ticks);u16(&w,0x55c4);u16(&w,0);end(&w,b);
 b=full(&w,"hdlr",0);zero(&w,4);bytes(&w,"vide",4);zero(&w,12);bytes(&w,"Camera video",13);end(&w,b);
 mi=begin(&w,"minf");b=full(&w,"vmhd",1);zero(&w,8);end(&w,b);
 a=begin(&w,"dinf");b=full(&w,"dref",0);u32(&w,1);mv=full(&w,"url ",1);end(&w,mv);end(&w,b);end(&w,a);
 st=begin(&w,"stbl");b=full(&w,"stsd",0);u32(&w,1);a=begin(&w,"avc1");zero(&w,6);u16(&w,1);zero(&w,16);
 u16(&w,(uint16_t)result.profile.width);u16(&w,(uint16_t)result.profile.height);
 u32(&w,0x480000);u32(&w,0x480000);zero(&w,4);u16(&w,1);zero(&w,32);u16(&w,24);u16(&w,65535);
 mv=begin(&w,"avcC");uint8_t prefix[6]={1,result.profile.profile,result.profile.compatibility,result.profile.level,0xff,0xe1};
 bytes(&w,prefix,6);u16(&w,(uint16_t)c->sps_bytes);bytes(&w,c->sps,c->sps_bytes);bytes(&w,"\1",1);
 u16(&w,(uint16_t)c->pps_bytes);bytes(&w,c->pps,c->pps_bytes);end(&w,mv);end(&w,a);end(&w,b);
 const char *tables[4]={"stts","stsc","stsz","stco"};
 for(unsigned i=0;i<4;i++){b=full(&w,tables[i],0);if(i==2)u32(&w,0);u32(&w,0);end(&w,b);}
 end(&w,st);end(&w,mi);end(&w,md);end(&w,tk);
 mv=begin(&w,"mvex");b=full(&w,"trex",0);u32(&w,1);u32(&w,1);u32(&w,0);u32(&w,0);u32(&w,0x1010000);end(&w,b);end(&w,mv);end(&w,moov);
 if(w.bad)return CAMERA_INVALID;
 result.bytes=w.n;result.generation=c->generation;*out=result;return CAMERA_OK;
}
