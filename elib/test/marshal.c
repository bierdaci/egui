#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#ifndef _INTSIZEOF
#define _INTSIZEOF(n)       ((sizeof(n) + sizeof(int)-1) & ~(sizeof(int) - 1))
#endif


typedef struct {
	char buf[15];
	int i;
} Buffer;

int test_print(void *obj)
{
	return 0;
}

#if 1
int __signal_call_marshal(char *obj, int (*func)(void *), va_list vp, int size)
{
	return func(obj);
}
#endif

static int test_call(void *p, ...)
{
	va_list ap;

	va_start(ap, p);
	__signal_call_marshal(p, test_print, ap, 16 + sizeof(Buffer));
	va_end(ap);
}

int main(void)
{
	Buffer aa;
	strcpy(aa.buf, "asdfasdf");
	test_call("zhang", aa, 'a', "hua");
	return 0;
}

