#include "interrupts.h"

#include "stages.h"

void get_int_addr(uint16_t interrupt, FullAddr *addr)
{
	asm volatile(
		"pushw %%ds\n\t"
		"movw %w3, %%ds\n\t"
		"movw (%w2), %w0\n\t"
		"movw 2(%w2), %w1\n\t"
		"popw %%ds"
		: "=a"(addr->offset), "=c"(addr->seg):"b"(interrupt*4),"a"(0)
	);
}

void set_int_addr(uint16_t interrupt, FullAddr *addr)
{	
	asm volatile(
		"pushfw\n\t"
		"cli\n\t"
		"pushw %%ds\n\t"
		"movw %w3, %%ds\n\t"
		"movw %w0, (%w2)\n\t"
		"movw %w1, 2(%w2)\n\t"
		"popw %%ds\n\t"
		"popfw\n\t"
		: : "a"(addr->offset), "c"(addr->seg), "b"(interrupt*4), "d"(0):
	);
}

void set_int_addr_func(uint16_t interrupt, uint32_t func){
	asm volatile(
		"pushfw\n\t"
		"cli\n\t"
		"pushw %%ds\n\t"
		"movw %w2, %%ds\n\t"
		"movw %w0, (%w1)\n\t"
		"movw %%cs, 2(%w1)\n\t"
		"popw %%ds\n\t"
		"popfw\n\t"
		: : "a"(func), "b"(interrupt*4), "c"(0):
	);
}

void call_int_addr(const FullAddr *addr){
	asm volatile(
		"pushfw\n\t"
		"lcallw *%0"
		: :"m"(*addr)
	);
}
