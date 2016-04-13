#ifndef _COMMONPROG_H_
#define _COMMONPROG_H_

#include "common.h"
/*поменять значения двух переменных*/
#define SWAP(x, y) do {typeof(x) SWAP=x; x=y; y=SWAP;}while (0)\

#define MAX(x, y) ((x)>(y)?(x):(y))

/*прочитать байт из порта*/
inline uint8_t inb(uint16_t port)
{
	uint8_t ret;
	asm volatile ( "inb %w1, %b0" : "=a"(ret) : "Nd"(port) );
	return ret;
}

/*выключить прерывания*/
inline void disable_int(){
	asm volatile("cli");
}

/*включить прерывания*/
inline void enable_int(){
	asm volatile("sti");
}

/*установить текстовый режим*/
void set_mode();

/*приостановить выполнение на usec микросекунд*/
void wait(uint32_t usec);

/*получить число тиков таймера с 00:00*/
uint32_t get_time_ticks();

#define ACC_TICKS_IN_S 18.20650736490885416666
#define SEC_IN_MIN 60
#define MIN_IN_HOUR 60
#define MS_IN_S 1000
#define TICKS_IN_S 18.2

typedef struct Time{
	uint8_t ticks;
	uint8_t seconds;
	uint8_t minutes;
	uint8_t hours;
} Time;

/*получить текущее время*/
void get_time(Time *time);

/*инициализировать генератор случайных чисел*/
void srand(uint32_t seed);
/*сгенерировать случайное целое размером 4 байта*/
uint32_t rand_uint32();
/*сгенерировать случайное вещественное число в диапазоне [0;1]*/
double rand_double();
#endif
