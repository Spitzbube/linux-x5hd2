#include <mach/platform.h>

	.macro  addruart, rp, rv, tmp	
	ldr	\rp, =REG_BASE_CUR_UART @physical base
	ldr	\rv, =IO_ADDRESS(REG_BASE_CUR_UART) @virtual base
	.endm

#include <asm/hardware/debug-pl01x.S>
