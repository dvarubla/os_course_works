#ifndef _SPINLOCK_H_
#define _SPINLOCK_H_

#include "common_defs.h"

typedef struct __attribute__((__packed__)) Spinlock{
	uint8_t busy;
} Spinlock;

void create_spinlock(Spinlock *lock);

bool acquire_spinlock(Spinlock *lock, uint32_t tries);

void release_spinlock(Spinlock *lock);
#endif
