#include "pc_speaker.h"
#include "common_prog.h"
#include "console_io.h"
void play_sound(float frequence){
	uint32_t div;
	uint8_t tmp;

	div=(uint32_t)(MAX_FREQ / frequence);
	outb(0x43, 0b10110110);
	outb(0x42, (uint8_t)(div) );
	outb(0x42, (uint8_t)(div >> 8));

	tmp=inb(0x61);
	if (tmp!=(tmp | 3)) {
		outb(0x61, tmp | 3);
	}
}
 
void stop_sound(){
	uint8_t tmp=inb(0x61) & 0xFC;
	outb(0x61, tmp);
}
