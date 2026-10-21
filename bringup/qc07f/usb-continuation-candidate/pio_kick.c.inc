/* SPDX-License-Identifier: GPL-2.0-only */
/* Caller holds the MUSB lock and has selected the nonzero TX endpoint. */
static inline void musb_h_tx_pio_kick(struct musb *musb, void __iomem *epio)
{
	/* Anyka shares FIFO direction in CSR2. A constant 16-bit kick clears
	 * MODE at packet two (and on NAK retry). Retail kicks CSR1 only; do
	 * not replay or clear the high byte, nor touch the hardware toggle.
	 * DMA-capable configurations retain the existing generic sequence.
	 */
	if ((musb->ops->quirks & MUSB_ANYKA_PIO_TOGGLE) && !is_dma_capable())
		musb_writeb(epio, MUSB_TXCSR, MUSB_TXCSR_TXPKTRDY);
	else
		musb_writew(epio, MUSB_TXCSR,
			    MUSB_TXCSR_H_WZC_BITS | MUSB_TXCSR_TXPKTRDY);
}
