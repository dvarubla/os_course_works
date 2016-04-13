#include "common_prog.h"
#include "interrupts.h"
#include "console_io.h"
#include "stages.h"
#include "atomic.h"
#include "string.h"

#include "main_prog.h"
#include "gen_primes.h"
#include "play_melody.h"
#include "move_obj.h"
#include "print_info.h"

static FullAddr prev_int8_addr;
//static FullAddr prev_int9_addr;

/*Цвета и буквы, соответствующие статусу потока*/
static struct{
	char letter;
	uint8_t color;
} status_letters[]={{'A', LIGHT_GREEN}, {'P', LIGHT_RED}, {'W', YELLOW}};

#define MIN_TIME_SLICE 1
#define MAX_TIME_SLICE 99

static struct ThreadInfo{
	ThreadStatus status;
	uint8_t time_slice;
	bool need_update;
} thread_info_arr[NUM_ALL_THREADS];

/*предыдущие статусы потоков производителя и потребителя*/
ThreadStatus prev_gen_primes_statuses[2];

void set_thread_status(ThreadId thread_id, ThreadStatus status){
	if(status==WAITING && thread_info_arr[thread_id].status==PAUSED){
		return;
	}
	thread_info_arr[thread_id].status=status;
	thread_info_arr[thread_id].need_update=true;
}

ThreadStatus get_thread_status(ThreadId thread_id){
	return thread_info_arr[thread_id].status;
}

uint8_t get_time_slice(ThreadId thread_id){
	return thread_info_arr[thread_id].time_slice;
}

static bool program_finished;

bool is_program_finished(){
	return program_finished;
}

static uint32_t can_write_to_screen;

bool get_can_write_to_screen(){
	return can_write_to_screen;
}

/*вызывается производителем и потребителем*/
void set_can_write_to_screen(bool val){
	atomic_write(&can_write_to_screen, val);
}

typedef enum LineType{
	LINE_HORIZONTAL,
	LINE_VERTICAL
} LineType;

typedef struct Line{
	LineType type;
	uint8_t start;
	uint8_t end;
	uint8_t another_coordinate;
} Line;
/*Линии - границы между окнами*/
static Line lines[]={
	{LINE_HORIZONTAL, 0, SCREEN_COLS-1, 1},
	{LINE_HORIZONTAL, 0, SCREEN_COLS-1, SCREEN_ROWS-5},
	{LINE_HORIZONTAL, 0, SCREEN_COLS-1, SCREEN_ROWS-2},
	{LINE_VERTICAL, 2, SCREEN_ROWS-6, SCREEN_COLS*2/5-1},
	{LINE_VERTICAL, 0, 0, SCREEN_COLS-1-7},
	{LINE_HORIZONTAL, 0, SCREEN_COLS*2/5-2, SCREEN_ROWS-5-1-3}
};

#define NUM_LINES (sizeof(lines)/sizeof(Line))
#define HORIZONTAL_LINE_SYM '-'
#define VERTICAL_LINE_SYM '|'
#define LINE_COLOR BRIGHT_WHITE<<4|BLACK

static void draw_lines(){
	for(uint8_t i=0; i<NUM_LINES; i++){
		if(lines[i].type==LINE_HORIZONTAL){
			set_cursor_pos(lines[i].start, lines[i].another_coordinate);
			for(uint8_t j=lines[i].start; j<=lines[i].end; j++){
				print_char_color(HORIZONTAL_LINE_SYM, LINE_COLOR);
			}
		} else if(lines[i].type==LINE_VERTICAL){
			for(uint8_t j=lines[i].start; j<=lines[i].end; j++){
				set_cursor_pos(lines[i].another_coordinate, j);
				print_char_color(VERTICAL_LINE_SYM, LINE_COLOR);
			}
		}
	}
}

#define STATUS_BLOCK_WIDTH 6
/*область экрана, где выводится номер, статус и квант потока*/
static TextBlock status_blocks[NUM_ALL_THREADS];

static void update_status(ThreadId thread_id){
	printf_in_block(
		&status_blocks[thread_id], 
		"%C%u %C%c %C%2u", 
		BRIGHT_WHITE, thread_id+1,	
		status_letters[thread_info_arr[thread_id].status].color,
		status_letters[thread_info_arr[thread_id].status].letter, 
		BRIGHT_WHITE, thread_info_arr[thread_id].time_slice
	);
}

static char *thread_titles[]={"Melody", "Primes produce", "Primes consume", "Print info", "Move obj"};

/*счётчик для вызова обработчика прерывания bios*/
static uint16_t call_bios_handler_counter;

static uint16_t cur_esp;

void int8_handler_func(){
	call_bios_handler_counter++;
	play_melody_task();
	move_obj_task();
	print_info_task();
	/*завершились ли потоки производителя и потребителя*/
	bool threads_finished=false;
	cur_esp=gen_primes_task(cur_esp, &threads_finished);
	/*сразу на экран вывести нельзя, так как выводить на экран могут
	производитель и потребитель
	*/
	if(can_write_to_screen){
		for(uint8_t i=0; i<NUM_ALL_THREADS; i++){
			if(thread_info_arr[i].need_update){
				update_status(i);
				thread_info_arr[i].need_update=false;
			}
		}
	}
	if(threads_finished){
		stop_cpu();
	}
	/*нужно ли вызвать старый обработчик прерывания*/
	if(call_bios_handler_counter==(1<<COUNTER_INCR)){
		call_bios_handler_counter=0;
		call_int_addr(&prev_int8_addr);
	} else {
		outb(0x20, 0x20);
	}
}

void int8_handler();
asm(
	"int8_handler:\n\t"
	"pushl %ebp\n\t"
	"movl %esp, %ebp\n\t"
	
	"andw $(~0xF), %sp\n\t"
	"subw $512, %sp\n\t"
	"fnsave (%esp)\n\t"
	"pushw %ds\n\t"
	"pushal\n\t"

	"xorw %ax, %ax\n\t"
	"movw %ax, %ds\n\t"
	"movw ("PASTE(DATA_LOC)"), %ax\n\t"
	"movw %ax, %ds\n\t"
	
	"movw %sp, cur_esp\n\t"

	"call int8_handler_func\n\t"
	/*переключение стека*/
	"movw cur_esp, %sp\n\t"
	
	/*выталкиваем значения уже из другого стека*/
	"popal\n\t"
	"popw %ds\n\t"
	"frstor (%esp)\n\t"
	"addw $(512), %sp\n\t"
	"leave\n\t"
	"iretw\n\t"
);



#define ESC 0x01
#define RELEASED_FLAG 0x80
#define LSHIFT 0x2A
#define LALT 0x38
#define LCTRL 0x1D
#define KEY1 0x02

static bool ctrl_pressed, shift_pressed, alt_pressed;

void int9_handler_func(){
	if(!program_finished){
		uint8_t key=inb(0x60);
		//outb(0x64, 0xD2);
		//while(inb(0x64)&0b10);
		//outb(0x60, key);
		uint8_t cleared_key=key;
		bool pressed=((key&RELEASED_FLAG)==0);
		if(!pressed){
			cleared_key^=RELEASED_FLAG;
		}
		switch(cleared_key){
			case LSHIFT: shift_pressed=pressed; break;
			case LCTRL: ctrl_pressed=pressed; break;
			case LALT: alt_pressed=pressed; break;
			case ESC: 
				program_finished=true;
				/*нужно завершить все потоки, кроме производителя и потребителя*/ 
				for(uint8_t i=0; i<NUM_ALL_THREADS; i++){
					if(
						i!=GEN_PRIMES_PRODUCER_THREAD && 
						i!=GEN_PRIMES_CONSUMER_THREAD
					){
						set_thread_status(i, PAUSED);						
					} else if(thread_info_arr[i].status!=WAITING){
						set_thread_status(i, ACTIVE);
					}
				}
			break;
		}
		/*увеличение кванта, уменьшение кванта, приостановка потока*/
		if(pressed && cleared_key>=KEY1 && cleared_key<=KEY1+NUM_ALL_THREADS-1){
			uint8_t thread_id=cleared_key-KEY1;
			if(ctrl_pressed && thread_info_arr[thread_id].time_slice<MAX_TIME_SLICE){
				thread_info_arr[thread_id].time_slice++;			
				thread_info_arr[thread_id].need_update=true;
			} else if(shift_pressed && thread_info_arr[thread_id].time_slice>MIN_TIME_SLICE){
				thread_info_arr[thread_id].time_slice--;
				thread_info_arr[thread_id].need_update=true;
			} else if(alt_pressed){
				/*у этих потоков нужно сохранить или восстановить прежний статус*/
				if(
					thread_id==GEN_PRIMES_PRODUCER_THREAD || 
					thread_id==GEN_PRIMES_CONSUMER_THREAD
				){
					if(thread_info_arr[thread_id].status==PAUSED){
						thread_info_arr[thread_id].status=
						prev_gen_primes_statuses[thread_id-GEN_PRIMES_PRODUCER_THREAD];
					} else {
						prev_gen_primes_statuses[thread_id-GEN_PRIMES_PRODUCER_THREAD]=
						thread_info_arr[thread_id].status;
						thread_info_arr[thread_id].status=PAUSED;
					}
					
				} else {
					thread_info_arr[thread_id].status=
					(thread_info_arr[thread_id].status==PAUSED)?ACTIVE:PAUSED;
				}
				thread_info_arr[thread_id].need_update=true;
				/*если все потоки приостановлены, значит, нужно завершить программу*/
				uint8_t i;
				for(i=0; i<NUM_ALL_THREADS; i++){
					if(thread_info_arr[i].status!=PAUSED){
						break;
					}
				}
				/*нужно возобновить потоки производителя и потребителя*/
				if(i==NUM_ALL_THREADS){
					set_thread_status(GEN_PRIMES_PRODUCER_THREAD, ACTIVE);
					set_thread_status(GEN_PRIMES_CONSUMER_THREAD, ACTIVE);
					program_finished=true;
				}
			}
		}
		move_obj_key_handler(cleared_key, pressed);
	}
	outb(0x20, 0x20);
	//call_int_addr(&prev_int9_addr);
}

INTERRUPT_ROUTINE(9, int9_handler_func);
/*инициализировать структуры с информацией и вывести данные*/
static void init_info(){
	uint8_t prev_w=0;
	for(uint8_t i=0; i<NUM_ALL_THREADS; i++){
		thread_info_arr[i].status=ACTIVE;
		thread_info_arr[i].time_slice=DEFAULT_TIME_SLICE;
		thread_info_arr[i].need_update=false;

		status_blocks[i].cursor_x=status_blocks[i].x_min=prev_w;
		prev_w=status_blocks[i].x_max=status_blocks[i].x_min+STATUS_BLOCK_WIDTH-1;
		prev_w+=MAX(0, (int8_t)strlen(thread_titles[i])-STATUS_BLOCK_WIDTH+1)+1;
		status_blocks[i].cursor_y=status_blocks[i].y_min=SCREEN_ROWS-3;
		status_blocks[i].y_max=SCREEN_ROWS-3;
		status_blocks[i].cursor_stored=true;
		update_status(i);
	}
	prev_gen_primes_statuses[0]=prev_gen_primes_statuses[1]=ACTIVE;
	set_cursor_pos(0, SCREEN_ROWS-4);

	for(uint8_t i=0; i<NUM_ALL_THREADS; i++){
		printf("%s ", thread_titles[i]);
	}
	/*информация об управляющих клавишах*/
	set_cursor_pos(prev_w, SCREEN_ROWS-4);
	printf("%CEsc%C=Finish %CLAlt+?%C=Pause", LIGHT_CYAN, BRIGHT_WHITE, LIGHT_CYAN, BRIGHT_WHITE);
	set_cursor_pos(prev_w, SCREEN_ROWS-3);
	printf("%CLCtrl+?%C=+ %CLShift+?%C=-", LIGHT_CYAN, BRIGHT_WHITE, LIGHT_CYAN, BRIGHT_WHITE);
}

#define DEF_TOP_LINE true
/*выводится ли информация о работе в верхней строке*/
bool top_line;

static void read_params(){
	uint8_t num;
	print("Top line or bottom line (0/1): ");
	bool status=read_number(&num, UINT8_ACT);
	if(status && num<=1){
		top_line=(num==0);
	} else {
		top_line=DEF_TOP_LINE;
		printf("Incorrect value. Info is displayed in the %s line\r\n", (DEF_TOP_LINE)?"top":"bottom");
	}
	print("Print enter: ");
	get_key();
}

void main(){
	set_mode();
	ctrl_pressed=shift_pressed=alt_pressed=false;
	program_finished=false;
	can_write_to_screen=true;

	call_bios_handler_counter=0;
	print_color("Lab 4\r\n", YELLOW);
	read_params();
	
	clear_screen();
	disable_blink();	
	
	gen_primes_init();
	play_melody_init();
	move_obj_init();
	print_info_init(top_line?0:SCREEN_ROWS-1);	
	
	init_info();	

	hide_cursor();
	draw_lines();

	get_int_addr(8, &prev_int8_addr);
	//get_int_addr(9, &prev_int9_addr);
	/*ускорение таймера*/
	uint16_t new_freq_div=(COUNTER_INCR==0)?0:1<<(16-COUNTER_INCR);
	disable_int();
	outb(0x40, new_freq_div&0xFF);
	outb(0x40, new_freq_div>>8);
	set_int_addr_func(8, (uint32_t)int8_handler);
	set_int_addr_func(9, (uint32_t)int9_handler);
	enable_int();

	while(1);
}
