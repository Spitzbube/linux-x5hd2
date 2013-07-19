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
#include <mach/platform.h>
#include <mach/io.h>
#include <asm/io.h>

#include "../cpu.h"
#include "../common/hi3716x_hinfc504.h"

/*****************************************************************************/

static struct device_resource *hi3712v100_device_resource[] = {
	&hi3716x_hinfc504_device_resource,
	NULL,
};
/*****************************************************************************/
extern struct clk hi3712v100_hinfc504_clk;

static struct clk sp804_clk = {
	.rate = HI3712_OSC_FREQ,
};

static struct clk_lookup hi3712v100_lookups[] = {
	{
		.dev_id = "hinfc504",
		.clk    = &hi3712v100_hinfc504_clk,
	}, {
		.dev_id = "sp804",
		.clk    = &sp804_clk,
	}
};
/*****************************************************************************/

static char *hi3712v100_get_cpu_version(void)
{
	unsigned int regval;
	static char *verstr[] = {"A", "B" , "C", "D", "E", "F", "G", "I"};

	regval = readl(IO_ADDRESS(REG_BASE_PERI_CTRL_START_MODE));
	regval = ((regval >> 14) && 0x07);

	return verstr[regval];
}
/*****************************************************************************/

static void hi3712v100_cpu_init(struct cpu_info *info)
{
	info->cpuversion = hi3712v100_get_cpu_version();

	clkdev_add_table(hi3712v100_lookups,
		ARRAY_SIZE(hi3712v100_lookups));
}
/*****************************************************************************/

struct cpu_info hi3712v100_cpu_info =
{
	.name = "Hi3712v100",
	.chipid = _HI3712_V100,
	.chipid_mask = _HI3712_MASK,
	.resource = hi3712v100_device_resource,
	.init = hi3712v100_cpu_init,
};
