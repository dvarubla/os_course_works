#include "spinlock.h"
#include "atomic.h"

void create_spinlock(Spinlock *lock){
	lock->busy=0;
}

bool acquire_spinlock(Spinlock *lock, uint32_t tries){
	bool forever=(tries==0),accuired;	
	for(;forever || tries;  tries-=(forever)?0:1){
		asm volatile(
			"lock cmpxchgb %b3, %1\n\t"
			"setz %b0\n\t"
			:"=a"(accuired), "=m"(lock->busy):"a"(0), "c"((uint8_t)1): "memory"
		);
		if(accuired){
			break;
		}
	}
	return accuired;
}

void release_spinlock(Spinlock *lock){
	asm volatile(
		"lock xchgb %0, %b1\n\t"
		:"=m"(lock->busy), "=a"((int32_t){0}) :"a"(0): "memory"
	);
}
