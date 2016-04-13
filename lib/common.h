#ifndef _COMMON_H_
#define _COMMON_H_
#include "common_defs.h"
/*Сколько раз повторить чтение с дискеты*/
#define DEFAULT_RETRY_TIMES 3

#define NUM_BYTES_IN_SECTOR 512

/*Записать значение в порт*/
void inline __REGPARM outb(uint16_t port, uint8_t val)
{
	asm volatile ("outb %0, %w1" : : "a"(val), "Nd"(port));
}

/*Остановить мотор флоппи-дисковода*/
void inline stop_floppy()
{
	outb(0x3f2, 0);
}

/*Остановить процессор*/
void inline stop_cpu(){
	asm volatile("cli \n\t hlt");
}

/*Загрузить секторы с дискеты в память*/
bool load_sectors(
	uint8_t cylinder, 
	uint8_t head, 
	uint8_t sector, 
	uint8_t num_sectors, 
	uint16_t seg, 
	uint16_t address
);

/*Попытаться `times` раз загрузить секторы с дискеты в память*/
bool try_load_sectors(
	uint8_t cylinder, 
	uint8_t head, 
	uint8_t sector, 
	uint8_t num_sectors, 
	uint16_t seg, 
	uint16_t address,
	uint16_t times
);

/*Напечатать символ в режиме телетайпа (с поддержкой \r и \n)*/
void __REGPARM print_char_tt(char c);

/*Напечатать строку*/
void __REGPARM print(const char *s);

/*Сброс контроллера гибкого диска*/
void reset_disk();
#endif
