
#include <stdio.h>
#include <stdarg.h>


void test1(float f1, double f2, float f3)
{
}
void test(float f1, ...)
{
	va_list vp;
	va_start(vp, f1);
	double f2 = va_arg(vp, double);	
	float  f3 = va_arg(vp, float);	
	test1(f1, f2, f3);
	va_end(vp);
}

int main(void)
{
	test(1.0, 2.0, 3.0);
}
