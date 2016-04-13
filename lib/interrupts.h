#ifndef _INTERRUPTS_H_
#define _INTERRUPTS_H_

#include "common_defs.h"

/*полный адрес обработчика прерывания -- сегмент и смещение*/
typedef struct __attribute__((__packed__)) FullAddr{
	uint16_t offset;
	uint16_t seg;
} FullAddr;

/*макрос для задания своего обработчика прерывания handler_func*/
#define INTERRUPT_ROUTINE(num, handler_func) \
void int##num##_handler();                   \
asm(                                         \
	"int" #num "_handler:\n\t"           \
	"pushl %ebp\n\t"                     \
	"movl %esp, %ebp\n\t"                \
	"andw $(~0xF), %sp\n\t"              \
                                             \
	"subw $512, %sp\n\t"                 \
	"fnsave (%esp)\n\t"                  \
	"pushw %ds\n\t"                      \
	"pushal\n\t"                         \
                                             \
	"xorw %ax, %ax\n\t"                  \
	"movw %ax, %ds\n\t"                  \
	"movw ("PASTE(DATA_LOC)"), %ax\n\t"  \
	"movw %ax, %ds\n\t"                  \
	"call " #handler_func "\n\t"         \
                                             \
	"popal\n\t"                          \
	"popw %ds\n\t"                       \
	"frstor (%esp)\n\t"                  \
	"addw $(512), %sp\n\t"               \
                                             \
	"leave\n\t"                          \
	"iretw\n\t"                          \
);

/*получить адрес обработчика прерывания*/
void get_int_addr(uint16_t interrupt, FullAddr *addr);
/*устанвить адрес обработчика прерывания*/
void set_int_addr(uint16_t interrupt, FullAddr *addr);
/*установить обработчиком прерывания функцию func*/
void set_int_addr_func(uint16_t interrupt, uint32_t func);
/*вызвать обработчик прерывания по адресу addr*/
void call_int_addr(const FullAddr *addr);
#endif
