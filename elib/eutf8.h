#ifndef __ELIB_UTF8__
#define __ELIB_UTF8__

#include <elib/types.h>

extern const echar * const e_utf8_skip;
#define e_utf8_next_char(p)		\
	((echar *)((p) + e_utf8_skip[*(const euchar *)(p)]))
#define e_utf8_char_len(p)		\
	((eint)e_utf8_skip[*(const euchar *)(p)])

eunichar e_utf8_get_char(const echar *p);
elong e_utf8_strlen(const echar *p, essize max);

#endif
