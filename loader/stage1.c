#include "common.h"
#include "stages.h"

#define DATA_OFFSET BOOTSECTOR_OFFSET

asm (
	"movw $("PASTE(STACK_START)"/16), %ax\n\t"
	"movw %ax, %ss\n\t"
	"movw $("PASTE(INITIAL_SP)"), %sp\n\t" /*создание стека*/
	"movw $("PASTE(DATA_OFFSET)"/16), %ax\n\t"
	"movw %ax, %ds\n\t" /*установка сегмента %ds*/
	"jmp load"
);


void load(){
	reset_disk();
	if(!try_load_sectors(0, 0, STAGE2_START_SECT, STAGE2_NUM_SECTS, STAGE2_START, 0, DEFAULT_RETRY_TIMES)){
		goto error;
	}
	/*Переход ко второй стадии*/
	asm volatile("jmpl $0, $("PASTE(STAGE2_START)")");
	error:
	print("Disk error");
	stop_floppy();
	stop_cpu();
}  
