#ifndef _STRING_H_
#define _STRING_H_

#include "common_defs.h"

uint32_t strlen(const char *str);
char* strrev(char *str);
char* int_to_str(int32_t value, char *buf, int radix);
/*если radix меньше 0, то используются большие буквы*/
char* uint_to_str(uint32_t value, char *buf, int radix);
char* double_to_str(double val, uint16_t frac_len, char *buf);
#endif
