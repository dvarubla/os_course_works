#ifndef _COMMON2_H_
#define _COMMON2_H_
#include "common_defs.h"

/*Получить число головок и секторов в цилиндре*/
void get_drive_params(uint8_t *max_heads, uint8_t *max_sectors);

#endif
