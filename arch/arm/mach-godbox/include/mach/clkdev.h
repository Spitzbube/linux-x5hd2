#ifndef __ASM_MACH_CLKDEV_H
#define __ASM_MACH_CLKDEV_H

struct clk_ops;

struct clk {
	unsigned long rate;
	struct clk_ops *ops;
	int enable_count;
};

struct clk_ops {
	int (*enable)(struct clk *clk);
	int (*disable)(struct clk *clk);

	int (*unprepare)(struct clk *clk);
	int (*prepare)(struct clk *clk);

	int (*set_rate)(struct clk *clk, unsigned long rate);
	unsigned long (*get_rate)(struct clk *clk);

	unsigned long (*round_rate)(struct clk *clk, unsigned long rate);
};

#define __clk_get(clk) ({ 1; })
#define __clk_put(clk) do { } while (0)
#endif
