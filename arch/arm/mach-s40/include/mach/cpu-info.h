/******************************************************************************
 *    COPYRIGHT (C) 2013 Czyong. Hisilicon
 *    All rights reserved.
 * ***
 *    Create by Czyong 2013-03-07
 *
******************************************************************************/

#ifndef CPUINFOH
#define CPUINFOH
/******************************************************************************/


#define _HI3716CV200ES                           (0x0019400200LL)
#define _HI3716CV200ES_MASK                      (0xFFFFFFFFFFLL)

#define _HI3716CV200                             (0x0037160200LL)
#define _HI3716CV200_MASK                        (0xFFFFFFFFFFLL)

void get_clock(unsigned int *cpu, unsigned int *timer);

long long get_chipid(void);

const char * get_cpu_name(void);

const char * get_cpu_version(void);

/******************************************************************************/
#endif /* CPUINFOH */
