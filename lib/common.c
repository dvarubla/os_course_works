#include "common.h"

bool load_sectors(
	uint8_t cylinder, 
	uint8_t head, 
	uint8_t sector, 
	uint8_t num_sectors, 
	uint16_t seg, 
	uint16_t address
){
	uint8_t err;	
	asm volatile(
		"pushw %%es\n\t"
		"movw %w1, %%es\n\t"
		"int $0x13\n\t"
		"xorw %%ax, %%ax\n\t"
		"setc %%al\n\t" /*при ошибке устанавливается флаг переноса*/
		"popw %%es\n\t"
		: "=a"(err)
		: "r"(seg/16), "b"(address), "a"(0x0200| num_sectors), "c"(cylinder<<8|sector), "d"(head<<8)
		: "cc"
	);
	return err==0;
}

bool try_load_sectors(
	uint8_t cylinder, 
	uint8_t head, 
	uint8_t sector, 
	uint8_t num_sectors, 
	uint16_t seg, 
	uint16_t address,
	uint16_t times
){
	bool is_loaded;
	for(;times--;){
		is_loaded=load_sectors(cylinder, head, sector, num_sectors, seg, address);
		if(is_loaded){
			return true;
		}
		reset_disk();
	}
	return false;
}

void __REGPARM print_char_tt(char c){
	/*На всякий случай символ печатается белым цветом
	  хотя это работает только в режиме VGA*/
	asm volatile("int $0x10" : : "a"(0x0E00 | c), "b"(7));
}

void __REGPARM print(const char *s){
	while(*s){
		print_char_tt(*s);
		++s;
	}
}

void reset_disk(){
	uint8_t err;
	reset:
	asm volatile(
		"int $0x13\n\t"
		"xorw %%ax, %%ax\n\t"
		"setc %%al\n\t" /*при ошибке устанавливается флаг переноса*/
	:"=a"(err) : "a"(0), "d"((uint8_t)0): "cc");
	if(err==1){
		goto reset;
	}
}
