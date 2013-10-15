#ifdef CONFIG_ARCH_S40

#include <mach/cpu-info.h>
#include "hieth.h"
#include "mdio.h"
#include "mac.h"
#include "ctrl.h"
#include "glb.h"

static void hieth_set_regbit(unsigned long addr, int bit, int shift)
{
	unsigned long reg;
	reg = readl(addr);
	bit = bit ? 1 : 0;
	reg &= ~(1<<shift);
	reg |= bit<<shift;
	writel(reg, addr);
}

static void hieth_reset(int rst)
{
//TODO:XXX
}

static inline void hieth_clk_ena(void)
{
//TODO:XXX
}

static inline void hieth_clk_dis(void)
{
//TODO:XXX
}

static void hieth_phy_reset(void)
{
//TODO:XXX
}

static void hieth_phy_suspend(void)
{
	/* FIXME: phy power down */
}

static void hieth_phy_resume(void)
{
	/* FIXME: phy power up */
	hieth_phy_reset();
}

static void hieth_funsel_config(void)
{
}

static void hieth_funsel_restore(void)
{
}

int hieth_port_reset(struct hieth_netdev_local *ld, int port)
{

	hieth_assert(port == ld->port);

	hieth_writel_bits(ld, 1, GLB_SOFT_RESET, HI3712_BITS_ETH_SOFT_RESET);
	msleep(1);
	hieth_writel_bits(ld, 0, GLB_SOFT_RESET, HI3712_BITS_ETH_SOFT_RESET);
	msleep(1);
	hieth_writel_bits(ld, 1, GLB_SOFT_RESET, HI3712_BITS_ETH_SOFT_RESET);
	msleep(1);
	hieth_writel_bits(ld, 0, GLB_SOFT_RESET, HI3712_BITS_ETH_SOFT_RESET);

	return 0;
}


#endif/*CONFIG_ARCH_GODBOX_V1*/

/* vim: set ts=8 sw=8 tw=78: */
