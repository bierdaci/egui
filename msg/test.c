#include <stdio.h>

int main(void)
{
	const char *p = "-0.500000 -0.500001 0.530002";
	int n;
	double f1, f2, f3;

	n = sscanf(p, "%lf  %lf  %lf", &f1, &f2, &f3);
	printf("%lf  %lf  %lf\n", f1, f2, f3);
	return 0;
}
