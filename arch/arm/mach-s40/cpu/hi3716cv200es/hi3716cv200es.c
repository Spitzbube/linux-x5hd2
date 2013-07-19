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
#include <asm/io.h>

#include "mach/io.h"
#include "../cpu.h"
#include "../common/hi3716cv200es_hinfc504.h"

/*****************************************************************************/

static struct device_resource *hi3716cv200es_device_resource[] = {
	&hi3716cv200es_hinfc504_device_resource,
	NULL,
};
/*****************************************************************************/
extern struct clk hi3716cv200es_hinfc504_clk;

static struct clk_lookup hi3716cv200es_lookups[] = {
	{
		.dev_id = "hinfc504",
		.clk    = &hi3716cv200es_hinfc504_clk,
	}, 
};
/*****************************************************************************/

static void hi3716cv200es_cpu_init(struct cpu_info *info)
{
	info->clk_cpu    = 800000000;
	info->clk_timer  = 24000000;
	info->cpuversion = "";
	clkdev_add_table(hi3716cv200es_lookups,
		ARRAY_SIZE(hi3716cv200es_lookups));

}
/*****************************************************************************/

struct cpu_info hi3716cv200es_cpu_info =
{
	.name = "Hi3716cv200es",
	.chipid = _HI3716CV200ES,
	.chipid_mask = _HI3716CV200ES_MASK,
	.resource = hi3716cv200es_device_resource,
	.init = hi3716cv200es_cpu_init,
};

struct cpu_info hi3716cv200_cpu_info =
{
	.name = "Hi3716cv200",
	.chipid = _HI3716CV200,
	.chipid_mask = _HI3716CV200_MASK,
	.resource = hi3716cv200es_device_resource,
	.init = hi3716cv200es_cpu_init,
};
