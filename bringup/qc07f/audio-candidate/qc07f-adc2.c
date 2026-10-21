// SPDX-License-Identifier: GPL-2.0-only
/* Anyka capture-only ALSA adaptation. Original ak_pcm.c copyright Anyka Inc.
 * and GPL v2-or-later notice preserved byte-exact under evidence upstream/.
 * No stock playback/DAC/speaker/HP/AEC/GPIO setup or raw L2 reset is reused.
 * Probe is UNCONDITIONALLY no-effect. This is an object/source candidate only.
 */
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/ktime.h>
#include <linux/mutex.h>
#include <linux/uio.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/control.h>
#include "capture_core.h"

/* Future qualified resource broker owns this context, device/card references,
 * provider code, coherent ring and serialization. NOT devm-owned. No broker is
 * instantiated here. Provider completion dispatch and ALSA stream lifetime must
 * be serialized by that owner; IRQ receipts must never retain a substream ptr.
 * Unknown retirement quarantines backing/code, even after ALSA handle close.
 */
struct qc_audio {
 struct device *dev;
 struct mutex lock;
 struct ac_core core;
 struct ac_clock clock;
 void *ring;
 dma_addr_t dma;
 struct ac_span borrowed;
 size_t consumed;
 bool has_borrow, module_pinned;
};
int qc_audio_attach_capture_candidate(struct snd_pcm *,struct qc_audio *);
int qc_audio_complete_candidate(struct qc_audio *,const struct ac_completion *);
static const unsigned int qc_rates[] = {8000,16000,24000,48000};
static const struct snd_pcm_hw_constraint_list qc_rate_list = {
 .count=ARRAY_SIZE(qc_rates),.list=qc_rates,.mask=0
};
static const struct snd_pcm_hardware qc_hardware = {
 .info=SNDRV_PCM_INFO_INTERLEAVED|SNDRV_PCM_INFO_BLOCK_TRANSFER,
 .formats=SNDRV_PCM_FMTBIT_S16_LE,.rates=SNDRV_PCM_RATE_KNOT,
 .rate_min=8000,.rate_max=48000,.channels_min=1,.channels_max=1,
 .buffer_bytes_max=AC_RING_BYTES,.period_bytes_min=AC_PERIOD_BYTES,
 .period_bytes_max=AC_PERIOD_BYTES,.periods_min=AC_SLOTS,.periods_max=AC_SLOTS
};
static int qc_open(struct snd_pcm_substream *s)
{
 struct qc_audio *a=snd_pcm_substream_chip(s);
 if(s->stream!=SNDRV_PCM_STREAM_CAPTURE||!a||a->core.self!=&a->core)return -EOPNOTSUPP;
 s->runtime->hw=qc_hardware;
 return snd_pcm_hw_constraint_list(s->runtime,0,SNDRV_PCM_HW_PARAM_RATE,&qc_rate_list);
}
static int qc_hw_params(struct snd_pcm_substream *s,struct snd_pcm_hw_params *p)
{
 struct qc_audio *a=snd_pcm_substream_chip(s);struct ac_params params;struct ac_rate rate;int ret;
 params.rate=params_rate(p);params.channels=params_channels(p);params.sample_bits=16;
 params.period_bytes=params_period_bytes(p);params.periods=params_periods(p);
 if(params_format(p)!=SNDRV_PCM_FORMAT_S16_LE||params_buffer_bytes(p)!=AC_RING_BYTES)return -EINVAL;
 ret=ac_rate_plan(&params,&a->clock,&rate);if(ret)return -EOPNOTSUPP;
 mutex_lock(&a->lock);
 if(!ac_can_free(&a->core)||a->ring){ret=-EBUSY;goto out;}
 if(a->core.generation==AC_MAX64){ret=-EOVERFLOW;goto out;}
 if(!try_module_get(THIS_MODULE)){ret=-ENODEV;goto out;}
 a->module_pinned=true;
 /* Own coherent backing, NOT ALSA-managed automatic free or speculative CMA.
  * Existing platform DMA policy applies; no CMA/reserve/config changes here. */
 a->ring=dma_alloc_coherent(a->dev,AC_RING_BYTES,&a->dma,GFP_KERNEL);
 if(!a->ring){module_put(THIS_MODULE);a->module_pinned=false;ret=-ENOMEM;goto out;}
 ret=ac_prepare(&a->core,&params,&a->clock,a->ring,AC_RING_BYTES,a->core.generation+1,ktime_get_ns());
 if(ret<0&&ac_can_free(&a->core)){
  dma_free_coherent(a->dev,AC_RING_BYTES,a->ring,a->dma);a->ring=NULL;
  module_put(THIS_MODULE);a->module_pinned=false;goto out;
 }
 s->runtime->dma_area=a->ring;s->runtime->dma_addr=a->dma;s->runtime->dma_bytes=AC_RING_BYTES;
 ret=ret==AC_OK?0:-EAGAIN; /* CLAIM/CONFIG ACK readiness is not fabricated. */
out: mutex_unlock(&a->lock);return ret;
}
static int qc_prepare(struct snd_pcm_substream *s)
{
 struct qc_audio *a=snd_pcm_substream_chip(s);int ret;
 mutex_lock(&a->lock);ret=a->core.state==AC_PREPARED&&!a->core.fault?0:-EAGAIN;
 mutex_unlock(&a->lock);return ret;
}
static int qc_trigger(struct snd_pcm_substream *s,int command)
{
 struct qc_audio *a=snd_pcm_substream_chip(s);int ret;
 /* Attach helper below selects nonatomic PCM ops: these callbacks may lock. */
 mutex_lock(&a->lock);
 switch(command){
 case SNDRV_PCM_TRIGGER_START:ret=ac_start(&a->core,ktime_get_ns());break;
 case SNDRV_PCM_TRIGGER_STOP:case SNDRV_PCM_TRIGGER_SUSPEND:ret=ac_close(&a->core,ktime_get_ns());break;
 default:ret=AC_INVALID;break;
 }
 mutex_unlock(&a->lock);return ret==AC_OK||ret==AC_RETAINED?0:-EIO;
}
static int qc_sync_stop(struct snd_pcm_substream *s)
{
 struct qc_audio *a=snd_pcm_substream_chip(s);int ret;
 mutex_lock(&a->lock);ret=ac_close(&a->core,ktime_get_ns());
 if(a->has_borrow){
  /* This is the glue's exact internal copy cursor, not an external borrow. */
  ac_return(&a->core,&a->borrowed.ticket,ktime_get_ns());a->has_borrow=false;
 }
 mutex_unlock(&a->lock);
 /* Never pretend an async fence/IRQ reset is complete to satisfy ALSA. Future
  * resource broker must finish bounded normal STOP/RELEASE and hold backing on
  * EAGAIN. No generic synchronize/reset of shared UART/MCI L2 is permissible. */
 return ret==AC_OK?0:-EAGAIN;
}
static int qc_hw_free(struct snd_pcm_substream *s)
{
 struct qc_audio *a=snd_pcm_substream_chip(s);int ret=0;
 mutex_lock(&a->lock);ac_close(&a->core,ktime_get_ns());
 if(!ac_can_free(&a->core)){ret=-EAGAIN;goto out;}
 if(a->ring){dma_free_coherent(a->dev,AC_RING_BYTES,a->ring,a->dma);a->ring=NULL;}
 if(a->module_pinned){module_put(THIS_MODULE);a->module_pinned=false;}
 s->runtime->dma_area=NULL;s->runtime->dma_bytes=0;
out: mutex_unlock(&a->lock);return ret;
}
static int qc_close(struct snd_pcm_substream *s){return qc_sync_stop(s);}
static snd_pcm_uframes_t qc_pointer(struct snd_pcm_substream *s)
{
 struct qc_audio *a=snd_pcm_substream_chip(s);snd_pcm_uframes_t pointer;
 mutex_lock(&a->lock);pointer=a->core.xrun?SNDRV_PCM_POS_XRUN:
  (snd_pcm_uframes_t)(a->core.next_sample_index%(AC_RING_BYTES/2));
 mutex_unlock(&a->lock);return pointer;
}
static int qc_copy(struct snd_pcm_substream *s,int channel,unsigned long position,
 struct iov_iter *destination,unsigned long bytes)
{
 struct qc_audio *a=snd_pcm_substream_chip(s);unsigned long done=0;int ret=0;
 if(channel!=0||bytes>AC_RING_BYTES||position>=AC_RING_BYTES||(bytes&1))return -EINVAL;
 mutex_lock(&a->lock);
 while(done<bytes){
  size_t part;
  if(!a->has_borrow){
   ret=ac_borrow(&a->core,&a->borrowed);if(ret){ret=-EAGAIN;break;}
   a->has_borrow=true;a->consumed=0;
  }
  if(ac_span_valid(&a->core,&a->borrowed)!=AC_OK||position!=(a->borrowed.slot*AC_PERIOD_BYTES+a->consumed)%AC_RING_BYTES){ret=-EPIPE;break;}
  part=min_t(size_t,bytes-done,AC_PERIOD_BYTES-a->consumed);
  /* Nonatomic copy path holds owner mutex, preventing mute/close/effect races.
   * Provider completion dispatch must be deferred, not hard-IRQ/mutex context. */
  if(copy_to_iter(a->borrowed.data+a->consumed,part,destination)!=part){ret=-EFAULT;break;}
  a->consumed+=part;done+=part;position=(position+part)%AC_RING_BYTES;
  if(a->consumed==AC_PERIOD_BYTES){
   ac_return(&a->core,&a->borrowed.ticket,ktime_get_ns());a->has_borrow=false;
  }
 }
 if(ret&&a->has_borrow){
  ac_source_fault(&a->core,ktime_get_ns());ac_return(&a->core,&a->borrowed.ticket,ktime_get_ns());a->has_borrow=false;
 }
 mutex_unlock(&a->lock);return ret;
}
static const struct snd_pcm_ops qc_pcm_ops = {
 .open=qc_open,.close=qc_close,.hw_params=qc_hw_params,.hw_free=qc_hw_free,
 .prepare=qc_prepare,.trigger=qc_trigger,.sync_stop=qc_sync_stop,.pointer=qc_pointer,.copy=qc_copy
};
static int qc_gain_info(struct snd_kcontrol *control,struct snd_ctl_elem_info *info)
{
 (void)control;info->type=SNDRV_CTL_ELEM_TYPE_INTEGER;info->count=1;
 info->value.integer.min=0;info->value.integer.max=7;return 0;
}
static int qc_gain_get(struct snd_kcontrol *control,struct snd_ctl_elem_value *value)
{
 struct qc_audio *a=snd_kcontrol_chip(control);
 if(a->clock.ready_mask!=AC_RESOURCE_REQUIRED)return -EOPNOTSUPP;
 value->value.integer.value[0]=a->clock.mic_gain_code;return 0; /* raw code, NOT dB */
}
static const struct snd_kcontrol_new qc_gain_control = {
 .iface=SNDRV_CTL_ELEM_IFACE_MIXER,.name="ADC2 Candidate Mic Gain Raw Code",
 .access=SNDRV_CTL_ELEM_ACCESS_READ,.info=qc_gain_info,.get=qc_gain_get
};
/* Actual glue seam, not a native backend: caller supplies broker-owned card/PCM
 * and context. No card/provider is allocated or registered by probe. External
 * completion/period notifications must retain ALSA backing until known fence;
 * this native broker/callback lifetime remains an explicit qualification blocker.
 */
int qc_audio_attach_capture_candidate(struct snd_pcm *pcm,struct qc_audio *a)
{
 if(!pcm||!a||a->core.self!=&a->core||a->clock.ready_mask!=AC_RESOURCE_REQUIRED)return -EOPNOTSUPP;
 pcm->private_data=a;pcm->nonatomic=true;
 snd_pcm_set_ops(pcm,SNDRV_PCM_STREAM_CAPTURE,&qc_pcm_ops);
 /* Metadata-only read control; no set-gain/mute GPIO or playback control. */
 return snd_ctl_add(pcm->card,snd_ctl_new1(&qc_gain_control,a));
}
int qc_audio_complete_candidate(struct qc_audio *a,const struct ac_completion *receipt)
{
 int ret;
 mutex_lock(&a->lock);ret=ac_complete(&a->core,receipt,ktime_get_ns());
 if(ret==AC_OK&&a->core.state==AC_CAPTURING&&!a->core.operation)ret=ac_pump(&a->core,ktime_get_ns());
 mutex_unlock(&a->lock);return ret;
}
static int qc_probe(struct platform_device *pdev)
{
 /* Unconditional admission refusal BEFORE allocation, device/card claim,
  * GPIO/mic bias/ADC2 clock/L2/IRQ/DMA access. No typed flag can bypass this. */
 (void)pdev;return -EOPNOTSUPP;
}
static const struct of_device_id qc_audio_of[] = {{.compatible="anyka,qc07f-adc2-capture-candidate"},{}};
MODULE_DEVICE_TABLE(of,qc_audio_of);
static struct platform_driver qc_audio_driver = {.probe=qc_probe,
 .driver={.name="qc07f-adc2-capture-candidate",.of_match_table=qc_audio_of}};
module_platform_driver(qc_audio_driver);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("QC07F ADC2 capture-only source candidate; probe refuses all hardware");
