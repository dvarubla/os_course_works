#include "console_io.h"
#include "common.h"
#include "string.h"
#include "common_prog.h"
#include "math.h"
/*размер строки для printf дли перевода числа в строку*/
#define BUF_SIZE 64
/*размер дробной части по умолчанию*/
#define DEFAULT_PRINTF_FRAC_LEN 5

const MaxVal max_vals[]={
	{UINT8_MAX, 3},
	{UINT16_MAX, 5},
	{UINT32_MAX, 10},
	{INT32_MAX, 11},
	{UINT32_MAX, 20}
};

TextBlock full_screen_block={0, 0, SCREEN_COLS-1, SCREEN_ROWS-1, false};

void disable_blink(){
	asm volatile("int $0x10" : :"a"(0x1003), "b"(0));
}

void hide_cursor(){
	/*7>6, поэтому курсор не виден*/
	asm volatile("int $0x10" : :"a"(0x0100), "c"((7|1<<5)<<8|6));
}

void set_cursor_size(uint8_t start_line, uint8_t end_line){
	asm volatile("int $0x10" : :"a"(0x0100), "c"(start_line<<8|end_line));
}

void set_cursor_pos(uint8_t x, uint8_t y)
{
	asm volatile("int $0x10" : :"a"(0x0200), "b"(0), "d"(y<<8|x));
}
/*либо переместить курсор в положение, сохранённое в блоке, 
либо сохранить положение курсора в блок*/
void process_cursor_in_block(TextBlock *block){
	if(block->cursor_stored){
		set_cursor_pos(block->cursor_x, block->cursor_y);
	} else {
		get_cursor_pos(&block->cursor_x, &block->cursor_y);
	}
}
/*если длина строки меньше, чем pad_length, то на экран в дополнение к строке выводятся символы слева*/
void print_with_padding(uint8_t pad_length, char pad_sym, const char *str, uint8_t color, TextBlock *block){
	uint32_t str_len=strlen(str);
	if(pad_length>str_len){
		pad_length-=str_len;
		while(pad_length--){
			print_char_color_in_block(pad_sym, color, block);
		}
	}
	print_color_in_block(str, color, block);
}

void printf(const char *format, ...){
	va_list vargs;
	va_start(vargs, format);
	vsprintf(format, vargs, &full_screen_block);
	va_end(vargs);
}

void printf_in_block(TextBlock *block, const char *format, ...)
{
	va_list vargs;
	va_start(vargs, format);
	vsprintf(format, vargs, block);
	va_end(vargs);
}

void vsprintf(const char *format, va_list vargs, TextBlock *block)
{
	char ch,*str;
	char print_buffer[BUF_SIZE];
	uint8_t pad_length, frac_len;
	uint8_t color=WHITE;
	char pad_sym;

	process_cursor_in_block(block);
	while(1){
		ch=*format;
		if(!ch){
			break;
		}
		format++;
		if(ch=='%'){
			ch=*format;
			if(ch=='%'){
				print_char_color_in_block('%', color, block);
				format++;
				continue;
			}
			/*значения по умолчанию*/
			pad_length=0;
			frac_len=DEFAULT_PRINTF_FRAC_LEN;
			if(ch=='0'){
				pad_sym='0';
				format++;
			} else {
				pad_sym=' ';
			}
			format=read_uint_from_str(format, &pad_length, UINT8_ACT);
			while(1){
				ch=*format;
				format++;
				switch(ch){
					case '.':
						format=read_uint_from_str(format, &frac_len, UINT8_ACT);
					continue;					

					case 's':
						print_with_padding(pad_length, pad_sym, va_arg(vargs, char*), color, block);
					break;

					case 'u':
						str=uint_to_str(va_arg(vargs, uint32_t), print_buffer, 10);
						print_with_padding(pad_length, pad_sym, str, color, block);
					break;
				
					case 'd':
						str=int_to_str(va_arg(vargs, int32_t), print_buffer, 10);
						print_with_padding(pad_length, pad_sym, str, color, block);
						
					break;				
					
					case 'f':
						str=double_to_str(va_arg(vargs, double), frac_len, print_buffer);
						print_with_padding(pad_length, pad_sym, str, color, block);
					break;	

					case 'x':
					case 'X':
						str=uint_to_str(va_arg(vargs, uint32_t), print_buffer, (ch=='x')?16:-16);
						print_with_padding(pad_length, pad_sym, str, color, block);
					break;
					case 'c':
						print_char_color_in_block((char)va_arg(vargs, uint32_t), color, block);
					break;
					case 'C':
						color=va_arg(vargs, uint32_t);
					break;
					default: break;
				}
				break;
			}
		} else {
			print_char_color_in_block(ch, color, block);
		}
	}
}

void get_cursor_pos_and_size(uint8_t *x, uint8_t *y, uint8_t *start_line, uint8_t *end_line)
{
	uint16_t pos;
	uint16_t size;
	asm volatile("int $0x10" :"=d"(pos), "=c"(size), "=a"((int32_t){0}) :"a"(0x0300), "b"(0));
	(*x)=pos&0xFF;
	(*y)=(pos>>8)&0xFF;
	if(start_line!=0){
		(*end_line)=size&0xFF;
		(*start_line)=(size>>8)&0xFF;
	}
}

void get_cursor_pos(uint8_t *x, uint8_t *y)
{
	get_cursor_pos_and_size(x, y, 0, 0);
}

void scroll_up(uint8_t num_lines)
{
	scroll_up_block(num_lines, &full_screen_block);	
}

void clear_screen(){
	scroll_up(0);
}

void scroll_up_block(uint8_t num_lines, TextBlock *block){
	asm volatile(
		"int $0x10"
		: :"a"(0x0600|num_lines), "b"(0x0700), 
		"c"((block->y_min<<8)|block->x_min), "d"((block->y_max<<8)|block->x_max)
	);
}

void print_color(const char *str, uint8_t color)
{
	print_color_in_block(str, color, &full_screen_block);
}

void print_color_in_block(const char *str, uint8_t color, TextBlock *block)
{
	process_cursor_in_block(block);
	while(*str){
		print_char_color_in_block(*str, color, block);
		str++;
	}	
}

void print_char_color(char ch, uint8_t color)
{
	process_cursor_in_block(&full_screen_block);
	print_char_color_in_block(ch, color, &full_screen_block);
}

void print_char_color_in_block(char ch, uint8_t color, TextBlock *block)
{
	if(ch=='\r'){
		block->cursor_x=block->x_min;
	} else if(ch=='\n'){
	 	if(block->cursor_y!=block->y_max){
			block->cursor_y++;
		} else {
			scroll_up_block(1, block);
		}
	} else {
		/*курсор вышел за границы экрана, нужно выводить на следующей строке*/
		if(block->cursor_x==(block->x_max+1)){
			block->cursor_x=block->x_min;
			if(block->cursor_y!=block->y_max){
				block->cursor_y++;
			} else {
				scroll_up_block(1, block);
			}
		}
		set_cursor_pos(block->cursor_x, block->cursor_y);
		asm volatile("int $0x10" : : "a"(0x0900|ch), "b"((uint16_t)color), "c"(1));
		block->cursor_x++;
	}
	set_cursor_pos(block->cursor_x, block->cursor_y);
}

void __REGPARM print_char(char ch)
{
	asm volatile("int $0x10" : : "a"(0x0A00|ch), "b"(0), "c"(1));
}

char* read_uint_from_str(const char *str, void *ret_num, NumAction action){
	uint32_t num=0;
	uint8_t ch;
	/*обработан ли хотя бы один символ*/
	bool one_char_processed=false;
	while(*str){
		ch=*str;
		if(ch>='0' && ch<='9'){
			ch-='0';
			/*проверка на переполнение*/
			if(num<=(max_vals[action].max_num-ch)/10){
				num=num*10+ch;
				one_char_processed=true;
			} else {
				break;
			}
		} else {
			break;
		}
		str++;
	}
	if(one_char_processed){
		switch(action){
			case UINT8_ACT: (*((uint8_t*)ret_num))=num; break;
			case UINT16_ACT: (*((uint16_t*)ret_num))=num; break;
			case UINT32_ACT: (*((uint32_t*)ret_num))=num; break;
			default: break;
		}
	}
	return (char*)str;
}

char* read_float_from_str(const char *str, float *ret_num){
	uint32_t t_num=0;
	const char *str_pointer,*prev_str_pointer=str;
	str_pointer=read_uint_from_str(prev_str_pointer, &t_num, UINT32_ACT);
	(*ret_num)=t_num;
	if((*str_pointer)!='.'){
		return (char*)str_pointer;
	}
	prev_str_pointer=str_pointer+1;
	str_pointer=read_uint_from_str(prev_str_pointer, &t_num, UINT32_ACT);
	if(str_pointer!=prev_str_pointer){
		(*ret_num)+=pow(0.1, str_pointer-prev_str_pointer)*t_num;
	}
	return (char*)str_pointer;
}

bool read_number(void *ret_num, NumAction action)
{
	char str[max_vals[action].max_len+1];
	uint32_t typed_chars_len;
	if(
		/*не введено ни одного символа*/
		read_str(str, max_vals[action].max_len, &typed_chars_len)==0 
		|| 
		/*введено слишком много символов*/
		typed_chars_len>max_vals[action].max_len
	){		
		return false;
	}
	char *end_of_processing=
	(action==FLOAT_ACT)?
	read_float_from_str(str, (float*)ret_num):
	read_uint_from_str(str, ret_num, action);
	if(*(end_of_processing)!=0){
		return false;
	}
	return true;
}

bool read_numbers(void *ret_nums, uint16_t num_nums, uint32_t max_str_len, NumAction action){
	char str[max_str_len+1];
	uint32_t typed_chars_len;
	if(
		/*не введено ни одного символа*/
		read_str(str, max_str_len, &typed_chars_len)==0 
		|| 
		/*введено слишком много символов*/
		typed_chars_len>max_str_len
	){		
		return false;
	}
	char *prev_read=str;
	char *end_of_processing=str;
	for(uint16_t i=0; i<num_nums; i++){
		while((*prev_read)==' '){
			prev_read++;
		}
		switch(action){
			case UINT8_ACT: 
				end_of_processing=read_uint_from_str(prev_read, &((uint8_t*)ret_nums)[i], action); 
			break;
			case UINT16_ACT: 
				end_of_processing=read_uint_from_str(prev_read, &((uint16_t*)ret_nums)[i], action); 
			break;
			case UINT32_ACT: 
				end_of_processing=read_uint_from_str(prev_read, &((uint32_t*)ret_nums)[i], action); 
			break;
			case FLOAT_ACT:
				end_of_processing=read_float_from_str(prev_read, &((float*)ret_nums)[i]); 
			default: break;
		}
		if(end_of_processing==prev_read){
			return false;
		}
		prev_read=end_of_processing;
	}
	if(*(end_of_processing)!=0){
		return false;
	}
	return true;
}

bool has_keys(){
	uint8_t ret;
        asm volatile(
                "int $0x16\n\t"
		"setz %%al\n\t"
                : "=a"(ret) :"a"(0x0100): "cc"
	);
	return ret!=1;
}

uint16_t get_key() {
        uint16_t s;
        asm volatile(
                "int $0x16"
                : "=a"(s) :"a"(0) 
	);
        return s;
}

uint32_t read_str(char *str, uint32_t max_len, uint32_t* typed_chars_len){
	uint8_t x, y;
	get_cursor_pos(&x, &y);
	uint8_t first_line_chars=SCREEN_COLS-x;
	/*на первой линии могут быть другие символы, кроме тех которые введены с клавиатуры*/
	bool first_line=true;
	uint32_t write_len=0, tot_len=0;
	uint8_t ch, num_chars_on_line=0;
	while(1){
		ch=read_char();
		switch(ch){
			case '\r':
				str[write_len]=0;
				if(typed_chars_len){				
					(*typed_chars_len)=tot_len;
				}
				return write_len;
			break;
			case '\b':
				if(num_chars_on_line!=0){
					num_chars_on_line--;
					tot_len--;
					if(tot_len<max_len){
						write_len--;
					} 
					print_char_tt(ch);
					print_char(' ');
				}
			break;
			default:
				if(first_line){
					if(num_chars_on_line==(first_line_chars-1)){
						first_line=false;
						num_chars_on_line=0;
					} else {
						num_chars_on_line++;
					}
				} else {
					if(num_chars_on_line==(SCREEN_COLS-1)){
						num_chars_on_line=0;
					} else {
						num_chars_on_line++;
					}
				}
				if(tot_len<max_len){
					str[write_len]=ch;
					write_len++;
				}				
				tot_len++;
			break;
		}
	}
}

uint8_t read_char(){
	uint8_t ch=get_key()&0xFF;
	if(ch!='\b'){
		print_char_tt(ch);
	}
	if(ch=='\r'){
		print_char_tt('\n');
	}
	return ch;
}
