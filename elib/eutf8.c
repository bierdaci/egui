#include "eutf8.h"

#define UTF8_COMPUTE(Char, Mask, Len)		\
if (Char < 128)								\
{											\
	Len = 1;								\
	Mask = 0x7f;							\
}											\
else if ((Char & 0xe0) == 0xc0)				\
{											\
	Len = 2;								\
	Mask = 0x1f;							\
}											\
else if ((Char & 0xf0) == 0xe0)				\
{											\
	Len = 3;								\
	Mask = 0x0f;							\
}											\
else if ((Char & 0xf8) == 0xf0)				\
{											\
	Len = 4;								\
	Mask = 0x07;							\
}											\
else if ((Char & 0xfc) == 0xf8)				\
{											\
	Len = 5;								\
	Mask = 0x03;							\
}											\
else if ((Char & 0xfe) == 0xfc)				\
{											\
	Len = 6;								\
	Mask = 0x01;							\
}											\
else										\
	Len = -1;

#define UTF8_GET(Result, Chars, Count, Mask, Len)			\
(Result) = (Chars)[0] & (Mask);								\
for ((Count) = 1; (Count) < (Len); ++(Count))				\
{															\
    if (((Chars)[(Count)] & 0xc0) != 0x80)					\
    {														\
        (Result) = -1;										\
        break;												\
    }														\
    (Result) <<= 6;											\
    (Result) |= ((Chars)[(Count)] & 0x3f);					\
}

static const echar utf8_skip_data[256] = {
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,6,6,1,1
};
                   
const echar * const e_utf8_skip = utf8_skip_data;

eunichar e_utf8_get_char(const echar *p)
{
    int i, mask = 0, len;
    eunichar result;
    euchar c = (euchar)*p;

    UTF8_COMPUTE(c, mask, len);
    if (len == -1)
        return (eunichar)-1;
    UTF8_GET(result, p, i, mask, len);

    return result;
}

elong e_utf8_strlen(const echar *p, essize max)
{
    elong len = 0;
    const echar *start = p;

    if (!p) return 0;

    if (max < 0)
	{
        while (*p) {
            p = e_utf8_next_char(p);
            ++len;
        }
    }
    else
	{
        if (max == 0 || !*p)
            return 0;

        p = e_utf8_next_char(p);

        while (p - start < max && *p) {
            ++len;
            p = e_utf8_next_char(p);
        }

        if (p - start <= max)
            ++len;
    }

    return len;
}

