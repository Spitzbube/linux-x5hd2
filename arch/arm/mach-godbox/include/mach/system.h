#ifndef __ASM_ARCH_SYSTEM_H
#define __ASM_ARCH_SYSTEM_H

#include <linux/io.h>
#include <mach/platform.h>
#include <mach/cpu-info.h>

static inline void arch_idle(void)
{
	/*
	 * This should do all the clock switching
	 * and wait for interrupt tricks
	 */
	cpu_do_idle();
}

static inline void arch_reset(char mode,const char *cmd)
{
	unsigned long long chipid = get_chipid();

	local_irq_disable();
	if (chipid != _HI3712_V100) {
		if (chipid == _HI3716M_V300) {
			/*
			* We should config the follow gpio before reset
			* The config turn the pin to "start config" mode.
			*/
			writel(0x02, IO_ADDRESS(0x1020301C));
			writel(0x01, IO_ADDRESS(0x10203030));
			writel(0x01, IO_ADDRESS(0x10203034));
			writel(0x01, IO_ADDRESS(0x1020304C));
			writel(0x01, IO_ADDRESS(0x10203050));
			writel(0x01, IO_ADDRESS(0x1020306C));
			writel(0x03, IO_ADDRESS(0x10203074));
		} else {
			writel(0x01, IO_ADDRESS(0x10203030));
			writel(0x01, IO_ADDRESS(0x10203034));
			writel(0x01, IO_ADDRESS(0x1020304C));
			writel(0x01, IO_ADDRESS(0x10203050));
			writel(0x01, IO_ADDRESS(0x1020306C));
			writel(0x03, IO_ADDRESS(0x10203074));
		}
	}

	/* unclock wdg */
	writel(0x1ACCE551,  IO_ADDRESS(REG_BASE_WDG + 0xc00));
	/* wdg load value */
	writel(0x00000100,  IO_ADDRESS(REG_BASE_WDG + 0x0));
	/* bit0: int enable bit1: reboot enable */
	writel(0x00000003,  IO_ADDRESS(REG_BASE_WDG + 0x8));

	while (1)
		;
}
#endif
