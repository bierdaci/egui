#include <stdlib.h>

#include "thread.h"

struct pthread_arg {
	ThreadPool *tp;
	Pthread_t  *th;
	sem_t sem;
};
static void *pthread_handler(void *data);

ThreadPool* create_thread_pool(int max)
{
	ThreadPool *tp;

	tp = (ThreadPool *)malloc(sizeof(ThreadPool) + sizeof(Pthread_t) * max);
	tp->pool = (Pthread_t *)(tp + 1);
	tp->head = NULL;
	tp->cur = 0;
	tp->max = max;
	pthread_mutex_init(&tp->lock, NULL);

	return tp;
}

static int alloc_thread_pool(ThreadPool *tp, int num)
{
	pthread_t pid;
	int i;
	struct pthread_arg ptharg;

	ptharg.tp = tp;
	sem_init(&ptharg.sem, 0, 0);
	for (i = 0; tp->cur+1 < tp->max && i < num; i++, tp->cur++) {
		Pthread_t *th = &tp->pool[tp->cur];

		th->arg = NULL;
		th->handler = NULL;	
		pthread_mutex_init(&th->lock, NULL);
		pthread_mutex_lock(&th->lock);

		ptharg.th = th;
		if (pthread_create(&pid, NULL, pthread_handler, &ptharg) < 0)
			break;
		sem_wait(&ptharg.sem);

		th->pid = pid;
		th->next = tp->head;
		tp->head = th;
	}
	sem_destroy(&ptharg.sem);

	return i;
}

int alloc_thread(ThreadPool *tp, THandler handler, void *arg)
{
	Pthread_t *th;

	pthread_mutex_lock(&tp->lock);

	if (!tp->head && !alloc_thread_pool(tp, 10)) {
		pthread_mutex_unlock(&tp->lock);
		return -1;
	}

	th = tp->head;
	tp->head = th->next;
	th->arg  = arg;
	th->handler = handler;
	pthread_mutex_unlock(&th->lock);

	pthread_mutex_unlock(&tp->lock);

	return 0;
}

static void free_thread(ThreadPool *tp, Pthread_t *th)
{
	pthread_mutex_lock(&tp->lock);
	th->arg     = NULL;
	th->handler = NULL;
	th->next = tp->head;
	tp->head = th;
	pthread_mutex_unlock(&tp->lock);
}

static void *pthread_handler(void *data)
{
	struct pthread_arg *parg = (struct pthread_arg *)data;
	ThreadPool *tp = parg->tp;
	Pthread_t *th = parg->th;

	sem_post(&parg->sem);

	while (1) {
		pthread_mutex_lock(&th->lock);

		if (th->handler)
			th->handler(th->arg);

		free_thread(tp, th);
	}
	return NULL;
}
