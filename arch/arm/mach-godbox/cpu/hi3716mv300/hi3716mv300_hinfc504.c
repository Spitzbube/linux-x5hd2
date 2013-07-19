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
#include <mach/io.h>
#include <asm/io.h>
#include <asm/setup.h>
#include <mach/platform.h>

/*****************************************************************************/

#define PERI_CRG30                                  (0x00B8)
#define PERI_CRG30_CLK_EN                           (1U << 8)
#define PERI_CRG30_CLK_SEL_99M                      (1U << 16)

static int hi3716mv300_hinfc504_enable(struct clk *clk)
{
	unsigned int regval;
	unsigned int base = IO_ADDRESS(REG_BASE_CRG);

	regval = readl(base + PERI_CRG30);
	regval |= (PERI_CRG30_CLK_EN | PERI_CRG30_CLK_SEL_99M);
	writel(regval, (base + PERI_CRG30));

	return 0;
}
/*****************************************************************************/

static int hi3716mv300_hinfc504_disable(struct clk *clk)
{
	unsigned int regval;
	unsigned int base = IO_ADDRESS(REG_BASE_CRG);

	regval = readl(base + PERI_CRG30);
	regval &= ~(PERI_CRG30_CLK_EN);
	writel(regval, (base + PERI_CRG30));

	return 0;
}
/*****************************************************************************/

static struct clk_ops hi3716mv300_hinfc504_clk_ops = {
	.enable = hi3716mv300_hinfc504_enable,
	.disable = hi3716mv300_hinfc504_disable,
	.unprepare = NULL,
	.prepare = NULL,
	.set_rate = NULL,
	.get_rate = NULL,
	.round_rate = NULL,
};
/*****************************************************************************/

struct clk hi3716mv300_hinfc504_clk = {
	.ops  = &hi3716mv300_hinfc504_clk_ops,
};
