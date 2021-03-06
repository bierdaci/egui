#ifndef __ELIB_STD_H__
#define __ELIB_STD_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include <sys/types.h>

#ifdef WIN32

#define _WIN32_WINNT	0x0500

#include <windows.h>
#include <io.h>

#pragma comment(lib, "ws2_32.lib")

#else

#include <unistd.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <pthread.h>
#include <semaphore.h>
#include <dirent.h>
#include <errno.h>
#include <signal.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#endif

#include <elib/types.h>
#include <elib/memory.h>

typedef va_list				eValist;

#ifndef _INTSIZEOF
#define _INTSIZEOF(n)		((sizeof(n) + sizeof(eulong)-1) & ~(sizeof(eulong) - 1))
#endif
#define e_va_start(ap, v)	va_start(ap, v)
#define e_va_arg(ap, t)		va_arg(ap, t)
#define e_va_end(ap)		va_end(ap)

#ifdef WIN32

#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
#define DELTA_EPOCH_IN_MICROSECS 11644473600000000Ui64
#else
#define DELTA_EPOCH_IN_MICROSECS 11644473600000000ULL
#endif

struct timezone {
	int tz_minuteswest;
	int tz_dsttime;
};

int gettimeofday(struct timeval *tp, struct timezone *tzp);

typedef struct {
	HANDLE handle;
	LONG value;
} handle_sem_t;

#define e_thread_t				HANDLE
#define e_thread_mutexattr_t	ePointer
#define e_thread_rwlockattr_t	ePointer
#define e_thread_mutex_t		handle_sem_t
//#define e_thread_rwlock_t		SRWLOCK
#define e_thread_rwlock_t		HANDLE
#define e_thread_cond_t			CRITICAL_SECTION
#define e_thread_condattr_t		CRITICAL_SECTION
#define e_sem_t					handle_sem_t

#define PROT_READ	0x1
#define PROT_WRITE	0x2
#define PROT_EXEC	0x4

#define MAP_SHARED	0x01
#define MAP_PRIVATE	0x02

enum {
    DT_UNKNOWN = 0,
    DT_FIFO    = 1,
    DT_CHR     = 2,
    DT_DIR     = 4,
    DT_BLK     = 6,
    DT_REG     = 8,
    DT_LNK     = 10,
    DT_SOCK    = 12,
    DT_WHT     = 14
};

typedef struct {
    eint     dd_fd;
    elong    dd_loc;
    elong    dd_size;
    echar    *dd_buf;
    eint     dd_len;
    elong    dd_seek;
	HANDLE  hFind;
} DIR;

struct dirent {
     elong  d_ino;
     off_t  d_off;
     eulong d_reclen;
     euchar d_type;
     echar  d_name[256];
};

typedef struct {
	HANDLE fd;
	HANDLE map;
} eFileMap;

typedef eHandle eMapHandle;
typedef SOCKET eSocket_t;

#define __dirfd(dp)    ((dp)->dd_fd)
#ifndef _WIN64
extern FILE _iob[3];
#endif
DIR *e_opendir(const echar *);
struct dirent *e_readdir(DIR *);
void e_rewinddir(DIR *);
eint e_closedir(DIR *);

static INLINE void e_sleep(int time)
{
	Sleep(time * 1000);
}

#else

typedef int eSocket_t;
typedef int eMapHandle;

#define e_thread_t				pthread_t
#define e_thread_mutexattr_t	pthread_mutexattr_t
#define e_thread_rwlockattr_t	pthread_rwlockattr_t
#define e_thread_mutex_t		pthread_mutex_t
#define e_thread_rwlock_t		pthread_rwlock_t
#define e_thread_cond_t			pthread_cond_t
#define e_thread_condattr_t		pthread_condattr_t
#define e_sem_t					sem_t

static INLINE DIR *e_opendir(const char *path)
{
	return opendir(path);
}

static INLINE struct dirent *e_readdir(DIR *dir)
{
	return readdir(dir);
}

static INLINE void e_rewinddir(DIR *dir)
{
	rewinddir(dir);
}

static INLINE eint e_closedir(DIR *dir)
{
	return closedir(dir);
}

#define e_sleep	sleep

#endif
int e_open_file(const char *filename, int flags);
void e_close_file(int fd);
eMapHandle e_map_open_file(const char *name, int flags);
void e_map_close(eMapHandle fd);
int  e_map_read_file(eMapHandle hmap, void *buf, int size);
int  e_map_write_file(eMapHandle hmap, void *buf, int size);
int  e_map_seek(eMapHandle hmap, int offset, int whence);
int  e_map_get_size(eMapHandle hmap);
void *e_mmap(void *addr, size_t length, int prot, int flags, eMapHandle fd, off_t offset);
int  e_munmap(void *addr, size_t length);
int  e_socket_init(void);
int  e_socket_close(eSocket_t fd);

eint e_thread_create(e_thread_t *thread, void *(*routine)(void *), ePointer arg);
eint e_thread_join(e_thread_t thread, void **retval);
eint e_thread_mutex_init(e_thread_mutex_t *mutex, const e_thread_mutexattr_t *mutexattr);
eint e_thread_mutex_lock(e_thread_mutex_t *mutex);
eint e_thread_mutex_trylock(e_thread_mutex_t *mutex);
eint e_thread_mutex_unlock(e_thread_mutex_t *mutex);
eint e_thread_mutex_destroy(e_thread_mutex_t *mutex);
eint e_thread_rwlock_destroy(e_thread_rwlock_t *rwlock);
eint e_thread_rwlock_init(e_thread_rwlock_t *rwlock, e_thread_rwlockattr_t *attr);
eint e_thread_rwlock_rdlock(e_thread_rwlock_t *rwlock);
eint e_thread_rwlock_wrlock(e_thread_rwlock_t *rwlock);
eint e_thread_rwlock_tryrdlock(e_thread_rwlock_t *rwlock);
eint e_thread_rwlock_trywrlock(e_thread_rwlock_t *rwlock);
eint e_thread_rwlock_unlock(e_thread_rwlock_t *rwlock);
eint e_thread_rwlock_rdunlock(e_thread_rwlock_t *rwlock);
eint e_thread_rwlock_wrunlock(e_thread_rwlock_t *rwlock);
eint e_thread_cond_init(e_thread_cond_t *, e_thread_condattr_t *);
eint e_thread_cond_broadcast(e_thread_cond_t *);
eint e_thread_cond_wait(e_thread_cond_t *, e_thread_mutex_t *);
eint e_sem_init(e_sem_t *sem, euint value);
eint e_sem_destroy(e_sem_t *sem);
eint e_sem_post(e_sem_t *sem);
eint e_sem_wait(e_sem_t *sem);
eint e_sem_trywait(e_sem_t *sem);
eint e_sem_timedwait(e_sem_t *sem, const struct timespec *abs_timeout);
eint e_sem_getvalue(e_sem_t *sem, eint *val);
echar *e_strcasestr(const echar *s1, const echar *s2);

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
#define ALIGN_WORD(size) ((size + sizeof(elong) - 1) & ~(sizeof(elong) - 1))

static INLINE void *e_malloc(euint size)
{
	if (!size)
		return NULL;
	return e_heap_malloc(ALIGN_WORD(size), efalse);
}

static INLINE ePointer e_realloc(ePointer ptr, euint size)
{
	if (!size) {
		e_heap_free(ptr);
		return NULL;
	}
	if (ptr)
		return e_heap_realloc(ptr, ALIGN_WORD(size));
	return e_heap_malloc(ALIGN_WORD(size), efalse);
}

static INLINE ePointer e_calloc(euint nmemb, euint size)
{
	eint    nsize = ALIGN_WORD(size) * nmemb;
	ePointer addr = e_heap_malloc(nsize, efalse);
	e_memset(addr, 0, nsize);
	return addr;
}

static INLINE void e_free(ePointer ptr)
{
	e_heap_free(ptr);
}

static INLINE eint e_strlen(const echar *s)
{
	return strlen((const char *)s);
}

static INLINE echar *e_strncpy(echar *dest, const echar *src, size_t n)
{
	return (echar *)strncpy((char *)dest, (const char *)src, n);
}

static INLINE echar *e_strcat(echar *dest, const echar *src)
{
	return (echar *)strcat((char *)dest, (const char *)src);
}

static INLINE echar *e_strncat(echar *dest, const echar *src, eint n)
{
	return (echar *)strncat((char *)dest, (const char *)src, n);
}

static INLINE echar *e_strstr(const echar *s1, const echar *s2)
{
	return (echar *)strstr((const char *)s1, (const char *)s2);
}

static INLINE echar *e_strrchr(const echar *s, int c)
{
	return (echar *)strrchr((const char *)s, c);
}

static INLINE echar *e_strchr(const echar *s, int c)
{
	return (echar *)strchr((const char *)s, c);
}

static INLINE echar *e_strcpy(echar *s1, const echar *s2)
{
	return (echar *)strcpy((char *)s1, (const char *)s2);
}

static INLINE int e_strcmp(const echar *s1, const echar *s2)
{
	return strcmp((const char *)s1, (const char *)s2);
}

static INLINE eint e_strncmp(const echar *s1, const echar *s2, eint n)
{
	return strncmp((const char *)s1, (const char *)s2, n);
}

static INLINE int e_strcasecmp(const echar *s1, const echar *s2)
{
#ifdef WIN32
	return stricmp((const char *)s1, (const char *)s2);
#else
	return strcasecmp((const char *)s1, (const char *)s2);
#endif
}

static INLINE int e_strncasecmp(const echar *s1, const echar *s2, size_t n)
{
#ifdef WIN32
	return strnicmp((const char *)s1, (const char *)s2, n);
#else
	return strncasecmp((const char *)s1, (const char *)s2, n);
#endif
}

static INLINE echar *e_strdup(const echar *s)
{
	echar *dst = e_malloc(e_strlen(s) + 1);
	e_strcpy(dst, s);
	return  dst;
}

static INLINE echar *e_strndup(const echar *s, size_t n)
{
	echar *dst = e_malloc(n + 1);
	e_memcpy(dst, s, n);
	dst[n] = 0;
	return  dst;
}

static INLINE FILE *e_fopen(const echar *path, const echar *mode)
{
	return fopen((const char *)path, (const char *)mode);
}

static INLINE echar *e_fgets(echar *s, int size, FILE *stream)
{
	return (echar *)fgets((char *)s, size, stream);
}

static INLINE eint e_printf(const echar *format, ...)
{
	eint ret;
	va_list ap;
	va_start(ap, format);
	ret = (eint)vprintf((const char *)format, ap);
	va_end(ap);
	return ret;
}

static INLINE eint e_sprintf(echar *str, const echar *format, ...)
{
	eint ret;
	va_list ap;
	va_start(ap, format);
	ret = (eint)vsprintf((char *)str, (const char *)format, ap);
	va_end(ap);
	return ret;
}

static INLINE eint e_atoi(const echar *nptr)
{
	return (eint)atoi((const char *)nptr);
}

static INLINE elong e_atol(const echar *nptr)
{
	return (elong)atol((const char *)nptr);
}

static INLINE edouble e_atof(const echar *nptr)
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
