#include <linux/init.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/amba/bus.h>
#include <linux/amba/clcd.h>
#include <linux/clocksource.h>
#include <linux/clockchips.h>
#include <linux/cnt32_to_63.h>
#include <linux/io.h>
#include <linux/clkdev.h>

#include <asm/clkdev.h>
#include <asm/system.h>
#include <asm/irq.h>
#include <asm/leds.h>
#include <asm/hardware/arm_timer.h>
#include <asm/hardware/gic.h>
#include <asm/hardware/vic.h>
#include <asm/mach-types.h>

#include <asm/mach/arch.h>
#include <asm/mach/flash.h>
#include <asm/mach/irq.h>
#include <asm/mach/time.h>
#include <asm/mach/map.h>
#include <asm/sched_clock.h>
#include <mach/time.h>
#include <mach/hardware.h>
#include <mach/early-debug.h>
#include <mach/irqs.h>
#include <linux/bootmem.h>
#include <mach/cpu-info.h>
#include <mach/system.h>
#include <linux/delay.h>
#include <asm/hardware/timer-sp.h>

/*****************************************************************************/

extern void __init godbox_cpu_init(void);

void __init godbox_gic_init_irq(void)
{
	edb_trace();
	gic_init(0, HISI_GIC_IRQ_START,
		(void __iomem *) CFG_GIC_DIST_BASE,
		(void __iomem *) CFG_GIC_CPU_BASE);
}

static struct map_desc godbox_io_desc[] __initdata = {
	/* GODBOX_IOCH1 */
	{
		.virtual	= GODBOX_IOCH1_VIRT,
		.pfn		= __phys_to_pfn(GODBOX_IOCH1_PHYS),
		.length		= GODBOX_IOCH1_SIZE,
		.type		= MT_DEVICE
	},
	/* GODBOX_IOCH2 */
	{
		.virtual	= GODBOX_IOCH2_VIRT,
		.pfn		= __phys_to_pfn(GODBOX_IOCH2_PHYS),
		.length		= GODBOX_IOCH2_SIZE,
		.type		= MT_DEVICE
	},
	/* GODBOX_IOCH3 */
	{
		.virtual	= GODBOX_IOCH3_VIRT,
		.pfn		= __phys_to_pfn(GODBOX_IOCH3_PHYS),
		.length		= GODBOX_IOCH3_SIZE,
		.type		= MT_DEVICE
	},
	/* GODBOX_IOCH4 */
	{
		.virtual	= GODBOX_IOCH4_VIRT,
		.pfn		= __phys_to_pfn(GODBOX_IOCH4_PHYS),
		.length		= GODBOX_IOCH4_SIZE,
		.type		= MT_DEVICE
	},
	/* GODBOX_IOCH5 */
	{
		.virtual	= GODBOX_IOCH5_VIRT,
		.pfn		= __phys_to_pfn(GODBOX_IOCH5_PHYS),
		.length		= GODBOX_IOCH5_SIZE,
		.type		= MT_DEVICE
	},
	/* GODBOX_IOCH6 */
	{
		.virtual	= GODBOX_IOCH6_VIRT,
		.pfn		= __phys_to_pfn(GODBOX_IOCH6_PHYS),
		.length		= GODBOX_IOCH6_SIZE,
		.type		= MT_DEVICE
	},
	/* GODBOX_IOCH7 */
	{
		.virtual	= GODBOX_IOCH7_VIRT,
		.pfn		= __phys_to_pfn(GODBOX_IOCH7_PHYS),
		.length		= GODBOX_IOCH7_SIZE,
		.type		= MT_DEVICE
	}
};

void __init godbox_map_io(void)
{
	int i;

	iotable_init(godbox_io_desc, ARRAY_SIZE(godbox_io_desc));

	for(i=0; i<ARRAY_SIZE(godbox_io_desc); i++)
	{
		edb_putstr(" V: ");	edb_puthex(godbox_io_desc[i].virtual);
		edb_putstr(" P: ");	edb_puthex(godbox_io_desc[i].pfn);
		edb_putstr(" S: ");	edb_puthex(godbox_io_desc[i].length);
		edb_putstr(" T: ");	edb_putul(godbox_io_desc[i].type);
		edb_putstr("\n");
	}

	edb_trace();
}

#define HIL_AMBADEV_NAME(name) hil_ambadevice_##name

#define HIL_AMBA_DEVICE(name,busid,base,platdata)		\
static struct amba_device HIL_AMBADEV_NAME(name) = {		\
	.dev		= {					\
		.coherent_dma_mask = ~0,			\
		.init_name= busid,				\
		.platform_data = platdata,			\
	},							\
	.res		= {					\
		.start	= REG_BASE_##base,			\
		.end	= REG_BASE_##base + 0x1000 -1,	\
		.flags	= IORESOURCE_IO,			\
	},							\
	.dma_mask	= ~0,					\
	.irq		= { INTNR_##base }		\
}

#define UART0_IRQ	{ INTNR_UART0 }
#define UART1_IRQ	{ INTNR_UART1 }

HIL_AMBA_DEVICE(uart0, "uart:0",  UART0,    NULL);
HIL_AMBA_DEVICE(uart1, "uart:1",  UART1,    NULL);

static struct amba_device *amba_devs[] __initdata = {
	& HIL_AMBADEV_NAME(uart0),
	& HIL_AMBADEV_NAME(uart1),
};

/*
 * These are fixed clocks.
 */
static struct clk uart_clk = {
	.rate	= 54000000,
};

static struct clk_lookup lookups[] = {
	{
		/* UART0 */
		.dev_id		= "uart:0",
		.clk		= &uart_clk,
	}, {
		/* UART1 */
		.dev_id		= "uart:1",
		.clk		= &uart_clk,
	},
};

static void godbox_early_init(void)
{
	edb_trace();

	godbox_cpu_init();

	clkdev_add_table(lookups, ARRAY_SIZE(lookups));
}

void __init godbox_init(void)
{
	unsigned long i;

	edb_trace();

	for (i = 0; i < ARRAY_SIZE(amba_devs); i++) {
		edb_trace();
		amba_device_register(amba_devs[i], &iomem_resource);
	}

	arm_pm_restart = arch_reset;
}

static void __init godbox_timer_init(void)
{
	unsigned int regval;
	unsigned int base = IO_ADDRESS(REG_BASE_SCTL);
	edb_trace();

	regval = readl(base);
	regval |= CFG_TIMER0_CLK_SOURCE;
	writel(regval, base);

	sp804_clocksource_init((void *)CFG_TIMER_VABASE, "sp804");
	sp804_clockevents_init((void *)(CFG_TIMER_VABASE + 0x20),
		INTNR_TIMER_0_1, "sp804");

	edb_trace();
}

struct sys_timer godbox_timer = {
	.init = godbox_timer_init,
};

MACHINE_START(GODBOX, "godbox")
	.atag_offset    = 0x100,
	.map_io         = godbox_map_io,
	.init_early     = godbox_early_init,
	.init_irq       = godbox_gic_init_irq,
	.handle_irq     = gic_handle_irq,
	.timer          = &godbox_timer,
	.init_machine   = godbox_init,
MACHINE_END
