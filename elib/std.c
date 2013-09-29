#include "std.h"
#include "elist.h"

eint e_pthread_mutex_init(e_pthread_mutex_t *mutex, const e_pthread_mutexattr_t *mutexattr)
{
	return pthread_mutex_init(mutex, mutexattr);
}

eint e_pthread_mutex_lock(e_pthread_mutex_t *mutex)
{
	return pthread_mutex_lock(mutex);
}

eint e_pthread_mutex_trylock(e_pthread_mutex_t *mutex)
{
	return pthread_mutex_trylock(mutex);
}

eint e_pthread_mutex_unlock(e_pthread_mutex_t *mutex)
{
	return pthread_mutex_unlock(mutex);
}

eint e_pthread_mutex_destroy(e_pthread_mutex_t *mutex)
{
	return pthread_mutex_destroy(mutex);
}

eint e_pthread_cond_init(e_pthread_cond_t *cond, e_pthread_condattr_t *attr)
{
	return e_pthread_cond_init(cond, attr);
}

eint e_pthread_cond_broadcast(e_pthread_cond_t *cond)
{
	return pthread_cond_broadcast(cond);
}

eint e_pthread_cond_wait(e_pthread_cond_t *cond, e_pthread_mutex_t *mutex)
{
	return pthread_cond_wait(cond, mutex);
}

eint e_sem_init(e_sem_t *sem, euint value)
{
	return sem_init(sem, 0, value);
}

eint e_sem_destroy(sem_t *sem)
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

#if 0
char *e_strrchr(const char *s, int c)
{
	return strrchr(s, c);
}
#endif

echar** e_strsplit(const echar *str, const echar *delimiter, eint max)
{
	elist_t *list;
	elist_t *head = NULL;

	echar **str_array, *s;
	euint n = 0;
	const echar *remain;

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
			echar *new;

			len = s - remain;
			new = e_malloc(len + 1);
			e_strncpy(new, remain, len);
			new[len] = 0;
			head = e_list_prepend(head, new);
			n++;
			remain = s + delimiter_len;
			s = e_strstr(remain, delimiter);
		}
	}

	if (*str) {
		n++;
		head = e_list_prepend(head, e_strdup(remain));
	}

	str_array = e_calloc(sizeof(echar *), n + 1);

	str_array[n--] = NULL;
	for (list = head; list; list = list->next)
		str_array[n--] = list->data;

	e_list_free(head);

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

