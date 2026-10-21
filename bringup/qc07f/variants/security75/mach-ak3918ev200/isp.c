// SPDX-License-Identifier: GPL-2.0-only
/* Bounded QC07F ISP first-frame diagnostic, not a production V4L2 driver.
 * REA-retained isp2_module_init, buffer/stat/3DNR/OSD address setters, mode,
 * start and stop primitives; sanitized live retail calibration facts.
 * No stock executable code, SPI writes or old DMA buffer addresses are used.
 * All DMA destinations are owned coherent allocations, kept until reset.
 * CPU ISP IRQ remains masked: poll pending status for at most2s, then reset19
 * before making data readable. Do not equate changed bytes with valid imagery.
 */
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/mfd/syscon.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/crc32.h>
#include <linux/jiffies.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/irqdomain.h>
#include <linux/completion.h>
#include <linux/ktime.h>
#include <linux/vmalloc.h>
#include <linux/kref.h>
#include <linux/ioport.h>
#include <linux/of_address.h>
#include <linux/dma-map-ops.h>
#include "capture-retirement.h"
#include "capture-retirement.c"
#ifdef MODULE
#error "resource03 is built-in/static-DT only; module reload identity/provider lifetime not qualified"
#endif
#include "isp-reference.h"
#include "isp-stream.h"
#include "isp-whitebalance.h"
#include "dma_integrity.h"
#include "crc_ram01.h"
#include "stage-telemetry.h"

#define MAIN_PIXELS (1280U * 720U)
#define MAIN_BYTES (MAIN_PIXELS * 3U / 2U)
#define SUB_PIXELS (640U * 360U)
#define SUB_BYTES (SUB_PIXELS * 3U / 2U)
#define FRAME_BYTES (MAIN_BYTES + SUB_BYTES)
#define FRAME_ALLOC 0x200000U
#define TABLE_ALLOC 0x4000U
#define REF_ALLOC 0x400000U
#define STAT_BYTES 0x10c0U /* conservative retained 0x2e20 - 0x1d60 extent */
#define STAT_ALLOC 0x3000U
#define BUFFERS_MASK (GENMASK(18, 15) | GENMASK(24, 21))

struct ak_isp;
struct ak_isp_file {
 struct kref references;
 struct ak_isp *isp;
 struct qc_retirement_file receipt;
 bool receipt_visible;
};
struct ak_isp {
 struct kref references;
 struct qc_retirement_source receipt_source;
 struct ak_isp_file *session;
 struct device_node *sys_np;
 resource_size_t region_start,region_bytes;
 bool region_owned,ready,disconnected,quarantined,dma_retired,misc_registered,attrs_registered;
 int quarantine_result;
 enum qc_capture_stop_reason stop_reason;
	struct qc_stage_context telemetry; /* isp->lock; never written from IRQ */
	struct device *dev;
	void __iomem *base;
	struct regmap *sys;
	u32 *table;
	u8 *frame, *reference;
	dma_addr_t table_dma, frame_dma, reference_dma;
	struct miscdevice misc;
	struct mutex lock;
	u32 status, changed_main, changed_sub, guard_errors;
	u32 observed_ctrl, observed_table, observed_order, pll14_before, pll14_after;
	u32 frame_crc;
	u32 crc_table[256];
	u32 uv_gain;
	bool colour_override;
	u32 wb_preset, wb_table_preset;
	u32 wb_desired_words[3], wb_prepared_words[3];
	u64 wb_desired_changed_ns, wb_last_prepared_ns;
	bool wb_table_prepared;
	int irq;
	bool irq_mode,irq_registered,irq_inflight;
	u32 irq_status,irq_frames,irq_unexpected;
	u32 irq_no_done,irq_late_done,irq_last_extra_status;
	struct completion irq_done;
	u8 *ring_frame[4];
	dma_addr_t ring_dma[4];
	u8 *ring_stat[4];
	dma_addr_t ring_stat_dma[4];
	bool ring_test_active;
	u32 ring_completed,ring_status[16],ring_control[16],ring_elapsed[16];
	u32 ring_limit;
	bool ring_hold_selected,ring_held_ready,ring_bad_order;
	u32 ring_held_slot;
	struct completion ring_first_done;
	bool queue_mode,queue_started,queue_request,queue_held,queue_bad,queue_expired;
	u32 queue_slot,queue_status,queue_events,queue_dropped,queue_sequence,queue_crc;
	u64 queue_token_counter,queue_token;
	struct completion queue_done;
	raw_spinlock_t queue_lock;
	unsigned long ring_started;
	int last_result;
	bool readable;
	struct ak_isp_file *stream_owner;
	u32 stream_sequence;
	u32 stream_pre_gate_wait_timeouts;
	unsigned long stream_deadline;
};
/* SPDX-License-Identifier: GPL-2.0-only
 * Concrete resource/per-open lifetime for the COPIED scalar72 ISP only.
 * Built-in static DT scope; no claim of physical hot-unplug or power-loss safety.
 */
static DEFINE_MUTEX(ak_isp_registry_lock);
static struct qc_retirement_registry ak_isp_ids;
static struct ak_isp *ak_isp_live;
static void ak_isp_bundle_release(struct kref *ref);
static int ak_isp_stream_stop(struct ak_isp *isp);
static void ak_isp_bundle_put(struct ak_isp *isp)
{ kref_put(&isp->references, ak_isp_bundle_release); }
static void ak_isp_file_release_ref(struct kref *ref)
{
 struct ak_isp_file *fc=container_of(ref,struct ak_isp_file,references);
 struct ak_isp *isp=fc->isp;
 WARN_ON(fc->receipt.open || isp->session == fc);
 kfree(fc);ak_isp_bundle_put(isp);
}
static void ak_isp_file_put(struct ak_isp_file *fc)
{ kref_put(&fc->references,ak_isp_file_release_ref); }
static struct ak_isp_file *ak_isp_file_alloc(struct ak_isp *isp)
{
 struct ak_isp_file *fc=kzalloc(sizeof(*fc),GFP_KERNEL);
 int ret;
 if(!fc)return ERR_PTR(-ENOMEM);
 ret=qc_retirement_open(&isp->receipt_source,&fc->receipt);
 if(ret){kfree(fc);return ERR_PTR(ret);}
 kref_init(&fc->references);fc->isp=isp;kref_get(&isp->references);return fc;
}
static void ak_isp_quarantine(struct ak_isp *isp,int ret)
{
 if(!isp->quarantined){kref_get(&isp->references);isp->quarantined=true;}
 isp->quarantine_result=ret?ret:-EIO;
 /* Registry continues to own the one physical source slot; no rebind pileup. */
}
struct ak_isp_arm_copy {struct qc_capture_receipt q;};
static int ak_isp_arm_from(void *ctx,struct qc_capture_receipt *q)
{*q=((struct ak_isp_arm_copy *)ctx)->q;return 0;}
static int ak_isp_arm_to(void *ctx,const struct qc_capture_receipt *q)
{((struct ak_isp_arm_copy *)ctx)->q=*q;return 0;}
static int ak_isp_claim(struct ak_isp *isp,struct ak_isp_file *fc)
{
 struct ak_isp_arm_copy a={0};struct qc_retirement_uaccess u={ak_isp_arm_from,ak_isp_arm_to,&a};int ret;
 if(isp->disconnected)return -ENODEV;
 if(isp->quarantined)return -EIO;
 if(isp->session && isp->session != fc)return -EBUSY;
 if(fc->receipt.state==QC_RECEIPT_NONE){
  a.q.version=QC_CAPTURE_RECEIPT_VERSION;a.q.source_instance=isp->receipt_source.id;a.q.file_instance=fc->receipt.id;
  ret=qc_retirement_ioctl(&fc->receipt,QC_ISP_RECEIPT_ARM,&u);if(ret)return ret;
 }
 if(fc->receipt.state!=QC_RECEIPT_ARMED)return -EBUSY;
 if(!isp->session){kref_get(&fc->references);isp->session=fc;}
 fc->receipt.state=QC_RECEIPT_STARTING;isp->dma_retired=false;return 0;
}
static void ak_isp_finish_begin(struct ak_isp *isp,struct ak_isp_file *fc,int ret)
{
 fc->receipt.state=ret?QC_RECEIPT_UNCERTAIN:QC_RECEIPT_ACTIVE;
 fc->receipt.stop_result=ret?-EINPROGRESS:0;
 if(ret)ak_isp_quarantine(isp,ret);
}
static int ak_isp_retirement_enter(struct ak_isp *isp,struct qc_retirement_fence *fence)
{
 struct ak_isp_file *fc=isp->session;
 if(!fc)return -ESTALE;
 if(fc->receipt.state==QC_RECEIPT_STARTING)fc->receipt.state=QC_RECEIPT_UNCERTAIN;
 return qc_retirement_stop_enter(&fc->receipt,isp->stop_reason,fence);
}
static int ak_isp_retirement_result(struct ak_isp *isp,const struct qc_retirement_fence *fence,int ret)
{
 /* Called only after the full existing stop, with identity captured BEFORE
  * its MMIO/reset. All entries hold source lock; IRQ never changes receipt. */
 if(ret>0)ret=-EIO; /* preserve the receipt helper's error normalization */
 if(isp->session){
  int proof=qc_retirement_stop_finish(&isp->session->receipt,fence,ret,ktime_get_ns());
   if(!ret && proof){ret=proof;qc_stage_retirement(&isp->telemetry,ret);}
   if(ret){
    isp->session->receipt.state=QC_RECEIPT_UNCERTAIN;
    isp->session->receipt.stop_result=ret;
    isp->session->receipt.stopped_ns=0;
   }
 }else if(!ret)ret=-ESTALE;
 isp->dma_retired=!ret;
 if(ret)ak_isp_quarantine(isp,ret);
 return ret;
}
static void ak_isp_consume_retired(struct ak_isp *isp,struct ak_isp_file *fc)
{
 if(isp->session!=fc || fc->receipt.state!=QC_RECEIPT_RETIRED || fc->receipt.stop_result)return;
 isp->receipt_source.hold=NULL;fc->receipt.generation=0;fc->receipt.state=QC_RECEIPT_NONE;
 fc->receipt.reason=QC_STOP_NONE;fc->receipt.stop_result=0;fc->receipt.stopped_ns=0;
 isp->session=NULL;ak_isp_file_put(fc);
}
static int ak_isp_open(struct inode *inode,struct file *file)
{
 struct miscdevice *misc=file->private_data;
 struct ak_isp *isp=container_of(misc,struct ak_isp,misc);
 struct ak_isp_file *fc;int ret=0;
 (void)inode;
 /* Linux6.18 misc_open holds misc_mtx through this callback; registration ref
  * is retained until matching misc_deregister returns. First get is safe. */
 kref_get(&isp->references);mutex_lock(&isp->lock);
 if(!isp->ready || isp->disconnected || isp->quarantined){ret=isp->quarantined?-EIO:-ENODEV;file->private_data=NULL;goto out;}
 fc=ak_isp_file_alloc(isp);if(IS_ERR(fc)){ret=PTR_ERR(fc);file->private_data=NULL;goto out;}
 file->private_data=fc;
out:
 mutex_unlock(&isp->lock);ak_isp_bundle_put(isp);return ret;
}
struct ak_isp_user_copy {unsigned long arg;};
static int ak_isp_receipt_from(void *ctx,struct qc_capture_receipt *q)
{return copy_from_user(q,(void __user *)((struct ak_isp_user_copy *)ctx)->arg,sizeof(*q))?1:0;}
static int ak_isp_receipt_to(void *ctx,const struct qc_capture_receipt *q)
{return copy_to_user((void __user *)((struct ak_isp_user_copy *)ctx)->arg,q,sizeof(*q))?1:0;}
static int ak_isp_receipt_ioctl(struct ak_isp_file *fc,unsigned int cmd,unsigned long arg)
{
 struct ak_isp *isp=fc->isp;struct ak_isp_user_copy a={arg};
 struct qc_retirement_uaccess u={ak_isp_receipt_from,ak_isp_receipt_to,&a};int ret;
 if(cmd==QC_ISP_RECEIPT_ARM && (isp->disconnected || isp->quarantined))return isp->disconnected?-ENODEV:-EIO;
 ret=qc_retirement_ioctl(&fc->receipt,cmd,&u);
 if(!ret && cmd==QC_ISP_RECEIPT_ARM){fc->receipt_visible=true;kref_get(&fc->references);isp->session=fc;}
 if(!ret && cmd==QC_ISP_RECEIPT_ACK){isp->session=NULL;ak_isp_file_put(fc);}
 return ret;
}
/* SET policy is about retained session ownership, not just running DMA. The
 * same owner may edit its ARMED settings while idle; a RETIRED receipt must be
 * ACKed before anyone edits. GET never takes/consumes a session or stage. */
static bool ak_isp_control_busy(struct ak_isp *isp,struct ak_isp_file *fc)
{
 return isp->stream_owner || (isp->session &&
        (isp->session!=fc || fc->receipt.state!=QC_RECEIPT_ARMED));
}

static bool ak_isp_owned_dma(dma_addr_t address, u32 bytes)
{
	return address >= 0x81800000ULL && address + bytes <= 0x84000000ULL;
}
static int ak_isp_reset_impl(struct ak_isp *isp)
{
	int ret = regmap_update_bits(isp->sys, 0x20, BIT(19), BIT(19));

	if (ret)
		return ret;
	usleep_range(125000, 130000);
	return regmap_update_bits(isp->sys, 0x20, BIT(19), 0);
}
static int ak_isp_reset(struct ak_isp *isp)
{
	u64 start = ktime_get_ns();
	int ret = ak_isp_reset_impl(isp);
	qc_stage_sample(&isp->telemetry, QC_PHASE_RESET, start, ktime_get_ns(), 0, ret);
	return ret;
}
static int ak_isp_reset_retirement(struct ak_isp *isp)
{
	u64 start = ktime_get_ns();
	int ret = ak_isp_reset_impl(isp);
	qc_stage_sample(&isp->telemetry, QC_PHASE_RESET_RETIRE, start, ktime_get_ns(), 0, ret);
	qc_stage_retirement(&isp->telemetry, ret);
	return ret;
}
static void *qc_isp_alloc(struct ak_isp *isp, size_t bytes, dma_addr_t *dma, gfp_t gfp)
{
	u64 start = ktime_get_ns();
	void *p = dma_alloc_coherent(isp->dev, bytes, dma, gfp);
	qc_stage_sample(&isp->telemetry, QC_PHASE_ALLOC, start, ktime_get_ns(), bytes, p ? 0 : -ENOMEM);
	qc_stage_alloc(&isp->telemetry, bytes, !!p);
	return p;
}
static void qc_isp_free(struct ak_isp *isp, size_t bytes, void *p, dma_addr_t dma)
{
	u64 start = ktime_get_ns();
	dma_free_coherent(isp->dev, bytes, p, dma);
	qc_stage_sample(&isp->telemetry, QC_PHASE_FREE, start, ktime_get_ns(), bytes, 0);
	qc_stage_free(&isp->telemetry, bytes);
}
static void ak_isp_set_planes(struct ak_isp *isp, u32 offset, dma_addr_t main, dma_addr_t sub)
{
	isp->table[offset / 4] = main;
	isp->table[(offset + 4) / 4] = main + MAIN_PIXELS;
	isp->table[(offset + 8) / 4] = main + MAIN_PIXELS * 5 / 4;
	isp->table[(offset + 12) / 4] = sub;
	isp->table[(offset + 16) / 4] = sub + SUB_PIXELS;
	isp->table[(offset + 20) / 4] = sub + SUB_PIXELS * 5 / 4;
}
static void ak_isp_prepare_table_impl(struct ak_isp *isp)
{
	u32 slot, plane;
	dma_addr_t main = isp->frame_dma, sub = main + MAIN_BYTES;
	const u32 ref_offsets[] = {0, 0x258000, 0x258000, 0x96000, 0x2ee000, 0x96000};

	memset(isp->table, 0, TABLE_ALLOC);
	memcpy(isp->table, qc07f_isp_reference, sizeof(qc07f_isp_reference));
	/* REA reg10 base cdc +58/5c/60. Only verified WB fields change, at
	 * idle capture-table preparation; source/stat/DMA ownership unchanged.
	 */
	memcpy(&isp->table[0xd34 / 4], isp->wb_desired_words, sizeof(isp->wb_desired_words));
	memcpy(isp->wb_prepared_words, isp->wb_desired_words, sizeof(isp->wb_prepared_words));
	isp->wb_table_preset = isp->wb_preset;
	isp->wb_last_prepared_ns = ktime_get_ns();
	isp->wb_table_prepared = true;
	/* Exactretail effectsetter: reg2baseba8+a0=c48, UVgainbits16..24.
	 * RetainedSC1346conf effectpresets useUVgain64/0. Do notconfuse reg10
	 * bit24(dark-marginenable) withmonochrome; preserveYgain/offset/others.
	 */
	if (isp->colour_override)
		isp->table[0xc48 / 4] = (isp->table[0xc48 / 4] & ~GENMASK(24, 16)) | (isp->uv_gain << 16);
	/* Buffer0 belongs to register10; buffers1..3 are interleaved in reg0. */
	ak_isp_set_planes(isp, 0xd40, main, sub);
	for (slot = 0; slot < 3; slot++) {
		const dma_addr_t addresses[] = {main, main + MAIN_PIXELS,
			main + MAIN_PIXELS * 5 / 4, sub, sub + SUB_PIXELS,
			sub + SUB_PIXELS * 5 / 4};
		for (plane = 0; plane < 6; plane++)
			isp->table[(0x7c + slot * 4 + plane * 12) / 4] = addresses[plane];
		isp->table[(0xc4 + slot * 4) / 4] = isp->table_dma + 0x1d60;
	}
	isp->table[0xd58 / 4] = isp->table_dma + 0x1d60;
	for (slot = 0; slot < ARRAY_SIZE(ref_offsets); slot++)
		isp->table[(0xd4 + slot * 4) / 4] = (isp->reference_dma + ref_offsets[slot]) & 0x3fffffff;
	/* OSD is disabled, but even its inactive address fields own zeroed RAM. */
	isp->table[0xcb0 / 4] = isp->reference_dma & 0x3fffffff;
	isp->table[0xcbc / 4] = isp->reference_dma & 0x3fffffff;
	isp->table[0xd5c / 4] &= ~(BIT(22) | BIT(23));
	/* REA _set_3d_nr_attr: reg2+20 bit16 and reg10+80 bits9..14.
	 * First-frame diagnostics have no valid temporal history. Disable its
	 * processing/history consumers, preserving all other tuning bits.
	 */
	isp->table[0x1dc / 4] &= ~BIT(16);
	isp->table[0xd5c / 4] &= ~GENMASK(14, 9);
	isp->table[0xcd8 / 4] = 0xffff;
	memset(isp->frame, 0xa5, FRAME_ALLOC);
	memset(isp->reference, 0, REF_ALLOC);
	dma_wmb();
}
static void ak_isp_prepare_table(struct ak_isp *isp)
{
	u64 start = ktime_get_ns();
	ak_isp_prepare_table_impl(isp);
	qc_stage_sample(&isp->telemetry, QC_PHASE_PREPARE, start, ktime_get_ns(), 0, 0);
}
static int ak_isp_capture(struct ak_isp *isp)
{
	u32 control;
 int ret, reset_ret, fence_ret;
 struct qc_retirement_fence fence={0};

	if (isp->disconnected || isp->quarantined)
		return isp->disconnected ? -ENODEV : -EIO;
	if (isp->stream_owner)
		return -EBUSY;
	isp->readable = false;
	isp->changed_main = isp->changed_sub = isp->guard_errors = 0;
	ret = regmap_update_bits(isp->sys, 0x1c, BIT(19), 0);
	if (ret)
		return ret;
	/* Retail ak_camera_add_device clears PERIPLL1 bit25 after enabling the
	 * camera clock. This is a peripheral clock control bit, not PLL tuning.
	 * Record inherited/final values rather than copying the whole register.
	 */
	ret = regmap_read(isp->sys, 0x14, &isp->pll14_before);
	if (ret)
		return ret;
	ret = regmap_update_bits(isp->sys, 0x14, BIT(25), 0);
	if (ret)
		return ret;
	ret = regmap_read(isp->sys, 0x14, &isp->pll14_after);
	if (ret)
		return ret;
	ret = ak_isp_reset(isp);
	if (ret)
		return ret;
	ak_isp_prepare_table(isp);
	/* Measured retail direct input/timing registers; mode3 single RGB.
	 * Clear all buffer/start state before submitting owned table pointers.
	 */
	control = 0x0a71a011 & ~BUFFERS_MASK & ~BIT(20);
	control &= 0xfff7dcf3;
	writel(control, isp->base + 0x00);
	writel(0x00b40500, isp->base + 0x04);
	writel(0x1edc01be, isp->base + 0x08);
	writel(0x1605dc05, isp->base + 0x0c);
	writel(readl(isp->base + 0x10) | 0xffff, isp->base + 0x10);
	writel(1, isp->base + 0x14);
	writel(isp->table_dma, isp->base + 0x18);
	/* Disable SDPC use; its zeroed table is nevertheless an owned pointer. */
	writel((isp->table_dma + 0xd60) & 0x3fffffff, isp->base + 0x20);
	writel(1, isp->base + 0x1c);
	writel(control | 0x208000 | BIT(20), isp->base + 0x00);
	ret = readl_poll_timeout(isp->base + 0x10, isp->status,
				 isp->status & BIT(0), 100, 2000000);
	isp->observed_ctrl = readl(isp->base);
	isp->observed_table = readl(isp->base + 0x18);
	isp->observed_order = readl(isp->base + 0x1c);
	isp->stop_reason=ret?QC_STOP_ERROR:QC_STOP_END;
	fence_ret=ak_isp_retirement_enter(isp,&fence);
	writel(readl(isp->base) & ~(BIT(20) | BUFFERS_MASK), isp->base);
	/* Force controller quiescence even on timeout; never free DMA memory
	 * while a hardware transaction might still own it.
	 */
	reset_ret = ak_isp_reset_retirement(isp);
	if(fence_ret && !reset_ret){reset_ret=fence_ret;qc_stage_retirement(&isp->telemetry,reset_ret);}
	reset_ret = ak_isp_retirement_result(isp, &fence, reset_ret);
	if (reset_ret)
		return reset_ret;
	dma_rmb();
	isp->changed_main = camera_guard_errors_ram(isp->frame, MAIN_BYTES);
	isp->changed_sub = camera_guard_errors_ram(isp->frame + MAIN_BYTES, SUB_BYTES);
	isp->guard_errors = camera_guard_errors_ram(isp->frame + FRAME_BYTES, FRAME_ALLOC - FRAME_BYTES);
	if (isp->guard_errors)
		return -EOVERFLOW;
	if (!ret && !isp->changed_main)
		return -ENODATA;
	if (!ret)
		isp->frame_crc = crc_ram01_static4(isp->frame, FRAME_BYTES);
	isp->readable = !ret;
	return ret;
}
static ssize_t capture_store(struct device *dev, struct device_attribute *attr,
			     const char *buf, size_t count)
{
	struct ak_isp *isp = dev_get_drvdata(dev);
	int ret;

	if (!sysfs_streq(buf, "1"))
		return -EINVAL;
 kref_get(&isp->references);
	mutex_lock(&isp->lock);
 if(!isp->ready || isp->disconnected || isp->quarantined || isp->session || isp->stream_owner){
  ret=(!isp->ready || isp->disconnected)?-ENODEV:isp->quarantined?-EIO:-EBUSY;goto capture_out;
 }
 struct ak_isp_file *fc=ak_isp_file_alloc(isp);
 if(IS_ERR(fc)){ret=PTR_ERR(fc);goto capture_out;}
 ret=ak_isp_claim(isp,fc);
 if(ret){fc->receipt.open=0;isp->receipt_source.open_files--;ak_isp_file_put(fc);goto capture_out;}
 fc->receipt.state=QC_RECEIPT_ACTIVE;isp->stop_reason=QC_STOP_END;
	qc_stage_begin(&isp->telemetry, QC_STAGE_OTHER, ktime_get_ns());
	ret = ak_isp_capture(isp);
	isp->last_result = ret;
	qc_stage_end(&isp->telemetry, ret, 0, ktime_get_ns());
 if(!isp->dma_retired){fc->receipt.state=QC_RECEIPT_UNCERTAIN;ak_isp_quarantine(isp,ret?ret:-EIO);}
 fc->receipt.open=0;isp->receipt_source.open_files--;
 if(isp->dma_retired)ak_isp_consume_retired(isp,fc);
 ak_isp_file_put(fc);
capture_out:
	mutex_unlock(&isp->lock);
 ak_isp_bundle_put(isp);
	dev_info(dev, "bounded capture result=%d status=%08x main_changed=%u sub_changed=%u guard_errors=%u\n",
		 ret, isp->status, isp->changed_main, isp->changed_sub, isp->guard_errors);
	return ret ? ret : count;
}
static DEVICE_ATTR_WO(capture);
static ssize_t state_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct ak_isp *isp = dev_get_drvdata(dev);
	ssize_t n;

 kref_get(&isp->references);
	mutex_lock(&isp->lock);
 if(!isp->ready || isp->disconnected || isp->quarantined){int e=isp->quarantined?-EIO:-ENODEV;mutex_unlock(&isp->lock);ak_isp_bundle_put(isp);return e;}
	n = sysfs_emit(buf, "result=%d readable=%u status=%08x main_changed=%u sub_changed=%u guard_errors=%u table_dma=%pad frame_dma=%pad ref_dma=%pad main_bytes=%u sub_bytes=%u ctrl=%08x table=%08x order=%08x pll14_before=%08x pll14_after=%08x frame_crc32_bzip2=%08x temporal_nr_disabled=1\n",
		isp->last_result, isp->readable, isp->status, isp->changed_main,
		isp->changed_sub, isp->guard_errors, &isp->table_dma, &isp->frame_dma,
		&isp->reference_dma, MAIN_BYTES, SUB_BYTES, isp->observed_ctrl,
		isp->observed_table, isp->observed_order, isp->pll14_before,
		isp->pll14_after, isp->frame_crc);
	n += sysfs_emit_at(buf, n, "stream_pre_gate_wait_timeouts=%u\n", isp->stream_pre_gate_wait_timeouts);
	n += sysfs_emit_at(buf, n, "irq_mode=%u irq=%d irq_frames=%u irq_unexpected=%u\n", isp->irq_mode, isp->irq, isp->irq_frames, isp->irq_unexpected);
	n += sysfs_emit_at(buf, n, "irq_no_done=%u irq_late_done=%u irq_last_extra_status=%08x\n", isp->irq_no_done, isp->irq_late_done, isp->irq_last_extra_status);
	n += sysfs_emit_at(buf, n, "queue_mode=%u queue_started=%u queue_held=%u queue_events=%u queue_dropped=%u queue_sequence=%u\n", isp->queue_mode, isp->queue_started, isp->queue_held, isp->queue_events, isp->queue_dropped, isp->queue_sequence);
	n += sysfs_emit_at(buf, n, "wb_desired_preset=%u wb_table_prepared=%u wb_last_table_preset=%u wb_table_words=%08x/%08x/%08x; table metadata not live MMIO readback\n", isp->wb_preset, isp->wb_table_prepared, isp->wb_table_preset, isp->table[0xd34 / 4], isp->table[0xd38 / 4], isp->table[0xd3c / 4]);
	n += sysfs_emit_at(buf, n, "wb_desired_words=%08x/%08x/%08x wb_desired_changed_ns=%llu wb_last_prepared_ns=%llu wb_camera_id=2 wb_owner_active=%u; monotonic timestamps, raw offsets only\n", isp->wb_desired_words[0], isp->wb_desired_words[1], isp->wb_desired_words[2], (unsigned long long)isp->wb_desired_changed_ns, (unsigned long long)isp->wb_last_prepared_ns, !!isp->stream_owner);
	mutex_unlock(&isp->lock);
 ak_isp_bundle_put(isp);
	return n;
}
static DEVICE_ATTR_RO(state);
static struct attribute *ak_isp_attrs[] = { &dev_attr_capture.attr, &dev_attr_state.attr, NULL };
static const struct attribute_group ak_isp_attr_group = { .attrs = ak_isp_attrs };
/* Parenthwirq1wasobservedread-only:peripheral14=1/frame-done0=1 correlates
 * rootpending1 whileparentmask1=0. NoIRQrequest/unmaskatprobe. Explicitmode
 * lazilyregistersNO_AUTOEN, preparesownedcapturestate, enablesonlythatline.
 */
static irqreturn_t ak_isp_frame_irq(int irq, void *data)
{
	struct ak_isp *isp = data;
	u32 status = readl(isp->base + 0x10);
	if (READ_ONCE(isp->queue_mode)) {
		u32 id = (status >> 22) & 3;
		bool wake = false;
		raw_spin_lock(&isp->queue_lock);
		if (time_after_eq(jiffies, isp->stream_deadline))
			isp->queue_expired = true;
		if (isp->queue_expired || !(status & BIT(0)) || (isp->queue_held && id == isp->queue_slot)) {
			isp->queue_bad = true;
			if (!isp->queue_expired)
				isp->irq_unexpected++;
			writel(0, isp->base + 0x14);
			writel(readl(isp->base) & ~(BUFFERS_MASK | BIT(20)), isp->base);
			wake = true;
		} else {
			isp->queue_events++;
			isp->irq_frames++;
			if (isp->queue_request) {
				u32 pair = BIT(15 + id) | BIT(21 + id);
				writel(readl(isp->base) & ~pair, isp->base);
				readl(isp->base);
				isp->queue_slot = id;
				isp->queue_status = status;
				isp->queue_request = false;
				isp->queue_held = true;
				isp->queue_token = ++isp->queue_token_counter;
				wake = true;
			} else {
				isp->queue_dropped++; /* intentional producer frames, not deliveries */
			}
		}
		writel(status | 0xffff, isp->base + 0x10);
		readl(isp->base + 0x10);
		raw_spin_unlock(&isp->queue_lock);
		if (wake)
			complete(&isp->queue_done);
		return IRQ_HANDLED;
	}

	if (!(status & BIT(0))) {
		isp->irq_unexpected++;
		isp->irq_no_done++;
		isp->irq_last_extra_status = status;
		writel(0, isp->base + 0x14);
		writel(status | 0xffff, isp->base + 0x10);
		readl(isp->base + 0x10);
		if (READ_ONCE(isp->ring_test_active)) {
			WRITE_ONCE(isp->ring_bad_order, true);
			complete(&isp->ring_first_done);
			complete(&isp->irq_done);
		}
		return IRQ_HANDLED;
	}
	/* Retirebuffergrantsbeforepublishingcompletion. CRC/copy andPPstop/
	 * actualpost-gateidle remaininprocesscontext, notthehardhandler.
	 */
	if (READ_ONCE(isp->ring_test_active)) {
		u32 n = isp->ring_completed;
		if (n >= ARRAY_SIZE(isp->ring_status)) {
			WRITE_ONCE(isp->ring_bad_order, true);
			writel(0, isp->base + 0x14);
			writel(readl(isp->base) & ~(BUFFERS_MASK | BIT(20)), isp->base);
			WRITE_ONCE(isp->ring_test_active, false);
			writel(status | 0xffff, isp->base + 0x10);
			readl(isp->base + 0x10);
			complete(&isp->ring_first_done);
			complete(&isp->irq_done);
			return IRQ_HANDLED;
		}
		if (n < 16) {
			isp->ring_status[n] = status;
			isp->ring_control[n] = readl(isp->base);
			isp->ring_elapsed[n] = jiffies_to_msecs(jiffies - isp->ring_started);
			isp->ring_completed = n + 1;
			isp->irq_frames++;
		}
		if (isp->ring_hold_selected) {
			u32 id = (status >> 22) & 3;
			u32 pair = BIT(15 + isp->ring_held_slot) | BIT(21 + isp->ring_held_slot);
			if (!isp->ring_held_ready) {
				/*63shortbatches map initial all-enabled IDs to filled slots.
				 * Candidate now tests retirement of each selected slot.
				 */
				if (id != n || n > isp->ring_held_slot)
					WRITE_ONCE(isp->ring_bad_order, true);
				else if (id == isp->ring_held_slot) {
					writel(readl(isp->base) & ~pair, isp->base);
					readl(isp->base);
					WRITE_ONCE(isp->ring_held_ready, true);
				}
			} else if (id == isp->ring_held_slot || !(isp->ring_control[n] & BIT(20)) ||
				   (isp->ring_control[n] & pair)) {
				WRITE_ONCE(isp->ring_bad_order, true);
			}
		}
		if (isp->ring_completed >= isp->ring_limit || isp->ring_bad_order) {
			writel(0, isp->base + 0x14);
			writel(readl(isp->base) & ~(BUFFERS_MASK | BIT(20)), isp->base);
			WRITE_ONCE(isp->ring_test_active, false);
			complete(&isp->irq_done);
		}
		writel(status | 0xffff, isp->base + 0x10);
		readl(isp->base + 0x10);
		if (isp->ring_hold_selected && (isp->ring_held_ready || isp->ring_bad_order))
			complete(&isp->ring_first_done);
		return IRQ_HANDLED;
	}
	writel(0, isp->base + 0x14); /* gatedmode: no secondcompletion beforeNEXT */
	writel(readl(isp->base) & ~BUFFERS_MASK, isp->base);
	writel(status | 0xffff, isp->base + 0x10);
	readl(isp->base + 0x10); /* retireperipheralack beforeparentunmask */
	if (READ_ONCE(isp->irq_inflight)) {
		WRITE_ONCE(isp->irq_status, status);
		WRITE_ONCE(isp->irq_inflight, false);
		isp->irq_frames++;
		complete(&isp->irq_done);
	} else {
		isp->irq_unexpected++;
		isp->irq_late_done++;
		isp->irq_last_extra_status = status;
	}
	return IRQ_HANDLED;
}
static int ak_isp_prepare_irq(struct ak_isp *isp)
{
	struct device_node *np;
	struct irq_domain *domain;
	int ret;

	if (isp->irq_registered)
		return 0;
	np = of_find_compatible_node(NULL, NULL, "anyka,ak3918ev200-intc");
	if (!np)
		return -ENODEV;
	domain = irq_find_host(np);
	of_node_put(np);
	if (!domain)
		return -ENODEV;
	isp->irq = irq_create_mapping(domain, 1);
	if (!isp->irq)
		return -ENODEV;
	/* Target61readback showed lazy disable left parentmask1=1 despite
	 * peripheral14=0/pending1=0. Require actual chip masking at disable.
	 */
	irq_set_status_flags(isp->irq, IRQ_DISABLE_UNLAZY);
	ret = request_irq(isp->irq, ak_isp_frame_irq,
			       IRQF_NO_AUTOEN, "qc07f-isp-frame", isp);
	if (!ret)
		isp->irq_registered = true;
	return ret;
}
/* Exact retained retail apply_mode4 (continuousRGB), clear_irq_status,
 * buffer0disable and vi_stop busy14 protocol. Deliberately gated oneframe:
 * never read/copy while a transaction can still write the owned frame.
 * No IRQ ownership change, no temporalNR, bounded10-second/128-frame sessions.
 */
static int ak_isp_stream_stop(struct ak_isp *isp)
{
	int ret,fence_ret;
 struct qc_retirement_fence fence={0};
 fence_ret=ak_isp_retirement_enter(isp,&fence);

	if (isp->irq_mode) {
		writel(0, isp->base + 0x14);
		disable_irq(isp->irq); /* synchronizehandlerbeforechanging/freeingstate */
		WRITE_ONCE(isp->irq_inflight, false);
		isp->irq_mode = false;
	}
	WRITE_ONCE(isp->ring_test_active, false);
	WRITE_ONCE(isp->queue_mode, false);
	isp->queue_started = isp->queue_request = isp->queue_held = false;
	writel(readl(isp->base) & ~(BIT(20) | BUFFERS_MASK), isp->base);
	ret = ak_isp_reset_retirement(isp);
	if (!ret) {
		isp->stream_owner = NULL;
		isp->readable = false;
	}
 /* A failed entry does NOT skip any original mask/IRQ/reset operation. It
  * cannot mint a receipt, even if the physical reset itself returned zero. */
 if(fence_ret && !ret){ret=fence_ret;qc_stage_retirement(&isp->telemetry,ret);}
 ret = ak_isp_retirement_result(isp, &fence, ret); /* entire mask/IRQ/reset stop returned */
	return ret;
}
static int ak_isp_stream_begin_impl(struct ak_isp *isp, struct ak_isp_file *file)
{
	u32 control;
	int ret;

	if (isp->disconnected || isp->quarantined)
		return isp->disconnected ? -ENODEV : -EIO;
	if (isp->stream_owner)
		return -EBUSY;
	ret = regmap_update_bits(isp->sys, 0x1c, BIT(19), 0);
	if (ret)
		return ret;
	ret = regmap_update_bits(isp->sys, 0x14, BIT(25), 0);
	if (ret)
		return ret;
	ret = ak_isp_reset(isp);
	if (ret)
		return ret;
	ak_isp_prepare_table(isp);
	control = (0x0a71a011 & ~BUFFERS_MASK & ~BIT(20)) & 0xfff7fcf3;
	control |= 0x2000; /* retained apply_mode4, not singleRGBmode3 */
	writel(control, isp->base);
	writel(0x00b40500, isp->base + 4);
	writel(0x1edc01be, isp->base + 8);
	writel(0x1605dc05, isp->base + 12);
	writel(readl(isp->base + 0x10) | 0xffff, isp->base + 0x10);
	writel(0, isp->base + 0x14); /* noIRQsignalbeforeownedNEXTgrant */
	writel(isp->table_dma, isp->base + 0x18);
	writel((isp->table_dma + 0xd60) & 0x3fffffff, isp->base + 0x20);
	writel(1, isp->base + 0x1c);
	/* Remain idle untilNEXT grantsbuffer0 ANDcapturestart20 together.
	 * Retail vi_start enablesbuffers beforecapturestart. Starting withnone
	 * granted causedintermittentfresh-session busy14/timeouts in55. Sensor
	 * maystreamwhilethisengineidle; do notcreate an unownedtransaction.
	 */
	writel(control, isp->base);
	isp->stream_owner = file;
	isp->stream_sequence = 0;
	isp->stream_deadline = jiffies + 10 * HZ;
	isp->readable = false;
	return 0;
}
static int ak_isp_stream_begin(struct ak_isp *isp, struct ak_isp_file *file)
{
 int ret=ak_isp_claim(isp,file);
 if(ret)return ret;
 ret=ak_isp_stream_begin_impl(isp,file);
 ak_isp_finish_begin(isp,file,ret);return ret;
}
/* Local edge timestamps are aggregated only after the original gate/drain or
 * stop/reset attempt. Failed retirement is still failure, never safe-DMA proof.
 * Caller holds isp->lock; flushing clears reached so later guard failures do
 * not count the same wait/program intervals twice.
 */
static void qc_isp_next_flush(struct ak_isp *isp, const u64 start[6],
		const u64 end[6], const int result[6], u32 *reached)
{
	static const u32 phase[6] = { QC_PHASE_PRE_IDLE, QC_PHASE_PROGRAM, QC_PHASE_FRAME_WAIT,
		QC_PHASE_PP_COMMIT, QC_PHASE_PRE_GATE_DRAIN, QC_PHASE_POST_GATE_DRAIN };
	u32 i;
	for (i = 0; i < 6; i++)
		if (*reached & BIT(i))
			qc_stage_sample(&isp->telemetry, phase[i], start[i], end[i], 0, result[i]);
	*reached = 0;
}
static int ak_isp_stream_next(struct ak_isp *isp, struct qc_isp_next *next)
{
	u32 status, busy;
	int ret;
	u64 stamp;
	u64 edge_start[6] = {0}, edge_end[6] = {0};
	int edge_result[6] = {0};
	u32 reached = 0;

	if (next->bytes != FRAME_BYTES || !next->user_ptr)
		return -EINVAL;
	if (time_after_eq(jiffies, isp->stream_deadline) || isp->stream_sequence >= 128) {
		isp->stop_reason=QC_STOP_EXPIRY;
		ak_isp_stream_stop(isp);
		return -ETIMEDOUT;
	}
	edge_start[0] = ktime_get_ns();
	ret = readl_poll_timeout(isp->base + 0x10, busy, !(busy & BIT(14)), 100, 200000);
	edge_end[0] = ktime_get_ns();
	edge_result[0] = ret;
	reached |= BIT(0);
	if (ret)
		goto stop;
	/* No buffer owned by DMA here; guards survive all frames in the session. */
	edge_start[1] = ktime_get_ns();
	isp->table[0xcd8 / 4] |= 0xffff;
	dma_wmb();
	writel(1, isp->base + 0x1c);
	writel(readl(isp->base + 0x10) | 0xffff, isp->base + 0x10);
	if (isp->irq_mode) {
		reinit_completion(&isp->irq_done);
		WRITE_ONCE(isp->irq_inflight, true);
	}
	writel(1, isp->base + 0x14);
	writel(readl(isp->base) | BIT(20) | 0x208000, isp->base);
	edge_end[1] = ktime_get_ns();
	reached |= BIT(1);
	edge_start[2] = ktime_get_ns();
	if (isp->irq_mode) {
		long done = wait_for_completion_interruptible_timeout(&isp->irq_done, 2 * HZ);
		ret = done > 0 ? 0 : done == 0 ? -ETIMEDOUT : (int)done;
		status = READ_ONCE(isp->irq_status);
		writel(0, isp->base + 0x14);
	} else {
		ret = readl_poll_timeout(isp->base + 0x10, status, status & BIT(0), 100, 2000000);
	}
	edge_end[2] = ktime_get_ns();
	edge_result[2] = ret;
	reached |= BIT(2);
	writel(readl(isp->base) & ~BUFFERS_MASK, isp->base);
	if (ret)
		goto stop;
	/* Exact retail vi_stop stops postprocessing viareg3+88/tablecd8 and
	 * commits that table. Buffergrants alone did not provequiescence:
	 * 54's pre-copyCRC differed from retrievedpixels. Drain the stop and
	 * gate actualcapturestart before CRC/usercopy; no per-framereset.
	 */
	edge_start[3] = ktime_get_ns();
	isp->table[0xcd8 / 4] &= 0xffff0000;
	dma_wmb();
	writel(1, isp->base + 0x1c);
	/* Finish any in-flight frame before exposing/copying pixels. No per-frame
	 * reset or software bit-at-a-timeCRC; identicalBZIP2 CRC via kernel table.
	 */
	edge_end[3] = ktime_get_ns();
	reached |= BIT(3);
	edge_start[4] = ktime_get_ns();
	ret = readl_poll_timeout(isp->base + 0x10, busy, !(busy & BIT(14)), 100, 200000);
	edge_end[4] = ktime_get_ns();
	edge_result[4] = ret;
	reached |= BIT(4);
	writel(readl(isp->base) & ~BIT(20), isp->base);
	if (ret)
		isp->stream_pre_gate_wait_timeouts++;
	readl(isp->base); /* retire controllerwrite before DMAbarrier */
	/* Retail vi_stop gatesstart20 even ifitsinitialbusy14waitexpires.
	 * Read-only56snapshots provedframe-done butbusy14stayedhighwhilestart
	 * remainedenabled. DoNOThandoutpixels aftermerelyignoringthatwait:
	 * requireboundedidle AFTERtheactualcapturegate aswell, thenallCRCs.
	 */
	edge_start[5] = ktime_get_ns();
	ret = readl_poll_timeout(isp->base + 0x10, busy, !(busy & BIT(14)), 100, 200000);
	edge_end[5] = ktime_get_ns();
	edge_result[5] = ret;
	reached |= BIT(5);
	if (ret)
		goto stop;
	dma_rmb();
	qc_isp_next_flush(isp, edge_start, edge_end, edge_result, &reached);
	isp->guard_errors = 0;
	stamp = ktime_get_ns();
	isp->guard_errors = camera_guard_errors_ram(isp->frame + FRAME_BYTES, FRAME_ALLOC - FRAME_BYTES);
	qc_stage_sample(&isp->telemetry, QC_PHASE_GUARDS, stamp, ktime_get_ns(), FRAME_ALLOC - FRAME_BYTES, isp->guard_errors ? -EOVERFLOW : 0);
	if (isp->guard_errors) {
		ret = -EOVERFLOW;
		goto stop;
	}
	stamp = ktime_get_ns();
	isp->frame_crc = crc_ram01_static4(isp->frame, FRAME_BYTES);
	qc_stage_sample(&isp->telemetry, QC_PHASE_CRC_PRE, stamp, ktime_get_ns(), FRAME_BYTES, 0);
	next->sequence = ++isp->stream_sequence;
	next->status = isp->status = status;
	next->guard_errors = isp->guard_errors;
	next->crc32_bzip2 = isp->frame_crc;
	stamp = ktime_get_ns();
	if (copy_to_user((void __user *)(unsigned long)next->user_ptr, isp->frame, FRAME_BYTES)) {
		qc_stage_sample(&isp->telemetry, QC_PHASE_PIXEL_COPY, stamp, ktime_get_ns(), FRAME_BYTES, -EFAULT);
		return -EFAULT;
	}
	qc_stage_sample(&isp->telemetry, QC_PHASE_PIXEL_COPY, stamp, ktime_get_ns(), FRAME_BYTES, 0);
	stamp = ktime_get_ns();
	if (crc_ram01_static4(isp->frame, FRAME_BYTES) != isp->frame_crc) {
		qc_stage_sample(&isp->telemetry, QC_PHASE_CRC_POST, stamp, ktime_get_ns(), FRAME_BYTES, -EIO);
		ret = -EIO;
		goto stop;
	}
	qc_stage_sample(&isp->telemetry, QC_PHASE_CRC_POST, stamp, ktime_get_ns(), FRAME_BYTES, 0);
	return 0;
stop:
	isp->stop_reason=QC_STOP_ERROR;
	ak_isp_stream_stop(isp);
	qc_isp_next_flush(isp, edge_start, edge_end, edge_result, &reached);
	return ret;
}
/* Boundedringdiagnostic ONLY: fourdistinctbuffers, up to16IRQevents. Optional
 * held-slot0 candidate may inspect/copy that retired slot while others run,
 * but exposes NO pixels until final reset-quiesced CRC agrees. No encoder or
 * public consumer lease. Stop/reset retiresallDMA beforeguards/usercopy.
 * Do notcallthis productionV4L2 or zero-copy.
 */
static void ak_isp_ring_release_buffers(struct ak_isp *isp)
{
 if(!isp->dma_retired){ak_isp_quarantine(isp,-EIO);return;}
	u32 slot;
	/* Only before DMA start or after successful synchronized stop/reset.
	 * A failed reset retains allocations; never free possibly active DMA.
	 */
	for (slot = 0; slot < 4; slot++) {
		if (slot && isp->ring_frame[slot]) {
			qc_isp_free(isp, FRAME_ALLOC, isp->ring_frame[slot], isp->ring_dma[slot]);
			isp->ring_frame[slot] = NULL;
		}
		if (isp->ring_stat[slot]) {
			qc_isp_free(isp, STAT_ALLOC, isp->ring_stat[slot], isp->ring_stat_dma[slot]);
			isp->ring_stat[slot] = NULL;
		}
	}
}
static int ak_isp_ring_qualify(struct ak_isp *isp, struct ak_isp_file *file, struct qc_isp_ring_test *job)
{
	u32 slot,plane;
	u8 *shadow = NULL;
	int ret,stopped;
	long done;

	if (!job->frames || job->frames > 16 || job->slot >= 4 || job->bytes != FRAME_BYTES || !job->user_ptr)
		return -EINVAL;
	if (job->hold_selected > 1 || job->corrupt_shadow > 1 ||
	    (job->corrupt_shadow && !job->hold_selected) ||
	    (job->hold_selected && job->frames < job->slot + 8))
		return -EINVAL;
	if (isp->disconnected || isp->quarantined)
		return isp->disconnected ? -ENODEV : -EIO;
	if (isp->stream_owner || (isp->session && (isp->session != file || file->receipt.state != QC_RECEIPT_ARMED)))
		return -EBUSY;
	isp->ring_frame[0] = isp->frame;
	isp->ring_completed = 0;
	isp->ring_limit = job->frames;
	isp->ring_hold_selected = job->hold_selected;
	isp->ring_held_slot = job->slot;
	isp->ring_held_ready = isp->ring_bad_order = false;
	job->held_pre_crc = job->held_post_crc = job->held_final_crc = job->held_shadow_crc = 0;
	job->held_bracket_start_events = job->held_bracket_end_events = job->held_active_bracket = 0;
	memset(isp->ring_status, 0, sizeof(isp->ring_status));
	memset(isp->ring_control, 0, sizeof(isp->ring_control));
	memset(isp->ring_elapsed, 0, sizeof(isp->ring_elapsed));
	isp->ring_dma[0] = isp->frame_dma;
	for (slot = 1; slot < 4; slot++) {
		if (!isp->ring_frame[slot])
			isp->ring_frame[slot] = qc_isp_alloc(isp, FRAME_ALLOC, &isp->ring_dma[slot], GFP_KERNEL);
		if (!isp->ring_frame[slot]) { ret = -ENOMEM; goto release; }
		if (!ak_isp_owned_dma(isp->ring_dma[slot], FRAME_ALLOC)) { ret = -ERANGE; goto release; }
		memset(isp->ring_frame[slot], 0xa5, FRAME_ALLOC);
	}
	for (slot = 0; slot < 4; slot++) {
		if (!isp->ring_stat[slot])
			isp->ring_stat[slot] = qc_isp_alloc(isp, STAT_ALLOC, &isp->ring_stat_dma[slot], GFP_KERNEL);
		if (!isp->ring_stat[slot]) { ret = -ENOMEM; goto release; }
		if (!ak_isp_owned_dma(isp->ring_stat_dma[slot], STAT_ALLOC)) { ret = -ERANGE; goto release; }
		memset(isp->ring_stat[slot], 0, STAT_BYTES);
		memset(isp->ring_stat[slot] + STAT_BYTES, 0xa5, STAT_ALLOC - STAT_BYTES);
	}
	if (job->hold_selected) {
		shadow = kvmalloc(FRAME_BYTES, GFP_KERNEL);
		if (!shadow) { ret = -ENOMEM; goto release; }
	}
	ret = ak_isp_stream_begin(isp, file);
	if (ret)
		goto release;
	ret = ak_isp_prepare_irq(isp);
	if (ret)
		goto stop;
	for (slot = 0; slot < 4; slot++) {
		dma_addr_t main = isp->ring_dma[slot], sub = main + MAIN_BYTES;
		const dma_addr_t addresses[] = {main, main + MAIN_PIXELS, main + MAIN_PIXELS * 5 / 4,
			sub, sub + SUB_PIXELS, sub + SUB_PIXELS * 5 / 4};
		memset(isp->ring_frame[slot], 0xa5, FRAME_ALLOC);
		if (!slot) {
			ak_isp_set_planes(isp, 0xd40, main, sub);
			isp->table[0xd58 / 4] = isp->ring_stat_dma[slot];
		} else {
			for (plane = 0; plane < 6; plane++)
				isp->table[(0x7c + (slot - 1) * 4 + plane * 12) / 4] = addresses[plane];
			isp->table[(0xc4 + (slot - 1) * 4) / 4] = isp->ring_stat_dma[slot];
		}
	}
	dma_wmb();
	isp->ring_completed = 0;
	reinit_completion(&isp->irq_done);
	reinit_completion(&isp->ring_first_done);
	WRITE_ONCE(isp->ring_test_active, true);
	isp->ring_started = jiffies;
	isp->irq_mode = true;
	enable_irq(isp->irq);
	writel(readl(isp->base + 0x10) | 0xffff, isp->base + 0x10);
	writel(1, isp->base + 0x1c);
	writel(1, isp->base + 0x14);
	writel(readl(isp->base) | BUFFERS_MASK | BIT(20), isp->base);
	if (job->hold_selected) {
		done = wait_for_completion_interruptible_timeout(&isp->ring_first_done, 2 * HZ);
		if (done <= 0 || !READ_ONCE(isp->ring_held_ready) || READ_ONCE(isp->ring_bad_order)) {
			ret = done == 0 ? -ETIMEDOUT : done < 0 ? (int)done : -EPROTO;
			goto stop;
		}
		/* Diagnostic read of the candidate retired slot, never a public lease.
		 * No pixels leave the kernel until final reset-quiesced CRC agrees.
		 */
		dma_rmb();
		job->held_bracket_start_events = READ_ONCE(isp->ring_completed);
		job->held_pre_crc = crc_ram01_static4(isp->ring_frame[job->slot], FRAME_BYTES);
		memcpy(shadow, isp->ring_frame[job->slot], FRAME_BYTES);
		job->held_post_crc = crc_ram01_static4(isp->ring_frame[job->slot], FRAME_BYTES);
		job->held_bracket_end_events = READ_ONCE(isp->ring_completed);
		job->held_active_bracket = job->held_bracket_end_events > job->held_bracket_start_events &&
			job->held_bracket_end_events < job->frames && (readl(isp->base) & BIT(20));
		if (job->corrupt_shadow)
			shadow[17] ^= 1; /* explicit CPU-only negative fixture; no DMA write */
		job->held_shadow_crc = crc_ram01_static4(shadow, FRAME_BYTES);
		if (job->held_pre_crc != job->held_post_crc || job->held_pre_crc != job->held_shadow_crc ||
		    !job->held_active_bracket || READ_ONCE(isp->ring_bad_order)) {
			ret = -EIO;
			goto stop;
		}
	}
	done = wait_for_completion_interruptible_timeout(&isp->irq_done, 2 * HZ);
	ret = done > 0 ? 0 : done == 0 ? -ETIMEDOUT : (int)done;
	if (isp->ring_bad_order)
		ret = -EPROTO;
stop:
	isp->stop_reason=ret?QC_STOP_ERROR:QC_STOP_END;
	stopped = ak_isp_stream_stop(isp);
	job->completed = isp->ring_completed;
	memcpy(job->irq_status, isp->ring_status, sizeof(job->irq_status));
	memcpy(job->irq_control, isp->ring_control, sizeof(job->irq_control));
	memcpy(job->irq_elapsed_ms, isp->ring_elapsed, sizeof(job->irq_elapsed_ms));
	if (stopped) {
		kvfree(shadow);
		return stopped; /* IRQ traces safe; pixel DMA retirement not established */
	}
	dma_rmb();
	for (slot = 0; slot < 4; slot++) {
		job->slot_guards[slot] = camera_guard_errors_ram(isp->ring_frame[slot] + FRAME_BYTES, FRAME_ALLOC - FRAME_BYTES);
		job->slot_crc[slot] = crc_ram01_static4(isp->ring_frame[slot], FRAME_BYTES);
		job->stat_guards[slot] = camera_guard_errors_ram(isp->ring_stat[slot] + STAT_BYTES, STAT_ALLOC - STAT_BYTES);
		job->main_changed[slot] = camera_guard_errors_ram(isp->ring_frame[slot], MAIN_BYTES);
		job->sub_changed[slot] = camera_guard_errors_ram(isp->ring_frame[slot] + MAIN_BYTES, SUB_BYTES);
		if (job->slot_guards[slot] || job->stat_guards[slot])
			ret = -EOVERFLOW;
		if (!ret && job->frames >= 4 && (!job->main_changed[slot] || !job->sub_changed[slot]))
			ret = -ENODATA;
	}
	if (job->hold_selected) {
		job->held_final_crc = job->slot_crc[job->slot];
		if (!ret && job->held_final_crc != job->held_pre_crc)
			ret = -EIO;
	}
	if (ret)
		goto release;
	if (!job->main_changed[job->slot] || !job->sub_changed[job->slot]) {
		ret = -ENODATA;
		goto release;
	}
	if (copy_to_user((void __user *)(unsigned long)job->user_ptr, shadow ? shadow : isp->ring_frame[job->slot], FRAME_BYTES))
		ret = -EFAULT;
	else if (crc_ram01_static4(isp->ring_frame[job->slot], FRAME_BYTES) != job->slot_crc[job->slot])
		ret = -EIO;
	else if (shadow && crc_ram01_static4(shadow, FRAME_BYTES) != job->held_shadow_crc)
		ret = -EIO;
release:
	kvfree(shadow);
	ak_isp_ring_release_buffers(isp);
	return ret;
}
/* Experimental persistent copied-frame producer. A requested completion is
 * retired in IRQ context; other three slots may be overwritten intentionally.
 * At most one lease, exact monotonic token, no mmap/export/encoder attachment.
 * This API must be qualified before replacing the gated rollback or vb2.
 */
static int ak_isp_queue_stop(struct ak_isp *isp)
{
	int ret = ak_isp_stream_stop(isp);
	u32 slot;
	isp->queue_token = 0;
	if (!ret) {
		dma_rmb();
		for (slot = 0; slot < 4; slot++) {
			if (isp->ring_frame[slot] && camera_guard_errors_ram(isp->ring_frame[slot]+FRAME_BYTES, FRAME_ALLOC-FRAME_BYTES))
				ret = -EOVERFLOW;
			if (isp->ring_stat[slot] && camera_guard_errors_ram(isp->ring_stat[slot]+STAT_BYTES, STAT_ALLOC-STAT_BYTES))
				ret = -EOVERFLOW;
		}
		ak_isp_ring_release_buffers(isp);
	}
	return ret;
}
static int ak_isp_queue_begin(struct ak_isp *isp, struct ak_isp_file *file)
{
	u32 slot,plane;
	int ret;

	if (isp->disconnected || isp->quarantined)
		return isp->disconnected ? -ENODEV : -EIO;
	if (isp->stream_owner || (isp->session && (isp->session != file || file->receipt.state != QC_RECEIPT_ARMED)))
		return -EBUSY;
	isp->ring_frame[0] = isp->frame;
	isp->ring_dma[0] = isp->frame_dma;
	for (slot = 1; slot < 4; slot++) {
		if (!isp->ring_frame[slot])
			isp->ring_frame[slot] = qc_isp_alloc(isp, FRAME_ALLOC, &isp->ring_dma[slot], GFP_KERNEL);
		if (!isp->ring_frame[slot]) { ret = -ENOMEM; goto release; }
		if (!ak_isp_owned_dma(isp->ring_dma[slot], FRAME_ALLOC)) { ret = -ERANGE; goto release; }
		memset(isp->ring_frame[slot], 0xa5, FRAME_ALLOC);
	}
	for (slot = 0; slot < 4; slot++) {
		if (!isp->ring_stat[slot])
			isp->ring_stat[slot] = qc_isp_alloc(isp, STAT_ALLOC, &isp->ring_stat_dma[slot], GFP_KERNEL);
		if (!isp->ring_stat[slot]) { ret = -ENOMEM; goto release; }
		if (!ak_isp_owned_dma(isp->ring_stat_dma[slot], STAT_ALLOC)) { ret = -ERANGE; goto release; }
		memset(isp->ring_stat[slot], 0, STAT_BYTES);
		memset(isp->ring_stat[slot] + STAT_BYTES, 0xa5, STAT_ALLOC - STAT_BYTES);
	}
	ret = ak_isp_stream_begin(isp, file);
	if (ret)
		goto release;
	ret = ak_isp_prepare_irq(isp);
	if (ret) {
		isp->stop_reason=QC_STOP_ERROR;
		ak_isp_queue_stop(isp);
		return ret;
	}
	for (slot = 0; slot < 4; slot++) {
		dma_addr_t main = isp->ring_dma[slot], sub = main + MAIN_BYTES;
		const dma_addr_t addresses[] = {main,main+MAIN_PIXELS,main+MAIN_PIXELS*5/4,
			sub,sub+SUB_PIXELS,sub+SUB_PIXELS*5/4};
		memset(isp->ring_frame[slot], 0xa5, FRAME_ALLOC);
		if (!slot) {
			ak_isp_set_planes(isp, 0xd40, main, sub);
			isp->table[0xd58/4] = isp->ring_stat_dma[slot];
		} else {
			for (plane = 0; plane < 6; plane++)
				isp->table[(0x7c+(slot-1)*4+plane*12)/4] = addresses[plane];
			isp->table[(0xc4+(slot-1)*4)/4] = isp->ring_stat_dma[slot];
		}
	}
	dma_wmb();
	isp->queue_started = isp->queue_request = isp->queue_held = isp->queue_bad = isp->queue_expired = false;
	isp->queue_events = isp->queue_dropped = isp->queue_sequence = 0;
	isp->queue_token = 0;
	reinit_completion(&isp->queue_done);
	WRITE_ONCE(isp->queue_mode, true);
	isp->irq_mode = true;
	enable_irq(isp->irq); /* source off until explicit NEXT */
	return 0;
release:
	ak_isp_ring_release_buffers(isp);
	return ret;
}
static int ak_isp_queue_next(struct ak_isp *isp, struct qc_isp_queue_frame *job)
{
	unsigned long flags;
	u32 slot,guards;
	long done;
	int ret;

	if (job->bytes != FRAME_BYTES || !job->user_ptr)
		return -EINVAL;
	if (!isp->queue_mode)
		return -ESTALE;
	if (time_after_eq(jiffies, isp->stream_deadline)) {
		isp->stop_reason=QC_STOP_EXPIRY;
		ak_isp_queue_stop(isp);
		return -ETIMEDOUT;
	}
	if (isp->queue_token_counter == U64_MAX)
		return -EOVERFLOW;
	if (READ_ONCE(isp->queue_held))
		return -EBUSY;
	reinit_completion(&isp->queue_done);
	raw_spin_lock_irqsave(&isp->queue_lock, flags);
	if (isp->queue_bad) {
		raw_spin_unlock_irqrestore(&isp->queue_lock, flags);
		ret = isp->queue_expired ? -ETIMEDOUT : -EPROTO;
		goto stop;
	}
	isp->queue_request = true;
	if (!isp->queue_started) {
		writel(readl(isp->base+0x10)|0xffff, isp->base+0x10);
		writel(1, isp->base+0x1c);
		isp->queue_started = true;
		writel(1, isp->base+0x14);
		writel(readl(isp->base)|BUFFERS_MASK|BIT(20), isp->base);
	}
	raw_spin_unlock_irqrestore(&isp->queue_lock, flags);
	done = wait_for_completion_interruptible_timeout(&isp->queue_done, 2*HZ);
	if (done <= 0 || READ_ONCE(isp->queue_bad) || !READ_ONCE(isp->queue_held)) {
		ret = done == 0 || isp->queue_expired ? -ETIMEDOUT : done < 0 ? (int)done : -EPROTO;
		goto stop;
	}
	slot = isp->queue_slot;
	dma_rmb();
	guards = camera_guard_errors_ram(isp->ring_frame[slot]+FRAME_BYTES, FRAME_ALLOC-FRAME_BYTES);
	guards += camera_guard_errors_ram(isp->ring_stat[slot]+STAT_BYTES, STAT_ALLOC-STAT_BYTES);
	if (guards) { ret = -EOVERFLOW; goto stop; }
	isp->queue_crc = crc_ram01_static4(isp->ring_frame[slot], FRAME_BYTES);
	if (copy_to_user((void __user *)(unsigned long)job->user_ptr, isp->ring_frame[slot], FRAME_BYTES)) {
		ret = -EFAULT;
		goto stop;
	}
	if (crc_ram01_static4(isp->ring_frame[slot], FRAME_BYTES) != isp->queue_crc ||
	    camera_guard_errors_ram(isp->ring_frame[slot]+FRAME_BYTES, FRAME_ALLOC-FRAME_BYTES) ||
	    camera_guard_errors_ram(isp->ring_stat[slot]+STAT_BYTES, STAT_ALLOC-STAT_BYTES) || READ_ONCE(isp->queue_bad)) {
		ret = isp->queue_expired ? -ETIMEDOUT : -EIO;
		goto stop;
	}
	job->token = isp->queue_token;
	job->slot = slot;job->status = isp->queue_status;
	job->guard_errors = guards;job->crc32_bzip2 = isp->queue_crc;
	job->sequence = ++isp->queue_sequence;
	job->irq_events = READ_ONCE(isp->queue_events);
	job->dropped = READ_ONCE(isp->queue_dropped);
	return 0;
stop:
	isp->stop_reason=isp->queue_expired || time_after_eq(jiffies,isp->stream_deadline)?QC_STOP_EXPIRY:QC_STOP_ERROR;
	ak_isp_queue_stop(isp);
	return ret;
}
static int ak_isp_queue_return(struct ak_isp *isp, const struct qc_isp_queue_frame *job)
{
	unsigned long flags;
	u32 slot;

	if (!isp->queue_mode || !isp->queue_held || job->token != isp->queue_token)
		return -ESTALE;
	if (time_after_eq(jiffies, isp->stream_deadline) || READ_ONCE(isp->queue_expired)) {
		isp->stop_reason=QC_STOP_EXPIRY;
		ak_isp_queue_stop(isp);
		return -ETIMEDOUT;
	}
	slot = isp->queue_slot;
	dma_rmb();
	if (crc_ram01_static4(isp->ring_frame[slot], FRAME_BYTES) != isp->queue_crc ||
	    camera_guard_errors_ram(isp->ring_frame[slot]+FRAME_BYTES, FRAME_ALLOC-FRAME_BYTES) ||
	    camera_guard_errors_ram(isp->ring_stat[slot]+STAT_BYTES, STAT_ALLOC-STAT_BYTES)) {
		isp->stop_reason=QC_STOP_ERROR;
		ak_isp_queue_stop(isp);
		return -EIO;
	}
	raw_spin_lock_irqsave(&isp->queue_lock, flags);
	if (isp->queue_bad) {
		raw_spin_unlock_irqrestore(&isp->queue_lock, flags);
		isp->stop_reason=READ_ONCE(isp->queue_expired)?QC_STOP_EXPIRY:QC_STOP_ERROR;
		ak_isp_queue_stop(isp);
		return -EPROTO;
	}
	isp->queue_held = false;
	isp->queue_token = 0;
	writel(readl(isp->base)|BIT(15+slot)|BIT(21+slot), isp->base);
	readl(isp->base);
	raw_spin_unlock_irqrestore(&isp->queue_lock, flags);
	return 0;
}
/* Close/remove are real lifecycle operations, but may not nest an existing
 * stage. Owner counters remain observational and never substitute for proof. */
static int ak_isp_lifecycle_stop(struct ak_isp *isp,bool queue)
{
 bool staged=!isp->telemetry.active;
 int ret;
 if(staged)qc_stage_begin(&isp->telemetry,QC_STAGE_END,ktime_get_ns());
 ret=queue?ak_isp_queue_stop(isp):ak_isp_stream_stop(isp);
 qc_stage_owner(&isp->telemetry,!!isp->stream_owner,0);
 if(staged)qc_stage_end(&isp->telemetry,ret,0,ktime_get_ns());
 return ret;
}
static long ak_isp_stream_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
 struct ak_isp_file *fc=file->private_data;
 struct ak_isp *isp=fc->isp;
	struct qc_isp_next next;
	int ret;
 bool staged=false;

 if(cmd==QC_ISP_RECEIPT_BIND || cmd==QC_ISP_RECEIPT_ARM || cmd==QC_ISP_RECEIPT_QUERY || cmd==QC_ISP_RECEIPT_ACK){
  mutex_lock(&isp->lock);ret=ak_isp_receipt_ioctl(fc,cmd,arg);mutex_unlock(&isp->lock);return ret;
 }
 if(READ_ONCE(isp->disconnected))return -ENODEV;
	if (cmd == QC_ISP_STAGE_QUERY) {
		struct qc_stage_query query;
		mutex_lock(&isp->lock);
  if(isp->disconnected){mutex_unlock(&isp->lock);return -ENODEV;}
		if (copy_from_user(&query, (void __user *)arg, sizeof(query)))
			ret = -EFAULT;
		else if (qc_stage_query(&isp->telemetry, &query))
			ret = -EINVAL;
		else
			ret = copy_to_user((void __user *)arg, &query, sizeof(query)) ? -EFAULT : 0;
		mutex_unlock(&isp->lock);
		return ret;
	}
	if (cmd != QC_ISP_BEGIN && cmd != QC_ISP_BEGIN_IRQ && cmd != QC_ISP_RING_TEST && cmd != QC_ISP_NEXT && cmd != QC_ISP_END && cmd != QC_ISP_COLOUR &&
	    cmd != QC_ISP_QUEUE_BEGIN && cmd != QC_ISP_QUEUE_NEXT && cmd != QC_ISP_QUEUE_RETURN && cmd != QC_ISP_QUEUE_END && cmd != QC_ISP_WHITEBALANCE && cmd != QC_ISP_WHITEBALANCE_MANUAL)
		return -ENOTTY;
	mutex_lock(&isp->lock);
 if(isp->disconnected){mutex_unlock(&isp->lock);return -ENODEV;}
 if(isp->quarantined){mutex_unlock(&isp->lock);return -EIO;}
 if(cmd!=QC_ISP_WHITEBALANCE && cmd!=QC_ISP_WHITEBALANCE_MANUAL){
	staged=true;
	qc_stage_begin(&isp->telemetry,
		cmd == QC_ISP_NEXT ? (isp->irq_mode ? QC_STAGE_NEXT_IRQ : QC_STAGE_NEXT) :
		(cmd == QC_ISP_BEGIN || cmd == QC_ISP_BEGIN_IRQ) ? QC_STAGE_BEGIN :
		cmd == QC_ISP_END ? QC_STAGE_END :
		(cmd == QC_ISP_QUEUE_BEGIN || cmd == QC_ISP_QUEUE_NEXT || cmd == QC_ISP_QUEUE_RETURN || cmd == QC_ISP_QUEUE_END) ? QC_STAGE_QUEUE : QC_STAGE_OTHER,
		ktime_get_ns());
 }
	if (cmd == QC_ISP_WHITEBALANCE_MANUAL) {
		struct qc_isp_whitebalance_manual wb;
		u32 words[3];
		if (copy_from_user(&wb, (void __user *)arg, sizeof(wb)))
			ret = -EFAULT;
		else if (wb.version != QC_ISP_WB_MANUAL_VERSION || wb.set > 1 || wb.reserved)
			ret = -EINVAL;
		else {
			if(wb.set){staged=true;qc_stage_begin(&isp->telemetry,QC_STAGE_OTHER,ktime_get_ns());}
			if(wb.set && ak_isp_control_busy(isp,fc))ret=-EBUSY;
			else {
			memcpy(words, isp->wb_desired_words, sizeof(words));
			if (wb.set && qc_isp_wb_manual(words, wb.gain, wb.offset_raw)) {
				ret = -EINVAL;
			} else {
				wb.preset = wb.set ? QC_ISP_WB_CUSTOM : isp->wb_preset;
				qc_isp_wb_fields(words, wb.gain, wb.offset_raw);
				wb.table_prepared = isp->wb_table_prepared;
				wb.last_prepared_preset = isp->wb_table_preset;
				qc_isp_wb_fields(isp->wb_prepared_words, wb.last_prepared_gain, wb.last_prepared_offset_raw);
				wb.camera_id = 2;
				wb.owner_active = !!isp->stream_owner;
				wb.desired_changed_ns = wb.set ? ktime_get_ns() : isp->wb_desired_changed_ns;
				wb.last_prepared_ns = isp->wb_last_prepared_ns;
				ret = copy_to_user((void __user *)arg, &wb, sizeof(wb)) ? -EFAULT : 0;
				if (!ret && wb.set) {
					memcpy(isp->wb_desired_words, words, sizeof(words));
					isp->wb_preset = QC_ISP_WB_CUSTOM;
					isp->wb_desired_changed_ns = wb.desired_changed_ns;
				}
			}
			}
		}
	} else if (cmd == QC_ISP_WHITEBALANCE) {
		struct qc_isp_whitebalance wb;
		u32 words[3];
		if (copy_from_user(&wb, (void __user *)arg, sizeof(wb)))
			ret = -EFAULT;
		else if (wb.set > 1 || (wb.set && wb.preset > 1))
			ret = -EINVAL;
		else {
			if(wb.set){staged=true;qc_stage_begin(&isp->telemetry,QC_STAGE_OTHER,ktime_get_ns());}
			if(wb.set && ak_isp_control_busy(isp,fc))ret=-EBUSY;
			else {
			u32 preset = wb.set ? wb.preset : isp->wb_preset;
			if (wb.set) {
				memcpy(words, &qc07f_isp_reference[0xd34 / 4], sizeof(words));
				qc_isp_wb_preset(preset, words);
			} else {
				memcpy(words, isp->wb_desired_words, sizeof(words));
			}
			wb.preset = preset;
			qc_isp_wb_fields(words, wb.gain, wb.offset_raw);
			/* A failed reply must not commit the desired state. */
			ret = copy_to_user((void __user *)arg, &wb, sizeof(wb)) ? -EFAULT : 0;
			if (!ret && wb.set) {
				isp->wb_preset = preset;
				memcpy(isp->wb_desired_words, words, sizeof(words));
				isp->wb_desired_changed_ns = ktime_get_ns();
			}
			}
		}
	} else if (cmd == QC_ISP_QUEUE_BEGIN)
		ret = ak_isp_queue_begin(isp, fc);
	else if (cmd == QC_ISP_QUEUE_NEXT || cmd == QC_ISP_QUEUE_RETURN || cmd == QC_ISP_QUEUE_END) {
		struct qc_isp_queue_frame job;
		if (isp->stream_owner != fc)
			ret = -EPERM;
		else if (cmd == QC_ISP_QUEUE_END) {
			isp->stop_reason=QC_STOP_END;
			ret = ak_isp_queue_stop(isp);
		}
		else if (copy_from_user(&job, (void __user *)arg, sizeof(job)))
			ret = -EFAULT;
		else if (cmd == QC_ISP_QUEUE_RETURN)
			ret = ak_isp_queue_return(isp, &job);
		else {
			ret = ak_isp_queue_next(isp, &job);
			if (copy_to_user((void __user *)arg, &job, sizeof(job))) {
				if (isp->queue_mode) {
					isp->stop_reason=QC_STOP_ERROR;
					ak_isp_queue_stop(isp);
				}
				ret = -EFAULT;
			}
		}
	} else if (cmd == QC_ISP_RING_TEST) {
		struct qc_isp_ring_test job;
		if (copy_from_user(&job, (void __user *)arg, sizeof(job)))
			ret = -EFAULT;
		else {
			ret = ak_isp_ring_qualify(isp, fc, &job);
			if (copy_to_user((void __user *)arg, &job, sizeof(job)))
				ret = -EFAULT;
		}
	} else if (cmd == QC_ISP_COLOUR) {
		struct qc_isp_colour colour;
		if (ak_isp_control_busy(isp,fc))
			ret = -EBUSY;
		else if (copy_from_user(&colour, (void __user *)arg, sizeof(colour)))
			ret = -EFAULT;
		else if (colour.monochrome > 1)
			ret = -EINVAL;
		else {
			colour.uv_gain_readback = colour.monochrome ? 0 : 64;
			ret = copy_to_user((void __user *)arg, &colour, sizeof(colour)) ? -EFAULT : 0;
			if(!ret){isp->uv_gain=colour.uv_gain_readback;isp->colour_override=true;}
		}
	} else if (cmd == QC_ISP_BEGIN_IRQ) {
		ret = ak_isp_stream_begin(isp, fc);
		if (!ret) {
			ret = ak_isp_prepare_irq(isp);
			if (ret) {
				isp->stop_reason=QC_STOP_ERROR;
				ak_isp_stream_stop(isp);
			}
			else {
				isp->irq_mode = true;
				enable_irq(isp->irq);
			}
		}
	} else if (cmd == QC_ISP_BEGIN)
		ret = ak_isp_stream_begin(isp, fc);
	else if (isp->stream_owner != fc)
		ret = -EPERM;
	else if (cmd == QC_ISP_END && isp->queue_mode) {
		isp->stop_reason=QC_STOP_END;
		ret = ak_isp_queue_stop(isp);
	}
	else if (isp->queue_mode)
		ret = -EBUSY;
	else if (cmd == QC_ISP_END) {
		isp->stop_reason=QC_STOP_END;
		ret = ak_isp_stream_stop(isp);
	}
	else if (copy_from_user(&next, (void __user *)arg, sizeof(next)))
		ret = -EFAULT;
	else {
		ret = ak_isp_stream_next(isp, &next);
		if (copy_to_user((void __user *)arg, &next, sizeof(next)))
			ret = -EFAULT;
	}
 if(staged){
 if(!fc->receipt_visible)ak_isp_consume_retired(isp,fc);
	qc_stage_owner(&isp->telemetry, !!isp->stream_owner, 0);
	qc_stage_end(&isp->telemetry, ret, cmd == QC_ISP_NEXT || cmd == QC_ISP_QUEUE_NEXT, ktime_get_ns());
 }
	mutex_unlock(&isp->lock);
	return ret;
}
static int ak_isp_stream_release(struct inode *inode, struct file *file)
{
 struct ak_isp_file *fc=file->private_data;struct ak_isp *isp=fc->isp;int ret=0;
 (void)inode;kref_get(&isp->references);mutex_lock(&isp->lock);
 if(isp->session==fc && !isp->dma_retired && !isp->quarantined){
  isp->stop_reason=QC_STOP_CLOSE;
   ret=ak_isp_lifecycle_stop(isp,isp->queue_mode);
 }
 fc->receipt.open=0;isp->receipt_source.open_files--;file->private_data=NULL;
 if(isp->session==fc){
  if(fc->receipt.state==QC_RECEIPT_ARMED){isp->receipt_source.hold=NULL;isp->session=NULL;ak_isp_file_put(fc);}
  else if(isp->dma_retired)ak_isp_consume_retired(isp,fc);
  else {ret=isp->quarantine_result?isp->quarantine_result:-EIO;ak_isp_quarantine(isp,ret);}
 }
 qc_stage_owner(&isp->telemetry,!!isp->stream_owner,0);
 mutex_unlock(&isp->lock);ak_isp_file_put(fc);ak_isp_bundle_put(isp);return ret;
}
static ssize_t ak_isp_frame_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
 struct ak_isp_file *fc=file->private_data;
 struct ak_isp *isp=fc->isp;
	ssize_t ret;

	mutex_lock(&isp->lock);
 if(isp->disconnected){mutex_unlock(&isp->lock);return -ENODEV;}
 if(isp->quarantined){mutex_unlock(&isp->lock);return -EIO;}
	/* Historical sysfs diagnostic data shared across opens, NOT per-open pixel
	 * isolation or a delivery for this file's current receipt generation. */
	ret = isp->readable ? simple_read_from_buffer(buf, count, ppos, isp->frame, FRAME_BYTES) : -ENODATA;
	mutex_unlock(&isp->lock);
	return ret;
}
static loff_t ak_isp_frame_seek(struct file *file, loff_t off, int whence)
{
 struct ak_isp_file *fc=file->private_data;struct ak_isp *isp=fc->isp;loff_t ret;
 mutex_lock(&isp->lock);
 ret=isp->disconnected?-ENODEV:isp->quarantined?-EIO:default_llseek(file,off,whence);
 mutex_unlock(&isp->lock);return ret;
}
static const struct file_operations ak_isp_fops = {
	.owner = THIS_MODULE, .open = ak_isp_open, .read = ak_isp_frame_read, .llseek = ak_isp_frame_seek,
	.unlocked_ioctl = ak_isp_stream_ioctl, .release = ak_isp_stream_release,
};
/* Concrete manual teardown. No dma/mapping/IRQ devres owns these resources. */
static void ak_isp_bundle_release(struct kref *ref)
{
 struct ak_isp *isp=container_of(ref,struct ak_isp,references);
 struct device *dev=isp->dev;
 if(WARN_ON(!isp->disconnected || !isp->dma_retired || isp->session || isp->receipt_source.hold ||
            isp->receipt_source.open_files || isp->quarantined))return;
 if(isp->irq_registered){synchronize_irq(isp->irq);free_irq(isp->irq,isp);isp->irq_registered=false;}
 ak_isp_ring_release_buffers(isp); /* ring[0] aliases frame; helper never frees it */
 if(isp->reference)qc_isp_free(isp,REF_ALLOC,isp->reference,isp->reference_dma);
 if(isp->frame)qc_isp_free(isp,FRAME_ALLOC,isp->frame,isp->frame_dma);
 if(isp->table)qc_isp_free(isp,TABLE_ALLOC,isp->table,isp->table_dma);
 if(isp->base)iounmap(isp->base);
 if(isp->region_owned)release_mem_region(isp->region_start,isp->region_bytes);
 of_node_put(isp->sys_np);
 mutex_lock(&ak_isp_registry_lock);WARN_ON(ak_isp_live!=isp);ak_isp_live=NULL;mutex_unlock(&ak_isp_registry_lock);
 kfree(isp);put_device(dev);
}
static int ak_isp_probe(struct platform_device *pdev)
{
 struct device *dev=&pdev->dev;struct ak_isp *isp;struct resource *res;int ret;
#if defined(CONFIG_PM) || defined(CONFIG_ARM_DMA_USE_IOMMU) || defined(CONFIG_XEN) || defined(CONFIG_OF_DYNAMIC)
 return -EOPNOTSUPP; /* no provider/PM/IOMMU lease contract in this prototype */
#endif
 if(!of_machine_is_compatible("anyka,qc07f"))return -EOPNOTSUPP;
 /* dd.c removes DMA ops/range maps after .remove despite get_device(). Only
  * default direct ARM DMA with no per-device pool/range/CMA is supported. */
 if(get_dma_ops(dev) || dev->dma_range_map)return -EOPNOTSUPP;
#ifdef CONFIG_DMA_DECLARE_COHERENT
 if(dev->dma_mem)return -EOPNOTSUPP;
#endif
#ifdef CONFIG_DMA_CMA
 if(dev->cma_area)return -EOPNOTSUPP;
#endif
 /* Permanent generic-DT syscon only. Kernel72 is built-in / CONFIG_PM=n;
  * reject future dynamic resource properties rather than assume get_device pins
  * another provider driver's devres or physical power after removal.
  */
 mutex_lock(&ak_isp_registry_lock);
 if(ak_isp_live){mutex_unlock(&ak_isp_registry_lock);return -EBUSY;}
 isp=kzalloc(sizeof(*isp),GFP_KERNEL);
 if(!isp){mutex_unlock(&ak_isp_registry_lock);return -ENOMEM;}
 kref_init(&isp->references);mutex_init(&isp->lock);isp->dev=get_device(dev);
 ret=qc_retirement_source_init(&ak_isp_ids,&isp->receipt_source);
 if(ret){put_device(dev);kfree(isp);mutex_unlock(&ak_isp_registry_lock);return ret;}
 isp->dma_retired=true;isp->stop_reason=QC_STOP_END;ak_isp_live=isp;mutex_unlock(&ak_isp_registry_lock);
 qc_stage_isp_coverage(&isp->telemetry);camera_crc_bzip2_table(isp->crc_table);
 init_completion(&isp->irq_done);init_completion(&isp->ring_first_done);init_completion(&isp->queue_done);
 raw_spin_lock_init(&isp->queue_lock);
 res=platform_get_resource(pdev,IORESOURCE_MEM,0);
 if(!res){ret=-EINVAL;goto fail;}
 isp->region_start=res->start;isp->region_bytes=resource_size(res);
 if(!request_mem_region(isp->region_start,isp->region_bytes,"qc07f-isp-resource03")){ret=-EBUSY;goto fail;}
 isp->region_owned=true;
 isp->base=(res->flags&IORESOURCE_MEM_NONPOSTED)?ioremap_np(res->start,resource_size(res)):ioremap(res->start,resource_size(res));
 if(!isp->base){ret=-ENOMEM;goto fail;}
 isp->sys_np=of_parse_phandle(dev->of_node,"anyka,sysctrl",0);
 if(!isp->sys_np){ret=-ENODEV;goto fail;}
 if(!of_device_is_compatible(isp->sys_np,"anyka,ak3918ev200-intc") ||
    !of_device_is_compatible(isp->sys_np,"syscon") || of_find_property(isp->sys_np,"clocks",NULL) ||
    of_find_property(isp->sys_np,"resets",NULL) || of_find_property(isp->sys_np,"power-domains",NULL) ||
    of_find_property(dev->of_node,"power-domains",NULL) || of_find_property(dev->of_node,"clocks",NULL)){
  ret=-EOPNOTSUPP;goto fail;
 }
 isp->sys=syscon_node_to_regmap(isp->sys_np);
 if(IS_ERR(isp->sys)){ret=PTR_ERR(isp->sys);goto fail;}
 ret=dma_set_mask_and_coherent(dev,DMA_BIT_MASK(32));if(ret)goto fail;
 isp->table=qc_isp_alloc(isp,TABLE_ALLOC,&isp->table_dma,GFP_KERNEL);
 isp->frame=qc_isp_alloc(isp,FRAME_ALLOC,&isp->frame_dma,GFP_KERNEL);
 isp->reference=qc_isp_alloc(isp,REF_ALLOC,&isp->reference_dma,GFP_KERNEL);
 if(!isp->table||!isp->frame||!isp->reference){ret=-ENOMEM;goto fail;}
 if(!ak_isp_owned_dma(isp->table_dma,TABLE_ALLOC)||!ak_isp_owned_dma(isp->frame_dma,FRAME_ALLOC)||
    !ak_isp_owned_dma(isp->reference_dma,REF_ALLOC)){ret=-ERANGE;goto fail;}
 memcpy(isp->wb_desired_words,&qc07f_isp_reference[0xd34/4],sizeof(isp->wb_desired_words));
 isp->wb_desired_changed_ns=ktime_get_ns();isp->last_result=-ENODATA;
 isp->misc.minor=MISC_DYNAMIC_MINOR;isp->misc.name="qc07f-frame";isp->misc.fops=&ak_isp_fops;
 isp->misc.parent=dev;isp->misc.mode=0400;platform_set_drvdata(pdev,isp);
 ret=misc_register(&isp->misc);if(ret)goto fail;isp->misc_registered=true;
 ret=sysfs_create_group(&dev->kobj,&ak_isp_attr_group);if(ret)goto fail_registered;isp->attrs_registered=true;
 mutex_lock(&isp->lock);isp->ready=true;mutex_unlock(&isp->lock);
 dev_info(dev,"resource03 SOURCE PROTOTYPE manual retained bundle, no capture at probe\n");return 0;
fail_registered:
 mutex_lock(&isp->lock);isp->disconnected=true;mutex_unlock(&isp->lock);
 misc_deregister(&isp->misc);isp->misc_registered=false;platform_set_drvdata(pdev,NULL);
 /* misc was briefly visible: racing successful open owns its own bundle pin. */
 ak_isp_bundle_put(isp);return ret;
fail:
 isp->disconnected=true;platform_set_drvdata(pdev,NULL);ak_isp_bundle_put(isp);return ret;
}
static void ak_isp_remove(struct platform_device *pdev)
{
 struct ak_isp *isp=platform_get_drvdata(pdev);
 if(!isp)return;
 kref_get(&isp->references);mutex_lock(&isp->lock);isp->disconnected=true;mutex_unlock(&isp->lock);
 /* Removal of the sysfs group drains active callbacks; do not hold source lock.
  * misc_deregister serializes its first-kref/open handoff via misc_mtx. */
 if(isp->attrs_registered){sysfs_remove_group(&pdev->dev.kobj,&ak_isp_attr_group);isp->attrs_registered=false;}
 if(isp->misc_registered){misc_deregister(&isp->misc);isp->misc_registered=false;}
 mutex_lock(&isp->lock);
 if(!isp->dma_retired && !isp->quarantined){isp->stop_reason=QC_STOP_CLOSE;ak_isp_lifecycle_stop(isp,false);}
 qc_stage_owner(&isp->telemetry,!!isp->stream_owner,0);
 if(isp->irq_registered)synchronize_irq(isp->irq);
 /* No worker/timer exists in frozen72; IRQ synchronization plus source mutex
  * drains every actual async producer. No invented workqueue success callback. */
 if(!isp->dma_retired)ak_isp_quarantine(isp,isp->quarantine_result);
 if(isp->session && !isp->session->receipt.open && isp->dma_retired)
  ak_isp_consume_retired(isp,isp->session);
 mutex_unlock(&isp->lock);platform_set_drvdata(pdev,NULL);
 ak_isp_bundle_put(isp); /* registration */
 ak_isp_bundle_put(isp); /* remove operation */
 /* Failed reset retains one registry bundle + its own context/IRQ/MMIO/DMA.
  * There is deliberately no unsafe automatic recovery access after remove. */
}

static const struct of_device_id ak_isp_match[] = {
	{ .compatible = "anyka,qc07f-isp-diagnostic" }, { }
};
MODULE_DEVICE_TABLE(of, ak_isp_match);
static struct platform_driver ak_isp_driver = {
	.probe = ak_isp_probe,
 .remove = ak_isp_remove,
	.driver = { .name = "qc07f-isp-diagnostic", .of_match_table = ak_isp_match,
		    .suppress_bind_attrs = true },
};
module_platform_driver(ak_isp_driver);
MODULE_DESCRIPTION("QC07F bounded ISP first-frame diagnostic");
MODULE_LICENSE("GPL");
