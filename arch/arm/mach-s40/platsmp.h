#ifndef __PLATSMP__H__
#define __PLATSMP__H__

extern int __cpuinitdata pen_release;

void s40_secondary_startup(void);

void slave_cores_power_up(int cpu);

void slave_cores_power_off(int cpu);

#endif

