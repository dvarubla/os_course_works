#include "common2.h"

void get_drive_params(uint8_t *max_heads, uint8_t *max_sectors)
{ 	
	asm volatile(
		"pushw %%es\n\t"
		"movw %%di, %%es\n\t"
		"int $0x13\n\t"
		"shrw $8, %%dx\n\t"
		"xorb %%ch, %%ch\n\t"
		"popw %%es\n\t"
		:"=a"((int32_t){0}), "=D"((int32_t){0}), "=d"(*max_heads), "=c"(*max_sectors)
		:"a"(0x0800), "d"((uint8_t)0), "D"(0): "cc", "bl"
	);
	/*используются только младшие 5 бит*/
	(*max_sectors)&=0b11111;
}
