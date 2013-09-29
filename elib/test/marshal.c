#include <stdio.h>
#include <stdarg.h>

typedef va_list             eValist;

#ifndef _INTSIZEOF
#define _INTSIZEOF(n)       ((sizeof(n) + sizeof(int)-1) & ~(sizeof(int) - 1))
#endif
#define e_va_start(ap, v)   (ap = (eValist)&v + _INTSIZEOF(v))
#define e_va_arg(ap, t)     (*(t *)((ap += _INTSIZEOF(t)) - _INTSIZEOF(t)))
#define e_va_end(ap)        (ap = (eValist)0)


typedef struct {
	char buf[15];
	int i;
} Buffer;

int test_print(void *obj, char *data, char *p1, Buffer bb, char c, char *p3)
{
	return printf("%s  --  %s --  %s  %c  %s  %s\n", (char *)obj, data, p1, c, p3, bb.buf);
}

#if 0
int __signal_call_marshal(char *obj, int (*func)(void *, char *), void *data, eValist vp, int size)
{
	return func(obj, data);
}
#endif

static int test_call(int i, char *buf, Buffer bb, char c, char *buf2)
{
	eValist ap;
	printf("%d\n", sizeof(bb));

	e_va_start(ap, i);
	__signal_call_marshal("hello", test_print, "world!", ap, 12 + sizeof(Buffer));
	e_va_end(ap);
}

int main(void)
{
	Buffer aa;
	strcpy(aa.buf, "asdfasdf");
	test_call(0, "zhang", aa, 'a', "hua");
	return 0;
}

