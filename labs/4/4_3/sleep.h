#ifndef _SLEEP_H_
#define _SLEEP_H_

#include "common_defs.h"

uint16_t get_cur_thread_id();

void set_sleep(uint16_t id);

void sleep_wait(uint16_t id);

void wake(uint16_t id);

#endif
