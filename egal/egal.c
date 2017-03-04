#include "egal.h"

#define EVENT_LIST_MAX  2
static struct EventList {
	GalEvent event;
	struct EventList *next;	
} event_list_buf[EVENT_LIST_MAX];

static struct EventList *free_event_list = NULL;
static struct EventList *wait_event_head = NULL;
static struct EventList *wait_event_tail = NULL;
static e_thread_mutex_t event_list_lock;
static e_sem_t event_list_sem;
static Queue  *event_queue = NULL;
static ebool is_recv = efalse;

#ifndef WIN32
static ePointer wait_event_handler(ePointer data)
{
	GalEvent event;

	while (1) {
		if (egal_window_wait_event(&event, is_recv))
			egal_add_event_to_queue(&event);
	}

	return (ePointer)-1;
}
#endif

/*
static ePointer wait_add_async_event(ePointer data)
{
	GalEvent event;
	struct EventList *tmp;

	while (1) {
		e_sem_wait(&event_list_sem);

		while (wait_event_head) {
			e_thread_mutex_lock(&event_list_lock);
			tmp = wait_event_head;
			wait_event_head = tmp->next;
			tmp->next = free_event_list;
			free_event_list = tmp;
			event = tmp->event;
			e_thread_mutex_unlock(&event_list_lock);

			egal_add_event_to_queue(&event);
		}
	}
	return (ePointer)-1;
}
*/

eint egal_init(eint argc, char *const args[])
{
	e_thread_t pid;
	eint i;

	e_init();
	e_sem_init(&event_list_sem, 0);
	e_thread_mutex_init(&event_list_lock,  NULL);

	event_queue = e_queue_new(sizeof(GalEvent) * EVENT_QUEUE_MAX);
	if (!event_queue)
		return -1;

	for (i = 0; i < EVENT_LIST_MAX; i++) {
		event_list_buf[i].next = free_event_list;
		free_event_list = event_list_buf + i;
	}

	//if (e_thread_create(&pid, wait_add_async_event, NULL) < 0)
	//	return -1;

	if (egal_window_init() < 0)
		return -1;
#ifndef WIN32
	if (e_thread_create(&pid, wait_event_handler, NULL) < 0)
		return -1;
#endif
	return 0;
}

eint egal_event_init(void)
{
	is_recv = etrue;
	return 0;
}

static void add_async_event(void)
{
	GalEvent event;
	struct EventList *tmp;

	if (wait_event_head) {
		e_thread_mutex_lock(&event_list_lock);
		tmp = wait_event_head;
		wait_event_head = tmp->next;
		tmp->next = free_event_list;
		free_event_list = tmp;
		event = tmp->event;
		e_thread_mutex_unlock(&event_list_lock);

		e_queue_write_try(event_queue, (ePointer)&event, sizeof(GalEvent));
	}
}

ebool egal_wait_event(GalEvent *event)
{
#ifdef WIN32
	if (!egal_get_event_from_queue(event)) {
		do {
			egal_window_get_event(event);
		} while (!egal_get_event_from_queue(event));
	}
#else
	while (!egal_get_event_from_queue(event));
#endif

	if (event->type == GAL_ET_QUIT)
		return efalse;

	add_async_event();

	return etrue;
}

void egal_add_event_to_queue(GalEvent *event)
{
	if (event_queue) {
		if (event->type == GAL_ET_MOUSEMOVE 
				|| event->type == GAL_ET_EXPOSE) {
			egal_add_async_event_to_queue(event);
		}
		else {
#ifdef WIN32
			e_queue_write_try(event_queue, (ePointer)event, sizeof(GalEvent));
#else
			e_queue_write_wait(event_queue, (ePointer)event, sizeof(GalEvent));
#endif
		}
	}
}

eint egal_get_event_from_queue(GalEvent *event)
{
	int n;
	GalEvent t;

	if (!event_queue)
		return 0;

#ifdef WIN32
	n = e_queue_read_try(event_queue, (ePointer)event, sizeof(GalEvent));
#else
	n = e_queue_read_wait(event_queue, (ePointer)event, sizeof(GalEvent));
#endif
	if (n > 0 && event->type == GAL_ET_EXPOSE) {
		while (e_queue_seek(event_queue, (ePointer)&t, 0, sizeof(GalEvent))) {
			if (t.type != event->type || event->window != t.window)
				break;
			if (event->type == GAL_ET_EXPOSE) {
				eint x1 = event->e.expose.rect.x + event->e.expose.rect.w - 1;
				eint y1 = event->e.expose.rect.y + event->e.expose.rect.h - 1;
				eint x2 = t.e.expose.rect.x + t.e.expose.rect.w - 1;
				eint y2 = t.e.expose.rect.y + t.e.expose.rect.h - 1;
				if (event->e.expose.rect.x <= t.e.expose.rect.x 
						&& event->e.expose.rect.y <= t.e.expose.rect.y
						&& x1 >= x2 && y1 >= y2) {
					e_queue_read(event_queue, (ePointer)&t, sizeof(GalEvent));
					continue;
				}
				if (event->e.expose.rect.x >= t.e.expose.rect.x
						&& event->e.expose.rect.y >= t.e.expose.rect.y
						&& x1 <= x2 && y1 <= y2) {
					e_queue_read(event_queue, (ePointer)event, sizeof(GalEvent));
					continue;
				}
			}
			else if (event->type == GAL_ET_WHEELFORWARD) {
				e_queue_read(event_queue, (ePointer)&t, sizeof(GalEvent));
			}
			else if (event->type == GAL_ET_WHEELBACKWARD) {
				e_queue_read(event_queue, (ePointer)&t, sizeof(GalEvent));
			}
			break;
		}
	}
	return n / sizeof(GalEvent);
}

void egal_add_async_event_to_queue(GalEvent *event)
{
	//eint semval;
	struct EventList *tmp;

	e_thread_mutex_lock(&event_list_lock);

	if (!free_event_list) {
		tmp = wait_event_head;
		wait_event_head = tmp->next;
	}
	else {
		tmp = free_event_list;
		free_event_list = tmp->next;
	}

	tmp->next = NULL;
	tmp->event = *event;
	if (wait_event_head) {
		wait_event_tail->next = tmp;
		wait_event_tail = tmp;
	}
	else {
		wait_event_head = tmp;
		wait_event_tail = tmp;
	}

	e_thread_mutex_unlock(&event_list_lock);

	//e_sem_getvalue(&event_list_sem, &semval);
	//if (semval < 1)
	//	e_sem_post(&event_list_sem);
}
