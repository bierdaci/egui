#ifndef __ELIB_UTF8__
#define __ELIB_UTF8__

#include <elib/types.h>

#ifdef linux

extern const euchar * const e_utf8_skip;
#define e_uni_next_char(p)	((euchar *)((p) + e_utf8_skip[*(const euchar *)(p)]))
#define e_uni_char_len(p)	((eint)e_utf8_skip[*(const euchar *)(p)])
#define e_uni_strlen		e_utf8_strlen
#define e_uni_get_char		e_utf8_get_char

eunichar e_utf8_get_char(const euchar *p);
eint e_utf8_strlen(const euchar *p, essize max);

#elif WIN32

#define e_uni_char_len		e_gb2312_char_len
#define e_uni_strlen		e_gb2312_strlen
#define e_uni_get_char		e_gb2312_get_char

eint e_gb2312_char_len(const euchar *);
eunichar e_gb2312_get_char(const euchar *);
eint e_gb2312_strlen(const euchar *p, essize max);

#endif

#endif
