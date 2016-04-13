#include "print_info.h"
#include "console_io.h"
#include "main_prog.h"
#include "string.h"

#define VIDEO_MEMORY_SEG 0xB800
#define MIN_WAIT 20
/*множитель для вычисления числа тиков по величине кванта*/
#define SLICE_MULT 1

static char *info_str="Borisov Alexey course work";
static uint8_t info_x, info_y;

static uint16_t tick_counter;

/*текущее значение сегмента %ds*/
static uint16_t ds_offset;
/*цвет строки с информацией*/
static uint8_t info_color;

static void print_info_str(uint8_t x, uint8_t y, uint8_t color){
	VideoSym (*video_syms)[SCREEN_COLS]=0;
	/*переменная должна быть на стеке, 
	чтобы к ней можно было обратиться при любом значении %ds*/
	uint16_t ds_offset_copy=ds_offset;
	uint8_t info_str_len=strlen(info_str);
	/*смена сегмента*/
	asm volatile(
	"movw %w0, %%ds\n\t"
	: :"r"(VIDEO_MEMORY_SEG): "memory"
	);
	for(uint8_t i=0; i<info_str_len; i++){
		video_syms[y][x+i].attr=color;
	}
	/*вернуть старое значение сегмента*/
	asm volatile(
	"movw %w0, %%ds\n\t" 
	: :"r"(ds_offset_copy): "memory"
	);
}

void print_info_task(){
	if(is_program_finished() || get_thread_status(PRINT_INFO_THREAD)==PAUSED){
		return;
	}
	tick_counter++;
	if(tick_counter>=(MIN_WAIT+get_time_slice(PRINT_INFO_THREAD)*SLICE_MULT)){
		tick_counter=0;
		info_color++;
		if(info_color==NUM_COLORS){
			info_color=BLACK;
		}
		print_info_str(info_x, info_y, info_color);
	}
}

void print_info_init(uint8_t y){
	asm volatile(
	"movw %%ds, %0"
	:"=a"(ds_offset)
	);
	info_x=0;
	info_y=y;
	set_cursor_pos(info_x, info_y);
	/*один раз выводим информацию, дальше меняется только цвет*/
	print(info_str);
	info_color=BLACK;
	tick_counter=0;
}
