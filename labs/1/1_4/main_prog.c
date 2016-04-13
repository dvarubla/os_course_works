#include "common_prog.h"
#include "pc_speaker.h"
#include "interrupts.h"
#include "console_io.h"
#include "stages.h"

#define LSHIFT 0x2A
#define LCTRL 0x1D
#define ESC 0x01
#define RELEASED_FLAG 0x80

#define DEF_SOUND_FREQ 1500
#define DEF_SOUND_DELAY 50000

/*адрес предыдущего обработчика прерывания*/
FullAddr addr;
bool show_menu,sound,shift_pressed,esc_pressed,ctrl_pressed;
uint32_t sound_freq, sound_delay;

#define SPECIAL_DELAY_MULT 5
/*Клавиши, для которых устанавливается большая задержка*/
uint8_t special_scancodes[]={
	/*Backspace, правый Shift, Enter*/
	0x0E, 0x36, 0x1C
};
uint8_t special_scancodes_size=sizeof(special_scancodes)/sizeof(uint8_t);

void handler_func(){
	uint8_t key=inb(0x60);
	outb(0x64, 0xD2);
	while(inb(0x64)&0b10);
	outb(0x60, key);
	
	uint8_t cleared_key=key;
	bool pressed=((key&RELEASED_FLAG)==0);
	if(!pressed){
		cleared_key^=RELEASED_FLAG;
	}
	switch(cleared_key){
		case LSHIFT: shift_pressed=pressed; break;
		case LCTRL: ctrl_pressed=pressed; break;
		case ESC: esc_pressed=pressed; break;
	}
	if(pressed){
		if(esc_pressed && shift_pressed){			
			sound=!sound;
		}
		if(ctrl_pressed && shift_pressed){
			ctrl_pressed=shift_pressed=false;
			show_menu=true;
		}
	}
	if(sound && pressed){
		play_sound(sound_freq);
		bool is_special=false;		
		for(uint8_t i=0; i<special_scancodes_size; i++){
			if(cleared_key==special_scancodes[i]){
				is_special=true;
			}
		}
		wait((is_special)?sound_delay*SPECIAL_DELAY_MULT:sound_delay);
		stop_sound();
	}
	call_int_addr(&addr);
}

INTERRUPT_ROUTINE(9, handler_func);


void read_params(){
	bool status;
	uint32_t num;
	print("\r\nEnter frequency: ");
	status=read_number(&num, UINT32_ACT);
	if(status && num<MAX_FREQ){
		sound_freq=num;
	} else {
		sound_freq=DEF_SOUND_FREQ;
		print("Incorrect value. Frequency set to "PASTE(DEF_SOUND_FREQ)".\r\n");
	}
	
	print("Enter delay: ");
	status=read_number(&num, UINT32_ACT);
	if(status){
		sound_delay=num;
	} else {
		sound_delay=DEF_SOUND_DELAY;
		print("Incorrect value. Delay set to "PASTE(DEF_SOUND_DELAY)".\r\n");
	}
}

void main(){
	set_mode();
	sound=shift_pressed=esc_pressed=ctrl_pressed=false;

	show_menu=true;
	/*для теста, работает ли вообще pc speaker*/
	play_sound(800);
	wait(100000);
	stop_sound();

	print_color("Lab 1", YELLOW);
	get_int_addr(9, &addr);
	/*по умолчанию прерывания могут быть запрещены*/
	enable_int();	
	set_int_addr_func(9, (uint32_t)int9_handler);
	while(1){
		/*если нажаты ctrl и shift, показывается меню*/
		if(show_menu){
			set_int_addr(9, &addr);
			read_params();
			set_int_addr_func(9, (uint32_t)int9_handler);
			show_menu=false;
		}
		while(has_keys()){
			read_char();
		}
	}
}
