/******************************************************************************
 *    COPYRIGHT (C) 2013 llui. Hisilicon
 *    All rights reserved.
 * ***
 *    Create by llui 2013-09-03
 *
******************************************************************************/
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/ktime.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/spinlock.h>
#include <asm/byteorder.h>
#include <linux/io.h>
#include <asm/system.h>
#include <asm/unaligned.h>
#include <mach/hardware.h>

#define PERI_USB0                      __io_address(0xF8A20000 + 0x120)
#define DWC_OTG_SWITCH                 (1 << 28)

static int usbdev_connect = 0;
static int usbhost_connect = 0;

/*  CPU is in host status, check if there is device connectted. */
void set_usbhost_connect(int index, int online)
{
	if (index == 0)
		usbhost_connect = online;
}
EXPORT_SYMBOL(set_usbhost_connect);

/*  CPU is in device status, check if there is host connectted. */
void set_usbdev_connect(int index, int online)
{
	if (index == 0)
		usbdev_connect = online;
}
EXPORT_SYMBOL(set_usbdev_connect);

int do_usbotg(void)
{
	int reg;

	reg = readl(PERI_USB0);

	if (reg & DWC_OTG_SWITCH) {
		/* CPU is in device status */
		if (usbdev_connect)
			return 0;
		reg &= ~DWC_OTG_SWITCH;
	} else {
		/* CPU is in host status */
		if (usbhost_connect)
			return 0;
		reg |= DWC_OTG_SWITCH;
	}

	writel(reg, PERI_USB0);

	return 0;
}
EXPORT_SYMBOL(do_usbotg);
