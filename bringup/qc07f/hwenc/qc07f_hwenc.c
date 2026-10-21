// SPDX-License-Identifier: GPL-2.0-only
/* Bounded, explicit QC07F retail codec port. Numeric facts from owned REA:
 * libakstreamenc.so SHA c2cc60c7b4f4a1856bd01e760b294cd6a97bc0217088721a8c27601561ad416b,
 * VideoStream_Enc_Init/Reset, EncAsicFrameStart/Enable/GetRegisters and
 * register descriptors at3f4e4. No stock code execution or guessed MMIO.
 * Polling JPEG first; IRQ disabled at codec and no handler/source ownership
 * changes. No encoder start at probe; no arbitrary register/address ioctl.
 */
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/ioport.h>
#include <linux/mfd/syscon.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/uaccess.h>
#include <linux/ktime.h>
#include <linux/slab.h>
#include "qc07f_hwenc.h"
#include "dma_integrity.h"
#include "stage-telemetry.h"

#define CODEC_BASE 0x20020000U
#define CODEC_BYTES 0x500U
#define INPUT_ALLOC 0x200000U
#define OUTPUT_ALLOC 0x200000U
#define OUTPUT_LIMIT 0x100000U
#define CODEC_CLOCK BIT(20)
static const u8 quant_order[64]={
 0,8,16,24,1,9,17,25,32,40,48,56,33,41,49,57,
 2,10,18,26,3,11,19,27,34,42,50,58,35,43,51,59,
 4,12,20,28,5,13,21,29,36,44,52,60,37,45,53,61,
 6,14,22,30,7,15,23,31,38,46,54,62,39,47,55,63
};
static struct {
 struct platform_device *pdev;
 struct miscdevice misc;
 struct regmap *sys;
 void __iomem *regs;
 u8 *input,*output,*refs[2],*aux;
 dma_addr_t input_dma,output_dma,refs_dma[2],aux_dma;
 struct mutex lock;
 struct qc_hwenc_info info;
 bool buffers_ready,powered;
 bool guard_poisoned;
 u32 guarded_input_bytes;
 struct file *h264_owner;
 u32 h264_frame_num,h264_ref_index;
 u32 original_gate,original_reset;
} enc;
/* New telemetry is attached to the actual DMA device, never a global table.
 * Existing singleton hardware 'enc' is inherited, not fixed by this prototype.
 * All accesses after init are under enc.lock; no telemetry in an IRQ callback.
 */
static struct qc_stage_context *qc_codec_trace(void)
{return dev_get_drvdata(&enc.pdev->dev);}
static void qc_codec_legacy_trace(const char *format,...)
{(void)format;} /* quiet old frame prints; side-effectful MMIO args still evaluated */
static void *qc_codec_alloc(size_t bytes,dma_addr_t *dma,gfp_t gfp)
{
 u64 start=ktime_get_ns();
 void *p=dma_alloc_coherent(&enc.pdev->dev,bytes,dma,gfp);
 qc_stage_sample(qc_codec_trace(),QC_PHASE_ALLOC,start,ktime_get_ns(),bytes,p?0:-ENOMEM);
 qc_stage_alloc(qc_codec_trace(),bytes,!!p);return p;
}
static void qc_codec_free(size_t bytes,void *p,dma_addr_t dma)
{
 u64 start=ktime_get_ns();
 dma_free_coherent(&enc.pdev->dev,bytes,p,dma);
 qc_stage_sample(qc_codec_trace(),QC_PHASE_FREE,start,ktime_get_ns(),bytes,0);
 qc_stage_free(qc_codec_trace(),bytes);
}
static bool own_dma(dma_addr_t address,u32 bytes)
{return address>=0x81800000ULL && address+bytes<=0x84000000ULL;}
static int codec_reset_impl(void)
{
 int result;
 /* Exact retail gated-reset sequence, bit20 only; ISP19/USB15 untouched. */
 result=regmap_update_bits(enc.sys,0x1c,CODEC_CLOCK,CODEC_CLOCK);if(result)return result;
 result=regmap_update_bits(enc.sys,0x20,CODEC_CLOCK,CODEC_CLOCK);if(result)return result;
 result=regmap_update_bits(enc.sys,0x1c,CODEC_CLOCK,0);if(result)return result;
 udelay(10);
 result=regmap_update_bits(enc.sys,0x20,CODEC_CLOCK,0);if(result)return result;
 udelay(10);enc.powered=true;return 0;
}
static int codec_reset(void)
{
 u64 start=ktime_get_ns();int result=codec_reset_impl();
 qc_stage_sample(qc_codec_trace(),QC_PHASE_RESET,start,ktime_get_ns(),0,result);
 return result;
}
static int codec_reset_retirement(void)
{
 u64 start=ktime_get_ns();int result=codec_reset_impl();
 qc_stage_sample(qc_codec_trace(),QC_PHASE_RESET_RETIRE,start,ktime_get_ns(),0,result);
 qc_stage_retirement(qc_codec_trace(),result);return result;
}
static int codec_identity(void)
{
 int result=codec_reset();if(result)return result;
 enc.info.id=readl(enc.regs);enc.info.config=readl(enc.regs+0xfc);
 enc.info.status=readl(enc.regs+4);enc.info.control=readl(enc.regs+0x38);
 if(!enc.info.id || enc.info.id==0xffffffff)return -ENODEV;
 return 0;
}
static int prepare_buffers(void)
{
 if(enc.buffers_ready)return 0;
 enc.input=qc_codec_alloc(INPUT_ALLOC,&enc.input_dma,GFP_KERNEL);
 if(!enc.input)return -ENOMEM;
 enc.output=qc_codec_alloc(OUTPUT_ALLOC,&enc.output_dma,GFP_KERNEL);
 if(!enc.output){qc_codec_free(INPUT_ALLOC,enc.input,enc.input_dma);enc.input=NULL;return -ENOMEM;}
 if(!own_dma(enc.input_dma,INPUT_ALLOC)||!own_dma(enc.output_dma,OUTPUT_ALLOC)){
  qc_codec_free(INPUT_ALLOC,enc.input,enc.input_dma);enc.input=NULL;
  qc_codec_free(OUTPUT_ALLOC,enc.output,enc.output_dma);enc.output=NULL;
  return -ERANGE;
 }
 enc.buffers_ready=true;enc.info.input_dma=enc.input_dma;enc.info.output_dma=enc.output_dma;
 memset(enc.input,0xa5,INPUT_ALLOC);memset(enc.output,0xa5,OUTPUT_ALLOC);
 return 0;
}
static int encode_jpeg(struct qc_hwenc_job *job)
{
 u32 regs[64]={0},pixels,input_bytes,status,stream_bits,i,q,wmb,hmb;
 int result,reset_result;
 if(enc.guard_poisoned)return -EOVERFLOW;
 if(!((job->width==1280&&job->height==720)||(job->width==640&&job->height==360)||(job->width==320&&job->height==176)))return -EINVAL;
 pixels=job->width*job->height;input_bytes=pixels*3/2;
 if(job->input_bytes!=input_bytes || job->output_capacity<OUTPUT_LIMIT || !job->input_ptr || !job->output_ptr)return -EINVAL;
 for(i=0;i<128;i++)if(!job->quant[i])return -EINVAL;
 result=codec_identity();if(result)return result;
 /* Actual ASIC capability gate; do not bypass rejected JPEG/width support. */
 if(!(enc.info.config&BIT(25))||(enc.info.config&0xfff)<job->width)return -EOPNOTSUPP;
 result=prepare_buffers();if(result)return result;
 /* Guards are initialized at allocation and checked after EVERY job. Never
  * erase evidence of corruption with a per-frame full-buffer memset.
  * Reinitialize the input tail only for an explicitly changed geometry.
  */
 if(enc.guarded_input_bytes!=input_bytes){memset(enc.input+input_bytes,0xa5,INPUT_ALLOC-input_bytes);enc.guarded_input_bytes=input_bytes;}
 if(copy_from_user(enc.input,(void __user *)(unsigned long)job->input_ptr,input_bytes))return -EFAULT;
 /* Pixel edge padding stays inside owned input; supported first target720p
  * is16-row aligned. Half-height360 fallback is rejected until padded input
  * row-layout is explicitly implemented rather than risking an overread.
  */
 if(job->height%16)return -EOPNOTSUPP;
 wmb=job->width/16;hmb=job->height/16;
 regs[1]=BIT(1); /* documented IRQ disable, use bounded polling */
 regs[2]=0x900a|0x4005|BIT(4); /* retail bus swaps/burst16/clockgate */
 regs[5]=enc.output_dma;
 regs[11]=enc.input_dma;regs[12]=enc.input_dma+pixels;regs[13]=enc.input_dma+pixels*5/4;
 /* JpegEncInit retail local_10[0x7a]=1 maps toframeType at
  * EncAsicFrameStart param2[9]: JPEG usesINTRA, not its zero default.
  */
 regs[14]=(wmb<<19)|(hmb<<10)|BIT(6)|BIT(3)|(2<<1);
 regs[15]=job->width<<12; /* inputYUV420P, no rotation/offset */
 regs[20]=0; /* JPEG420, no slice/restart; hardware emitsEOI */
 regs[24]=OUTPUT_LIMIT/8; /* verified limit in64-bit addresses, size returnsbits */
 dma_wmb();
 for(i=1;i<63;i++)writel(regs[i],enc.regs+i*4);
 for(i=0;i<32;i++){
  q=i<16?0:64;
  q=((u32)job->quant[q+quant_order[(i%16)*4]]<<24)|
    ((u32)job->quant[q+quant_order[(i%16)*4+1]]<<16)|
    ((u32)job->quant[q+quant_order[(i%16)*4+2]]<<8)|
    job->quant[q+quant_order[(i%16)*4+3]];
  writel(q,enc.regs+0x100+i*4);
 }
 writel(regs[14]|BIT(0),enc.regs+0x38);
  qc_codec_legacy_trace("qc07f-hwenc: start id=%08x ctrl=%08x bus=%08x stride=%08x y=%08x u=%08x v=%08x out=%08x limit=%08x\n",enc.info.id,readl(enc.regs+0x38),readl(enc.regs+8),readl(enc.regs+0x3c),readl(enc.regs+0x2c),readl(enc.regs+0x30),readl(enc.regs+0x34),readl(enc.regs+0x14),readl(enc.regs+0x60));
 result=readl_poll_timeout(enc.regs+4,status,status&(BIT(2)|BIT(3)|BIT(4)|BIT(5)),100,1000000);
 stream_bits=readl(enc.regs+0x60);enc.info.status=status;enc.info.control=readl(enc.regs+0x38);
  {u32 gate=0,reset=0,pll=0;regmap_read(enc.sys,0x1c,&gate);regmap_read(enc.sys,0x20,&reset);regmap_read(enc.sys,0x14,&pll);qc_codec_legacy_trace("qc07f-hwenc: finish status=%08x ctrl=%08x bits=%08x gate=%08x reset=%08x pll=%08x\n",status,enc.info.control,stream_bits,gate,reset,pll);}
 writel(enc.info.control&~BIT(0),enc.regs+0x38);
  reset_result=codec_reset_retirement(); /* quiesce DMA before checking/copying/freeing */
 if(reset_result)return reset_result;
 dma_rmb();enc.info.guard_errors=0;
 enc.info.guard_errors=camera_guard_errors_ram(enc.input+input_bytes,INPUT_ALLOC-input_bytes);
 enc.info.guard_errors+=camera_guard_errors_ram(enc.output+OUTPUT_LIMIT,OUTPUT_ALLOC-OUTPUT_LIMIT);
 job->status=status;job->output_bytes=0;enc.info.last_bytes=0;
 if(enc.info.guard_errors){enc.guard_poisoned=true;return -EOVERFLOW;}
 if(result)return result;
 if(status&(BIT(3)|BIT(4)|BIT(5)))return -EIO;
 if(!(status&BIT(2)) || !stream_bits || (stream_bits&7) || stream_bits/8>OUTPUT_LIMIT)return -EIO;
 job->output_bytes=stream_bits/8;enc.info.last_bytes=job->output_bytes;
 if(copy_to_user((void __user *)(unsigned long)job->output_ptr,enc.output,job->output_bytes))return -EFAULT;
 return 0;
}
static int prepare_h264_buffers(void)
{
 unsigned i;
 if(enc.refs[0])return 0;
 /* Retail allocates each reference separately; do not require a4MiB
  * physically contiguous order10 block on the measured40MiB target.
  */
 for(i=0;i<2;i++){
  enc.refs[i]=qc_codec_alloc(0x200000,&enc.refs_dma[i],GFP_KERNEL);
  if(!enc.refs[i]||!own_dma(enc.refs_dma[i],0x200000))goto failed;
 }
 enc.aux=qc_codec_alloc(0x10000,&enc.aux_dma,GFP_KERNEL);
 if(!enc.aux||!own_dma(enc.aux_dma,0x10000))goto failed;
 for(i=0;i<2;i++){
  memset(enc.refs[i],0xa5,0x200000);
  memset(enc.refs[i],0,1280*720);
  memset(enc.refs[i]+0x100000,0,1280*720/2);
 }
 memset(enc.aux,0,0x10000);memset(enc.aux+0xff00,0xa5,0x100);
 return 0;
failed:
  if(enc.aux)qc_codec_free(0x10000,enc.aux,enc.aux_dma);
  for(i=0;i<2;i++){
   if(enc.refs[i])qc_codec_free(0x200000,enc.refs[i],enc.refs_dma[i]);
   enc.refs[i]=NULL;
  }
  enc.aux=NULL;
  return -ENOMEM;
}
static int encode_h264(struct qc_hwenc_h264_job *job,bool idr,u32 frame_num,u32 read_ref)
{
 u32 regs[64]={0},status=0,bits,i,guards=0;
 const u32 pixels=1280*720,bytes=pixels*3/2;
 int result;
 u64 stamp;
 u64 program_start,program_end,wait_start,wait_end;
 int reset_result;
 job->output_bytes=0;job->status=0;
 if(enc.guard_poisoned)return -EOVERFLOW;
 if(job->width!=1280||job->height!=720||job->input_bytes!=bytes||
    job->output_capacity<OUTPUT_LIMIT||!job->input_ptr||!job->output_ptr)return -EINVAL;
 result=codec_identity();if(result)return result;
 if(!(enc.info.config&BIT(27))||(enc.info.config&0xfff)<1280)return -EOPNOTSUPP;
 stamp=ktime_get_ns();
 result=prepare_buffers();
 qc_stage_sample(qc_codec_trace(),QC_PHASE_PREPARE,stamp,ktime_get_ns(),0,result);
 if(result)return result;
 stamp=ktime_get_ns();
 result=prepare_h264_buffers();
 qc_stage_sample(qc_codec_trace(),QC_PHASE_PREPARE,stamp,ktime_get_ns(),0,result);
 if(result)return result;
 if(enc.guarded_input_bytes!=bytes){memset(enc.input+bytes,0xa5,INPUT_ALLOC-bytes);enc.guarded_input_bytes=bytes;}
 /* Retail EncAsicMemAlloc_V2: MB*256 luma, MB*128 chroma;
  * two separate luma/chroma slots, with owned guard gaps. CAVLC selected,
  * CABAC/MV inactive addresses still resolve inside owned auxiliary RAM.
  */
 /* IDR has no previous-frame dependency. Reference allocation is initialized
  * once; hardware-reconstructed contents may persist between independentIDRs.
  * All ref/aux guard gaps are still validated before delivering each frame.
  */
 stamp=ktime_get_ns();
 if(copy_from_user(enc.input,(void __user *)(unsigned long)job->input_ptr,bytes)){
  qc_stage_sample(qc_codec_trace(),QC_PHASE_PIXEL_COPY,stamp,ktime_get_ns(),bytes,-EFAULT);return -EFAULT;
 }
 qc_stage_sample(qc_codec_trace(),QC_PHASE_PIXEL_COPY,stamp,ktime_get_ns(),bytes,0);
 program_start=ktime_get_ns();
 regs[1]=BIT(1);regs[2]=0x900a|0x4005|BIT(4);
 regs[5]=enc.output_dma;regs[6]=enc.aux_dma+0xc000;
 regs[7]=enc.refs_dma[read_ref];regs[8]=enc.refs_dma[read_ref]+0x100000;
 regs[9]=enc.refs_dma[read_ref^1];regs[10]=enc.refs_dma[read_ref^1]+0x100000;
 regs[11]=enc.input_dma;regs[12]=enc.input_dma+pixels;regs[13]=enc.input_dma+pixels*5/4;
 regs[14]=BIT(29)|(80<<19)|(45<<10)|(idr?BIT(3):0)|(3<<1);
 regs[15]=1280<<12;
 regs[16]=(26U<<26)|(2<<13)|BIT(5); /* PPS initialQP26/chroma+2, deblock disabled */
 regs[18]=BIT(16)|frame_num; /* CAVLC,no8x8,noMVC,16-bit frame_num */
 regs[19]=32; /* exact REA35598[QP26] DMVPenalty1p; 4p default0 */
 regs[24]=OUTPUT_LIMIT/8;
 regs[27]=(26U<<26)|(26U<<20)|(26U<<14); /* fixedQP26, checkpoints disabled */
 regs[51]=enc.aux_dma;regs[52]=enc.aux_dma+0xc400;
 /* Disabled areas use retail initialization width/height outside frame. */
 regs[56]=(80U<<24)|(80U<<16)|(45U<<8)|45;
 regs[60]=regs[61]=regs[56];
 dma_wmb();for(i=1;i<63;i++)writel(regs[i],enc.regs+i*4);
 writel(regs[14]|BIT(0),enc.regs+0x38);
 program_end=ktime_get_ns();
 wait_start=ktime_get_ns();
 result=readl_poll_timeout(enc.regs+4,status,status&(BIT(2)|BIT(3)|BIT(4)|BIT(5)),100,1000000);
 wait_end=ktime_get_ns();
 bits=readl(enc.regs+0x60);enc.info.control=readl(enc.regs+0x38);
 writel(enc.info.control&~BIT(0),enc.regs+0x38);
 reset_result=codec_reset_retirement();
 /* Stop/control and the original reset attempt precede bounded aggregation.
  * Failure still skips all guards/copies as before; it is not retirement proof.
  */
 qc_stage_sample(qc_codec_trace(),QC_PHASE_PROGRAM,program_start,program_end,0,0);
 qc_stage_sample(qc_codec_trace(),QC_PHASE_FRAME_WAIT,wait_start,wait_end,0,result);
 if(reset_result)return -EIO;
 dma_rmb();job->status=status;enc.info.status=status;enc.info.last_bytes=0;
 stamp=ktime_get_ns();
 guards=camera_guard_errors_ram(enc.input+bytes,INPUT_ALLOC-bytes);
 guards+=camera_guard_errors_ram(enc.output+OUTPUT_LIMIT,OUTPUT_ALLOC-OUTPUT_LIMIT);
 for(i=0;i<2;i++){
  guards+=camera_guard_errors_ram(enc.refs[i]+pixels,0x100000-pixels);
  guards+=camera_guard_errors_ram(enc.refs[i]+0x100000+pixels/2,0x100000-pixels/2);
 }
 guards+=camera_guard_errors_ram(enc.aux+0xff00,0x100);
 enc.info.guard_errors=guards;
 qc_stage_sample(qc_codec_trace(),QC_PHASE_GUARDS,stamp,ktime_get_ns(),
  INPUT_ALLOC-bytes+OUTPUT_ALLOC-OUTPUT_LIMIT+2*(0x100000-pixels+0x100000-pixels/2)+0x100,guards?-EOVERFLOW:0);
 qc_codec_legacy_trace("qc07f-hwenc: H264-%s num=%u result=%d status=%08x bits=%u guards=%u ref0=%pad ref1=%pad aux=%pad\n",idr?"IDR":"P",frame_num,result,status,bits,guards,&enc.refs_dma[0],&enc.refs_dma[1],&enc.aux_dma);
 if(guards){enc.guard_poisoned=true;return -EOVERFLOW;}
 if(result)return result;
 if((status&(BIT(3)|BIT(4)|BIT(5)))||!(status&BIT(2))||!bits||(bits&7)||bits/8>OUTPUT_LIMIT)return -EIO;
 job->output_bytes=bits/8;enc.info.last_bytes=job->output_bytes;
 stamp=ktime_get_ns();
 if(copy_to_user((void __user *)(unsigned long)job->output_ptr,enc.output,job->output_bytes)){
  qc_stage_sample(qc_codec_trace(),QC_PHASE_OUTPUT_COPY,stamp,ktime_get_ns(),job->output_bytes,-EFAULT);return -EFAULT;
 }
 qc_stage_sample(qc_codec_trace(),QC_PHASE_OUTPUT_COPY,stamp,ktime_get_ns(),job->output_bytes,0);
 return 0;
}
static long encoder_ioctl(struct file *file,unsigned int command,unsigned long argument)
{
 int result;
 if(command==QC_HWENC_STAGE_QUERY){
  struct qc_stage_query query;
  if(mutex_lock_interruptible(&enc.lock))return -ERESTARTSYS;
  if(copy_from_user(&query,(void __user *)argument,sizeof(query)))result=-EFAULT;
  else if(qc_stage_query(qc_codec_trace(),&query))result=-EINVAL;
  else result=copy_to_user((void __user *)argument,&query,sizeof(query))?-EFAULT:0;
  mutex_unlock(&enc.lock);return result;
 }
 if(command!=QC_HWENC_INFO && command!=QC_HWENC_JPEG && command!=QC_HWENC_H264_IDR && command!=QC_HWENC_H264_STREAM)return -ENOTTY;
 if(mutex_lock_interruptible(&enc.lock))return -ERESTARTSYS;
 qc_stage_begin(qc_codec_trace(),command==QC_HWENC_INFO?QC_STAGE_INFO:
  command==QC_HWENC_JPEG?QC_STAGE_JPEG:QC_STAGE_H264,ktime_get_ns());
 if(enc.h264_owner&&enc.h264_owner!=file){
  qc_stage_end(qc_codec_trace(),-EBUSY,0,ktime_get_ns());mutex_unlock(&enc.lock);return -EBUSY;
 }
 if(command==QC_HWENC_INFO){
  result=codec_identity();
  if(!result&&copy_to_user((void __user *)argument,&enc.info,sizeof(enc.info)))result=-EFAULT;
 }else if(command==QC_HWENC_H264_STREAM){
  struct qc_hwenc_h264_stream_job job;
  if(copy_from_user(&job,(void __user *)argument,sizeof(job)))result=-EFAULT;
  else if(job.keyframe>1)result=-EINVAL;
  else if(!job.keyframe&&!enc.h264_owner)result=-ENODATA;
  else{
   u32 num=job.keyframe?0:(enc.h264_frame_num+1)&0xffff;
   u32 ref=job.keyframe?0:enc.h264_ref_index;
   result=encode_h264(&job.frame,job.keyframe,num,ref);job.frame_num=num;
   if(copy_to_user((void __user *)argument,&job,sizeof(job)))result=-EFAULT;
    if(!result){enc.h264_owner=file;enc.h264_frame_num=num;enc.h264_ref_index=ref^1;
     qc_stage_owner(qc_codec_trace(),1,job.keyframe);}
   else enc.h264_owner=NULL;
  }
 }else if(command==QC_HWENC_H264_IDR){
  struct qc_hwenc_h264_job job;
  if(copy_from_user(&job,(void __user *)argument,sizeof(job)))result=-EFAULT;
  else{
   result=enc.h264_owner?-EBUSY:encode_h264(&job,true,0,0);
   if(copy_to_user((void __user *)argument,&job,sizeof(job)))result=-EFAULT;
  }
 }else{
  struct qc_hwenc_job job;
  if(copy_from_user(&job,(void __user *)argument,sizeof(job)))result=-EFAULT;
  else{
    result=enc.h264_owner?-EBUSY:encode_jpeg(&job);
   if(copy_to_user((void __user *)argument,&job,sizeof(job)))result=-EFAULT;
   qc_codec_legacy_trace("qc07f-hwenc: JPEG result=%d status=%08x bytes=%u guard_errors=%u\n",result,enc.info.status,enc.info.last_bytes,enc.info.guard_errors);
  }
 }
 qc_stage_owner(qc_codec_trace(),!!enc.h264_owner,0);
 qc_stage_end(qc_codec_trace(),result,command!=QC_HWENC_INFO,ktime_get_ns());
 mutex_unlock(&enc.lock);return result;
}
static int encoder_release(struct inode *inode,struct file *file)
{
 mutex_lock(&enc.lock);
 if(enc.h264_owner==file){
  qc_stage_begin(qc_codec_trace(),QC_STAGE_END,ktime_get_ns());
  enc.h264_owner=NULL;
  qc_stage_owner(qc_codec_trace(),!!enc.h264_owner,0);
  qc_stage_end(qc_codec_trace(),0,0,ktime_get_ns());
 }
 mutex_unlock(&enc.lock);return 0;
}
static const struct file_operations encoder_ops={.owner=THIS_MODULE,.unlocked_ioctl=encoder_ioctl,.release=encoder_release};
static int __init encoder_init(void)
{
 int result;
 struct qc_stage_context *telemetry;
 mutex_init(&enc.lock);
 enc.sys=syscon_regmap_lookup_by_compatible("anyka,ak3918ev200-intc");if(IS_ERR(enc.sys))return PTR_ERR(enc.sys);
 if(regmap_read(enc.sys,0x1c,&enc.original_gate)||regmap_read(enc.sys,0x20,&enc.original_reset))return -EIO;
 if(!request_mem_region(CODEC_BASE,CODEC_BYTES,"qc07f-hwenc"))return -EBUSY;
 enc.regs=ioremap(CODEC_BASE,CODEC_BYTES);if(!enc.regs){result=-ENOMEM;goto release;}
 enc.pdev=platform_device_register_simple("qc07f-hwenc-dma",-1,NULL,0);if(IS_ERR(enc.pdev)){result=PTR_ERR(enc.pdev);goto unmap;}
 telemetry=devm_kzalloc(&enc.pdev->dev,sizeof(*telemetry),GFP_KERNEL);
 if(!telemetry){result=-ENOMEM;goto device;}
 qc_stage_codec_coverage(telemetry);
 dev_set_drvdata(&enc.pdev->dev,telemetry);
 enc.pdev->dev.coherent_dma_mask=DMA_BIT_MASK(32);enc.pdev->dev.dma_mask=&enc.pdev->dev.coherent_dma_mask;
 result=dma_set_mask_and_coherent(&enc.pdev->dev,DMA_BIT_MASK(32));if(result)goto device;
 enc.misc.minor=MISC_DYNAMIC_MINOR;enc.misc.name="qc07f-encoder";enc.misc.fops=&encoder_ops;enc.misc.mode=0600;
 result=misc_register(&enc.misc);if(result)goto device;
 pr_info("qc07f-hwenc: explicit owned codec20020000..20020500; no DMA/clock/reset/start at probe\n");return 0;
device:platform_device_unregister(enc.pdev);
unmap:iounmap(enc.regs);
release:release_mem_region(CODEC_BASE,CODEC_BYTES);return result;
}
static void __exit encoder_exit(void)
{
 misc_deregister(&enc.misc);mutex_lock(&enc.lock);
 qc_stage_begin(qc_codec_trace(),QC_STAGE_END,ktime_get_ns());
 if(enc.powered){writel(0,enc.regs+0x38);codec_reset_retirement();}
 if(enc.buffers_ready){qc_codec_free(INPUT_ALLOC,enc.input,enc.input_dma);qc_codec_free(OUTPUT_ALLOC,enc.output,enc.output_dma);}
 if(enc.refs[0]){qc_codec_free(0x200000,enc.refs[0],enc.refs_dma[0]);qc_codec_free(0x200000,enc.refs[1],enc.refs_dma[1]);qc_codec_free(0x10000,enc.aux,enc.aux_dma);}
 if(enc.powered){regmap_update_bits(enc.sys,0x1c,CODEC_CLOCK,enc.original_gate&CODEC_CLOCK);regmap_update_bits(enc.sys,0x20,CODEC_CLOCK,enc.original_reset&CODEC_CLOCK);}
 qc_stage_owner(qc_codec_trace(),0,0);
 qc_stage_end(qc_codec_trace(),qc_codec_trace()->active_record.retirement_failed?-EIO:0,0,ktime_get_ns()); /* inherited unsafe exit still ignores reset failure */
 mutex_unlock(&enc.lock);platform_device_unregister(enc.pdev);iounmap(enc.regs);release_mem_region(CODEC_BASE,CODEC_BYTES);
}
module_init(encoder_init);module_exit(encoder_exit);
MODULE_LICENSE("GPL");MODULE_DESCRIPTION("QC07F bounded retail-correlated hardware JPEG port; explicit-only owned DMA");
