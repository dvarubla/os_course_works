#include "math.h"

double sqrt(double num)
{
	asm volatile("fsqrt":"+t"(num));
	return num;
}

double round_with_mask(double val, uint16_t clear_mask, uint16_t set_mask){
	uint16_t cw,cw_round;
	asm volatile("fnstcw %0" :"=m"(cw));
	cw_round=(cw&clear_mask)|set_mask;
	asm volatile("fldcw %0" : :"m"(cw_round));
	asm volatile("frndint":"+t"(val));
	asm volatile("fldcw %0" :"=m"(cw));
	return val;
}

double round_to_nearest(double val){
	return round_with_mask(val, ~(0b11<<10), 0);
}

double round_to_zero(double val){      
	return round_with_mask(val, ~(0b00<<10), 0b11<<10);
}

/*x^y=2^(y*log2(x))*/
double pow(double base, double exp){
	double tmp, tmp2;
	asm volatile("fyl2x" :"=t"(tmp) :"0"(base), "u"(exp):"st(1)");
	tmp2=round_to_zero(tmp); /*целая часть*/
	tmp-=tmp2; /*дробная часть*/
	asm volatile("f2xm1": "+t"(tmp));
	tmp++; /*дробная часть ^ exp*/
	asm volatile("fscale": "+t"(tmp): "u"(tmp2));
	return tmp;
}

double fmod(double numer, double denom){
	double result;
	asm volatile(
		"1:"
		"fprem\n\t"
		"fstsw %%ax\n\t"
		"sahf\n\t"
		"jp 1b\n\t"
		: "=t"(result): "0"(numer), "u"(denom): "ax"
	);
	return result;
}

double fabs(double num){
	asm volatile("fabs":"+t"(num));
	return num;
}

