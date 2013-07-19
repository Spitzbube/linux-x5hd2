/******************************************************************************
 *    COPYRIGHT (C) 2013 Czyong. Hisilicon
 *    All rights reserved.
 * ***
 *    Create by Czyong 2013-03-15
 *
******************************************************************************/

#include <linux/completion.h>
#include <linux/kernel.h>
#include <asm/mach/resource.h>
#include <mach/clkdev.h>
#include <mach/io.h>
#include <asm/io.h>
#include <asm/setup.h>
#include <mach/platform.h>

/*****************************************************************************/

static struct resource hi3716x_hinfc504_resources[] = {
	{
		.name   = "base",
		.start  = 0x60010000,
		.end    = 0x60010000 + 0x100,
		.flags  = IORESOURCE_MEM,
	}, {
		.name   = "buffer",
		.start  = 0x24000000,
		.end    = 0x24000000 + 2048 + 128,
		.flags  = IORESOURCE_MEM,
	},
};
/*****************************************************************************/

struct device_resource hi3716x_hinfc504_device_resource = {
	.name           = "hinfc504",
	.resource       = hi3716x_hinfc504_resources,
	.num_resources  = ARRAY_SIZE(hi3716x_hinfc504_resources),
};
/*****************************************************************************/

