/******************************************************************************
 *    COPYRIGHT (C) 2013 Czyong. Hisilicon
 *    All rights reserved.
 * ***
 *    Create by Czyong 2013-02-07
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

#include "../cpu.h"

/*****************************************************************************/

extern struct device_resource hi3716mv300_hinfc504_device_resource;

static struct device_resource *hi3716mv300_device_resource[] = {
	&hi3716mv300_hinfc504_device_resource,
	NULL,
};
/*****************************************************************************/
extern struct clk hi3716mv300_hinfc504_clk;


static struct clk sp804_clk = {
	.rate = 0,
};

static struct clk_lookup hi3716mv300_lookups[] = {
	{
		.dev_id = "hinfc504",
		.clk    = &hi3716mv300_hinfc504_clk,
	}, {
		.dev_id = "sp804",
		.clk    = &sp804_clk,
	}
};
/*****************************************************************************/

static void get_hi3716mv300_clock(unsigned int *cpu, unsigned int *timer)
{
	unsigned long FREF = 28800000;

	static unsigned int clk_cpu = 0;
	static unsigned int clk_axi = 0;
	static unsigned int clk_l2cache = 0;
	static unsigned int clk_ahb = 0;
	static unsigned int clk_apb = 0;

	if (clk_cpu)
		goto exit;

	clk_cpu = (get_clk_posdiv(FREF) >> 1);
	clk_l2cache = clk_cpu;
	clk_axi = (clk_l2cache >> 1);
	clk_ahb = (clk_axi << 1) / 3;
	clk_apb = (clk_ahb >> 1);

exit:
	if (cpu)
		(*cpu) = clk_cpu;
	if (timer)
		(*timer) = clk_apb;
}
/*****************************************************************************/

static void hi3716mv300_cpu_init(struct cpu_info *info)
{
	get_hi3716mv300_clock(&info->clk_cpu, &info->clk_timer);

	sp804_clk.rate = info->clk_timer;

	info->cpuversion = "";

	clkdev_add_table(hi3716mv300_lookups,
		ARRAY_SIZE(hi3716mv300_lookups));
}
/*****************************************************************************/

struct cpu_info hi3716mv300_cpu_info =
{
	.name = "Hi3716Mv300",
	.chipid = _HI3716M_V300,
	.chipid_mask = _HI3716X_MASK,
	.resource = hi3716mv300_device_resource,
	.init = hi3716mv300_cpu_init,
};
