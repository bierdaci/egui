#include "queue.h"
#include "std.h"

Queue *e_queue_new(eint max)
{
	Queue *queue;

	queue = e_malloc(sizeof(Queue) + max);
	queue->pos = 0;
	queue->len = 0;
	queue->size = max;
	e_sem_init(&queue->r_sem, 0);
	e_sem_init(&queue->w_sem, 0);
	e_pthread_mutex_init(&queue->lock, NULL);

	return queue;
}

eint e_queue_read(Queue *queue, ePointer buf, eint size)
{
	echar *t = (echar *)buf;

	if (size > queue->len)
		size = queue->len;

	if (queue->pos + size > queue->size) {
		eint n = queue->size - queue->pos;
		e_memcpy(t, queue->data + queue->pos, n);
		queue->pos  = 0;
		queue->len -= n;
		size -= n;
		t    += n;
	}

	e_memcpy(t, queue->data + queue->pos, size);
	queue->pos += size;
	queue->len -= size;
	t += size;

	if (queue->len == 0)
		queue->pos = 0;

	return (eint)(t - (echar *)buf);
}

eint e_queue_write(Queue *queue, ePointer buf, eint size)
{
	echar *t = (echar *)buf;

	if (size > queue->size - queue->len)
		size = queue->size - queue->len;

	while (size > 0) {
		eint n, begin;

		if (queue->len == 0) {
			begin = 0;
			n = queue->size;
		}
		else {
			begin = (queue->pos + queue->len) % queue->size;
			if (begin > queue->pos)
				n = queue->size - begin;
			else
				n = queue->pos - begin;
		}

		if (n > size) n = size;

		e_memcpy(queue->data + begin, t, n);

		queue->len += n;
		size -= n;
		t    += n;
	}

	return (eint)(t - (echar *)buf);
}

eint e_queue_write_wait(Queue *queue, ePointer buf, eint size)
{
	eint semval;
	eint retval;

	e_pthread_mutex_lock(&queue->lock);
	while (e_queue_space(queue) < size) {
		e_pthread_mutex_unlock(&queue->lock);
		e_sem_wait(&queue->w_sem);
		e_pthread_mutex_lock(&queue->lock);
	}

	retval = e_queue_write(queue, buf, size);

	e_sem_getvalue(&queue->r_sem, &semval);
	if (semval < 1)
		e_sem_post(&queue->r_sem);
	e_pthread_mutex_unlock(&queue->lock);
	return retval;
}

eint e_queue_read_wait(Queue *queue, ePointer buf, eint size)
{
	eint semval;
	eint retval;

	e_pthread_mutex_lock(&queue->lock);
	while (e_queue_size(queue) < size) {
		e_pthread_mutex_unlock(&queue->lock);
		e_sem_wait(&queue->r_sem);
		e_pthread_mutex_lock(&queue->lock);
	}

	retval = e_queue_read(queue, buf, size);

	e_sem_getvalue(&queue->w_sem, &semval);
	if (semval < 1)
		e_sem_post(&queue->w_sem);
	e_pthread_mutex_unlock(&queue->lock);

	return retval;
}
