#include "semaphore.h"
#include "common_prog.h"

void sema_init(Sema *sema, int16_t counter){
	create_spinlock(&(sema->lock));
	sema->counter=counter;
	sema->queue_start=0;
	sema->queue_end=0;
}

void sema_p(Sema *sema){
	acquire_spinlock(&(sema->lock), 0);
	sema->counter--;
	if(sema->counter<0){
		sema->queue[sema->queue_end]=get_cur_thread_id();
		if(sema->queue_end==MAX_QUEUE_SIZE-1){
			sema->queue_end=0;
		} else {
			sema->queue_end++;
		}
		disable_int();
		set_sleep(get_cur_thread_id());
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
		wake(sema->queue[sema->queue_start]);
		if(sema->queue_start==MAX_QUEUE_SIZE-1){
			sema->queue_start=0;
		} else {
			sema->queue_start++;
		}
	}
	release_spinlock(&(sema->lock));
}
