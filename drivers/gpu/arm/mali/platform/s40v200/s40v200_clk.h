#ifndef __S40V200_CLK_H__
#define __S40V200_CLK_H__

#ifdef __cplusplus
extern "C" {
#endif

void __init gpu_init_clocks(void);
void gpu_deinit_clocks(void);

void hisi_crg_reset(void);
void hisi_crg_clockon(void);
void hisi_crg_clockoff(void);

int mali_gpu_set_voltage(unsigned int freq);
int clk_gpu_set_voltage(unsigned int volt);

void gpu_set_freq_reg(unsigned int freq);
void gpu_set_utilization_reg(unsigned int utilization);


#ifdef __cplusplus
}
#endif
#endif

