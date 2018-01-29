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

#ifdef linux
static ePointer wait_event_handler(ePointer data)
{
	GalEvent event;

	while (1) {
		if (egal_window_get_event(&event, is_recv) > 0)
			egal_add_event(&event);
	}

	return (ePointer)-1;
}
#endif

eint egal_init(eint argc, char *const args[])
{
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

	if (egal_window_init() < 0)
		return -1;
#ifdef linux
	e_thread_t pid;
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

static eint get_async_event(GalEvent *event)
{
	struct EventList *tmp;

	if (wait_event_head) {
		e_thread_mutex_lock(&event_list_lock);
		tmp = wait_event_head;
		wait_event_head = tmp->next;
		tmp->next = free_event_list;
		free_event_list = tmp;
		*event = tmp->event;
		e_thread_mutex_unlock(&event_list_lock);
		return 1;
	}
	return 0;
}

eint egal_wait_event(Queue *queue)
{
	GalEvent event;
	int n = 0;
#ifdef WIN32
		while (egal_window_get_event(NULL, 0) > 0) {
			while (egal_get_event(&event)) {
				if (!e_queue_write_try(queue, &event, sizeof(GalEvent)))
					return n;
				n++;
			}
		}
#else
	while (egal_get_event(&event) > 0) {
		if (!e_queue_write_try(queue, &event, sizeof(GalEvent)))
			return n;
		n++;
	}
#endif

	if (get_async_event(&event) && 
			e_queue_write_try(queue, &event, sizeof(GalEvent)) > 0)
		n++;

	if (n == 0) {
#ifdef WIN32
		Sleep(1);
#else
		usleep(1000);
#endif
		event.type = GAL_ET_TIMEOUT;
		event.window = 0;
		e_queue_write(queue, &event, sizeof(GalEvent));
		n = 1;	
	}

	return n;
}

void egal_add_event(GalEvent *event)
{
	while (!e_queue_write_try(event_queue, (ePointer)event, sizeof(GalEvent)))
		e_queue_throw(event_queue, sizeof(GalEvent));
}

eint egal_get_event(GalEvent *event)
{
	if (event_queue &&
			e_queue_read_try(event_queue, (ePointer)event, sizeof(GalEvent)) > 0) {
		return 1;
	}
	return 0;
}

eint egal_size_event(void)
{
	if (event_queue)
		return event_queue->len / sizeof(GalEvent);
	return 0;
}

void egal_add_async_event(GalEvent *event)
{
	struct EventList *tmp;

	if (event->type == GAL_ET_QUIT 
			|| event->type == GAL_ET_PRIVATE_SAFE) {
		egal_add_event(event);
		return;
	}

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
}
