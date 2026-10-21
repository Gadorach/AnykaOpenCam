// SPDX-License-Identifier: GPL-2.0-only
/* QC07F UART-first prototype, not a validated retail-board BSP. */
#include <asm/mach/arch.h>
#include <linux/kernel.h>
#include <linux/sizes.h>
#include <asm/mach/map.h>
#include "io.h"

static struct map_desc ak3918ev200_io[] __initdata = {
	{ .virtual = AK_IO_CONTROLLER_VA,
	  .pfn = __phys_to_pfn(AK_IO_CONTROLLER_PA),
	  .length = SZ_1M, .type = MT_DEVICE },
	{ .virtual = AK_IO_SRAM_VA,
	  .pfn = __phys_to_pfn(AK_IO_SRAM_PA),
	  .length = SZ_1M, .type = MT_DEVICE },
};
static void __init ak3918ev200_map_io(void)
{
	iotable_init(ak3918ev200_io, ARRAY_SIZE(ak3918ev200_io));
}

static const char *const ak3918ev200_compat[] __initconst = {
	"anyka,qc07f", "anyka,ak3918ev200", NULL,
};

DT_MACHINE_START(AK3918EV200_DT, "Anyka AK3918EV200 experimental DT")
	.dt_compat = ak3918ev200_compat,
	.map_io = ak3918ev200_map_io,
MACHINE_END
