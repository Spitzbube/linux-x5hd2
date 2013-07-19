/******************************************************************************
 *    COPYRIGHT (C) 2013 Czyong. Hisilicon
 *    All rights reserved.
 * ***
 *    Create by Czyong 2013-02-06
 *
******************************************************************************/

#ifndef HISFC350_OSH
#define HISFC350_OSH
/******************************************************************************/

#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <asm/setup.h>
#include <asm/io.h>
#include <mach/cpu-info.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/string_helpers.h>

#include "../spi_ids.h"

/*****************************************************************************/

#define MTD_TO_HOST(_mtd)               ((struct hisfc_host *)(_mtd))

#define hisfc_read(_host, _reg) \
	readl((unsigned)((char *)_host->regbase + (_reg)))

#define hisfc_write(_host, _reg, _value) \
	writel((unsigned)(_value), (unsigned)((char *)_host->regbase + (_reg)))

#define HISFC350_CMD_WAIT_CPU_FINISH(_host) do {\
	unsigned int timeout = 0x10000000; \
	while (((hisfc_read((_host), HISFC350_CMD_CONFIG) \
		& HISFC350_CMD_CONFIG_START)) && timeout) \
		--timeout; \
	if (!timeout) \
		PR_BUG("cmd wait cpu finish timeout\n"); \
} while (0)

#define HISFC350_DMA_WAIT_CPU_FINISH(_host) do {\
	unsigned int timeout = 0x10000000; \
	while (((hisfc_read((_host), HISFC350_BUS_DMA_CTRL) \
		& HISFC350_BUS_DMA_CTRL_START)) && timeout) { \
		--timeout; cond_resched(); }\
	if (!timeout) \
		PR_BUG("dma wait cpu finish timeout\n"); \
} while (0)

/*****************************************************************************/
#if 1
#  define DBG_MSG(_fmt, arg...)
#else
#  define DBG_MSG(_fmt, arg...) \
	printk(KERN_INFO "%s(%d): " _fmt, __FILE__, __LINE__, ##arg);
#endif

#define PR_MSG(_fmt, arg...) \
	printk(_fmt, ##arg)

#define PR_WARN(_fmt, arg...) \
	printk(KERN_INFO "%s(%d): " _fmt, __FILE__, __LINE__, ##arg);

#define PR_BUG(fmt, args...) do {\
	printk(KERN_ERR "%s(%d): BUG: " fmt, __FILE__, __LINE__, ##args); \
	while(1); \
} while (0)
/*****************************************************************************/

#ifndef NULL
#  define NULL         (void*)0
#endif

/******************************************************************************/
#endif /* HISFC350_OSH */
