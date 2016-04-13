#ifndef _MAIN_PROG_H_
#define _MAIN_PROG_H_

#include "common_defs.h"

/*в 2^COUNTER_INCR раз быстрее, чем по умолчанию, будет работать таймер*/
#define COUNTER_INCR 3

#define DEFAULT_TIME_SLICE 50

/*идентификатор потока*/
typedef enum ThreadId{
	PLAY_MELODY_THREAD=0,
	GEN_PRIMES_PRODUCER_THREAD,
	GEN_PRIMES_CONSUMER_THREAD,
	PRINT_INFO_THREAD,
	MOVE_OBJ_THREAD,
	NUM_ALL_THREADS
} ThreadId;

typedef enum ThreadStatus{
	ACTIVE=0,
	PAUSED,
	WAITING
} ThreadStatus;

typedef struct __attribute__((__packed__)) VideoSym{
	char symbol;
	uint8_t attr;
} VideoSym;

void set_thread_status(ThreadId thread_id, ThreadStatus status);

ThreadStatus get_thread_status(ThreadId thread_id);

void set_can_write_to_screen(bool val);

bool get_can_write_to_screen();

bool is_program_finished();

uint8_t get_time_slice(ThreadId thread_id);

#endif
