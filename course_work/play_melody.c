#include "common_prog.h"
#include "pc_speaker.h"
#include "main_prog.h"

typedef struct Note{
	float freq;
	uint16_t duration;
} Note;

#include "melody.h"

static float melody_counter;
static uint8_t note_index;
static bool melody_started;
static bool melody_paused;

static float ticks_in_ms(float ms){
	return ms*(1<<COUNTER_INCR)*TICKS_IN_S/MS_IN_S;
}

static float ticks_in_note_length(float length){
	return ticks_in_ms(length)*get_time_slice(PLAY_MELODY_THREAD)/DEFAULT_TIME_SLICE;
}

static void play_note(){
	if(notes[note_index].freq==WAIT_FREQ){
		stop_sound();
	} else {
		play_sound(notes[note_index].freq);
	}
}

void play_melody_init(){
	melody_paused=false;
	melody_started=false;
}

void play_melody_task(){
	if(is_program_finished()){
		stop_sound();
		return;
	}
	if(get_thread_status(PLAY_MELODY_THREAD)==PAUSED){
		melody_paused=true;
		stop_sound();
		return;
	}
	/*воспроизведение было прервано, нужно его возобновить*/	
	if(melody_paused){
		play_note();
	}	

	melody_counter++;
	if(!melody_started){
		melody_started=true;
		note_index=0;
		melody_counter=0;
		play_note();
	} else {
		/*нота закончила звучать*/
		if(melody_counter>=ticks_in_note_length(notes[note_index].duration)){
			melody_counter-=ticks_in_note_length(notes[note_index].duration);			
			note_index++;
			if(note_index==num_notes){
				note_index=0;
			}
			play_note();
		}	
	}
}
