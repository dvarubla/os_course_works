#include "spinlock.h"
#include "math.h"
#include "semaphore.h"
#include "atomic.h"
#include "console_io.h"
#include "common_prog.h"
#include "main_prog.h"
#include "gen_primes.h"

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
static uint32_t cur_thread_id;
/*величины квантов времени*/
#define IDLE_TIME_SLICE 10
/*множитель для вычисления числа тиков по величине кванта*/
#define MAX_TIME_SLICE_MULT 10
/*текущий квант*/
static uint16_t cur_time_slice;
/*нужно ли передать управление другому потоку, независимо от кванта*/
static uint32_t skip_time_slice;
/*нужно ли генерировать новые числа, завершился ли производитель*/
static uint32_t generate_more_numbers, producer_finished;

uint16_t get_cur_thread_id(){
	return atomic_read(&cur_thread_id);
}

/*был ли запущен хоть один поток, или выполнялась функция main*/
static bool main_thread_running;

static void sleep_msec(uint32_t msec);

static void producer();
static void consumer();
static void idle();

static Thread threads[]={
	{(uint32_t)idle, 0},
	{(uint32_t)producer, 0.5}, 
	{(uint32_t)consumer, 0.5}
};

#define NUM_THREADS (sizeof(threads)/sizeof(Thread))

static Sema 
	/*мьютекс для вывода на экран*/
	print_sema, 
	buffer_op_sema, full_sema, empty_sema;

/*разрешить вывод на экран только этому потоку*/
static void output_lock(){
	sema_p(&print_sema);
	set_can_write_to_screen(false);
}

static void output_unlock(){	
	set_can_write_to_screen(true);
	sema_v(&print_sema);
}

/*области экрана для вывода информации*/
static TextBlock 
	producer_block={SCREEN_COLS*2/5, 2, SCREEN_COLS-1, 4, true, SCREEN_COLS*2/5, 2}, 
	consumer_block={SCREEN_COLS*2/5, SCREEN_ROWS-1-5-2, SCREEN_COLS-1, SCREEN_ROWS-5-1, true, SCREEN_COLS*2/5, SCREEN_ROWS-1-5-2},
	buffer_block={SCREEN_COLS*2/5, 5, SCREEN_COLS-1, SCREEN_ROWS-1-4-3, true, SCREEN_COLS*2/5, 5},
	buffer_percent_block={SCREEN_COLS-1-6, 0, SCREEN_COLS-1, 0, true, SCREEN_COLS-1-6, 0};

#define MAX_BUF_SIZE 40

static uint16_t buf_size;

#define DEF_DELAY_INTERVAL 100

static uint16_t delay_interval;

static uint32_t buffer[MAX_BUF_SIZE];
/*предыдущие позиции курсора*/
/*используется при удалении чисел из буфера*/
static struct {
	uint8_t cursor_x; 
	uint8_t cursor_y;
} cursor_pos[MAX_BUF_SIZE+1]={
	{SCREEN_COLS*2/5, 5}
}; 
static volatile uint32_t buffer_index;

/*вывести процент заполненности буфера*/
static void print_buf_percent(){
	printf_in_block(&buffer_percent_block, "%C%6.2f%%", LIGHT_MAGENTA, (double)buffer_index/buf_size*100);
}

/*сгенерировать простое число большее предыдущего*/
static void produce(uint32_t *prev){
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
static void append(uint32_t prime){
	sema_p(&empty_sema);
	sema_p(&buffer_op_sema);

	buffer[buffer_index]=prime;
	buffer_index++;
	output_lock();
	printf_in_block(&buffer_block, "%C%u ", LIGHT_GREEN, prime);
	cursor_pos[buffer_index].cursor_x=buffer_block.cursor_x;
	cursor_pos[buffer_index].cursor_y=buffer_block.cursor_y;
	print_buf_percent();
	if(buffer_index==buf_size){
		printf_in_block(&producer_block, "%CLast number! ", BRIGHT_WHITE);
	}
	output_unlock();

	sema_v(&buffer_op_sema);
	sema_v(&full_sema);
}
/*удалить число из буфера*/
static uint32_t remove(){
	uint32_t prime;
	sema_p(&full_sema);
	
	/*производитель завершился - значение семафора full_sema никто не увеличит*/
	if(atomic_read(&producer_finished)==true && buffer_index==0){	
		return 0;
	}
	sema_p(&buffer_op_sema);
	buffer_index--;
	prime=buffer[buffer_index];
	output_lock();
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
	output_unlock();

	sema_v(&buffer_op_sema);
	sema_v(&empty_sema);
	return prime;
}

static void producer(){
	uint32_t prime=3;
	while(atomic_read(&generate_more_numbers)){
		produce(&prime);
		output_lock();
		printf_in_block(&producer_block, "%C%u ", YELLOW, prime);
		output_unlock();
		append(prime);
		sleep_msec(delay_interval);
		//sleep_msec(rand_uint32()%delay_interval);
	}
	atomic_write(&producer_finished, true);
	sema_v(&full_sema);
}

static void consumer(){
	uint32_t prime;
	while(1){
		prime=remove();
		/*кончились ли числа в буфере*/
		if(prime==0){
			break;
		}
		output_lock();
		printf_in_block(&consumer_block, "%C%u ", BRIGHT_WHITE, prime);
		output_unlock();
		sleep_msec(delay_interval);
		//sleep_msec(rand_uint32()%delay_interval);
	}
}
/*ничего не делать*/
static void idle(){
	while(1);
}
/*обёртка над каждым потоком*/
static void main_thread_func(){
	((void (*)()) (threads[cur_thread_id].func)) ();
	atomic_write(&threads[cur_thread_id].finished, true);
	atomic_write(&skip_time_slice, true);
	while(1);
}

static void sleep_msec(uint32_t msec){
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

static uint16_t thread_tick_counter;

void gen_primes_init()
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
	buf_size=MAX_BUF_SIZE;
	delay_interval=DEF_DELAY_INTERVAL;	
	
	sema_init(&print_sema, 1);
	sema_init(&empty_sema, buf_size);
	sema_init(&full_sema, 0);
	sema_init(&buffer_op_sema, 1);

	generate_more_numbers=true;
	producer_finished=false;
	thread_tick_counter=0;
	buffer_index=0;
	cur_time_slice=1;
	main_thread_running=true;

	srand(get_time_ticks());	
}

static uint16_t switch_to_thread(uint16_t index, uint16_t cur_esp){
	skip_time_slice=false;
	if(main_thread_running){
		main_thread_running=false;
	} else {
		threads[cur_thread_id].esp=cur_esp;
	}
	thread_tick_counter=0;
	cur_thread_id=index;
	return threads[index].esp;
}

uint16_t gen_primes_task(uint16_t cur_esp, bool *threads_finished){
	generate_more_numbers=!is_program_finished();
	/*поток был приостановлен*/
	bool thread_paused=(cur_thread_id!=0 && get_thread_status(GEN_PRIMES_PRODUCER_THREAD+cur_thread_id-1)==PAUSED);
	if(!thread_paused){
		thread_tick_counter++;
	}
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
	if(
		/*квант времени кончился - нужно переключить потоки*/		
		(skip_time_slice || thread_tick_counter==cur_time_slice || thread_paused) &&
		/*нельзя переключить потоки, если поток приостановлен и нельзя выводить на экран*/
		(!thread_paused || get_can_write_to_screen())
	){		
		uint8_t num_sleeping_threads=0, active_thread_id=0;
		/*узнать число ждущих потоков, установить их статус*/
		for(uint8_t i=1; i<NUM_THREADS; i++){
			if(
				threads[i].finished || 				
				(threads[i].sleep & SLEEPING) ||  
				(get_thread_status(GEN_PRIMES_PRODUCER_THREAD+i-1)==PAUSED)
			){
				num_sleeping_threads++;
				set_thread_status(
					GEN_PRIMES_PRODUCER_THREAD+i-1, 
					(threads[i].sleep & SLEEPING)?WAITING:PAUSED
				);
				active_thread_id=(i==1)?2:1;
			}
		}
		if(threads[1].finished && threads[2].finished){
			/*все потоки завершились*/
			(*threads_finished)=true;
			return 0;
		}
		if(num_sleeping_threads==NUM_THREADS-1){
			/*все потоки ждут*/
			cur_time_slice=IDLE_TIME_SLICE;
			return switch_to_thread(0, cur_esp);
		}
		/*нужно выбрать один поток из двух*/
		if(num_sleeping_threads==0){
			float rand_priority=(float)rand_double();
			if(rand_priority<threads[1].priority){
				active_thread_id=1;
				set_thread_status(GEN_PRIMES_CONSUMER_THREAD, WAITING);
			} else {
				active_thread_id=2;
				set_thread_status(GEN_PRIMES_PRODUCER_THREAD, WAITING);
			}
		}
		set_thread_status(GEN_PRIMES_PRODUCER_THREAD+active_thread_id-1, ACTIVE);
		/*квант времени для активного потока*/
		cur_time_slice=
			rand_uint32()%
			(get_time_slice(GEN_PRIMES_PRODUCER_THREAD+active_thread_id-1)*MAX_TIME_SLICE_MULT)+1
		;
		return switch_to_thread(active_thread_id, cur_esp);
	}
	return cur_esp;
}
