#include "stages.h"
#include "common_defs.h"
/*код, который выполняется в начале каждой программы*/
asm (                                        
	"xorw %ax, %ax\n\t"                  
	"movw %ax, %ds\n\t"                  
	"movw ("PASTE(DATA_LOC)"), %ax\n\t"  
	"movw %ax, %ds\n\t"                  
	"movw %ax, %ss\n\t"                  
	"movw $0xFFFB, %sp\n\t"              
	"fninit\n\t"                         
	"jmp main"                           
);
