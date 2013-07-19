/******************************************************************************
 *    COPYRIGHT (C) 2013 Czyong. Hisilicon
 *    All rights reserved.
 * ***
 *    Create by Czyong 2013-02-07
 *
******************************************************************************/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/string.h>
#include <mach/platform.h>
#include <linux/io.h>
#include <mach/io.h>
#include <asm/bug.h>

#include "cpu.h"

/*****************************************************************************/

extern struct cpu_info hi3716mv300_cpu_info;
extern struct cpu_info hi3712v100_cpu_info;
extern struct cpu_info hi3716mv200_cpu_info;
extern struct cpu_info hi3716cv100_cpu_info;
extern struct cpu_info hi3716mv100_cpu_info;
extern struct cpu_info hi3716hv100_cpu_info;

static struct cpu_info *godbox_cpu_infos[] = {
	&hi3716mv300_cpu_info,
	&hi3712v100_cpu_info,
	&hi3716mv200_cpu_info,
	&hi3716cv100_cpu_info,
	&hi3716mv100_cpu_info,
	&hi3716hv100_cpu_info,
	NULL,
};

static struct cpu_info *godbox_cpu_info = NULL;
/******************************************************************************/

static long long get_chipid_reg(void)
{
	long long chipid = 0;
	unsigned int subchipid;
	unsigned int regval;

	regval = readl(IO_ADDRESS(REG_BASE_PERI_CTRL_START_MODE));
	chipid = (long long)((regval >> 14) & 0x1F);

	regval = readl(IO_ADDRESS(REG_BASE_SCTL + REG_SC_SYSID0));
	subchipid = (regval & 0xFF);
	regval = readl(IO_ADDRESS(REG_BASE_SCTL + REG_SC_SYSID1));
	subchipid |= ((regval & 0xFF) << 8);
	regval = readl(IO_ADDRESS(REG_BASE_SCTL + REG_SC_SYSID2));
	subchipid |= ((regval & 0xFF) << 16);
	regval = readl(IO_ADDRESS(REG_BASE_SCTL + REG_SC_SYSID3));
	subchipid |= ((regval & 0xFF) << 24);

	chipid = ((chipid << 32) | (long long)subchipid);

	return chipid;
}
/*****************************************************************************/

void __init godbox_cpu_init(void)
{
	struct cpu_info **info;
	unsigned long long chipid = get_chipid_reg();

	if (godbox_cpu_info) {
		printk("CPU repeat initialized\n");
		BUG();
		return;
	}

	for (info = godbox_cpu_infos; (*info); info++) {
		if (((*info)->chipid & (*info)->chipid_mask)
		    == (chipid & (*info)->chipid_mask)) {
			godbox_cpu_info = (*info);
			break;
		}
	}

	if (!godbox_cpu_info) {
		printk("Can't find CPU information.\n");
		BUG();
		return;
	}

	godbox_cpu_info->init(godbox_cpu_info);

	pr_notice("CPU: %s %s\n",
		godbox_cpu_info->name,
		godbox_cpu_info->cpuversion);
}
/*****************************************************************************/

long long get_chipid(void)
{
	return  (godbox_cpu_info->chipid & godbox_cpu_info->chipid_mask);
}
EXPORT_SYMBOL(get_chipid);
/*****************************************************************************/

void get_clock(unsigned int *cpu, unsigned int *timer)
{
	if (cpu)
		(*cpu) = godbox_cpu_info->clk_cpu;
	if (timer)
		(*timer) = godbox_cpu_info->clk_timer;
}
EXPORT_SYMBOL(get_clock);
/*****************************************************************************/

const char *get_cpu_name(void)
{
	return godbox_cpu_info->name;
}
EXPORT_SYMBOL(get_cpu_name);
/******************************************************************************/

const char * get_cpu_version(void)
{
	return godbox_cpu_info->cpuversion;
}
EXPORT_SYMBOL(get_cpu_version);
/******************************************************************************/

int find_cpu_resource(const char *name, struct resource **resource,
		      int *num_resources)
{
	struct cpu_info *info = godbox_cpu_info;
	struct device_resource **res = info->resource;

	for (; res; res++) {
		if (!strcmp((*res)->name, name)) {
			if (resource)
				(*resource) = (*res)->resource;
			if (num_resources)
				(*num_resources) = (*res)->num_resources;
			return 0;
		}
	}
	return -ENODEV;
}
EXPORT_SYMBOL(find_cpu_resource);
