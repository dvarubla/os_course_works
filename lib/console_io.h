#ifndef _CONSOLEIO_H_
#define _CONSOLEIO_H_

#include "common_defs.h"

/*цвета для текстового режима*/
typedef enum Color{
      BLACK=0, BLUE, GREEN, CYAN, RED, MAGENTA, BROWN, WHITE,
      DARK_GRAY, LIGHT_BLUE, LIGHT_GREEN, LIGHT_CYAN, LIGHT_RED,
      LIGHT_MAGENTA, YELLOW, BRIGHT_WHITE, NUM_COLORS
} Color;

/*с числом какого размера проводится действие*/
typedef enum NumAction{
	UINT8_ACT=0,
	UINT16_ACT,
	UINT32_ACT,
	INT32_ACT,
	FLOAT_ACT
} NumAction;

/*максимальные значения -- число и длина*/
typedef struct MaxVal{
	uint32_t max_num;
	uint8_t max_len;
} MaxVal;

/*Текстовый блок -- область экрана с определенным положением курсора*/
typedef struct TextBlock{
	uint8_t x_min;
	uint8_t y_min;
	uint8_t x_max;
	uint8_t y_max;
	/*Хранится ли положение курсора в блоке или оно берётся с экрана*/
	bool cursor_stored;
	uint8_t cursor_x;
	uint8_t cursor_y;
} TextBlock;

/*Число столбцов и строк всего экрана*/
#define SCREEN_COLS 80
#define SCREEN_ROWS 25

/*Блок всего экрана*/
extern TextBlock full_screen_block;
/*убрать мерцание символа*/
void disable_blink();
/*Спрятать курсор*/
void hide_cursor();
/*установить размер курсора*/
void set_cursor_size(uint8_t start_line, uint8_t end_line);
/*Получить положение курсора*/
void get_cursor_pos(uint8_t *x, uint8_t *y);
/*Получить положение курсора и размер*/
void get_cursor_pos_and_size(uint8_t *x, uint8_t *y, uint8_t *start_line, uint8_t *end_line);
/*Установить положение курсора на экране*/
void set_cursor_pos(uint8_t x, uint8_t y);
/*Прокрутить экран вверх на num_lines*/
void scroll_up(uint8_t num_lines);
/*Прокрутить блок вверх*/
void scroll_up_block(uint8_t num_lines, TextBlock *block);
/*Очистить экран*/
void clear_screen();


void printf(const char *format, ...);
void printf_in_block(TextBlock *block, const char *format, ...);
/*как printf, только аргументы передаются в va_list*/
void vsprintf(const char *format, va_list vargs, TextBlock *block);

/*нажата ли какая-нибудь клавиша*/
bool has_keys();
/*получить скан-код и ASCII-код клавиши*/
uint16_t get_key();
/*получить ASCII-код и напечатать символ на экране*/
uint8_t read_char();

/*считать строку длины max_len. typed_chars_len -- сколько всего введено символов*/
uint32_t read_str(char *str, uint32_t max_len, uint32_t* typed_chars_len);
/*напечатать строку цветом color*/
void print_color(const char *str, uint8_t color);
void print_color_in_block(const char *str, uint8_t color, TextBlock *block);
/*напечатать символ в позиции курсора.*/
void __REGPARM print_char(char ch);
void print_char_color(char ch, uint8_t color);
void print_char_color_in_block(char ch, uint8_t color, TextBlock *block);
/*прочитать целое число из строки и вернуть место, где чтение закончилось*/
char* read_uint_from_str(const char *str, void *ret_num, NumAction action);
/*прочитать вещественное число из строки и вернуть место, где чтение закончилось*/
char* read_float_from_str(const char *str, float *ret_num);
/*ввести целое число с клавиатуры*/
bool read_number(void *ret_num, NumAction action);
/*ввести num_nums целых чисел с клавиатуры*/
bool read_numbers(void *ret_nums, uint16_t num_nums, uint32_t max_str_len, NumAction action);
#endif
