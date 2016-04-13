#ifndef _ATOMIC_H_
#define _ATOMIC_H_

#include "common_defs.h"

/*атомарная запись*/
void inline atomic_write(uint32_t *var, uint32_t value){
	asm volatile(
		"lock xchgl %0, %1\n\t"
		:"=m"(*var), "=a"((int32_t){0}) :"a"(value): "memory"
	);
}

/*атомарное чтение*/
uint32_t inline atomic_read(uint32_t *var){
	uint32_t ret;
	asm volatile(
		"lock cmpxchgl %0, %1\n\t"
		:"=a"(ret),"+m"(*var): : "memory"
	);
	return ret;
}
#endif
