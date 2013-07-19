/******************************************************************************
*    Copyright (c) 2009-2011 by Hisi.
*    All rights reserved.
* ***
*
*
******************************************************************************/
#ifndef CPUINFOH
#define CPUINFOH
/******************************************************************************/

#define _HI3716L          (0x00)
#define _HI3716M          (0x08)
#define _HI3716H          (0x0D)
#define _HI3716C          (0x1E)

#define _HI3716M_V100     (0x0837200200LL)
#define _HI3716M_V200     (0x0837160200LL)
#define _HI3716M_V300     (0x0837160300LL)
#define _HI3716C_V100     (0x1E37200200LL)
#define _HI3716H_V100     (0x0D37200200LL)
#define _HI3712_V100      (0x0037120100LL)
#define _HI3716X_MASK     (0xFFFFFFFFFFLL)
#define _HI3712_MASK      (0x00FFFFFFFFLL)
#define _HI3712_V100A      (0)
#define _HI3712_V100B      (1)
#define _HI3712_V100C      (2)
#define _HI3712_V100D      (3)
#define _HI3712_V100E      (4)
#define _HI3712_V100F      (5)
#define _HI3712_V100G      (6)
#define _HI3712_V100I      (7)

void get_clock(unsigned int *cpu, unsigned int *timer);
long long get_chipid(void);
const char *get_cpu_name(void);
const char * get_cpu_version(void);

/******************************************************************************/
#endif /* CPUINFOH */
