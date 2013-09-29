#ifndef __ELIB_QUEUE_H_
#define __ELIB_QUEUE_H_
#include <elib/std.h>

typedef struct _Queue Queue;

struct _Queue {
    eint pos;
    eint len;
    eint size;
	e_pthread_mutex_t lock;
	e_sem_t r_sem, w_sem;
    echar data[0];
};

inline static eint e_queue_size(Queue *queue)
{
	return queue->len;
}

inline static eint e_queue_space(Queue *queue)
{
	return queue->size - queue->len;
}

inline static eint e_queue_empty(Queue *queue)
{
	return (queue->len == 0);
}

eint e_queue_read(Queue *, void *, eint);
eint e_queue_write(Queue *, void *, eint);
Queue *e_queue_new(eint);
eint e_queue_write_wait(Queue *queue, ePointer buf, eint size);
eint e_queue_read_wait(Queue *queue, ePointer buf, eint size);

#endif
