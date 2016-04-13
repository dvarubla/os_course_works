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

uint16_t 
	/*величина дискрета*/
	show_interval, 
	show_tick_counter,
	correct_tick_counter;

uint8_t cursor_x, cursor_y;

Time time;

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

void int1C_handler_func(){
	show_tick_counter++;
	correct_tick_counter++;
	add_tick_to_time(&time);
	/*корректировка времени*/
	if(correct_tick_counter==CORRECT_INTERVAL){
		correct_tick_counter=0;
		add_tick_to_time(&time);
	}
	if(show_tick_counter==(uint8_t)((float)show_interval*TICKS_IN_SEC)){
		show_tick_counter=0;
		uint8_t old_cursor_x, old_cursor_y, old_start_line, old_end_line;
		get_cursor_pos_and_size(&old_cursor_x, &old_cursor_y, &old_start_line, &old_end_line);
		hide_cursor();	
		set_cursor_pos(cursor_x, cursor_y);
		printf("%02u:%02u:%02u", time.hours, time.minutes, time.seconds);
		set_cursor_pos(old_cursor_x, old_cursor_y);
		set_cursor_size(old_start_line, old_end_line);
	}
}

INTERRUPT_ROUTINE(1C, int1C_handler_func);

void read_params(){
	bool status;
	uint16_t num;
	print("\r\nEnter x, y: ");
	uint8_t coords[2];
	status=read_numbers(coords, 2, 10, UINT8_ACT);
	if(status && coords[0]<=(SCREEN_COLS-TIME_LEN) && coords[1]<SCREEN_ROWS){
		cursor_x=coords[0];	
		cursor_y=coords[1];
	} else {
		cursor_x=DEF_X;
		cursor_y=DEF_Y;
		print("Incorrect values. X set to "PASTE(DEF_X)", y - to "PASTE(DEF_Y)".\r\n");
	}
	print("Enter show interval: ");
	status=read_number(&num, UINT16_ACT);
	if(status){
		show_interval=num;
	} else {
		show_interval=DEF_SHOW_INTERVAL;
		print("Incorrect value. Show interval set to "PASTE(DEF_SHOW_INTERVAL)".\r\n");
	}
	
}

int main(){
	set_mode();
	correct_tick_counter=show_tick_counter=0;
	print_color("Lab 3", YELLOW);
	read_params();
	get_time(&time);
	set_int_addr_func(0x1C, (uint32_t)int1C_handler);
	while(1){
		while(has_keys()){
			read_char();
		}
	}
}
