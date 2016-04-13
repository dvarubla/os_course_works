#include "common_prog.h"
#include "stages.h"
#include "math.h"

void set_mode()
{
	asm volatile("int $0x10": :"a"(0x0003));	
}

void wait(uint32_t usec)
{
	asm volatile("int $0x15": "=a"((int32_t){0}): "a"(0x8600), "c"(usec>>16), "d"(usec&0xFFFF): "cc");
}

uint32_t get_time_ticks()
{
	uint16_t t1, t2;
	asm volatile("int $0x1A": "=c"(t1), "=d"(t2), "=a"((int32_t){0}): "a"(0));
	return (t1<<16)+t2;
	
}

void get_time(Time *time){
	uint32_t total_ticks=get_time_ticks(),t;
	time->ticks=fmod(total_ticks, ACC_TICKS_IN_S);
	t=round_to_zero(total_ticks/ACC_TICKS_IN_S);
	time->seconds=t%SEC_IN_MIN;
	t/=SEC_IN_MIN;
	time->minutes=t%MIN_IN_HOUR;
	time->hours=t/MIN_IN_HOUR;
}

/*числа, которыми будет проинициализирован генератор по умолчанию*/
uint32_t rand1=521288629, rand2=362436069;

void srand(uint32_t seed)
{
	rand1=seed+1;
	rand2=seed/3+1;
}

uint32_t rand_uint32()
{
	rand1=36969*(rand1&0xFF)+(rand1>>16);
    	rand2=18000*(rand2&0xFF)+(rand2>>16);
    	return (rand1<<16)+rand2;
}

double rand_double(){
	return (double)rand_uint32()/UINT32_MAX;
}
