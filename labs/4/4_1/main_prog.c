#include "common_prog.h"
#include "interrupts.h"
#include "console_io.h"
#include "stages.h"
#include "atomic.h"
#include "spinlock.h"
#include "math.h"
#include "semaphore.h"

FullAddr prev_int8_addr, prev_int9_addr;
/*в 2^COUNTER_INCR раз быстрее, чем по умолчанию, будет работать таймер*/
#define COUNTER_INCR 6

#define THREAD_STACK_SIZE (4*1024)

/*флаги для члена структуры потока sleep*/
#define NO_SLEEP 0
#define SLEEPING 1
#define TIME_SLEEP 0b10

typedef struct Thread{
	/*указатель на функцию потока*/	
	uint32_t func;
	/*приоритет*/
	float priority;
	uint8_t stack[THREAD_STACK_SIZE];
	/*вершина стека - регистр esp*/
	uint16_t esp;
	/*завершился ли поток*/
	uint32_t finished;

	uint32_t sleep;
	/*счётчик тиков для определения, нужно ли закончить ожидание*/
	uint32_t sleep_counter;
} Thread;
/*индекс в массиве потоков*/
uint32_t cur_thread_id;
/*величины квантов времени*/
#define IDLE_TIME_SLICE 10
#define DEF_MAX_TIME_SLICE 250
uint16_t max_time_slice;
/*текущий квант*/
uint16_t cur_time_slice;
/*нужно ли передать управление другому потоку, независимо от кванта*/
uint32_t skip_time_slice;
/*был ли запущен поток, нужно ли генерировать новые числа, завершился ли производитель*/
uint32_t thread_switched, generate_more_numbers, producer_finished;

uint16_t get_cur_thread_id(){
	return atomic_read(&cur_thread_id);
}

/*был ли запущен хоть один поток, или выполнялась функция main*/
bool main_thread_running;

void sleep_msec(uint32_t msec);

void producer();
void consumer();
void idle();

Thread threads[]={
	{(uint32_t)idle, 0},
	{(uint32_t)producer, 0.5}, 
	{(uint32_t)consumer, 0.5}
};

#define NUM_THREADS (sizeof(threads)/sizeof(Thread))

Sema 
	/*мьютекс для вывода на экран*/
	print_sema, 
	buffer_op_sema, full_sema, empty_sema;

/*области экрана для вывода информации*/
TextBlock 
	producer_block={0, 1, SCREEN_COLS-1, 3, true, 0, 1}, 
	consumer_block={0, SCREEN_ROWS-1-2, SCREEN_COLS-1, SCREEN_ROWS-1, true, 0, SCREEN_ROWS-1-2},
	buffer_block={0, 4, SCREEN_COLS-1, SCREEN_ROWS-1-3, true, 0, 4},
	buffer_percent_block={SCREEN_COLS-1-6, 0, SCREEN_COLS-1, 0, true, SCREEN_COLS-1-6, 0},
	thread_status_block={SCREEN_COLS-1-8, 0, SCREEN_COLS-1-8, 0, true, SCREEN_COLS-1-8, 0};

#define MAX_BUF_SIZE 100

uint16_t buf_size;

#define DEF_DELAY_INTERVAL 30

uint16_t delay_interval;

uint32_t buffer[MAX_BUF_SIZE];
/*предыдущие позиции курсора*/
/*используется при удалении чисел из буфера*/
struct {
	uint8_t cursor_x; 
	uint8_t cursor_y;
} cursor_pos[MAX_BUF_SIZE+1]={
	{0, 4}
}; 
volatile uint32_t buffer_index;

/*вывести, какой поток активен*/
void print_status(char ch, uint8_t color){
	if(atomic_read(&thread_switched)){
		sema_p(&print_sema);
		printf_in_block(&thread_status_block, "%C%c", color, ch);
		sema_v(&print_sema);
		atomic_write(&thread_switched, false);
	}
}

/*вывести процент заполненности буфера*/
void print_buf_percent(){
	printf_in_block(&buffer_percent_block, "%C%6.2f%%", LIGHT_MAGENTA, (double)buffer_index/buf_size*100);
}

/*сгенерировать простое число большее предыдущего*/
void produce(uint32_t *prev){
	bool is_prime;
	for(uint32_t num=(*prev)+1; ;num++){
		is_prime=true;
		for(uint32_t i=2; i<(uint32_t)(sqrt(num)+1); i++){
			if((num%i)==0){
				is_prime=false;
				break;
			}
		}
		if(is_prime){
			(*prev)=num;
			break;
		}
	}
}
/*записать число в буфер*/
void append(uint32_t prime){
	sema_p(&empty_sema);
	sema_p(&buffer_op_sema);

	buffer[buffer_index]=prime;
	buffer_index++;
	sema_p(&print_sema);
	printf_in_block(&buffer_block, "%C%u ", LIGHT_GREEN, prime);
	cursor_pos[buffer_index].cursor_x=buffer_block.cursor_x;
	cursor_pos[buffer_index].cursor_y=buffer_block.cursor_y;
	print_buf_percent();
	if(buffer_index==buf_size){
		printf_in_block(&producer_block, "%CLast number! ", BRIGHT_WHITE);
	}
	sema_v(&print_sema);

	sema_v(&buffer_op_sema);
	sema_v(&full_sema);
}
/*удалить число из буфера*/
uint32_t remove(){
	uint32_t prime;
	sema_p(&full_sema);
	/*производитель завершился - значение семафора full_sema никто не увеличит*/
	if(atomic_read(&producer_finished)==true && buffer_index==0){	
		return 0;
	}
	sema_p(&buffer_op_sema);
	buffer_index--;
	prime=buffer[buffer_index];
	sema_p(&print_sema);
	/*нужно удалить всё после последнего сохранённого положения курсора*/
	buffer_block.cursor_x=cursor_pos[buffer_index].cursor_x;
	buffer_block.cursor_y=cursor_pos[buffer_index].cursor_y;
	/*число символов, которые нужно стереть*/
	uint16_t num_syms=
		(cursor_pos[buffer_index+1].cursor_y-buffer_block.cursor_y)*
		(buffer_block.x_max-buffer_block.x_min)+
		cursor_pos[buffer_index+1].cursor_x-buffer_block.cursor_x
	;
	/*удаление символов*/
	for(uint16_t i=0; i<num_syms; i++){		
		print_char_color_in_block(' ', WHITE, &buffer_block);
	}
	buffer_block.cursor_x=cursor_pos[buffer_index].cursor_x;
	buffer_block.cursor_y=cursor_pos[buffer_index].cursor_y;
	print_buf_percent();
	if(buffer_index==0){
		printf_in_block(&consumer_block, "%CLast number! ", YELLOW);
	}
	sema_v(&print_sema);

	sema_v(&buffer_op_sema);
	sema_v(&empty_sema);
	return prime;
}

void producer(){
	uint32_t prime=3;
	while(atomic_read(&generate_more_numbers)){
		print_status('P', YELLOW);
		produce(&prime);
		sema_p(&print_sema);
		printf_in_block(&producer_block, "%C%u ", YELLOW, prime);
		sema_v(&print_sema);
		append(prime);
		sleep_msec(delay_interval);
		//sleep_msec(rand_uint32()%delay_interval);
	}
	atomic_write(&producer_finished, true);
	/*поток-потребитель может ждать на этом семафоре*/
	sema_v(&full_sema);
}

void consumer(){
	uint32_t prime;
	while(1){
		print_status('C', BRIGHT_WHITE);
		prime=remove();
		/*кончились ли числа в буфере*/
		if(prime==0){
			printf_in_block(&thread_status_block, "%C%c", LIGHT_MAGENTA, 'X');
			break;
		}
		sema_p(&print_sema);
		printf_in_block(&consumer_block, "%C%u ", BRIGHT_WHITE, prime);
		sema_v(&print_sema);
		sleep_msec(delay_interval);
		//sleep_msec(rand_uint32()%delay_interval);
	}
}
/*ничего не делать*/
void idle(){
	while(1);
}
/*обёртка над каждым потоком*/
void main_thread_func(){
	((void (*)()) (threads[cur_thread_id].func)) ();
	atomic_write(&threads[cur_thread_id].finished, true);
	atomic_write(&skip_time_slice, true);
	while(1);
}

void sleep_msec(uint32_t msec){
	//atomic_write(&skip_time_slice, true);
	disable_int();
	threads[cur_thread_id].sleep=SLEEPING|TIME_SLEEP;
	threads[cur_thread_id].sleep_counter=msec*(1<<COUNTER_INCR)*TICKS_IN_S/MS_IN_S;
	enable_int();
	while(atomic_read(&threads[cur_thread_id].sleep) & SLEEPING);
}

void set_sleep(uint16_t id){
	atomic_write(&threads[id].sleep, SLEEPING);
}

void sleep_wait(uint16_t id){
	atomic_write(&skip_time_slice, true);
	while(atomic_read(&threads[id].sleep) & SLEEPING);
}

void wake(uint16_t id){
	atomic_write(&threads[id].sleep, NO_SLEEP);
}

uint16_t thread_tick_counter;
/*счётчик для вызова обработчика прерывания bios*/
uint16_t call_bios_handler_counter;

uint16_t cur_esp;

void switch_to_thread(uint16_t index){
	cur_thread_id=index;
	cur_esp=threads[index].esp;
	thread_switched=true;
}

void int8_handler_func(){
	thread_tick_counter++;
	call_bios_handler_counter++;
	uint16_t i;
	/*проверки, нужно ли закончить ожидание потокам*/
	for(i=1; i<NUM_THREADS; i++){
		if((threads[i].sleep & (SLEEPING|TIME_SLEEP))==(SLEEPING|TIME_SLEEP)){
			if(threads[i].sleep_counter==0){
				threads[i].sleep=NO_SLEEP;
			} else {
				threads[i].sleep_counter--;
			}
		}
	}
	/*квант времени кончился - нужно переключить потоки*/
	if(skip_time_slice || thread_tick_counter==cur_time_slice){
		skip_time_slice=false;
		if(main_thread_running){
			main_thread_running=false;
		} else {
			threads[cur_thread_id].esp=cur_esp;
		}
		thread_tick_counter=0;
		bool have_running_threads=false;
		bool have_not_sleeping_threads=false;

		float rand_priority=(float)rand_double();
		
		/*нужно скорректировать rand_priority 
		с учётом того, что некоторые потоки ждут*/
		float priority_sum=0;		
		for(i=1; i<NUM_THREADS; i++){
			if(!threads[i].finished){
				if(!(threads[i].sleep & SLEEPING)){
					priority_sum+=threads[i].priority;
					have_not_sleeping_threads=true;
				}
				have_running_threads=true;
			}
		}
		if(!have_running_threads){
			/*все потоки завершились*/
			stop_cpu();
		}
		if(!have_not_sleeping_threads){
			/*все потоки ждут*/
			cur_time_slice=IDLE_TIME_SLICE;
			switch_to_thread(0);
		} else {
			cur_time_slice=rand_uint32()%max_time_slice+1;
			rand_priority*=priority_sum;
			priority_sum=0;
			/*выбор потока, который будет возобновлён*/
			for(i=1; i<NUM_THREADS-1; i++){
				if(!threads[i].finished && !(threads[i].sleep & SLEEPING)){
					priority_sum+=threads[i].priority;
					if(priority_sum>=rand_priority){
						switch_to_thread(i);
						break;
					}
				}
			}
			if(i==NUM_THREADS-1){
				switch_to_thread(i);
			}
		}
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

void init_threads()
{
	uint16_t i;
	for(i=0; i<NUM_THREADS; i++){
		threads[i].finished=false;
		threads[i].sleep=NO_SLEEP;
		/*нужно инициализировать начальный кадр стека*/
		asm volatile(
			/*сохранить значение вершины стека*/
			"movw %%sp, %%dx\n\t"
			/*поместить в %sp вершину стека потока*/
			"movw %w1, %%sp\n\t"
			
			"pushw $0x0202\n\t" /*из флагов только IF*/
			/*адрес возврата - сегмент и смещение*/			
			"pushw %%cs\n\t"
			"pushw %w2\n\t"
			"pushl $0\n\t" /*начальное значение %ebp*/
			"movl %%esp, %%edi\n\t" /*значение %ebp после пролога*/
			"andw $(~0xF), %%sp\n\t"
			"subw $512, %%sp\n\t"
			"fnsave (%%esp)\n\t"
			"pushw %%ds\n\t"
			/*в цикле записать часть регистров*/
			"movw $5, %%cx\n\t"
			"1:"
			"pushl $0\n\t"
			"loopw 1b\n\t"
			"pushl %%edi\n\t" /*%ebp*/
			/*остальные регистры*/
			"pushl $0\n\t"
			"pushl $0\n\t"

			"movw %%sp, %0\n\t" /*сохранить вершину стека в память*/
			"movw %%dx, %%sp\n\t" /*вернуться в стек функции init_threads*/
			:"=m"(threads[i].esp) 
			:"a"(threads[i].stack+THREAD_STACK_SIZE), "b"(main_thread_func): "dx","cx","edi"
		);
	}
	sema_init(&print_sema, 1);
	sema_init(&empty_sema, buf_size);
	sema_init(&full_sema, 0);
	sema_init(&buffer_op_sema, 1);
	generate_more_numbers=true;
	producer_finished=false;
	skip_time_slice=false;	
}

#define ESC 0x01
#define RELEASED_FLAG 0x80

void int9_handler_func(){
	uint8_t key=inb(0x60);
	if(key==(ESC | RELEASED_FLAG)){
		/*завершить поток-производитель*/
		generate_more_numbers=false;
	}
	call_int_addr(&prev_int9_addr);
}

INTERRUPT_ROUTINE(9, int9_handler_func);

void read_params(){
	bool status;
	uint32_t num32;
	uint16_t num16;
	print("\r\nEnter seed: ");
	status=read_number(&num32, UINT32_ACT);
	if(!status){
		num32=get_time_ticks();
		printf("Incorrect value. Seed set to %u\r\n", num32);
	}	
	srand(num32);

	print("Enter buffer size: ");
	status=read_number(&num16, UINT16_ACT);
	if(status && num16<=MAX_BUF_SIZE){
		buf_size=num16;
	} else {
		buf_size=MAX_BUF_SIZE;
		print("Incorrect value. Buffer size set to "PASTE(MAX_BUF_SIZE)".\r\n");
	}
	
	print("Enter delay: ");
	status=read_number(&num16, UINT16_ACT);
	if(status){
		delay_interval=num16;
	} else {
		delay_interval=DEF_DELAY_INTERVAL;
		print("Incorrect value. Delay set to "PASTE(DEF_DELAY_INTERVAL)".\r\n");
	}

	print("Enter max time slice: ");
	status=read_number(&num16, UINT16_ACT);
	if(status && num16>0){
		max_time_slice=num16;
	} else {
		max_time_slice=DEF_MAX_TIME_SLICE;
		print("Incorrect value. Max time slice set to "PASTE(DEF_MAX_TIME_SLICE)".\r\n");
	}
	print("Print enter: ");
	get_key();
}

void main(){
	set_mode();
	main_thread_running=true;
	
	call_bios_handler_counter=thread_tick_counter=0;
	buffer_index=0;
	cur_time_slice=1;
	print_color("Lab 4\r\n", YELLOW);
	read_params();
	
	init_threads();	
	clear_screen();
	set_cursor_pos(0,0);
	print("Esc = Exit");
	hide_cursor();
	get_int_addr(8, &prev_int8_addr);
	get_int_addr(9, &prev_int9_addr);
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
