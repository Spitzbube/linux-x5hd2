#include <linux/phy.h>
#include "../higmac.h"
#include "autoeee.h"

void init_autoeee(struct higmac_netdev_local *ld, int link_stat)
{
	int phy_addr = ld->phy->addr;
	int phy_id = 0, eee_available, lp_eee_capable, v;
	struct phy_info *phy_info;

	if (ld->eee_init)
		goto eee_init;

	v = get_phy_id(ld->mii_bus_using, phy_addr, &phy_id);
	if (v) {
		pr_err("mii_bus:%s get phy(addr=%d) failed!\n",
				ld->mii_bus_using->name, phy_addr);
		return;
	}

	phy_info = phy_search_ids(phy_id);
	if (phy_info) {
		eee_available = phy_info->eee_available;
		if (DEBUG)
			pr_info("fit phy_id:0x%x, phy_name:%s, eee:%d\n",
				phy_info->phy_id, phy_info->name,
				eee_available);

		if (!eee_available)
			goto not_support;

		ld->eee_init = phy_info->eee_init;
eee_init:
		lp_eee_capable = ld->eee_init(ld->phy);
		if (link_stat & HIGMAC_LINKED) {
			if (lp_eee_capable & link_stat) {
				/* EEE_1us: 0x7c for 125M */
				writel(0x7c, ld->gmac_iobase + EEE_TIME_CLK_CNT);

				v = readl(ld->gmac_iobase + EEE_LINK_STATUS);
				v |= 0x3 << 1;/* auto EEE and ... */
				v |= BIT_PHY_LINK_STATUS;/* phy linkup */
				writel(v, ld->gmac_iobase + EEE_LINK_STATUS);

				v = readl(ld->gmac_iobase + EEE_ENABLE);
				v |= BIT_EEE_ENABLE;/* enable EEE */
				writel(v, ld->gmac_iobase + EEE_ENABLE);

				if (DEBUG)
					pr_info("enter auto-EEE mode\n");
				return;
			} else {
				if (DEBUG)
					pr_info("link partner not support EEE\n");
				return;
			}
		} else {
			v = readl(ld->gmac_iobase + EEE_LINK_STATUS);
			v &= ~(BIT_PHY_LINK_STATUS);/* phy linkdown */
			writel(v, ld->gmac_iobase + EEE_LINK_STATUS);
			return;
		}
	}

not_support:
	ld->eee_init = NULL;
	if (DEBUG)
		pr_info("non-EEE mode\n");
}

void eee_phy_linkdown(struct higmac_netdev_local *ld)
{
	int v = readl(ld->gmac_iobase + EEE_LINK_STATUS);
	/* update phy link state */
	v &= ~BIT_PHY_LINK_STATUS;
	writel(v, ld->gmac_iobase + EEE_LINK_STATUS);
}

void eee_phy_linkup(struct higmac_netdev_local *ld)
{
	int v = readl(ld->gmac_iobase + EEE_LINK_STATUS);
	/* update phy link state */
	v |= BIT_PHY_LINK_STATUS;
	writel(v, ld->gmac_iobase + EEE_LINK_STATUS);
}
