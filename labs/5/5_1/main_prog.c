#include "stages.h"
#include "console_io.h"
#include "common_prog.h"
#include "interrupts.h"

#define VIDEO_MEMORY_SEG 0xB800

#define RECT_WIDTH 2
#define RECT_HEIGHT 5
#define RECT_COLOR LIGHT_RED

#define RECT_DX 1
#define RECT_DY 2

#define MIN_WAIT 20
#define DEF_SPEED 10
/*текущее значение сегмента %ds*/
uint16_t ds_offset;

typedef struct __attribute__((__packed__)) VideoSym{
	char symbol;
	uint8_t attr;
} VideoSym;

/*символы, которые находятся "под" прямоугольником*/
VideoSym rect_syms[RECT_HEIGHT][RECT_WIDTH];

int8_t rect_x, rect_y;
int8_t rect_cur_dx, rect_cur_dy;

bool rect_shown;

FullAddr prev_int9_addr, prev_int1C_addr;

uint16_t tick_counter;
uint16_t speed;

void draw_rect(uint8_t x, uint8_t y){
	VideoSym (*video_syms)[SCREEN_COLS]=0;
	/*переменная должна быть на стеке, 
	чтобы к ней можно было обратиться при любом значении %ds*/
	uint16_t ds_offset_copy=ds_offset;
	/*смена сегмента*/
	asm volatile(
	"movw %w0, %%ds\n\t"
	: :"r"(VIDEO_MEMORY_SEG): "memory"
	);
	for(uint8_t i=0; i<RECT_HEIGHT; i++){
		for(uint8_t j=0; j<RECT_WIDTH; j++){
			video_syms[y+i][x+j].symbol=' ';
			video_syms[y+i][x+j].attr=RECT_COLOR<<4;
		}
	}
	/*вернуть старое значение сегмента*/
	asm volatile(
	"movw %w0, %%ds\n\t" 
	: :"r"(ds_offset_copy): "memory"
	);
}

void get_syms_for_rect(uint8_t x, uint8_t y){
	VideoSym (*video_syms)[SCREEN_COLS]=0;
	uint8_t t_attr;
	char t_symbol;
	uint16_t ds_offset_copy=ds_offset;
	for(uint8_t i=0; i<RECT_HEIGHT; i++){
		for(uint8_t j=0; j<RECT_WIDTH; j++){
			asm volatile(
			"movw %w0, %%ds\n\t" 
			: :"r"(VIDEO_MEMORY_SEG): "memory"
			);
			t_symbol=video_syms[y+i][x+j].symbol;
			t_attr=video_syms[y+i][x+j].attr;
			asm volatile(
			"movw %w0, %%ds\n\t" 
			: :"r"(ds_offset_copy): "memory"
			);
			rect_syms[i][j].symbol=t_symbol;
			rect_syms[i][j].attr=t_attr;
		}
	}
	
}

void print_syms_for_rect(uint8_t x, uint8_t y){
	VideoSym (*video_syms)[SCREEN_COLS]=0;
	uint8_t t_attr;
	char t_symbol;
	uint16_t ds_offset_copy=ds_offset;
	for(uint8_t i=0; i<RECT_HEIGHT; i++){
		for(uint8_t j=0; j<RECT_WIDTH; j++){
			t_symbol=rect_syms[i][j].symbol;
			t_attr=rect_syms[i][j].attr;
			asm volatile(
			"movw %w0, %%ds\n\t" 
			: :"r"(VIDEO_MEMORY_SEG): "memory"
			);
			video_syms[y+i][x+j].symbol=t_symbol;
			video_syms[y+i][x+j].attr=t_attr;
			asm volatile(
			"movw %w0, %%ds\n\t" 
			: :"r"(ds_offset_copy) : "memory"
			);
		}
	}
}

#define RELEASED_FLAG 0x80
#define ENTER 0x1C
#define ESC 0x01

void int9_handler_func(){
	uint8_t key=inb(0x60);
	bool pressed=((key&RELEASED_FLAG)==0);
	if(pressed){
		if(rect_shown){
			print_syms_for_rect(rect_x, rect_y);
			rect_shown=false;
		}
	}
	if(pressed && key==ESC){
		set_int_addr(9, &prev_int9_addr);
		set_int_addr(0x1C, &prev_int1C_addr);
	}
	call_int_addr(&prev_int9_addr);
}

void int1C_handler_func(){
	tick_counter++;
	bool need_repaint=!rect_shown;
	if(tick_counter==(MIN_WAIT-speed)){
		tick_counter=0;
		if(rect_shown){
			print_syms_for_rect(rect_x, rect_y);
			rect_shown=false;
		}
		rect_x+=rect_cur_dx;
		if(rect_x<=0){
			rect_x=0;
			rect_cur_dx*=-1;
		} else if(rect_x>=(SCREEN_COLS-RECT_WIDTH)){
			rect_x=SCREEN_COLS-RECT_WIDTH;
			rect_cur_dx*=-1;
		}
		rect_y+=rect_cur_dy;
		if(rect_y<=0){
			rect_y=0;
			rect_cur_dy*=-1;
		} else if(rect_y>=(SCREEN_ROWS-RECT_HEIGHT)){
			rect_y=SCREEN_ROWS-RECT_HEIGHT;
			rect_cur_dy*=-1;
		}
		need_repaint=true;
	}
	if(need_repaint){
		get_syms_for_rect(rect_x,rect_y);
		draw_rect(rect_x, rect_y);
		rect_shown=true;
	}
}

INTERRUPT_ROUTINE(9, int9_handler_func)
INTERRUPT_ROUTINE(1C, int1C_handler_func)

void read_params(){
	bool status;
	uint16_t num;
	print("\r\nEnter speed: ");
	status=read_number(&num, UINT16_ACT);
	if(status && num<MIN_WAIT){
		speed=num;
	} else {
		speed=DEF_SPEED;
		print("Incorrect value. Speed set to "PASTE(DEF_SPEED)".\r\n");
	}
}

int main(){
	asm volatile(
	"movw %%ds, %0"
	:"=a"(ds_offset)
	);
	tick_counter=0;
	rect_cur_dx=RECT_DX;
	rect_cur_dy=RECT_DY;

	set_mode();
	disable_blink();
	print_color("Lab 5", YELLOW);
	read_params();

	print("12345678901234567890\r\n12345678901234567890\r\n");
	rect_x=0;
	rect_y=0;
	get_syms_for_rect(rect_x, rect_y);
	draw_rect(rect_x, rect_y);
	rect_shown=true;

	get_int_addr(9, &prev_int9_addr);
	get_int_addr(0x1C, &prev_int1C_addr);
	set_int_addr_func(9, (uint32_t)int9_handler);
	set_int_addr_func(0x1C, (uint32_t)int1C_handler);
	enable_int();

	while(1){
		while(has_keys()){
			read_char();
		}
	}
}
