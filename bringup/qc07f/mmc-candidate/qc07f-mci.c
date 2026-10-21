// SPDX-License-Identifier: GPL-2.0-only
/* Offline Linux 6.18.55 host adapter candidate. Not a deployable driver.
 * Controller encoding is in host-core.c (Anyka GPLv2 derivation).
 * No provider exists for measured retail power/CD/clock/mux/L2 retirement.
 * Probe MUST remain a no-effect refusal until that provider is reviewed.
 */
#include <linux/errno.h>
#include <linux/mmc/host.h>
#include <linux/mmc/core.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/scatterlist.h>
#include <linux/workqueue.h>
#include "host-core.h"

struct qc_host {
 struct mmc_host *mmc;
 struct mmc_request *mrq;
 struct work_struct work;
 struct qc_engine engine;
 struct qc_io io;
};

static int qc_errno(int result)
{
 switch (result) {
 case QC_OK: return 0;
 case QC_DENIED: return -EROFS;
 case QC_TIMEOUT: return -ETIMEDOUT;
 case QC_CRC: return -EILSEQ;
 case QC_CANCELLED: return -ENOMEDIUM;
 case QC_EXHAUSTED: return -EOVERFLOW;
 default: return -EIO;
 }
}

static void qc_work(struct work_struct *work)
{
 struct qc_host *h = container_of(work, struct qc_host, work);
 struct mmc_request *mrq = h->mrq;
 struct mmc_command *cmd = mrq->cmd;
 struct qc_request r = { .opcode = cmd->opcode, .arg = cmd->arg };
 int result;
 if (cmd->flags & MMC_RSP_PRESENT) r.response_flags |= QC_RSP_PRESENT;
 if (cmd->flags & MMC_RSP_136) r.response_flags |= QC_RSP_LONG;
 if (cmd->flags & MMC_RSP_CRC) r.response_flags |= QC_RSP_CRC;
 if (cmd->flags & MMC_RSP_BUSY) r.response_flags |= QC_RSP_BUSY;
 if (mrq->data) {
  struct mmc_data *d = mrq->data;
  d->bytes_xfered = 0;
  r.blocks = d->blocks;
  /* Check before multiplication. Never map caller SG for DMA. */
  if (!d->blksz || d->blksz > QC_MAX_BYTES || d->blocks != 1 ||
      d->sg_len != 1 || !d->sg || d->sg->length != d->blksz ||
      d->flags != MMC_DATA_READ) {
   result = QC_DENIED;
   goto done;
  }
  r.bytes = d->blksz;
  r.data_flags = QC_READ;
 }
 if (mrq->sbc || mrq->stop) { result = QC_DENIED; goto done; }
 result = qc_run(&h->engine, &r, &h->io);
 if (!result) memcpy(cmd->resp, r.response, sizeof(cmd->resp));
done:
 cmd->error = qc_errno(result);
 cmd->retries = 0; /* No driver retry/forced-profile fallback. */
 if (mrq->data) {
  mrq->data->error = cmd->error;
  if (!cmd->error) mrq->data->bytes_xfered = r.bytes;
 }
 h->mrq = NULL;
 mmc_request_done(h->mmc, mrq);
}

static void qc_request(struct mmc_host *mmc, struct mmc_request *mrq)
{
 struct qc_host *h = mmc_priv(mmc);
 int ready = qc_request_ready(&h->engine);
 /* MMC core serializes one request; a future probe must initialize work and
  * an exclusive ordered queue; remove must cancel/drain BEFORE freeing h.
  * No such resources are allocated by the current refusal-only probe. */
 if (h->mrq || ready) {
  mrq->cmd->error = h->mrq ? -EIO : qc_errno(ready);
  mrq->cmd->retries = 0;
  mmc_request_done(mmc, mrq);
  return;
 }
 h->mrq = mrq;
 schedule_work(&h->work);
}

static int qc_ro(struct mmc_host *mmc) { return 1; }

/* No set_ios/get_cd implementation is fabricated. Required provider must
 * supply measured CD polarity, external-power contract, exclusive masked
 * clock/reset/pinctrl ownership, and bounded DMA_FROM_DEVICE bounce+retire.
 * The adapter is deliberately unreachable until these callbacks exist.
 */
static const struct mmc_host_ops qc_ops __used = {
 .request = qc_request,
 .get_ro = qc_ro,
};

/* Retain the dormant glue in the reviewed ARM object. This initializes only
 * software fields; it does NOT supply missing ios/CD/DMA callbacks, register
 * a host, or make the refusal-only probe usable. Provider/queue/removal
 * integration remains an explicit source blocker, not a DT boolean. */
static void __used qc_init_host(struct qc_host *h, struct mmc_host *mmc,
                                const struct qc_io *io)
{
 h->mmc = mmc;
 h->mrq = NULL;
 memset(&h->engine, 0, sizeof(h->engine));
 h->io = *io;
 INIT_WORK(&h->work, qc_work);
 mmc->ops = &qc_ops;
 mmc->caps = 0; /* No 4-bit, HS, UHS, SDIO IRQ or unmeasured capability. */
 mmc->caps2 = MMC_CAP2_NO_MMC | MMC_CAP2_NO_SDIO;
 mmc->max_segs = 1;
 mmc->max_seg_size = QC_MAX_BYTES;
 mmc->max_req_size = QC_MAX_BYTES;
 mmc->max_blk_size = QC_MAX_BYTES;
 mmc->max_blk_count = 1;
}

static int qc_probe(struct platform_device *pdev)
{
 /* Do not request MMIO/IRQ/GPIO/clock/reset/pinctrl, allocate DMA, register an
  * MMC host (which starts card discovery), or trust a DT "verified" boolean.
  * A future implementation requires a reviewed code/provider patch, not an
  * enabling property/module parameter. No default pin state is selected. */
 return dev_err_probe(&pdev->dev, -EOPNOTSUPP,
  "QC07F MMC refused: retail routing/CD/voltage and shared L2 retirement provider unqualified\n");
}

static const struct of_device_id qc_match[] = {
 { .compatible = "anyka,qc07f-mci-candidate" }, { }
};
MODULE_DEVICE_TABLE(of, qc_match);
static struct platform_driver qc_driver = {
 .probe = qc_probe,
 .driver = {
  .name = "qc07f-mci-candidate",
  .of_match_table = qc_match,
  .suppress_bind_attrs = true,
 },
};
builtin_platform_driver(qc_driver);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("QC07F MCI offline candidate; fail-closed probe, no hardware provider");
