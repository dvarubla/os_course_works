#include "common_prog.h"
#include "pc_speaker.h"
#include "interrupts.h"
#include "console_io.h"
#include "stages.h"

#define TICK_FREQ 500

/*все значения времени в миллисекундах*/
#define DEF_TICK_DURATION 500

typedef struct Note{
	float freq;
	uint16_t duration;
} Note;

#include "melody.h"

bool tick_started, tick_occured, melody_started;

float tick_counter, melody_counter;

/*интервал между звуковыми тиками, длительность звукового тика*/
uint16_t tick_interval, tick_duration;

/*интервал, через который начнёт играть мелодия*/
float ring_interval;
/*индекс ноты в массиве нот*/
uint8_t note_index;

float ticks_in_ms(float ms){
	return ms*TICKS_IN_S/MS_IN_S;
}

void play_note(){
	if(notes[note_index].freq==WAIT_FREQ){
		stop_sound();
	} else {
		play_sound(notes[note_index].freq);
	}
}

void handler_func(){
	melody_counter++;
	if(!melody_started){
		if(melody_counter>=ticks_in_ms(ring_interval)){
			melody_started=true;
			melody_counter-=ticks_in_ms(ring_interval);
			/*если звучит тик, нужно его выключить*/
			if(tick_started){
				tick_started=false;
			}
			note_index=0;
			play_note();		
		}
	} else {
		/*нота закончила звучать*/
		if(melody_counter>=ticks_in_ms(notes[note_index].duration)){
			melody_counter-=ticks_in_ms(notes[note_index].duration);			
			note_index++;
			if(note_index==num_notes){
				stop_sound();
				melody_started=false;
			} else {
				play_note();
			}
		}	
	}
	/*даже если мелодия начала играть, тики все равно отсчитываются, но не звучат*/
	tick_counter++;
	if(tick_started){
		if(tick_counter>=ticks_in_ms(tick_duration)){
			tick_started=false;
			tick_counter-=ticks_in_ms(tick_duration);
			if(!melody_started){
				stop_sound();
			}
		}
	} else {
		uint16_t actual_interval=tick_interval;
		if(tick_occured){
			actual_interval-=tick_duration;
		}
		if(tick_counter>=ticks_in_ms(actual_interval)){
			tick_counter-=ticks_in_ms(actual_interval);
			if(!melody_started){
				tick_started=tick_occured=true;
				play_sound(TICK_FREQ);
			}
		}
	}
}



void read_params(){
	bool status;
	uint16_t num;
	uint16_t tick_duration_in_sec=(tick_duration/MS_IN_S)+(tick_duration%MS_IN_S==0)?0:1;
	printf("\r\nEnter tick interval (greater than %u): ", tick_duration_in_sec);
	status=read_number(&num, UINT16_ACT);
	if(status && num>=tick_duration_in_sec){
		tick_interval=num*MS_IN_S;
	} else {
		tick_interval=tick_duration_in_sec*MS_IN_S;
		printf("Incorrect value. Interval set to %u.\r\n", tick_duration_in_sec);
	}

	float tick_interval_in_minutes=(float)tick_interval/MS_IN_S/SEC_IN_MIN;
	float num_f;
	printf("Enter ring interval (greater than %f): ", tick_interval_in_minutes);
	status=read_number(&num_f, FLOAT_ACT);
	if(status && num_f>tick_interval_in_minutes){
		ring_interval=num_f*MS_IN_S*SEC_IN_MIN;
	} else {
		ring_interval=tick_interval*10;
		printf("Incorrect value. Ring interval set to %f.\r\n", tick_interval_in_minutes*10);
	}
}

INTERRUPT_ROUTINE(1C, handler_func);

void main(){
	set_mode();

	tick_duration=DEF_TICK_DURATION;

	tick_counter=melody_counter=0;
	tick_started=tick_occured=melody_started=false;

	print_color("Lab 2", YELLOW);
	read_params();
	//disable_int();
	//outb(0x43, 0x36);
	//outb(0x40, 0xFF);
	//outb(0x40, 0xFF);
	set_int_addr_func(0x1C, (uint32_t)int1C_handler);
	enable_int();
	while(1){
		read_char();
	}
}
