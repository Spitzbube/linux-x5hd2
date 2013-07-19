		.macro	addruart,rp,rv,tmp
		ldr	\rv, =0xFE1FB000	@virtual base
		ldr	\rp, =0x101FB000	@physical base
		.endm

#include <asm/hardware/debug-pl01x.S>
