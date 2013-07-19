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

#define HI3712_PERI_CRG30                           (0x00B8)
#define HI3712_PERI_CRG30_CLK_EN                    (1U << 8)
#define HI3712_PERI_CRG30_CLK_SEL_215M              (1U << 1) /* 214.2MHz  */

/*****************************************************************************/

static int hi3712v100_hinfc504_enable(struct clk *clk)
{
	unsigned int regval;
	unsigned int base = IO_ADDRESS(REG_BASE_CRG);

	regval = readl(base + HI3712_PERI_CRG30);
	regval |= (HI3712_PERI_CRG30_CLK_EN | HI3712_PERI_CRG30_CLK_SEL_215M);
	writel(regval, (base + HI3712_PERI_CRG30));

	return 0;
}
/*****************************************************************************/

static int hi3712v100_hinfc504_disable(struct clk *clk)
{
	unsigned int regval;
	unsigned int base = IO_ADDRESS(REG_BASE_CRG);

	regval = readl(base + HI3712_PERI_CRG30);
	regval &= ~(HI3712_PERI_CRG30_CLK_EN);
	writel(regval, (base + HI3712_PERI_CRG30));

	return 0;
}
/*****************************************************************************/

static struct clk_ops hi3712v100_hinfc504_clk_ops = {
	.enable  = hi3712v100_hinfc504_enable,
	.disable = hi3712v100_hinfc504_disable,
};
/*****************************************************************************/

struct clk hi3712v100_hinfc504_clk = {
	.ops  = &hi3712v100_hinfc504_clk_ops,
};
