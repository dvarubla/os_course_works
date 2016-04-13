#include "console_io.h"
#include "common_prog.h"
#include "main_prog.h"

#define VIDEO_MEMORY_SEG 0xB800

#define RECT_WIDTH 2
#define RECT_HEIGHT 5
#define RECT_COLOR LIGHT_RED

#define RECT_DX 1
#define RECT_DY 2

#define MIN_WAIT 40
/*множитель для вычисления числа тиков по величине кванта*/
#define SLICE_MULT 4/5

/*текущее значение сегмента %ds*/
static uint16_t ds_offset;

/*символы, которые находятся "под" прямоугольником*/
static VideoSym rect_syms[RECT_HEIGHT][RECT_WIDTH];

static int8_t rect_x, rect_y;
static int8_t rect_cur_dx, rect_cur_dy;

static uint16_t tick_counter;

static uint8_t x_min, x_max, y_min, y_max;
/*двигается ли объект свободно или управляется с клавиатуры*/
static bool automatic_movement;

static void draw_rect(uint8_t x, uint8_t y){
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

static void get_syms_for_rect(uint8_t x, uint8_t y){
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

static void print_syms_for_rect(uint8_t x, uint8_t y){
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
/*проверить, выходит ли прямоугольник за пределы экрана*/
void check_rect_coords(bool change_dirs){
	if(rect_x<=x_min){
		rect_x=x_min;
		if(change_dirs){
			rect_cur_dx*=-1;
		}
	} else if(rect_x>=(x_max-RECT_WIDTH+1)){
		rect_x=x_max-RECT_WIDTH+1;
		if(change_dirs){
			rect_cur_dx*=-1;
		}
	}
	if(rect_y<=y_min){
		rect_y=y_min;
		if(change_dirs){
			rect_cur_dy*=-1;
		}
	} else if(rect_y>=(y_max-RECT_HEIGHT+1)){
		rect_y=y_max-RECT_HEIGHT+1;
		if(change_dirs){
			rect_cur_dy*=-1;
		}
	}
}

bool automatic_movement_needs_update;

static void print_automatic_movement_status(){
	set_cursor_pos(27, SCREEN_ROWS-5-1-1);
	printf(
		"%C%3s", 
		(automatic_movement)?LIGHT_GREEN:LIGHT_RED, 
		(automatic_movement)?"on":"off"
	);
}

static void print_info(){
	set_cursor_pos(1, SCREEN_ROWS-5-1-1);
	printf("%CLeft %CA D %CRight", BRIGHT_WHITE, LIGHT_CYAN, BRIGHT_WHITE);
	set_cursor_pos(3, SCREEN_ROWS-5-1-2);
	printf("%CTop %CW", BRIGHT_WHITE, LIGHT_CYAN);
	set_cursor_pos(0, SCREEN_ROWS-5-1);
	printf("%CBottom %CS", BRIGHT_WHITE, LIGHT_CYAN);
	set_cursor_pos(3, SCREEN_ROWS-5-1-2);
	
	set_cursor_pos(18, SCREEN_ROWS-5-1-2);
	print("Automatic");
	set_cursor_pos(18, SCREEN_ROWS-5-1-1);
	print("movement");
	set_cursor_pos(18, SCREEN_ROWS-5-1);
	printf("%CEnter%C=Toggle", LIGHT_CYAN, BRIGHT_WHITE);
}

#define ENTER 0x1C
#define KEY_W 0x11
#define KEY_A 0x1E
#define KEY_S 0x1F
#define KEY_D 0x20

void move_obj_key_handler(uint8_t key, bool pressed){
	if(pressed && key==ENTER){
		automatic_movement=!automatic_movement;
		automatic_movement_needs_update=true;
	}
	if(
		!automatic_movement && pressed && 
		(key==KEY_W || key==KEY_A || key==KEY_S || key==KEY_D)
	){
		print_syms_for_rect(rect_x, rect_y);
		switch(key){
			case KEY_W: 
				rect_y-=RECT_DY;
			break;
			case KEY_A:
				rect_x-=RECT_DX;
			break;
			case KEY_S: 
				rect_y+=RECT_DY;
			break;
			case KEY_D:
				rect_x+=RECT_DX;
			break;
		}
		check_rect_coords(false);
		get_syms_for_rect(rect_x,rect_y);
		draw_rect(rect_x, rect_y);
	}
}

void move_obj_task(){
	if(automatic_movement_needs_update && get_can_write_to_screen()){
		print_automatic_movement_status();
		automatic_movement_needs_update=false;
	}
	if(
		is_program_finished() || 
		get_thread_status(MOVE_OBJ_THREAD)==PAUSED ||
		!automatic_movement
	){
		return;
	}
	tick_counter++;
	if(tick_counter>=(MIN_WAIT+get_time_slice(MOVE_OBJ_THREAD)*SLICE_MULT)){
		tick_counter=0;
		print_syms_for_rect(rect_x, rect_y);
		rect_x+=rect_cur_dx;
		rect_y+=rect_cur_dy;
		check_rect_coords(true);
		get_syms_for_rect(rect_x,rect_y);
		draw_rect(rect_x, rect_y);
	}
}

void move_obj_init(){
	asm volatile(
	"movw %%ds, %0"
	:"=a"(ds_offset)
	);
	x_min=0;
	x_max=SCREEN_COLS*2/5-1-1;
	y_min=2;
	y_max=SCREEN_ROWS-5-1-4;
	automatic_movement=true;
	tick_counter=0;
	rect_cur_dx=RECT_DX;
	rect_cur_dy=RECT_DY;
	rect_x=x_min;
	rect_y=y_min;
	
	set_cursor_pos(x_min, y_min);
	print("Example text.\r\n" "It will not be modified.");

	automatic_movement_needs_update=false;
	print_info();
	print_automatic_movement_status();

	get_syms_for_rect(rect_x, rect_y);
	draw_rect(rect_x, rect_y);	
}
