#ifndef _COMMONDEFS_H_
#define _COMMONDEFS_H_
/*Нужен для типов uint8_t и т.п*/
#include <stdint.h>

typedef enum bool {
	false=0, true
} bool;

/*Макрос, указывающий, что аргументы функции передаются в регистрах*/ 
#define __REGPARM   __attribute__ ((regparm(3)))
/*Макросы для вставки других макросов как строк*/
#define STRINGIFY(x) #x
#define PASTE(x) STRINGIFY(x)

/*Макрос для отладчика*/
#define BREAK asm volatile("xchgw %bx, %bx\n\t")

/*Макросы для поддержки переменного числа аргументов (printf)*/
typedef __builtin_va_list va_list;
#define va_start(vargs, last_param) __builtin_va_start(vargs, last_param)
#define va_end(vargs) __builtin_va_end(vargs)
#define va_arg(vargs, arg_type) __builtin_va_arg(vargs, arg_type)

#endif
