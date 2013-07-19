/******************************************************************************
 *    COPYRIGHT (C) 2013 Czyong. Hisilicon
 *    All rights reserved.
 * ***
 *    Create by Czyong 2013-03-15
 *
******************************************************************************/
#ifndef HI3716X_CLOCK_DIVH
#define HI3716X_CLOCK_DIVH
/******************************************************************************/

unsigned int get_clk_posdiv(unsigned long osc);

void get_hi3716xv100_clock(unsigned int *cpu, unsigned int *timer);

/******************************************************************************/
#endif /* HI3716X_CLOCK_DIVH */
