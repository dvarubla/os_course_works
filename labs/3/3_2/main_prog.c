#include "stages.h"
#include "console_io.h"
#include "common_prog.h"
#include "interrupts.h"
#include "math.h"

#define TICKS_IN_SEC 18.2
/*интервал, через который проводится корректировка времени*/
#define CORRECT_INTERVAL 5
#define TIME_LEN 8
#define DEF_X 0
#define DEF_Y 0
#define DEF_SHOW_INTERVAL 1

typedef struct Clock{
	/*величина дискрета*/
	uint16_t show_interval;
	uint16_t show_tick_counter;
	uint16_t correct_tick_counter;
	uint8_t cursor_x;
	uint8_t cursor_y;
	Time time;
} Clock;

#define NUM_CLOCKS 2

Clock clocks[NUM_CLOCKS];

/*величина, обратная вероятности появления вторых часов*/
#define CLOCK2_SHOW_INV_PROB 10 

bool clock2_shown;

/*добавить 1 тик ко времени*/
void add_tick_to_time(Time *time){
	if(time->ticks==(uint8_t)(TICKS_IN_SEC-1)){
		time->ticks=0;
		if(time->seconds==(SEC_IN_MIN-1)){
			time->seconds=0;
			if(time->minutes==(SEC_IN_MIN-1)){
				time->minutes=0;
				if(time->hours==(MIN_IN_HOUR-1)){
					time->hours=0;
				} else {
					time->hours++;
				}
			} else {
				time->minutes++;
			}
		} else {
			time->seconds++;
		}
	} else {
		time->ticks++;
	}
}

void process_tick_for_clock(uint8_t clock_index){
	clocks[clock_index].show_tick_counter++;
	clocks[clock_index].correct_tick_counter++;
	add_tick_to_time(&clocks[clock_index].time);
	/*корректировка времени*/
	if(clocks[clock_index].correct_tick_counter==CORRECT_INTERVAL){
		clocks[clock_index].correct_tick_counter=0;
		add_tick_to_time(&clocks[clock_index].time);
	}
	if(clocks[clock_index].show_tick_counter==(uint8_t)((float)clocks[clock_index].show_interval*TICKS_IN_SEC)){
		clocks[clock_index].show_tick_counter=0;
		uint8_t old_cursor_x, old_cursor_y, old_start_line, old_end_line;
		get_cursor_pos_and_size(&old_cursor_x, &old_cursor_y, &old_start_line, &old_end_line);
		hide_cursor();	
		set_cursor_pos(clocks[clock_index].cursor_x, clocks[clock_index].cursor_y);
		printf(
			"%02u:%02u:%02u", 
			clocks[clock_index].time.hours, 
			clocks[clock_index].time.minutes, 
			clocks[clock_index].time.seconds
		);
		set_cursor_pos(old_cursor_x, old_cursor_y);
		set_cursor_size(old_start_line, old_end_line);
	}
}

void int1C_handler_func(){
	process_tick_for_clock(0);
	if(!clock2_shown && rand_uint32()%CLOCK2_SHOW_INV_PROB==0){
		get_time(&clocks[1].time);
		clock2_shown=true;
	}
	if(clock2_shown){
		process_tick_for_clock(1);
	}
}

INTERRUPT_ROUTINE(1C, int1C_handler_func);

void read_params(){
	bool status;
	print("\r\nEnter x, y: ");
	uint8_t coords[2];
	status=read_numbers(coords, 2, 10, UINT8_ACT);
	if(status && coords[0]<=(SCREEN_COLS-TIME_LEN) && coords[1]<(SCREEN_ROWS-NUM_CLOCKS+1)){
		for(uint8_t i=0; i<NUM_CLOCKS; i++){
			clocks[i].cursor_x=coords[0];
			clocks[i].cursor_y=coords[1]+i;
		}
	} else {
		for(uint8_t i=0; i<NUM_CLOCKS; i++){
			clocks[i].cursor_x=DEF_X;
			clocks[i].cursor_y=DEF_Y+i;
		}
		print("Incorrect values. X set to "PASTE(DEF_X)", y - to "PASTE(DEF_Y)".\r\n");
	}
	uint16_t intervals[NUM_CLOCKS];
	print("Enter show intervals: ");
	status=read_numbers(&intervals, NUM_CLOCKS, 10, UINT16_ACT);
	if(status){
		for(uint8_t i=0; i<NUM_CLOCKS; i++){
			clocks[i].show_interval=intervals[i];
		}
	} else {
		for(uint8_t i=0; i<NUM_CLOCKS; i++){
			clocks[i].show_interval=DEF_SHOW_INTERVAL;
		};
		print("Incorrect value. Show intervals set to "PASTE(DEF_SHOW_INTERVAL)".\r\n");
	}
	
}

int main(){
	set_mode();
	for(uint8_t i=0; i<NUM_CLOCKS; i++){
		clocks[i].correct_tick_counter=clocks[i].show_tick_counter=0;
	}
	print_color("Lab 3", YELLOW);
	read_params();
	get_time(&clocks[0].time);
	set_int_addr_func(0x1C, (uint32_t)int1C_handler);
	while(1){
		while(has_keys()){
			read_char();
		}
	}
}
