#include "egal.h"

#define EVENT_LIST_MAX  20
static struct EventList {
	GalEvent event;
	struct EventList *next;	
} event_list_buf[EVENT_LIST_MAX];

static struct EventList *free_event_list = NULL;
static struct EventList *wait_event_head = NULL;
static struct EventList *wait_event_tail = NULL;
static e_pthread_mutex_t event_list_lock;
static e_sem_t event_list_sem;
static Queue  *event_queue = NULL;

static ePointer wait_event_handler(ePointer data)
{
	GalEvent event;

	while (egal_window_wait_event(&event))
		egal_add_event_to_queue(&event);

	return (ePointer)-1;
}

static ePointer wait_add_async_event(ePointer data)
{
	GalEvent event;

	while (1) {
		e_sem_wait(&event_list_sem);

		while (wait_event_head) {
			e_pthread_mutex_lock(&event_list_lock);
			struct EventList *tmp = wait_event_head;
			wait_event_head = tmp->next;
			tmp->next = free_event_list;
			free_event_list = tmp;
			event = tmp->event;
			e_pthread_mutex_unlock(&event_list_lock);

			egal_add_event_to_queue(&event);
		}
	}
	return (ePointer)-1;
}

eint egal_init(eint argc, char *const args[])
{
	pthread_t pid;
	eint i;

	e_sem_init(&event_list_sem, 0);
	e_pthread_mutex_init(&event_list_lock,  NULL);

	event_queue = e_queue_new(sizeof(GalEvent) * EVENT_QUEUE_MAX);
	if (!event_queue)
		return -1;

	for (i = 0; i < EVENT_LIST_MAX; i++) {
		event_list_buf[i].next = free_event_list;
		free_event_list = event_list_buf + i;
	}

	if (pthread_create(&pid, NULL, wait_add_async_event, NULL) < 0)
		return -1;

	if (egal_window_init() < 0)
		return -1;

	return 0;
}

eint egal_event_init(void)
{
	pthread_t pid;

	if (pthread_create(&pid, NULL, wait_event_handler, NULL) < 0)
		return -1;

	return 0;
}

bool egal_wait_event(GalEvent *event)
{
	egal_get_event_from_queue(event);

	if (event->type == GAL_ET_QUIT)
		return false;

	return true;
}

void egal_add_event_to_queue(GalEvent *event)
{
	if (event_queue)
		e_queue_write_wait(event_queue, (ePointer)event, sizeof(GalEvent));
}

void egal_get_event_from_queue(GalEvent *event)
{
	if (event_queue)
		e_queue_read_wait(event_queue, (ePointer)event, sizeof(GalEvent));
}

void egal_add_async_event_to_queue(GalEvent *event)
{
	eint semval;

	e_pthread_mutex_lock(&event_list_lock);
	if (free_event_list) {
		struct EventList *tmp = free_event_list;
		free_event_list = tmp->next;
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
	}
	e_pthread_mutex_unlock(&event_list_lock);

	e_sem_getvalue(&event_list_sem, &semval);
	if (semval < 1)
		e_sem_post(&event_list_sem);
}
