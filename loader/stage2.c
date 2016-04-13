#include "common.h"
#include "common2.h"
#include "stages.h"

#define DATA_OFFSET STAGE2_START

asm (
	"movw $("PASTE(DATA_OFFSET)"/16), %ax\n\t"
	"movw %ax, %ds\n\t" /*установка сегмента %ds*/
	"jmp load2"
);

void load2(){
	/*Определяем число головок и секторов в цилиндре*/
	uint8_t max_heads, max_sectors;
	get_drive_params(&max_heads, &max_sectors);
	/*Сначала нужно считать размер главной программы, потом уже её саму*/
	if(!try_load_sectors(0, 0, MAIN_START_SECT, 1, MAIN_CODE_START, 0, DEFAULT_RETRY_TIMES)){
		goto error;
	}
	/*Размер главной программы*/
	/*Учитывается то, что %ds не равно 0*/
	uint16_t num_main_sect=*((uint16_t*)(MAIN_CODE_START-DATA_OFFSET));
	uint16_t i,j;
	uint16_t start_sect,num_read_sect,load_addr=0;
	/*По всем цилиндрам и головкам дискеты*/	
	for(i=0; ;i++){
		for(j=0; j<=max_heads; j++){
			/*Если мы на первом цилиндре и головке*/
			if(i==0 && j==0){
				/*начинаем считывать не с первого сектора, так как там загрузчик*/
				start_sect=MAIN_START_SECT;
			} else {
				start_sect=1;
			}
			/*num_main_sect -- число секторов, которые осталось считать*/
			/*num_read_sect -- число секторов, которые считываются на этой итерации*/

			/*Если все секторы, которые нужно считать, не умещаются на текущем цилиндре*/
			if(num_main_sect>(max_sectors+1-start_sect)){
				num_read_sect=(max_sectors+1-start_sect);
				num_main_sect-=num_read_sect;			
			} else {
				num_read_sect=num_main_sect;
				num_main_sect=0;
			}
			if(!try_load_sectors(
				i, j, start_sect, num_read_sect, 
				MAIN_CODE_START, load_addr, 
				DEFAULT_RETRY_TIMES
			)){
				goto error;
			}
			load_addr+=NUM_BYTES_IN_SECTOR*num_read_sect;
			if(num_main_sect==0){
				goto success;
			}
		}
	}
	success:
	stop_floppy();
	/*Переход к основной программе, с изменением %cs*/
	asm volatile("jmpl $("PASTE(MAIN_CODE_START)"/16), $("PASTE(SIZEOF_MAIN_SIZE+SIZEOF_DATA_LOC)")");
	error:
	print("Disk error2");
	stop_floppy();
	stop_cpu();
}
