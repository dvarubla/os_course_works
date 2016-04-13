#include "semaphore.h"
#include "common_prog.h"

void sema_init(Sema *sema, int16_t counter){
	create_spinlock(&(sema->lock));
	sema->counter=counter;
}

void sema_p(Sema *sema){
	acquire_spinlock(&(sema->lock), 0);
	sema->counter--;
	if(sema->counter<0){
		disable_int();
		set_sleep(get_cur_thread_id());
		sema->queue[-sema->counter-1]=get_cur_thread_id();
		release_spinlock(&(sema->lock));
		enable_int();
		sleep_wait(get_cur_thread_id());
	} else {
		release_spinlock(&(sema->lock));
	}
}

void sema_v(Sema *sema){
	acquire_spinlock(&(sema->lock), 0);
	sema->counter++;
	if(sema->counter<=0){
		wake(sema->queue[-sema->counter]);
	}
	release_spinlock(&(sema->lock));
}
