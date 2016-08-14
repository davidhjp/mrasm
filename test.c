#include<stdlib.h>
#include <math.h>
#include "test.h"
int main(void){
	unsigned long f = 0x50000000;
	unsigned long n = 0x0100;
	printf("%08X\n",f);
	printf("%04X\n",(f >> 18));
	printf("%04X\n",n);
	printf("%08X\n",(n << 18));
}
