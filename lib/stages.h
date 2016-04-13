#ifndef _STAGES_H_
#define _STAGES_H_

/*Размер загрузочного сектора*/
#define BOOTSECTOR_SIZE 512
/*Размер стека*/
#define STACK_SIZE 1024
/*Размер кода второй стадии*/
#define STAGE2_SIZE 1024
/*Начальное значение %sp. (%ss -- совпадает с началом второй стадии)*/
#define INITIAL_SP (STACK_SIZE+STAGE2_SIZE)
/*Размер размера главной программы*/
#define SIZEOF_MAIN_SIZE 2
/*Размер размера значения сегмента %ds для главной программы*/
#define SIZEOF_DATA_LOC 2

/*Число секторов второй стадии*/
#define STAGE2_NUM_SECTS 2

/*Начала всех частей программы*/
#define BOOTSECTOR_OFFSET 0x7C00
#define STAGE2_START (BOOTSECTOR_OFFSET+BOOTSECTOR_SIZE)
#define STACK_START (STAGE2_START)
#define MAIN_CODE_START (STACK_START+INITIAL_SP)
/*Где компоновщик помещает значение %ds*/
#define DATA_LOC (MAIN_CODE_START+SIZEOF_MAIN_SIZE)

/*Начальный сектор второй стадии*/
#define STAGE2_START_SECT 2
/*Начальный сектор основного кода*/
#define MAIN_START_SECT (STAGE2_START_SECT+STAGE2_NUM_SECTS)

#endif

