#ifndef __ELIB_TYPES_H__
#define __ELIB_TYPES_H__

#include <egconfig.h>

#undef  MAX
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))

#undef  MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
typedef enum {false = 0, true = 1} bool;

#ifndef __ELIB_STD_H__
#endif

typedef long				elong;
typedef char				eint8;
typedef short				eint16;
typedef int					eint;
typedef int					eint32;
typedef long long			eint64;
typedef long long			edlong;
typedef unsigned char		echar;
typedef unsigned char		euchar;
typedef unsigned char		euint8;
typedef unsigned short		euint16;
typedef unsigned long		euint32;
typedef unsigned long long	euint64;
typedef unsigned long		eulong;
typedef unsigned long long	eudlong;
typedef unsigned int		euint;
typedef double				efloat;
typedef double				edouble;
typedef void				evoid;

typedef eulong 				eunichar;
typedef signed int 			essize;

typedef unsigned long		eHandle;
typedef void *				ePointer;
typedef const void *		eConstPointer;

//typedef size_t			esize_t;

typedef void (*eVoidFunc)(void);

#ifndef NULL
#  ifdef __cplusplus
#    define NULL		(0L)
#  else
#    define NULL		((void *)0)
#  endif
#endif
#define ENULL			((ePointer)NULL)
#define E_LITTLE_ENDIAN 1234
#define E_BIG_ENDIAN    4321
#define E_PDP_ENDIAN    3412

typedef eint		(*eCompareFunc)(eConstPointer a, eConstPointer b);
typedef eint		(*eCompareDataFunc)(eConstPointer a, eConstPointer b, ePointer user_data);
typedef void		(*eFunc)(ePointer data, ePointer user_data);
typedef void		(*eDestroyNotify)(ePointer data);
#define _(a)		((const echar *)(a))

#define STRUCT_OFFSET(type, offset) ((euint32)(&((type *)0)->offset))
#define TABLES_SIZEOF(arr)           (sizeof(arr) / sizeof((arr)[0]))

#endif
