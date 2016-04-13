#ifndef _SEMAPHORE_H_
#define _SEMAPHORE_H_

#include "sleep.h"
#include "spinlock.h"

#define MAX_QUEUE_SIZE 20

typedef struct Sema{
	Spinlock lock;
	int16_t counter;
	uint16_t queue[MAX_QUEUE_SIZE];
	uint16_t queue_start;
	uint16_t queue_end;
} Sema;

void sema_init(Sema *sema, int16_t counter);

void sema_p(Sema *sema);

void sema_v(Sema *sema);

#endif
