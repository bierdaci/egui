#include "std.h"
#include "elist.h"
#include <time.h>

#ifdef WIN32

int gettimeofday(struct timeval *tp, void *tzp)
{
    time_t clock;
    struct tm tm;
    SYSTEMTIME wtm;
    GetLocalTime(&wtm);
    tm.tm_year = wtm.wYear - 1900;
    tm.tm_mon  = wtm.wMonth - 1;
    tm.tm_mday = wtm.wDay;
    tm.tm_hour = wtm.wHour;
    tm.tm_min  = wtm.wMinute;
    tm.tm_sec  = wtm.wSecond;
    tm.tm_isdst = -1;
    clock = mktime(&tm);
    tp->tv_sec = clock;
    tp->tv_usec = wtm.wMilliseconds * 1000;
    return 0;
}

eint e_thread_create(e_thread_t *thread, void *(*routine)(void *), ePointer arg)
{
	*thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)routine, arg, 0, NULL);
	if (*thread == 0)
		return -1;
	return 0;
}

eint e_thread_mutex_init(e_thread_mutex_t *mutex, const e_thread_mutexattr_t *mutexattr)
{
	mutex->handle = CreateMutex(NULL, FALSE, NULL);
	mutex->value  = 1;
	return 0;
}

eint e_thread_mutex_lock(e_thread_mutex_t *mutex)
{
	mutex->value--;
	WaitForSingleObject(mutex->handle, INFINITE);
	return 0;
}

eint e_thread_mutex_trylock(e_thread_mutex_t *mutex)
{
	return -1;
}

eint e_thread_mutex_unlock(e_thread_mutex_t *mutex)
{
	mutex->value++;
	ReleaseMutex(mutex->handle);
	return 0;
}

eint e_thread_mutex_destroy(e_thread_mutex_t *mutex)
{
	CloseHandle(mutex->handle);
	return 0;
}

eint e_thread_cond_init(e_thread_cond_t *cond, e_thread_condattr_t *attr)
{
	return -1;
}

eint e_thread_cond_broadcast(e_thread_cond_t *cond)
{
	return -1;
}

eint e_thread_cond_wait(e_thread_cond_t *cond, e_thread_mutex_t *mutex)
{
	return -1;
}

eint e_sem_init(e_sem_t *sem, euint value)
{
	sem->handle = CreateSemaphore(NULL, value, 1000, NULL);
	sem->value  = value;
	return 0;
}

eint e_sem_destroy(e_sem_t *sem)
{
	CloseHandle(sem->handle);
	return 0;
}

eint e_sem_post(e_sem_t *sem)
{
	ReleaseSemaphore(sem->handle, 1, NULL);
	sem->value++;
	return 0;
}

eint e_sem_wait(e_sem_t *sem)
{
	sem->value--;
	WaitForSingleObject(sem->handle, INFINITE);
	return 0;
}

eint e_sem_trywait(e_sem_t *sem)
{
	return -1;
}

eint e_sem_timedwait(e_sem_t *sem, const struct timespec *abs_timeout)
{
	return -1;
}

eint e_sem_getvalue(e_sem_t *sem, eint *val)
{
	*val = sem->value;
	return 0;
}

#else

eint e_thread_create(e_thread_t *thread, void *(*routine)(void *), ePointer arg)
{
	return pthread_create(thread, NULL, routine, arg);
}

eint e_thread_mutex_init(e_thread_mutex_t *mutex, const e_thread_mutexattr_t *mutexattr)
{
	return pthread_mutex_init(mutex, mutexattr);
}

eint e_thread_mutex_lock(e_thread_mutex_t *mutex)
{
	return pthread_mutex_lock(mutex);
}

eint e_thread_mutex_trylock(e_thread_mutex_t *mutex)
{
	return pthread_mutex_trylock(mutex);
}

eint e_thread_mutex_unlock(e_thread_mutex_t *mutex)
{
	return pthread_mutex_unlock(mutex);
}

eint e_thread_mutex_destroy(e_thread_mutex_t *mutex)
{
	return pthread_mutex_destroy(mutex);
}

eint e_thread_cond_init(e_thread_cond_t *cond, e_thread_condattr_t *attr)
{
	return e_thread_cond_init(cond, attr);
}

eint e_thread_cond_broadcast(e_thread_cond_t *cond)
{
	return pthread_cond_broadcast(cond);
}

eint e_thread_cond_wait(e_thread_cond_t *cond, e_thread_mutex_t *mutex)
{
	return pthread_cond_wait(cond, mutex);
}

eint e_sem_init(e_sem_t *sem, euint value)
{
	return sem_init(sem, 0, value);
}

eint e_sem_destroy(e_sem_t *sem)
{
	return sem_destroy(sem);
}

eint e_sem_post(e_sem_t *sem)
{
	return sem_post(sem);
}

eint e_sem_wait(e_sem_t *sem)
{
	return sem_wait(sem);
}

eint e_sem_trywait(e_sem_t *sem)
{
	return sem_trywait(sem);
}

eint e_sem_timedwait(e_sem_t *sem, const struct timespec *abs_timeout)
{
	return sem_timedwait(sem, abs_timeout);
}

eint e_sem_getvalue(e_sem_t *sem, eint *val)
{
	return sem_getvalue(sem, val);
}

#endif

#if 0
char *e_strrchr(const char *s, int c)
{
	return strrchr(s, c);
}
#endif

echar** e_strsplit(const echar *str, const echar *delimiter, eint max)
{
	elist_t list = {sizeof(echar *), 0, NULL, NULL};

	echar **str_array, *s;
	euint n = 0;
	const echar *remain;
	echar *new;

	e_return_val_if_fail(str != NULL, NULL);
	e_return_val_if_fail(delimiter != NULL, NULL);
	e_return_val_if_fail(delimiter[0] != '\0', NULL);

	if (max < 1)
		max = 0x7fff;

	remain = str;
	s = e_strstr(remain, delimiter);
	if (s) {
		eint delimiter_len = e_strlen(delimiter);

		while (--max && s) {
			eint   len;     

			len = s - remain;
			new = e_malloc(len + 1);
			e_strncpy(new, remain, len);
			new[len] = 0;
			e_list_push_data(&list, &new);
			n++;
			remain = s + delimiter_len;
			s = e_strstr(remain, delimiter);
		}
	}

	if (*str) {
		new = e_strdup(remain);
		n++;
		e_list_push_data(&list, &new);
	}

	str_array = e_calloc(sizeof(echar *), n + 1);

	str_array[n--] = NULL;
	while (list.head) {
		e_list_pop_data(&list, &str_array[n--]);
	}

	return str_array;
}

void e_strfreev(echar **str_array)
{
	if (str_array) {
		int i;
		for(i = 0; str_array[i]; i++)
			e_free(str_array[i]);
		e_free(str_array);
	}
}

echar *e_strcasestr(const echar *s1, const echar *s2)
{
#ifdef WIN32
	echar c, sc;
	size_t len;

	if ((c = *s2++) != 0) {
		c = tolower((euchar)c);
		len = e_strlen(s2);
		do {
			do {
				if ((sc = *s1++) == 0)
					return NULL;
			} while ((echar)tolower((euchar)sc) != c);
		} while (e_strncasecmp(s1, s2, len) != 0);
		s1--;
	}
	return (echar *)s1;
#else
	return (echar *)strcasestr(s1, s2);
#endif
}
