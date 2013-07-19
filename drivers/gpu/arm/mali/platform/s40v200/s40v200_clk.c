#include "s40v200_reg.h"
#include <linux/clk.h>
#include <mach/platform.h>
#include <linux/clkdev.h>
#include <asm/clkdev.h>
#include <mach/clock.h>
#include <linux/io.h>
#include <linux/module.h>   /* kernel module definitions */
#include <linux/fs.h>       /* file system operations */
#include <linux/cdev.h>     /* character device definitions */
#include "mali_kernel_common.h"
#include "linux/mali/mali_utgard.h"
#include "mali_hw_core.h"

#include "s40v200_cfg.h"
#include "s40v200_clk.h"
#include "s40v200_pmu.h"

typedef struct hiGPU_VF_S {
	unsigned int    freq;       /* unit: KHZ */
	unsigned int    voltage;    /* unit: mv  */
	unsigned int    pwmvalue;   /* duty value */
} GPU_VF_S;


#ifdef S40V200_FPGA
#define MAX_FREQ_NUM    3
GPU_VF_S gpu_freq_volt_table[MAX_FREQ_NUM] =
{
	{30000, 1100, 0x0}, {20000, 1100, 0x0},  {15000, 1100, 0x0},
};

#define DEFAULT_FREQ          15000
#define MAX_FREQ              30000
#define DEFAULT_VOLT          1100
#define DEFAULT_VOLT_INDEX    2
#else
#define MAX_FREQ_NUM          8
GPU_VF_S gpu_freq_volt_table[MAX_FREQ_NUM] =
{
	{432000, 1300, 0x000700A7}, {400000, 1200, 0x002D00A7}, {375000, 1200, 0x002D00A7}, {345600, 1100, 0x005300A7},
	{300000, 1100, 0x005300A7}, {250000, 1100, 0x005300A7}, {200000, 1100, 0x005300A7}, {150000, 1050, 0x006300A7}
};

#define DEFAULT_FREQ          345600
#define MAX_FREQ              432000
#define DEFAULT_VOLT          1100
#define DEFAULT_VOLT_INDEX    3

#endif

#define MAX_STEP_CHANGE     2

static struct mali_hw_core *s_hisi_crg = NULL;
static struct mali_hw_core *s_hisi_pmc = NULL;

/* GPU reset */
void hisi_crg_reset(void)
{
	u32 gpuclock;
	gpuclock = mali_hw_core_register_read(s_hisi_crg, CRG_REG_ADDR_SOFT_RST);
    
    /* reset */
    gpuclock = (gpuclock | GPU_ALL_RESET_MASK);
	mali_hw_core_register_write(s_hisi_crg, CRG_REG_ADDR_SOFT_RST, gpuclock);

    _mali_osk_time_ubusydelay(1);

    /* cancel reset */
    gpuclock = (gpuclock & (~GPU_ALL_RESET_MASK));
    mali_hw_core_register_write(s_hisi_crg, CRG_REG_ADDR_SOFT_RST, gpuclock);
}

/* GPU clock on                                                         */
void hisi_crg_clockon(void)
{
	u32 gpuclock;
#ifndef GPU_DVFS_ENABLE
	s_hisi_pmc = hisi_pmu_get();
	s_hisi_crg = hisi_crg_get();
#endif
	gpuclock = mali_hw_core_register_read(s_hisi_crg, CRG_REG_ADDR_SOFT_RST);
    /* clock on */
	gpuclock = (gpuclock | GPU_CLOCK_ON_OFF_MASK);

	mali_hw_core_register_write(s_hisi_crg, CRG_REG_ADDR_SOFT_RST, gpuclock);
        
	MALI_DEBUG_PRINT(2, ("Hisi gpu clock on\n"));
}

/* GPU clock off                                                        */
void hisi_crg_clockoff(void)
{
	u32 gpuclock = mali_hw_core_register_read(s_hisi_crg, CRG_REG_ADDR_SOFT_RST);

    /* clock off */
	gpuclock = (gpuclock & (~GPU_CLOCK_ON_OFF_MASK));
	
	mali_hw_core_register_write(s_hisi_crg, CRG_REG_ADDR_SOFT_RST, gpuclock);
	MALI_DEBUG_PRINT(2, ("Hisi gpu clock off\n"));
}

int clk_gpu_get_index(unsigned rate)
{
	int i = MAX_FREQ_NUM - 1;

	for (; i > 0; i--)
		if (gpu_freq_volt_table[i].freq >= rate)
			break;

	return i;
}

#ifdef GPU_DVFS_ENABLE

#define VMAX        1320    /*mv*/
#define VMIN        900     /*mv*/
#define PWM_STEP    5       /*mv*/
#define PWM_CLASS   2


#define PERI_PMC8               (HISI_PMC_BASE+0x20)
#define PERI_PMC9               (HISI_PMC_BASE+0x24)

#define PWM_GPU_DUTY_PERIOD     PERI_PMC8
#define PWM_GPU_ENABLE          PERI_PMC9

#define PWM_DUTY_MASK           0xffff0000
#define PWM_PERIOD_MASK         0xffff
#define PWM_ENABLE_BIT          0x2

unsigned int gpu_cur_volt = DEFAULT_VOLT;

int clk_gpu_set_voltage(unsigned int volt)
{
    unsigned int duty, v, tmp;
    unsigned int vmax, vmin, pwc, pws;

    vmax  = VMAX;
    vmin  = VMIN;
    pwc   = PWM_CLASS;
    pws   = PWM_STEP;

    duty = (((vmax - volt) / pws) * pwc) - 1;
    v = mali_hw_core_register_read(s_hisi_pmc, PMC_REG_ADDR_PWM2_DUTY);
    tmp = PWM_DUTY_MASK;
    v &= ~tmp;
    v |= duty << 16; 
    mali_hw_core_register_write(s_hisi_pmc, PMC_REG_ADDR_PWM2_DUTY, v);

    gpu_cur_volt = volt;

	MALI_DEBUG_PRINT(2, ("Mali DVFS: set voltage = %d\n", volt));
    
	return 0;
}


int mali_gpu_set_voltage(unsigned int freq)
{
	int i = clk_gpu_get_index(freq);

	if(gpu_cur_volt != gpu_freq_volt_table[i].voltage)
		clk_gpu_set_voltage(gpu_freq_volt_table[i].voltage);

	return 0;
}


/* set the div for the rate ,the parent must be set firstly */
int clk_gpu_set_rate(struct clk *clk, unsigned rate)
{
	u32 freqctrl;
	u32 lowpower;
	u32 timeout;
	int i = clk_gpu_get_index(rate);

	freqctrl = mali_hw_core_register_read(s_hisi_pmc, PMC_REG_ADDR_GPU_FREQ);

	if (gpu_freq_volt_table[i].freq > clk->rate)
		/* increase frequency  */
		freqctrl = freqctrl | GPU_FREQ_SW_TREND_PMC_MASK;
	else
		/* decrease frequency  */
		freqctrl = freqctrl & (~GPU_FREQ_SW_TREND_PMC_MASK);


	mali_hw_core_register_write(s_hisi_pmc, PMC_REG_ADDR_GPU_FREQ, freqctrl);
	lowpower = mali_hw_core_register_read(s_hisi_crg, CRG_REG_ADDR_LOW_POWER);

	/* set freq   */
	lowpower = lowpower & (~GPU_FREQ_SEL_CFG_CRG_MASK);
	lowpower = lowpower | (i);

	mali_hw_core_register_write(s_hisi_crg, CRG_REG_ADDR_LOW_POWER, lowpower);

	/* frequency request */
	freqctrl = freqctrl | GPU_FREQ_SW_REQ_PMC_MASK;

	mali_hw_core_register_write(s_hisi_pmc, PMC_REG_ADDR_GPU_FREQ, freqctrl);

	timeout = 5000;
	do {
		u32 status;
		status = mali_hw_core_register_read(s_hisi_pmc, PMC_REG_ADDR_COREX_STATUS);
		/* Get status of sleeping cores */
		if (status & GPU_CLK_SW_OK_PMC) {
			break;
		}
		_mali_osk_time_ubusydelay(10);
		timeout--;
	} while (timeout > 0);


	/* cancel frequency request */
	freqctrl = freqctrl & (~GPU_FREQ_SW_REQ_PMC_MASK);
	mali_hw_core_register_write(s_hisi_pmc, PMC_REG_ADDR_GPU_FREQ, freqctrl);

	if (0 == timeout) {
		MALI_PRINT_ERROR(("Mali DVFS:wait time good timeout!\n"));
        
		return -1;
	}

	clk->rate = gpu_freq_volt_table[i].freq;

	MALI_DEBUG_PRINT(2, ("Mali DVFS:adjust frequency to %d\n", gpu_freq_volt_table[i].freq));
    
#ifdef S40V200_VMIN_TEST
	gpu_set_freq_reg(clk->rate);
#endif
	return 0;
}

static unsigned int clk_gpu_get_rate(struct clk *clk)
{
	return clk->rate;
}

static long clk_gpu_round_rate(struct clk *clk, long rate)
{
	int i = clk_gpu_get_index(rate);
    int cur_step = clk_gpu_get_index(clk->rate);

    /* Make sure we do not jump large than 2 steps */
    if(i > (cur_step + MAX_STEP_CHANGE)){
        i = cur_step + MAX_STEP_CHANGE;
    }
    else if((i + MAX_STEP_CHANGE) < cur_step){
        i = cur_step - MAX_STEP_CHANGE;
    }
    
	return gpu_freq_volt_table[i].freq;
}

static struct clk_ops clk_gpu_ops = {
	.get_rate   = clk_gpu_get_rate,
	.set_rate   = clk_gpu_set_rate,
	.round_rate = clk_gpu_round_rate,
};

struct clk clk_gpu =
{
	.name       = "clk_gpu",
	.rate       = DEFAULT_FREQ,
	.max_rate   = MAX_FREQ,
	.ops        = &clk_gpu_ops,
};

void clk_gpu_initial(void)
{
	u32 freqctrl;
	u32 lowpower;
	u32 timeout;
	freqctrl = mali_hw_core_register_read(s_hisi_pmc, PMC_REG_ADDR_GPU_FREQ);

	/* use hardware status machine  */
	freqctrl = freqctrl | (GPU_FREQ_FSM_ENABLE_MASK);
	/* clear power good value  GPU	*/
	freqctrl = freqctrl & (~GPU_FREQ_TIME_WAIT_POWER_GOOD_MASK);
	/* set power good value  GPU, set in the initial status */
	freqctrl = freqctrl | (GPU_FREQ_TIME_WAIT_POWER_GOOD_VALUE);
	/* use hardware power good   */
	freqctrl = freqctrl & (~GPU_USE_POWER_GOOD_CPU);

	mali_hw_core_register_write(s_hisi_pmc, PMC_REG_ADDR_GPU_FREQ, freqctrl);

	/* set power good wait time */
	mali_hw_core_register_write(s_hisi_pmc, PMC_REG_ADDR_POWER_TIME, 0x00ffff88);
    

	lowpower = mali_hw_core_register_read(s_hisi_crg, CRG_REG_ADDR_LOW_POWER);

	/* use status machine output */
	lowpower = lowpower & (~(GPU_BEGIN_CFG_BYPASS_MASK | GPU_DIV_CFG_BYPASS_MASK | GPU_FREQ_DIV_CFG_CRG_MASK | GPU_FREQ_SEL_CFG_CRG_MASK));
	/* select 345.6MHz */
	lowpower = lowpower | 0x3 ;
	mali_hw_core_register_write(s_hisi_crg, CRG_REG_ADDR_LOW_POWER, lowpower);

	/* start adjust frequency */ 
	freqctrl = freqctrl | GPU_FREQ_SW_REQ_PMC_MASK;
	mali_hw_core_register_write(s_hisi_pmc, PMC_REG_ADDR_GPU_FREQ, freqctrl);
    
	timeout = 5000;
	do {
		u32 status;
		status = mali_hw_core_register_read(s_hisi_pmc, PMC_REG_ADDR_COREX_STATUS);
		/* Get status of sleeping cores */
		if (status & GPU_CLK_SW_OK_PMC) {
			break;
		}
		_mali_osk_time_ubusydelay(10);
		timeout--;
	} while (timeout > 0);


	/* cancel frequency request */
	freqctrl = freqctrl & (~GPU_FREQ_SW_REQ_PMC_MASK);
	mali_hw_core_register_write(s_hisi_pmc, PMC_REG_ADDR_GPU_FREQ, freqctrl);
}


void __init gpu_init_clocks(void)
{
	clk_init(&clk_gpu);

	s_hisi_pmc = hisi_pmu_get();
	s_hisi_crg = hisi_crg_get();

	hisi_crg_clockon();

	hisi_crg_reset();

	clk_gpu_initial();

}

void gpu_deinit_clocks(void)
{
	hisi_crg_clockoff();
}

#ifdef S40V200_VMIN_TEST
#define GPU_REG_FREQ        0xF80000F8
#define GPU_REG_UTILIZATION 0xF80000FC

void gpu_set_freq_reg(unsigned int freq)
{
	volatile unsigned int *pu32Freq = ioremap_nocache(GPU_REG_FREQ, 32);
	*pu32Freq = freq;
	iounmap(pu32Freq);
}

void gpu_set_utilization_reg(unsigned int utilization)
{
	volatile unsigned int *pu32Utilization = ioremap_nocache(GPU_REG_UTILIZATION, 32);
	*pu32Utilization = (utilization * 100) / 256;
	iounmap(pu32Utilization);
}

int gpu_dvfs_set_voltage(unsigned int voltage)
{
	return clk_gpu_set_voltage(voltage);
}

EXPORT_SYMBOL_GPL(gpu_dvfs_set_voltage);

int gpu_dvfs_set_freq(unsigned int freq)
{
	struct clk *gpu_clk = &clk_gpu;
	return clk_set_rate(gpu_clk, freq);
}
EXPORT_SYMBOL_GPL(gpu_dvfs_set_freq);

unsigned int gpu_dvfs_get_voltage(void)
{
	return gpu_cur_volt;
}
EXPORT_SYMBOL_GPL(gpu_dvfs_get_voltage);

unsigned int gpu_dvfs_get_freq(void)
{
	struct clk *gpu_clk = &clk_gpu;

	return clk_get_rate(gpu_clk);
}
EXPORT_SYMBOL_GPL(gpu_dvfs_get_freq);

#endif


#endif

