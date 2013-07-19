/*
 * Copyright (C) 2010-2012 ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 *
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/**
 * @file mali_platform.c
 * Platform specific Mali driver functions for a default platform
 */

#include <linux/clk.h>
#include <mach/platform.h>
#include <linux/clkdev.h>
#include <asm/clkdev.h>
#include <mach/clock.h>
#include <linux/module.h>   /* kernel module definitions */
#include <linux/fs.h>       /* file system operations */
#include <linux/cdev.h>     /* character device definitions */
#include "mali_kernel_common.h"
#include "linux/mali/mali_utgard.h"

#include "s40v200_cfg.h"
#include "s40v200_reg.h"
#include "s40v200_pmu.h"
#include "s40v200_clk.h"


#define HISI_PMU_NO_VSYNC_MIN_UTILISATION       26    /* 256 * 10% = 25.6 */
#define HISI_PMU_NO_VSYNC_MAX_UTILISATION       102     /* 256 * 40% = 102.4 */

/* Module parameter to control dvfs utilization */
int mali_dvfs_min_utilization = HISI_PMU_NO_VSYNC_MIN_UTILISATION;
module_param(mali_dvfs_min_utilization, int, S_IRUSR | S_IWUSR | S_IWGRP | S_IRGRP | S_IROTH); /* rw-rw-r-- */
MODULE_PARM_DESC(mali_dvfs_min_utilization, "min utilization");

/* Module parameter to control dvfs utilization */
int mali_dvfs_max_utilization = HISI_PMU_NO_VSYNC_MAX_UTILISATION;
module_param(mali_dvfs_max_utilization, int, S_IRUSR | S_IWUSR | S_IWGRP | S_IRGRP | S_IROTH); /* rw-rw-r-- */
MODULE_PARM_DESC(mali_dvfs_max_utilization, "max utilization");

#ifdef CONFIG_GPU_DVFS_ENABLE
int mali_dvfs_enable = 1;
#else
int mali_dvfs_enable = 0;
#endif
module_param(mali_dvfs_enable, int, S_IRUSR | S_IWUSR | S_IWGRP | S_IRGRP | S_IROTH); /* rw-rw-r-- */
MODULE_PARM_DESC(mali_dvfs_enable, "enable utilization");

unsigned int mali_dvfs_utilization = 0;

static void mali_dvfs_work_handler(struct work_struct *w);

static struct workqueue_struct *mali_dvfs_wq = 0;
static DECLARE_WORK(mali_dvfs_work, mali_dvfs_work_handler);

static u32 bPowerOff = 1;

typedef enum hiGPU_CORE_E
{
	GPU_CORE_GP,
	GPU_CORE_PP0,
	GPU_CORE_PP1,
	GPU_CORE_PP3,
	GPU_CORE_PP4
}GPU_CORE_E;

struct mali_hw_core *hisi_crg = NULL;
struct mali_hw_core *hisi_pmc = NULL;


mali_bool init_mali_dvfs_status(void)
{
	/*default status
	add here with the right function to get initilization value.
	*/

	if (!mali_dvfs_wq)
		mali_dvfs_wq = create_singlethread_workqueue("mali_dvfs");

	return MALI_TRUE;
}

void deinit_mali_dvfs_status(void)
{
	if (mali_dvfs_wq) {
		destroy_workqueue(mali_dvfs_wq);
		mali_dvfs_wq = NULL;
	}
}


struct mali_hw_core * hisi_pmu_get(void)
{
	return hisi_pmc;
}

struct mali_hw_core * hisi_crg_get(void)
{
	return hisi_crg;
}

#ifdef GPU_PMU_HISI
static _mali_osk_errcode_t hisi_pmu_powerup_singlecore(GPU_CORE_E eCore)
{
	u32 pwreq;
	u32 pwstat;
	u32 timeout;

	struct mali_hw_core  *pmc = hisi_pmu_get();

	if (GPU_CORE_PP0 == eCore) {
		pwreq = mali_hw_core_register_read(pmc, PMC_REG_ADDR_CORE0_POWER);

		/* enable pmc, do not use ack, power up request */
		pwreq = ((pwreq | GPU_CORE0_PMC_ENABLE_MASK)
			& (~GPU_CORE0_WAIT_MTCMOS_ACK_MASK)
			& (~GPU_CORE0_POWERDOWN_REQ_MASK));
		mali_hw_core_register_write(pmc, PMC_REG_ADDR_CORE0_POWER, pwreq);
		/* Wait for cores to be powered up (100 x 100us = 100ms) */
		timeout = 100;
		do {
			pwstat = mali_hw_core_register_read(pmc, PMC_REG_ADDR_COREX_STATUS);
			/* Get status of sleeping cores */
			if (GPU_CORE0_POWERUP_OK
			    == (pwstat & GPU_CORE0_POWER_STATUS_MACHINE_MASK))
				break;

			_mali_osk_time_ubusydelay(100);
			timeout--;
		} while( timeout > 0 );

		if (timeout == 0) {
			MALI_DEBUG_PRINT(2, ("Hisi Powerup pp0 timeout\n"));
			return _MALI_OSK_ERR_TIMEOUT;
		}
		MALI_DEBUG_PRINT(2, ("Hisi Powerup pp0 successful\n"));
		return _MALI_OSK_ERR_OK;
	}

	if (GPU_CORE_PP1 == eCore) {
		pwreq = mali_hw_core_register_read(pmc, PMC_REG_ADDR_CORE1_POWER);
		/* enable pmc, do not use ack, power up request */
		pwreq = ((pwreq | GPU_CORE1_PMC_ENABLE_MASK)
			& (~GPU_CORE1_WAIT_MTCMOS_ACK_MASK)
			& (~GPU_CORE1_POWERDOWN_REQ_MASK));
		mali_hw_core_register_write(pmc, PMC_REG_ADDR_CORE1_POWER, pwreq);
		/* Wait for cores to be powered up (100 x 100us = 100ms) */
		timeout = 100;

		do {
			pwstat = mali_hw_core_register_read(pmc, PMC_REG_ADDR_COREX_STATUS);
			/* Get status of sleeping cores */
			if(GPU_CORE1_POWERUP_OK
				== (pwstat & GPU_CORE1_POWER_STATUS_MACHINE_MASK))
				break;
			_mali_osk_time_ubusydelay(100);
			timeout--;
		} while( timeout > 0 );

		if (timeout == 0) {
			MALI_DEBUG_PRINT(2, ("Hisi Powerup pp1 timeout\n"));
			return _MALI_OSK_ERR_TIMEOUT;
		}

		MALI_DEBUG_PRINT(2, ("Hisi Powerup pp1 successful\n"));
		return _MALI_OSK_ERR_OK;
	}

	return _MALI_OSK_ERR_UNSUPPORTED;
}

static int hisi_pmu_powerdown_singlecore(GPU_CORE_E eCore)
{
	u32 pwreq;
	u32 pwstat;
	u32 timeout;

	struct mali_hw_core  *pmc = hisi_pmu_get();

	if(GPU_CORE_PP0 == eCore)
	{
		pwreq = mali_hw_core_register_read(pmc, PMC_REG_ADDR_CORE0_POWER);
		/* enable pmc, do not use ack, power down request */
		pwreq = (((pwreq | GPU_CORE0_PMC_ENABLE_MASK) & (~GPU_CORE0_WAIT_MTCMOS_ACK_MASK))
			| GPU_CORE0_POWERDOWN_REQ_MASK);
		mali_hw_core_register_write(pmc, PMC_REG_ADDR_CORE0_POWER, pwreq);

		/* Wait for cores to be powered up (100 x 100us = 100ms) */
		timeout = 100;
		do
		{
			pwstat = mali_hw_core_register_read(pmc, PMC_REG_ADDR_COREX_STATUS);
			/* Get status of sleeping cores */
			if(GPU_CORE0_POWERDOWN_OK  == (pwstat & GPU_CORE0_POWER_STATUS_MACHINE_MASK))
			{
				break;
			}
			_mali_osk_time_ubusydelay(100);
			timeout--;
		} while( timeout > 0 );

		if( timeout == 0 )
		{
			MALI_DEBUG_PRINT(2, ("Hisi Powerdown pp0 timeout\n"));
			return _MALI_OSK_ERR_TIMEOUT;
		}

		MALI_DEBUG_PRINT(2, ("Hisi Powerdown pp0 successful\n"));
		return _MALI_OSK_ERR_OK;
	}

	if(GPU_CORE_PP1 == eCore)
	{
		pwreq = mali_hw_core_register_read(pmc, PMC_REG_ADDR_CORE1_POWER);
		/* enable pmc, do not use ack, power down request */
		pwreq = (((pwreq | GPU_CORE1_PMC_ENABLE_MASK) & (~GPU_CORE1_WAIT_MTCMOS_ACK_MASK))
			| GPU_CORE1_POWERDOWN_REQ_MASK);
		mali_hw_core_register_write(pmc, PMC_REG_ADDR_CORE1_POWER, pwreq);
		/* Wait for cores to be powered up (100 x 100us = 100ms) */
		timeout = 100;

		do
		{
			pwstat = mali_hw_core_register_read(pmc, PMC_REG_ADDR_COREX_STATUS);
			/* Get status of sleeping cores */
			if(GPU_CORE1_POWERDOWN_OK  == (pwstat & GPU_CORE1_POWER_STATUS_MACHINE_MASK))
			{
				break;
			}
			MALI_DEBUG_PRINT(4, ("Hisi Powerdown pwstat=0x%x\n", pwstat));
			_mali_osk_time_ubusydelay(100);
			timeout--;
		} while( timeout > 0 );

		if( timeout == 0 )
		{
			MALI_DEBUG_PRINT(2, ("Hisi Powerdown pp1 timeout\n"));
			return _MALI_OSK_ERR_TIMEOUT;
		}

		MALI_DEBUG_PRINT(2, ("Hisi Powerdown pp1 successful\n"));
		return _MALI_OSK_ERR_OK;
	}
	return _MALI_OSK_ERR_UNSUPPORTED;
}

static int hisi_pmu_powerup(void)
{
	hisi_pmu_powerup_singlecore(GPU_CORE_PP0);
	hisi_pmu_powerup_singlecore(GPU_CORE_PP1);

	hisi_crg_clockon();
	hisi_crg_reset();

	return 0;
}

static int hisi_pmu_powerdown(void)
{
	hisi_crg_clockoff();

	hisi_pmu_powerdown_singlecore(GPU_CORE_PP1);
	hisi_pmu_powerdown_singlecore(GPU_CORE_PP0);

	return 0;
}

#endif


static struct mali_hw_core * hisi_hw_core_create(u32 phys_addr, u32 size, char* description)
{
	struct mali_hw_core *hwcore = (struct mali_hw_core *)_mali_osk_malloc(sizeof(struct mali_hw_core));
	if(NULL == hwcore)
	{
		MALI_PRINT_ERROR(("Failed to malloc %s hardware!\n", description));
		return NULL;
	}
	hwcore->phys_addr   = phys_addr;
	hwcore->size        = size;
	hwcore->description = NULL;
	hwcore->mapped_registers = _mali_osk_mem_mapioregion(hwcore->phys_addr, hwcore->size, hwcore->description);
	if (NULL == hwcore->mapped_registers)
	{
		_mali_osk_free(hwcore);
		hwcore = NULL;
		MALI_PRINT_ERROR(("Failed to map memory region for %s phys_addr 0x%08X\n", description, hwcore->phys_addr));
		return NULL;
	}
	return hwcore;
}

static void hisi_hw_core_destroy(struct mali_hw_core *hwcore)
{
	if(NULL == hwcore)
	{
		return ;
	}
	_mali_osk_mem_unmapioregion(hwcore->phys_addr, hwcore->size, hwcore->mapped_registers);

	_mali_osk_free(hwcore);
}

_mali_osk_errcode_t mali_platform_init(void)
{
	u32 pmu_select;

	if(NULL != hisi_crg)
	{
		MALI_SUCCESS;
	}

	hisi_crg = hisi_hw_core_create(HISI_CRG_BASE, CRG_REG_ADDR_SIZE, "hisi_crg");
	if(NULL== hisi_crg)
	{
		return _MALI_OSK_ERR_FAULT;
	}

	hisi_pmc = hisi_hw_core_create(HISI_PMC_BASE, PMC_REG_ADDR_SIZE, "hisi_pmc");
	if(NULL== hisi_pmc)
	{
		hisi_hw_core_destroy(hisi_crg);
		return _MALI_OSK_ERR_FAULT;
	}
	pmu_select = mali_hw_core_register_read(hisi_pmc, PMC_REG_ADDR_GPU_CTRL);
#ifdef GPU_PMU_HISI
	/* pmu selection: use external pmu */
	pmu_select = pmu_select | (GPU_PMU_BYPASS_MASK);
#else
	/* pmu selection: use mali pmu */
	pmu_select = pmu_select & (~GPU_PMU_BYPASS_MASK);
#endif
	/* use cnt */
	pmu_select = pmu_select & (~GPU_PMU_CNT_BYPASS_MASK);

	mali_hw_core_register_write(hisi_pmc, PMC_REG_ADDR_GPU_CTRL, pmu_select);

    gpu_init_clocks();

#ifdef GPU_DVFS_ENABLE
	if (!init_mali_dvfs_status()) {
		MALI_DEBUG_PRINT(1, ("mali_platform_init failed\n"));
	}
#endif

	MALI_DEBUG_PRINT(2, ("Hisi platform init successful\n"));
	MALI_SUCCESS;
}

_mali_osk_errcode_t mali_platform_deinit(void)
{
	MALI_DEBUG_PRINT(2, ("Hisi platform deinit Enter\n"));

#ifdef GPU_DVFS_ENABLE
	deinit_mali_dvfs_status();
#endif

	gpu_deinit_clocks();

	if(NULL == hisi_crg) {
		MALI_SUCCESS;
	}

	hisi_hw_core_destroy(hisi_crg);
	hisi_hw_core_destroy(hisi_pmc);

	hisi_crg = NULL;
	hisi_pmc = NULL;

	MALI_DEBUG_PRINT(2, ("Hisi platform deinit successful\n"));
	MALI_SUCCESS;
}

#ifdef GPU_PMU_HISI
_mali_osk_errcode_t mali_platform_power_mode_change(mali_power_mode power_mode)
{
	switch (power_mode) {
		case MALI_POWER_MODE_ON:
			if (bPowerOff == 1)
			{
				hisi_pmu_powerup();
				bPowerOff = 0;
			}
			break;

		case MALI_POWER_MODE_LIGHT_SLEEP:
			if (bPowerOff == 0)
			{
				hisi_pmu_powerdown_singlecore(GPU_CORE_PP0);
				hisi_pmu_powerdown_singlecore(GPU_CORE_PP1);
				bPowerOff = 1;
			}
			break;

		case MALI_POWER_MODE_DEEP_SLEEP:
			if (bPowerOff == 0)
			{
				hisi_pmu_powerdown();
				bPowerOff = 1;
			}
			else
			{
				hisi_crg_clockoff();
			}
			break;
	}
	MALI_SUCCESS;
}
#else
_mali_osk_errcode_t mali_platform_power_mode_change(mali_power_mode power_mode)
{
	switch (power_mode)
	{
		case MALI_POWER_MODE_ON:
			if (bPowerOff == 1)
			{
				hisi_crg_clockon();
				hisi_crg_reset();
				mali_pmu_powerup();
				bPowerOff = 0;
			}
		break;
		case MALI_POWER_MODE_LIGHT_SLEEP:
		case MALI_POWER_MODE_DEEP_SLEEP:
			if (bPowerOff == 0)
			{
				mali_pmu_powerdown();
                hisi_crg_clockoff();
				bPowerOff = 1;
			}
		break;
	}
	MALI_SUCCESS;
}
#endif

static inline unsigned clk_gpu_get_next_rate(unsigned current_rate, unsigned utilization)
{
	if(0 == (mali_dvfs_min_utilization + mali_dvfs_max_utilization))
	{
		return 0xffff;
	}

	return current_rate * utilization * 2 / (mali_dvfs_min_utilization + mali_dvfs_max_utilization);
}


mali_bool mali_dvfs_handler(unsigned int utilization)
{
	mali_dvfs_utilization = utilization;

	queue_work(mali_dvfs_wq,&mali_dvfs_work);

	/*add error handle here*/

	return MALI_TRUE;
}

extern struct clk clk_gpu;

static void mali_dvfs_work_handler(struct work_struct *w)
{
#ifdef GPU_DVFS_ENABLE
	struct clk *gpu_clk;
	u32 cur_rate;
	long next_rate;
	unsigned int utilization = mali_dvfs_utilization;
	MALI_DEBUG_PRINT(2, ("Mali DVFS:  utilization(%d)...\n",  utilization));
#endif	
#ifdef S40V200_VMIN_TEST
	gpu_set_utilization_reg(mali_dvfs_utilization);
#endif
#ifdef GPU_DVFS_ENABLE
	if(0 == mali_dvfs_enable){
	    return;
	}

	if((utilization >= mali_dvfs_min_utilization) && (utilization <= mali_dvfs_max_utilization)){
	    /* Nothing to do                                                    */
	    return ;
	}
	gpu_clk = &clk_gpu;
	cur_rate = clk_get_rate(gpu_clk);
	next_rate = clk_gpu_get_next_rate(cur_rate, utilization);
	next_rate = clk_round_rate(gpu_clk, next_rate);
	MALI_DEBUG_PRINT(2, ("Mali DVFS: cur_rate (%d), next_rate (%d) ...\n", cur_rate, next_rate));

	if(next_rate == cur_rate){
	    return;
	}

	/* if increase frequency, adjust voltage first */
	if(next_rate > cur_rate){
	    mali_gpu_set_voltage(next_rate);
	}

	if(clk_set_rate(gpu_clk, next_rate) < 0){
	    /* roll back to previous voltage */
	    if(next_rate > cur_rate){
	        mali_gpu_set_voltage(cur_rate);
	    }
	        
	    return ;
	}
	/* if decrease frequency, decrease voltage after adjust frequency */
	if(next_rate < cur_rate){
	    mali_gpu_set_voltage(next_rate);
	}
#endif
}


void mali_gpu_utilization_handler(u32 utilization)
{
if(0 == bPowerOff){
#ifdef GPU_DVFS_ENABLE
	if(!mali_dvfs_handler(utilization))
		MALI_DEBUG_PRINT(1,( "error on mali dvfs status in utilization\n"));
#endif
}

	MALI_DEBUG_PRINT(2, ("Mali DVFS: utilization = %d\n", utilization));
}

void set_mali_parent_power_domain(void* dev)
{

}



