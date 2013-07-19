/******************************************************************************
 *    COPYRIGHT (C) 2013 Czyong. Hisilicon
 *    All rights reserved.
 * ***
 *    Create by Czyong 2013-03-15
 *
******************************************************************************/

#include <linux/err.h>
#include <linux/completion.h>
#include <linux/kernel.h>
#include <asm/mach/resource.h>
#include <mach/clkdev.h>
#include <asm/clkdev.h>
#include <asm/system.h>
#include <linux/clkdev.h>
#include <mach/platform.h>
#include <linux/io.h>
#include <mach/io.h>

#include "../cpu.h"
#include "../common/hi3716x_clock_div.h"

static struct clk sp804_clk = {
	.rate = 0,
};

static struct clk_lookup hi3716mv200_lookups[] = {
	{
		.dev_id = "sp804",
		.clk    = &sp804_clk,
	}
};
/*****************************************************************************/

static void get_hi3716mv200_clock(unsigned int *cpu, unsigned int *timer)
{
	unsigned long regval;
	unsigned long FREF = 24000000;
	unsigned long regbase = IO_ADDRESS(REG_BASE_SCTL);

	static unsigned int clk_cpu = 0;
	static unsigned int clk_axi = 0;
	static unsigned int clk_l2cache = 0;
	static unsigned int clk_ahb = 0;
	static unsigned int clk_apb = 0;

	if (clk_cpu)
		goto exit;

	regval = readl(regbase + REG_SC_CTRL);
	regval = ((regval >> 12) & 0x03);

	if (regval == 0x00) { /* 2:1 */
		clk_cpu = get_clk_posdiv(FREF);
		clk_l2cache = clk_cpu;
		clk_axi = (clk_l2cache >> 1);
	} else if (regval == 0x3) { /* 3:2 */
		unsigned int posdiv = get_clk_posdiv(FREF);
		if (posdiv == 800000000) /* logic stipulate */
			posdiv = 1200000000;
		clk_cpu = (posdiv >> 1);
		clk_l2cache = (clk_cpu << 1) / 3;
		clk_axi = clk_l2cache;
	} else {
		/* BUG. */
	}

	clk_ahb = (clk_axi >> 1);
	clk_apb = (clk_ahb >> 1);

exit:
	if (cpu)
		(*cpu) = clk_cpu;
	if (timer)
		(*timer) = clk_apb;
}
/*****************************************************************************/

static void hi3716mv200_cpu_init(struct cpu_info *info)
{
	get_hi3716mv200_clock(&info->clk_cpu, &info->clk_timer);

	sp804_clk.rate = info->clk_timer;

	info->cpuversion = "";

	clkdev_add_table(hi3716mv200_lookups,
		ARRAY_SIZE(hi3716mv200_lookups));
}
/*****************************************************************************/

struct cpu_info hi3716mv200_cpu_info =
{
	.name = "Hi3716Mv200",
	.chipid = _HI3716M_V200,
	.chipid_mask = _HI3716X_MASK,
	.init = hi3716mv200_cpu_init,
};

