#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/module.h>
#include <linux/io.h>

/* hisilicon sata reg */
#define HI_SATA_PHY0_CTLL       0x54
#define HI_SATA_PHY0_CTLH       0x58
#define HI_SATA_PHY1_CTLL       0x60
#define HI_SATA_PHY1_CTLH       0x64
#define HI_SATA_DIS_CLK     (1 << 12)
#define HI_SATA_OOB_CTL         0x6c
#define HI_SATA_PORT_PHYCTL     0x74


#ifdef CONFIG_ARCH_GODBOX_V1
#include "ahci_sys_godbox_v1_defconfig.c"
#endif/*CONFIG_ARCH_GODBOX_V1*/

#ifdef CONFIG_ARCH_GODBOX
#include "ahci_sys_godbox_defconfig.c"
#endif/*CONFIG_ARCH_GODBOX*/

#ifdef CONFIG_ARCH_S40
#include "ahci_sys_s40_defconfig.c"
#endif/*CONFIG_ARCH_S40*/


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Hisilicon osdrv group");

static int phy_config = CONFIG_HI_SATA_PHY_CONFIG;
static int n_ports = CONFIG_HI_SATA_PORTS;
static int mode_3g = CONFIG_HI_SATA_MODE;

#ifdef MODULE
module_param(phy_config, uint, 0600);
MODULE_PARM_DESC(phy_config, "sata phy config (default:0x0e262709)");
module_param(n_ports, uint, 0600);
MODULE_PARM_DESC(n_ports, "sata port number (default:2)");
module_param(mode_3g, uint, 0600);
MODULE_PARM_DESC(mode_3g, "set sata 3G mode (0:1.5G(default);1:3G)");
#endif


void hi_sata_init(void __iomem *mmio)
{
#ifdef CONFIG_ARCH_S40
#ifdef CONFIG_S40_FPGA
	unsigned int tmp_val;
	int i;

	tmp_val = readl(HI_SATA3_CTRL);
	tmp_val |= HI_SATA3_BUS_SRST;
	writel(tmp_val, HI_SATA3_CTRL);

	msleep(100);

	tmp_val = readl(HI_SATA3_CTRL);
	tmp_val &=  ~HI_SATA3_BUS_SRST ;
	writel(tmp_val, HI_SATA3_CTRL);

	msleep(100);

	tmp_val = readl(HI_SATA3_CTRL);
	tmp_val |= HI_SATA3_CKO_ALIVE_SRST;
	writel(tmp_val, HI_SATA3_CTRL);

	msleep(100);

	tmp_val = readl(HI_SATA3_CTRL);
	tmp_val &= ~HI_SATA3_CKO_ALIVE_SRST ;
	writel(tmp_val, HI_SATA3_CTRL);

	msleep(100);

	for (i = 0; i < n_ports; i++)
		writel(phy_config, (mmio + 0x100 + i*0x80 + HI_SATA_PORT_PHYCTL));

	msleep(100);
#else
	// TODO: XXXX
	unsigned int tmp_val;

	/* Power on SATA disk */
	tmp_val = readl(IO_ADDRESS(0xF8A20008));
	tmp_val |= 1<<10;
	writel(tmp_val, IO_ADDRESS(0xF8A20008));
	msleep(10);

	/* Config SATA clock */
	writel(0x1f, IO_ADDRESS(0xF8A220A8));
	msleep(10);
	writel(0x1, IO_ADDRESS(0xF8A220AC));
	msleep(10);

	/* Config and reset the SATA PHY, SSC enabled */
	writel(0x49000679, IO_ADDRESS(0xF99000A0));
	msleep(10);
	writel(0x49000678, IO_ADDRESS(0xF99000A0));
	msleep(10);

	/* Config PHY controller register 1 */
	writel(0x345cb8, IO_ADDRESS(0xF9900148));
	msleep(10);

	/* Config PHY controller register 2, and reset SerDes lane */
	writel(0x00060545, IO_ADDRESS(0xF990014C));
	msleep(10);
	writel(0x00020545, IO_ADDRESS(0xF990014C));
	msleep(10);

	/* Data invert between phy and sata controller*/
	writel(0x8, IO_ADDRESS(0xF99000A4));
	msleep(10);

	/* Config Spin-up */
	writel(0x600000, IO_ADDRESS(0xF9900118));
	msleep(10);
	writel(0x600002, IO_ADDRESS(0xF9900118));
	msleep(10);

	/*
	 * Config SATA Port phy controller.
	 * To take effect for 0xF990014C, 
	 * we should force controller to 1.5G mode first
	 * and then force it to 6G mode.
	 */
	writel(0xE100000, IO_ADDRESS(0xF9900174));
	msleep(10);
	writel(0xE5A0000, IO_ADDRESS(0xF9900174));
	msleep(10);
	writel(0xE4A0000, IO_ADDRESS(0xF9900174));
	msleep(10);

	/* Force to 3G mode for HiS40, due to the bug of 6G mode  */
	writel(0xE250000, IO_ADDRESS(0xF9900174));
	msleep(10);

#endif
#else

	unsigned int tmp;
	int i;

	hi_sata_poweron();
	msleep(20);
	hi_sata_clk_open();
	hi_sata_phy_clk_sel();
	hi_sata_unreset();
	msleep(20);
	hi_sata_phy_unreset();
	msleep(20);
	tmp = readl(mmio + HI_SATA_PHY0_CTLH);
	tmp |= HI_SATA_DIS_CLK;
	writel(tmp, (mmio + HI_SATA_PHY0_CTLH));
	tmp = readl(mmio + HI_SATA_PHY1_CTLH);
	tmp |= HI_SATA_DIS_CLK;
	writel(tmp, (mmio + HI_SATA_PHY1_CTLH));
	if (mode_3g) {
		tmp = 0x8a0ec888;
		phy_config = CONFIG_HI_SATA_3G_PHY_CONFIG;
	} else {
		tmp = 0x8a0ec788;
	}
	writel(tmp, (mmio + HI_SATA_PHY0_CTLL));
	writel(0x2121, (mmio + HI_SATA_PHY0_CTLH));
	writel(tmp, (mmio + HI_SATA_PHY1_CTLL));
	writel(0x2121, (mmio + HI_SATA_PHY1_CTLH));
	writel(0x84060c15, (mmio + HI_SATA_OOB_CTL));
	for (i = 0; i < n_ports; i++)
		writel(phy_config, (mmio + 0x100 + i*0x80
					+ HI_SATA_PORT_PHYCTL));

	hi_sata_phy_reset();
	msleep(20);
	hi_sata_phy_unreset();
	msleep(20);
	hi_sata_clk_unreset();
	msleep(20);

#endif
}
EXPORT_SYMBOL(hi_sata_init);

void hi_sata_exit(void)
{
#if CONFIG_ARCH_S40
#ifdef CONFIG_S40_FPGA
	unsigned int tmp_val;

	tmp_val = readl(HI_SATA3_CTRL);
	tmp_val |= HI_SATA3_BUS_SRST;
	writel(tmp_val, HI_SATA3_CTRL);

	msleep(100);

	tmp_val = readl(HI_SATA3_CTRL);
	tmp_val |= HI_SATA3_CKO_ALIVE_SRST;
	writel(tmp_val, HI_SATA3_CTRL);

	msleep(100);
#else
	//TODO: add in ASIC
#endif
#else

	hi_sata_phy_reset();
	msleep(20);
	hi_sata_reset();
	msleep(20);
	hi_sata_clk_reset();
	msleep(20);
	hi_sata_clk_close();
	hi_sata_poweroff();
	msleep(20);

#endif

}
EXPORT_SYMBOL(hi_sata_exit);
