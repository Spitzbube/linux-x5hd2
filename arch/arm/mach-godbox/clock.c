/******************************************************************************
 *    COPYRIGHT (C) 2013 Czyong. Hisilicon
 *    All rights reserved.
 * ***
 *    Create by Czyong 2013-03-15
 *
******************************************************************************/

#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <mach/clkdev.h>

static DEFINE_SPINLOCK(enable_lock);
static DEFINE_MUTEX(prepare_lock);

static void __clk_disable(struct clk *clk)
{
	if (!clk)
		return;

	if (WARN_ON(clk->enable_count == 0))
		return;

	if (--clk->enable_count > 0)
		return;

	if (clk->ops && clk->ops->disable)
		clk->ops->disable(clk);
}

void clk_disable(struct clk *clk)
{
	unsigned long flags;

	spin_lock_irqsave(&enable_lock, flags);
	__clk_disable(clk);
	spin_unlock_irqrestore(&enable_lock, flags);
}
EXPORT_SYMBOL_GPL(clk_disable);

static int __clk_enable(struct clk *clk)
{
	int ret = 0;

	if (!clk)
		return 0;

	if (clk->enable_count == 0) {
		if (clk->ops && clk->ops->enable) {
			ret = clk->ops->enable(clk);
			if (ret)
				return ret;
		}
	}

	clk->enable_count++;
	return 0;
}

static unsigned long __clk_get_rate(struct clk *clk)
{
	unsigned long ret;

	if (!clk) {
		ret = -EINVAL;
		goto out;
	}

	ret = clk->rate;

out:
	return ret;
}

int clk_enable(struct clk *clk)
{
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&enable_lock, flags);
	ret = __clk_enable(clk);
	spin_unlock_irqrestore(&enable_lock, flags);

	return ret;
}
EXPORT_SYMBOL_GPL(clk_enable);

unsigned long clk_get_rate(struct clk *clk)
{
	unsigned long rate;

	mutex_lock(&prepare_lock);
	rate = __clk_get_rate(clk);
	mutex_unlock(&prepare_lock);

	return rate;
}
EXPORT_SYMBOL_GPL(clk_get_rate);

unsigned long __clk_round_rate(struct clk *clk, unsigned long rate)
{
	if (!clk)
		return -EINVAL;

	if (!clk->ops || !clk->ops->round_rate)
		return clk->rate;

	return clk->ops->round_rate(clk, rate);
}

long clk_round_rate(struct clk *clk, unsigned long rate)
{
	unsigned long ret;

	mutex_lock(&prepare_lock);
	ret = __clk_round_rate(clk, rate);
	mutex_unlock(&prepare_lock);

	return ret;
}
EXPORT_SYMBOL_GPL(clk_round_rate);

int clk_set_rate(struct clk *clk, unsigned long rate)
{
	/* prevent racing with updates to the clock topology */
	mutex_lock(&prepare_lock);

	/* bail early if nothing to do */
	if (rate != clk->rate) {
		if (clk->ops && clk->ops->set_rate)
			clk->ops->set_rate(clk, rate);
	}

	mutex_unlock(&prepare_lock);

	return 0;
}
EXPORT_SYMBOL_GPL(clk_set_rate);
