#ifndef __ELIB_STD_H__
#define __ELIB_STD_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <pthread.h>
#include <semaphore.h>
#include <assert.h>

#include <elib/types.h>
#include <elib/memory.h>

typedef va_list				eValist;

#ifndef _INTSIZEOF
#define _INTSIZEOF(n)		((sizeof(n) + sizeof(eulong)-1) & ~(sizeof(eulong) - 1))
#endif
#define e_va_start(ap, v)	va_start(ap, v)
#define e_va_arg(ap, t)		va_arg(ap, t)
#define e_va_end(ap)		va_end(ap)

#define e_pthread_mutexattr_t	pthread_mutexattr_t
#define e_pthread_mutex_t		pthread_mutex_t
#define e_pthread_cond_t		pthread_cond_t
#define e_pthread_condattr_t    pthread_condattr_t
#define e_sem_t					sem_t

eint e_pthread_mutex_init(e_pthread_mutex_t *mutex, const e_pthread_mutexattr_t *mutexattr);
eint e_pthread_mutex_lock(e_pthread_mutex_t *mutex);
eint e_pthread_mutex_trylock(e_pthread_mutex_t *mutex);
eint e_pthread_mutex_unlock(e_pthread_mutex_t *mutex);
eint e_pthread_mutex_destroy(e_pthread_mutex_t *mutex);
eint e_pthread_cond_init(e_pthread_cond_t *, e_pthread_condattr_t *);
eint e_pthread_cond_broadcast(e_pthread_cond_t *);
eint e_pthread_cond_wait(e_pthread_cond_t *, e_pthread_mutex_t *);
eint e_sem_init(e_sem_t *sem, euint value);
eint e_sem_destroy(sem_t *sem);
eint e_sem_post(e_sem_t *sem);
eint e_sem_wait(e_sem_t *sem);
eint e_sem_trywait(e_sem_t *sem);
eint e_sem_timedwait(e_sem_t *sem, const struct timespec *abs_timeout);
eint e_sem_getvalue(e_sem_t *sem, eint *val);

echar** e_strsplit(const echar *str, const echar *delimiter, eint max);
void e_strfreev(echar **str_array);

#define __malloc 			malloc
#define __calloc 			calloc
#define __realloc			realloc
#define __free				free

#define e_memcpy			memcpy
#define e_memset			memset
#define e_memcmp			memcmp
#define e_snprintf			snprintf
#define e_memmove			memmove
#define e_assert			assert

#define e_new(type, nmemb)         e_malloc(sizeof(type) * nmemb)
#define e_renew(type, ptr, nmemb)  e_realloc(ptr, sizeof(type) * nmemb)

static inline ePointer e_malloc(euint size)
{
	if (!size)
		return NULL;
	return e_heap_malloc((size + 3) & ~3, false);
}

static inline ePointer e_realloc(ePointer ptr, euint size)
{
	if (!size) {
		e_heap_free(ptr);
		return NULL;
	}
	if (ptr)
		return e_heap_realloc(ptr, (size + 3) & ~3);
	return e_heap_malloc((size + 3) & ~3, false);
}

static inline ePointer e_calloc(euint nmemb, euint size)
{
	eint    nsize = (nmemb * size + 3) & ~3;
	ePointer addr = e_heap_malloc(nsize, false);
	e_memset(addr, 0, nsize);
	return addr;
}

static inline void e_free(ePointer ptr)
{
	e_heap_free(ptr);
}

static inline eint e_strlen(const echar *s)
{
	return strlen((const char *)s);
}

static inline echar *e_strncpy(echar *dest, const echar *src, size_t n)
{
	return (echar *)strncpy((char *)dest, (const char *)src, n);
}

static inline echar *e_strcat(echar *dest, const echar *src)
{
	return (echar *)strcat((char *)dest, (const char *)src);
}

static inline echar *e_strncat(echar *dest, const echar *src, eint n)
{
	return (echar *)strncat((char *)dest, (const char *)src, n);
}

static inline echar *e_strstr(const echar *s1, const echar *s2)
{
	return (echar *)strstr((const char *)s1, (const char *)s2);
}

static inline echar *e_strrchr(const echar *s, int c)
{
	return (echar *)strrchr((const char *)s, c);
}

static inline echar *e_strchr(const echar *s, int c)
{
	return (echar *)strchr((const char *)s, c);
}

static inline echar *e_strcpy(echar *s1, const echar *s2)
{
	return (echar *)strcpy((char *)s1, (const char *)s2);
}

static inline int e_strcmp(const echar *s1, const echar *s2)
{
	return strcmp((const char *)s1, (const char *)s2);
}

static inline eint e_strncmp(const echar *s1, const echar *s2, eint n)
{
	return strncmp((const char *)s1, (const char *)s2, n);
}

static inline int e_strcasecmp(const echar *s1, const echar *s2)
{
	return strcasecmp((const char *)s1, (const char *)s2);
}

static inline int e_strncasecmp(const echar *s1, const echar *s2, size_t n)
{
	return strncasecmp((const char *)s1, (const char *)s2, n);
}

static inline echar *e_strdup(const echar *s)
{
	echar *dst = e_malloc(e_strlen(s) + 1);
	e_strcpy(dst, s);
	return  dst;
}

static inline echar *e_strndup(const echar *s, size_t n)
{
	echar *dst = e_malloc(n + 1);
	e_memcpy(dst, s, n);
	dst[n] = 0;
	return  dst;
}

static inline FILE *e_fopen(const echar *path, const echar *mode)
{
	return fopen((const char *)path, (const char *)mode);
}

static inline echar *e_fgets(echar *s, int size, FILE *stream)
{
	return (echar *)fgets((char *)s, size, stream);
}

static inline eint e_printf(const echar *format, ...)
{
	eint ret;
	va_list ap;
	va_start(ap, format);
	ret = (eint)vprintf((const char *)format, ap);
	va_end(ap);
	return ret;
}

static inline eint e_sprintf(echar *str, const echar *format, ...)
{
	eint ret;
	va_list ap;
	va_start(ap, format);
	ret = (eint)vsprintf((char *)str, (const char *)format, ap);
	va_end(ap);
	return ret;
}

static inline eint e_atoi(const echar *nptr)
{
	return (eint)atoi((const char *)nptr);
}

static inline elong e_atol(const echar *nptr)
{
	return (elong)atol((const char *)nptr);
}

static inline edouble e_atof(const echar *nptr)
{
	return (edouble)atof((const char *)nptr);
}

#define e_return_val_if_fail(expr, val) \
	do { \
		if (!(expr)) return val; \
	} while (0)

#define e_return_if_fail(expr) \
	do { \
		if (!(expr)) return; \
	} while (0)

#endif
