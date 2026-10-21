/* SPDX-License-Identifier: GPL-2.0-only */
/* Actual injected helper under bus-write mocks: not a controller emulator. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "register_constants.h"
#define __iomem
#define BIT(n) (1u << (n))
struct platform_ops { unsigned quirks; };
struct musb { const struct platform_ops *ops; };
static unsigned dma_capable;
#define is_dma_capable() (dma_capable)
static unsigned byte_writes, word_writes;

static void musb_writeb(void *addr, unsigned offset, unsigned value)
{
	assert(offset == MUSB_TXCSR && value == MUSB_TXCSR_TXPKTRDY);
	((uint8_t *)addr)[offset] = value;
	byte_writes++;
}

static void musb_writew(void *addr, unsigned offset, unsigned value)
{
	assert(offset == MUSB_TXCSR);
	assert(value == (MUSB_TXCSR_H_WZC_BITS | MUSB_TXCSR_TXPKTRDY));
	/* Selected Anyka accessor writes high then low. Generic branch's
	 * value/access width is unchanged; this is not WZC hardware modeling. */
	((uint8_t *)addr)[offset + 1] = value >> 8;
	((uint8_t *)addr)[offset] = value;
	word_writes++;
}

#include "actual_helper.inc"

int main(void)
{
	unsigned dma, anyka, before, cases = 0;
	/* Negative mutants can legitimately remove one callback's call sites. */
	(void)&musb_writeb;
	(void)&musb_writew;
	assert(MUSB_TXCSR_H_WZC_BITS == 0xa6);
	assert(MUSB_TXCSR_MODE == 0x2000);
	for (dma = 0; dma < 2; dma++) {
		for (anyka = 0; anyka < 2; anyka++) {
			struct platform_ops ops = { .quirks = BIT(3) |
				(anyka ? MUSB_ANYKA_PIO_TOGGLE : 0) };
			struct musb musb = { .ops = &ops };
			dma_capable = dma;
			for (before = 0; before <= UINT16_MAX; before++) {
				uint8_t memory[16], expected[16];
				memset(memory, 0x5a, sizeof(memory));
				memory[MUSB_TXCSR] = before;
				memory[MUSB_TXCSR + 1] = before >> 8;
				memcpy(expected, memory, sizeof(memory));
				byte_writes = word_writes = 0;
				musb_h_tx_pio_kick(&musb, memory);
				assert(dma_capable == dma);
				if (anyka && !dma) {
					expected[MUSB_TXCSR] = 1;
					assert(byte_writes == 1 && word_writes == 0);
				} else {
					expected[MUSB_TXCSR] = 0xa7;
					expected[MUSB_TXCSR + 1] = 0;
					assert(byte_writes == 0 && word_writes == 1);
				}
				assert(memcmp(expected, memory, sizeof(memory)) == 0);
				assert(musb.ops == &ops);
				assert(ops.quirks == (BIT(3) |
					(anyka ? MUSB_ANYKA_PIO_TOGGLE : 0)));
				cases++;
			}
		}
	}
	printf("{\"helper_cases\":%u,\"hardware_qualified\":false}\n", cases);
	return 0;
}
