#ifndef _MATH_H_
#define _MATH_H_

#include "common_defs.h"

double sqrt(double num);

double round_with_mask(double val, uint16_t clear_mask, uint16_t set_mask);

/*округлить к ближайшему целому*/
double round_to_nearest(double val);

/*округлить к 0*/
double round_to_zero(double val);

double pow(double base, double exp);

double fmod(double numer, double denom);

double fabs(double num);

#endif
