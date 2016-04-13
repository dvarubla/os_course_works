#include "string.h"
#include "math.h"

uint32_t strlen(const char *str) {
	const char *chr=str;
	while (*chr){
		chr++;
	}
	return (chr-str);
}

char* strrev(char *str) {
	char tmp;
	uint32_t len=strlen(str), half=len / 2;
	for (uint32_t i=0, j=len-1; i<half; i++, j=len-i-1){
		tmp=str[i];
		str[i]=str[j];
		str[j]=tmp;
	}
	return str;
}

char* int_to_str(int32_t value, char *buf, int radix) {
	char *str=buf;
	if(value<0){
		value*=-1;
		*buf='-';
		buf++;
	}
	uint_to_str(value, buf, radix);
	return str;
}

char* uint_to_str(uint32_t value, char *buf, int radix) {
	char *str=buf, lbase;
	if(radix>0){
		lbase='a';
	} else {
		lbase='A'; 
		radix=-radix;
	}
	if(radix<2||radix>36) {
		*buf=0;
		return buf;
	}
	lbase-=10;
	do{
		int32_t rem=value%radix;
		(*buf)=rem+(rem<10?'0':lbase);
		buf++;
	}
	while (value/=radix);

	*buf=0;
	return strrev(str);
}

char* double_to_str(double val, uint16_t frac_len, char *buf){
	char *str=buf;
	double int_part=round_to_zero(val);
	bool one_char_converted=false;
	bool less_than_zero=val<0;
	val=fabs(val);

	double frac_part=round_to_nearest((val-int_part)*pow(10, frac_len));
	if(frac_len!=0){
		for(uint16_t i=0; i<frac_len; i++){
			*buf=fmod(frac_part, 10)+'0';
			frac_part=round_to_zero(frac_part/10);
			buf++;
		}
		*buf='.';
		buf++;
	}
	int_part+=frac_part;
	do{
		*buf=fmod(int_part, 10)+'0';
		int_part=round_to_zero(int_part/10);
		buf++;
		one_char_converted=true;
	} while(int_part>0 || !one_char_converted);
	if(less_than_zero){
		*buf='-';
		buf++;
	}
	*buf=0;
	return strrev(str);	
}
