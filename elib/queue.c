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
	e_thread_mutex_init(&queue->lock, NULL);

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

	e_thread_mutex_lock(&queue->lock);
	while (e_queue_space(queue) < size) {
		e_thread_mutex_unlock(&queue->lock);
		e_sem_wait(&queue->w_sem);
		e_thread_mutex_lock(&queue->lock);
	}

	retval = e_queue_write(queue, buf, size);

	e_sem_getvalue(&queue->r_sem, &semval);
	if (semval < 1)
		e_sem_post(&queue->r_sem);
	e_thread_mutex_unlock(&queue->lock);
	return retval;
}

eint e_queue_read_wait(Queue *queue, ePointer buf, eint size)
{
	eint semval;
	eint retval;

	e_thread_mutex_lock(&queue->lock);
	while (e_queue_size(queue) < size) {
		e_thread_mutex_unlock(&queue->lock);
		e_sem_wait(&queue->r_sem);
		e_thread_mutex_lock(&queue->lock);
	}

	retval = e_queue_read(queue, buf, size);

	e_sem_getvalue(&queue->w_sem, &semval);
	if (semval < 1)
		e_sem_post(&queue->w_sem);
	e_thread_mutex_unlock(&queue->lock);

	return retval;
}

eint e_queue_write_try(Queue *queue, ePointer buf, eint size)
{
	eint retval;

	e_thread_mutex_lock(&queue->lock);
	if (e_queue_space(queue) < size)
		retval = 0;
	else
		retval = e_queue_write(queue, buf, size);
	e_thread_mutex_unlock(&queue->lock);

	return retval;
}

eint e_queue_read_try(Queue *queue, ePointer buf, eint size)
{
	eint retval;

	e_thread_mutex_lock(&queue->lock);
	if (e_queue_size(queue) < size)
		retval = 0;
	else
		retval = e_queue_read(queue, buf, size);
	e_thread_mutex_unlock(&queue->lock);

	return retval;
}

static eint __queue_peek(Queue *queue, ePointer buf, eint size)
{
	echar *t = (echar *)buf;
	eint pos = queue->pos;

	if (pos + size > queue->size) {
		eint n = queue->size - pos;
		e_memcpy(t, queue->data + pos, n);
		pos   = 0;
		size -= n;
		t    += n;
	}

	e_memcpy(t, queue->data + pos, size);
	t += size;

	return (eint)(t - (echar *)buf);
}

eint e_queue_peek(Queue *queue, ePointer buf, eint size)
{
	eint retval;

	e_thread_mutex_lock(&queue->lock);
	if (e_queue_size(queue) < size)
		retval = 0;
	else
		retval = __queue_peek(queue, buf, size);
	e_thread_mutex_unlock(&queue->lock);

	return retval;
}

static eint __queue_seek(Queue *queue, ePointer buf, eint offset, eint size)
{
	echar *t = (echar *)buf;
	eint pos = queue->pos;

	if (pos + offset > queue->size)
		pos = pos + offset - queue->size;
	else
		pos = pos + offset;

	if (pos + size > queue->size) {
		eint n = queue->size - pos;
		e_memcpy(t, queue->data + pos, n);
		pos   = 0;
		size -= n;
		t    += n;
	}

	e_memcpy(t, queue->data + pos, size);
	t += size;

	return (eint)(t - (echar *)buf);
}

eint e_queue_seek(Queue *queue, ePointer buf, eint offset, eint size)
{
	eint retval;

	e_thread_mutex_lock(&queue->lock);
	if (e_queue_size(queue) < size + offset)
		retval = 0;
	else
		retval = __queue_seek(queue, buf, offset, size);
	e_thread_mutex_unlock(&queue->lock);

	return retval;
}
