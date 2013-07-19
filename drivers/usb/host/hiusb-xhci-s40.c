#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/types.h>

#include "xhci.h"
#include "hixhci.h"

MODULE_LICENSE("Dual MIT/GPL");

#define PERI_CRG44            IO_ADDRESS(0xF8A22000 + 0xb0)
#define USB3_VCC_SRST_REQ     (1<<13)
#define USB3_VAUX_SRST_REQ    (1<<12)
#define PERI_USB5             IO_ADDRESS(0xF8A20000 + 0x134)

#define REG_GCTL              0xc110
#define BIT_SOFT_RESET        (0x1 << 11)
#define BIT_PORT_DIR_HOST     (~(0x1 << 13))|(0x1 <<12)

#define REG_GUSB3_PIPECTL     0xC2C0
#define BIT_PHY_SOFT_RESET    (0x1 << 31)
#define BIT_TX_MARGIN0        (0x1 << 3)

#define REG_GUSB2_CFG         0xC200
#define BIT_UTMI_ULPI         (0x1 << 4)
#define BIT_UTMI_8_16         (0x1 << 3)

static void hiusb_start_hcd(void __iomem *base)
{
	unsigned long flags;
	unsigned int reg;

	local_irq_save(flags);

		/*step 1: cancel reset*/
		reg = readl(PERI_CRG44);
		writel(reg&(~USB3_VCC_SRST_REQ), PERI_CRG44);
		udelay(200);

		/*step 2: config port power */
		reg = readl(PERI_USB5);
		writel(0X840, PERI_USB5);
		udelay(200);

#ifdef CONFIG_S40_FPGA
		/*step 3: USB2 PHY chose utmi 16bit interface */
		reg = readl(base + REG_GUSB2_CFG);
		writel(reg&(~BIT_UTMI_ULPI)|BIT_UTMI_8_16, base + REG_GUSB2_CFG);
		wmb();
		mdelay(20);
#else
		/*step 3: USB2 PHY chose utmi 8bit interface */
		reg = readl(base + REG_GUSB2_CFG);
		writel(reg&(~BIT_UTMI_ULPI)|(~BIT_UTMI_8_16), base + REG_GUSB2_CFG);
		wmb();
		mdelay(20);
#endif

		/*step 4: core soft reset */
		reg = readl(base + REG_GCTL);
		writel(reg | BIT_SOFT_RESET, base + REG_GCTL);

		/* usb3 phy soft reset */
		reg = readl(base + REG_GUSB3_PIPECTL);
		reg |= BIT_PHY_SOFT_RESET;
		writel(reg, base + REG_GUSB3_PIPECTL);
		mdelay(100);
		reg = readl(base + REG_GUSB3_PIPECTL);
		reg &= ~(BIT_PHY_SOFT_RESET);
		writel(reg, base + REG_GUSB3_PIPECTL);
		mdelay(500);

		/* cancel core soft reset */
		reg = readl(base + REG_GCTL);
		writel(reg & (~BIT_SOFT_RESET), base + REG_GCTL);
		mdelay(500);

		/*setp 5: config port direction fro host */
		reg = readl(base + REG_GCTL);
		writel(reg & BIT_PORT_DIR_HOST, base + REG_GCTL);
		mdelay(500);

		reg = readl(PERI_CRG44);
		writel(reg|0x1ff, PERI_CRG44);
		udelay(200);

	local_irq_restore(flags);
}

static void hiusb_stop_hcd(void)
{
	unsigned long flags;
	unsigned int reg;

	local_irq_save(flags);

	reg = readl(PERI_CRG44);
	writel(reg|(USB3_VCC_SRST_REQ), PERI_CRG44);
	udelay(200);

	local_irq_restore(flags);
}

static struct hiusb_plat_data  hiusb_data = {
	.start_hcd = hiusb_start_hcd,
	.stop_hcd = hiusb_stop_hcd,
};

static struct resource hiusb_xhci_res[] = {
	[0] = {
		.start	= CONFIG_HIUSB_XHCI_IOBASE,
		.end	= CONFIG_HIUSB_XHCI_IOBASE
			+ CONFIG_HIUSB_XHCI_IOSIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= CONFIG_HIUSB_XHCI_IRQNUM,
		.end	= CONFIG_HIUSB_XHCI_IRQNUM,
		.flags	= IORESOURCE_IRQ,
	},
};

static u64 usb_dmamask = DMA_BIT_MASK(32);

static struct platform_device hiusb_xhci_platdev = {
	.name = "xhci-hcd",
	.id = -1,
	.dev = {
		.init_name = "hiusb3.0",
		.platform_data = &hiusb_data,
		.dma_mask = &usb_dmamask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
	},
	.num_resources = ARRAY_SIZE(hiusb_xhci_res),
	.resource = hiusb_xhci_res,
};

static int __init xhci_device_init(void)
{
	if (usb_disabled())
		return -ENODEV;
	return platform_device_register(&hiusb_xhci_platdev);
}

static void __exit xhci_device_exit(void)
{
	platform_device_unregister(&hiusb_xhci_platdev);
}

module_init(xhci_device_init);
module_exit(xhci_device_exit);
