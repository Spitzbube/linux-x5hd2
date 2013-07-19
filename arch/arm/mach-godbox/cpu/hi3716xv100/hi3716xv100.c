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

static struct clk_lookup hi3716xv100_lookups[] = {
	{
		.dev_id = "sp804",
		.clk    = &sp804_clk,
	}
};
/*****************************************************************************/

static void hi3716xv100_cpu_init(struct cpu_info *info)
{
	get_hi3716xv100_clock(&info->clk_cpu, &info->clk_timer);

	sp804_clk.rate = info->clk_timer;

	info->cpuversion = "";

	clkdev_add_table(hi3716xv100_lookups,
		ARRAY_SIZE(hi3716xv100_lookups));
}
/*****************************************************************************/

struct cpu_info hi3716cv100_cpu_info =
{
	.name = "Hi3716Cv100",
	.chipid = _HI3716C_V100,
	.chipid_mask = _HI3716X_MASK,
	.init = hi3716xv100_cpu_init,
};
struct cpu_info hi3716mv100_cpu_info =
{
	.name = "Hi3716Mv100",
	.chipid = _HI3716M_V100,
	.chipid_mask = _HI3716X_MASK,
	.init = hi3716xv100_cpu_init,
};
struct cpu_info hi3716hv100_cpu_info =
{
	.name = "Hi3716Hv100",
	.chipid = _HI3716H_V100,
	.chipid_mask = _HI3716X_MASK,
	.init = hi3716xv100_cpu_init,
};