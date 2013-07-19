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
#include <mach/platform.h>
#include <linux/io.h>
#include <mach/io.h>

/*****************************************************************************/

unsigned int get_clk_posdiv(unsigned long osc)
{
	unsigned long regbase = IO_ADDRESS(REG_BASE_SCTL);
	unsigned long apll_fbdiv;
	unsigned long apll_frac;
	unsigned long apll_refdiv;
	unsigned long apll_postdiv1;
	unsigned long apll_postdiv2;
	unsigned long FOUTVCO;
	unsigned long regval;
	unsigned long FOUTPOSTDIV;

	static unsigned int clk_postdiv = 0;
	if (clk_postdiv)
		return clk_postdiv;

	regval = readl(regbase + REG_SC_APLLFREQCTRL0);
	apll_postdiv2 = ((regval >> 27) & 0x07);
	apll_postdiv1 = ((regval >> 24) & 0x07);
	apll_frac  = (regval & 0xFFFFFF);

	regval = readl(regbase + REG_SC_APLLFREQCTRL1);
	apll_refdiv = ((regval >> 12) & 0x3F);
	apll_fbdiv  = (regval & 0xFFF);

	FOUTVCO = (osc / apll_refdiv) * (apll_fbdiv + (apll_frac >> 24));
	FOUTPOSTDIV = FOUTVCO / (apll_postdiv2 * apll_postdiv1);

	clk_postdiv = FOUTPOSTDIV;

	return clk_postdiv;
}
/*****************************************************************************/

void get_hi3716xv100_clock(unsigned int *cpu, unsigned int *timer)
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
		clk_cpu = ((get_clk_posdiv(FREF) >> 2) * 3);
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