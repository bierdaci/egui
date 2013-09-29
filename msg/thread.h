#ifndef __THREADPOLL_H
#define __THREADPOLL_H

#include <pthread.h>
#include <semaphore.h>

typedef int (*THandler)(void *data);
typedef struct _Pthread_t Pthread_t;
struct _Pthread_t {
	void *arg;
	THandler handler;
	pthread_mutex_t	lock;
	Pthread_t *next;
	pthread_t pid;
};

typedef struct _ThreadPool {
	int max, cur;
	Pthread_t *pool;
	Pthread_t *head;
	pthread_mutex_t lock;
} ThreadPool;

ThreadPool* create_thread_pool(int max);
int alloc_thread(ThreadPool *tp, THandler handler, void *arg);
#endif
