#ifndef __ELIB_TYPES_H__
#define __ELIB_TYPES_H__

#undef  MAX
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))

#undef  MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))

#ifndef __ELIB_BOOL
#define __ELIB_BOOL
typedef enum {false = 0, true = 1} bool;
#endif

#ifndef __ELIB_STD_H__
#endif
typedef enum {efalse = 0, etrue = 1} ebool;
typedef long				elong;
typedef short				eint16;
typedef int					eint;
typedef int					eint32;
typedef short				eshort;

typedef char				echar;
typedef unsigned char		euchar;
typedef unsigned char		ebyte;
typedef char				eint8;
typedef unsigned char		euint8;
typedef unsigned short		euint16;
typedef unsigned int 		euint32;
typedef unsigned long		eulong;
typedef unsigned short		eushort;

#ifdef WIN32

#define INLINE
typedef __int64				eint64;
typedef __int64				ellong;
typedef unsigned __int64	euint64;
typedef unsigned __int64	eullong;

#elif linux

#define INLINE inline
typedef long long			eint64;
typedef long long			ellong;
typedef unsigned long long	euint64;
typedef unsigned long long	eullong;

#endif

typedef unsigned int		euint;
typedef float				efloat;
typedef double				edouble;
typedef void				evoid;

#ifdef WIN32
typedef eushort				eunichar;
#else
typedef euint 				eunichar;
#endif
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

#define STRUCT_OFFSET(type, offset)  ((eulong)(&((type *)0)->offset))
#define TABLES_SIZEOF(arr)           (sizeof(arr) / sizeof((arr)[0]))

#endif
